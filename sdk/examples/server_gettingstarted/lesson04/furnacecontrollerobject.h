#ifndef __FURNACECONTROLLEROBJECT_H__
#define __FURNACECONTROLLEROBJECT_H__

#include "controllerobject.h"

class FurnaceControllerObject :
    public ControllerObject
{
    UA_DISABLE_COPY(FurnaceControllerObject);
public:
    FurnaceControllerObject(
        const UaString& name,
        const UaNodeId& newNodeId,
        const UaString& defaultLocaleId,
        NmBuildingAutomation* pNodeManager,
        OpcUa_UInt32 deviceAddress,
        BaCommunicationInterface *pCommIf);
    virtual ~FurnaceControllerObject(void);

    virtual UaNodeId typeDefinitionId() const;
    
    // override Controller::call()
    virtual UaStatus call(
        UaMethod*             pMethod,
        const UaVariantArray& inputArguments,
        UaVariantArray&       /*outputArguments*/,
        UaStatusCodeArray&    inputArgumentResults,
        UaDiagnosticInfos&    /*inputArgumentDiag*/);
    
private:
    UaMethodGeneric *m_pMethodStartWithSetpoint;
};

#endif
