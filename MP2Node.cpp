#include "MP2Node.h"
#include "Transport.h"
#include <utility>


namespace dsproto {

class Message {
public:
    using Header = dsproto::Header;

    Message() = default;
    Message(uint8_t type, Address addr) {
        this->type = type;
        this->status = 0;
        this->replicaType = 0;
        this->transaction = 0;
        this->address = addr;
    }

    void setKeyValue(string key, string val) {
        this->key = move(key);
        this->value = move(val);
    }

    void setTransaction(uint32_t transaction) {
        this->transaction = transaction;
    }

    Address getAddress() {
        return address;
    }

    string str() {
        static const char* typeRepr[] = {
            "CREATE", "READ", "UPDATE", "DELETE", "REPLY", "READREPLY"
        };
        auto append = [](string &lhs, const string &val) {
            lhs.append(val);
            lhs.append("::");
        };

        string repr;
        repr.reserve(200);
        append(repr, to_string(transaction));
        append(repr, address.str());
        append(repr, typeRepr[type]);
        if (key.size() > 0)
            append(repr, key);
        if (key.size() > 0 and value.size() > 0)
            append(repr, value);

        return repr;
    }

    vector<char> serialize() {
        vector<char> msgbuff;

        auto headerSize = sizeof(dsproto::Header);
        auto keySize = (uint32_t)key.size() + 1;
        auto valSize = (uint32_t)value.size() + 1;
        auto payloadSize = (uint32_t)sizeof(keySize) + keySize +
                           (uint32_t)sizeof(valSize) + valSize;
        msgbuff.resize(headerSize + payloadSize + 1, 0);

        bool addKey = keySize > 1;
        bool addVal = addKey && (valSize > 1);
        bool addStatus = false;
        bool addReplicaType = false;

        uint8_t flags =
            (addKey         ? dsproto::flags::KEY       : 0u) |
            (addVal         ? dsproto::flags::VAL       : 0u) |
            (addStatus      ? dsproto::flags::STATUS    : 0u) |
            (addReplicaType ? dsproto::flags::REPLICA   : 0u);

        auto *header = (dsproto::Header *)msgbuff.data();
        *header = dsproto::Header {
            dsproto::magic,
            dsproto::version,
            type,
            flags,
            transaction,
            address.getIp(),
            address.getPort(),
            0,
            payloadSize
        };

        auto *offset = msgbuff.data() + sizeof(dsproto::Header);
        auto addStringToken = [&](const string &val) {
            auto valSize = (uint32_t)val.size() + 1;
            memcpy(offset, (char *)&valSize, sizeof(valSize));
            memcpy(offset + sizeof(valSize), val.data(), valSize);
            offset += sizeof(valSize) + valSize;
        };

        if (addKey)
            addStringToken(key);
        if (addVal)
            addStringToken(value);

        return msgbuff;
    }

    static Message deserialize(char *data, size_t size) {
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
            assert(offset - data <= size);
            memcpy(&valSize, offset, sizeof(valSize));
            assert(offset - data + valSize <= size);
            val += (offset + sizeof(valSize));
            offset += sizeof(valSize) + valSize;
        };

        if (hasKey) {
            nextStringToken(msg.key);
        }
        if (hasValue) {
            nextStringToken(msg.value);
        }

        return msg;
    }

private:
    uint8_t type;
    uint8_t status;
    uint8_t replicaType;
    uint32_t transaction;
    string  key;
    string  value;
    Address address;

};

class Protocol {
    using Msg = Message;
public:
    Protocol(shared_ptr<net::Transport> net) {
        net_ = net;
    }

    void send(Address remote, Msg &msg) {
        auto msgbuff = msg.serialize();
        if (auto net = net_.lock()) {
            net->send(remote, msgbuff.data(), msgbuff.size());
        }
    }

    Msg recieve() {
        Msg msg;
        if (auto net = net_.lock()) {
            auto buf = net->recieve();
            msg = Msg::deserialize((char *)buf.data, buf.size);
            free(buf.data);
        }
        return msg;
    }

    Msg buildCreate(string &&key, string &&value) {
        Address addr;
        if (auto net = net_.lock())
            addr = net->getAddress();

        auto msg = Msg { dsproto::CREATE, addr };
        msg.setKeyValue(key, value);
        msg.setTransaction(++transaction);

        return msg;
    }


private:
    weak_ptr<net::Transport> net_;
    int32_t transaction = 0;
};

}

class Coordinator {
public:

    Coordinator(dsproto::Protocol *proto) {
        this->proto = proto;
    }

    void create(string &&key, string &&value) {
        auto msg = proto->buildCreate(move(key), move(value));
        //TODO self send, just for now
        proto->send(msg.getAddress(), msg);
        printf("send: %s\n", msg.str().data());
    }

private:
    dsproto::Protocol *proto;
};

class StoreNodeImpl {
    friend class StoreNode;

public:
    StoreNodeImpl(shared_ptr<Member> member,
                  shared_ptr<net::Transport> transport, Log *log)
            : member(member), transport(transport), proto(transport),
                coordinator(&proto), log(log) {
    }

    void create(string &&key, string &&value) {
        coordinator.create(move(key), move(value));
    }

    void read(const string &key) {
    }

    void update(const string &key, const string &value) {
    }

    void remove(const string &key) {
    }

    bool drainTransportLayer() {
        return transport->drain();
    }

    bool drainIngressQueue() {
        while (transport->pollnb()) {
            auto msg = proto.recieve();
            printf("recv: %s\n", msg.str().data());
        }
        return true;
    }


private:
    shared_ptr<Member>          member;
    shared_ptr<net::Transport>  transport;
    dsproto::Protocol           proto;
    Coordinator                 coordinator;
    Log                         *log;
};


/**
 * Constructs default implementation
 */
StoreNode::StoreNode(shared_ptr<Member> member, Params*,
                     EmulNet *emulNet, Log *log, Address *address)
        : StoreNode(new StoreNodeImpl{
            member,
            make_shared<net::Transport>(emulNet, &member->mp2q, *address),
            log
        }) {
}

/**
 * Dependency injection constructor
 */
StoreNode::StoreNode(StoreNodeImpl* impl) {
    this->impl = unique_ptr<StoreNodeImpl>(impl);
}

StoreNode::~StoreNode() {
}

/* Store Node Pimpl dispatchers */
void StoreNode::clientCreate(string key, string value) {
    return impl->create(move(key), move(value));
}

void StoreNode::clientRead(const string &key) {
    return impl->read(key);
}

void StoreNode::clientUpdate(const string &key, const string &value) {
    return impl->update(key, value);
}

void StoreNode::clientDelete(const string &key) {
    return impl->remove(key);
}

bool StoreNode::recvLoop() {
    return impl->drainTransportLayer();
}

void StoreNode::checkMessages() {
    impl->drainIngressQueue();
}

vector<Node> StoreNode::findNodes(const string &key) {
	return vector<Node>();
}

void StoreNode::updateRing() {
}

Member* StoreNode::getMemberNode() {
    return impl->member.get();
}
/* Store Node Pimpl dispatchers end */
