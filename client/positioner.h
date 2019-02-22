#ifndef __POSITIONER_H__
#define __POSITIONER_H__

#include "uaserver/methodmanager.h"
#include "components.h"
#include "pasobject.h"

class PasNodeManager;
class PasCommunicationInterface;
class UaMethodGeneric;
struct Identity;

class PositionerObject : public PasObject
{
    UA_DISABLE_COPY(PositionerObject);
public:
    PositionerObject(
        const UaString& name,
        const UaNodeId& newNodeId,
        const UaString& defaultLocaleId,
        PasNodeManager* pNodeManager,
        Identity identity,
        PasCommunicationInterface *pCommIf);
    virtual ~PositionerObject(void);

    UaStatus call(
        const ServiceContext&  serviceContext,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments,
        UaVariantArray&        outputArguments,
        UaStatusCodeArray&     inputArgumentResults,
        UaDiagnosticInfos&     inputArgumentDiag);

    UaNodeId typeDefinitionId() const;

private:
    UaMethodGeneric* m_pMethodInit;
    UaMethodGeneric* m_pMethodStop;
    UaMethodGeneric* m_pMethodSetEnergy;
    UaMethodGeneric* m_pMethodMoveTo;
};

#endif
