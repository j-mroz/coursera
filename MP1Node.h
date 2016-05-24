/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"

#include <unordered_set>
#include <set>

/**
 * Macros
 */
#define TREMOVE 20
#define TFAIL 5

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Message Types
 */
enum MsgTypes {
    JOINREQ,
    JOINRSP,
    ADD_MEMBERS_REQ
};

#define ESUCCESS 0
#define ESIZE 1


struct Request {
    int16_t msgType;
};

struct JoinRequest {
    int16_t msgType;
    int32_t id;
    int16_t port;
    int64_t heartbeat;
};

struct JoinResponse {
    int16_t msgType;
    int32_t id;
    int16_t port;
    int64_t heartbeat;
    uint64_t membersCount;
};

struct MemberData {
    int32_t id;
    int16_t port;
    int64_t heartbeat;
};

struct AddMembersReq {
    int16_t msgType;
    int32_t id;
    int16_t port;
    int64_t heartbeat;
    uint64_t membersCount;
};

/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
public:
	MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
    virtual ~MP1Node();

    int     init();
    void    initMembersList();
    int     join(Address *joinAddress);
    int     finishUpThisNode();

    int32_t getId();
    int16_t getPort();
    int64_t getHeartbeat();
    long    getTimestamp();
	Member* getMemberNode() { return memberNode; }

    using MembersList = decltype( ((Member*)0)->memberList );
    MembersList &getMembersList() {
        return memberNode->memberList;
    }


    void    nodeStart(char *servaddrstr, short serverport);
    void    nodeLoop();
	void    handleIngress();
    void    handleNodeOperations();

    void    handleJoinRequest(void *rawReq);
    void    handleJoinResponse(void *rawReq);
    void    handleAddMembersRequest(void *rawReq);
    void    handleMembersData(char *buff, uint64_t count);

    int     recvCallBack(void *env, char *data, int size);
    int     recvLoop();
    int     send(Address addr, char *data, size_t len);

private:
    void    prepareJoinResponseHeader(char *buff);
    void    prepareJoinResponsePayload(char *buff);
    void    addMemberEntry(const MemberData &member);

    void    advanceHeartbeat();
    void    advanceTimestamp();

    Address getJoinAddress();
    void    printAddress(Address *addr);

// private:
public:
    using PeersCache = std::set<int64_t>;

    EmulNet     *emulNet;
    Log         *log;
    Params      *par;
    Member      *memberNode;
    PeersCache  peersCache;
    bool        peersChangeDetected = false;
    long        timestamp = 0;
};

#endif /* _MP1NODE_H_ */
