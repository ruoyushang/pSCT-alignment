#include "mydatavariable.h"

/** Creates a MyFolder object with default settings.*/
MyDataVariable::MyDataVariable(MyTreeNodeBase* pParentNode, const UaString& sName, OpcUa_BuiltInType dataType, OpcUa_Byte accessLevel)
: MyTreeVariable(pParentNode, sName),
  m_dataType(dataType),
  m_accessLevel(accessLevel)
{
}

/** Destroys the MyFolder object. */
MyDataVariable::~MyDataVariable()
{
}

/** Returns node class Variable
 */
OpcUa_NodeClass MyDataVariable::getNodeClass()
{
    return OpcUa_NodeClass_Variable;
}

/** Returns type definition BaseDataVariableType
 */
UaNodeId MyDataVariable::getTypeDefinition(OpcUa_UInt16 namespaceIndexNodeManager)
{
    OpcUa_ReferenceParameter(namespaceIndexNodeManager);
    return UaNodeId(OpcUaId_BaseDataVariableType);
}

/** Returns configured built in type as data type NodeId
 */
UaNodeId MyDataVariable::getDataType(OpcUa_UInt16 namespaceIndexNodeManager)
{
    OpcUa_ReferenceParameter(namespaceIndexNodeManager);
    return UaNodeId(m_dataType);
}

/** Returns skalar (-1) as ValueRank
 */
OpcUa_Int32 MyDataVariable::getValueRank()
{
    return -1;
}

/** Returns configured access level
 */
OpcUa_Byte MyDataVariable::getAccessLevel()
{
    return m_accessLevel;
}
