#ifndef MESSAGE_H_
#define MESSAGE_H_

#include <memory>
#include <string>
#include <vector>

#include "Address.h"
#include "net/Transport.h"
// using std::string;
// using std::vector;

#undef _packed_
#define _packed_ __attribute__((packed, aligned(2)))

namespace dsproto {

enum MsgType { CREATE, READ, UPDATE, DELETE, REPLY,
    CREATE_RSP, READ_RSP, DELETE_RSP, UPDATE_RSP, VIEW_CHANGE,
    SYNC_BEGIN, SYNC_END };

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
    static const uint8_t COMMAND = 0b00001000;
}

static const uint8_t DsProto        = 0xDB;
static const uint8_t DsProtoVersion = 0x01;

static const uint8_t VsProto        = 0x3B;
static const uint8_t VsProtoVersion = 0x01;

using KeyValueList = std::vector<tuple<std::string, std::string>>;

class Message {
public:

    Message() {};
    Message(uint8_t type, Address addr);

    uint8_t getType() const;

    void setKeyValue(std::string key, std::string val);
    void setKey(std::string key);
    const string& getKey() const;
    const string& getValue() const;

    void setStatus(uint8_t status);
    uint8_t getStatus() const;

    void setTransaction(uint32_t transaction);
    uint32_t getTransaction() const;

    void addSyncKeyValue(std::string key, std::string value);
    KeyValueList& getSyncKeyValues();

    Address getAddress() const;

    std::string str() const;
    std::vector<char> serialize();
    static Message deserialize(char *data, size_t size);

private:

    uint8_t  type;
    uint8_t  status = OK;
    uint8_t  replicaType;
    uint32_t transaction;
    std::string   key;
    std::string   value;
    KeyValueList  syncKeyValues;
    Address       address;
};


class MessageStream {
    using Msg = Message;
public:
    MessageStream(std::shared_ptr<net::Transport> net) : transport(net) {
        addr = transport->getAddress();
    }

    void send(Address remote, Msg &msg) {
        auto msgbuff = msg.serialize();
        transport->send(remote, msgbuff.data(), msgbuff.size());
    }

    bool recieveMessages() {
        return transport->drain();
    }

    Msg dequeue() {
        auto buf = transport->recieve();
        auto msg = Msg::deserialize((char *)buf.data, buf.size);
        free(buf.data);
        return msg;
    }

    Address getLocalAddress() {
        return addr;
    }

    bool empty() {
        return !transport->pollnb();
    }

private:
    Address                          addr;
    std::shared_ptr<net::Transport>  transport;
    int32_t                          transaction = 0;
};

}

#endif
