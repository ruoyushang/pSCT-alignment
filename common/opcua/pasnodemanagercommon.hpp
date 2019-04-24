#ifndef COMMON_PASNODEMANAGERCOMMON_HPP
#define COMMON_PASNODEMANAGERCOMMON_HPP

#include <memory>

#include "uaserver/nodemanagerbase.h"

class PasComInterfaceCommon;

class PasNodeManagerCommon : public NodeManagerBase
{
public:
    PasNodeManagerCommon();

    virtual ~PasNodeManagerCommon() = default;

    // NodeManagerUaNode implementation
    virtual UaStatus afterStartUp() = 0;
    virtual UaStatus beforeShutDown() = 0;

    // IOManagerUaNode implementation
    virtual UaStatus readValues(const UaVariableArray &arrUaVariables, UaDataValueArray &arrDataValues);
    virtual UaStatus writeValues(const UaVariableArray &arrUaVariables, const PDataValueArray &arrpDataValues, UaStatusCodeArray &arrStatusCodes);

    // EventManagerUaNode implementation
    virtual UaStatus OnAcknowledge(const ServiceContext& serviceContext, OpcUa::AcknowledgeableConditionType* pCondition, const UaByteString& EventId, const UaLocalizedText& Comment);

    UaVariable* getInstanceDeclarationVariable(OpcUa_UInt32 numericIdentifier);

    const std::unique_ptr<PasComInterfaceCommon> &getComInterface() { return m_pCommIf; }

protected:
    UaStatus createTypeNodes();

    std::unique_ptr<PasComInterfaceCommon> m_pCommIf;
};

#endif // COMMON_PASNODEMANAGER_HPP
