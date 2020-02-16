#include "historymanagercache.h"
#include "serverconfig.h"
#include "variablehandleuanode.h"
#include "eventmanagerbase.h"
#include "nodemanagerroot.h"
#include <set>

#define HISTORYMANAGERCACHE_SAMPLING_INTERVAL 500
#define HISTORYMANAGERCACHE_QUEUE_SIZE 2000

// History manager specific continuation point class
class HistoryReadCPUserDataCache : public HistoryReadCPUserDataBase
{
public:
    HistoryReadCPUserDataCache(const UaNodeId& affectedNode, HistoryManager::TransactionType historyTransactionType)
    : HistoryReadCPUserDataBase(affectedNode, historyTransactionType)
    {}
    ~HistoryReadCPUserDataCache(){}

    // We use the next timestamp to return as continuation point
    UaDateTime m_nextTimeStamp;
};

// History manager specific continuation point class
class HistoryReadEventCPUserDataCache : public HistoryReadCPUserDataBase
{
public:
    HistoryReadEventCPUserDataCache(const UaNodeId& affectedNode, HistoryManager::TransactionType historyTransactionType)
    : HistoryReadCPUserDataBase(affectedNode, historyTransactionType)
    {}
    ~HistoryReadEventCPUserDataCache(){}

    // We use the next eventId return as continuation point
    UaByteString m_nextEventId;
};



HistoryManagerCache::HistoryManagerCache()
: m_pSession(NULL),
  m_pServerManager(NULL),
  m_loggingActive(OpcUa_False)
{
}

HistoryManagerCache::~HistoryManagerCache()
{
    std::map<UaNodeId, HistorizedVariable*>::iterator itVar;
    for ( itVar=m_mapVariables.begin(); itVar!=m_mapVariables.end(); itVar++ )
    {
        delete itVar->second;
        itVar->second = NULL;
    }

#if SUPPORT_Event_Subscription_Server_Facet
    std::map<UaNodeId, HistorizedEventNotifier*>::iterator itEvNot;
    for ( itEvNot=m_mapObjects.begin(); itEvNot!=m_mapObjects.end(); itEvNot++ )
    {
        delete itEvNot->second;
        itEvNot->second = NULL;
    }
#endif // SUPPORT_Event_Subscription_Server_Facet
}

UaStatus HistoryManagerCache::readRaw (
    const ServiceContext&       serviceContext,
    HistoryVariableHandle*      pVariableHandle,
    HistoryReadCPUserDataBase** ppContinuationPoint,
    OpcUa_TimestampsToReturn    timestampsToReturn,
    OpcUa_UInt32                maxValues,
    OpcUa_DateTime&             startTime,
    OpcUa_DateTime&             endTime,
    OpcUa_Boolean               returnBounds,
    OpcUa_HistoryReadValueId*   pReadValueId,
    UaDataValues&               dataValues)
{
    OpcUa_ReferenceParameter(serviceContext);
    OpcUa_ReferenceParameter(pReadValueId);

    // The NodeManagerBase is creating history variable handles of the type HistoryVariableHandleUaNode
    if ( pVariableHandle && (pVariableHandle->getHandleImplementation() != HistoryVariableHandle::UA_NODE) )
    {
        // Unexpected handle type
        return OpcUa_BadNodeIdUnknown;
    }

    if ( *ppContinuationPoint && (*ppContinuationPoint)->historyTransactionType() != HistoryManager::TransactionReadRaw)
    {
        delete (*ppContinuationPoint);
        *ppContinuationPoint = NULL;
        return OpcUa_BadHistoryOperationUnsupported;
    }

    UaNodeId nodeIdToRead(pReadValueId->NodeId);
    if ( *ppContinuationPoint && (*ppContinuationPoint)->affectedNode() != nodeIdToRead )
    {
        delete (*ppContinuationPoint);
        *ppContinuationPoint = NULL;
        return OpcUa_BadHistoryOperationUnsupported;
    }

    HistoryVariableHandleUaNode* pUaNodeVariableHandle = (HistoryVariableHandleUaNode*)pVariableHandle;

    // Check if the NodeId provided in the HistoryVariableHandleUaNode is a valid node to read
    std::map<UaNodeId, HistorizedVariable*>::iterator it;
    it = m_mapVariables.find(pUaNodeVariableHandle->pUaNode()->nodeId());
    if ( (it == m_mapVariables.end()) )
    {
        return OpcUa_BadNodeIdUnknown;
    }

    UaStatus            ret;
    HistorizedVariable* pVariable = it->second;
    OpcUa_UInt32        i         = 0;
    UaDateTime          dtStart(startTime);
    UaDateTime          dtEnd(endTime);
    OpcUa_Int64         iStart    = dtStart;
    OpcUa_Int64         iEnd      = dtEnd;
    OpcUa_DateTime nullDateTime;
    OpcUa_DateTime_Initialize(&nullDateTime);

    // Check if we have a continuation point
    HistoryReadCPUserDataCache* pOldContinuationPoint = (HistoryReadCPUserDataCache*)*ppContinuationPoint;
    if ( pOldContinuationPoint )
    {
        // Set time from continuation point as new start time
        dtStart = pOldContinuationPoint->m_nextTimeStamp;
        iStart    = dtStart;
        // Delete continuation point
        delete pOldContinuationPoint;
        // Set ppContinuationPoint point in/out to NULL
        *ppContinuationPoint = NULL;
    }

    if ( maxValues == 0 )
    {
        maxValues = OpcUa_Int32_Max;
    }

    // Lock access to list of values
    UaMutexLocker lock(&pVariable->m_mutex);

    if ( iStart < iEnd )
    {
        // Read in forward direction
        std::list<UaDataValue>::iterator itValues;
        dataValues.create((OpcUa_UInt32) pVariable->m_values.size());
        for ( itValues=pVariable->m_values.begin(); itValues!=pVariable->m_values.end(); itValues++ )
        {
            // Check if we reached max values
            if ( i == maxValues )
            {
                // Create a continuation point
                HistoryReadCPUserDataCache* pContinuationPoint = new HistoryReadCPUserDataCache(nodeIdToRead, HistoryManager::TransactionReadRaw);
                // Use next timestamp as starting point
                pContinuationPoint->m_nextTimeStamp= itValues->sourceTimestamp();
                // Return continuation point
                *ppContinuationPoint = pContinuationPoint;
                break;
            }

            UaDateTime  dtVal(itValues->sourceTimestamp());
            OpcUa_Int64 iVal = dtVal;

            if ( iVal < iStart )
            {
                // We have not found the start time yet
                continue;
            }

            if ( iVal > iEnd )
            {
                // We are behind the end time
                break;
            }

            if ( (i == 0) && (returnBounds != OpcUa_False) && (itValues != pVariable->m_values.begin()) )
            {
                // Bounds handling
                itValues--;
                itValues->copyTo(&dataValues[i]);
                // Delete not requested timestamps
                if ( (timestampsToReturn == OpcUa_TimestampsToReturn_Server) ||
                     (timestampsToReturn == OpcUa_TimestampsToReturn_Neither) )
                {
                    dataValues[i].SourceTimestamp = nullDateTime;
                }
                if ( (timestampsToReturn == OpcUa_TimestampsToReturn_Source) ||
                     (timestampsToReturn == OpcUa_TimestampsToReturn_Neither) )
                {
                    dataValues[i].ServerTimestamp = nullDateTime;
                }
                itValues++;
                i++;
            }

            itValues->copyTo(&dataValues[i]);
            // Delete not requested timestamps
            if ( (timestampsToReturn == OpcUa_TimestampsToReturn_Server) ||
                 (timestampsToReturn == OpcUa_TimestampsToReturn_Neither) )
            {
                dataValues[i].SourceTimestamp = nullDateTime;
            }
            if ( (timestampsToReturn == OpcUa_TimestampsToReturn_Source) ||
                 (timestampsToReturn == OpcUa_TimestampsToReturn_Neither) )
            {
                dataValues[i].ServerTimestamp = nullDateTime;
            }

            i++;
        }

        // Make size smaller if necessary
        dataValues.resize(i);
    }
    else
    {
        // Read in inverse direction
        std::list<UaDataValue>::reverse_iterator ritValues;
        dataValues.create((OpcUa_UInt32) pVariable->m_values.size());
        for ( ritValues=pVariable->m_values.rbegin(); ritValues!=pVariable->m_values.rend(); ritValues++ )
        {
            // Check if we reached max values
            if ( i == maxValues )
            {
                // Create a continuation point
                HistoryReadCPUserDataCache* pContinuationPoint = new HistoryReadCPUserDataCache(nodeIdToRead, HistoryManager::TransactionReadRaw);
                // Use next timestamp as starting point
                pContinuationPoint->m_nextTimeStamp= ritValues->sourceTimestamp();
                // Return continuation point
                *ppContinuationPoint = pContinuationPoint;
                break;
            }

            UaDateTime  dtVal(ritValues->sourceTimestamp());
            OpcUa_Int64 iVal = dtVal;

            if ( iVal > iStart )
            {
                // We have not found the start time yet
                continue;
            }

            if ( iVal < iEnd )
            {
                // We are behind the end time
                break;
            }

            if ( (i == 0) && (returnBounds != OpcUa_False) && (ritValues != pVariable->m_values.rbegin()) )
            {
                // Bounds handling
                ritValues--;
                ritValues->copyTo(&dataValues[i]);
                // Delete not requested timestamps
                if ( (timestampsToReturn == OpcUa_TimestampsToReturn_Server) ||
                     (timestampsToReturn == OpcUa_TimestampsToReturn_Neither) )
                {
                    dataValues[i].SourceTimestamp = nullDateTime;
                }
                if ( (timestampsToReturn == OpcUa_TimestampsToReturn_Source) ||
                     (timestampsToReturn == OpcUa_TimestampsToReturn_Neither) )
                {
                    dataValues[i].ServerTimestamp = nullDateTime;
                }
                ritValues++;
                i++;
            }

            ritValues->copyTo(&dataValues[i]);
            // Delete not requested timestamps
            if ( (timestampsToReturn == OpcUa_TimestampsToReturn_Server) ||
                 (timestampsToReturn == OpcUa_TimestampsToReturn_Neither) )
            {
                dataValues[i].SourceTimestamp = nullDateTime;
            }
            if ( (timestampsToReturn == OpcUa_TimestampsToReturn_Source) ||
                 (timestampsToReturn == OpcUa_TimestampsToReturn_Neither) )
            {
                dataValues[i].ServerTimestamp = nullDateTime;
            }

            i++;
        }

        // Make size smaller if necessary
        dataValues.resize(i);
    }

    return ret;
}

