#ifndef __PASNODEMANAGERCOMMON_H__
#define __PASNODEMANAGERCOMMON_H__

#include "uaserver/nodemanagerbase.h"

class PasComInterfaceCommon;

class PasNodeManagerCommon : public NodeManagerBase
{
public:
    PasNodeManagerCommon();
    virtual ~PasNodeManagerCommon();

    // NodeManagerUaNode implementation
    virtual UaStatus afterStartUp() = 0;
    virtual UaStatus beforeShutDown() = 0;

    // IOManagerUaNode implementation
    virtual UaStatus readValues(const UaVariableArray &arrUaVariables, UaDataValueArray &arrDataValues);
    virtual UaStatus writeValues(const UaVariableArray &arrUaVariables, const PDataValueArray &arrpDataValues, UaStatusCodeArray &arrStatusCodes);

    // EventManagerUaNode implementation
    virtual UaStatus OnAcknowledge(const ServiceContext& serviceContext, OpcUa::AcknowledgeableConditionType* pCondition, const UaByteString& EventId, const UaLocalizedText& Comment);

    UaVariable* getInstanceDeclarationVariable(OpcUa_UInt32 numericIdentifier);
    PasComInterfaceCommon *getComInterface() {return m_pCommIf;}

protected:
    UaStatus createTypeNodes();

    std::unique_ptr<PasComInterfaceCommon> m_pCommIf;
};

#endif // __PASNODEMANAGER_H__
