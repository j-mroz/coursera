/**********************************
 * FILE NAME: Member.h
 *
 * DESCRIPTION: Definition of all Member related class
 **********************************/

#ifndef MEMBER_H_
#define MEMBER_H_

#include "stdincludes.h"
#include "net/Address.h"
#include <arpa/inet.h>
/**
 * CLASS NAME: q_elt
 *
 * DESCRIPTION: Entry in the queue
 */
class q_elt {
public:
	void *elt;
	int size;
	q_elt(void *elt, int size);
};


struct MemberListEntry {
	int32_t id;
	int16_t port;
	int64_t heartbeat;
	int64_t timestamp;
};


/**
 * Class representing a member in the distributed system
 */
class Member {
public:
	using MemberList = vector<MemberListEntry>;

	// This member's Address
	Address addr;
	// boolean indicating if this member is up
	bool inited        = false;
	// boolean indicating if this member is in the group
	bool inGroup       = false;
	// boolean indicating if this member has failed
	bool bFailed       = false;
	// number of my neighbors
	int nnb            = 0;
	// the node's own heartbeat
	long heartbeat     = 0;
	int timestamp      = 0;
	// Membership table
	MemberList memberList;
	// vector<MemberListEntry> memberList;
	// My position in the membership table
	//vector<MemberListEntry>::iterator myPos;
	// Queue for failure detection messages
	queue<q_elt> mp1q;
	// Queue for KVstore messages
	queue<q_elt> mp2q;

	Member()                                       = default;
	Member(const Member &)                         = default;
	Member& operator=(const Member &anotherMember) = default;
	virtual ~Member() {}
};

#endif /* MEMBER_H_ */
