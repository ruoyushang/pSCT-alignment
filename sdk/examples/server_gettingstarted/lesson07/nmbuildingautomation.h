#ifndef __NMBUILDINGAUTOMATION_H__
#define __NMBUILDINGAUTOMATION_H__

#include "nodemanagerbase.h"

class BaCommunicationInterface;
class HistoryManagerBuildingAutomation;

class NmBuildingAutomation : public NodeManagerBase
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

    // EventManagerUaNode implementation
    virtual UaStatus OnAcknowledge(const ServiceContext& serviceContext, OpcUa::AcknowledgeableConditionType* pCondition, const UaByteString& EventId, const UaLocalizedText& Comment);

    UaVariable* getInstanceDeclarationVariable(OpcUa_UInt32 numericIdentifier);

private:
    UaStatus createTypeNodes();

    BaCommunicationInterface*         m_pCommIf;
    HistoryManagerBuildingAutomation* m_pHistoryManager;
};

#endif // __NMBUILDINGAUTOMATION_H__
