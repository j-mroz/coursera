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
 * Compare two Address objects
 * Return true/non-zero if they have the same ip address and port number
 * Return false/zero if they are different
 */
