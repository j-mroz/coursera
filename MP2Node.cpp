#include "MP2Node.h"
#include "Transport.h"
#include <algorithm>
#include <future>
#include <set>
#include <utility>
#include <unordered_map>


namespace dsproto {

class MessageStream {
    using Msg = Message;
public:
    MessageStream(shared_ptr<net::Transport> net) : transport(net) {
        addr = transport->getAddress();
    }

    void send(Address remote, Msg &msg) {
        auto msgbuff = msg.serialize();
        transport->send(remote, msgbuff.data(), msgbuff.size());
    }

    bool recieveMessages() {
        transport->drain();
    }

    Msg dequeue() {
        auto buf = transport->recieve();
        auto msg = Msg::deserialize((char *)buf.data, buf.size);
        free(buf.data);
        return msg;
    }

    Address getLocalAddress() {
        return addr;
    }

    bool empty() {
        return !transport->pollnb();
    }

private:
    Address                     addr;
    shared_ptr<net::Transport>  transport;
    int32_t                     transaction = 0;
};

}

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


// class VirtualSynchronyCreateCommand {
//     vector<Address> endpoints;
//     string key;
//     string val;
// public:
//     void execute(dsproto::MessageStream &msgStream) {
//         for (auto &remote : endpoints) {
//             msgStream.sendCreate(remote, move(key), move(val));
//         }
//     }
// };

//
// class VirtualSynchronyView {
//     using Message = dsproto::Message;
//
//     class Member {
//         Address endpoint;
//         bool isActive;
//         uint64_t clock;
//         vector<Message> msgs;
//     public:
//
//     };
//
//     using Members = unordered_map<uint64_t, Member>;
// public:
//
//     // void multicast(const vector<Address> &remotes, const Message &msg) {
//     //
//     // }
//
//     SendCommand createViewChange() {
//         auto endpoints = vector<Address>();
//         // TODO
//         auto msg = Message(dsproto::VIEW_CHANGE, Address());
//         return SendCommand(move(endpoints), move(msg));
//     }
//
//     void installViewChange() {
//     }
//
//     void sendUnstableMsgs() {
//
//     }
//
//     void flush() {
//     }
//
//     void handle(const Message& msg) {
//         auto isViewChange = false;
//
//         if (isViewChange) {
//             sendUnstableMsgs();
//             flush();
//         }
//     }
//
// private:
//     // vector<Address> remotes;
//     Members members;
// };


static int64_t addressHash(int32_t id, int16_t port) {
    return ((int64_t)id << 32) + port;
}


class RingClusterInfo {
public:
    using MembersList = vector<MemberListEntry>;

    RingClusterInfo(shared_ptr<Member> member) {
        this->member = member;
    }

    MembersList& getMembersList() {
        return member->memberList;
    }

    Address getLocalAddres() {
        return member->addr;
    }

private:
    shared_ptr<Member> member;
};


/******************************************************************************
 * Replication strategy - responsible for backend jobs
 ******************************************************************************/
class ReplicationStrategy {
public:
    using MembersList = vector<MemberListEntry>;
    using AddressList = vector<Address>;
    using Message = dsproto::Message;

    ReplicationStrategy()          = default;
    virtual ~ReplicationStrategy() = default;
    virtual AddressList getNaturalNodes(const string&)  = 0;
    virtual void updateCluster()                        = 0;
    virtual bool check(const Message &msg)              = 0;
    virtual void handle(const Message &msg)             = 0;
};


template<typename ClusterInfo>
class SimpleReplicationStrategy : public ReplicationStrategy {
public:

    SimpleReplicationStrategy(shared_ptr<dsproto::MessageStream> msgStream,
                              unique_ptr<ClusterInfo> &&clusterInfo,
                              size_t replicationFactor, Log *log) {
        this->replicationFactor = replicationFactor;
        this->clusterInfo = move(clusterInfo);
        this->log = log;
        this->msgStream = msgStream;
    }

    virtual ~SimpleReplicationStrategy() = default;

