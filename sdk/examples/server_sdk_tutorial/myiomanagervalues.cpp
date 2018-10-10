#include "myiomanagervalues.h"
#include "iomanagertree.h"
#include "mytree.h"
#include "srvtrace.h"

/** IOTransactionContextMyValues
 */
class IOTransactionContextMyValues
{
    UA_DISABLE_COPY(IOTransactionContextMyValues);
public:
    /** construction with Variable initialization */
    IOTransactionContextMyValues()
    {
        m_pCallback          = NULL;
        m_hTransaction       = 0;
        m_totalItemCountHint = 0;
        m_maxAge             = 0;
        m_timestampsToReturn = OpcUa_TimestampsToReturn_Both;
        m_pSession           = NULL;
        m_transactionType    = IOManager::TransactionInvalid;
        m_pIOManagerValues   = NULL;
        m_returnDiagnostics  = 0;
    }
    /** destruction */
    virtual ~IOTransactionContextMyValues()
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
    MyIOManagerValues*         m_pIOManagerValues;
private:
    Session*                   m_pSession;
};


/** construction */
MyIOManagerValues::MyIOManagerValues(MyTree* pTree)
: m_pTree(pTree),
  m_isStopped(false)
{
    for ( OpcUa_UInt16 i=0; i<MY_IOMANAGER_VALUE_SAMPLING_GROUP_COUNT; i++ )
    {
        if ( i == 0 ) m_sampleGroups[i].setSamplingInterval(100);
        if ( i == 1 ) m_sampleGroups[i].setSamplingInterval(200);
        if ( i == 2 ) m_sampleGroups[i].setSamplingInterval(500);
        if ( i == 3 ) m_sampleGroups[i].setSamplingInterval(1000);
        if ( i == 4 ) m_sampleGroups[i].setSamplingInterval(2000);
    }

    start();
}

