/**********************************
 * FILE NAME: Member.cpp
 *
 * DESCRIPTION: Definition of all Member related class
 **********************************/

#include "Member.h"

/**
 * Constructor
 */
q_elt::q_elt(void *elt, int size): elt(elt), size(size) {}

/**
 * Copy constructor
 */
Address::Address(const Address &anotherAddress) {
	memcpy(&addr, &anotherAddress.addr, sizeof(addr));
}

/**
 * Assignment operator overloading
 */
Address& Address::operator =(const Address& anotherAddress) {
    memcpy(&addr, &anotherAddress.addr, sizeof(addr));
	return *this;
}

/**
 * Compare two Address objects
 * Return true/non-zero if they have the same ip address and port number
 * Return false/zero if they are different
 */
bool Address::operator ==(const Address& anotherAddress) {
	return !memcmp(this->addr, anotherAddress.addr, sizeof(this->addr));
}
