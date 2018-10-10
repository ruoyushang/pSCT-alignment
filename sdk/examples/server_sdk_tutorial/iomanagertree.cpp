#include "iomanagertree.h"
#include "treeinterface.h"
#include "srvtrace.h"

/** IOTransactionContextTreeConfig
 */
class IOTransactionContextTreeConfig
{
    UA_DISABLE_COPY(IOTransactionContextTreeConfig);
public:
    /** construction with Variable initialization */
    IOTransactionContextTreeConfig()
    {
        m_pCallback          = NULL;
        m_hTransaction       = 0;
        m_totalItemCountHint = 0;
        m_maxAge             = 0;
        m_timestampsToReturn = OpcUa_TimestampsToReturn_Both;
        m_pSession           = NULL;
        m_transactionType    = IOManager::TransactionInvalid;
        m_pIOManagerTree     = NULL;
        m_returnDiagnostics  = 0;
    }
    /** destruction */
    virtual ~IOTransactionContextTreeConfig()
    {
        if ( m_pSession )
        {
            m_pSession->releaseReference();
        }
    }

    /** Set the Session of the current Session.
     *  @param pSession a pointer to the Session to be set.
     */
    void setSession(Session* pSession)
    {
        if ( pSession )
        {
            m_pSession = pSession;
            pSession->addReference();
        }
    }

    /** Get the actual Session via pointer.
     *  @return the actual Session.
     */
    inline Session* pSession()
    {
        return m_pSession;
    }

    IOManagerCallback*         m_pCallback;
    OpcUa_UInt32               m_hTransaction;
    OpcUa_UInt32               m_totalItemCountHint;
    OpcUa_Double               m_maxAge;
    OpcUa_TimestampsToReturn   m_timestampsToReturn;
    IOManager::TransactionType m_transactionType;
    OpcUa_UInt32               m_returnDiagnostics;
    IOManagerTree*             m_pIOManagerTree;
private:
    Session*                   m_pSession;
};


/** construction */
IOManagerTree::IOManagerTree(TreeInterface* pTree, OpcUa_UInt16 nNamespaceIndex)
: m_pTree(pTree),
  m_nNamespaceIndex(nNamespaceIndex),
  m_lastHandle(0)
{
}

/** destruction */
IOManagerTree::~IOManagerTree()
{
}

/** Start a transaction.
 *  @param pCallback Callback interface used for the transaction. The IOManager must use this interface to finish the action for each passed node in the transaction.
 *  @param serviceContext General context for the service calls containing information like the session object, return diagnostic mask and timeout hint.
 *  @param hTransaction Handle for the transaction used by the SDK to identify the transaction in the callbacks. This handle was passed in to the IOManager with the beginTransaction method.
 *  @param totalItemCountHint A hint for the IOManager about the total number of nodes in the transaction. The IOManager may not be responsible for all nodes but he can use this hint if he wants to optimize memory allocation.
 *  @param maxAge Max age parameter used only in the Read service.
 *  @param timestampsToReturn Indicates which timestamps should be returned in a Read or a Publish response.
 *  The possible enum values are:
 *      OpcUa_TimestampsToReturn_Source
 *      OpcUa_TimestampsToReturn_Server
 *      OpcUa_TimestampsToReturn_Both
 *      OpcUa_TimestampsToReturn_Neither
 *  @param transactionType Type of the transaction. The possible enum values are:
 *      READ
 *      WRITE
 *      MONITOR_BEGIN
 *      MONITOR_MODIFY
 *      MONITOR_STOP
 *  @param hIOManagerContext IOManager handle for the transaction context. This handle is used to identify the transaction in the IOManager.
 *  @return Error code
 */
