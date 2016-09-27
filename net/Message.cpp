#include "Message.h"

#include <cassert>
#include <string>
#include <vector>

using std::vector;
using std::to_string;

namespace dsproto {

Message::Message(uint8_t type, Address addr) {
    this->type = type;
    this->status = 0;
    this->replicaType = 0;
    this->transaction = 0;
    this->address = addr;
}

uint8_t Message::getType() const {
    return type;
}

void Message::setKeyValue(string key, string val) {
    this->key = move(key);
    this->value = move(val);
}

void Message::setKey(string key) {
    this->key = move(key);
}

const string& Message::getKey() const {
    return key;
}

const string& Message::getValue() const {
    return value;
}

void Message::setStatus(uint8_t status) {
    this->status = status;
}

uint8_t Message::getStatus() const {
    return status;
}

void Message::setTransaction(uint32_t transaction) {
    this->transaction = transaction;
}

uint32_t Message::getTransaction() const {
    return transaction;
}

Address Message::getAddress() const {
    return address;
}

string Message::str() const {
    static const char* typeRepr[] = {
        "CREATE", "READ", "UPDATE", "DELETE", "REPLY",  "READ_RSP", "DELETE_RSP", "VIEW_CHANGE"
    };
    static const char* statusRepr[] = { "OK", "FAIL" };

    auto append = [](string &lhs, const string &val) {
        lhs.append(val);
        lhs.append("::");
    };

    string repr;
    repr.reserve(200);
    append(repr, to_string(transaction));
    append(repr, address.str());
    append(repr, typeRepr[type]);
    append(repr, statusRepr[status]);
    if (key.size() > 0)
        append(repr, key);
    if (key.size() > 0 and value.size() > 0)
        append(repr, value);

    return repr;
}


vector<char> Message::serialize() {
    auto headerSize = sizeof(dsproto::Header);
    auto keySize = (uint32_t)  key.size() + 1;
    auto valSize = (uint32_t)value.size() + 1;
    auto payloadSize = (uint32_t)sizeof(keySize) + keySize +
                       (uint32_t)sizeof(valSize) + valSize;
    auto msgbuff = vector<char>(headerSize + payloadSize + 1, 0);

    bool addKey = keySize > 1;
    bool addVal = addKey && (valSize > 1);
    bool addStatus = true;
    bool addReplicaType = false;

    uint8_t flags =
        (addKey         ? dsproto::flags::KEY       : 0u) |
        (addVal         ? dsproto::flags::VAL       : 0u) |
        (addStatus      ? dsproto::flags::STATUS    : 0u) |
        (addReplicaType ? dsproto::flags::REPLICA   : 0u);

    uint16_t crc = 0;
    dsproto::Header header = dsproto::Header {
        dsproto::DsProto, dsproto::DsProtoVersion,
        type, flags, transaction,
        address.getIp(), address.getPort(),
        crc, payloadSize
    };
    memcpy(msgbuff.data(), (char *)&header, sizeof(header));

    auto *offset = msgbuff.data() + sizeof(dsproto::Header);
    auto addStringToken = [&](const string &str) {
        assert(str.size() < UINT32_MAX);
        auto strLen = (uint32_t)str.size() + 1;
        auto *strLenOffset = offset, *strValOffset = offset + sizeof(strLen);
        memcpy(strLenOffset, (char *)&strLen, sizeof(strLen));
        memcpy(strValOffset, (char *)str.data(), strLen);
        offset = offset + sizeof(strLen) + strLen;
    };
    auto addUint8Token = [&](const uint8_t val) {
        memcpy(offset, (char *)&val, sizeof(val));
        offset = offset + sizeof(val);
    };

    if (addStatus)  addUint8Token(status);
    if (addKey)     addStringToken(key);
    if (addVal)     addStringToken(value);

    return msgbuff;
}

Message Message::deserialize(char *data, size_t size) {
    dsproto::Header header;
    memcpy((char *)&header, data, sizeof(Header));

    auto msg =  Message {
        header.msgType, Address(header.id, header.port)
    };
    msg.transaction = header.transaction;

    auto hasKey     = !!(header.flags & dsproto::flags::KEY);
    auto hasValue   = !!(header.flags & dsproto::flags::VAL);
    auto hasStatus  = !!(header.flags & dsproto::flags::STATUS);
    auto hasReplica = !!(header.flags & dsproto::flags::REPLICA);

    auto *offset = data + sizeof(header);
    auto nextStringToken = [&](string &val) {
        uint32_t valSize;
        memcpy(&valSize, offset, sizeof(valSize));
        assert(size_t(offset - data) <= size);
        assert(size_t(offset - data + valSize) <= size);
        val += (offset + sizeof(valSize));
        offset += sizeof(valSize) + valSize;
    };
    auto nextUint8Token = [&](uint8_t &val) {
        memcpy(&val, offset, sizeof(val));
        offset += sizeof(val);
    };

    if (hasStatus) nextUint8Token(msg.status);
    if (hasKey)    nextStringToken(msg.key);
    if (hasValue)  nextStringToken(msg.value);

    return msg;
}

}
