/**********************************
 * FILE NAME: Member.h
 *
 * DESCRIPTION: Definition of all Member related class
 **********************************/

#ifndef MEMBER_H_
#define MEMBER_H_

#include "stdincludes.h"
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

/**
 * CLASS NAME: Address
 *
 * DESCRIPTION: Class representing the address of a single node
 */
class Address {
public:
	char addr[6] = {0, 0, 0, 0, 0, 0};

	Address()                  = default;
	Address(const Address&)    = default;
	Address(const string &address) {
		size_t pos = address.find(":");
		uint32_t id   = (uint32_t)stoul(address.substr(0, pos));
		uint16_t port = (uint16_t)stoul(address.substr(pos + 1));
		memcpy(&addr[0], (char *)&id, sizeof(uint32_t));
		memcpy(&addr[4], (char *)&port, sizeof(uint16_t));
	}
	Address(int32_t id, int16_t port) {
		memcpy(&addr[0], (char *)&id, sizeof(uint32_t));
		memcpy(&addr[4], (char *)&port, sizeof(uint16_t));
	}
	Address& operator=(const Address&) = default;
	bool operator==(const Address &);

	string getAddress() const {
		return to_string(getIp()) + ":" + to_string(getPort());
	}

	string str() const {
		return string(inet_ntoa(in_addr{getIp()})) + ":" + to_string(getPort());
	}


	uint16_t getPort() const {
		uint16_t port;
		memcpy(&port, addr + 4, sizeof(uint16_t));
		return port;
	}

	uint32_t getIp() const {
		uint32_t ip;
		memcpy(&ip, addr, sizeof(uint32_t));
		return ip;
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
