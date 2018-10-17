#ifndef __NMBUILDINGAUTOMATION_H__
#define __NMBUILDINGAUTOMATION_H__

#include "nodemanagerbase.h"

class BaCommunicationInterface;
#if SUPPORT_Historical_Access
class HistoryManagerCache;
#endif // SUPPORT_Historical_Access

class NmBuildingAutomation :
    // SamplingOnRequestExample change begin
    // Added: Worker thread to execute the sampling
    public UaThread,
    // SamplingOnRequestExample change end
    public NodeManagerBase
{
    UA_DISABLE_COPY(NmBuildingAutomation);
public:
    NmBuildingAutomation();
    virtual ~NmBuildingAutomation();

    // NodeManagerUaNode implementation
    virtual UaStatus   afterStartUp();
    virtual UaStatus   beforeShutDown();

    // IOManagerUaNode implementation
    virtual UaStatus readValues(const UaVariableArray &arrUaVariables, UaDataValueArray &arrDataValues);
    virtual UaStatus writeValues(const UaVariableArray &arrUaVariables, const PDataValueArray &arrpDataValues, UaStatusCodeArray &arrStatusCodes);

    // SamplingOnRequestExample change begin
    // Added: Overwrite of function variableCacheMonitoringChanged() to get informed by NodeManagerBase
    void variableCacheMonitoringChanged(UaVariableCache* pVariable, TransactionType transactionType);
    // Added: Main function for worker thread used to execute the sampling
    void run();
    // SamplingOnRequestExample change end

#if SUPPORT_Event_Subscription_Server_Facet
    // EventManagerUaNode implementation
    virtual UaStatus OnAcknowledge(const ServiceContext& serviceContext, OpcUa::AcknowledgeableConditionType* pCondition, const UaByteString& EventId, const UaLocalizedText& Comment);
#endif // SUPPORT_Event_Subscription_Server_Facet

    UaVariable* getInstanceDeclarationVariable(OpcUa_UInt32 numericIdentifier);

    void startLogging();
    void stopLogging();

private:
    UaStatus createTypeNodes();

    BaCommunicationInterface*         m_pCommIf;
#if SUPPORT_Historical_Access
    HistoryManagerCache*              m_pHistoryManagerCache;
#endif // SUPPORT_Historical_Access

    // SamplingOnRequestExample change begin
    // Added: Member variables for internal sampling in worker thread
    bool                                         m_stopThread;
    UaMutex                                      m_mutexMonitoredVariables;
    bool                                         m_changedMonitoredVariables;
    std::map<UaVariableCache*, UaVariableCache*> m_mapMonitoredVariables;
    UaVariableArray                              m_arrayMonitoredVariables;
    // SamplingOnRequestExample change end
};

#endif // __NMBUILDINGAUTOMATION_H__
