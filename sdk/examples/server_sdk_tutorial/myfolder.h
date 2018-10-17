#ifndef __MYFOLDER_H__
#define __MYFOLDER_H__

#include "mytreenodebase.h"

/** Sample implementation of a tree node representing a OPC UA Folder
  *
  */
class MyFolder : public MyTreeNodeWithChildren
{
    UA_DISABLE_COPY(MyFolder);
    MyFolder();
public:
    MyFolder(MyTreeNodeBase* pParentNode, const UaString& sName);
    virtual ~MyFolder();

    virtual OpcUa_NodeClass getNodeClass();
    virtual UaNodeId getTypeDefinition(OpcUa_UInt16 namespaceIndexNodeManager);
};

#endif // __MYFOLDER_H__