UaStatus IOManagerTree::beginTransaction (
    IOManagerCallback*       pCallback,
    const ServiceContext&    serviceContext,
    OpcUa_UInt32             hTransaction,
    OpcUa_UInt32             totalItemCountHint,
    OpcUa_Double             maxAge,
    OpcUa_TimestampsToReturn timestampsToReturn,
    TransactionType          transactionType,
    OpcUa_Handle&            hIOManagerContext)
{
    TRACE0_INOUT(SERVER_CORE, UA_T"--> IOManagerTree::beginTransaction");
    UaStatus      ret;

    IOTransactionContextTreeConfig* pTransaction = new IOTransactionContextTreeConfig;
    pTransaction->setSession(serviceContext.pSession());
    pTransaction->m_pCallback          = pCallback;
    pTransaction->m_hTransaction       = hTransaction;
    pTransaction->m_totalItemCountHint = totalItemCountHint;
    pTransaction->m_maxAge             = maxAge;
    pTransaction->m_timestampsToReturn = timestampsToReturn;
    pTransaction->m_transactionType    = transactionType;
    pTransaction->m_pIOManagerTree     = this;
    pTransaction->m_returnDiagnostics  = serviceContext.returnDiagnostics();

    hIOManagerContext = (OpcUa_Handle)pTransaction;

    TRACE1_INOUT(SERVER_CORE, UA_T"<-- IOManagerTree::beginTransaction [ret=0x%lx]", ret.statusCode());
    return ret;
}

/** Finish a transaction.
 *  @param hIOManagerContext IOManager handle for the transaction context. This handle is used to identify the transaction in the IOManager.
 *  @return Error code
 */
UaStatus IOManagerTree::finishTransaction(OpcUa_Handle hIOManagerContext)
{
    TRACE0_INOUT(SERVER_CORE, UA_T"--> IOManagerTree::finishTransaction");
    UaStatus      ret;

    IOTransactionContextTreeConfig* pTransaction = (IOTransactionContextTreeConfig*)hIOManagerContext;

    if (pTransaction)
    {
        delete pTransaction;
    }

    TRACE1_INOUT(SERVER_CORE, UA_T"<-- IOManagerTree::finishTransaction [ret=0x%lx]", ret.statusCode());
    return ret;
}

/** Internal method to get the requested attribute value
 */