UaStatus HistoryManagerCache::readEvents (
    const ServiceContext&       serviceContext,
    HistoryVariableHandle*      pVariableHandle,
    HistoryReadCPUserDataBase** ppContinuationPoint,
    OpcUa_UInt32                numValuesPerNode,
    OpcUa_DateTime&             startTime,
    OpcUa_DateTime&             endTime,
    const OpcUa_EventFilter&    filter,
    OpcUa_HistoryReadValueId*   pReadValueId,
    UaHistoryEventFieldLists&   events)
{
    OpcUa_ReferenceParameter(pReadValueId);

#if SUPPORT_Event_Subscription_Server_Facet
    UaStatus                    ret;
    OpcUa_UInt32                i = 0;
    UaDateTime                  dtStart(startTime);
    UaDateTime                  dtEnd(endTime);
    OpcUa_Boolean               isInverse = OpcUa_False;
    std::list<UaVariantArray*>  tempEvents;
    std::list<UaVariantArray*>::iterator itEvents;
    OpcUa_EventFilterResult     filterResult;
    OpcUa_EventFilterResult_Initialize(&filterResult);
    OpcUa_Boolean               hasFilterError = OpcUa_False;

    // The NodeManagerBase is creating history variable handles of the type HistoryVariableHandleUaNode
    if ( pVariableHandle && (pVariableHandle->getHandleImplementation() != HistoryVariableHandle::UA_NODE) )
    {
        // Unexpected handle type
        return OpcUa_BadNodeIdUnknown;
    }

    if ( *ppContinuationPoint && (*ppContinuationPoint)->historyTransactionType() != HistoryManager::TransactionReadEvents)
    {
        delete (*ppContinuationPoint);
        *ppContinuationPoint = NULL;
        return OpcUa_BadHistoryOperationUnsupported;
    }

    UaNodeId nodeIdToRead(pReadValueId->NodeId);
    if ( *ppContinuationPoint && (*ppContinuationPoint)->affectedNode() != nodeIdToRead )
    {
        delete (*ppContinuationPoint);
        *ppContinuationPoint = NULL;
        return OpcUa_BadHistoryOperationUnsupported;
    }

    HistoryVariableHandleUaNode* pUaNodeVariableHandle = (HistoryVariableHandleUaNode*)pVariableHandle;

    // Check if the NodeId provided in the HistoryVariableHandleUaNode is a valid node to read
    std::map<UaNodeId, HistorizedEventNotifier*>::iterator itEventNotifier;
    itEventNotifier = m_mapObjects.find(pUaNodeVariableHandle->pUaNode()->nodeId());
    if ( (itEventNotifier == m_mapObjects.end()) )
    {
        return OpcUa_BadNodeIdUnknown;
    }
    HistorizedEventNotifier*    pNotifierObject = itEventNotifier->second;

    // get index of time field
    UaSimpleAttributeOperand selectClause;
    OpcUa_SimpleAttributeOperand attributeOperand;
    OpcUa_SimpleAttributeOperand_Initialize(&attributeOperand);
    OpcUa_StatusCode fieldResult;
    OpcUa_UInt32 indexTime;
    OpcUa_UInt32 indexEventId;
    selectClause.setBrowsePathElement(0, UaQualifiedName("Time", 0), 1);
    selectClause.copyTo(&attributeOperand);
    indexTime = EventManagerBase::getFieldIndex(attributeOperand, fieldResult);
    OpcUa_SimpleAttributeOperand_Clear(&attributeOperand);
    if ( OpcUa_IsNotGood(fieldResult) )
    {
        return fieldResult;
    }
    selectClause.setBrowsePathElement(0, UaQualifiedName("EventId", 0), 1);
    selectClause.copyTo(&attributeOperand);
    indexEventId = EventManagerBase::getFieldIndex(attributeOperand, fieldResult);
    OpcUa_SimpleAttributeOperand_Clear(&attributeOperand);
    if ( OpcUa_IsNotGood(fieldResult) )
    {
        return fieldResult;
    }

    // check direction
    if ( dtStart > dtEnd )
    {
        isInverse = OpcUa_True;
    }

    // Check if we have a continuation point
    HistoryReadEventCPUserDataCache* pOldContinuationPoint = (HistoryReadEventCPUserDataCache*)*ppContinuationPoint;
    UaByteString continuationEventId;
    if ( pOldContinuationPoint )
    {
        // Set eventId from continuation point
        continuationEventId = pOldContinuationPoint->m_nextEventId;
        // Delete continuation point
        delete pOldContinuationPoint;
        // Set ppContinuationPoint point in/out to NULL
        *ppContinuationPoint = NULL;
    }

    if ( numValuesPerNode == 0 )
    {
        numValuesPerNode = OpcUa_Int32_Max;
    }

    // Create Event Filter
    EventFilter eventFilter(NULL, pNotifierObject->m_pObject->nodeId(), serviceContext.pSession());
    if ( eventFilter.updateFilter(&filter, &filterResult, hasFilterError).isNotGood() || (hasFilterError != OpcUa_False) )
    {
        OpcUa_EventFilterResult_Clear(&filterResult);
        return OpcUa_BadHistoryOperationInvalid;
    }
    if ( eventFilter.buildFilterTree(&filterResult, hasFilterError).isNotGood() || (hasFilterError != OpcUa_False) )
    {
        OpcUa_EventFilterResult_Clear(&filterResult);
        return OpcUa_BadHistoryOperationInvalid;
    }
    OpcUa_EventFilterResult_Clear(&filterResult);

    // Lock access to list of events
    UaMutexLocker lock(&pNotifierObject->m_mutex);

    // read in inverse direction
    if (isInverse)
    {
        OpcUa_Variant data;
        UaVariant varData;
        UaDateTime eventTime;
        UaByteString eventId;
        OpcUa_Variant_Initialize(&data);

        std::list<HistorizedEventData*>::reverse_iterator ritEventFieldLists;
        for ( ritEventFieldLists=pNotifierObject->m_eventFieldLists.rbegin(),i=0; ritEventFieldLists!=pNotifierObject->m_eventFieldLists.rend(); ritEventFieldLists++ )
        {
            if ( *ritEventFieldLists )
            {
                // read eventId and time
                (*ritEventFieldLists)->getFieldData(indexEventId, serviceContext.pSession(), data);
                varData = UaVariant(&data, OpcUa_True);
                if ( OpcUa_IsBad(varData.toByteString(eventId)) )
                {
                    continue;
                }
                (*ritEventFieldLists)->getFieldData(indexTime, serviceContext.pSession(), data);
                varData = UaVariant(&data, OpcUa_True);
                if ( OpcUa_IsBad(varData.toDateTime(eventTime)) )
                {
                    continue;
                }

                // if we have a continuation point we skip until we find the continuation point
                if (continuationEventId.length() > 0)
                {
                    // get eventId
                    if (continuationEventId == eventId)
                    {
                        continuationEventId.clear();
                    }
                    else
                    {
                        continue;
                    }
                }

                if ( eventTime >= dtStart && eventTime <= dtEnd )
                {
                    if ( i == numValuesPerNode )
                    {
                        // Create a continuation point
                        HistoryReadEventCPUserDataCache* pContinuationPoint = new HistoryReadEventCPUserDataCache(nodeIdToRead, HistoryManager::TransactionReadEvents);
                        // Use next timestamp as starting point
                        pContinuationPoint->m_nextEventId = eventId;
                        // Return continuation point
                        *ppContinuationPoint = pContinuationPoint;
                        break;
                    }

                    UaVariantArray eventFields;
                    if ( eventFilter.getEventFields(*ritEventFieldLists, eventFields, serviceContext.pSession()) != OpcUa_False )
                    {
                        UaVariantArray* pEventFields = new UaVariantArray;
                        pEventFields->attach(eventFields.length(), eventFields.rawData());
                        eventFields.detach();
                        tempEvents.push_back(pEventFields);
                        i++;
                    }
                }
            }
        }
    }
    // read in forward direction
    else
    {
        OpcUa_Variant data;
        UaVariant varData;
        UaDateTime eventTime;
        UaByteString eventId;
        OpcUa_Variant_Initialize(&data);

        std::list<HistorizedEventData*>::iterator itEventFieldLists;
        for ( itEventFieldLists=pNotifierObject->m_eventFieldLists.begin(); itEventFieldLists!=pNotifierObject->m_eventFieldLists.end(); itEventFieldLists++ )
        {
            if ( *itEventFieldLists )
            {
                // read eventId and time
                (*itEventFieldLists)->getFieldData(indexEventId, serviceContext.pSession(), data);
                varData = UaVariant(&data, OpcUa_True);
                if ( OpcUa_IsBad(varData.toByteString(eventId)) )
                {
                    continue;
                }
                (*itEventFieldLists)->getFieldData(indexTime, serviceContext.pSession(), data);
                varData = UaVariant(&data, OpcUa_True);
                if ( OpcUa_IsBad(varData.toDateTime(eventTime)) )
                {
                    continue;
                }

                // if we have a continuation point we skip until we find the continuation point
                if (continuationEventId.length() > 0)
                {
                    // get eventId
                    if (continuationEventId == eventId)
                    {
                        continuationEventId.clear();
                    }
                    else
                    {
                        continue;
                    }
                }

                if ( eventTime >= dtStart && eventTime <= dtEnd )
                {
                    if ( i == numValuesPerNode )
                    {
                        // Create a continuation point
                        HistoryReadEventCPUserDataCache* pContinuationPoint = new HistoryReadEventCPUserDataCache(nodeIdToRead, HistoryManager::TransactionReadEvents);
                        // Use next timestamp as starting point
                        pContinuationPoint->m_nextEventId = eventId;
                        // Return continuation point
                        *ppContinuationPoint = pContinuationPoint;
                        break;
                    }

                    UaVariantArray eventFields;
                    if ( eventFilter.getEventFields(*itEventFieldLists, eventFields, serviceContext.pSession()) != OpcUa_False )
                    {
                        UaVariantArray* pEventFields = new UaVariantArray;
                        pEventFields->attach(eventFields.length(), eventFields.rawData());
                        eventFields.detach();
                        tempEvents.push_back(pEventFields);
                        i++;
                    }
                }
            }
        }
    }

    lock.unlock();

    // Detach results
    events.create((OpcUa_UInt32) tempEvents.size());
    for ( itEvents=tempEvents.begin(), i=0; itEvents!=tempEvents.end(); itEvents++, i++ )
    {
        events[i].NoOfEventFields = (*itEvents)->length();
        events[i].EventFields     = (*itEvents)->detach();
        delete (*itEvents);
        (*itEvents) = NULL;
    }

    return ret;
#else // SUPPORT_Event_Subscription_Server_Facet
    OpcUa_ReferenceParameter(serviceContext);
    OpcUa_ReferenceParameter(pVariableHandle);
    OpcUa_ReferenceParameter(ppContinuationPoint);
    OpcUa_ReferenceParameter(numValuesPerNode);
    OpcUa_ReferenceParameter(startTime);
    OpcUa_ReferenceParameter(endTime);
    OpcUa_ReferenceParameter(filter);
    OpcUa_ReferenceParameter(events);
    return OpcUa_BadNotImplemented;
#endif // SUPPORT_Event_Subscription_Server_Facet
}

