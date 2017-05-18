#ifndef DHT_H_
#define DHT_H_

#include <memory>
#include <vector>
#include "net/Address.h"
#include "net/Message.h"
#include "net/Transport.h"
#include "simulator/Member.h"

class Log;

using MembersList = std::vector<MemberListEntry>;
using AddressList = std::vector<Address>;
// using Message = dsproto::Message;
using proto::dht::Message;
using proto::dht::MessageQueue;


class MembershipServiceIface {
public:
    virtual const AddressList& getMembersList() = 0;
    virtual Address getLocalAddress()           = 0;
};
using MembershipProxy = shared_ptr<MembershipServiceIface>;


/******************************************************************************
 * Replication strategy - responsible for backend jobs
 ******************************************************************************/
class DHTBackend {
public:
    DHTBackend()          = default;
    virtual ~DHTBackend() = default;
    virtual AddressList getNaturalNodes(const string&)  = 0;
    virtual void updateCluster()                        = 0;
    virtual bool probe(const Message &msg)              = 0;
    virtual void handle(Message &msg)                   = 0;
};


/******************************************************************************
 * Coordination strategy - used for clients coordination
 ******************************************************************************/
class DHTCoordinator {
public:
    DHTCoordinator()          = default;
    virtual ~DHTCoordinator() = default;

    virtual void   create(string &&key, string &&value) = 0;
    virtual string read(const string &key)              = 0;
    virtual void   update(string &&key, string &&value) = 0;
    virtual void   remove(const string &key)            = 0;
    virtual bool   probe(Message &msg)                  = 0;
    virtual void   handle(Message &msg)                 = 0;
    virtual void   onClusterUpdate()                    = 0;
};

/******************************************************************************
 * Distributed store service implementation
 ******************************************************************************/
class DistributedHashTableService {
public:
    DistributedHashTableService(MembershipProxy membershipProxy,
        shared_ptr<MessageQueue> msgQueue, Log *log);

    void create(string &&key, string &&value);
    void read(const string &key);
    void update(string &&key, string &&value);
    void remove(const string &key);
    bool recieveMessages();
    bool processMessages();
    void updateCluster();
    AddressList getNaturalNodes(const string &key);

private:
    shared_ptr<MessageQueue>    msgQueue;
    shared_ptr<DHTBackend>      backend;
    unique_ptr<DHTCoordinator>  coordinator;
    Log                         *log;
};

#endif
