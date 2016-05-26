/**********************************
 * Author: Jaroslaw Mroz
 **********************************/

#include "MP1Node.h"

#include <numeric>
#include <cmath>
#include <string>

#ifdef DEBUGLOG
#define debug_log(a, b, ...) do { log->LOG(a, b, ##__VA_ARGS__); } while (0)
#define debug_log_node_add(a, b) do { log->logNodeAdd(a, b); } while (0)
#define debug_log_node_remove(a, b) do { log->logNodeRemove(a, b); } while (0)
#else
#define debug_log(a, b, ...)
#define debug_log_node_add(a, b)
#define debug_log_node_remove(a, b)
#endif


template <typename Aligned>
Aligned getUnaligned(void *ptr) {
    Aligned result;
    memcpy((char*)&result, (char*)ptr, sizeof(Aligned));
    return result;
}

static Address toAddress(int32_t id, int16_t port) {
    Address result;
    memset(&result, 0, sizeof(Address));
    memcpy(&result.addr[0], (char *)&id, sizeof(char)*4);
    memcpy(&result.addr[4], (char *)&port, sizeof(char)*2);
    return result;
}

static char* copyMemberBytes(char *buff, const MemberListEntry &entry) {
    auto data = MemberData {
        entry.id, entry.port, entry.heartbeat
    };
    return copy((char *)&data, (char *)(&data+1), buff);
}

static int64_t addressHash(int32_t id, int16_t port) {
    return ((int64_t)id << 32) + port;
}

bool operator==(const MemberListEntry& lhs, const MemberListEntry& rhs) {
    return addressHash(lhs.id, lhs.port) == addressHash(rhs.id, rhs.port);
}

size_t MemberListEntryHash::operator()(const MemberListEntry& member) const {
    return addressHash(member.id, member.port);
}

/**
 *
 */
class GossipBase {
protected:
    MP1Node *node;
    vector<size_t> membersIndices;

    GossipBase(MP1Node *n) {
        node = n;
    }

    virtual ~GossipBase() { };

    vector<size_t> pickGossipGroup() {
        auto membersCount = node->getMembersList().size();
        membersIndices.resize(membersCount);
        iota(membersIndices.begin(), membersIndices.end(), 0);
        random_shuffle(membersIndices.begin(), membersIndices.end());

        auto gossipRange = (int64_t)log(membersCount);
        if (gossipRange < membersCount) ++gossipRange;
        if (gossipRange < membersCount) ++gossipRange;

        return vector<size_t>(membersIndices.begin(),
                              membersIndices.begin() + gossipRange);
    }

    void sendGossip(const vector<char> &msg) {
        auto gossipPeers = pickGossipGroup();
        for (auto peerIndex : gossipPeers) {
            auto &member = node->getMembersList()[peerIndex];
            auto memberAddr = toAddress(member.getid(), member.getport());
            node->send(move(memberAddr), (char *)msg.data(), msg.size());
        }
    }
};

/**
 *
 */
class GossipDisseminator : public GossipBase, public Task {
    vector<char> msgBuff;

public:
    GossipDisseminator(MP1Node *node) : GossipBase(node), Task() { }
    virtual ~GossipDisseminator() { }

    void run() {
        // Allocate buffer
        auto membersCount = node->getMembersList().size();
        auto reqHeaderSize = sizeof(AddMembersRequest);
        auto reqPayloadSize = sizeof(MemberData) * membersCount;
        msgBuff.resize(reqHeaderSize + reqPayloadSize);

        // Prepare header
        auto req = AddMembersRequest {
            ADD_MEMBERS_REQ,
            node->getId(),
            node->getPort(),
            node->getHeartbeat(),
            node->getMemberNode()->memberList.size()
        };
        memcpy(msgBuff.data(), (char *)&req, sizeof(AddMembersRequest));

        // prepare payload
        auto payload = (char*)msgBuff.data() + reqHeaderSize;
        for (auto &entry : node->getMembersList())
            payload = copyMemberBytes(payload, entry);

        // And send
        sendGossip(msgBuff);
    }
};

class HearbeatService : public GossipBase, public Task {
    vector<char> msgBuff;

public:
    HearbeatService(MP1Node *node) : GossipBase(node), Task() {
        msgBuff.resize(sizeof(Heartbeat));
    }

    virtual ~HearbeatService() { }

    void run() {
        // Prepare
        auto heartbeat = Heartbeat {
            HEARTBEAT,
            node->getId(),
            node->getPort(),
            node->getHeartbeat(),
        };
        memcpy(msgBuff.data(), (char *)&heartbeat, sizeof(Heartbeat));

        // Send
        sendGossip(msgBuff);
    }
};

/**
 *
 */
class FailureDetector : public Task {
    MP1Node *node;
    long failTimeout = TFAIL;
    long removeTimeout = TREMOVE;

public:
    FailureDetector(MP1Node *node) : Task(), node(node) { }
    virtual ~FailureDetector() { }

