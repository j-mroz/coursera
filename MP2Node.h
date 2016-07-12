/**********************************
 * FILE NAME: MP2Node.h
 *
 * DESCRIPTION: MP2Node class header file
 **********************************/

#ifndef MP2NODE_H_
#define MP2NODE_H_

/**
 * Header files
 */
#include "stdincludes.h"
#include "EmulNet.h"
#include "Node.h"
#include "HashTable.h"
#include "Log.h"
#include "Params.h"
#include "Message.h"
#include "Queue.h"


#include <memory>

using std::unique_ptr;
using NodeList = vector<Node>;

class DSService;

class DSNode {
public:
    DSNode(shared_ptr<Member>, Params*, EmulNet*, Log*, Address*);
    DSNode(DSService* impl);
    DSNode()        = delete;
    DSNode(DSNode&) = delete;
    virtual ~DSNode();

    // Client side CRUD APIs
    void        clientCreate(string key, string value);
    void        clientRead  (const string &key);
    void        clientUpdate(const string &key, const string &value);
    void        clientDelete(const string &key);

    // Emulnet and Appliction API
    bool        recvLoop();
    void        checkMessages();
    NodeList    findNodes(const string &key);
    void        updateRing();
    Member*     getMemberNode();

private:
    unique_ptr<DSService> impl;
};

using MP2Node = DSNode;

#endif /* MP2NODE_H_ */
