#include "DistributedHashTable.h"

#include "simulator/Log.h"
#include "net/Message.h"
#include "net/Transport.h"

#include <algorithm>
#include <set>
#include <memory>
#include <utility>
#include <unordered_map>

using namespace std;

using AddressList = vector<Address>;


class RingPartitioner {
    uint16_t replicationFactor;
    uint64_t ringSize;
public:

    RingPartitioner(uint16_t replicationFactor, uint64_t ringSize) {
        this->replicationFactor = replicationFactor;
        this->ringSize = ringSize;
    }

    int64_t addressHash(int32_t id, int16_t port) {
        return ((int64_t)id << 32) + port;
    }

    uint64_t getRingPos(const Address &addr) {
        uint64_t hash = (uint64_t)addressHash(addr.getIp(), addr.getPort());
        hash = (hash * 2654435761ul) >> 32;
        return hash % (ringSize);
    }

    uint64_t getRingPos(const string &key) {
        static std::hash<string> hashString;
        uint64_t hash = (hashString(key) * 2654435761ul) >> 32;
        return hash % (ringSize);
    }

    AddressList getNaturalNodes(const string &key, const AddressList &endpoints) {
        assert(endpoints.size() > 0);

        using HashIndexTuple= tuple<uint64_t, size_t>;
        enum {Hash, Index};
        auto cmpHashIndexTuple = [](HashIndexTuple a, HashIndexTuple b) {
            return get<Hash>(a) < get<Hash>(b);
        };

        // Create the ring
        auto hashesRing = vector<HashIndexTuple>();
        for (auto idx = 0ul; idx < endpoints.size(); ++idx) {
            hashesRing.push_back(make_tuple(getRingPos(endpoints[idx]), idx));
        }
        sort(hashesRing.begin(), hashesRing.end());

        // Find natural nodes (replica nodes) for key
        auto naturalNodes = AddressList();
        naturalNodes.reserve(replicationFactor);

        auto startNode = lower_bound(hashesRing.cbegin(), hashesRing.cend(),
                                     make_tuple(getRingPos(key), 0),
                                     cmpHashIndexTuple);
        auto node = startNode;
        do {
            if (node == hashesRing.end())
                node = hashesRing.begin();
            naturalNodes.push_back(endpoints[get<Index>(*node)]);
        } while (naturalNodes.size() < replicationFactor && ++node != startNode);

        return naturalNodes;
    }
};


/******************************************************************************
 * Commands
 ******************************************************************************/
class Command {
protected:
    using Message = dsproto::Message;

public:
    struct EndpointEntry {
        Address address;
        bool failed;
        bool responded;
    };

    vector<EndpointEntry> endpoints;
    Message msg;

public:
    uint16_t failRspCount       = 0;
    uint16_t successRspCount    = 0;
    uint16_t timeout            = 15;
    bool finished               = false;

public:
    Command() {};
    Command(vector<Address> &&addrList, Message &&msg) : msg(move(msg)) {
        endpoints.reserve(addrList.size());
        for (auto address : addrList) {
            endpoints.push_back(EndpointEntry{ move(address), false, false });
        }
    }

    const Message& getMessage() {
        return msg;
    }

    const vector<EndpointEntry>& getEndpoints() {
        return endpoints;
    }

    void multicast(shared_ptr<dsproto::MessageStream> msgStream) {
        for (auto &remote : endpoints) {
            msgStream->send(remote.address, msg);
        }
    }

    decltype(endpoints.begin()) getEntry(const Address &addr) {
        auto cmpEntry = [&addr](const EndpointEntry &entry) {
            return entry.address == addr;
        };
        return find_if(endpoints.begin(), endpoints.end(), cmpEntry);
    }

    void markEndpoindFailed(const Address &addr) {
        auto entryPos = getEntry(addr);
        if (entryPos != endpoints.end()) {
            entryPos->failed = true;
            failRspCount++;
        }
    }

    void markEndpointResponded(const Address &addr) {
        auto entryPos = getEntry(addr);
        if (entryPos != endpoints.end()) {
            entryPos->responded = true;
            successRspCount++;
        }
    }

    bool endpoitResponded(const Address &addr) {
        auto entryPos = getEntry(addr);
        if (entryPos == endpoints.end())
            return false;
        return entryPos->responded;
    }

    bool endpoitFailed(const Address &addr) {
        auto entryPos = getEntry(addr);
        if (entryPos == endpoints.end())
            return true;
        return entryPos->failed;
    }
};


