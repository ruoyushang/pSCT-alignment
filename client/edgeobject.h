#ifndef __PASEDGE_H__
#define __PASEDGE_H__

#include "uaserver/methodmanager.h"
#include "common/alignment/device.hpp"
#include "pasobject.h"

class PasNodeManager;
class PasCommunicationInterface;
class UaMethodGeneric;

class EdgeObject : public PasObject
{
    UA_DISABLE_COPY(EdgeObject);
public:
    EdgeObject(
            const UaString& name,
            const UaNodeId& newNodeId,
            const UaString& defaultLocaleId,
            PasNodeManager* pNodeManager,
            Device::Identity identity,
            PasCommunicationInterface *pCommIf);
    virtual ~EdgeObject(void);

    UaStatus call(
        const ServiceContext&  serviceContext,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments,
        UaVariantArray&        outputArguments,
        UaStatusCodeArray&     inputArgumentResults,
        UaDiagnosticInfos&     inputArgumentDiag);

    UaNodeId typeDefinitionId() const;

private:
    UaMethodGeneric* m_pMethodStop;
    UaMethodGeneric* m_pMethodMove;
    UaMethodGeneric* m_pMethodFindMatrix;
    UaMethodGeneric* m_pMethodAlign;
    UaMethodGeneric* m_pMethodRead;
};

#endif

