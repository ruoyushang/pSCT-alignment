#ifndef __PAS_GAS_H__
#define __PAS_GAS_H__

#include "uaserver/methodmanager.h"
#include "components.h"
#include "pasobject.h"

class PasNodeManager;
class PasCommunicationInterface;
class UaMethodGeneric;
struct Identity;

class OptTableObject : public PasObject
{
    UA_DISABLE_COPY(OptTableObject);
public:
    OptTableObject(
        const UaString& name,
        const UaNodeId& newNodeId,
        const UaString& defaultLocaleId,
        PasNodeManager* pNodeManager,
        Identity identity,
        PasCommunicationInterface *pCommIf);
    virtual ~OptTableObject(void);

    UaStatus call(
        const ServiceContext&  serviceContext,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments,
        UaVariantArray&        outputArguments,
        UaStatusCodeArray&     inputArgumentResults,
        UaDiagnosticInfos&     inputArgumentDiag);

    UaNodeId typeDefinitionId() const;

private:
    UaMethodGeneric* m_pMethodFindMatrix;
    UaMethodGeneric* m_pMethodAlign;
    UaMethodGeneric* m_pMethodStepAll;
};

class CCDObject : public PasObject
{
    UA_DISABLE_COPY(CCDObject);
public:
    CCDObject(
        const UaString& name,
        const UaNodeId& newNodeId,
        const UaString& defaultLocaleId,
        PasNodeManagerCommon* pNodeManager,
        Identity identity,
        PasComInterfaceCommon *pCommIf);
    virtual ~CCDObject(void);

    // implement the synchronous call
    UaStatus call(
        const ServiceContext&  serviceContext,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments,
        UaVariantArray&        outputArguments,
        UaStatusCodeArray&     inputArgumentResults,
        UaDiagnosticInfos&     inputArgumentDiag);

    UaNodeId typeDefinitionId() const;

private:
    UaMethodGeneric* m_pMethodStart;
    UaMethodGeneric* m_pMethodStop;
    UaMethodGeneric* m_pMethodRead;
};


#endif
