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
enum MsgTypes{
    JOINREQ,
    JOINRSP,
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
    // int64_t heartbeat;
    uint64_t membersCount;
};

struct MemberData {
    int32_t id;
    int16_t port;
    int64_t heartbeat;
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

    int32_t getId(Address &a);
    int16_t getPort(Address &a);
    Address getAddress(int32_t id, int16_t port);
	Member* getMemberNode() { return memberNode; }
    int64_t getHeartbeat();


    void    nodeLoop();
	void    handleIngress();
    void    handleNodeOperations();

    void    handleJoinRequest(void *rawReq);
    void    handleJoinResponse(void *rawReq);

    int     recvCallBack(void *env, char *data, int size);
    int     recvLoop();

	static int enqueueWrapper(void *env, char *buff, int size);
	void nodeStart(char *servaddrstr, short serverport);
	void checkMessages();
	int isNullAddress(Address *addr);
	Address getJoinAddress();
	void printAddress(Address *addr);

private:
    void    prepareJoinResponseHeader(char *buff);
    void    prepareJoinResponsePayload(char *buff);
    void    addMemberEntry(const MemberData &member);
    size_t  copyMemberEntry(char* buff, const MemberListEntry &entry);

private:
    EmulNet *emulNet;
    Log *log;
    Params *par;
    Member *memberNode;
    char NULLADDR[6];
};

#endif /* _MP1NODE_H_ */
