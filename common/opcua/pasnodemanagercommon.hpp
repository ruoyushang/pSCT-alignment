#ifndef COMMON_PASNODEMANAGERCOMMON_HPP
#define COMMON_PASNODEMANAGERCOMMON_HPP

#include <memory>

#include "uaserver/nodemanagerbase.h"

class PasComInterfaceCommon;

class PasNodeManagerCommon : public NodeManagerBase
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

protected:
    UaStatus createTypeNodes();

    std::shared_ptr<PasComInterfaceCommon> m_pCommIf;
};

#endif // COMMON_PASNODEMANAGER_HPP
