/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

#include <numeric>
#include <cmath>

#ifdef DEBUGLOG
#define debug_log(a, b, ...) do { log->LOG(a, b, ##__VA_ARGS__); } while (0)
#define debug_log_node_add(a, b) do { log->logNodeAdd(a, b); } while (0)
#else
#define debug_log(a, b, ...)
#define debug_log_node_add(a, b)
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
		auto membersCount = node->getMemberNode()->memberList.size();
		auto gossipRange = (int64_t)log(membersCount) + 1;

		membersIndices.resize(membersCount);
        iota(membersIndices.begin(), membersIndices.end(), 0);
        random_shuffle(membersIndices.begin(), membersIndices.end());

		return vector<size_t>(membersIndices.begin(),
							  membersIndices.begin() + gossipRange);

	}

    void gossipMembersList() {
		size_t membersCount = node->getMemberNode()->memberList.size();
		size_t reqHeaderSize = sizeof(AddMembersReq);
		size_t reqPayloadSize = sizeof(MemberData) * membersCount;
		vector<char> msgBuff(reqHeaderSize + reqPayloadSize);

		auto req = AddMembersReq {
			ADD_MEMBERS_REQ,
			node->getId(),
			node->getPort(),
			node->getHeartbeat(),
			node->getMemberNode()->memberList.size()
		};

		char *buffOffset = msgBuff.data();
		memcpy(buffOffset, (char*)&req, reqHeaderSize);
		buffOffset += reqHeaderSize;

		for (auto &entry : node->getMemberNode()->memberList) {
			auto copiedBytes = copyMemberEntry(buffOffset, entry);
			buffOffset += copiedBytes;
		}

		auto gossipPeers = pickGossipGroup();
		for (auto peerIndex : gossipPeers) {
			auto &member  = node->getMemberNode()->memberList[peerIndex];
			node->send(makeAddress(member.getid(), member.getport()),
					   msgBuff.data(), reqHeaderSize + reqPayloadSize);
		}
    }
};

// class FailureDetector {
// 	MP1Node *node;
// 	long failTimeout;
//
// public:
// 	FailureDetector(MP1Node *node) : node(node) { }
//
// 	void run() {
// 		detectStaleMembers();
// 	}
//
// 	void detectStaleMembers() {
//
// 		for (auto &member : node->getMembersList()) {
// 			if (member->timestamp < node->getTimestamp() - failTimeout) {
// 				markFailed(member)
// 			}
// 		}
//
// 	}
//
// };


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

	#define handle(REQ_TYPE) { 						\
		assert((size_t)size >= sizeof(#REQ_TYPE));	\
		handle##REQ_TYPE(rawReq);					\
		break;										\
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
    auto req = getUnaligned<AddMembersReq>(raw);
    auto *payload = (char*)raw + sizeof(AddMembersReq);
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
	if (peersCache.count(member.id) != 0) {
		return;
	}
	peersCache.insert(member.id);
	peersChangeDetected = true;

	memberNode->memberList.push_back({
		member.id,
		member.port,
		member.heartbeat,
		timestamp
	});
	Address remote = makeAddress(member.id, member.port);
	debug_log_node_add(&memberNode->addr, &remote);
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::handleNodeOperations() {
	if (peersChangeDetected) {
		GossipDisseminator disseminator(this);
		disseminator.gossipMembersList();
		peersChangeDetected = false;
	}
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

void MP1Node::advanceHeartbeat() {
	++memberNode->heartbeat;
}

void MP1Node::advanceTimestamp() {
	++timestamp;
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
