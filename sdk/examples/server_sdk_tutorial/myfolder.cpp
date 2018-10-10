#include "myfolder.h"

/** Creates a MyFolder object with default settings.*/
MyFolder::MyFolder(MyTreeNodeBase* pParentNode, const UaString& sName)
: MyTreeNodeWithChildren(pParentNode, sName)
{
}

/** Destroys the MyFolder object. */
MyFolder::~MyFolder()
{
}

OpcUa_NodeClass MyFolder::getNodeClass()
{
    return OpcUa_NodeClass_Object;
}

UaNodeId MyFolder::getTypeDefinition(OpcUa_UInt16 namespaceIndexNodeManager)
{
    OpcUa_ReferenceParameter(namespaceIndexNodeManager);
    return UaNodeId(OpcUaId_FolderType);
}
