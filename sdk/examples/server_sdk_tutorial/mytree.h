#ifndef __MYTREE_H__
#define __MYTREE_H__

#include "treeinterface.h"
#include <map>

class MyTreeNodeBase;
class MyFolder;
class MyIOManagerValues;

/** Sample implementation of the tree interface for the access to the OPC UA address space tree
  *
  */
class MyTree : public TreeInterface
{
    UA_DISABLE_COPY(MyTree);
public:
    MyTree();
    virtual ~MyTree();

    // --- TreeInterface implementation ----------------------------------
    virtual UaStatus startUp(ServerManager* pServerManager);
    virtual UaStatus shutDown();
    virtual TreeNodeInterface* findNode(const UaString& nodeIdentifier);
    virtual VariableHandle* getValueVariableHandle(const UaString& nodeIdentifier);
    virtual void lockTree();
    virtual void unlockTree();
    // --- TreeInterface implementation ----------------------------------

    UaStatus addTreeNode(MyTreeNodeBase* pNewNode);

    // --- Access to shared memory simulation ----------------------------
    inline UaMutex* pSharedMemoryMutex(){return &m_mutex;}
    UaStatus read(OpcUa_UInt32 offset, UaVariant& value);
    UaStatus write(OpcUa_UInt32 offset, const UaVariant& value);
    // --- Access to shared memory simulation ----------------------------

private:
    UaMutex                                m_mutex;
    std::map<UaString, TreeNodeInterface*> m_mapTreeNodes;
    MyFolder*                              m_pRootNode;
    MyIOManagerValues*                     m_pIOManagerValues;

    UaMutex                                m_mutexSharedMemory;
    UaInt32Array                           m_sharedMemorySimulation;
};

#endif // __MYTREE_H__