UaStatus HistoryManagerCache::updateData (
        const ServiceContext&   serviceContext,
        HistoryVariableHandle*  pVariableHandle,
        OpcUa_PerformUpdateType performInsertReplace,
        OpcUa_Boolean           isStructureUpdate,
        const UaDataValues&     updateValue,
        UaStatusCodeArray&      operationResults,
        UaDiagnosticInfos&      operationDiagnosticInfos)
{
    OpcUa_ReferenceParameter(serviceContext);
    OpcUa_ReferenceParameter(operationDiagnosticInfos);
    OpcUa_ReferenceParameter(isStructureUpdate);

    // The NodeManagerBase is creating history variable handles of the type HistoryVariableHandleUaNode
    if ( pVariableHandle && (pVariableHandle->getHandleImplementation() != HistoryVariableHandle::UA_NODE) )
    {
        // Unexpected handle type
        return OpcUa_BadNodeIdUnknown;
    }
    HistoryVariableHandleUaNode* pUaNodeVariableHandle = (HistoryVariableHandleUaNode*)pVariableHandle;

    // Check if the NodeId provided in the HistoryVariableHandleUaNode is a valid node to read
    std::map<UaNodeId, HistorizedVariable*>::iterator it;
    it = m_mapVariables.find(pUaNodeVariableHandle->pUaNode()->nodeId());
    if ( (it == m_mapVariables.end()) )
    {
        return OpcUa_BadNodeIdUnknown;
    }

    UaStatus            ret;
    HistorizedVariable* pVariable = it->second;
    OpcUa_UInt32        i         = 0;
    UaDateTime          dtNew;
    UaDateTime          dtCurrent;
    UaDataValue         dataValue;

    operationResults.create(updateValue.length());

    // Lock access to list of values
    UaMutexLocker lock(&pVariable->m_mutex);

    // for each DataValue to insert we search the best place in the list
    // for bigger histories this is slow - find a better datastructure.
    for ( i=0; i < updateValue.length(); i++ )
    {
        dataValue = UaDataValue(updateValue[i]);
        OpcUa_Boolean bValueExists = OpcUa_False;
        dtNew = UaDateTime(dataValue.sourceTimestamp());

        // find first entry that's older than the new timestamp and check if a value exists already at the given timestamp
        std::list<UaDataValue>::iterator itNextValue;
        for ( itNextValue=pVariable->m_values.begin(); itNextValue!=pVariable->m_values.end(); itNextValue++ )
        {
            dtCurrent = UaDateTime(itNextValue->sourceTimestamp());

            // new value is earlier
            if (dtNew < dtCurrent)
            {
                break;
            }
            // a value exists already at this timestamp
            if (dtNew == dtCurrent)
            {
                bValueExists = OpcUa_True;
                break;
            }
        }

        switch (performInsertReplace)
        {
            // no entry may exists for the timestamp
        case OpcUa_PerformUpdateType_Insert:
            {
                if (bValueExists != OpcUa_False)
                {
                    operationResults[i] = OpcUa_BadEntryExists;
                }
                else
                {
                    // insert at beginning of the list
                    if (itNextValue == pVariable->m_values.end())
                    {
                        pVariable->m_values.push_front(dataValue);
                    }
                    // insert before current element
                    else
                    {
                        pVariable->m_values.insert(itNextValue, dataValue);
                    }
                }
            }
            break;
            // an entry must exists already for the timestamp
        case OpcUa_PerformUpdateType_Replace:
            {
                if (bValueExists == OpcUa_False)
                {
                    operationResults[i] = OpcUa_BadNoEntryExists;
                }
                else
                {
                    // delete current element from the list
                    itNextValue = pVariable->m_values.erase(itNextValue);
                    // insert new
                    pVariable->m_values.insert(itNextValue, dataValue);
                }
            }
            break;
            // overwrite existing or insert a value for a new timestamp
        case OpcUa_PerformUpdateType_Update:
            {
                // overwrite
                if (bValueExists != OpcUa_False)
                {
                    // delete current element from the list
                    itNextValue = pVariable->m_values.erase(itNextValue);
                }

                // insert at beginning of the list
                if (itNextValue == pVariable->m_values.end())
                {
                    pVariable->m_values.push_front(dataValue);
                }
                // insert before current element
                else
                {
                    pVariable->m_values.insert(itNextValue, dataValue);
                }
            }
            break;
        default:
            operationResults[i] = OpcUa_BadHistoryOperationInvalid;
        }
    }

    return ret;
}

