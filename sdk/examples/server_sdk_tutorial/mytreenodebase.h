#ifndef __MYTREENODEBASE_H__
#define __MYTREENODEBASE_H__

#include "treenodeinterface.h"
#include "variablehandle.h"

class TreeSampleGroupItem;

#define MY_IOMANAGER_VALUE_SAMPLING_GROUP_COUNT 5

/** Base class for all tree nodes used in this example
 *
 * Sample implementation of the tree interface for the access to the OPC UA address space tree
 */
class MyTreeNodeBase : public TreeNodeInterface
{
    UA_DISABLE_COPY(MyTreeNodeBase);
    MyTreeNodeBase();
public:
    MyTreeNodeBase(MyTreeNodeBase* pParentNode, const UaString& sName);
    virtual ~MyTreeNodeBase();

    virtual bool supportsChildren(){return false;}

    // Default child handling
    virtual OpcUa_UInt32       getChildCount(){return 0;}
    virtual TreeNodeInterface* getFirstChildNode(){return NULL;}
    virtual TreeNodeInterface* getNextChildNode(){return NULL;}

    // Parent handling
    virtual MyTreeNodeBase* getParentNode();
    void setParentNode(MyTreeNodeBase* pParentNode);

    // Attribute handling
    virtual UaString           getNodeIdentifier();
    virtual UaString           getName();
    virtual UaNodeId           getDataType(OpcUa_UInt16 namespaceIndexNodeManager);
    virtual OpcUa_Int32        getValueRank();
    virtual void               getArrayDimensions(UaUInt32Array& arrayDimensions);
    virtual OpcUa_Byte         getAccessLevel();
    virtual OpcUa_Double       getMinimumSamplingInterval();
    // Setters
    void setName(const UaString& sName);

private:
    MyTreeNodeBase* m_pParentNode;
    UaString        m_sName;
};

/** Base class for all tree nodes with children used in this example
 *
 * Sample implementation of the tree interface for the access to the OPC UA address space tree
 */
class MyTreeNodeWithChildren : public MyTreeNodeBase
{
    UA_DISABLE_COPY(MyTreeNodeWithChildren);
    MyTreeNodeWithChildren();
public:
    MyTreeNodeWithChildren(MyTreeNodeBase* pParentNode, const UaString& sName);
    virtual ~MyTreeNodeWithChildren();

    virtual bool supportsChildren(){return true;}

    // Child handling
    virtual OpcUa_UInt32       getChildCount();
    virtual TreeNodeInterface* getFirstChildNode();
    virtual TreeNodeInterface* getNextChildNode();
    void addChildNode(MyTreeNodeBase* pChildNode);

private:
    UaObjectPointerArray<TreeNodeInterface> m_children;
    OpcUa_UInt32                            m_currentPosition;
};

/** VariableHandle implementation for a tree variable access an offset in the shared memory simulation
*/
class MyVariableHandleOffset : public VariableHandle
{
    UA_DISABLE_COPY(MyVariableHandleOffset);
public:
    MyVariableHandleOffset(OpcUa_UInt32 offset, OpcUa_Byte accessLevel);

    inline void setInvalid(){m_isValid = false;}
    inline OpcUa_UInt32 offset(){return m_offset;}
    inline OpcUa_Byte accessLevel(){return m_accessLevel;}
    inline bool isValid(){return m_isValid;}

    inline TreeSampleGroupItem* getGroupItem(OpcUa_UInt16 index){return m_sampleGroupItems[index];}
    inline void setGroupItem(OpcUa_UInt16 index, TreeSampleGroupItem* pItem){m_sampleGroupItems[index] = pItem;}

    void setWriteChangeFlag();

protected:
    virtual ~MyVariableHandleOffset();

private:
    OpcUa_UInt32 m_offset;
    OpcUa_Byte   m_accessLevel;
    bool         m_isValid;
    TreeSampleGroupItem* m_sampleGroupItems[MY_IOMANAGER_VALUE_SAMPLING_GROUP_COUNT];
};

/** Base class for all tree nodes representing variables
 *
 * This class stores the offset of the variable in the shared memory simulation.
 *
 * Sample implementation of the tree interface for the access to the OPC UA address space tree
 */
class MyTreeVariable : public MyTreeNodeBase
{
    UA_DISABLE_COPY(MyTreeVariable);
    MyTreeVariable();
public:
    MyTreeVariable(MyTreeNodeBase* pParentNode, const UaString& sName);
    virtual ~MyTreeVariable();

    inline OpcUa_UInt32 offset(){return m_offset;}
    inline void setOffset(OpcUa_UInt32 offset){m_offset = offset;}

    MyVariableHandleOffset* getVariableHandle();

private:
    OpcUa_UInt32            m_offset;
    MyVariableHandleOffset* m_pVariableHandle;
};
#endif // __MYTREENODEBASE_H__