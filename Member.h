/**********************************
 * FILE NAME: Member.h
 *
 * DESCRIPTION: Definition of all Member related class
 **********************************/

#ifndef MEMBER_H_
#define MEMBER_H_

#include "stdincludes.h"

#include <memory>
using std::unique_ptr;


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

/**
 * CLASS NAME: Address
 *
 * DESCRIPTION: Class representing the address of a single node
 */
class Address {
public:
	char addr[6];
	Address() {}
	// Copy constructor
	Address(const Address &anotherAddress);
	 // Overloaded = operator
	Address& operator =(const Address &anotherAddress);
	bool operator ==(const Address &anotherAddress);
	Address(string address) {
		size_t pos = address.find(":");
		int id = stoi(address.substr(0, pos));
		short port = (short)stoi(address.substr(pos + 1, address.size()-pos-1));
		memcpy(&addr[0], &id, sizeof(int));
		memcpy(&addr[4], &port, sizeof(short));
	}
	string getAddress() {
		int id = 0;
		short port;
		memcpy(&id, &addr[0], sizeof(int));
		memcpy(&port, &addr[4], sizeof(short));
		return to_string(id) + ":" + to_string(port);
	}
	void init() {
		memset(&addr, 0, sizeof(addr));
	}
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
    // using MemberList = vector<MemberPtr>;
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

	Member()                                       = default;
	Member(const Member &)                         = default;
	Member& operator=(const Member &anotherMember) = default;
	virtual ~Member() {}
};

#endif /* MEMBER_H_ */
