#include "MP2Node.h"
#include "Transport.h"
#include <algorithm>
#include <utility>


namespace dsproto {

class Protocol {
    using Msg = Message;
public:
    Protocol(shared_ptr<net::Transport> net) : transport(net) {
        addr = transport->getAddress();
    }

    void send(Address remote, Msg &msg) {
        auto msgbuff = msg.serialize();
        transport->send(remote, msgbuff.data(), msgbuff.size());
    }

    void sendCreate(Address remote, string &&key, string &&value) {
        auto msg = Msg { dsproto::CREATE, addr };
        msg.setKeyValue(key, value);
        msg.setTransaction(++transaction);
        send(remote, msg);
    }

    Msg recieve() {
        auto buf = transport->recieve();
        auto msg = Msg::deserialize((char *)buf.data, buf.size);
        free(buf.data);
        return msg;
    }

    Address getLocalAddres() {
        return addr;
    }

    shared_ptr<net::Transport> getTransport() {
        return transport;
    }

private:
    Address                     addr;
    shared_ptr<net::Transport>  transport;
    int32_t                     transaction = 0;
};

}

static int64_t addressHash(int32_t id, int16_t port) {
    return ((int64_t)id << 32) + port;
}

class RingPos {
public:
    RingPos() = default;

    RingPos(const Address &addr) {
        hash = (uint64_t)addressHash(addr.getIp(), addr.getPort());
        hash = (hash * 2654435761ul) >> 32;
        hash %= (RING_SIZE);
    }

    RingPos(const string &key) {
        static std::hash<string> hashString;
        hash = (hashString(key) * 2654435761ul) >> 32;
        hash %= (RING_SIZE);
    }

    uint64_t getHash() const {
        return hash;
    }

private:
    uint64_t hash;
};

class RingClusterInfo {
public:
    using MembersList = vector<MemberListEntry>;

    RingClusterInfo(shared_ptr<Member> member) {
        this->member = member;
    }

    MembersList& getMembersList() {
        return member->memberList;
    }

private:
    shared_ptr<Member> member;
};


class ReplicationStrategy {
public:
    using MembersList = vector<MemberListEntry>;

    ReplicationStrategy()          = default;
    virtual ~ReplicationStrategy() = default;
    virtual MembersList getNaturalNodes(const string&) = 0;
};

template<typename ClusterInfo>
class SimpleReplicationStrategy : public ReplicationStrategy {
public:
    SimpleReplicationStrategy(unique_ptr<ClusterInfo> &&clusterInfo,
                              size_t replicationFactor) {
        this->replicationFactor = replicationFactor;
        this->clusterInfo = move(clusterInfo);
    }
    virtual ~SimpleReplicationStrategy() = default;

    MembersList getNaturalNodes(const string &key) override {
        using OrderedMemberIndex = tuple<RingPos, size_t>;
        enum OrderedMemberIndexFields { RingPosIdx, ListPosIdx };
        auto &members = clusterInfo->getMembersList();

        auto orderedMembers = vector<OrderedMemberIndex>(members.size());
        for (auto entryIdx = 0ul; entryIdx < members.size(); ++entryIdx) {
            auto addr = Address(members[entryIdx].id, members[entryIdx].port);
            orderedMembers[entryIdx] = make_tuple(RingPos(addr), entryIdx);
        }

        auto compareMembers = [](const OrderedMemberIndex &l,
                                 const OrderedMemberIndex &r) {
            return get<RingPosIdx>(l).getHash() < get<RingPosIdx>(r).getHash();
        };

        auto keyRingPos = make_tuple(RingPos(key), 0);
        sort(orderedMembers.begin(), orderedMembers.end(), compareMembers);
        auto startNode = lower_bound(orderedMembers.begin(), orderedMembers.end(),
                                     keyRingPos, compareMembers);

        auto naturalNodes = MembersList();
        naturalNodes.reserve(replicationFactor);
        auto node = startNode;
        do {
            if (node == orderedMembers.end())
                node = orderedMembers.begin();
            naturalNodes.push_back(members[get<ListPosIdx>(*node)]);
        } while (naturalNodes.size() < replicationFactor && ++node != startNode);

        return naturalNodes;
    }

private:
    size_t                  replicationFactor;
    unique_ptr<ClusterInfo> clusterInfo;
};

