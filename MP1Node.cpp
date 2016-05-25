/**********************************
 * Author: Jaroslaw Mroz
 **********************************/

#include "MP1Node.h"

#include <numeric>
#include <cmath>

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

static Address makeAddress(int32_t id, int16_t port) {
    Address result;
    memset(&result, 0, sizeof(Address));
    *(int32_t *)(&result.addr[0]) = id;
    *(int16_t *)(&result.addr[4]) = port;
    return result;
}

static size_t copyMemberEntry(char *buff, const MemberListEntry &entry) {
    auto data = MemberData {
        entry.id, entry.port, entry.heartbeat
    };
    memcpy(buff, (char*)&data, sizeof(MemberData));
    return sizeof(MemberData);
}

static int64_t toInt64(const MemberData& member) {
    return ((int64_t)member.id << 32) + member.port;
}

static int64_t toInt64(const MemberListEntry& member) {
    auto m = const_cast<MemberListEntry&>(member);
    return ((int64_t)m.getid() << 32) + m.getport();
}

bool operator==(const MemberListEntry& lhs, const MemberListEntry& rhs) {
    return toInt64(lhs) == toInt64(rhs);
}

size_t MemberListEntryHash::operator()(const MemberListEntry& member) const {
    return toInt64(member);
}

/**
 *
 */
class GossipDisseminator {
    MP1Node *node;

public:
    GossipDisseminator(MP1Node *node) : node(node) { }

    void run() {
        gossipMembersList();
    }

    vector<size_t> pickGossipGroup() {
        static auto membersIndices = vector<int>();
        auto membersCount = node->getMembersList().size();
        auto gossipRange = (int64_t)log(membersCount) + 1;
        if (gossipRange < membersCount)
            ++gossipRange;

        membersIndices.resize(membersCount);
        iota(membersIndices.begin(), membersIndices.end(), 0);
        random_shuffle(membersIndices.begin(), membersIndices.end());

        return vector<size_t>(membersIndices.begin(),
                              membersIndices.begin() + gossipRange);

    }

    void gossipMembersList() {
        size_t membersCount = node->getMembersList().size();
        size_t reqHeaderSize = sizeof(AddMembersRequest);
        size_t reqPayloadSize = sizeof(MemberData) * membersCount;
        vector<char> msgBuff(reqHeaderSize + reqPayloadSize);

        auto req = AddMembersRequest {
            ADD_MEMBERS_REQ,
            node->getId(),
            node->getPort(),
            node->getHeartbeat(),
            node->getMemberNode()->memberList.size()
        };

        char *buffOffset = msgBuff.data();
        memcpy(buffOffset, (char*)&req, reqHeaderSize);
        buffOffset += reqHeaderSize;

        for (auto &entry : node->getMembersList()) {
            auto copiedBytes = copyMemberEntry(buffOffset, entry);
            buffOffset += copiedBytes;
        }

        auto gossipPeers = pickGossipGroup();
        for (auto peerIndex : gossipPeers) {
            auto &member  = node->getMembersList()[peerIndex];
            node->send(makeAddress(member.getid(), member.getport()),
                       msgBuff.data(), reqHeaderSize + reqPayloadSize);
        }
    }
};

/**
 *
 */
class FailureDetector {
    MP1Node *node;
    long failTimeout = TFAIL;
    long removeTimeout = TREMOVE;

public:
    FailureDetector(MP1Node *node) : node(node) { }

    void run() {
        detectFailedMembers();
        removeFailedMembers();
    }

    void detectFailedMembers() {
        auto &members = node->getMembersList();
        auto timestamp = node->getTimestamp();

        printf("[" );
        for (auto &member : members) {
            printf("%ld:%ld ", member.heartbeat, member.timestamp);
            member = node->getCachedEntry(member);  // write from cache
            if (timestamp - member.gettimestamp() >= failTimeout) {
                node->markFailed(member);
            }
        }
        printf("]\n" );
    }

    void removeFailedMembers() {
        auto &members = node->getMembersList();
        auto timestamp = node->getTimestamp();

        auto isRemovable = [&](const MemberListEntry& member) {
            return timestamp - member.timestamp >= removeTimeout;
        };
        auto hardFailedBegin = remove_if(members.begin(), members.end(), isRemovable);
        auto hardFailedEnd = members.end();

        for (auto peer = hardFailedBegin; peer != hardFailedEnd; ++peer)
            node->eraseCached(*peer);

        members.erase(hardFailedBegin, hardFailedEnd);
    }

};


/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
    // std::fill(NULLADDR, NULLADDR+6, 0);
    this->memberNode = member;
    this->memberNode->addr = *address;
    this->emulNet = emul;
    this->log = log;
    this->par = params;
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
    auto joinaddr = getJoinAddress();

    // Self booting routines
    if (init() != 0) {
        debug_log(&memberNode->addr, "init failed. Exit.");
        exit(1);
    }

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
    initMembersList();

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
        // Boot up the group
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

/**
 * This function receives message from the network and pushes into the queue
 * This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if (memberNode->bFailed)
        return false;

    auto queueIngress = [&](void *env, char *buff, int size) -> int  {
        return Queue::enqueue((queue<q_elt> *)env, (void *)buff, size);
    };

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

    handleIngress();

    if (!memberNode->inGroup)
        return;

    handleNodeOperations();
}

/**
 * Check messages in the queue and call the respective message handler
 */
void MP1Node::handleIngress() {
    // Pop waiting messages from memberNode's mp1q
    while (!memberNode->mp1q.empty()) {
        auto *ptr = memberNode->mp1q.front().elt;
        auto size = memberNode->mp1q.front().size;
        memberNode->mp1q.pop();
        recvCallBack((void *)memberNode, (char *)ptr, size);
    }
}