class RingDHTBackend : public DHTBackend {
public:

    RingDHTBackend(shared_ptr<dsproto::MessageStream> msgStream,
                   MembershipProxy membershipProxy,
                   size_t replicationFactor, Log *log)
            : partitioner(replicationFactor, RING_SIZE) {
        this->replicationFactor = replicationFactor;
        this->clusterInfo = move(membershipProxy);
        this->log = log;
        this->msgStream = msgStream;
    }

    virtual ~RingDHTBackend() = default;

    AddressList getNaturalNodes(const string &key) override {
        return partitioner.getNaturalNodes(key, clusterInfo->getMembersList());
    }

    void updateCluster() override {
    }

    bool check(const Message &msg) override {
        auto msgTypes = set<uint8_t>{
            dsproto::CREATE, dsproto::READ, dsproto::UPDATE, dsproto::DELETE
        };
        return msgTypes.count(msg.getType()) > 0;
    }

    void handle(const Message &msg) override {
        auto localAddr = clusterInfo->getLocalAddres();
        auto remoteAddr = msg.getAddress();
        auto transaction = msg.getTransaction();
        auto key = msg.getKey();
        auto value = msg.getValue();

        if (msg.getType() == dsproto::CREATE) {
            if (log != nullptr) {
                log->logCreateSuccess(&remoteAddr, false, 0, key, value);
            }
            hashTable[key] = value;
            auto rsp = Message(dsproto::CREATE_RSP, localAddr);
            rsp.setTransaction(transaction);
            rsp.setKey(key);
            rsp.setStatus(dsproto::OK);
            msgStream->send(remoteAddr, rsp);
        }
        if (msg.getType() == dsproto::DELETE) {
            auto rsp = Message(dsproto::DELETE_RSP, localAddr);
            rsp.setTransaction(transaction);
            rsp.setKey(key);

            if (hashTable.count(key) == 0) {
                if (log != nullptr) {
                    log->logDeleteFail(&remoteAddr, false, 0, key);
                }
                rsp.setStatus(dsproto::FAIL);
            } else {
                if (log != nullptr) {
                    log->logDeleteSuccess(&remoteAddr, false, 0, key);
                }
                hashTable.erase(key);
            }
            msgStream->send(remoteAddr, rsp);
        }
        if (msg.getType() == dsproto::READ) {
            cout << msgStream->getLocalAddress().getAddress() << ": READ REQ BACKEND ";
            cout << "from " << msg.getAddress().getAddress() << endl;
            auto rsp = Message(dsproto::READ_RSP, localAddr);
            rsp.setTransaction(transaction);

            auto valueIterator = hashTable.find(key);
            if (valueIterator != hashTable.end()) {
                value = valueIterator->second;
                if (log != nullptr)
                    log->logReadSuccess(&localAddr, false, transaction, key, value);
                rsp.setStatus(dsproto::OK);
            } else {
                if (log != nullptr) {
                    log->logReadFail(&localAddr, false, transaction, key);
                }
                rsp.setStatus(dsproto::FAIL);
            }
            rsp.setKeyValue(key, value);
            msgStream->send(remoteAddr, rsp);
        }
        if (msg.getType() == dsproto::UPDATE) {
            auto rsp = Message(dsproto::UPDATE_RSP, localAddr);
            rsp.setTransaction(transaction);
            auto valueIterator = hashTable.find(key);
            if (valueIterator != hashTable.end()) {
                valueIterator->second = value;
                if (log != nullptr)
                    log->logUpdateSuccess(&localAddr, false, transaction, key, value);
                rsp.setStatus(dsproto::OK);
            } else {
                if (log != nullptr)
                    log->logUpdateFail(&localAddr, false, transaction, key, value);
                rsp.setStatus(dsproto::FAIL);
                rsp.setKeyValue(key, value);
                msgStream->send(remoteAddr, rsp);
            }
        }
    }

private:
    uint64_t                transaction = 0;
    size_t                  replicationFactor;
    MembershipProxy         clusterInfo;
    Log *log = nullptr;
    unordered_map<string, string> hashTable;
    shared_ptr<dsproto::MessageStream> msgStream;
    RingPartitioner partitioner;
};


class RingDHTCoordinator : public DHTCoordinator {
public:
    RingDHTCoordinator(shared_ptr<dsproto::MessageStream> msgStream,
            RingPartitioner partitioner, MembershipProxy membershipProxy,
            Log *log)
            : partitioner(partitioner) {
        this->membershipProxy = membershipProxy;
        this->msgStream = msgStream;
        this->log = log;
    }

