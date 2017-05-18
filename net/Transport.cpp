#include "simulator/EmulNet.h"
#include "simulator/Member.h"
#include "simulator/Queue.h"
#include "Transport.h"

namespace net {

Transport::Transport(EmulNet *emulNet, queue<q_elt> *inQueue, Address address) {
    this->emulNet = emulNet;
    this->address = address;
    this->inQueue = inQueue;
}

static int enqueueMsgCallback(void *env, char *buff, int size) {
    return Queue::enqueue((queue<q_elt> *)env, (void *)buff, size);
}

bool Transport::drain() {
    return emulNet->ENrecv(&address, enqueueMsgCallback,
                           nullptr, 1, inQueue);
}

int Transport::send(Address remote, char *data, size_t len) {
    return emulNet->ENsend(&address, &remote, (char *)data, len);
}

bool Transport::pollnb() {
    return !inQueue->empty();
}

IOBuf Transport::recieve() {
    auto buf = IOBuf { nullptr, 0 };
    if (!inQueue->empty()) {
        auto msg = inQueue->front();
        inQueue->pop();
        buf = IOBuf { msg.elt, size_t(msg.size) };
    }
    return buf;
}

Address Transport::getAddress() {
    return address;
}


} // namespace net