/**
 * Message handler for different message types
 */
int MP1Node::recvCallBack(void *env, char *data, int size) {
    if ((size_t)size < sizeof(Request)) {
        debug_log(&memberNode->addr,
                 "%s: Msg size (%d) too small", __func__, size);
        return ESIZE;
    }

    void *rawReq = (void *) data;
    Request req = getUnaligned<Request>(rawReq);

    #define handle(REQ_TYPE) {                        \
        assert((size_t)size >= sizeof(#REQ_TYPE));    \
        handle##REQ_TYPE(rawReq);                     \
        break;                                        \
    }

    switch (req.msgType) {
        case JOINREQ:
            handle(JoinRequest);
        case JOINRSP:
            handle(JoinResponse);
        case ADD_MEMBERS_REQ:
            handle(AddMembersRequest);
        default:
            break;
    }

    return ESUCCESS;
}

/**
 *
 */
void MP1Node::handleJoinRequest(void *rawReq) {
    static auto buff = vector<char>();
    auto payloadSize = memberNode->memberList.size() * sizeof(MemberData);
    buff.resize(sizeof(JoinResponse) + payloadSize);

    prepareJoinResponseHeader(buff.data());
    prepareJoinResponsePayload(buff.data() + sizeof(JoinResponse));

    auto req = getUnaligned<JoinRequest>(rawReq);
    Address remote = makeAddress(req.id, req.port);
    send(remote, buff.data(), buff.size());

    addMemberEntry(MemberData{req.id, req.port, req.heartbeat});
}

/**
 *
 */
void MP1Node::prepareJoinResponseHeader(char *buff) {
    auto rsp = JoinResponse {
        JOINRSP,
        getId(),
        getPort(),
        getHeartbeat(),
        memberNode->memberList.size(),
    };
    memcpy(buff, &rsp, sizeof(JoinResponse));
}

/**
 *
 */
void MP1Node::prepareJoinResponsePayload(char *buff) {
    for (auto &entry : memberNode->memberList) {
        size_t copied = copyMemberEntry(buff, entry);
        buff += copied;
    }
}

/**
 *
 */
void MP1Node::handleJoinResponse(void *raw) {
    auto rsp = getUnaligned<JoinResponse>(raw);
    auto *payload = (char*)raw + sizeof(JoinResponse);
    addMemberEntry({rsp.id, rsp.port, rsp.heartbeat});
    handleMembersData(payload, rsp.membersCount);
    memberNode->inGroup = true;
}

/**
 *
 */
void MP1Node::handleAddMembersRequest(void *raw) {
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
void MP1Node::addMemberEntry(const MemberData &member) {
    auto memberEntry = MemberListEntry {
        member.id,
        member.port,
        member.heartbeat,
        timestamp
    };
    auto hash = toInt64(memberEntry);
    auto cachePos = peersCache.find(hash);

    if (cachePos != peersCache.end()) {
        if (cachePos->second.heartbeat < memberEntry.heartbeat) {
            cachePos->second.heartbeat = memberEntry.heartbeat;
            cachePos->second.timestamp = memberEntry.timestamp;
            peersChangeDetected = true;
        }
    } else {
        peersCache[hash] = memberEntry;
        memberNode->memberList.push_back(memberEntry);
        Address remote = makeAddress(member.id, member.port);
        debug_log_node_add(&memberNode->addr, &remote);
        peersChangeDetected = true;
    }

}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * Check if any node hasn't responded within a timeout period and then delete
 *                 the nodes
 *                 Propagate your membership list
 */
void MP1Node::handleNodeOperations() {
    advanceTimestamp();
    advanceHeartbeat();

    if (peersChangeDetected) {
        GossipDisseminator disseminator(this);
        disseminator.run();
        peersChangeDetected = false;
    }
    FailureDetector failureDetector(this);
    failureDetector.run();
}

/**
 * Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    return makeAddress(1, 0);
}

/**
 * Initialize the membership list
 */
void MP1Node::initMembersList() {
    memberNode->memberList.clear();
}

int32_t MP1Node::getId() {
    return *(int32_t*)(&memberNode->addr.addr);
}

int16_t MP1Node::getPort() {
    return *(int16_t*)(&memberNode->addr.addr[4]);
}

int64_t MP1Node::getHeartbeat() {
    return memberNode->heartbeat;
}

long MP1Node::getTimestamp() {
    return timestamp;
}

MemberListEntry &MP1Node::getCachedEntry(MemberListEntry& entry) {
    auto hash = toInt64(entry);
    auto pos = peersCache.find(hash);
    if (pos != peersCache.end())
        return pos->second;
    return entry;
}


void MP1Node::advanceHeartbeat() {
    ++memberNode->heartbeat;
}

void MP1Node::advanceTimestamp() {
    ++timestamp;
}

void MP1Node::markFailed(MemberListEntry &member) {
    failedPeers.insert(member);
}

void MP1Node::eraseCached(MemberListEntry &member) {
    printf("usuwam\n" );
    Address remote = makeAddress(member.id, member.port);
    debug_log_node_remove(&memberNode->addr, &remote);
    peersCache.erase(toInt64(member));
}

int MP1Node::send(Address addr, char *data, size_t len) {
    return emulNet->ENsend(&memberNode->addr, &addr, data, len);
}


/**
* Print the Address
*/
void MP1Node::printAddress(Address *addr) {
    printf("%d.%d.%d.%d:%d \n",
          addr->addr[0], addr->addr[1],addr->addr[2],
          addr->addr[3], *(short*)&addr->addr[4]) ;
}
