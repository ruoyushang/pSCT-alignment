#ifndef __MYDATAVARIABLE_H__
#define __MYDATAVARIABLE_H__

#include "mytreenodebase.h"

/** Sample implementation of a tree node representing a OPC UA Folder
  *
  */
class MyDataVariable : public MyTreeVariable
{
    UA_DISABLE_COPY(MyDataVariable);
    MyDataVariable();
public:
    MyDataVariable(MyTreeNodeBase* pParentNode, const UaString& sName, OpcUa_BuiltInType dataType, OpcUa_Byte accessLevel);
    virtual ~MyDataVariable();

    virtual OpcUa_NodeClass getNodeClass();
    virtual UaNodeId getTypeDefinition(OpcUa_UInt16 namespaceIndexNodeManager);
    virtual UaNodeId getDataType(OpcUa_UInt16 namespaceIndexNodeManager);
    virtual OpcUa_Int32 getValueRank();
    virtual OpcUa_Byte getAccessLevel();

private:
    OpcUa_BuiltInType m_dataType;
    OpcUa_Byte        m_accessLevel;
};

#endif // __MYDATAVARIABLE_H__