/** destruction */
MyIOManagerValues::~MyIOManagerValues()
{
    // Stop worker thread
    UaMutexLocker lock(&m_mutex);
    m_isStopped = true;
    lock.unlock();

    // Wait for thread termination
    wait();
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
UaStatus MyIOManagerValues::beginTransaction (
    IOManagerCallback*       pCallback,
    const ServiceContext&    serviceContext,
    OpcUa_UInt32             hTransaction,
    OpcUa_UInt32             totalItemCountHint,
    OpcUa_Double             maxAge,
    OpcUa_TimestampsToReturn timestampsToReturn,
    TransactionType          transactionType,
    OpcUa_Handle&            hIOManagerContext)
{
    TRACE0_INOUT(SERVER_CORE, UA_T"--> MyIOManagerValues::beginTransaction");
    UaStatus      ret;

    IOTransactionContextMyValues* pTransaction = new IOTransactionContextMyValues;
    pTransaction->setSession(serviceContext.pSession());
    pTransaction->m_pCallback          = pCallback;
    pTransaction->m_hTransaction       = hTransaction;
    pTransaction->m_totalItemCountHint = totalItemCountHint;
    pTransaction->m_maxAge             = maxAge;
    pTransaction->m_timestampsToReturn = timestampsToReturn;
    pTransaction->m_transactionType    = transactionType;
    pTransaction->m_pIOManagerValues   = this;
    pTransaction->m_returnDiagnostics  = serviceContext.returnDiagnostics();

    hIOManagerContext = (OpcUa_Handle)pTransaction;

    TRACE1_INOUT(SERVER_CORE, UA_T"<-- MyIOManagerValues::beginTransaction [ret=0x%lx]", ret.statusCode());
    return ret;
}

/** Finish a transaction.
 *  @param hIOManagerContext IOManager handle for the transaction context. This handle is used to identify the transaction in the IOManager.
 *  @return Error code
 */
UaStatus MyIOManagerValues::finishTransaction(OpcUa_Handle hIOManagerContext)
{
    TRACE0_INOUT(SERVER_CORE, UA_T"--> MyIOManagerValues::finishTransaction");
    UaStatus      ret;

    IOTransactionContextMyValues* pTransaction = (IOTransactionContextMyValues*)hIOManagerContext;

    if (pTransaction)
    {
        delete pTransaction;
    }

    TRACE1_INOUT(SERVER_CORE, UA_T"<-- MyIOManagerValues::finishTransaction [ret=0x%lx]", ret.statusCode());
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
UaStatus MyIOManagerValues::beginRead (
    OpcUa_Handle       hIOManagerContext,
    OpcUa_UInt32       callbackHandle,
    VariableHandle*    pVariableHandle,
    OpcUa_ReadValueId* pReadValueId)
{
    IOTransactionContextMyValues* pTransaction = (IOTransactionContextMyValues*)hIOManagerContext;

    if (pTransaction)
    {
        if ( pVariableHandle->m_AttributeID != OpcUa_Attributes_Value )
        {
            // Only Value attribute is handled by this IOManager
            return OpcUa_BadInternalError;
        }

        // Check if a special data encoding is requested
        if ( (OpcUa_String_IsNull(&pReadValueId->DataEncoding.Name) == OpcUa_False) ||
             (pReadValueId->DataEncoding.NamespaceIndex != 0) )
        {
            // We have no structure data types
            return OpcUa_BadDataEncodingUnsupported;
        }

        // Check if index range access is requested
        if ( (OpcUa_String_IsNull(&pReadValueId->IndexRange) == OpcUa_False) &&
             (OpcUa_String_IsEmpty(&pReadValueId->IndexRange) == OpcUa_False) )
        {
            // We have no array values
            return OpcUa_BadIndexRangeNoData;
        }

        UaStatus              ret;
        UaVariant             tempValue;
        MyVariableHandleOffset* pVariableHandleOffset = (MyVariableHandleOffset*)pVariableHandle;

        if ( pVariableHandleOffset->isValid() == false )
        {
            // The node is not longer valid
            return OpcUa_BadNodeIdUnknown;
        }

        if ( !(pVariableHandleOffset->accessLevel() & OpcUa_AccessLevels_CurrentRead) )
        {
            // The node is not readable
            return OpcUa_BadNotReadable;
        }

        // Get the current value
        UaMutexLocker lockSharedMemory(m_pTree->pSharedMemoryMutex());
        ret = m_pTree->read( pVariableHandleOffset->offset(), tempValue);
        lockSharedMemory.unlock();

        // Create data value for read result
        UaDateTime sourceTS;
        UaDateTime serverTS;
        if ( (pTransaction->m_timestampsToReturn == OpcUa_TimestampsToReturn_Source) ||
             (pTransaction->m_timestampsToReturn == OpcUa_TimestampsToReturn_Both) )
        {
            // Source timestamp requested
            sourceTS = UaDateTime::now();
        }
        if ( (pTransaction->m_timestampsToReturn == OpcUa_TimestampsToReturn_Server) ||
             (pTransaction->m_timestampsToReturn == OpcUa_TimestampsToReturn_Both) )
        {
            // Server timestamp requested
            serverTS = UaDateTime::now();
        }
        UaDataValue tempDataValue(tempValue, ret.statusCode(), sourceTS, serverTS);

        pTransaction->m_pCallback->finishRead(
            pTransaction->m_hTransaction,
            callbackHandle,
            tempDataValue,
            OpcUa_True,
            OpcUa_False,
            ret.pDiagnosticInfo());
    }
    else
    {
        UA_ASSERT(false);
        return OpcUa_BadInvalidArgument;
    }

    return OpcUa_Good;
}

/** Write attribute value of a node.
 *  @param hIOManagerContext IOManager handle for the transaction context. This handle is used to identify the transaction in the IOManager.
 *  @param callbackHandle Handle for the node in the callback. This handle was passed in to the IOManager with the beginModifyMonitoring method.
 *  @param pVariableHandle Variable handle provided by the NodeManager::getVariableHandle.
 *      This object contains the information needed by the IOManager to identify the node to read in its context.
 *  @param pWriteValue Context for the node to write. The context contains the IndexRange requested by the client. The NodeId and Attribute parameters of this context are already handled by the VariableHandle
 *  @return Error code
 */
UaStatus MyIOManagerValues::beginWrite (
    OpcUa_Handle      hIOManagerContext,
    OpcUa_UInt32      callbackHandle,
    VariableHandle*   pVariableHandle,
    OpcUa_WriteValue* pWriteValue)
{
    IOTransactionContextMyValues* pTransaction = (IOTransactionContextMyValues*)hIOManagerContext;

    if (pTransaction)
    {
        if ( pVariableHandle->m_AttributeID != OpcUa_Attributes_Value )
        {
            // Only Value attribute is handled by this IOManager
            return OpcUa_BadInternalError;
        }

        // Check if index range access is requested
        if ( (OpcUa_String_IsNull(&pWriteValue->IndexRange) == OpcUa_False) &&
             (OpcUa_String_IsEmpty(&pWriteValue->IndexRange) == OpcUa_False) )
        {
            // We have no array values
            return OpcUa_BadIndexRangeNoData;
        }

        // Check handling of timestamp write
        if ( pWriteValue->Value.ServerTimestamp.dwHighDateTime != 0 || pWriteValue->Value.ServerTimestamp.dwLowDateTime != 0 ||
             pWriteValue->Value.SourceTimestamp.dwHighDateTime != 0 || pWriteValue->Value.SourceTimestamp.dwLowDateTime != 0 )
        {
            // We do not support writing of timestamps
            return OpcUa_BadWriteNotSupported;
        }

        // Check handling of status write
        if ( pWriteValue->Value.StatusCode != 0 )
        {
            // We do not support writing of status
            return OpcUa_BadWriteNotSupported;
        }

        UaStatus              ret;
        UaVariant             tempValue(pWriteValue->Value.Value);
        MyVariableHandleOffset* pVariableHandleOffset = (MyVariableHandleOffset*)pVariableHandle;

        if ( pVariableHandleOffset->isValid() == false )
        {
            // The node is not longer valid
            return OpcUa_BadNodeIdUnknown;
        }

        if ( !(pVariableHandleOffset->accessLevel() & OpcUa_AccessLevels_CurrentWrite) )
        {
            // The node is not writable
            return OpcUa_BadNotWritable;
        }

        // Set the current value
        UaMutexLocker lockSharedMemory(m_pTree->pSharedMemoryMutex());
        ret = m_pTree->write( pVariableHandleOffset->offset(), tempValue);
        lockSharedMemory.unlock();

        if ( ret.isGood() )
        {
            // Lock access to sampling management objects
            UaMutexLocker lock(&m_mutex);
            pVariableHandleOffset->setWriteChangeFlag();
        }

        pTransaction->m_pCallback->finishWrite(
            pTransaction->m_hTransaction,
            callbackHandle,
            ret,
            OpcUa_True);
    }
    else
    {
        UA_ASSERT(false);
        return OpcUa_BadInvalidArgument;
    }

    return OpcUa_Good;
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
UaStatus MyIOManagerValues::beginStartMonitoring(
    OpcUa_Handle         hIOManagerContext,
    OpcUa_UInt32         callbackHandle,
    IOVariableCallback*  pIOVariableCallback,
    VariableHandle*      pVariableHandle,
    MonitoringContext&   monitoringContext)
{
    IOTransactionContextMyValues* pTransaction = (IOTransactionContextMyValues*)hIOManagerContext;

    if (pTransaction)
    {
        if ( pVariableHandle->m_AttributeID != OpcUa_Attributes_Value )
        {
            // Only Value attribute is handled by this IOManager
            return OpcUa_BadInternalError;
        }

        // Check if a special data encoding is requested
        if ( (monitoringContext.pDataEncoding != NULL) &&
             ( (OpcUa_String_IsNull(&monitoringContext.pDataEncoding->Name) == OpcUa_False) ||
              (monitoringContext.pDataEncoding->NamespaceIndex != 0) ) )
        {
            // We have no structure data types
            return OpcUa_BadDataEncodingUnsupported;
        }

        // Check if index range access is requested
        if ( (monitoringContext.pIndexRange != NULL) &&
             (OpcUa_String_IsNull(monitoringContext.pIndexRange) == OpcUa_False) &&
             (OpcUa_String_IsEmpty(monitoringContext.pIndexRange) == OpcUa_False) )
        {
            // We have no array values
            return OpcUa_BadIndexRangeNoData;
        }

        // We do not want to handle deadband
        monitoringContext.sdkMustHandleAbsoluteDeadband = OpcUa_True;

        UaStatus              ret;
        UaVariant             tempValue;
        MyVariableHandleOffset* pVariableHandleOffset = (MyVariableHandleOffset*)pVariableHandle;

        if ( pVariableHandleOffset->isValid() == false )
        {
            // The node is not longer valid
            return OpcUa_BadNodeIdUnknown;
        }

        if ( !(pVariableHandleOffset->accessLevel() & OpcUa_AccessLevels_CurrentRead) )
        {
            // The node is not readable
            return OpcUa_BadNotReadable;
        }

        // Get the current value
        UaMutexLocker lockSharedMemory(m_pTree->pSharedMemoryMutex());
        ret = m_pTree->read( pVariableHandleOffset->offset(), tempValue);
        lockSharedMemory.unlock();

        // Create data value for initial data value
        UaDateTime sourceTS;
        UaDateTime serverTS;
        if ( (pTransaction->m_timestampsToReturn == OpcUa_TimestampsToReturn_Source) ||
             (pTransaction->m_timestampsToReturn == OpcUa_TimestampsToReturn_Both) )
        {
            // Source timestamp requested
            sourceTS = UaDateTime::now();
        }
        if ( (pTransaction->m_timestampsToReturn == OpcUa_TimestampsToReturn_Server) ||
             (pTransaction->m_timestampsToReturn == OpcUa_TimestampsToReturn_Both) )
        {
            // Server timestamp requested
            serverTS = UaDateTime::now();
        }
        UaDataValue tempDataValue(tempValue, ret.statusCode(), sourceTS, serverTS);

        OpcUa_UInt32 hMonitoredItem = 0;
        OpcUa_Double revisedSamplingInterval = monitoringContext.samplingInterval;
        OpcUa_UInt16 samplingGroupIndex = 0;
        if ( ret.isGood() )
        {
            // First find right sampling group
            for ( OpcUa_UInt16 i=0; i<MY_IOMANAGER_VALUE_SAMPLING_GROUP_COUNT; i++ )
            {
                if ( (revisedSamplingInterval <= m_sampleGroups[i].samplingInterval()) ||
                     (i+1 == MY_IOMANAGER_VALUE_SAMPLING_GROUP_COUNT) )
                {
                    samplingGroupIndex = i;
                    revisedSamplingInterval = m_sampleGroups[i].samplingInterval();
                    break;
                }
            }

            // Lock access to handle manager and sampling management objects
            UaMutexLocker lock(&m_mutex);

            // Check if we have already a sampling group item for the variable
            TreeSampleGroupItem* pGroupItem = pVariableHandleOffset->getGroupItem(samplingGroupIndex);
            if ( pGroupItem == NULL )
            {
                // Create a group item and set it in the VariableHandle
                pGroupItem = new TreeSampleGroupItem(pVariableHandleOffset, tempValue);
                pVariableHandleOffset->setGroupItem(samplingGroupIndex, pGroupItem);
                pGroupItem->setGroupIndex(samplingGroupIndex);

                // Add group item to sampling group
                m_sampleGroups[samplingGroupIndex].addGroupItem(pGroupItem);
            }

            // Create the management object in this IOManager for the monitored item
            TreeSampleItem* pTreeSampleItem = new TreeSampleItem(pGroupItem, pIOVariableCallback, pTransaction->m_timestampsToReturn);

            // Add sample item to sample group item
            pGroupItem->addSampleItem(pTreeSampleItem);

            // Add sample item to handle manager
            hMonitoredItem = m_handlesSampleItems.add(pTreeSampleItem);
        }

        pTransaction->m_pCallback->finishStartMonitoring(
            pTransaction->m_hTransaction,
            callbackHandle,
            hMonitoredItem,
            revisedSamplingInterval,
            OpcUa_True,
            tempDataValue,
            ret);
    }
    else
    {
        UA_ASSERT(false);
        return OpcUa_BadInvalidArgument;
    }

    return OpcUa_Good;
}

/** Notify IOManager after modifying monitoring parameters of an item.
 *  @param hIOManagerContext IOManager handle for the transaction context. This handle is used to identify the transaction in the IOManager.
 *  @param callbackHandle Handle for the node in the callback. This handle was passed in to the IOManager with the beginModifyMonitoring method.
 *  @param hIOVariable The handle of the variable in the IOManager created with beginStartMonitoring.
 *      The handle was passed to the SDK in the callback finishStartMonitoring.
 *  @param monitoringContext handle for the monitoring context.
 *  @return Error code
 */
UaStatus MyIOManagerValues::beginModifyMonitoring(
    OpcUa_Handle       hIOManagerContext,
    OpcUa_UInt32       callbackHandle,
    OpcUa_UInt32       hIOVariable,
    MonitoringContext& monitoringContext)
{
    IOTransactionContextMyValues* pTransaction = (IOTransactionContextMyValues*)hIOManagerContext;

    if (pTransaction)
    {
        // Check if a special data encoding is requested
        if ( (monitoringContext.pDataEncoding != NULL) &&
             ( (OpcUa_String_IsNull(&monitoringContext.pDataEncoding->Name) == OpcUa_False) ||
              (monitoringContext.pDataEncoding->NamespaceIndex != 0) ) )
        {
            // We have no structure data types
            return OpcUa_BadDataEncodingUnsupported;
        }

        // Check if index range access is requested
        if ( (monitoringContext.pIndexRange != NULL) &&
             (OpcUa_String_IsNull(monitoringContext.pIndexRange) == OpcUa_False) &&
             (OpcUa_String_IsEmpty(monitoringContext.pIndexRange) == OpcUa_False) )
        {
            // We have no array values
            return OpcUa_BadIndexRangeNoData;
        }

        // We do not want to handle deadband
        monitoringContext.sdkMustHandleAbsoluteDeadband = OpcUa_True;

        // Lock access to handle manager and sampling management objects
        UaMutexLocker lock(&m_mutex);

        // Add sample item to handle manager
        TreeSampleItem* pTreeSampleItem = m_handlesSampleItems.get(hIOVariable);
        if ( pTreeSampleItem == NULL )
        {
            return OpcUa_BadInternalError;
        }

        // Disconnect from sample group item
        TreeSampleGroupItem* pGroupItem = pTreeSampleItem->pSampleGroupItem();
        pGroupItem->removeSampleItem(pTreeSampleItem);

        MyVariableHandleOffset* pVariableHandleOffset = pGroupItem->pVariableHandle();
        // Make sure the variable handle stays valid
        pVariableHandleOffset->addReference();

        if ( pGroupItem->sampleItemCount() == 0 )
        {
            // Remove group item from sampling group
            m_sampleGroups[pGroupItem->groupIndex()].removeGroupItem(pGroupItem);

            // This was the last sampling item for this variable and sampling interval
            // Disconnect from VariableHandle
            pGroupItem->pVariableHandle()->setGroupItem(pGroupItem->groupIndex(), NULL);
            // Delete sampling group item
            delete pGroupItem;
        }

        OpcUa_Double revisedSamplingInterval = monitoringContext.samplingInterval;
        OpcUa_UInt16 samplingGroupIndex = 0;
        // First find right sampling group
        for ( OpcUa_UInt16 i=0; i<MY_IOMANAGER_VALUE_SAMPLING_GROUP_COUNT; i++ )
        {
            if ( (revisedSamplingInterval <= m_sampleGroups[i].samplingInterval()) ||
                 (i+1 == MY_IOMANAGER_VALUE_SAMPLING_GROUP_COUNT) )
            {
                samplingGroupIndex = i;
                revisedSamplingInterval = m_sampleGroups[i].samplingInterval();
                break;
            }
        }

        // Check if we have already a sampling group item for the variable
        pGroupItem = pVariableHandleOffset->getGroupItem(samplingGroupIndex);
        if ( pGroupItem == NULL )
        {
            // Create a group item and set it in the VariableHandle
            UaVariant nullValue;
            pGroupItem = new TreeSampleGroupItem(pVariableHandleOffset, nullValue);
            pVariableHandleOffset->setGroupItem(samplingGroupIndex, pGroupItem);
            pGroupItem->setGroupIndex(samplingGroupIndex);

            // Add group item to sampling group
            m_sampleGroups[samplingGroupIndex].addGroupItem(pGroupItem);
        }

        // Add sample item to sample group item
        pGroupItem->addSampleItem(pTreeSampleItem);

        // Set new group item to sample item
        pTreeSampleItem->setTreeSampleGroupItem(pGroupItem);

        // Release our local reference
        pVariableHandleOffset->releaseReference();

        pTransaction->m_pCallback->finishModifyMonitoring(
            pTransaction->m_hTransaction,
            callbackHandle,
            revisedSamplingInterval,
            OpcUa_Good);
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
UaStatus MyIOManagerValues::beginStopMonitoring(
    OpcUa_Handle   hIOManagerContext,
    OpcUa_UInt32   callbackHandle,
    OpcUa_UInt32   hIOVariable)
{
    IOTransactionContextMyValues* pTransaction = (IOTransactionContextMyValues*)hIOManagerContext;

    if (pTransaction)
    {
        // Lock access to handle manager and sampling management objects
        UaMutexLocker lock(&m_mutex);

        // Add sample item to handle manager
        TreeSampleItem* pTreeSampleItem = m_handlesSampleItems.get(hIOVariable);
        if ( pTreeSampleItem == NULL )
        {
            return OpcUa_BadInternalError;
        }

        // Disconnect from sample group item
        TreeSampleGroupItem* pGroupItem = pTreeSampleItem->pSampleGroupItem();
        pGroupItem->removeSampleItem(pTreeSampleItem);

        if ( pGroupItem->sampleItemCount() == 0 )
        {
            // Remove group item from sampling group
            m_sampleGroups[pGroupItem->groupIndex()].removeGroupItem(pGroupItem);

            // This was the last sampling item for this variable and sampling interval
            // Disconnect from VariableHandle
            pGroupItem->pVariableHandle()->setGroupItem(pGroupItem->groupIndex(), NULL);
            // Delete sampling group item
            delete pGroupItem;
        }

        // Remove item from handle manager
        m_handlesSampleItems.remove(hIOVariable);

        lock.unlock();

        pTransaction->m_pCallback->finishStopMonitoring(
            pTransaction->m_hTransaction,
            callbackHandle,
            OpcUa_Good);
    }
    else
    {
        UA_ASSERT(false);
        return OpcUa_BadInvalidArgument;
    }

    return OpcUa_Good;
}

/** Main function of worker thread
 */
void MyIOManagerValues::run()
{
    UaDateTime nullTS;
    OpcUa_UInt16 baseSamplingRate = 100;
    std::list<TreeSampleGroupItem*> invalidGroupItems;

    UaMutexLocker lock(&m_mutex);

    // First find right sampling group
    for ( OpcUa_UInt16 i=0; i<MY_IOMANAGER_VALUE_SAMPLING_GROUP_COUNT; i++ )
    {
        m_sampleGroups[i].setBaseSamplingInterval(baseSamplingRate);
    }

    while ( m_isStopped == false )
    {
        for ( OpcUa_UInt16 sgIndex=0; sgIndex<MY_IOMANAGER_VALUE_SAMPLING_GROUP_COUNT; sgIndex++ )
        {
            // Check if we need to process sampling group
            if ( m_sampleGroups[sgIndex].checkSampling() && (m_sampleGroups[sgIndex].sampleGroupItemCount() > 0) )
            {
                // Lock shared memory simulation mutex
                UaMutexLocker lockSharedMemory(m_pTree->pSharedMemoryMutex());
                TreeSampleGroupItem* pSampleGroupItem = m_sampleGroups[sgIndex].getFirstSampleGroupItem();
                while ( pSampleGroupItem != NULL )
                {
                    UaStatus              ret;
                    UaVariant             currentValue;
                    if ( pSampleGroupItem->pVariableHandle()->isValid() == false )
                    {
                        // The node is not longer valid
                        ret = OpcUa_BadNodeIdUnknown;
                        invalidGroupItems.push_back(pSampleGroupItem);
                    }
                    else
                    {
                        // Get the current value
                        ret = m_pTree->read(  pSampleGroupItem->pVariableHandle()->offset(), currentValue);
                    }

                    // Send value to all monitored items
                    pSampleGroupItem->dataChange(currentValue);

                    pSampleGroupItem = m_sampleGroups[sgIndex].getNextSampleGroupItem();
                }
                lockSharedMemory.unlock();

                if ( invalidGroupItems.size() > 0 )
                {
                    // Remove invalid group items from sampling group
                    std::list<TreeSampleGroupItem*>::iterator itInvalid = invalidGroupItems.begin();
                    while ( itInvalid != invalidGroupItems.end() )
                    {
                        m_sampleGroups[sgIndex].removeGroupItem(*itInvalid);
                        itInvalid++;
                    }
                    invalidGroupItems.clear();
                }
            }
        }

        lock.unlock();
        UaThread::msleep(baseSamplingRate);
        lock.lock(&m_mutex);
    }
}


TreeSampleItem::TreeSampleItem(TreeSampleGroupItem* pSampleGroupItem,
    IOVariableCallback* pIOVariableCallback,
    OpcUa_TimestampsToReturn timestampsToReturn)
: m_pSampleGroupItem(pSampleGroupItem),
  m_pIOVariableCallback(pIOVariableCallback),
  m_timestampsToReturn(timestampsToReturn)
{
}

TreeSampleItem::~TreeSampleItem()
{
}

void TreeSampleItem::dataChange(const UaVariant& currentValue)
{
    // Create data value for data change
    UaDateTime sourceTS;
    UaDateTime serverTS;
    if ( (m_timestampsToReturn == OpcUa_TimestampsToReturn_Source) ||
         (m_timestampsToReturn == OpcUa_TimestampsToReturn_Both) )
    {
        // Source timestamp requested
        sourceTS = UaDateTime::now();
    }
    if ( (m_timestampsToReturn == OpcUa_TimestampsToReturn_Server) ||
         (m_timestampsToReturn == OpcUa_TimestampsToReturn_Both) )
    {
        // Server timestamp requested
        serverTS = UaDateTime::now();
    }
    UaDataValue tempDataValue(currentValue, OpcUa_Good, sourceTS, serverTS);

    m_pIOVariableCallback->dataChange(tempDataValue);
}

void TreeSampleItem::statusChange(const UaStatus& newStatus)
{
    UaVariant nullValue;

    // Create data value for data change
    UaDateTime sourceTS;
    UaDateTime serverTS;
    if ( (m_timestampsToReturn == OpcUa_TimestampsToReturn_Source) ||
         (m_timestampsToReturn == OpcUa_TimestampsToReturn_Both) )
    {
        // Source timestamp requested
        sourceTS = UaDateTime::now();
    }
    if ( (m_timestampsToReturn == OpcUa_TimestampsToReturn_Server) ||
         (m_timestampsToReturn == OpcUa_TimestampsToReturn_Both) )
    {
        // Server timestamp requested
        serverTS = UaDateTime::now();
    }
    UaDataValue tempDataValue(nullValue, newStatus.statusCode(), sourceTS, serverTS);

    m_pIOVariableCallback->dataChange(tempDataValue);
}

SampleGroup::SampleGroup()
: m_samplingInterval(0),
  m_maxCounter(1),
  m_currentCounter(0)
{
}

SampleGroup::~SampleGroup()
{
}

void SampleGroup::addGroupItem(TreeSampleGroupItem* pGroupItem)
{
    // Add the sample item to the list
    m_sampleGroupItems.push_back(pGroupItem);
}

void SampleGroup::removeGroupItem(TreeSampleGroupItem* pGroupItem)
{
    // Search group item in list and remove it
    std::list<TreeSampleGroupItem*>::iterator it = m_sampleGroupItems.begin();
    while ( it != m_sampleGroupItems.end() )
    {
        // We just search in the list
        // This is not time critical since it is only done when a monitored item is deleted
        if ( *it == pGroupItem )
        {
            m_sampleGroupItems.erase(it);
            return;
        }
        it++;
    }
}

TreeSampleGroupItem* SampleGroup::getFirstSampleGroupItem()
{
    m_currentSampleGroupItem = m_sampleGroupItems.begin();
    if ( m_currentSampleGroupItem != m_sampleGroupItems.end() )
    {
        return *m_currentSampleGroupItem;
    }
    else
    {
        return NULL;
    }
}

TreeSampleGroupItem* SampleGroup::getNextSampleGroupItem()
{
    if ( m_currentSampleGroupItem != m_sampleGroupItems.end() )
    {
        m_currentSampleGroupItem++;
        if ( m_currentSampleGroupItem != m_sampleGroupItems.end() )
        {
            return *m_currentSampleGroupItem;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
}

void SampleGroup::setBaseSamplingInterval(OpcUa_Double baseSamplingInterval)
{
    m_maxCounter = (OpcUa_UInt16)(m_samplingInterval / baseSamplingInterval);
    if ( m_maxCounter < 1 )
    {
        m_maxCounter = 1;
    }
}

bool SampleGroup::checkSampling()
{
    m_currentCounter++;
    if ( m_currentCounter >= m_maxCounter )
    {
        m_currentCounter = 0;
        return true;
    }
    return false;
}



TreeSampleGroupItem::TreeSampleGroupItem(MyVariableHandleOffset* pVariableHandle, UaVariant initialValue)
: m_pVariableHandle(pVariableHandle),
  m_groupIndex(0),
  m_lastValue(initialValue),
  m_lastStatus(OpcUa_Good),
  m_writeChangeFlag(false)
{
    m_pVariableHandle->addReference();
}

TreeSampleGroupItem::~TreeSampleGroupItem()
{
    m_pVariableHandle->releaseReference();
    m_pVariableHandle = NULL;
}

void TreeSampleGroupItem::addSampleItem(TreeSampleItem* pSampleItem)
{
    // Add the sample item to the list
    m_sampleItems.push_back(pSampleItem);
}

void TreeSampleGroupItem::removeSampleItem(TreeSampleItem* pSampleItem)
{
    // Search sample item in list and remove it
    std::list<TreeSampleItem*>::iterator it = m_sampleItems.begin();
    while ( it != m_sampleItems.end() )
    {
        // We just search in the list
        // This is not time critical since it is only done when a monitored item is deleted
        if ( *it == pSampleItem )
        {
            m_sampleItems.erase(it);
            return;
        }
        it++;
    }
}

void TreeSampleGroupItem::dataChange(const UaVariant& currentValue)
{
    // Check if the value was changed or the write flag is set
    // The write flag triggers a new source timestamp even if the value
    // was not changed since the last sample
    if ( (currentValue != m_lastValue) || (m_lastStatus != OpcUa_Good) || m_writeChangeFlag )
    {
        m_writeChangeFlag = false;
        m_lastValue = currentValue;
        m_lastStatus = OpcUa_Good;

        // Send data change to all monitored items
        std::list<TreeSampleItem*>::iterator it = m_sampleItems.begin();
        while ( it != m_sampleItems.end() )
        {
            (*it)->dataChange(currentValue);
            it++;
        }
    }
}

void TreeSampleGroupItem::statusChange(const UaStatus& newStatus)
{
    if ( m_lastStatus != newStatus.statusCode() )
    {
        // Send status change to all monitored items
        std::list<TreeSampleItem*>::iterator it = m_sampleItems.begin();
        while ( it != m_sampleItems.end() )
        {
            (*it)->statusChange(newStatus);
            it++;
        }
    }
}