UaStatus IOManagerTree::getAttributeValue(Session* pSession, VariableHandle* pVariableHandle, UaDataValue& uaDataValue)
{
    OpcUa_ReferenceParameter(pSession);
    UaStatus    ret;
    UaVariant   uaValue;

    VariableHandleTreeNode* pVarHandleConfig;

    pVarHandleConfig = ((VariableHandleTreeNode*)pVariableHandle);

    uaDataValue.setServerTimestamp(UaDateTime::now());

    if ( pVarHandleConfig == NULL )
    {
        UA_ASSERT(false);
        ret = OpcUa_BadNodeIdUnknown;
    }
    else
    {
        // Lock access to tree
        m_pTree->lockTree();

        // Try to find the node
        TreeNodeInterface* pNode = m_pTree->findNode(pVarHandleConfig->m_sNodeIdentifier);

        if ( pNode )
        {
            OpcUa_NodeClass nodeClass = pNode->getNodeClass();

            switch ( pVariableHandle->m_AttributeID )
            {
            // Attributes Base Node Class
            case OpcUa_Attributes_NodeId:
            {
                uaValue.setNodeId(UaNodeId(pVarHandleConfig->m_sNodeIdentifier, m_nNamespaceIndex));
                uaDataValue.setValue(uaValue, OpcUa_True);
                break;
            }
            case OpcUa_Attributes_NodeClass:
            {
                uaValue.setInt32(nodeClass);
                uaDataValue.setValue(uaValue, OpcUa_True);
                break;
            }
            case OpcUa_Attributes_BrowseName:
            {
                uaValue.setQualifiedName(UaQualifiedName(pNode->getName(), m_nNamespaceIndex));
                uaDataValue.setValue(uaValue, OpcUa_True);
                break;
            }
            case OpcUa_Attributes_DisplayName:
            {
                uaValue.setLocalizedText(UaLocalizedText("", pNode->getName()));
                uaDataValue.setValue(uaValue, OpcUa_True);
                break;
            }
            case OpcUa_Attributes_Description:
            {
                uaValue.setLocalizedText(UaLocalizedText("", ""));
                uaDataValue.setValue(uaValue, OpcUa_True);
                break;
            }
            case OpcUa_Attributes_WriteMask:
            {
                uaValue.setUInt32(0);
                uaDataValue.setValue(uaValue, OpcUa_True);
                break;
            }
            case OpcUa_Attributes_UserWriteMask:
            {
                uaValue.setUInt32(0);
                uaDataValue.setValue(uaValue, OpcUa_True);
                break;
            }
            // Attributes Variable and Variable Type Node Class
            case OpcUa_Attributes_DataType:
            {
                if ( (nodeClass == OpcUa_NodeClass_Variable) ||
                     (nodeClass == OpcUa_NodeClass_VariableType) )
                {
                    uaValue.setNodeId(pNode->getDataType(m_nNamespaceIndex));
                    uaDataValue.setValue(uaValue, OpcUa_True);
                }
                else
                {
                    uaDataValue.setStatusCode(OpcUa_BadAttributeIdInvalid);
                }
                break;
            }
            case OpcUa_Attributes_ValueRank:
            {
                if ( (nodeClass == OpcUa_NodeClass_Variable) ||
                     (nodeClass == OpcUa_NodeClass_VariableType) )
                {
                    uaValue.setInt32(pNode->getValueRank());
                    uaDataValue.setValue(uaValue, OpcUa_True);
                }
                else
                {
                    uaDataValue.setStatusCode(OpcUa_BadAttributeIdInvalid);
                }
                break;
            }
            case OpcUa_Attributes_ArrayDimensions:
            {
                if ( (nodeClass == OpcUa_NodeClass_Variable) ||
                     (nodeClass == OpcUa_NodeClass_VariableType) )
                {
                    UaUInt32Array arrayDimensions;
                    pNode->getArrayDimensions(arrayDimensions);
                    uaValue.setUInt32Array(arrayDimensions, OpcUa_True);
                    uaDataValue.setValue(uaValue, OpcUa_True);
                }
                else
                {
                    uaDataValue.setStatusCode(OpcUa_BadAttributeIdInvalid);
                }
                break;
            }
            case OpcUa_Attributes_AccessLevel:
            {
                if ( nodeClass == OpcUa_NodeClass_Variable )
                {
                    uaValue.setByte(pNode->getAccessLevel());
                    uaDataValue.setValue(uaValue, OpcUa_True);
                }
                else
                {
                    uaDataValue.setStatusCode(OpcUa_BadAttributeIdInvalid);
                }
                break;
            }
            case OpcUa_Attributes_UserAccessLevel:
            {
                if ( nodeClass == OpcUa_NodeClass_Variable )
                {
                    uaValue.setByte(pNode->getAccessLevel());
                    uaDataValue.setValue(uaValue, OpcUa_True);
                }
                else
                {
                    uaDataValue.setStatusCode(OpcUa_BadAttributeIdInvalid);
                }
                break;
            }
            case OpcUa_Attributes_MinimumSamplingInterval:
            {
                if ( nodeClass == OpcUa_NodeClass_Variable )
                {
                    uaValue.setDouble(pNode->getMinimumSamplingInterval());
                    uaDataValue.setValue(uaValue, OpcUa_True);
                }
                else
                {
                    uaDataValue.setStatusCode(OpcUa_BadAttributeIdInvalid);
                }
                break;
            }
            case OpcUa_Attributes_Historizing:
            {
                if ( nodeClass == OpcUa_NodeClass_Variable )
                {
                    uaValue.setBool(OpcUa_False);
                    uaDataValue.setValue(uaValue, OpcUa_True);
                }
                else
                {
                    uaDataValue.setStatusCode(OpcUa_BadAttributeIdInvalid);
                }
                break;
            }
            // EventNotifier attribute of Object and View Node Classes
            case OpcUa_Attributes_EventNotifier:
            {
                if ( (nodeClass == OpcUa_NodeClass_Object) ||
                     (nodeClass == OpcUa_NodeClass_View) )
                {
                    uaValue.setByte(0);
                    uaDataValue.setValue(uaValue, OpcUa_True);
                }
                else
                {
                    uaDataValue.setStatusCode(OpcUa_BadAttributeIdInvalid);
                }
                break;
            }
            default:
            {
                uaDataValue.setStatusCode(OpcUa_BadAttributeIdInvalid);
            }
            }
        }
        else
        {
            uaDataValue.setStatusCode(OpcUa_BadNodeIdInvalid);
        }

        // Unlock tree
        m_pTree->unlockTree();
    }
    return ret;
}

