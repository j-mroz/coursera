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

#include <functional>
#include <memory>
#include <set>
#include <unordered_set>
#include <unordered_map>

/**
 * Macros
 */
#define TREMOVE 20
#define TFAIL 5

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

struct AddMembersRequest {
    int16_t msgType;
    int32_t id;
    int16_t port;
    int64_t heartbeat;
    uint64_t membersCount;
};

struct Heartbeat {
    int16_t msgType;
    int32_t id;
    int16_t port;
    int64_t heartbeat;
};

// Hashing forward declarations
bool operator==(const MemberListEntry& lhs, const MemberListEntry& rhs);
struct MemberListEntryHash {
    size_t operator()(const MemberListEntry& member) const;
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
    using MembersSet = std::unordered_set<MemberListEntry, MemberListEntryHash>;
    using MembersMap = std::unordered_map<int64_t, MemberListEntry>;

	MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
    virtual ~MP1Node();

// Handlers API
    int32_t             getId();
    int16_t             getPort();
    int64_t             getHeartbeat();
    long                getTimestamp();
	Member*             getMemberNode();
    MemberListEntry&    getCachedEntry(MemberListEntry& entry);
    MembersList&        getMembersList();
    MembersMap&         getMembersMap();
    MembersSet&         getFailedMembers();


// Emulator API
    int     init();
    int     join(Address *joinAddress);
    int     finishUpThisNode();
    void    nodeStart(char *servaddrstr, short serverport);
    void    nodeLoop();
    int     recvLoop();

    int     send(Address addr, char *data, size_t len);
    void    eraseCached(MemberListEntry &member);
    void    removeFailed(const MemberListEntry &member);

private:
    void    drainIngressQueue();
    void    runTasks();

    int     handleRequest(void *env, char *data, int size);
    void    handleJoinRequest(void *data, size_t size);
    void    handleJoinResponse(void *data, size_t size);
    void    handleAddMembersRequest(void *data, size_t size);
    void    handleHeartbeat(void *rawReq, size_t size);

    void    handleMembersData(char *buff, uint64_t count);



private:
    void    addMemberEntry(const MemberData &member);

    void    advanceHeartbeat();
    void    advanceTimestamp();

    Address getJoinAddress();

private:
    using TasksList = vector<unique_ptr<Task>>;

    EmulNet     *emulNet;
    Log         *log;
    Params      *par;
    Member      *memberNode;
    MembersMap  peersCache;
    MembersSet  failedPeers;
    bool        peersChangeDetected = false;
    long        timestamp = 0;
    TasksList   tasks;
};

#endif /* _MP1NODE_H_ */
