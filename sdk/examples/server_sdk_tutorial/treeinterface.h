#ifndef __TREEINTERFACE_H__
#define __TREEINTERFACE_H__

#include "treenodeinterface.h"
#include "variablehandle.h"
#include "servermanager.h"

/** Defines the interface for the access to the OPC UA address space tree
  *
  */
class TreeInterface
{
public:
    TreeInterface(){}
    virtual ~TreeInterface(){}

    virtual UaStatus startUp(ServerManager* pServerManager) = 0;
    virtual UaStatus shutDown() = 0;

    /** Find a tree node based on the string identifier
     *  @param nodeIdentifier String indentifier for the node to find
     *  @return Pointer to TreeNodeInterface for the found node or NULL if no node was found
     */
    virtual TreeNodeInterface* findNode(const UaString& nodeIdentifier) = 0;

    /** Get a variable handle for the value attribute of a tree node
     *  @param nodeIdentifier String indentifier for the node to get a handle for
     *  @return Pointer to VariableHandle for the node or NULL if node was found
     */
    virtual VariableHandle* getValueVariableHandle(const UaString& nodeIdentifier) = 0;

    /** Lock the tree during the time a treenode pointer is used */
    virtual void lockTree() = 0;

    /** Unlock the tree */
    virtual void unlockTree() = 0;

private:
};

#endif // __TREEINTERFACE_H__