/** Read attribute value of a node.
 *  @param hIOManagerContext IOManager handle for the transaction context. This handle is used to identify the transaction in the IOManager.
 *  @param callbackHandle Handle for the node in the callback. This handle was passed in to the IOManager with the beginModifyMonitoring method.
 *  @param pVariableHandle Variable handle provided by the NodeManager::getVariableHandle.
 *      This object contains the information needed by the IOManager to identify the node to read in its context.
 *  @param pReadValueId Context for the node to read. The context contains the IndexRange and the DataEncoding requested by the client. The other parameters of this context are already handled by the VariableHandle.
 *  @return Error code
 */
UaStatus IOManagerTree::beginRead (
    OpcUa_Handle       hIOManagerContext,
    OpcUa_UInt32       callbackHandle,
    VariableHandle*    pVariableHandle,
    OpcUa_ReadValueId* /*pReadValueId*/) // ToDo handle IndexRange
{
    UaStatus    ret;
    UaDataValue uaDataValue;

    IOTransactionContextTreeConfig* pTransaction = (IOTransactionContextTreeConfig*)hIOManagerContext;

    if (pTransaction)
    {
        if ( pVariableHandle->m_AttributeID == OpcUa_Attributes_Value )
        {
            // Value attribute is handled by another IOManager
            return OpcUa_BadInternalError;
        }

        ret = getAttributeValue(pTransaction->pSession(), pVariableHandle, uaDataValue);
        if ( ret.isGood() )
        {
            pTransaction->m_pCallback->finishRead(
                pTransaction->m_hTransaction,
                callbackHandle,
                uaDataValue,
                OpcUa_True,
                OpcUa_False,
                ret.pDiagnosticInfo());
        }
    }
    else
    {
        UA_ASSERT(false);
        ret = OpcUa_BadInvalidArgument;
    }

    return ret;
}

/** Write attribute value of a node.
 *  @param hIOManagerContext IOManager handle for the transaction context. This handle is used to identify the transaction in the IOManager.
 *  @param callbackHandle Handle for the node in the callback. This handle was passed in to the IOManager with the beginModifyMonitoring method.
 *  @param pVariableHandle Variable handle provided by the NodeManager::getVariableHandle.
 *      This object contains the information needed by the IOManager to identify the node to read in its context.
 *  @param pWriteValue Context for the node to write. The context contains the IndexRange requested by the client. The NodeId and Attribute parameters of this context are already handled by the VariableHandle
 *  @return Error code
 */
UaStatus IOManagerTree::beginWrite (
    OpcUa_Handle      hIOManagerContext,
    OpcUa_UInt32      callbackHandle,
    VariableHandle*   pVariableHandle,
    OpcUa_WriteValue* pWriteValue)
{
    OpcUa_ReferenceParameter(hIOManagerContext);
    OpcUa_ReferenceParameter(callbackHandle);
    OpcUa_ReferenceParameter(pVariableHandle);
    OpcUa_ReferenceParameter(pWriteValue);
    return OpcUa_BadNotWritable;
}

/** Start monitoring of an item.
 *  @param hIOManagerContext IOManager handle for the transaction context. This handle is used to identify the transaction in the IOManager.
 *  @param callbackHandle Handle for the node in the callback. This handle was passed in to the IOManager with the beginModifyMonitoring method.
 *  @param pIOVariableCallback Callback interface used for data change callbacks to the MonitoredItem managed by the SDK.
 *  @param pVariableHandle Variable handle provided by the NodeManager::getVariableHandle.
 *      This object contains the information needed by the IOManager to identify the node to read in its context.
 *  @param monitoringContext handle for the monitoring context.
 *  @return Error code
 */
