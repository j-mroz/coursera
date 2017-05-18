/**********************************
 * FILE NAME: MP2Node.h
 *
 * DESCRIPTION: MP2Node class header file
 **********************************/

#ifndef MP2NODE_H_
#define MP2NODE_H_

#include "EmulNet.h"
#include "Node.h"
#include "service/DistributedHashTable.h"

#include <memory>
#include <vector>

using std::unique_ptr;
using NodeList = std::vector<Node>;

class DSService;
class EmulNet;
class Log;

class DSNode {
public:
    DSNode(shared_ptr<Member>, Params*, EmulNet*, Log*, Address*);
    DSNode()        = delete;
    DSNode(DSNode&) = delete;
    virtual ~DSNode();

    // Client side CRUD APIs
    void        clientCreate(string key, string value);
    void        clientRead  (const string &key);
    void        clientUpdate(string key, string value);
    void        clientDelete(const string &key);

    // Emulnet and Appliction API
    bool        recvLoop();
    void        checkMessages();
    NodeList    findNodes(const string &key);
    void        updateRing();
    Member*     getMemberNode();

private:
    unique_ptr<DistributedHashTableService> impl;
    Member *member;
};

using MP2Node = DSNode;

#endif /* MP2NODE_H_ */
