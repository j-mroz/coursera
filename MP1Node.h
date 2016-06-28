/**********************************
 * Membership protocol run by this Node.
 * Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"

#include <functional>
#include <memory>
#include <unordered_set>
#include <unordered_map>

#define TREMOVE 20    // Remove member timeout
#define TFAIL   5     // Mark member as failed timeout

/**
 * Message Types
 */
enum MsgTypes {
    JOINREQ,
    JOINRSP,
    ADD_MEMBERS_REQ,
    HEARTBEAT,
    MSG_TYPES_COUNT
};

#define ESUCCESS 0
#define EFAIL    1

#define _packed_ __attribute__((packed, aligned(2)))

struct _packed_ Request {
    int16_t msgType;
};

struct _packed_ JoinRequest {
    int16_t msgType;
    int32_t id;
    int16_t port;
    int64_t heartbeat;
};

struct _packed_ JoinResponse {
    int16_t msgType;
    int32_t id;
    int16_t port;
    int64_t heartbeat;
    uint64_t membersCount;
};

struct _packed_ MemberData {
    int32_t id;
    int16_t port;
    int64_t heartbeat;
};

struct _packed_ AddMembersRequest {
    int16_t msgType;
    int32_t id;
    int16_t port;
    int64_t heartbeat;
    uint64_t membersCount;
};

struct _packed_ Heartbeat {
    int16_t msgType;
    int32_t id;
    int16_t port;
    int64_t heartbeat;
};

struct Task {
    virtual void run() = 0;
    virtual ~Task() { }
};

/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
public:
    using MembersList = decltype( ((Member*)0)->memberList );
    using MembersMap = std::unordered_map<int64_t, MemberListEntry>;

    MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
    virtual ~MP1Node();

// Handlers API
    int32_t             getId();
    int16_t             getPort();
    int64_t             getHeartbeat();
    int64_t             getTimestamp();
    Member*             getMemberNode();
    MembersList&        getMembersList();
    MembersMap&         getActiveMembers();
    MembersMap&         getFailedMembers();
    int                 send(Address addr, char *data, size_t len);

    void logNode(const char *fmt, ...);
    void logNodeAdd(Address other);
    void logNodeRemove(Address other);

// Emulator API
    int     init();
    int     join(Address *joinAddress);
    int     finishUpThisNode();
    void    nodeStart(char *servaddrstr, short serverport);
    void    nodeLoop();
    int     recvLoop();

private:
    void    drainIngressQueue();
    void    runTasks();

    int     handleRequest(void *env, char *data, int size);
    void    handleJoinRequest(void *data, size_t size);
    void    handleJoinResponse(void *data, size_t size);
    void    handleAddMembersRequest(void *data, size_t size);
    void    handleHeartbeat(void *rawReq, size_t size);
    void    handleMembersData(char *buff, uint64_t count);

    void    updateMemberEntry(MemberListEntry entry);
    void    advanceHeartbeat();
    void    advanceTimestamp();

    Address getJoinAddress();

private:
    using TasksList = vector<unique_ptr<Task>>;

    EmulNet     *emulNet;
    Log         *log;
    Params      *par;
    Member      *memberNode;
    MembersMap  activeMembers;
    MembersMap  failedMembers;
    TasksList   tasks;
};

#endif /* _MP1NODE_H_ */