class Coordinator {
public:
    Coordinator(shared_ptr<dsproto::Protocol> proto,
                shared_ptr<ReplicationStrategy> replictor) {
        this->proto = proto;
        this->replictor = replictor;
    }

    void create(string &&key, string &&value) {
        auto nodes = replictor->getNaturalNodes(key);
        if (nodes.size() == 0)
            return;

        auto remote = Address(nodes[0].id, nodes[0].port);
        if (remote == proto->getLocalAddres()) {
            printf("self send!, returning\n");
            return;
        }
        proto->sendCreate(remote, move(key), move(value));
    }

private:
    shared_ptr<dsproto::Protocol>   proto;
    shared_ptr<ReplicationStrategy> replictor;
};

class DSService {
    friend class DSNode;
    // using ReplicationStrategyPtr = shared_ptr<ReplicationStrategy<RingClusterInfo>>;
    // using CoordinatorPtr = unique_ptr<Coordinator>;

public:
    DSService(shared_ptr<Member> member, shared_ptr<dsproto::Protocol> proto,
              Log *log) {
        this->member = member;
        this->proto = proto;
        transport = proto->getTransport();

        using MyReplicationStrategy = SimpleReplicationStrategy<RingClusterInfo>;
        static const size_t REPLICATION_FACTOR = 3;

        auto clusterInfo = unique_ptr<RingClusterInfo>(
                new RingClusterInfo(member));
        replicator = make_shared<MyReplicationStrategy>(
                move(clusterInfo), REPLICATION_FACTOR);
        coordinator = unique_ptr<Coordinator>(
                new Coordinator(proto, replicator));

        log = log;
    }

    void create(string &&key, string &&value) {
        coordinator->create(move(key), move(value));
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
            auto msg = proto->recieve();
            printf("recv: %s\n", msg.str().data());
        }
        return true;
    }


private:
    shared_ptr<Member>              member;
    shared_ptr<dsproto::Protocol>   proto;
    shared_ptr<net::Transport>      transport;
    shared_ptr<ReplicationStrategy> replicator;
    unique_ptr<Coordinator>         coordinator;
    Log                             *log;
};


/**
 * Constructs default implementation
 */
DSNode::DSNode(shared_ptr<Member> member, Params*,
              EmulNet *emulNet, Log *log, Address *local) {
    auto transport = make_shared<net::Transport>(emulNet, &member->mp2q, *local);
    auto proto = make_shared<dsproto::Protocol>(transport);
    this->impl = unique_ptr<DSService>(new DSService(member, proto, log));
}

/**
 * Dependency injection constructor
 */
DSNode::DSNode(DSService* impl) {
    this->impl = unique_ptr<DSService>(impl);
}

DSNode::~DSNode() {
}

/* Store Node Pimpl dispatchers */
void DSNode::clientCreate(string key, string value) {
    return impl->create(move(key), move(value));
}

void DSNode::clientRead(const string &key) {
    return impl->read(key);
}

void DSNode::clientUpdate(const string &key, const string &value) {
    return impl->update(key, value);
}

void DSNode::clientDelete(const string &key) {
    return impl->remove(key);
}

bool DSNode::recvLoop() {
    return impl->drainTransportLayer();
}

void DSNode::checkMessages() {
    impl->drainIngressQueue();
}

vector<Node> DSNode::findNodes(const string &key) {
	return vector<Node>();
}

void DSNode::updateRing() {
}

Member* DSNode::getMemberNode() {
    return impl->member.get();
}
/* Store Node Pimpl dispatchers end */