UaStatus HistoryManagerCache::updateEvents (
    const ServiceContext&           serviceContext,
    HistoryVariableHandle*          pVariableHandle,
    OpcUa_PerformUpdateType         performInsertReplace,
    const OpcUa_EventFilter&        filter,
    const UaHistoryEventFieldLists& eventData,
    UaStatusCodeArray&              operationResults,
    UaDiagnosticInfos&              operationDiagnosticInfos)
{
    OpcUa_ReferenceParameter(serviceContext);
    OpcUa_ReferenceParameter(operationDiagnosticInfos);

#if SUPPORT_Event_Subscription_Server_Facet
    // The NodeManagerBase is creating history variable handles of the type HistoryVariableHandleUaNode
    if ( pVariableHandle && (pVariableHandle->getHandleImplementation() != HistoryVariableHandle::UA_NODE) )
    {
        // Unexpected handle type
        return OpcUa_BadNodeIdUnknown;
    }
    HistoryVariableHandleUaNode* pUaNodeVariableHandle = (HistoryVariableHandleUaNode*)pVariableHandle;

    // Check if the NodeId provided in the HistoryVariableHandleUaNode is a valid node to read
    std::map<UaNodeId, HistorizedEventNotifier*>::iterator itEventNotifier;
    itEventNotifier = m_mapObjects.find(pUaNodeVariableHandle->pUaNode()->nodeId());
    if ( (itEventNotifier == m_mapObjects.end()) )
    {
        return OpcUa_BadNodeIdUnknown;
    }

    UaStatus                        ret;
    HistorizedEventNotifier*        pNotifierObject = itEventNotifier->second;
    OpcUa_UInt32                    i = 0;
    OpcUa_Int32                     j = 0;
    OpcUa_StatusCode                fieldResult;
    OpcUa_UInt32                    fieldIndex;
    OpcUa_UInt32                    indexTime;
    OpcUa_UInt32                    indexEventType;
    OpcUa_UInt32                    indexEventId;
    OpcUa_UInt32                    indexEventIdNotifier;
    OpcUa_UInt32                    filterIndexTime = 0;
    OpcUa_UInt32                    filterIndexEventId = 0;
    OpcUa_Boolean                   containsTime = OpcUa_False;
    OpcUa_Boolean                   containsEventType = OpcUa_False;
    OpcUa_Boolean                   containsEventId = OpcUa_False;

    // get index of Time, EventType and EventId field
    UaSimpleAttributeOperand selectClause;
    OpcUa_SimpleAttributeOperand attributeOperand;
    OpcUa_SimpleAttributeOperand_Initialize(&attributeOperand);
    selectClause.setBrowsePathElement(0, UaQualifiedName("Time", 0), 1);
    selectClause.copyTo(&attributeOperand);
    indexTime = EventManagerBase::getFieldIndex(attributeOperand, fieldResult);
    OpcUa_SimpleAttributeOperand_Clear(&attributeOperand);
    if ( OpcUa_IsNotGood(fieldResult) )
    {
        return fieldResult;
    }
    selectClause.setBrowsePathElement(0, UaQualifiedName("EventType", 0), 1);
    selectClause.copyTo(&attributeOperand);
    indexEventType = EventManagerBase::getFieldIndex(attributeOperand, fieldResult);
    OpcUa_SimpleAttributeOperand_Clear(&attributeOperand);
    if ( OpcUa_IsNotGood(fieldResult) )
    {
        return fieldResult;
    }
    selectClause.setBrowsePathElement(0, UaQualifiedName("EventId", 0), 1);
    selectClause.copyTo(&attributeOperand);
    indexEventId = EventManagerBase::getFieldIndex(attributeOperand, fieldResult);
    OpcUa_SimpleAttributeOperand_Clear(&attributeOperand);
    if ( OpcUa_IsNotGood(fieldResult) )
    {
        return fieldResult;
    }

    // get index of EventId field on HistorizedEventNotifier
    std::map<OpcUa_UInt32, OpcUa_UInt32>::iterator itFieldIndex;
    itFieldIndex = pNotifierObject->m_mapFieldIndex.find(indexEventId);
    if ( itFieldIndex == pNotifierObject->m_mapFieldIndex.end() )
    {
        return OpcUa_BadInternalError;
    }
    else
    {
        indexEventIdNotifier = (*itFieldIndex).second;
    }

    // build mapping from eventField index in eventFilter to eventField index in HistorizedEventNotifier
    // we need that when filling the event fields in the history with the eventfields from eventData
    UaUInt32Array indexArray;
    indexArray.create(filter.NoOfSelectClauses);

    for ( j=0; j < filter.NoOfSelectClauses; j++ )
    {
        // get SDK index for each index in eventData
        fieldIndex = EventManagerBase::getFieldIndex(filter.SelectClauses[j], fieldResult);
        if ( OpcUa_IsNotGood(fieldResult) )
        {
            continue;
        }

        // lookup index in HistorizedEventNotifier
        itFieldIndex = pNotifierObject->m_mapFieldIndex.find(fieldIndex);
        if ( itFieldIndex == pNotifierObject->m_mapFieldIndex.end() )
        {
            continue;
        }

        if ( fieldIndex == indexTime )
        {
            filterIndexTime = j;
            containsTime = OpcUa_True;
        }
        if ( fieldIndex == indexEventType )
        {
            containsEventType = OpcUa_True;
        }
        if ( fieldIndex == indexEventId )
        {
            filterIndexEventId = j;
            containsEventId = OpcUa_True;
        }
        indexArray[j] = (*itFieldIndex).second;
    }

    // the select clause of the filter must provide EventType and Time
    if ( !(containsEventType && containsTime) )
    {
        return OpcUa_BadArgumentsMissing;
    }

    UaDateTime      dtNew;
    UaDateTime      dtCurrent;
    UaByteString    bsEventIdNew;
    UaByteString    bsEventIdCurrent;
    OpcUa_Variant   data;
    UaVariant       varData;

    operationResults.create(eventData.length());

    // Lock access to list of values
    UaMutexLocker lock(&pNotifierObject->m_mutex);

    // for each EventFielsList to insert we search the best place in the list
    // for bigger histories this is slow - find a better datastructure.
    for ( i=0; i < eventData.length(); i++ )
    {
        OpcUa_Boolean bDelete = OpcUa_False;
        OpcUa_Boolean bInsert = OpcUa_False;
        bsEventIdNew.clear();

        // number of fields has to match the filter
        if ( eventData[i].NoOfEventFields != filter.NoOfSelectClauses )
        {
            operationResults[i] = OpcUa_BadInvalidArgument;
            continue;
        }

        std::list<HistorizedEventData*>::iterator itMatch = pNotifierObject->m_eventFieldLists.end();
        std::list<HistorizedEventData*>::iterator itEventFieldLists;

        // get time of event to update
        varData = UaVariant(eventData[i].EventFields[filterIndexTime]);
        if ( OpcUa_IsBad(varData.toDateTime(dtNew)) )
        {
            operationResults[i] = OpcUa_BadInvalidArgument;
            continue;
        }

        // get eventId of event to update
        if ( containsEventId )
        {
            varData = UaVariant(eventData[i].EventFields[filterIndexEventId]);
            if ( OpcUa_IsBad(varData.toByteString(bsEventIdNew)) )
            {
                bsEventIdNew.clear();
            }
        }

        switch (performInsertReplace)
        {
            // no entry may exist for the given eventId
        case OpcUa_PerformUpdateType_Insert:
            {
                OpcUa_Boolean bFindTimeStamp = OpcUa_True;
                OpcUa_Boolean bFindEventId = OpcUa_False;

                // only search for the eventId if it's set
                if ( bsEventIdNew.length() > 0 )
                {
                    bFindEventId = OpcUa_True;
                }

                // check if an eventId exists and find first entry that's older than the new timestamp
                std::list<HistorizedEventData*>::iterator itTemp = pNotifierObject->m_eventFieldLists.end();
                for ( itEventFieldLists=pNotifierObject->m_eventFieldLists.begin(); itEventFieldLists!=pNotifierObject->m_eventFieldLists.end(); itEventFieldLists++ )
                {
                    if ( *itEventFieldLists )
                    {
                        // find time slot
                        if ( bFindTimeStamp )
                        {
                            // read time
                            (*itEventFieldLists)->getFieldData(indexTime, serviceContext.pSession(), data);
                            varData = UaVariant(&data, OpcUa_True);
                            if ( OpcUa_IsGood(varData.toDateTime(dtCurrent)) )
                            {
                                // new value is earlier
                                if (dtNew < dtCurrent)
                                {
                                    itTemp = itEventFieldLists;
                                    bFindTimeStamp = OpcUa_False;
                                }
                            }
                        }
                        // find eventId
                        if ( bFindEventId )
                        {
                            // read eventId
                            (*itEventFieldLists)->getFieldData(indexEventId, serviceContext.pSession(), data);
                            varData = UaVariant(&data, OpcUa_True);
                            if ( OpcUa_IsGood(varData.toByteString(bsEventIdCurrent)) )
                            {
                                // an event with this eventId exists already
                                if (bsEventIdCurrent == bsEventIdNew)
                                {
                                    itMatch = itEventFieldLists;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (itMatch != pNotifierObject->m_eventFieldLists.end())
                {
                    operationResults[i] = OpcUa_BadEntryExists;
                    continue;
                }
                else
                {
                    // insert before current element
                    itMatch = itTemp;
                    bInsert = OpcUa_True;
                }
            }
            break;
            // an entry must exists already for the timestamp
        case OpcUa_PerformUpdateType_Replace:
            {
                // the eventId has to be specified
                if ( bsEventIdNew.length() <= 0 )
                {
                    operationResults[i] = OpcUa_BadNoEntryExists;
                    continue;
                }

                // find the specified eventId
                for ( itEventFieldLists=pNotifierObject->m_eventFieldLists.begin(); itEventFieldLists!=pNotifierObject->m_eventFieldLists.end(); itEventFieldLists++ )
                {
                    if ( *itEventFieldLists )
                    {
                        // read eventId
                        (*itEventFieldLists)->getFieldData(indexEventId, serviceContext.pSession(), data);
                        varData = UaVariant(&data, OpcUa_True);
                        if ( OpcUa_IsGood(varData.toByteString(bsEventIdCurrent)) )
                        {
                            // an event with this eventId exists already
                            if (bsEventIdCurrent == bsEventIdNew)
                            {
                                itMatch = itEventFieldLists;
                                break;
                            }
                        }
                    }
                }

                if (itMatch == pNotifierObject->m_eventFieldLists.end())
                {
                    operationResults[i] = OpcUa_BadNoEntryExists;
                }
                else
                {
                    // delete existing and insert new element
                    bDelete = OpcUa_True;
                    bInsert = OpcUa_True;
                }
            }
            break;
            // overwrite existing entry or insert an entry for a new timestamp
        case OpcUa_PerformUpdateType_Update:
            {
                OpcUa_Boolean bFindTimeStamp = OpcUa_True;
                OpcUa_Boolean bFindEventId = OpcUa_False;

                // if the eventId is set we serch for eventId - otherwise we search for timestamp
                if ( bsEventIdNew.length() > 0 )
                {
                    bFindEventId = OpcUa_True;
                    bFindTimeStamp = OpcUa_False;
                }

                // check if an eventId exists and find first entry that's older than the new timestamp
                for ( itEventFieldLists=pNotifierObject->m_eventFieldLists.begin(); itEventFieldLists!=pNotifierObject->m_eventFieldLists.end(); itEventFieldLists++ )
                {
                    if ( *itEventFieldLists )
                    {
                        // find time slot
                        if ( bFindTimeStamp )
                        {
                            // read time
                            (*itEventFieldLists)->getFieldData(indexTime, serviceContext.pSession(), data);
                            varData = UaVariant(&data, OpcUa_True);
                            if ( OpcUa_IsGood(varData.toDateTime(dtCurrent)) )
                            {
                                // new value is earlier
                                if (dtNew < dtCurrent)
                                {
                                    itMatch = itEventFieldLists;
                                    break;
                                }
                            }
                        }
                        // find eventId
                        if ( bFindEventId )
                        {
                            // read eventId
                            (*itEventFieldLists)->getFieldData(indexEventId, serviceContext.pSession(), data);
                            varData = UaVariant(&data, OpcUa_True);
                            if ( OpcUa_IsGood(varData.toByteString(bsEventIdCurrent)) )
                            {
                                // an event with this eventId exists already
                                if (bsEventIdCurrent == bsEventIdNew)
                                {
                                    itMatch = itEventFieldLists;
                                    break;
                                }
                            }
                        }
                    }
                }

                // an eventId was specified
                if ( bsEventIdNew.length() > 0 )
                {
                     // eventId was not found
                    if ( itMatch == pNotifierObject->m_eventFieldLists.end() )
                    {
                        operationResults[i] = OpcUa_BadNoEntryExists;
                        continue;
                    }
                    else
                    {
                        // delete current element from the list
                        bDelete = OpcUa_True;
                    }
                }
                // insert new
                bInsert = OpcUa_True;
            }
            break;
        default:
            operationResults[i] = OpcUa_BadHistoryOperationInvalid;
        }

        // delete existing entry
        if ( bDelete )
        {
            delete (*itMatch);
            *itMatch = NULL;
            itMatch = pNotifierObject->m_eventFieldLists.erase(itMatch);
        }

        // add new entry
        if ( bInsert )
        {
            UaVariantArray eventFields;
            eventFields.create((OpcUa_UInt32) pNotifierObject->m_mapFieldIndex.size());

            // if no eventId was specified we have to create one here
            if ( bsEventIdNew.length () <= 0 )
            {
                // fill eventId
                EventManagerBase::buildEventId(UaByteString(), bsEventIdNew);
                bsEventIdNew.toVariant(eventFields[indexEventIdNotifier]);
            }

            // fill other event fields
            for ( j=0; j < eventData[i].NoOfEventFields; j++)
            {
                OpcUa_UInt32 mappedIndex = indexArray[j];
                if (mappedIndex != 0)
                {
                    OpcUa_Variant_CopyTo(&eventData[i].EventFields[j], &eventFields[mappedIndex]);
                }
            }
            HistorizedEventData* pData = new HistorizedEventData(eventFields, pNotifierObject);
            pNotifierObject->m_eventFieldLists.insert(itMatch, pData);
        }
    }

    return ret;
#else // SUPPORT_Event_Subscription_Server_Facet
    OpcUa_ReferenceParameter(pVariableHandle);
    OpcUa_ReferenceParameter(performInsertReplace);
    OpcUa_ReferenceParameter(filter);
    OpcUa_ReferenceParameter(eventData);
    OpcUa_ReferenceParameter(operationResults);
    return OpcUa_BadNotImplemented;
#endif // SUPPORT_Event_Subscription_Server_Facet
}

UaStatus HistoryManagerCache::deleteRawModified (
        const ServiceContext&    serviceContext,
        HistoryVariableHandle*   pVariableHandle,
        OpcUa_Boolean            isDeleteModified,
        OpcUa_DateTime&          startTime,
        OpcUa_DateTime&          endTime)
{
    OpcUa_ReferenceParameter(serviceContext);

    // we support no modified values in this sample
    if ( isDeleteModified )
    {
        return OpcUa_BadNoData;
    }

    // The NodeManagerBase is creating history variable handles of the type HistoryVariableHandleUaNode
    if ( pVariableHandle && (pVariableHandle->getHandleImplementation() != HistoryVariableHandle::UA_NODE) )
    {
        // Unexpected handle type
        return OpcUa_BadNodeIdUnknown;
    }
    HistoryVariableHandleUaNode* pUaNodeVariableHandle = (HistoryVariableHandleUaNode*)pVariableHandle;

    // Check if the NodeId provided in the HistoryVariableHandleUaNode is a valid node to read
    std::map<UaNodeId, HistorizedVariable*>::iterator it;
    it = m_mapVariables.find(pUaNodeVariableHandle->pUaNode()->nodeId());
    if ( (it == m_mapVariables.end()) )
    {
        return OpcUa_BadNodeIdUnknown;
    }

    UaStatus            ret;
    HistorizedVariable* pVariable = it->second;
    UaDateTime          dtStart(startTime);
    UaDateTime          dtEnd(endTime);
    OpcUa_Int64         iStart    = dtStart;
    OpcUa_Int64         iEnd      = dtEnd;

    // Lock access to list of values
    UaMutexLocker lock(&pVariable->m_mutex);

    // swap start and end
    if ( iStart > iEnd )
    {
        OpcUa_Int64 iVal = iStart;
        iStart = iEnd;
        iEnd = iVal;
    }

    // delete all entries between starttime and endtime
    std::list<UaDataValue>::iterator itValues;
    for ( itValues=pVariable->m_values.begin(); itValues!=pVariable->m_values.end(); )
    {
        UaDateTime  dtVal(itValues->sourceTimestamp());
        OpcUa_Int64 iVal = dtVal;

        if ( iVal < iStart )
        {
            // We have not found the start time yet
            itValues++;
            continue;
        }

        if ( iVal > iEnd )
        {
            // We are behind the end time
            break;
        }

        // delete current element from the list
        itValues = pVariable->m_values.erase(itValues);
    }

    return ret;
}

UaStatus HistoryManagerCache::deleteEvents (
    const ServiceContext&    serviceContext,
    HistoryVariableHandle*   pVariableHandle,
    const UaByteStringArray& eventIds,
    UaStatusCodeArray&       operationResults,
    UaDiagnosticInfos&       operationDiagnosticInfos)
{
    OpcUa_ReferenceParameter(operationResults);
    OpcUa_ReferenceParameter(operationDiagnosticInfos);

#if SUPPORT_Event_Subscription_Server_Facet
    UaStatus                    ret;
    OpcUa_UInt32                i = 0;

    // The NodeManagerBase is creating history variable handles of the type HistoryVariableHandleUaNode
    if ( pVariableHandle && (pVariableHandle->getHandleImplementation() != HistoryVariableHandle::UA_NODE) )
    {
        // Unexpected handle type
        return OpcUa_BadNodeIdUnknown;
    }
    HistoryVariableHandleUaNode* pUaNodeVariableHandle = (HistoryVariableHandleUaNode*)pVariableHandle;

    // Check if the NodeId provided in the HistoryVariableHandleUaNode is a valid node to read
    std::map<UaNodeId, HistorizedEventNotifier*>::iterator itEventNotifier;
    itEventNotifier = m_mapObjects.find(pUaNodeVariableHandle->pUaNode()->nodeId());
    if ( (itEventNotifier == m_mapObjects.end()) )
    {
        return OpcUa_BadNodeIdUnknown;
    }
    HistorizedEventNotifier*    pNotifierObject = itEventNotifier->second;

    // fill eventIds in a set
    std::map<UaByteString, OpcUa_UInt32> mapEventIdIndex;
    for ( i=0; i<eventIds.length(); i++)
    {
        mapEventIdIndex[eventIds[i]] = i;
    }

    // get index for EventId field
    UaSimpleAttributeOperand selectClause;
    OpcUa_SimpleAttributeOperand attributeOperand;
    OpcUa_SimpleAttributeOperand_Initialize(&attributeOperand);
    OpcUa_StatusCode fieldResult;
    OpcUa_UInt32 indexEventId;
    selectClause.setBrowsePathElement(0, UaQualifiedName("EventId", 0), 1);
    selectClause.copyTo(&attributeOperand);
    indexEventId = EventManagerBase::getFieldIndex(attributeOperand, fieldResult);
    if ( OpcUa_IsNotGood(fieldResult) )
    {
        return fieldResult;
    }

    // Lock access to list of events
    UaMutexLocker lock(&pNotifierObject->m_mutex);

    // go through list of events and find eventId
    std::list<HistorizedEventData*>::iterator itEventFieldLists;

    for ( itEventFieldLists=pNotifierObject->m_eventFieldLists.begin(); itEventFieldLists!=pNotifierObject->m_eventFieldLists.end() && mapEventIdIndex.size() > 0; )
    {
        OpcUa_Variant data;
        OpcUa_Variant_Initialize(&data);
        (*itEventFieldLists)->getFieldData(indexEventId, serviceContext.pSession(), data);
        UaVariant varData(&data, OpcUa_True);
        UaByteString bsEventId;
        if (OpcUa_IsGood(varData.toByteString(bsEventId)))
        {
            // check if the eventId is in the map
            std::map<UaByteString, OpcUa_UInt32>::iterator it = mapEventIdIndex.find(bsEventId);
            if (it != mapEventIdIndex.end())
            {
                delete (*itEventFieldLists);
                *itEventFieldLists = NULL;
                itEventFieldLists = pNotifierObject->m_eventFieldLists.erase(itEventFieldLists);
                mapEventIdIndex.erase(it);
                continue;
            }
        }
        itEventFieldLists++;
    }

    lock.unlock();

    return ret;
#else // SUPPORT_Event_Subscription_Server_Facet
    OpcUa_ReferenceParameter(serviceContext);
    OpcUa_ReferenceParameter(pVariableHandle);
    OpcUa_ReferenceParameter(eventIds);
    return OpcUa_BadNotImplemented;
#endif // SUPPORT_Event_Subscription_Server_Facet
}

void HistoryManagerCache::startUp(ServerManager* pServerManager)
{
    m_pServerManager = pServerManager;

    // Create internal session
    m_pSession = pServerManager->createInternalSession("InternalHistorizing", "en", NULL);
}

void HistoryManagerCache::shutDown()
{
    UaMutexLocker locker(&m_mutexVariables);
    OpcUa_UInt32 i;
    UaUInt32Array monitoredItemIds;
    UaStatusCodeArray results;
    std::map<UaNodeId, HistorizedVariable*>::iterator itVar;
    std::map<UaNodeId, HistorizedEventNotifier*>::iterator itEvNot;

    // Remove monitored items
    if ( m_pServerManager && m_pSession )
    {
        monitoredItemIds.create((OpcUa_UInt32) m_mapVariables.size());
        for ( itVar=m_mapVariables.begin(), i=0; itVar!=m_mapVariables.end(); itVar++, i++ )
        {
            HistorizedVariable* pVariable = itVar->second;
            monitoredItemIds[i] = pVariable->m_monitoredItemId;
        }

        m_pServerManager->deleteMonitoredItems(m_pSession, monitoredItemIds, results);
    }

    for ( itVar=m_mapVariables.begin(); itVar!=m_mapVariables.end(); itVar++ )
    {
        delete itVar->second;
        itVar->second = NULL;
    }
    m_mapVariables.clear();

#if SUPPORT_Event_Subscription_Server_Facet
    // Remove event monitored items
    if ( m_pServerManager && m_pSession )
    {
        monitoredItemIds.create((OpcUa_UInt32) m_mapObjects.size());
        for ( itEvNot=m_mapObjects.begin(), i=0; itEvNot!=m_mapObjects.end(); itEvNot++, i++ )
        {
            HistorizedEventNotifier* pNotifier = itEvNot->second;
            monitoredItemIds[i] = pNotifier->m_monitoredItemId;
        }

        m_pServerManager->deleteMonitoredItems(m_pSession, monitoredItemIds, results);
    }

    for ( itEvNot=m_mapObjects.begin(); itEvNot!=m_mapObjects.end(); itEvNot++ )
    {
        delete itEvNot->second;
        itEvNot->second = NULL;
    }
    m_mapObjects.clear();
#endif // SUPPORT_Event_Subscription_Server_Facet

    // Release Session object
    if ( m_pSession )
    {
        m_pSession->releaseReference();
        m_pSession = NULL;
    }

    m_pServerManager = NULL;
}

// Start internal monitoring and log history
void HistoryManagerCache::startLogging()
{
    UaMutexLocker locker(&m_mutexVariables);

    // Create internal monitored items for historizing
    if ( m_pServerManager && m_pSession )
    {
        // History for data monitored items
        DataMonitoredItemSpecArray dataMonitoredItems;
        OpcUa_UInt32               i = 0;
        std::map<UaNodeId, HistorizedVariable*>::iterator itVar;

        dataMonitoredItems.create((OpcUa_UInt32) m_mapVariables.size());

        // Collect information for the monitored items to create
        for ( itVar=m_mapVariables.begin(); itVar!=m_mapVariables.end(); itVar++, i++ )
        {
            HistorizedVariable* pVariable = itVar->second;
            pVariable->m_pVariable->nodeId().copyTo(&dataMonitoredItems[i].m_itemToMonitor.NodeId);
            dataMonitoredItems[i].m_itemToMonitor.AttributeId = OpcUa_Attributes_Value;
            dataMonitoredItems[i].m_requestedSamplingInterval = HISTORYMANAGERCACHE_SAMPLING_INTERVAL;
            dataMonitoredItems[i].m_pDataCallback = pVariable;
        }

        // Create the monitored items
        UaStatus status = m_pServerManager->createDataMonitoredItems(m_pSession, dataMonitoredItems);

        if ( status.isGood() )
        {
            i = 0;
            // Store the create results
            for ( itVar=m_mapVariables.begin(); itVar!=m_mapVariables.end(); itVar++, i++ )
            {
                HistorizedVariable* pVariable = itVar->second;
                if ( dataMonitoredItems[i].m_createResult.isGood() )
                {
                    pVariable->m_isHistorizing = OpcUa_True;
                    pVariable->m_monitoredItemId = dataMonitoredItems[i].m_monitoredItemId;
                    if ( dataMonitoredItems[i].m_isInitialValueProvided != OpcUa_False )
                    {
                        pVariable->dataChange(dataMonitoredItems[i].m_initialValue);
                    }
                    // set historizing attribute
                    pVariable->m_pVariable->setAttributeValue(m_pSession, OpcUa_Attributes_Historizing, UaDataValue(pVariable->m_isHistorizing, OpcUa_Good, UaDateTime::now(), UaDateTime::now()), OpcUa_False);
                }
            }
        }

#if SUPPORT_Event_Subscription_Server_Facet
        // History for event notifier
        std::map<UaNodeId, HistorizedEventNotifier*>::iterator itEvNot;

        // Collect information for the monitored items to create
        for ( itEvNot=m_mapObjects.begin(); itEvNot!=m_mapObjects.end(); itEvNot++, i++ )
        {
            HistorizedEventNotifier* pNotifier = itEvNot->second;

            // Create the monitored items
            status = m_pServerManager->createEventMonitoredItem(
                m_pSession,
                pNotifier->m_pObject->nodeId(),
                pNotifier,
                &pNotifier->m_eventFilter,
                pNotifier->m_monitoredItemId);

            if ( status.isGood() )
            {
                pNotifier->m_isHistorizing = OpcUa_True;
            }
        }
#endif // SUPPORT_Event_Subscription_Server_Facet
    }

    m_loggingActive = OpcUa_True;
}

// Stop internal monitoring and logging of history
void HistoryManagerCache::stopLogging()
{
    UaMutexLocker locker(&m_mutexVariables);

    // Remove monitored items
    if ( m_loggingActive && m_pServerManager && m_pSession )
    {
        OpcUa_UInt32 i = 0;
        UaUInt32Array monitoredItemIds;
        UaStatusCodeArray results;

        // Remove data monitored items
        std::map<UaNodeId, HistorizedVariable*>::iterator itVar;
        monitoredItemIds.create((OpcUa_UInt32) m_mapVariables.size());
        i = 0;
        for ( itVar=m_mapVariables.begin(); itVar!=m_mapVariables.end(); itVar++, i++ )
        {
            HistorizedVariable* pVariable = itVar->second;
            monitoredItemIds[i] = pVariable->m_monitoredItemId;
            pVariable->m_isHistorizing = OpcUa_False;
            // set historizing attribute
            pVariable->m_pVariable->setAttributeValue(m_pSession, OpcUa_Attributes_Historizing, UaDataValue(pVariable->m_isHistorizing, OpcUa_Good, UaDateTime::now(), UaDateTime::now()), OpcUa_False);
        }
        m_pServerManager->deleteMonitoredItems(m_pSession, monitoredItemIds, results);

#if SUPPORT_Event_Subscription_Server_Facet
        // Remove event notifier monitored items
        std::map<UaNodeId, HistorizedEventNotifier*>::iterator itEvNot;
        monitoredItemIds.clear();
        monitoredItemIds.create((OpcUa_UInt32) m_mapObjects.size());
        i = 0;
        for ( itEvNot=m_mapObjects.begin(); itEvNot!=m_mapObjects.end(); itEvNot++, i++ )
        {
            HistorizedEventNotifier * pNotifier = itEvNot->second;
            monitoredItemIds[i] = pNotifier->m_monitoredItemId;
            pNotifier->m_isHistorizing = OpcUa_False;
        }
        m_pServerManager->deleteMonitoredItems(m_pSession, monitoredItemIds, results);
#endif // SUPPORT_Event_Subscription_Server_Facet
    }

    m_loggingActive = OpcUa_False;
}

// Add a variable for historizing.
void HistoryManagerCache::addVariableToHistorize(UaNode* pNode)
{
    UaMutexLocker locker(&m_mutexVariables);
    HistorizedVariable* pVariable = NULL;

    // check input argument
    if ( (pNode != NULL) && (pNode->nodeClass() == OpcUa_NodeClass_Variable) )
    {
        std::map<UaNodeId, HistorizedVariable*>::iterator it = m_mapVariables.find(pNode->nodeId());

        if (it != m_mapVariables.end())
        {
            // HistorizedVariable exists already - nothing to do
        }
        else
        {
            pVariable = new HistorizedVariable;
            pVariable->m_pVariable = (UaVariable*)pNode;
            // Increment reference counter for our copy
            pVariable->m_pVariable->addReference();
            m_mapVariables[pNode->nodeId()] = pVariable;
        }
    }

    if (pVariable)
    {
        // logging is active - we create a monitored item here
        if (m_loggingActive)
        {
            if ( m_pServerManager && m_pSession )
            {
                DataMonitoredItemSpecArray dataMonitoredItems;
                dataMonitoredItems.create(1);

                // Collect information for the monitored items to create
                pVariable->m_pVariable->nodeId().copyTo(&dataMonitoredItems[0].m_itemToMonitor.NodeId);
                dataMonitoredItems[0].m_itemToMonitor.AttributeId = OpcUa_Attributes_Value;
                dataMonitoredItems[0].m_requestedSamplingInterval = HISTORYMANAGERCACHE_SAMPLING_INTERVAL;
                dataMonitoredItems[0].m_pDataCallback = pVariable;

                // Create the monitored items
                UaStatus status = m_pServerManager->createDataMonitoredItems(m_pSession, dataMonitoredItems);

                if ( status.isGood() )
                {
                    if ( dataMonitoredItems[0].m_createResult.isGood() )
                    {
                        pVariable->m_isHistorizing = OpcUa_True;
                        pVariable->m_monitoredItemId = dataMonitoredItems[0].m_monitoredItemId;
                        if ( dataMonitoredItems[0].m_isInitialValueProvided )
                        {
                            pVariable->dataChange(dataMonitoredItems[0].m_initialValue);
                        }
                        // set historizing attribute
                        pVariable->m_pVariable->setAttributeValue(m_pSession, OpcUa_Attributes_Historizing, UaDataValue(pVariable->m_isHistorizing, OpcUa_Good, UaDateTime::now(), UaDateTime::now()), OpcUa_False);
                    }
                }
            }
        }
        // logging is not active
        else
        {
            // we already added the HistorizedVariable to the map so there's nothing to do here
        }
    }
}

// Remove a variable from historizing.
void HistoryManagerCache::removeVariableToHistorize(UaNode* pNode)
{
    UaMutexLocker locker(&m_mutexVariables);
    HistorizedVariable* pVariable = NULL;

    // find node in map
    if ( pNode != NULL )
    {
        std::map<UaNodeId, HistorizedVariable*>::iterator it = m_mapVariables.find(pNode->nodeId());

        if (it != m_mapVariables.end())
        {
            // HistorizedVariable exists already - nothing to do
            pVariable = it->second;
            m_mapVariables.erase(it);
        }
    }

    if (pVariable)
    {
        // logging is active - we delete the monitored item here
        if (m_loggingActive)
        {
            if ( m_pServerManager && m_pSession )
            {
                UaUInt32Array monitoredItemIds;
                UaStatusCodeArray results;

                monitoredItemIds.create(1);
                monitoredItemIds[0] = pVariable->m_monitoredItemId;
                pVariable->m_isHistorizing = OpcUa_False;
                // set historizing attribute
                pVariable->m_pVariable->setAttributeValue(m_pSession, OpcUa_Attributes_Historizing, UaDataValue(pVariable->m_isHistorizing, OpcUa_Good, UaDateTime::now(), UaDateTime::now()), OpcUa_False);

                m_pServerManager->deleteMonitoredItems(m_pSession, monitoredItemIds, results);
                delete pVariable;
            }
        }
        // logging is not active
        else
        {
            delete pVariable;
        }
    }
}

// Add an EventNotifier for historizing.
UaStatus HistoryManagerCache::addEventNotifierToHistorize(UaNode* pNode, const UaEventFilter &eventFilter)
{
#if SUPPORT_Event_Subscription_Server_Facet
    UaMutexLocker locker(&m_mutexVariables);
    HistorizedEventNotifier* pNotifier = NULL;
    UaDataValue uaDataValue;
    UaVariant variantValue;
    UaStatus result;

    // check input argument node
    if (pNode == NULL || pNode->nodeClass() != OpcUa_NodeClass_Object)
    {
        return OpcUa_BadInvalidArgument;
    }

    // check if the event filter contains at least eventId and time
    const UaSimpleAttributeOperands* pSelectClause = eventFilter.pSelectClause();
    OpcUa_Boolean bFoundEventId = OpcUa_False;
    OpcUa_Boolean bFoundTime = OpcUa_False;
    UaString sTmp;

    if (pSelectClause == NULL)
    {
        return OpcUa_BadEventFilterInvalid;
    }

    for (OpcUa_UInt32 i = 0; i < pSelectClause->length(); i++)
    {
        if ((*pSelectClause)[i].NoOfBrowsePath != 1)
        {
            continue;
        }

        // check for namespace 0
        if ((*pSelectClause)[i].BrowsePath[0].NamespaceIndex != 0)
        {
            continue;
        }

        // check for time
        sTmp = UaString((*pSelectClause)[i].BrowsePath[0].Name);
        if (sTmp == "Time")
        {
            bFoundTime = OpcUa_True;
        }

        // check for eventId
        sTmp = UaString((*pSelectClause)[i].BrowsePath[0].Name);
        if (sTmp == "EventId")
        {
            bFoundEventId = OpcUa_True;
        }

        if (bFoundEventId && bFoundTime)
        {
            break;
        }
    }

    if (!(bFoundEventId & bFoundTime))
    {
        return OpcUa_BadEventFilterInvalid;
    }

    // check for EventNotifier
    pNode->getAttributeValue(m_pSession, OpcUa_Attributes_EventNotifier, uaDataValue);
    variantValue = UaVariant(*uaDataValue.value());
    OpcUa_Byte eventNotifierAttribute;
    result = variantValue.toByte(eventNotifierAttribute);

    std::map<UaNodeId, HistorizedEventNotifier*>::iterator it = m_mapObjects.find(pNode->nodeId());

    if (it != m_mapObjects.end())
    {
        // HistorizedEventNotifier exists already - nothing to do
    }
    else
    {
        pNotifier = new HistorizedEventNotifier(eventFilter);
        pNotifier->m_pObject = (UaObject*)pNode;
        // Increment reference counter for our copy
        pNotifier->m_pObject->addReference();
        m_mapObjects[pNode->nodeId()] = pNotifier;
    }

    if (pNotifier)
    {
        // logging is active - we create a monitored item here
        if (m_loggingActive)
        {
            if ( m_pServerManager && m_pSession )
            {
                // Create the monitored items
                UaStatus status = m_pServerManager->createEventMonitoredItem(
                    m_pSession,
                    pNode->nodeId(),
                    pNotifier,
                    &pNotifier->m_eventFilter,
                    pNotifier->m_monitoredItemId);

                if ( status.isGood() )
                {
                    pNotifier->m_isHistorizing = OpcUa_True;
                }
            }
        }
        // logging is not active
        else
        {
            // we already added the HistorizedEventNotifier to the map so there's nothing to do here
        }
    }

    return OpcUa_Good;
#else // SUPPORT_Event_Subscription_Server_Facet
    OpcUa_ReferenceParameter(pNode);
    OpcUa_ReferenceParameter(eventFilter);
    return OpcUa_BadNotImplemented;
#endif // SUPPORT_Event_Subscription_Server_Facet
}

// Remove an EventNotifier from historizing.
void HistoryManagerCache::removeEventNotifierFromHistorize(UaNode* pNode)
{
#if SUPPORT_Event_Subscription_Server_Facet
    UaMutexLocker locker(&m_mutexVariables);
    HistorizedEventNotifier* pNotifier = NULL;

    // find node in map
    if ( pNode != NULL )
    {
        std::map<UaNodeId, HistorizedEventNotifier*>::iterator it = m_mapObjects.find(pNode->nodeId());

        if (it != m_mapObjects.end())
        {
            // HistorizedVariable exists already - nothing to do
            pNotifier = it->second;
            m_mapObjects.erase(it);
        }
    }

    if (pNotifier)
    {
        // logging is active - we delete the monitored item here
        if (m_loggingActive)
        {
            if ( m_pServerManager && m_pSession )
            {
                UaUInt32Array monitoredItemIds;
                UaStatusCodeArray results;

                monitoredItemIds.create(1);
                monitoredItemIds[0] = pNotifier->m_monitoredItemId;
                pNotifier->m_isHistorizing = OpcUa_False;

                m_pServerManager->deleteMonitoredItems(m_pSession, monitoredItemIds, results);
                delete pNotifier;
            }
        }
        // logging is not active
        else
        {
            delete pNotifier;
        }
    }
#else // SUPPORT_Event_Subscription_Server_Facet
    OpcUa_ReferenceParameter(pNode);
#endif // SUPPORT_Event_Subscription_Server_Facet
}

HistorizedVariable::HistorizedVariable()
: m_pVariable(NULL),
  m_monitoredItemId(0),
  m_isHistorizing(OpcUa_False)
{
}

HistorizedVariable::~HistorizedVariable()
{
    // Release our reference to the monitored node
    if ( m_pVariable )
    {
        m_pVariable->releaseReference();
        m_pVariable = NULL;
    }
}

// Handle data changes from the monitored items and store the values in the
// memory buffer
void HistorizedVariable::dataChange(const UaDataValue& dataValue)
{
    UaMutexLocker locker(&m_mutex);
    m_values.push_back(dataValue);
    if ( m_values.size() > HISTORYMANAGERCACHE_QUEUE_SIZE )
    {
        m_values.erase(m_values.begin());
    }
}

UaDataValue HistorizedVariable::getLastValue()
{
    // Default implementation - method is not needed
    UaDataValue ret;
    return ret;
}

#if SUPPORT_Event_Subscription_Server_Facet
HistorizedEventNotifier::HistorizedEventNotifier(const UaEventFilter &filter)
: m_pObject(NULL),
  m_monitoredItemId(0),
  m_isHistorizing(OpcUa_False)
{
    // build map for eventField indexes
    OpcUa_Int32 i;
    OpcUa_UInt32 indexSDK;
    OpcUa_UInt32 indexHistoryNotifier;
    OpcUa_StatusCode fieldResult;

    OpcUa_EventFilter_Initialize(&m_eventFilter);
    filter.copyFilter(m_eventFilter);

    for ( i=0, indexHistoryNotifier = 0; i<m_eventFilter.NoOfSelectClauses; i++)
    {
        indexSDK = EventManagerBase::getFieldIndex(m_eventFilter.SelectClauses[i], fieldResult);

        // add result to map
        if (OpcUa_IsGood(fieldResult))
        {
            m_mapFieldIndex[indexSDK] = indexHistoryNotifier;
            indexHistoryNotifier++;
        }
    }
}

HistorizedEventNotifier::~HistorizedEventNotifier()
{
    std::list<HistorizedEventData*>::iterator it = m_eventFieldLists.begin();
    while (it != m_eventFieldLists.end())
    {
        delete (*it);
        *it = NULL;
        it++;
    }
    m_eventFieldLists.clear();
    m_mapFieldIndex.clear();
    OpcUa_EventFilter_Clear(&m_eventFilter);

    // Release our reference to the monitored node
    if ( m_pObject )
    {
        m_pObject->releaseReference();
        m_pObject = NULL;
    }
}

void HistorizedEventNotifier::newEvent(UaVariantArray& eventFields, OpcUa_Boolean detach)
{
    // save event fields
    UaMutexLocker locker(&m_mutex);
    m_eventFieldLists.push_back(new HistorizedEventData(eventFields, this, detach));
    if ( m_eventFieldLists.size() > HISTORYMANAGERCACHE_QUEUE_SIZE )
    {
        std::list<HistorizedEventData*>::iterator it =  m_eventFieldLists.begin();
        delete (*it);
        m_eventFieldLists.erase(it);
    }
}

void HistorizedEventNotifier::invalidateEventManager(OpcUa_UInt32 eventManagerIndex)
{
    // This callback method is called by an EventManager if the EventManager is removed from the system
    NodeManagerRoot* pNM = NodeManagerRoot::CreateRootNodeManager();
    // Forward the call to the monitored item management object in the ServerManager
    pNM->pServerManager()->setEventManagerInvalid(m_monitoredItemId, eventManagerIndex);
}

OpcUa_UInt32 HistorizedEventNotifier::getFieldIndex(OpcUa_UInt32 indexSDK, OpcUa_StatusCode& fieldResult)
{
    std::map<OpcUa_UInt32, OpcUa_UInt32>::iterator it = m_mapFieldIndex.find(indexSDK);
    if ( it != m_mapFieldIndex.end() )
    {
        fieldResult = OpcUa_Good;
        return it->second;
    }
    else
    {
        fieldResult = OpcUa_BadNotFound;
        return 0;
    }
}

HistorizedEventData::HistorizedEventData(UaVariantArray eventFields, const HistorizedEventNotifier* pHistorizedEventNotifier, OpcUa_Boolean detach)
    : m_pHistorizedEventNotifier(pHistorizedEventNotifier)
{
    // we can detach the data
    if (detach)
    {
        OpcUa_UInt32 length = eventFields.length();
        OpcUa_Variant* pData = eventFields.detach();
        m_eventFields.attach(length, pData);
    }
    // need to copy the data
    else
    {
        m_eventFields = UaVariantArray(eventFields);
    }
}

HistorizedEventData::~HistorizedEventData()
{
    m_eventFields.clear();
}

void HistorizedEventData::getFieldData(OpcUa_UInt32 index, Session* pSession, OpcUa_Variant& data)
{
    OpcUa_ReferenceParameter(pSession);
    // get index
    OpcUa_UInt32 indexHistoryNotifier;
    std::map<OpcUa_UInt32, OpcUa_UInt32>::const_iterator it;
    it = m_pHistorizedEventNotifier->m_mapFieldIndex.find(index);

    if (it != m_pHistorizedEventNotifier->m_mapFieldIndex.end())
    {
        indexHistoryNotifier = it->second;

        if ( indexHistoryNotifier < m_eventFields.length() )
        {
            // no need to check result since we can't return an error anyway
            OpcUa_Variant_CopyTo(&m_eventFields[indexHistoryNotifier], &data);
        }
    }
    else
    {
        OpcUa_Variant_Clear(&data);
    }
}
#endif // SUPPORT_Event_Subscription_Server_Facet
