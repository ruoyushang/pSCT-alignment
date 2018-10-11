#ifndef __HISTORYMANAGERBUILDINGAUTOMATION_H__
#define __HISTORYMANAGERBUILDINGAUTOMATION_H__

#include "historymanagerbase.h"
#include "uabasenodes.h"
#include "servermanager.h"

class HistorizedVariable: public IOVariableCallback
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
    // Flag indicating if the variable is valid
    OpcUa_Boolean          m_isValid;
    // Memory buffer for the changed values
    std::list<UaDataValue> m_values;
};

class HistoryManagerBuildingAutomation : public HistoryManagerBase
{
    UA_DISABLE_COPY(HistoryManagerBuildingAutomation);
public:
    HistoryManagerBuildingAutomation();
    virtual ~HistoryManagerBuildingAutomation();

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
        OpcUa_HistoryReadValueId*   /*pReadValueId*/,
        UaDataValues&               dataValues);
    // Interface HistoryManagerBase

    // Start up history manager for internal monitoring
    void startUp(ServerManager* pServerManager);
    // Shut down history manager to stop internal monitoring
    void shutDown();
    // Add a variable for historizing - must be called before startUp
    void addVariableToHistorize(UaNode* pNode);

private:
    // List of variable to historize
    std::map<UaNodeId, HistorizedVariable*> m_mapVariables;
    // Internal session used for monitoring
    Session*                                m_pSession;
    // Server manager used for internal monitoring
    ServerManager*                          m_pServerManager;
};

#endif // __HISTORYMANAGERBUILDINGAUTOMATION_H__
