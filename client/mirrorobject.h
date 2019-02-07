#ifndef __PASMIRROR_H__
#define __PASMIRROR_H__

#include "uaserver/methodmanager.h"
#include "components.h"
#include "pasobject.h"

class PasNodeManager;
class PasCommunicationInterface;
class UaMethodGeneric;
struct Identity;

class MirrorObject : public PasObject
{
    UA_DISABLE_COPY(MirrorObject);
public:
    MirrorObject(
        const UaString& name,
        const UaNodeId& newNodeId,
        const UaString& defaultLocaleId,
        PasNodeManager* pNodeManager,
        Identity identity,
        PasCommunicationInterface *pCommIf);
    virtual ~MirrorObject(void);

    UaStatus call(
        const ServiceContext&  serviceContext,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments,
        UaVariantArray&        outputArguments,
        UaStatusCodeArray&     inputArgumentResults,
        UaDiagnosticInfos&     inputArgumentDiag);

    UaNodeId typeDefinitionId() const;

private:
    UaMethodGeneric* m_pMethodMove;
    UaMethodGeneric* m_pMethodReadPos;
    UaMethodGeneric* m_pMethodReadAlign;
    UaMethodGeneric* m_pMethodAlign;
    UaMethodGeneric* m_pMethodStop;
};

#endif