    void run() {
        auto &members = node->getMembersList();
        auto timestamp = node->getTimestamp();

        auto hasFailed = [&](const MemberListEntry& member) {
            return timestamp - member.timestamp >= failTimeout;
        };

        // Move failed members to failed list
        for (auto &member : members) {
            member = node->getCachedEntry(member);  // write from cache
            if (hasFailed(member)) {
                node->getFailedMembers().insert(member);
                node->eraseCached(member);
            }
        }
        members.erase(remove_if(members.begin(), members.end(), hasFailed),
                      members.end());

        auto isRemovable = [&](const MemberListEntry& member) {
          return timestamp - member.timestamp >= removeTimeout;
        };

        for (auto &peer : node->getFailedMembers()) {
          if (isRemovable(peer))
            node->removeFailed(peer);
        }
    }
};

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params,
                EmulNet *emul, Log *log, Address *address) {
    this->memberNode = member;
    this->memberNode->addr = *address;
    this->emulNet = emul;
    this->log = log;
    this->par = params;

    tasks.push_back(unique_ptr<Task>(new FailureDetector(this)));
    tasks.push_back(unique_ptr<Task>(new GossipDisseminator(this)));
    tasks.push_back(unique_ptr<Task>(new HearbeatService(this)));
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * This function bootstraps the node
 * All initializations routines for a member.
 * Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    // Self booting routines
    if (init() != 0) {
        debug_log(&memberNode->addr, "init failed. Exit.");
        exit(1);
    }

    auto joinaddr = getJoinAddress();
    if (join(&joinaddr) != 0) {
        finishUpThisNode();
        debug_log(&memberNode->addr, "Unable to join self to group. Exiting.");
        exit(1);
    }
}

/**
 * Find out who I am and start up
 */
int MP1Node::init() {
    memberNode->bFailed = false;
    memberNode->inGroup = false;
    memberNode->nnb = 0;
    memberNode->heartbeat = 0;
    memberNode->pingCounter = TFAIL;
    memberNode->timeOutCounter = -1;
    memberNode->memberList.clear();

    return 0;
}

/**
 * Join the distributed system
 */
int MP1Node::join(Address *joinaddr) {

    bool selfJoin = !memcmp((char *) (& memberNode->addr.addr),
                            (char *) (& joinaddr->addr),
                            sizeof(memberNode->addr.addr));

    if (selfJoin) {
        // I am the group booter (first process to join the group).
        debug_log(&memberNode->addr, "Starting up group...");
        memberNode->inGroup = true;
    } else {
        JoinRequest req {
            JOINREQ,
            getId(),
            getPort(),
            getHeartbeat()
        };

        debug_log(&memberNode->addr, "Trying to join... ");
        send(*joinaddr, (char*)&req, sizeof(req));
    }

    return 0;
}

/**
 * Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode() {
    return 0;
}

int queueIngress(void *env, char *buff, int size) {
    return Queue::enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * This function receives message from the network and pushes into the queue
 * This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if (memberNode->bFailed)
        return false;

    return emulNet->ENrecv(&memberNode->addr, queueIngress,
                           nullptr, 1, &memberNode->mp1q);
}

/**
 * Executed periodically at each member
 * Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed)
        return;

    drainIngressQueue();

    if (!memberNode->inGroup)
        return;

    runTasks();
}

/**
 * Check messages in the queue and call the respective message handler
 */
void MP1Node::drainIngressQueue() {
    while (!memberNode->mp1q.empty()) {
        auto *ptr = memberNode->mp1q.front().elt;
        auto size = memberNode->mp1q.front().size;
        memberNode->mp1q.pop();
        handleRequest((void *)memberNode, (char *)ptr, size);
    }
}


/**
 * Message handler for different message types
 */
int MP1Node::handleRequest(void *env, char *data, int size) {
    if ((size_t)size < sizeof(Request))
        return ESIZE;

    void *rawReq = (void *) data;
    Request req = getUnaligned<Request>(rawReq);

    switch (req.msgType){
    case JOINREQ:
        handleJoinRequest(rawReq, size);
        break;
    case JOINRSP:
        handleJoinResponse(rawReq, size);
        break;
    case ADD_MEMBERS_REQ:
        handleAddMembersRequest(rawReq, size);
    case HEARTBEAT:
        handleHeartbeat(rawReq, size);
    default:
        break;
    }

    return ESUCCESS;
}

/**
 *
 */
void MP1Node::handleJoinRequest(void *rawReq, size_t size) {
    static auto buff = vector<char>();
    auto payloadSize = memberNode->memberList.size() * sizeof(MemberData);
    buff.resize(sizeof(JoinResponse) + payloadSize);

    // prepare header
    auto header = JoinResponse {
        JOINRSP,
        getId(),
        getPort(),
        getHeartbeat(),
        memberNode->memberList.size(),
    };
    copy((char*)&header, (char*)(&header+1), buff.begin());

    // prepare payload
    char *offset = buff.data() + sizeof(header);
    for (auto &entry : memberNode->memberList)
        offset = copyMemberBytes(offset, entry);

    auto req = getUnaligned<JoinRequest>(rawReq);
    send(toAddress(req.id, req.port), buff.data(), buff.size());

    addMemberEntry(MemberData{req.id, req.port, req.heartbeat});
}


