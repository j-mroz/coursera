/**********************************
 * Author: Jaroslaw Mroz
 **********************************/

#include "MP1Node.h"

#include <cmath>
#include <numeric>

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

        auto gossipRange = (size_t)log(membersCount);
        if (gossipRange < membersCount) ++gossipRange;
        if (gossipRange < membersCount) ++gossipRange;

        return vector<size_t>(membersIndices.begin(),
                              membersIndices.begin() + gossipRange);
    }

    void sendGossip(const vector<char> &msg) {
        auto gossipPeers = pickGossipGroup();
        for (auto peerIndex : gossipPeers) {
            auto &member = node->getMembersList()[peerIndex];
            auto memberAddr = toAddress(member.id, member.port);
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
        auto &activeMembers = node->getActiveMembers();
        auto &failedMembers = node->getFailedMembers();
        auto timestamp = node->getTimestamp();

        // Move failed members to failed list
        auto hasFailed = [&](const MemberListEntry& member) {
            return timestamp - member.timestamp >= failTimeout;
        };
        for (auto &member : members) {
            auto hash = addressHash(member.id, member.port);

            // get latest value from active list and write back to original list
            auto activePos = activeMembers.find(hash);
            if (activePos != activeMembers.end())
                member = activePos->second;

            if (hasFailed(member)) {
                failedMembers[hash] = member;
                activeMembers.erase(hash);
            }
        }
        members.erase(remove_if(members.begin(), members.end(), hasFailed),
                      members.end());

        // Remove hard failed members
        auto isRemovable = [&](const MemberListEntry& member) {
          return timestamp - member.timestamp >= removeTimeout;
        };

        auto &failed = node->getFailedMembers();
        for (auto memberPos = failed.begin(); memberPos != failed.end(); ) {

            if (isRemovable(memberPos->second)) {
                node->logNodeRemove(toAddress(memberPos->second.id,
                                              memberPos->second.port));
                memberPos = failed.erase(memberPos);
            } else {
                ++memberPos;
            }
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
        logNode("init failed. Exit.");
        exit(1);
    }

    auto joinaddr = getJoinAddress();
    if (join(&joinaddr) != 0) {
        finishUpThisNode();
        logNode("Unable to join self to group. Exiting.");
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
    memberNode->timestamp = 0;
    memberNode->memberList.clear();

    return ESUCCESS;
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
        logNode("Starting up group...");
        memberNode->inGroup = true;
    } else {
        JoinRequest req {
            JOINREQ,
            getId(),
            getPort(),
            getHeartbeat()
        };

        logNode("Trying to join... ");
        send(*joinaddr, (char*)&req, sizeof(req));
    }

    return ESUCCESS;
}

/**
 * Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode() {
    return ESUCCESS;
}

static int queueIngress(void *env, char *buff, int size) {
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
        auto msg = memberNode->mp1q.front();
        memberNode->mp1q.pop();
        handleRequest((void *)memberNode, (char *)msg.elt, msg.size);
        free(msg.elt);
    }
}


/**
 * Message handler for different message types
 */
int MP1Node::handleRequest(void *env, char *data, int size) {
    if ((size_t)size < sizeof(Request))
        return EFAIL;

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
        break;
    case HEARTBEAT:
        handleHeartbeat(rawReq, size);
        break;
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
    auto *offset = buff.data() + sizeof(header);
    for (auto &entry : memberNode->memberList)
        offset = copyMemberBytes(offset, entry);

    auto req = getUnaligned<JoinRequest>(rawReq);
    send(toAddress(req.id, req.port), buff.data(), buff.size());

    addMemberEntry({req.id, req.port, req.heartbeat, getTimestamp()});
}


/**
 *
 */
void MP1Node::handleJoinResponse(void *raw, size_t size) {
    auto rsp = getUnaligned<JoinResponse>(raw);
    auto *payload = (char*)raw + sizeof(JoinResponse);
    addMemberEntry({rsp.id, rsp.port, rsp.heartbeat, getTimestamp()});
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
        addMemberEntry({entry.id, entry.port, entry.heartbeat, getTimestamp()});
        buff += sizeof(entry);
    }
}

/**
 *
 */
void MP1Node::handleHeartbeat(void *rawReq, size_t size) {
    auto req = getUnaligned<Heartbeat>(rawReq);

    int64_t hash = addressHash(req.id, req.port);
    failedMembers.erase(hash);
    auto peer = activeMembers.find(hash);
    if (peer != activeMembers.end()) {
        peer->second.heartbeat = req.heartbeat;
        peer->second.timestamp = getTimestamp();
    }
}


/**
 *
 */
void MP1Node::addMemberEntry(MemberListEntry entry) {
    auto hash = addressHash(entry.id, entry.port);
    auto activePos = activeMembers.find(hash);
    auto failedPos = failedMembers.find(hash);

    if (activePos != activeMembers.end() ) {
        if (activePos->second.heartbeat < entry.heartbeat) {
            activePos->second.heartbeat = entry.heartbeat;
            activePos->second.timestamp = entry.timestamp;
        }
    } else if (failedPos != failedMembers.end() &&
               failedPos->second.heartbeat < entry.heartbeat) {
        activeMembers[hash] = move(entry);
        failedMembers.erase(failedPos);
        //logNodeAdd(toAddress(entry.id, entry.port));
    } else if (failedPos == failedMembers.end()) {
        activeMembers[hash] = entry;
        memberNode->memberList.push_back(move(entry));
        logNodeAdd(toAddress(entry.id, entry.port));
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
    return getUnaligned<int32_t>(&memberNode->addr.addr[0]);

}

int16_t MP1Node::getPort() {
    return getUnaligned<int16_t>(&memberNode->addr.addr[4]);
}

int64_t MP1Node::getHeartbeat() {
    return memberNode->heartbeat;
}

int64_t MP1Node::getTimestamp() {
    return memberNode->timestamp;
}

Member* MP1Node::getMemberNode() {
    return memberNode;
}

MP1Node::MembersList& MP1Node::getMembersList() {
    return memberNode->memberList;
}

MP1Node::MembersMap& MP1Node::getActiveMembers() {
    return activeMembers;
}

void MP1Node::advanceHeartbeat() {
    ++memberNode->heartbeat;
}

void MP1Node::advanceTimestamp() {
    ++memberNode->timestamp;
}

MP1Node::MembersMap& MP1Node::getFailedMembers() {
    return failedMembers;
}

int MP1Node::send(Address addr, char *data, size_t len) {
    return emulNet->ENsend(&memberNode->addr, &addr, data, len);
}

void MP1Node::logNode(const char *fmt, ...) {
#ifdef DEBUGLOG
    va_list args;
    va_start(args, fmt);
    log->LOG(&memberNode->addr, fmt, args);
    va_end(args);
#endif
}

void MP1Node::logNodeAdd(Address other) {
#ifdef DEBUGLOG
    log->logNodeAdd(&memberNode->addr, &other);
#endif
}

void MP1Node::logNodeRemove(Address other) {
#ifdef DEBUGLOG
    log->logNodeRemove(&memberNode->addr, &other);
#endif
}
