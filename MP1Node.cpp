/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
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


// class PushGossipProtocol : public NodeOperation {
//     MP1Node *node;
//     bool    infected = false;
//
// public:
//     PushGossipProtocol(MP1Node *node) : node(node) {}
//
//     void handleCallback(char *data, size_t size) {
//
//     }
//     void run() {
//         if (!infected)
//             return;
//
//         auto membersCount = node->getMembers().size();
//         auto indices = vector<int>(membersCount);
//         iota(indices.begin(), indices.end(), 0);
//         random_shuffle(indices.begin(), indices.end());
//         auto gossipRange = (int)log(membersCount);
//         printf("Gossip range is %d\n", gossipRange);
//     }
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
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if (memberNode->bFailed) {
        return false;
    }
    else {
        return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper,
                               NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
    Queue q;
    return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
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
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::init() {
    /*
     * This function is partially implemented and may require changes
     */

    memberNode->bFailed = false;
    memberNode->inGroup = false;
    // node is up!
    memberNode->nnb = 0;
    memberNode->heartbeat = 0;
    memberNode->pingCounter = TFAIL;
    memberNode->timeOutCounter = -1;
    initMembersList();

    // auto pushGossip = unique_ptr<NodeOperation>(new PushGossipProtocol(this));
    // operations.push_back(move(pushGossip));

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
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
            getId(memberNode->addr),
            getPort(memberNode->addr),
            memberNode->heartbeat,
        };

        debug_log(&memberNode->addr, "Trying to join... %d", getId(*joinaddr));

        // send JoinRequest message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char*)&req, sizeof(req));
    }

    return 0;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode() {
   /*
    * Your code goes here
    */
    printf("help!!\n");
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
        return;
    }

    // Check my messages
    handleIngress();

    // Wait until you're in the group...
    if (!memberNode->inGroup) {
        return;
    }

    // ...then jump in and share your responsibilites!
    handleNodeOperations();
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
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
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
int MP1Node::recvCallBack(void *env, char *data, int size) {
    if (size < sizeof(Request)) {
        debug_log(&memberNode->addr,
                 "%s: Msg size (%d) too small", __func__, size);
        return ESIZE;
    }

    void *rawReq = (void *) data;
    Request req = getUnaligned<Request>(rawReq);

    switch (req.msgType) {
        case JOINREQ: {
            assert(size == sizeof(JoinRequest));
            handleJoinRequest(rawReq);
            break;
        }

        case JOINRSP: {
            assert(size >= sizeof(JoinResponse));
            handleJoinResponse(rawReq);
            break;
        }

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
    auto req = getUnaligned<JoinRequest>(rawReq);

    addMemberEntry({req.id, req.port, req.heartbeat});

    // Send back all members known
    prepareJoinResponseHeader(buff.data());
    prepareJoinResponsePayload(buff.data() + sizeof(JoinResponse));

    Address remote = getAddress(req.id, req.port);
    emulNet->ENsend(&memberNode->addr, &remote, buff.data(), buff.size());
}

/**
 *
 */
void MP1Node::prepareJoinResponseHeader(char *buff) {
    auto rsp = JoinResponse {
        JOINRSP,
        getId(memberNode->addr),
        getPort(memberNode->addr),
        memberNode->memberList.size(),
    };
    memcpy(buff, &rsp, sizeof(rsp));
}

/**
 *
 */
void MP1Node::prepareJoinResponsePayload(char *buff) {
    for (auto &entry : memberNode->memberList) {
        buff += copyMemberEntry(buff, entry);
    }
}

/**
 *
 */
size_t MP1Node::copyMemberEntry(char* buff, const MemberListEntry &entry) {
    auto data = MemberData {
        entry.id, entry.port, entry.heartbeat
    };
    memcpy(buff, (char*)&data, sizeof(MemberData));
    return sizeof(MemberData);
}

/**
 *
 */
void MP1Node::handleJoinResponse(void *raw) {
    auto rsp = getUnaligned<JoinResponse>(raw);
    auto *payload = (char*)raw + sizeof(JoinResponse);

    uint64_t membersCount = rsp.membersCount;
    for (auto memberIdx = 0ull; memberIdx < membersCount; ++memberIdx) {
        auto entry = getUnaligned<MemberData>((void*)payload);
        addMemberEntry(entry);
        payload += sizeof(entry);
    }

    memberNode->inGroup = true;
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::handleNodeOperations() {
    // for (auto &ops : operations) {
    //     ops->run();
    // }
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    return getAddress(1, 0);
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMembersList() {
    memberNode->memberList.clear();
}

/**
 *
 */
void MP1Node::addMemberEntry(const MemberData &member) {
    memberNode->memberList.push_back({
        member.id,
        member.port,
        member.heartbeat,
		getHeartbeat()
    });
    Address remote = getAddress(member.id, member.port);
    debug_log_node_add(&memberNode->addr, &remote);
}

int32_t MP1Node::getId(Address &a) {
    return *(int32_t*)(&a.addr);
}

int16_t MP1Node::getPort(Address &a) {
    return *(int16_t*)(&a.addr[4]);
}

Address MP1Node::getAddress(int32_t id, int16_t port) {
    Address result;
    memset(&result, 0, sizeof(Address));
    *(int32_t *)(&result.addr[0]) = id;
    *(int16_t *)(&result.addr[4]) = port;
    return result;
}

int64_t MP1Node::getHeartbeat() {
	return memberNode->heartbeat;
}

/**
* FUNCTION NAME: printAddress
*
* DESCRIPTION: Print the Address
*/
// void MP1Node::printAddress(Address *addr) {
//     printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
//            addr->addr[3], *(short*)&addr->addr[4]) ;
// }

// Member* MP1Node::getMemberNode() {
//     return memberNode;
// }

/**
* FUNCTION NAME: isNullAddress
*
* DESCRIPTION: Function checks if the address is NULL
*/
// int MP1Node::isNullAddress(Address *addr) {
//     return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
// }
