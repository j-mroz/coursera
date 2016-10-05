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

struct RingNode {
    uint64_t rangeBegin;
    uint64_t rangeEnd;
    size_t   index;
};

bool operator<(const RingNode &a, const RingNode &b) {
    return a.rangeEnd < b.rangeEnd;
};


class RingPartitioner {
    uint16_t replicationFactor;
    uint64_t ringSize;
    vector<RingNode> ring;

public:
    RingPartitioner(uint16_t replicationFactor, uint64_t ringSize) {
        this->replicationFactor = replicationFactor;
        this->ringSize = ringSize;
    }

    uint16_t getReplicationFactor() {
        return replicationFactor;
    }

    uint64_t getRingPos(const Address &addr) {
        uint64_t hash = ((int64_t)addr.getIp() << 32) + addr.getPort();
        hash = (hash * 2654435761ul) >> 32;
        return hash % (ringSize);
    }

    uint64_t getRingPos(const string &key) {
        static std::hash<string> hashString;
        uint64_t hash = (hashString(key) * 2654435761ul) >> 32;
        return hash % (ringSize);
    }

    using ReplicaSet = tuple<vector<RingNode>, size_t>;

    // Returns replica set centered around addr
    ReplicaSet getReplicaSet(const Address &addr) {
        auto addrRingNode = RingNode{ 0, getRingPos(addr), 0 };
        auto addrRingNodeIter = lower_bound(ring.cbegin(), ring.cend(), addrRingNode);
        auto first = addrRingNodeIter;
        auto last = addrRingNodeIter;
        assert(first != ring.end());

        for (auto sibling = 1ul; sibling < replicationFactor; ++sibling) {
            if (previous(first) == last) {
                break;
            }
            first = previous(first);
            if (first == next(last)) {
                break;
            }
            last = next(last);
        }

        vector<RingNode> replicaNodes;
        replicaNodes.reserve(replicationFactor*2);
        auto replicatorIdx = 0ul;
        while (true) {
            replicaNodes.push_back(*first);
            if (first == addrRingNodeIter) {
                replicatorIdx = replicaNodes.size() - 1;
            }
            if (first == last) {
                break;
            }
            first = next(first);
        }

        return tie(replicaNodes, replicatorIdx);
    }

    template<typename Iter>
    Iter next(Iter iter) {
        ++iter;
        if (iter == ring.end())
            iter = ring.begin();
        return iter;
    }

    template<typename Iter>
    Iter previous(Iter iter) {
        if (iter == ring.begin())
            iter = ring.end();
        return --iter;
    }

    AddressList getNaturalNodes(const string &key, const AddressList &endpoints) {
        return getNaturalNodes(getRingPos(key), endpoints);
    }

    AddressList getNaturalNodes(const Address &addr, const AddressList &endpoints) {
        return getNaturalNodes(getRingPos(addr), endpoints);
    }

    void updateRing(const AddressList &endpoints) {
        assert(endpoints.size() > 0);
        ring.resize(endpoints.size());
        for (auto idx = 0ul; idx < endpoints.size(); ++idx) {
            auto nextIdx = (idx + 1) % ring.size();
            auto ringPos = getRingPos(endpoints[idx]);
            ring[nextIdx].rangeBegin = ringPos;
            ring[idx].rangeEnd = ringPos;
            ring[idx].index = idx;
        }
        sort(ring.begin(), ring.end());
    }

