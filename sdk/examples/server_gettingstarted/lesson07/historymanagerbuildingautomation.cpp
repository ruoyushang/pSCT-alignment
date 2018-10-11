#include "historymanagerbuildingautomation.h"
#include "serverconfig.h"
#include "variablehandleuanode.h"

#define HISTORYMANAGERBA_SAMPLING_INTERVAL 500
#define HISTORYMANAGERBA_QUEUE_SIZE 2000

// History manager specific continuation point class
class HistoryReadCPUserDataBA : public HistoryReadCPUserDataBase
{
public:
    HistoryReadCPUserDataBA(const UaNodeId& affectedNode, HistoryManager::TransactionType historyTransactionType)
        : HistoryReadCPUserDataBase(affectedNode, historyTransactionType)
    {}
    ~HistoryReadCPUserDataBA(){}

    // We use the next timestamp to return as continuation point
    UaDateTime m_newStartTime;
};


HistoryManagerBuildingAutomation::HistoryManagerBuildingAutomation()
: m_pSession(NULL),
  m_pServerManager(NULL)
{
}

HistoryManagerBuildingAutomation::~HistoryManagerBuildingAutomation()
{
    std::map<UaNodeId, HistorizedVariable*>::iterator it;
    for ( it=m_mapVariables.begin(); it!=m_mapVariables.end(); it++ )
    {
        delete it->second;
        it->second = NULL;
    }
}

UaStatus HistoryManagerBuildingAutomation::readRaw (
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
    HistoryVariableHandleUaNode* pUaNodeVariableHandle = (HistoryVariableHandleUaNode*)pVariableHandle;

    // Check if the NodeId provided in the HistoryVariableHandleUaNode is a valid node to read
    std::map<UaNodeId, HistorizedVariable*>::iterator it;
    it = m_mapVariables.find(pUaNodeVariableHandle->pUaNode()->nodeId());
    if ( (it == m_mapVariables.end()) || (it->second->m_isValid == OpcUa_False) )
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
    HistoryReadCPUserDataBA* pOldContinuationPoint = (HistoryReadCPUserDataBA*)*ppContinuationPoint;
    if ( pOldContinuationPoint )
    {
        // Set time from continuation point as new start time
        dtStart = pOldContinuationPoint->m_newStartTime;
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

    UaNodeId nodeIdToRead(pReadValueId->NodeId);

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
                HistoryReadCPUserDataBA* pContinuationPoint = new HistoryReadCPUserDataBA(nodeIdToRead, HistoryManager::TransactionReadRaw);
                // Use next timestamp as starting point
                pContinuationPoint->m_newStartTime = itValues->serverTimestamp();
                // Return continuation point
                *ppContinuationPoint = pContinuationPoint;
                break;
            }

            UaDateTime  dtVal(itValues->serverTimestamp());
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
                HistoryReadCPUserDataBA* pContinuationPoint = new HistoryReadCPUserDataBA(nodeIdToRead, HistoryManager::TransactionReadRaw);
                // Use next timestamp as starting point
                pContinuationPoint->m_newStartTime = ritValues->serverTimestamp();
                // Return continuation point
                *ppContinuationPoint = pContinuationPoint;
                break;
            }

            UaDateTime  dtVal(ritValues->serverTimestamp());
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

// Add a variable for historizing - must be called before startUp
void HistoryManagerBuildingAutomation::addVariableToHistorize(UaNode* pNode)
{
    if ( (pNode != NULL) && (pNode->nodeClass() == OpcUa_NodeClass_Variable) )
    {
        HistorizedVariable* pVariable = new HistorizedVariable;
        pVariable->m_pVariable = (UaVariable*)pNode;
        m_mapVariables[pNode->nodeId()] = pVariable;
    }
}

