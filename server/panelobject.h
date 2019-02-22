#ifndef __PASPANEL_H__
#define __PASPANEL_H__

#include "uaserver/methodmanager.h"
#include "components.h"
#include "pasobject.h"

class PasNodeManager;
class PasCommunicationInterface;
class UaMethodGeneric;
struct Identity;

class PanelObject : public PasObject
{
    UA_DISABLE_COPY(PanelObject);
public:
    PanelObject(
        const UaString& name,
        const UaNodeId& newNodeId,
        const UaString& defaultLocaleId,
        PasNodeManager* pNodeManager,
        Identity identity,
        PasCommunicationInterface *pCommIf);
    virtual ~PanelObject(void);

    UaStatus call(
        const ServiceContext&  serviceContext,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments,
        UaVariantArray&        outputArguments,
        UaStatusCodeArray&     inputArgumentResults,
        UaDiagnosticInfos&     inputArgumentDiag);

    UaNodeId typeDefinitionId() const;

private:
    UaMethodGeneric* m_pMethodStepAll;
    UaMethodGeneric* m_pMethodMoveTo;
    UaMethodGeneric* m_pMethodStop;
};

#endif