    AddressList getNaturalNodes(uint64_t ringPos, const AddressList &endpoints) {
        if (endpoints.size() == 0)
            return AddressList();

        // Find natural nodes (replica nodes) for key
        auto naturalNodes = AddressList();
        naturalNodes.reserve(replicationFactor);

        auto wantedNode = RingNode{ 0, /*.rangeEnd=*/ringPos, 0};
        auto startNode = lower_bound(ring.cbegin(), ring.cend(), wantedNode);
        auto node = startNode;
        do {
            if (node == ring.end())
                node = ring.begin();
            naturalNodes.push_back(endpoints[node->index]);
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
        Message rsp;
    };

    vector<EndpointEntry> endpoints;
    vector<Message>       endpoinsRsp;
    Message req;



private:
    uint16_t rspCount           = 0;
    uint16_t failRspCount       = 0;
    uint16_t successRspCount    = 0;
    uint32_t timeout            = 10;
    bool finished               = false;

public:
    Command() {};
    Command(vector<Address> &&addrList, Message &&msg) : req(move(msg)) {
        endpoints.reserve(addrList.size());
        for (auto address : addrList) {
            endpoints.push_back(EndpointEntry{ move(address), false, false, Message() });
        }
    }

    const Message& getRequest() {
        return req;
    }

    const vector<EndpointEntry>& getEndpoints() {
        return endpoints;
    }

    void multicast(shared_ptr<dsproto::MessageStream> msgStream) {
        for (auto &remote : endpoints) {
            msgStream->send(remote.address, req);
        }
    }

    void addResponse(Message rsp) {
        auto entryPos = getEntry(rsp.getAddress());
        if (entryPos == endpoints.end()) {
            return;
        }
        entryPos->responded = true;
        rspCount++;
        if (rsp.getStatus() == dsproto::OK) {
            successRspCount++;
        } else {
            failRspCount++;
            entryPos->failed = true;
        }
        entryPos->rsp = move(rsp);
    }

    const Message& getFirstSuccesRsp() {
        for (auto &remote : endpoints) {
            if (remote.responded && !remote.failed) {
                return remote.rsp;
            }
        }
        return req; //TODO whatto return?
    }

    auto getEntry(const Address &addr) -> decltype(endpoints.begin()) {
        auto cmpEntry = [&addr](const EndpointEntry &entry) {
            return entry.address == addr;
        };
        return find_if(endpoints.begin(), endpoints.end(), cmpEntry);
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

    uint16_t getRspCount() {
        return rspCount;
    }

    uint16_t getFailRspCount() {
        return failRspCount;
    }

    uint16_t getSuccessRspCount() {
        return successRspCount;
    }

    void finish() {
        finished = true;
    }

    bool hasFinished() {
        return finished;
    }

    void updateTimeLeft() {
        if (timeout == 0)
            return;
        timeout--;
    }

    uint32_t getTimeLeft() {
        return timeout;
    }
};


class CommandLogger {
    Log *log;
    Address localAddr;
    bool isCoordinator;

public:
    CommandLogger(Log *log, Address addr, bool isCoordinator) {
        this->log = log;
        this->localAddr = addr;
        this->isCoordinator = isCoordinator;
    }

    void logSuccess(const Message &req, const Message &rsp) {
        if (log == nullptr)
            return;

        auto &reqKey    = req.getKey();
        auto &reqValue  = req.getValue();
        auto &rspKey    = rsp.getKey();
        auto &rspValue  = rsp.getValue();
        auto transaction = req.getTransaction();

        switch (req.getType()) {
        case dsproto::CREATE:
            log->logCreateSuccess(&localAddr, isCoordinator, 0, reqKey, reqValue);
            break;
        case dsproto::READ:
            log->logReadSuccess(&localAddr, isCoordinator, transaction, rspKey, rspValue);
            break;
        case dsproto::UPDATE:
            log->logUpdateSuccess(&localAddr, isCoordinator, transaction, reqKey, reqValue);
            break;
        case dsproto::DELETE:
            log->logDeleteSuccess(&localAddr, isCoordinator, 0, reqKey);
            break;
        default:
            break;
        }
    }

    void logFailure(const Message &req) {
        if (log == nullptr)
            return;

        auto &reqKey = req.getKey();
        auto &reqValue = req.getValue();
        auto transaction = req.getTransaction();

        switch (req.getType()) {
        case dsproto::CREATE:
            log->logCreateFail(&localAddr, isCoordinator, transaction, reqKey, reqKey);
            break;
        case dsproto::READ:
            log->logReadFail(&localAddr, isCoordinator, transaction, reqKey);
            break;
        case dsproto::UPDATE:
            log->logUpdateFail(&localAddr, isCoordinator, transaction, reqKey, reqValue);
            break;
        case dsproto::DELETE:
            log->logDeleteFail(&localAddr, isCoordinator, transaction, reqKey);
            break;
        default:
            break;
        }
    }
};


class RingDHTBackend : public DHTBackend {
public:
    RingDHTBackend(shared_ptr<dsproto::MessageStream> msgStream,
                   MembershipProxy membershipProxy,
                   size_t replicationFactor, Log *log)
        : partitioner(replicationFactor, RING_SIZE),
          requestsLoger(log, membershipProxy->getLocalAddress(), false) {
        this->thisNodeAddr = membershipProxy->getLocalAddress();
        this->membershipProxy = move(membershipProxy);
        this->msgStream = msgStream;
    }

    virtual ~RingDHTBackend() = default;

    AddressList getNaturalNodes(const string &key) override {
        auto members = membershipProxy->getMembersList();
        partitioner.updateRing(members);
        return partitioner.getNaturalNodes(key, members);
    }

    void updateCluster() override {
        auto members = membershipProxy->getMembersList();
        if (members.size() <= 1) {
            return;
        }

        //TODO this is inefficent, implement listener callbacks on members and timestamp
        partitioner.updateRing(members);

        auto naturalNodes = partitioner.getNaturalNodes(thisNodeAddr, members);
        assert(naturalNodes.size() > 1 && naturalNodes[0] == thisNodeAddr);

        if (nextNodeAddr == naturalNodes[1]) {
            return;
        }
        nextNodeAddr = naturalNodes[1];

        if (hashTable.size() == 0)
            return;

        auto replicaNodes = vector<RingNode>();
        auto replicatorIdx = size_t(0);
        tie(replicaNodes, replicatorIdx) = partitioner.getReplicaSet(thisNodeAddr);
        sync(replicaNodes, replicatorIdx);
    }

    void sync(const vector<RingNode> replicaNodes, size_t replicatorIdx) {
        if (!(replicatorIdx < replicaNodes.size())) {
            return;
        }
        auto syncMsgs = vector<Message>(replicaNodes.size() - replicatorIdx - 1,
                                        Message(dsproto::SYNC_BEGIN, thisNodeAddr));
        ++transaction;
        for (auto &msg : syncMsgs) {
            msg.setTransaction(transaction);
        }

        for (auto &kv : hashTable) {
            auto &key = kv.first;
            auto &value  = kv.second;
            auto ringPos = partitioner.getRingPos(key);

            for (auto i = 0ul; i < syncMsgs.size(); ++i) {
                if (ringPos > replicaNodes[i].rangeBegin ||
                    replicaNodes[i].rangeEnd < replicaNodes[i].rangeBegin) {
                    syncMsgs[i].addSyncKeyValue(key, value);
                }
            }
        }
        // TODO again calling getMembersList??
        auto members = membershipProxy->getMembersList();
        for (auto i = 0ul; i < syncMsgs.size(); ++i) {
            msgStream->send(members[replicaNodes[replicatorIdx + 1 + i].index],
                            syncMsgs[i]);
        }
    }

    bool probe(const Message &msg) override {
        static auto msgTypes = set<uint8_t>{
            dsproto::CREATE, dsproto::READ, dsproto::UPDATE, dsproto::DELETE,
            dsproto::SYNC_BEGIN
        };
        return msgTypes.count(msg.getType()) > 0;
    }

    void handle(Message &msg) override {
        if (msg.getType() == dsproto::CREATE) {
            handleCreateRequest(msg);
        } else if (msg.getType() == dsproto::READ) {
            hadleReadRequest(msg);
        } else if (msg.getType() == dsproto::UPDATE) {
            handleUpdateRequest(msg);
        } else if (msg.getType() == dsproto::DELETE) {
            handleDeleteRequest(msg);
        } else if (msg.getType() == dsproto::SYNC_BEGIN) {
            handleSync(msg);
        }
    }

    void handleCreateRequest(Message &req) {
        auto key = req.getKey();
        auto rsp = Message(dsproto::CREATE_RSP, thisNodeAddr);
        rsp.setTransaction(req.getTransaction());

        if (hashTable.count(key)) {
            rsp.setStatus(dsproto::FAIL);
            requestsLoger.logFailure(req);
        } else {
            hashTable[key] = req.getValue();
            rsp.setKey(key);
            rsp.setStatus(dsproto::OK);
            requestsLoger.logSuccess(req, rsp);
        }

        msgStream->send(req.getAddress(), rsp);
    }

    void hadleReadRequest(Message &req) {
        auto key = req.getKey();
        auto rsp = Message(dsproto::READ_RSP, thisNodeAddr);
        rsp.setTransaction(req.getTransaction());

        auto valueIterator = hashTable.find(key);
        if (valueIterator != hashTable.end()) {
            auto value = valueIterator->second;
            rsp.setKeyValue(key, value);
            rsp.setStatus(dsproto::OK);
            requestsLoger.logSuccess(req, rsp);
        } else {
            rsp.setStatus(dsproto::FAIL);
            requestsLoger.logFailure(req);
        }
        msgStream->send(req.getAddress(), rsp);
    }

    void handleUpdateRequest(Message &req) {
        auto key = req.getKey();
        auto rsp = Message(dsproto::UPDATE_RSP, thisNodeAddr);
        rsp.setTransaction(req.getTransaction());
        rsp.setKey(key);

        auto valueIterator = hashTable.find(key);
        if (valueIterator != hashTable.end()) {
            valueIterator->second = req.getValue();
            rsp.setStatus(dsproto::OK);
            requestsLoger.logSuccess(req, rsp);
        } else {
            rsp.setStatus(dsproto::FAIL);
            requestsLoger.logFailure(req);
        }
        msgStream->send(req.getAddress(), rsp);
    }

    void handleDeleteRequest(Message &req) {
        auto key = req.getKey();
        auto rsp = Message(dsproto::DELETE_RSP, thisNodeAddr);
        rsp.setTransaction(req.getTransaction());
        rsp.setKey(key);

        if (hashTable.count(key) == 0) {
            rsp.setStatus(dsproto::FAIL);
            requestsLoger.logFailure(req);
        } else {
            hashTable.erase(key);
            requestsLoger.logSuccess(req, rsp);
        }
        msgStream->send(req.getAddress(), rsp);
    }

    void handleSync(Message &msg) {
        for (auto &tuple : msg.getSyncKeyValues()) {
            auto kv = make_pair(move(get<0>(tuple)), move(get<1>(tuple)));
            hashTable.insert(kv);
            assert(hashTable.count(kv.first) != 0);
        }
    }

private:
    using MsgStreamPtr = shared_ptr<dsproto::MessageStream>;
    using HashTable = unordered_map<string, string>;

    uint64_t            transaction = 0;
    size_t              replicationFactor;
    Address             thisNodeAddr;
    Address             nextNodeAddr;
    MembershipProxy     membershipProxy;
    RingPartitioner     partitioner;
    HashTable           hashTable;
    MsgStreamPtr        msgStream;
    CommandLogger       requestsLoger;
};


class RingDHTCoordinator : public DHTCoordinator {
public:
    RingDHTCoordinator(shared_ptr<dsproto::MessageStream> msgStream,
            RingPartitioner partitioner, MembershipProxy membershipProxy,
            Log *log)
                : partitioner(partitioner),
                  requestsLoger(log, msgStream->getLocalAddress(), true) {
        this->membershipProxy = membershipProxy;
        this->msgStream = msgStream;
    }

    void create(string &&key, string &&value) override {
        auto msg = Message(dsproto::CREATE, msgStream->getLocalAddress());
        msg.setTransaction(++transaction);
        msg.setKeyValue(key, move(value));
        auto createCommand = Command(getNaturalNodes(key), move(msg));
        execute(move(createCommand));
    }

    string read(const string &key) override {
        auto msg = Message(dsproto::READ, msgStream->getLocalAddress());
        msg.setTransaction(++transaction);
        msg.setKey(key);
        auto readCommand = Command(getNaturalNodes(key), move(msg));
        execute(move(readCommand));

        //TODO
        return string("");
    }

    void update(string &&key, string &&value) override {
        auto msg = Message(dsproto::UPDATE, msgStream->getLocalAddress());
        msg.setTransaction(++transaction);
        msg.setKeyValue(key, move(value));
        auto createCommand = Command(getNaturalNodes(key), move(msg));
        execute(move(createCommand));
    }

    void remove(const string &key) override {
        auto msg = Message(dsproto::DELETE, msgStream->getLocalAddress());
        msg.setTransaction(++transaction);
        msg.setKey(key);
        auto removeCommand = Command(getNaturalNodes(key), move(msg));
        execute(move(removeCommand));
    }

    bool probe(Message &msg) override {
        static auto msgTypes = set<uint8_t>{
            dsproto::READ_RSP, dsproto::DELETE_RSP, dsproto::CREATE_RSP, dsproto::UPDATE_RSP
        };
        return msgTypes.count(msg.getType()) > 0;
    }

    void handle(Message &msg) override {
        if (msg.getType() == dsproto::CREATE_RSP) {
            handleCreateResponse(msg);
        } else if (msg.getType() == dsproto::DELETE_RSP) {
            handleDeleteResponse(msg);
        } else if (msg.getType() == dsproto::READ_RSP) {
            handleReadResponse(msg);
        } else if (msg.getType() == dsproto::UPDATE_RSP) {
            handleUpdate(msg);
        }
    }

    void handleCreateResponse(Message &msg) {
        auto reqTransaction = msg.getTransaction();
        auto commandIterator = pendingCommands.find(reqTransaction);

        if (commandIterator == pendingCommands.end())
            return;

        auto &command = commandIterator->second;
        auto endpointsCount = command.getEndpoints().size();

        command.addResponse(move(msg));

        if (command.getSuccessRspCount() == endpointsCount) {
            requestsLoger.logSuccess(command.getRequest(),
                                     command.getFirstSuccesRsp());
            command.finish();
        }
    }

    void handleReadResponse(Message &msg) {
        auto reqTransaction = msg.getTransaction();
        auto commandIterator = pendingCommands.find(reqTransaction);
        if (commandIterator == pendingCommands.end())
            return;

        auto &command = commandIterator->second;
        auto reqKey = command.getRequest().getKey();
        auto rspValue = msg.getValue();
        auto quromMin = command.getEndpoints().size()/2 + 1;

        command.addResponse(move(msg));

        if (command.hasFinished())
            return;

        if (command.getSuccessRspCount() >= quromMin) {
            requestsLoger.logSuccess(command.getRequest(),
                                     command.getFirstSuccesRsp());
            command.finish();
        } else if (command.getFailRspCount() >= quromMin) {
            requestsLoger.logFailure(command.getRequest());
            command.finish();
        }
    }

    void handleUpdate(const Message &msg) {
        auto reqTransaction = msg.getTransaction();
        auto commandIterator = pendingCommands.find(reqTransaction);
        if (commandIterator == pendingCommands.end())
            return;

        auto &command = commandIterator->second;
        command.addResponse(move(msg));

        auto quorumMin = command.getEndpoints().size()/2 + 1;
        if (command.getSuccessRspCount() >= quorumMin && !command.hasFinished()) {
            requestsLoger.logSuccess(command.getRequest(),
                                     command.getFirstSuccesRsp());
            command.finish();
        } else if (command.getSuccessRspCount() >= quorumMin && !command.hasFinished()) {
            requestsLoger.logFailure(command.getRequest());
            command.finish();
        }
    }

    void handleDeleteResponse(const Message &msg) {
        auto reqTransaction = msg.getTransaction();
        auto commandIterator = pendingCommands.find(reqTransaction);
        if (commandIterator == pendingCommands.end())
            return;

        auto &command = commandIterator->second;
        auto endpointsCount = command.getEndpoints().size();
        command.addResponse(move(msg));

        if (command.getSuccessRspCount() == endpointsCount) {
            requestsLoger.logSuccess(command.getRequest(),
                                     command.getFirstSuccesRsp());
            command.finish();
        } else if (command.getRspCount() == endpointsCount) {
            requestsLoger.logFailure(command.getRequest());
            command.finish();
        }
    }

    void execute(Command&& command) {
        pendingCommands.emplace(transaction, move(command));
        pendingCommands[transaction].multicast(msgStream);
    }

    void onClusterUpdate() override {
        for (auto &hashCommandPair : pendingCommands) {
            auto &command = get<1>(hashCommandPair);
            if (command.hasFinished())
                continue;

            command.updateTimeLeft();
            if (command.getTimeLeft() == 0) {
                requestsLoger.logFailure(command.getRequest());
                command.finish();
            }
        }
    }

    AddressList getNaturalNodes(const string &key) {
        auto members = membershipProxy->getMembersList();
        partitioner.updateRing(members);
        return partitioner.getNaturalNodes(key, members);
    }

private:
    shared_ptr<dsproto::MessageStream>  msgStream;
    RingPartitioner partitioner;
    MembershipProxy membershipProxy;
    CommandLogger   requestsLoger;

    uint32_t transaction = 0;

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
        if (backend->probe(msg)) {
            backend->handle(msg);
        } else if (coordinator->probe(msg)) {
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