    void create(string &&key, string &&value) override {
        auto msg = Message(dsproto::CREATE, msgStream->getLocalAddress());
        msg.setTransaction(++transaction);
        msg.setKeyValue(key, move(value));
        auto createCommand = Command(
            partitioner.getNaturalNodes(key, membershipProxy->getMembersList()),
            move(msg));
        execute(move(createCommand));
    }

    string read(const string &key) override {
        cout << msgStream->getLocalAddress().getAddress() << ": READ REQ\n";
        auto endpoints = partitioner.getNaturalNodes(key, membershipProxy->getMembersList());
        for (auto &addr : endpoints) {
            cout << "-> " << addr.getAddress() << endl;
        }

        auto msg = Message(dsproto::READ, msgStream->getLocalAddress());
        msg.setTransaction(++transaction);
        msg.setKey(key);
        auto readCommand = Command(
            partitioner.getNaturalNodes(key, membershipProxy->getMembersList()),
            move(msg));
        execute(move(readCommand));
        return string("");
    }

    void update(string &&key, string &&value) override {
        auto msg = Message(dsproto::UPDATE, msgStream->getLocalAddress());
        msg.setTransaction(++transaction);
        msg.setKeyValue(key, move(value));
        auto createCommand = Command(
            partitioner.getNaturalNodes(key,membershipProxy->getMembersList()),
            move(msg));
        execute(move(createCommand));
    }

    void remove(const string &key) override {
        auto msg = Message(dsproto::DELETE, msgStream->getLocalAddress());
        msg.setTransaction(++transaction);
        msg.setKey(key);
        auto removeCommand = Command(
            partitioner.getNaturalNodes(key, membershipProxy->getMembersList()),
            move(msg));
        execute(move(removeCommand));
    }

    bool check(Message &msg) override {
        auto msgTypes = set<uint8_t>{
            dsproto::READ_RSP, dsproto::DELETE_RSP, dsproto::CREATE_RSP, dsproto::UPDATE_RSP
        };
        return msgTypes.count(msg.getType()) > 0;
    }

    void handle(const Message &msg) override {
        auto addr = msgStream->getLocalAddress();
        auto reqTransaction = msg.getTransaction();
        auto commandIdentifier = make_pair(msg.getTransaction(), msg.getKey());
        responseCount[commandIdentifier]++;

        if (msg.getType() == dsproto::CREATE_RSP) {
            auto commandIterator = pendingCommands.find(reqTransaction);
            if (commandIterator != pendingCommands.end()) {
                auto command = commandIterator->second;
                auto reqKey = command.getMessage().getKey();
                auto reqValue = command.getMessage().getValue();
                auto peersCount = command.getEndpoints().size();
                if (responseCount[commandIdentifier] == peersCount) {
                    if (log != nullptr)
                        log->logCreateSuccess(&addr, true, 0, reqKey, reqValue);
                }
            }
        }
        if (msg.getType() == dsproto::DELETE_RSP) {
            if (log != nullptr && msg.getStatus() == dsproto::OK) {
                if (responseCount[commandIdentifier] == 3) {
                    log->logDeleteSuccess(&addr, true, 0, msg.getKey());
                }
            } else if (log != nullptr) {
                // TODO
                if (responseCount[commandIdentifier] == 3) {
                    log->logDeleteFail(&addr, true, 0, msg.getKey());
                }
            }
        }

        if (msg.getType() == dsproto::READ_RSP) {
            cout << msgStream->getLocalAddress().getAddress() << ": READ RSP "
                 << "from " << msg.getAddress().getAddress()
                 << " status: " << (msg.getStatus() == dsproto::OK) << endl;
            auto commandIterator = pendingCommands.find(reqTransaction);
            if (commandIterator != pendingCommands.end()) {
                auto &command = commandIterator->second;
                auto reqKey = command.getMessage().getKey();
                auto rspValue = msg.getValue();
                auto peersCount = command.getEndpoints().size();

                if (msg.getStatus() == dsproto::OK) {
                    cout << "OK \n";
                    command.successRspCount++;
                }

                if (command.successRspCount > peersCount/2 && !command.finished) {
                    cout << "XXX \n";
                    log->logReadSuccess(&addr, true, reqTransaction, reqKey, rspValue);
                    command.finished = true;
                } else if (command.successRspCount > peersCount/2 && !command.finished) {

                }
            }
        }

        if (msg.getType() == dsproto::UPDATE_RSP) {
            cout << "update\n";
            auto commandIterator = pendingCommands.find(reqTransaction);
            if (commandIterator != pendingCommands.end()) {
                auto &command = commandIterator->second;
                auto reqKey = command.getMessage().getKey();
                auto reqValue = command.getMessage().getValue();
                auto peersCount = command.getEndpoints().size();

                if (msg.getStatus() == dsproto::OK) {
                    command.successRspCount++;
                }

                if (command.successRspCount > peersCount/2 && !command.finished) {
                    log->logUpdateSuccess(&addr, true, reqTransaction, reqKey, reqValue);
                    command.finished = true;
                } else if (command.successRspCount > peersCount/2 && !command.finished) {

                }
            }
        }
    }

