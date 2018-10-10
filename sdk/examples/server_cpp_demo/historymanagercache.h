#ifndef __HISTORYMANAGERCACHE_H__
#define __HISTORYMANAGERCACHE_H__

#include "historymanagerbase.h"
#include "uabasenodes.h"
#include "servermanager.h"
#include "uaeventfilter.h"
#include "uaeventdata.h"

class HistorizedEventNotifier;

class HistorizedVariable : public IOVariableCallback
{
    UA_DISABLE_COPY(HistorizedVariable);
public:
    HistorizedVariable();
    virtual ~HistorizedVariable();
 
    // Interface IOVariableCallback
    void dataChange(const UaDataValue& dataValue);
    UaDataValue getLastValue();

    // Mutex for the data and settings
    UaMutex                m_mutex;
    // Variable to historize
    UaVariable*            m_pVariable;
    // MonitoredItemId for internal monitoring
    OpcUa_UInt32           m_monitoredItemId;
    // Flag indicating if the variable is added for internal monitoring
    OpcUa_Boolean          m_isHistorizing;
    // Memory buffer for the changed values
    std::list<UaDataValue>  m_values;
};

class HistorizedEventData : public UaEventData
{
    UA_DISABLE_COPY(HistorizedEventData);
public:
    HistorizedEventData(UaVariantArray eventFields, const HistorizedEventNotifier* pHistorizedEventNotifier, OpcUa_Boolean detach = OpcUa_False);
    virtual ~HistorizedEventData();

    virtual void getFieldData(OpcUa_UInt32 index, Session* pSession, OpcUa_Variant& data);

    const HistorizedEventNotifier* m_pHistorizedEventNotifier;
private:
    UaVariantArray m_eventFields;
};

#if SUPPORT_Event_Subscription_Server_Facet
class HistorizedEventNotifier : public EventCallback
{
    UA_DISABLE_COPY(HistorizedEventNotifier);
public:
    HistorizedEventNotifier(const UaEventFilter &filter);
    virtual ~HistorizedEventNotifier();

    // Interface EventCallback
    virtual void newEvent(UaVariantArray& eventFields, OpcUa_Boolean detach);
    virtual void invalidateEventManager(OpcUa_UInt32 eventManagerIndex);

    OpcUa_UInt32 getFieldIndex(OpcUa_UInt32 indexSDK, OpcUa_StatusCode& fieldResult);

    // Mutex for the data and settings
    UaMutex                                 m_mutex;
    // EventNotifier object
    UaObject*                               m_pObject;
    // MonitoredItemId for internal monitoring
    OpcUa_UInt32                            m_monitoredItemId;
    // Flag indicating if the variable is added for internal monitoring
    OpcUa_Boolean                           m_isHistorizing;
    // Memory buffer for the event data
    std::list<HistorizedEventData*>         m_eventFieldLists;
    // map of global index of eventField in SDK to index in local VariantArray;
    // key is global index - value is index of UaVariant in one entry of m_eventFieldLists.
    std::map<OpcUa_UInt32, OpcUa_UInt32>    m_mapFieldIndex;
    // save the filter for the event monitored item here
    OpcUa_EventFilter                       m_eventFilter;
};
#endif // SUPPORT_Event_Subscription_Server_Facet

class HistoryManagerCache : public HistoryManagerBase
{
    UA_DISABLE_COPY(HistoryManagerCache);
public:
    HistoryManagerCache();
    virtual ~HistoryManagerCache();

    // Interface HistoryManagerBase
    virtual UaStatus readRaw (
        const ServiceContext&       serviceContext,
        HistoryVariableHandle*      pVariableHandle,
        HistoryReadCPUserDataBase** ppContinuationPoint,
        OpcUa_TimestampsToReturn    timestampsToReturn,
        OpcUa_UInt32                maxValues,
        OpcUa_DateTime&             startTime,
        OpcUa_DateTime&             endTime,
        OpcUa_Boolean               returnBounds,
        OpcUa_HistoryReadValueId*   pReadValueId,
        UaDataValues&               dataValues);
    virtual UaStatus readEvents (
        const ServiceContext&       serviceContext,
        HistoryVariableHandle*      pVariableHandle,
        HistoryReadCPUserDataBase** ppContinuationPoint,
        OpcUa_UInt32                numValuesPerNode,
        OpcUa_DateTime&             startTime,
        OpcUa_DateTime&             endTime,
        const OpcUa_EventFilter&    filter,
        OpcUa_HistoryReadValueId*   pReadValueId,
        UaHistoryEventFieldLists&   events);
    virtual UaStatus updateData (
        const ServiceContext&   serviceContext,
        HistoryVariableHandle*  pVariableHandle,
        OpcUa_PerformUpdateType performInsertReplace,
        OpcUa_Boolean           isStructureUpdate,
        const UaDataValues&     updateValue,
        UaStatusCodeArray&      operationResults,
        UaDiagnosticInfos&      operationDiagnosticInfos);
    virtual UaStatus updateEvents (
        const ServiceContext&           serviceContext,
        HistoryVariableHandle*          pVariableHandle,
        OpcUa_PerformUpdateType         performInsertReplace,
        const OpcUa_EventFilter&        filter,
        const UaHistoryEventFieldLists& eventData,
        UaStatusCodeArray&              operationResults,
        UaDiagnosticInfos&              operationDiagnosticInfos);
    virtual UaStatus deleteRawModified (
        const ServiceContext&    serviceContext,
        HistoryVariableHandle*   pVariableHandle,
        OpcUa_Boolean            isDeleteModified,
        OpcUa_DateTime&          startTime,
        OpcUa_DateTime&          endTime);
    virtual UaStatus deleteEvents (
        const ServiceContext&    serviceContext,
        HistoryVariableHandle*   pVariableHandle,
        const UaByteStringArray& eventIds,
        UaStatusCodeArray&       operationResults,
        UaDiagnosticInfos&       operationDiagnosticInfos);
    // Interface HistoryManagerBase

    // control Historizing
    // Startup the HistoryManagerCache
    void startUp(ServerManager* pServerManager);
    // Shutdown the HistoryManagerCache
    void shutDown();
    // Start internal monitoring and log history
    void startLogging();
    // Stop internal monitoring and logging of history
    void stopLogging();
    // Add a variable for historizing.
    void addVariableToHistorize(UaNode* pNode);
    // Remove a variable from historizing.
    void removeVariableToHistorize(UaNode* pNode);
    // Add an EventNotifier for historizing.
    UaStatus addEventNotifierToHistorize(UaNode* pNode, const UaEventFilter &eventFilter);
    // Remove an EventNotifier from historizing.
    void removeEventNotifierFromHistorize(UaNode* pNode);

private:
    // List of variable to historize
    std::map<UaNodeId, HistorizedVariable*> m_mapVariables;
#if SUPPORT_Event_Subscription_Server_Facet
    // List of objects to historize
    std::map<UaNodeId, HistorizedEventNotifier*> m_mapObjects;
#endif // SUPPORT_Event_Subscription_Server_Facet
    // mutex to protect the maps above
    UaMutex                                 m_mutexVariables;
    // Internal session used for monitoring
    Session*                                m_pSession;
    // Server manager used for internal monitoring
    ServerManager*                          m_pServerManager;
    // flag to indicate if loggin is active or not
    OpcUa_Boolean                           m_loggingActive;
};

#endif // __HISTORYMANAGERCACHE_H__