/**
 *
 */
void MP1Node::handleJoinResponse(void *raw, size_t size) {
    auto rsp = getUnaligned<JoinResponse>(raw);
    auto *payload = (char*)raw + sizeof(JoinResponse);
    addMemberEntry({rsp.id, rsp.port, rsp.heartbeat});
    handleMembersData(payload, rsp.membersCount);
    memberNode->inGroup = true;
}

/**
 *
 */
void MP1Node::handleAddMembersRequest(void *raw, size_t size) {
    auto req = getUnaligned<AddMembersRequest>(raw);
    auto *payload = (char*)raw + sizeof(AddMembersRequest);
    handleMembersData(payload, req.membersCount);
}

/**
 *
 */
void MP1Node::handleMembersData(char *buff, uint64_t count) {
    for (auto memberIdx = 0ull; memberIdx < count; ++memberIdx) {
        auto entry = getUnaligned<MemberData>((void*)buff);
        addMemberEntry(entry);
        buff += sizeof(entry);
    }
}

/**
 *
 */
void MP1Node::handleHeartbeat(void *rawReq, size_t size) {
    auto req = getUnaligned<Heartbeat>(rawReq);
    failedPeers.erase(MemberListEntry { req.id, req.port, 0, 0 });

    int64_t hash = addressHash(req.id, req.port);
    auto peer = peersCache.find(hash);
    if (peer != peersCache.end()) {
        peer->second.heartbeat = req.heartbeat;
        peer->second.timestamp = timestamp;
    }
}


/**
 *
 */
void MP1Node::addMemberEntry(const MemberData &member) {
    auto memberEntry = MemberListEntry {
        member.id,
        member.port,
        member.heartbeat,
        timestamp
    };
    auto hash = addressHash(memberEntry.id, memberEntry.port);
    auto cachePos = peersCache.find(hash);
    auto failedPos = failedPeers.find(memberEntry);

    if (cachePos != peersCache.end() ) {
        if (cachePos->second.heartbeat < memberEntry.heartbeat) {
            cachePos->second.heartbeat = memberEntry.heartbeat;
            cachePos->second.timestamp = memberEntry.timestamp;
            peersChangeDetected = true;
        }
        return;
    } else if (failedPos != failedPeers.end() &&
               failedPos->heartbeat < memberEntry.heartbeat) {
        failedPeers.erase(failedPos);
    } else if (failedPos == failedPeers.end()) {
        Address remote = toAddress(member.id, member.port);
        debug_log_node_add(&memberNode->addr, &remote);
        peersCache[hash] = memberEntry;
        memberNode->memberList.push_back(memberEntry);
        peersChangeDetected = true;
    }
}

/**
 * Check if any node hasn't responded within a timeout period and then delete
 * the nodes
 * Propagate your membership list
 */
void MP1Node::runTasks() {
    advanceTimestamp();
    advanceHeartbeat();

    for (auto &task : tasks) {
        task->run();
    }

}

/**
 * Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    return toAddress(1, 0);
}

int32_t MP1Node::getId() {
    // return *(int32_t*)(&memberNode->addr.addr);
    return getUnaligned<int32_t>(&memberNode->addr.addr[0]);

}

int16_t MP1Node::getPort() {
    return getUnaligned<int16_t>(&memberNode->addr.addr[4]);
    // return *(int16_t*)(&memberNode->addr.addr[4]);
}

int64_t MP1Node::getHeartbeat() {
    return memberNode->heartbeat;
}

long MP1Node::getTimestamp() {
    return timestamp;
}

MemberListEntry& MP1Node::getCachedEntry(MemberListEntry& entry) {
    auto hash = addressHash(entry.id, entry.port);
    auto pos = peersCache.find(hash);
    if (pos != peersCache.end())
        return pos->second;
    return entry;
}

Member* MP1Node::getMemberNode() {
    return memberNode;
}

MP1Node::MembersList& MP1Node::getMembersList() {
    return memberNode->memberList;
}

void MP1Node::advanceHeartbeat() {
    ++memberNode->heartbeat;
}

void MP1Node::advanceTimestamp() {
    ++timestamp;
}

MP1Node::MembersSet& MP1Node::getFailedMembers() {
    return failedPeers;
}

void MP1Node::removeFailed(const MemberListEntry &member) {
    failedPeers.erase(member);
    Address remote = toAddress(member.id, member.port);
    debug_log_node_remove(&memberNode->addr, &remote);
}

void MP1Node::eraseCached(MemberListEntry &member) {
    peersCache.erase(addressHash(member.id, member.port));
}

int MP1Node::send(Address addr, char *data, size_t len) {
    return emulNet->ENsend(&memberNode->addr, &addr, data, len);
}
