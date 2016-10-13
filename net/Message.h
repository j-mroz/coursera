#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "Address.h"
#include "net/Transport.h"


#include "protocol/dht_proto_constants.h"
#include "protocol/dht_proto_types.h"
#include "protocol/dht_proto_types.tcc"

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TCompactProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include <boost/make_shared.hpp>

#include <memory>
#include <string>
#include <vector>


namespace proto {
namespace dht {

using namespace apache::thrift;
using namespace apache::thrift::transport;
using namespace apache::thrift::protocol;

using KeyValueList = std::vector<tuple<std::string, std::string>>;

inline int32_t decodeAsIp4(const string &bytes) {
    assert(bytes.size() == 16);
    assert(bytes[10] == (char)0xFF);
    assert(bytes[11] == (char)0xFF);

    //TODO network bytes order!
    auto ip4 = uint32_t(0);
    memcpy((char *)&ip4, bytes.data() + 12, sizeof(uint32_t));
    return ip4;
}

inline string encodeAsIp6(uint32_t ip4) {
    //TODO network byte order!
    auto *ip4Bytes = (char *)&ip4;
    auto ip6Bytes = string(16, 0);
    ip6Bytes[10] = (char)0xFF;
    ip6Bytes[11] = (char)0xFF;
    ip6Bytes[12] = ip4Bytes[0];
    ip6Bytes[13] = ip4Bytes[1];
    ip6Bytes[14] = ip4Bytes[2];
    ip6Bytes[15] = ip4Bytes[3];
    return ip6Bytes;
}

class MessageQueue {
    using Msg = proto::dht::Message;
    using Protocol = TCompactProtocol;
    using MemoryBufferPtr = boost::shared_ptr<TMemoryBuffer>;
    using ProtocolPtr = boost::shared_ptr<TCompactProtocol>;
    using TransportPtr = std::shared_ptr<net::Transport>;

public:
    MessageQueue(std::shared_ptr<net::Transport> net)
        : transport(net),
          inputBuffer(boost::make_shared<TMemoryBuffer>(nullptr, 0)),
          outputBuffer(boost::make_shared<TMemoryBuffer>()),
          inputProtocol(boost::make_shared<Protocol>(inputBuffer)),
          outputProtocol(boost::make_shared<Protocol>(outputBuffer)) {
        addr = transport->getAddress();
    }

    void send(Address remote, Msg &msg) {
        outputBuffer->resetBuffer();
        msg.write(outputProtocol.get());
        auto size = outputBuffer->available_read();
        auto *buf = outputBuffer->borrow(nullptr, &size);
        transport->send(remote, (char*)buf, size);
    }

    bool recieveMessages() {
        return transport->drain();
    }

    Msg dequeue() {
        auto iobuf = transport->recieve();
        inputBuffer->resetBuffer((uint8_t *)iobuf.data, iobuf.size);

        auto msg = proto::dht::Message();
        msg.read(inputProtocol.get());

        free(iobuf.data);
        return msg;
    }

    Address getLocalAddress() {
        return addr;
    }

    bool empty() {
        return !transport->pollnb();
    }

private:
    TransportPtr        transport;
    MemoryBufferPtr     inputBuffer;
    MemoryBufferPtr     outputBuffer;
    ProtocolPtr         inputProtocol;
    ProtocolPtr         outputProtocol;
    Address             addr;
};

}
}

#endif
