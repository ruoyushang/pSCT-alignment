#include "mytreenodebase.h"
#include "myiomanagervalues.h"

/** Creates a MyTree object with default settings.*/
MyTreeNodeBase::MyTreeNodeBase(MyTreeNodeBase* pParentNode, const UaString& sName)
: m_pParentNode(pParentNode),
  m_sName(sName)
{
}

/** Destroys the MyTree object. */
MyTreeNodeBase::~MyTreeNodeBase()
{
}

/** Get the parent node of this tree node
 *  @return Pointer to TreeNodeInterface for the parent node or NULL if this node is the root node
 */
MyTreeNodeBase* MyTreeNodeBase::getParentNode()
{
    return m_pParentNode;
}

UaString MyTreeNodeBase::getNodeIdentifier()
{
    UaString sIdentifier = m_sName;
    TreeNodeInterface* pParent = m_pParentNode;
    while ( pParent != NULL )
    {
        UaString sNewIdentifier = UaString("%1.%2").arg(pParent->getName()).arg(sIdentifier);
        sIdentifier = sNewIdentifier;
        pParent = pParent->getParentNode();
    }

    return sIdentifier;
}

UaString MyTreeNodeBase::getName()
{
    return m_sName;
}

UaNodeId MyTreeNodeBase::getDataType(OpcUa_UInt16 namespaceIndexNodeManager)
{
    OpcUa_ReferenceParameter(namespaceIndexNodeManager);
    return UaNodeId(OpcUaType_Null);
}

OpcUa_Int32 MyTreeNodeBase::getValueRank()
{
    return 0;
}

void MyTreeNodeBase::getArrayDimensions(UaUInt32Array& arrayDimensions)
{
    arrayDimensions.clear();
}

OpcUa_Byte MyTreeNodeBase::getAccessLevel()
{
    return 0;
}

OpcUa_Double MyTreeNodeBase::getMinimumSamplingInterval()
{
    return 0;
}

void MyTreeNodeBase::setParentNode(MyTreeNodeBase* pParentNode)
{
    m_pParentNode = pParentNode;
}

void MyTreeNodeBase::setName(const UaString& sName)
{
    m_sName = sName;
}


/** Creates a MyTree object with default settings.*/
MyTreeNodeWithChildren::MyTreeNodeWithChildren(MyTreeNodeBase* pParentNode, const UaString& sName)
: MyTreeNodeBase(pParentNode, sName),
  m_currentPosition(0)
{
}

/** Destroys the MyTree object. */
MyTreeNodeWithChildren::~MyTreeNodeWithChildren()
{
    OpcUa_UInt32 length = m_children.length();
    OpcUa_UInt32 i;
    for ( i=0; i<length; i++ )
    {
        if (m_children[i])
        {
            delete m_children[i];
            m_children[i] = NULL;
        }
    }
    m_children.clear();
}

OpcUa_UInt32 MyTreeNodeWithChildren::getChildCount()
{
    return m_children.length();
}

TreeNodeInterface* MyTreeNodeWithChildren::getFirstChildNode()
{
    // Set current position to begin of child list
    m_currentPosition = 0;

    // Check if we have an entry
    if ( m_children.length() == 0 )
    {
        return NULL;
    }

    // Return first position
    return m_children[m_currentPosition];
}

TreeNodeInterface* MyTreeNodeWithChildren::getNextChildNode()
{
    m_currentPosition++;

    // Check if we have an entry
    if ( m_children.length() <= m_currentPosition )
    {
        return NULL;
    }

    // Return next position
    return m_children[m_currentPosition];
}

void MyTreeNodeWithChildren::addChildNode(MyTreeNodeBase* pChildNode)
{
    m_children.addObject(pChildNode);
}



MyTreeVariable::MyTreeVariable(MyTreeNodeBase* pParentNode, const UaString& sName)
: MyTreeNodeBase(pParentNode, sName),
  m_offset(0),
  m_pVariableHandle(0)
{
}

MyTreeVariable::~MyTreeVariable()
{
    if ( m_pVariableHandle )
    {
        // We have a variable handle - set it invalid
        // This makes the variable invalid if handle is still in use
        m_pVariableHandle->setInvalid();
        // Release our reference
        m_pVariableHandle->releaseReference();
        m_pVariableHandle = NULL;
    }
}

MyVariableHandleOffset* MyTreeVariable::getVariableHandle()
{
    if ( m_pVariableHandle == NULL )
    {
        // We do not have a variable handle yet
        // Create one - reference counter is 1 for the reference we keep
        m_pVariableHandle = new MyVariableHandleOffset(m_offset, this->getAccessLevel());
    }

    // Increment reference counter and return the handle
    m_pVariableHandle->addReference();
    return m_pVariableHandle;
}


MyVariableHandleOffset::MyVariableHandleOffset(OpcUa_UInt32 offset, OpcUa_Byte accessLevel)
: m_offset(offset),
    m_accessLevel(accessLevel),
    m_isValid(true)
{
    for ( OpcUa_UInt16 i=0; i<MY_IOMANAGER_VALUE_SAMPLING_GROUP_COUNT; i++ )
    {
        m_sampleGroupItems[i] = NULL;
    }
}

MyVariableHandleOffset::~MyVariableHandleOffset()
{
}

void MyVariableHandleOffset::setWriteChangeFlag()
{
    // Set a write change flag to all sample group items to update
    // source timestamp even if the value is unchanged at the next sample time
    for ( OpcUa_UInt16 i=0; i<MY_IOMANAGER_VALUE_SAMPLING_GROUP_COUNT; i++ )
    {
        if ( m_sampleGroupItems[i] != NULL )
        {
            m_sampleGroupItems[i]->setWriteChangeFlag();
        }
    }
}