UaStatus IOManagerTree::beginStartMonitoring(
    OpcUa_Handle         hIOManagerContext,
    OpcUa_UInt32         callbackHandle,
    IOVariableCallback*  pIOVariableCallback,
    VariableHandle*      pVariableHandle,
    MonitoringContext&   monitoringContext)
{
    OpcUa_ReferenceParameter(pIOVariableCallback);
    UaStatus    ret;
    UaDataValue uaDataValue;

    IOTransactionContextTreeConfig* pTransaction = (IOTransactionContextTreeConfig*)hIOManagerContext;

    if (pTransaction)
    {
        if ( pVariableHandle->m_AttributeID == OpcUa_Attributes_Value )
        {
            // Value attribute is handled by another IOManager
            return OpcUa_BadInternalError;
        }

        ret = getAttributeValue(pTransaction->pSession(), pVariableHandle, uaDataValue);
        if ( ret.isGood() )
        {
            m_lastHandle++;
            std::map<OpcUa_UInt32, OpcUa_UInt32>::iterator it = m_mapHandles.find(m_lastHandle);
            while ( it != m_mapHandles.end() )
            {
                m_lastHandle++;
                m_mapHandles.find(m_lastHandle);
            }

            pTransaction->m_pCallback->finishStartMonitoring(
                pTransaction->m_hTransaction,
                callbackHandle,
                m_lastHandle,
                monitoringContext.samplingInterval,
                OpcUa_True,
                uaDataValue,
                ret);
        }
    }
    else
    {
        UA_ASSERT(false);
        ret = OpcUa_BadInvalidArgument;
    }

    return ret;
}

/** Notify IOManager after modifying monitoring parameters of an item.
 *  @param hIOManagerContext IOManager handle for the transaction context. This handle is used to identify the transaction in the IOManager.
 *  @param callbackHandle Handle for the node in the callback. This handle was passed in to the IOManager with the beginModifyMonitoring method.
 *  @param hIOVariable The handle of the variable in the IOManager created with beginStartMonitoring.
 *      The handle was passed to the SDK in the callback finishStartMonitoring.
 *  @param monitoringContext handle for the monitoring context.
 *  @return Error code
 */
UaStatus IOManagerTree::beginModifyMonitoring(
    OpcUa_Handle       hIOManagerContext,
    OpcUa_UInt32       callbackHandle,
    OpcUa_UInt32       hIOVariable,
    MonitoringContext& monitoringContext)
{
    IOTransactionContextTreeConfig* pTransaction = (IOTransactionContextTreeConfig*)hIOManagerContext;

    if (pTransaction)
    {
        std::map<OpcUa_UInt32, OpcUa_UInt32>::iterator it = m_mapHandles.find(hIOVariable);
        if ( it == m_mapHandles.end() )
        {
            pTransaction->m_pCallback->finishModifyMonitoring(
                pTransaction->m_hTransaction,
                callbackHandle,
                monitoringContext.samplingInterval,
                OpcUa_Good);
        }
        else
        {
            return OpcUa_BadInternalError;
        }
    }
    else
    {
        UA_ASSERT(false);
        return OpcUa_BadInvalidArgument;
    }

    return OpcUa_Good;
}

/** Notify IOManager after stopping monitoring of an item.
 *  @param hIOManagerContext IOManager handle for the transaction context. This handle is used to identify the transaction in the IOManager.
 *  @param callbackHandle Handle for the node in the callback. This handle was passed in to the IOManager with the beginModifyMonitoring method.
 *  @param hIOVariable The handle of the variable in the IOManager created with beginStartMonitoring.
 *      The handle was passed to the SDK in the callback finishStartMonitoring.
 *  @return Error code
*/
UaStatus IOManagerTree::beginStopMonitoring(
    OpcUa_Handle   hIOManagerContext,
    OpcUa_UInt32   callbackHandle,
    OpcUa_UInt32   hIOVariable)
{
    IOTransactionContextTreeConfig* pTransaction = (IOTransactionContextTreeConfig*)hIOManagerContext;

    if (pTransaction)
    {
        std::map<OpcUa_UInt32, OpcUa_UInt32>::iterator it = m_mapHandles.find(hIOVariable);
        if ( it == m_mapHandles.end() )
        {
            m_mapHandles.erase(it);
            pTransaction->m_pCallback->finishStopMonitoring(
                pTransaction->m_hTransaction,
                callbackHandle,
                OpcUa_Good);
        }
        else
        {
            return OpcUa_BadInternalError;
        }
    }
    else
    {
        UA_ASSERT(false);
        return OpcUa_BadInvalidArgument;
    }

    return OpcUa_Good;
}