    void execute(Command&& command) {
        pendingCommands.emplace(transaction, move(command));
        pendingCommands[transaction].multicast(msgStream);
    }

    void onClusterUpdate() override {
        for (auto &hashCommandPair : pendingCommands) {
            auto &command = get<1>(hashCommandPair);
            if (command.finished)
            continue;

            auto addr = command.getMessage().getAddress();

            command.timeout--;
            if (command.timeout == 0) {
                if (command.getMessage().getType() == dsproto::READ) {
                    log->logReadFail(&addr, true,
                                     command.getMessage().getTransaction(),
                                     command.getMessage().getKey());
                    //TODO
                    command.finished = true;
                }
                if (command.getMessage().getType() == dsproto::UPDATE) {
                    log->logUpdateFail(&addr, true,
                                     command.getMessage().getTransaction(),
                                     command.getMessage().getKey(),
                                     command.getMessage().getValue());
                    //TODO
                    command.finished = true;
                }
            }
            // for (auto endpoint : command.getEndpoints()) {
            //     if (!endpoint.responded && !endpoint.failed) {
            //         // hasFailed(remoteAddr) && command.status[]) {
            //     //     command.failRspCount++;
            //     }
            // }
        }
    }

private:
    shared_ptr<dsproto::MessageStream>  msgStream;
    RingPartitioner partitioner;
    MembershipProxy membershipProxy;

    uint32_t transaction = 0;
    Log *log = nullptr;

    using PendingTransactionIdentifier = pair<uint32_t, string>;
    map<PendingTransactionIdentifier, uint32_t> responseCount;
    unordered_map<uint32_t, Command>            pendingCommands;
};


/******************************************************************************
 * Distributed store service implementation
 ******************************************************************************/
DistributedHashTableService::DistributedHashTableService(
        MembershipProxy membershipProxy,
        shared_ptr<dsproto::MessageStream> msgStream,
        Log *log) {
    static const size_t REPLICATION_FACTOR = 3;

    this->msgStream = msgStream;

    auto *dhtBacked = new (std::nothrow) RingDHTBackend(
        msgStream, membershipProxy, REPLICATION_FACTOR, log);
    backend = shared_ptr<DHTBackend>(dhtBacked);

    auto *dhtCordinator = new RingDHTCoordinator(
        msgStream,
        RingPartitioner(REPLICATION_FACTOR, RING_SIZE),
        membershipProxy, log);
    coordinator = unique_ptr<DHTCoordinator>(dhtCordinator);

    this->log = log;
}

void DistributedHashTableService::create(string &&key, string &&value) {
    coordinator->create(move(key), move(value));
}

void DistributedHashTableService::read(const string &key) {
    coordinator->read(key);
}

void DistributedHashTableService::update(string &&key, string &&value) {
    coordinator->update(move(key), move(value));
}

void DistributedHashTableService::remove(const string &key) {
    coordinator->remove(key);
}

bool DistributedHashTableService::recieveMessages() {
    return msgStream->recieveMessages();
}

bool DistributedHashTableService::processMessages() {
    while (!msgStream->empty()) {
        auto msg = msgStream->dequeue();
        if (backend->check(msg)) {
            backend->handle(msg);
        }
        if (coordinator->check(msg)) {
            coordinator->handle(msg);
        }
    }
    return true;
}

AddressList DistributedHashTableService::getNaturalNodes(const string &key) {
    return backend->getNaturalNodes(key);
}

void DistributedHashTableService::updateCluster() {
    backend->updateCluster();
    coordinator->onClusterUpdate();
}
