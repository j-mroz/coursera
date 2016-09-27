#ifndef TRANSPORT_H_
#define TRANSPORT_H_

#include "simulator/Member.h"

#include <queue>
using std::queue;

struct IOBuf {
    void    *data;
    size_t  size;
};

namespace net {

class Transport {
public:
    Transport(EmulNet *emulNet, queue<q_elt> *inQueue, Address address);
    bool    drain();
    bool    pollnb();
    int     send(Address remote, char *data, size_t len);
    IOBuf   recieve();
    Address getAddress();
private:
    EmulNet         *emulNet;
    queue<q_elt>    *inQueue;
    Address         address;
};

} // namespace net

#endif