// Start up history manager for internal monitoring
void HistoryManagerBuildingAutomation::startUp(ServerManager* pServerManager)
{
    UaStatus status;
    m_pServerManager = pServerManager;

    // Create internal session
    m_pSession = pServerManager->getServerConfig()->createSession(0, UaNodeId());
    if ( m_pSession )
    {
        status = m_pSession->open("InternalHistorizing", UaByteString(), OpcUa_UInt32_Max).statusCode();
        if ( status.isGood() )
        {
            UaStringArray localeIDs;
            localeIDs.create(1);
            UaString sTemp("en");
            sTemp.copyTo(&localeIDs[0]);
            status = m_pSession->activate(0, NULL, localeIDs).statusCode();
        }
    }

    // Create internal monitored items for historizing
    if ( status.isGood() )
    {
        DataMonitoredItemSpecArray dataMonitoredItems;
        OpcUa_UInt32               i = 0;
        std::map<UaNodeId, HistorizedVariable*>::iterator it;

        dataMonitoredItems.create((OpcUa_UInt32) m_mapVariables.size());

        // Collect information for the monitored items to create
        for(it=m_mapVariables.begin(); it!=m_mapVariables.end(); it++)
        {
            HistorizedVariable* pVariable = it->second;
            pVariable->m_pVariable->nodeId().copyTo(&dataMonitoredItems[i].m_itemToMonitor.NodeId);
            dataMonitoredItems[i].m_itemToMonitor.AttributeId = OpcUa_Attributes_Value;
            dataMonitoredItems[i].m_requestedSamplingInterval = HISTORYMANAGERBA_SAMPLING_INTERVAL;
            dataMonitoredItems[i].m_pDataCallback = pVariable;
            i++;
        }

        // Create the monitored items
        status = pServerManager->createDataMonitoredItems(m_pSession, dataMonitoredItems);
    
        if ( status.isGood() )
        {
            i = 0;
            // Store the create results
            for(it=m_mapVariables.begin(); it!=m_mapVariables.end(); it++)
            {
                HistorizedVariable* pVariable = it->second;
                if ( dataMonitoredItems[i].m_createResult.isGood() )
                {
                    pVariable->m_isValid = OpcUa_True;
                    pVariable->m_monitoredItemId = dataMonitoredItems[i].m_monitoredItemId;
                    if ( dataMonitoredItems[i].m_isInitialValueProvided != OpcUa_False )
                    {
                        pVariable->dataChange(dataMonitoredItems[i].m_initialValue);
                    }
                }
                i++;
            }
        }
    }
}

void HistoryManagerBuildingAutomation::shutDown()
{
    std::map<UaNodeId, HistorizedVariable*>::iterator it;

    // Remove monitored items
    if ( m_pServerManager && m_pSession )
    {
        OpcUa_UInt32 i = 0;
        UaUInt32Array monitoredItemIds;
        UaStatusCodeArray results;

        monitoredItemIds.create((OpcUa_UInt32) m_mapVariables.size());
        i = 0;
        for(it=m_mapVariables.begin(); it!=m_mapVariables.end(); it++)
        {
            HistorizedVariable* pVariable = it->second;
            monitoredItemIds[i] = pVariable->m_monitoredItemId;
            i++;
        }

        m_pServerManager->deleteMonitoredItems(m_pSession, monitoredItemIds, results);
    }

    for ( it=m_mapVariables.begin(); it!=m_mapVariables.end(); it++ )
    {
        delete it->second;
        it->second = NULL;
    }
    m_mapVariables.clear();

    // Release Session object
    if( m_pSession )
    {
        m_pSession->releaseReference();
        m_pSession = NULL;
    }
}


HistorizedVariable::HistorizedVariable()
: m_pVariable(NULL),
  m_monitoredItemId(0),
  m_isValid(OpcUa_False)
{
}

HistorizedVariable::~HistorizedVariable()
{
}

// Handle data changes from the monitored items and store the values in the
// memory buffer
void HistorizedVariable::dataChange(const UaDataValue& dataValue)
{
    UaMutexLocker locker(&m_mutex);
    m_values.push_back(dataValue);
    if ( m_values.size() > HISTORYMANAGERBA_QUEUE_SIZE )
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