    AddressList getNaturalNodes(const string &key) override {
        auto &members = clusterInfo->getMembersList();
        assert(members.size() > 0);

        // Create the ring
        auto membersIds = vector<uint64_t>();
        for (auto &entry : members) {
            auto addr = Address(entry.id, entry.port);
            membersIds.push_back(getRingPos(addr));
        }
        sort(membersIds.begin(), membersIds.end());

        // Find the first node on the ring
        auto keyRingPos = getRingPos(key);
        auto startNode = lower_bound(membersIds.cbegin(), membersIds.cend(),
                                     keyRingPos);

        // Find natural nodes (replica nodes) for key
        auto naturalNodes = AddressList();
        naturalNodes.reserve(replicationFactor);
        auto node = startNode;
        do {
            if (node == membersIds.end())
                node = membersIds.begin();
            if (ringPosToAddress.count(*node) == 0)
                assert(false);
            naturalNodes.push_back(ringPosToAddress[*node]);
        } while (naturalNodes.size() < replicationFactor && ++node != startNode);

        return naturalNodes;
    }

    void updateCluster() override {
        // auto activeIds = vector<uint64_t>();
        for (auto &entry : clusterInfo->getMembersList()) {
            auto addr = Address(entry.id, entry.port);
            auto ringPos = getRingPos(addr);
            ringPosToAddress[ringPos] = addr;
            // activeIds.push_back(ringPos);
        }
        // sort(activeIds.begin(), activeIds.end());
        //
        // auto failedNodes = vector<uint64_t>();
        // failedNodes.reserve(activeIds.size() / 2 );
        // set_difference(currentMembersIndices.cbegin(), currentMembersIndices.cend(),
        //                activeIds.cbegin(), activeIds.cend(),
        //                back_inserter(failedNodes));
        // if (failedNodes.size() > 0) {
        //     printf("have %d failed\n", failedNodes.size());
        // }
        // currentMembersIndices = move(activeIds);
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
            cout << "READ REQ BACKEND\n";
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
    static uint64_t getRingPos(const Address &addr) {
        uint64_t hash = (uint64_t)addressHash(addr.getIp(), addr.getPort());
        hash = (hash * 2654435761ul) >> 32;
        return hash % (RING_SIZE);
    }

    static uint64_t getRingPos(const string &key) {
        static std::hash<string> hashString;
        uint64_t hash = (hashString(key) * 2654435761ul) >> 32;
        return hash % (RING_SIZE);
    }

private:
    uint64_t                transaction = 0;
    size_t                  replicationFactor;
    unique_ptr<ClusterInfo> clusterInfo;
    vector<uint64_t>        currentMembersIndices;
    unordered_map<uint64_t, Address> ringPosToAddress;
    Log *log = nullptr;
    unordered_map<string, string> hashTable;
    shared_ptr<dsproto::MessageStream> msgStream;
};


/******************************************************************************
 * Coordination strategy - used for clients coordination
 ******************************************************************************/
class CoordinationStrategy {
protected:
    using Message = dsproto::Message;

public:
    CoordinationStrategy()          = default;
    virtual ~CoordinationStrategy() = default;

    virtual void   create(string &&key, string &&value) = 0;
    virtual string read(const string &key)              = 0;
    virtual void   update(string &&key, string &&value) = 0;
    virtual void   remove(const string &key)            = 0;
    virtual bool   check(Message &msg)                  = 0;
    virtual void   handle(const Message &msg)           = 0;
    virtual void   updateCluster()                      = 0;
};


class SimpleCoordinationStrategy : public CoordinationStrategy {
    using Message = dsproto::Message;

public:
    SimpleCoordinationStrategy(shared_ptr<dsproto::MessageStream> msgStream,
                shared_ptr<ReplicationStrategy> replicator,
                Log *log) {
        this->msgStream = msgStream;
        this->replicator = replicator;
        this->log = log;
    }

    void create(string &&key, string &&value) override {
        auto msg = Message(dsproto::CREATE, msgStream->getLocalAddress());
        msg.setTransaction(++transaction);
        msg.setKeyValue(key, move(value));
        auto createCommand = Command(replicator->getNaturalNodes(key),
                                     move(msg));
        execute(move(createCommand));
    }

    string read(const string &key) override {
        cout << "READ REQ\n";
        auto msg = Message(dsproto::READ, msgStream->getLocalAddress());
        msg.setTransaction(++transaction);
        msg.setKey(key);
        auto readCommand = Command(replicator->getNaturalNodes(key), move(msg));
        execute(move(readCommand));
    }

    void update(string &&key, string &&value) override {
        auto msg = Message(dsproto::UPDATE, msgStream->getLocalAddress());
        msg.setTransaction(++transaction);
        msg.setKeyValue(key, move(value));
        auto createCommand = Command(replicator->getNaturalNodes(key),
                                     move(msg));
        execute(move(createCommand));
    }

