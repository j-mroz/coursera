#ifndef COMMON_H_
#define COMMON_H_

/**
 * Global variable
 */

// message types, reply is the message from node to coordinator
enum MessageType {CREATE, READ, UPDATE, DELETE, REPLY, READREPLY};
// enum of replica types
enum ReplicaType {PRIMARY, SECONDARY, TERTIARY};

#endif
