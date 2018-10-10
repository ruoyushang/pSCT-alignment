#ifndef __IOMANAGERTREE_H__
#define __IOMANAGERTREE_H__

#include "iomanager.h"
#include <map>

class TreeInterface;

/** VariableHandle implementation for a tree node
*/
class VariableHandleTreeNode : public VariableHandle
{
public:
    VariableHandleTreeNode(){}

    UaString m_sNodeIdentifier;
protected:
    virtual ~VariableHandleTreeNode(){}
};

/** Implements the IOMaanger interface for the static attributes of a tree based OPC UA server address space
  *
  */
class IOManagerTree :
    public IOManager
{
    UA_DISABLE_COPY(IOManagerTree);
    IOManagerTree(){}
public:
    IOManagerTree(TreeInterface* pTree, OpcUa_UInt16 nNamespaceIndex);
    virtual ~IOManagerTree();

    //- Interface IOManager ------------------------------------------------------------------
    UaStatus beginTransaction(IOManagerCallback* pCallback, const ServiceContext& serviceContext, OpcUa_UInt32 hTransaction, OpcUa_UInt32 totalItemCountHint,
        OpcUa_Double maxAge, OpcUa_TimestampsToReturn timestampsToReturn, TransactionType transactionType, OpcUa_Handle& hIOManagerContext);
    UaStatus beginStartMonitoring(OpcUa_Handle hIOManagerContext, OpcUa_UInt32 callbackHandle, IOVariableCallback* pIOVariableCallback,
        VariableHandle* pVariableHandle, MonitoringContext& monitoringContext);
    UaStatus beginModifyMonitoring(OpcUa_Handle hIOManagerContext, OpcUa_UInt32 callbackHandle, OpcUa_UInt32 hIOVariable, MonitoringContext& monitoringContext);
    UaStatus beginStopMonitoring(OpcUa_Handle hIOManagerContext, OpcUa_UInt32 callbackHandle, OpcUa_UInt32 hIOVariable);
    UaStatus beginRead(OpcUa_Handle hIOManagerContext, OpcUa_UInt32 callbackHandle, VariableHandle* pVariableHandle, OpcUa_ReadValueId* pReadValueId);
    UaStatus beginWrite(OpcUa_Handle hIOManagerContext, OpcUa_UInt32 callbackHandle, VariableHandle* pVariableHandle, OpcUa_WriteValue* pWriteValue);
    UaStatus finishTransaction(OpcUa_Handle hIOManagerContext);
    //- Interface NodeManager ------------------------------------------------------------------

private:
    UaStatus getAttributeValue(Session* pSession, VariableHandle* pVariableHandle, UaDataValue& uaDataValue);

private:
    TreeInterface*                       m_pTree;
    OpcUa_UInt16                         m_nNamespaceIndex;
    std::map<OpcUa_UInt32, OpcUa_UInt32> m_mapHandles;
    OpcUa_UInt32                         m_lastHandle;
};

#endif // __IOMANAGERTREE_H__