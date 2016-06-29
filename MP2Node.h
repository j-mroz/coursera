/**********************************
 * FILE NAME: MP2Node.h
 *
 * DESCRIPTION: MP2Node class header file
 **********************************/

#ifndef MP2NODE_H_
#define MP2NODE_H_

/**
 * Header files
 */
#include "stdincludes.h"
#include "EmulNet.h"
#include "Node.h"
#include "HashTable.h"
#include "Log.h"
#include "Params.h"
#include "Message.h"
#include "Queue.h"

#include <memory>

using std::unique_ptr;
using NodeList = vector<Node>;


// proto::version::req_type::transaction::addr::?key::?value::?success
// transID::fromAddr::CREATE::key::value::ReplicaType
// transID::fromAddr::READ::key
// transID::fromAddr::UPDATE::key::value::ReplicaType
// transID::fromAddr::DELETE::key
// transID::fromAddr::REPLY::sucess
// transID::fromAddr::READREPLY::value
// Header = proto::version::req_type::transaction::addr
// Payload =

#define _packed_ __attribute__((packed, aligned(2)))


namespace dsproto {

    enum MsgType {CREATE, READ, UPDATE, DELETE, REPLY, READREPLY};

    struct _packed_ Header {
        uint8_t  proto;
        uint8_t  version;
        uint8_t  msgType;
        uint8_t  flags;
        uint32_t transaction;
        uint32_t id;
        uint16_t port;
        uint16_t crc;
        uint32_t payloadSize;
    };

    namespace flags {
        static const uint8_t KEY     = 0b10000000;
        static const uint8_t VAL     = 0b01000000;
        static const uint8_t STATUS  = 0b00100000;
        static const uint8_t REPLICA = 0b00010000;
    }

    static const uint8_t magic   = 0xDB;
    static const uint8_t version = 0x01;
}


class StoreNodeImpl;

class StoreNode {
public:
    StoreNode(shared_ptr<Member>, Params*, EmulNet*, Log*, Address*);
    StoreNode(StoreNodeImpl* impl);
    virtual ~StoreNode();

    // Client side CRUD APIs
    void        clientCreate(string key, string value);
    void        clientRead  (const string &key);
    void        clientUpdate(const string &key, const string &value);
    void        clientDelete(const string &key);

    // Emulnet and Appliction API
    bool        recvLoop();
    void        checkMessages();
    NodeList    findNodes(const string &key);
    void        updateRing();
    Member*     getMemberNode();

private:
    unique_ptr<StoreNodeImpl> impl;
};

using MP2Node = StoreNode;

#endif /* MP2NODE_H_ */