    void remove(const string &key) override {
        auto msg = Message(dsproto::DELETE, msgStream->getLocalAddress());
        msg.setTransaction(++transaction);
        msg.setKey(key);
        auto removeCommand = Command(replicator->getNaturalNodes(key), move(msg));
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
            cout << "READ RSP \n";
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

    void updateCluster() override {
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
    shared_ptr<ReplicationStrategy>     replicator;
    uint32_t transaction = 0;
    Log *log = nullptr;

    using PendingTransactionIdentifier = pair<uint32_t, string>;
    map<PendingTransactionIdentifier, uint32_t> responseCount;
    unordered_map<uint32_t, Command>            pendingCommands;
};


/******************************************************************************
 * Distributed store service implementation
 ******************************************************************************/
class DSService {
    friend class DSNode;

public:
    DSService(shared_ptr<Member> member, shared_ptr<dsproto::MessageStream> msgStream,
              Log *log) {
        this->member = member;
        this->msgStream = msgStream;

        using MyReplicationStrategy = SimpleReplicationStrategy<RingClusterInfo>;
        using MyCoordinationStrategy = SimpleCoordinationStrategy;

        static const size_t REPLICATION_FACTOR = 3;

        auto clusterInfo = unique_ptr<RingClusterInfo>(
                new RingClusterInfo(member));
        replicator = make_shared<MyReplicationStrategy>(
                msgStream, move(clusterInfo), REPLICATION_FACTOR, log);
        coordinator = unique_ptr<CoordinationStrategy>(
                new MyCoordinationStrategy(msgStream, replicator, log));
        this->log = log;
    }

    void create(string &&key, string &&value) {
        coordinator->create(move(key), move(value));
    }

    void read(const string &key) {
        coordinator->read(key);
    }

    void update(string &&key, string &&value) {
        coordinator->update(move(key), move(value));
    }

    void remove(const string &key) {
        coordinator->remove(key);
    }

    bool recieveMessages() {
        return msgStream->recieveMessages();
    }

    bool processMessages() {
        while (!msgStream->empty()) {
            auto msg = msgStream->dequeue();
            if (replicator->check(msg)) {
                replicator->handle(msg);
            }
            if (coordinator->check(msg)) {
                coordinator->handle(msg);
            }
        }
        return true;
    }

    vector<Node> getNaturalNodes(const string &key) {
        auto addrsList = replicator->getNaturalNodes(key);
        auto result = vector<Node>(addrsList.size());
        auto addrToNode = [](const Address &addr) {
            auto node = Node();
            node.setAddress(addr);
            return node;
        };
        transform(addrsList.begin(), addrsList.end(), result.begin(), addrToNode);

        return result;
    }

    void updateCluster() {
        replicator->updateCluster();
        coordinator->updateCluster();
    }


private:
    shared_ptr<Member>                  member;
    shared_ptr<dsproto::MessageStream>  msgStream;
    shared_ptr<ReplicationStrategy>     replicator;
    unique_ptr<CoordinationStrategy>    coordinator;
    Log                                 *log;
};


/**
 * Constructs default implementation
 */
DSNode::DSNode(shared_ptr<Member> member, Params*,
               EmulNet *emulNet, Log *log, Address *local) {
    auto transport = make_shared<net::Transport>(emulNet, &member->mp2q, *local);
    auto msgStream = make_shared<dsproto::MessageStream>(transport);
    this->impl = unique_ptr<DSService>(new DSService(member, msgStream, log));
}

/**
 * Dependency injection constructor
 */
DSNode::DSNode(DSService* impl) {
    this->impl = unique_ptr<DSService>(impl);
}

DSNode::~DSNode() {
}

/* Store Node Pimpl dispatchers */
void DSNode::clientCreate(string key, string value) {
    return impl->create(move(key), move(value));
}

void DSNode::clientRead(const string &key) {
    return impl->read(key);
}

void DSNode::clientUpdate(string key, string value) {
    return impl->update(move(key), move(value));
}

void DSNode::clientDelete(const string &key) {
    return impl->remove(key);
}

bool DSNode::recvLoop() {
    return impl->recieveMessages();
}

void DSNode::checkMessages() {
    impl->processMessages();
}

vector<Node> DSNode::findNodes(const string &key) {
    return impl->getNaturalNodes(key);
}

void DSNode::updateRing() {
    impl->updateCluster();
}

Member* DSNode::getMemberNode() {
    return impl->member.get();
}
/* Store Node Pimpl dispatchers end */
