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

void Message::addSyncKeyValue(std::string key, std::string value) {
    syncKeyValues.push_back(make_tuple(move(key), move(value)));
}

KeyValueList& Message::getSyncKeyValues() {
    return syncKeyValues;
}

Address Message::getAddress() const {
    return address;
}

string Message::str() const {
    static const char* typeRepr[] = {
        "CREATE", "READ", "UPDATE", "DELETE", "REPLY", "CREATE_RSP",
        "READ_RSP", "DELETE_RSP", "UPDATE_RSP", "VIEW_CHANGE"
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

template<typename T>
char* serializeValue(char *buffer, const T &val) {
    memcpy(buffer, (char *)&val, sizeof(T));
    return buffer + sizeof(T);
};


template<>
char* serializeValue<string>(char *buffer, const string &str) {
    assert(str.size() < UINT32_MAX);

    auto strLen = (uint32_t)str.size(); /* + 1 <-that was screwing things*/;
    auto *strLenOffset = buffer, *strValOffset = buffer + sizeof(strLen);
    memcpy(strLenOffset, (char *)&strLen, sizeof(strLen));
    memcpy(strValOffset, (char *)str.data(), strLen);

    return buffer + sizeof(strLen) + strLen;
};

template<typename T>
T deserializeValue(char * &buffer, char *fence) {
    assert(buffer + sizeof(T) <= fence);

    T val;
    memcpy(&val, buffer, sizeof(val));
    buffer += sizeof(val);

    return val;
};

template<>
string deserializeValue<string>(char * &buffer, char *fence) {
    auto valSize = deserializeValue<uint32_t>(buffer, fence);
    assert(buffer + valSize <= fence);

    auto val = string(buffer, (size_t)valSize);
    buffer += valSize;

    return val;
}


vector<char> Message::serialize() {
    bool addKey = key.size() > 0;
    bool addVal = addKey && (value.size() > 0);
    bool addStatus = true;
    bool addReplica = syncKeyValues.size() > 0;

    auto headerSize = sizeof(dsproto::Header);
    auto keySize = addKey ? (uint32_t)  key.size() + 1 : 0;
    auto valSize = addVal ? (uint32_t)value.size() + 1 : 0;
    auto replicaSize = uint32_t(0);
    for (auto &kv : syncKeyValues) {
        replicaSize += sizeof(uint32_t) * 2;
        replicaSize += get<0>(kv).size() + 1;
        replicaSize += get<1>(kv).size() + 1;
    }
    assert(replicaSize < UINT32_MAX);

    auto payloadSize =
        (addKey      ? ((uint32_t)sizeof(keySize)     + keySize)      : 0) +
        (addVal      ? ((uint32_t)sizeof(valSize)     + valSize)      : 0) +
        (addReplica  ? ((uint32_t)sizeof(replicaSize) + replicaSize)  : 0) + 1;

    auto msgbuff = vector<char>(headerSize + payloadSize + 1, 0);

    uint8_t flags =
        (addKey         ? dsproto::flags::KEY       : 0u) |
        (addVal         ? dsproto::flags::VAL       : 0u) |
        (addStatus      ? dsproto::flags::STATUS    : 0u) |
        (addReplica     ? dsproto::flags::REPLICA   : 0u);

    uint16_t crc = 0;
    dsproto::Header header = dsproto::Header {
        dsproto::DsProto, dsproto::DsProtoVersion,
        type, flags, transaction,
        address.getIp(), address.getPort(),
        crc, payloadSize
    };
    memcpy(msgbuff.data(), (char *)&header, sizeof(header));

    auto *offset = msgbuff.data() + sizeof(dsproto::Header);

    if (addStatus)
        offset = serializeValue<uint8_t>(offset, status);
    if (addKey)
        offset = serializeValue<string>(offset, key);
    if (addVal)
        offset = serializeValue<string>(offset, value);
    if (addReplica) {
        assert(syncKeyValues.size() <= UINT32_MAX);
        offset = serializeValue<uint32_t>(offset, (uint32_t)syncKeyValues.size());
        for (auto &kv : syncKeyValues) {
            offset = serializeValue<string>(offset, get<0>(kv));
            offset = serializeValue<string>(offset, get<1>(kv));
        }
    }

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

    if (hasStatus)
        msg.status = deserializeValue<uint8_t>(offset, data + size);
    if (hasKey)
        msg.key = deserializeValue<string>(offset, data + size);
    if (hasValue)
        msg.value = deserializeValue<string>(offset, data + size);
    if (hasReplica) {
        auto keyValCount = deserializeValue<uint32_t>(offset, data + size);
        for (auto i = 0ul; i < keyValCount; ++i) {
            auto key = deserializeValue<string>(offset, data + size);
            auto val = deserializeValue<string>(offset, data + size);
            msg.addSyncKeyValue(move(key), move(val));
        }
    }

    return msg;
}

}
