#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "stdincludes.h"
#include "Member.h"
#include <string>
using std::string;

#undef _packed_
#define _packed_ __attribute__((packed, aligned(2)))

namespace dsproto {

enum MsgType { CREATE, READ, UPDATE, DELETE, REPLY, CREATE_RSP, READ_RSP, DELETE_RSP, UPDATE_RSP, VIEW_CHANGE };

enum ReqStatus { OK, FAIL };

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

static const uint8_t DsProto        = 0xDB;
static const uint8_t DsProtoVersion = 0x01;

static const uint8_t VsProto        = 0x3B;
static const uint8_t VsProtoVersion = 0x01;



class Message {
public:
    Message() {};
    Message(uint8_t type, Address addr);

    uint8_t getType() const;

    void setKeyValue(string key, string val);
    void setKey(string key);
    const string& getKey() const;
    const string& getValue() const;

    void setStatus(uint8_t status);
    uint8_t getStatus() const;

    void setTransaction(uint32_t transaction);
    uint32_t getTransaction() const;

    Address getAddress() const;

    string str() const;
    vector<char> serialize();
    static Message deserialize(char *data, size_t size);

private:
    uint8_t  type;
    uint8_t  status = OK;
    uint8_t  replicaType;
    uint32_t transaction;
    string   key;
    string   value;
    Address  address;
};



}

#endif
