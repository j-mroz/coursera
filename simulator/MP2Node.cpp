#include "MP2Node.h"
#include "Log.h"
#include "net/Message.h"
#include "net/Transport.h"
#include "service/DistributedHashTable.h"

#include <algorithm>
#include <set>
#include <utility>
#include <unordered_map>


class MembershipServiceAdapter : public MembershipServiceIface {
    AddressList addrList;
public:

    MembershipServiceAdapter(shared_ptr<Member> member) {
        this->member = member;
    }

    // TODO: not efficient!
    const AddressList& getMembersList() override {
        addrList.clear();
        for (auto member : member->memberList) {
            addrList.push_back(Address(member.id, member.port));
        }
        return addrList;
    }

    Address getLocalAddres() override {
        return member->addr;
    }

private:
    shared_ptr<Member> member;
};


/**
 * Constructs default implementation
 */
DSNode::DSNode(shared_ptr<Member> member, Params*,
               EmulNet *emulNet, Log *log, Address *local) {
    this->member = member.get();
    auto transport = make_shared<net::Transport>(emulNet, &member->mp2q, *local);
    auto msgStream = make_shared<dsproto::MessageStream>(transport);
    auto membershipAdapter = make_shared<MembershipServiceAdapter>(member);
    this->impl = unique_ptr<DistributedHashTableService>(
        new DistributedHashTableService(membershipAdapter, msgStream, log));
}


DSNode::~DSNode() {
    // printf("~DSNode\n");
}

/* Store Node Pimpl dispatchers */
void DSNode::clientCreate(string key, string value) {
    return impl->create(move(key), move(value));
}

void DSNode::clientRead(const string &key) {
    return impl->read(key);
}

void DSNode::clientUpdate(string key, string value) {
    return impl->update(move(key), move(value));
}

void DSNode::clientDelete(const string &key) {
    return impl->remove(key);
}

bool DSNode::recvLoop() {
    return impl->recieveMessages();
}

void DSNode::checkMessages() {
    impl->processMessages();
}

vector<Node> DSNode::findNodes(const string &key) {
    auto addrList = impl->getNaturalNodes(key);
    auto nodeList = NodeList(addrList.size());
    for (auto i = 0ul; i < addrList.size(); ++i)
        nodeList[i].setAddress(addrList[i]);
    return nodeList;
}

void DSNode::updateRing() {
    impl->updateCluster();
}

Member* DSNode::getMemberNode() {
    return member;
}
/* Store Node Pimpl dispatchers end */
