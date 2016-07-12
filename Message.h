#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "stdincludes.h"
#include "Member.h"
#include <string>
using std::string;

#undef _packed_
#define _packed_ __attribute__((packed, aligned(2)))

namespace dsproto {

enum MsgType { CREATE, READ, UPDATE, DELETE, REPLY, READREPLY };

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


class Message {
public:
    Message() = default;
    Message(uint8_t type, Address addr);

    void setKeyValue(string key, string val);
    void setTransaction(uint32_t transaction);
    Address getAddress();
    string str();
    vector<char> serialize();
    static Message deserialize(char *data, size_t size);

private:
    uint8_t  type;
    uint8_t  status;
    uint8_t  replicaType;
    uint32_t transaction;
    string   key;
    string   value;
    Address  address;
};
}

#endif
