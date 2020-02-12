#ifndef COMMON_PASNODEMANAGERCOMMON_HPP
#define COMMON_PASNODEMANAGERCOMMON_HPP

#include <memory>

#include "uaserver/nodemanagerbase.h"

class PasComInterfaceCommon;

class PasNodeManagerCommon :
//        // SamplingOnRequestExample change begin
//        // Added: Worker thread to execute the sampling
//        public UaThread,
//        // SamplingOnRequestExample change end
        public NodeManagerBase
{
public:
    PasNodeManagerCommon();

    // IOManagerUaNode implementation
    UaStatus readValues(const UaVariableArray &arrUaVariables, UaDataValueArray &arrDataValues) override;
    UaStatus writeValues(const UaVariableArray &arrUaVariables, const PDataValueArray &arrpDataValues, UaStatusCodeArray &arrStatusCodes) override;

    // EventManagerUaNode implementation
    UaStatus OnAcknowledge(const ServiceContext& serviceContext, OpcUa::AcknowledgeableConditionType* pCondition, const UaByteString& EventId, const UaLocalizedText& Comment) override;

    UaVariable* getInstanceDeclarationVariable(OpcUa_UInt32 numericIdentifier);

    const std::shared_ptr<PasComInterfaceCommon> &getComInterface() { return m_pCommIf; }

//    void startLogging();
//    void stopLogging();

    void __updatepCommIf(std::shared_ptr<PasComInterfaceCommon> pCommIf) { m_pCommIf = pCommIf; }
protected:
    UaStatus createTypeNodes();

//#if SUPPORT_Historical_Access
//    HistoryManagerCache*              m_pHistoryManagerCache;
//#endif // SUPPORT_Historical_Access
    std::shared_ptr<PasComInterfaceCommon> m_pCommIf;

//    // SamplingOnRequestExample change begin
//    // Added: Member variables for internal sampling in worker thread
//    bool                                         m_stopThread;
//    UaMutex                                      m_mutexMonitoredVariables;
//    bool                                         m_changedMonitoredVariables;
//    std::map<UaVariableCache*, UaVariableCache*> m_mapMonitoredVariables;
//    UaVariableArray                              m_arrayMonitoredVariables;
//    // SamplingOnRequestExample change end

};

#endif // COMMON_PASNODEMANAGER_HPP
