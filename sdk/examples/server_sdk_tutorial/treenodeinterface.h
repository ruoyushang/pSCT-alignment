#ifndef __TREENODEINTERFACE_H__
#define __TREENODEINTERFACE_H__

#include "uanodeid.h"
#include "uaqualifiedname.h"
#include "ualocalizedtext.h"
#include "uaarraytemplates.h"
#include "uamutex.h"

/** Defines the interface for the access to an OPC UA address space tree node
  *
  */
class TreeNodeInterface
{
    UA_DISABLE_COPY(TreeNodeInterface);
public:
    TreeNodeInterface(){}
    virtual ~TreeNodeInterface(){}

    // Parent and child handling
    virtual TreeNodeInterface* getParentNode() = 0;
    virtual OpcUa_UInt32       getChildCount() = 0;
    virtual TreeNodeInterface* getFirstChildNode() = 0;
    virtual TreeNodeInterface* getNextChildNode() = 0;

    // Attribute handling
    virtual UaString           getNodeIdentifier() = 0;
    virtual OpcUa_NodeClass    getNodeClass() = 0;
    virtual UaNodeId           getTypeDefinition(OpcUa_UInt16 namespaceIndexNodeManager) = 0;
    virtual UaString           getName() = 0;
    virtual UaNodeId           getDataType(OpcUa_UInt16 namespaceIndexNodeManager) = 0;
    virtual OpcUa_Int32        getValueRank() = 0;
    virtual void               getArrayDimensions(UaUInt32Array& arrayDimensions) = 0;
    virtual OpcUa_Byte         getAccessLevel() = 0;
    virtual OpcUa_Double       getMinimumSamplingInterval() = 0;
};

#endif // __TREENODEINTERFACE_H__