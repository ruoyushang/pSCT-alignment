#ifndef __CONTROLLEROBJECT_H__
#define __CONTROLLEROBJECT_H__

#include "uaobjecttypes.h"
#include "methodmanager.h"
#include "version_coremodule.h"
#include "bauserdata.h"
#if SUPPORT_Event_Subscription_Server_Facet
#include "opcua_offnormalalarmtype.h"
#endif // SUPPORT_Event_Subscription_Server_Facet

class NmBuildingAutomation;
class BaCommunicationInterface;
class UaMethodGeneric;

class ControllerObject :
    public UaObjectBase,
    public MethodManager
{
    UA_DISABLE_COPY(ControllerObject);
public:
    ControllerObject(
        const UaString& name,
        const UaNodeId& newNodeId,
        const UaString& defaultLocaleId,
        NmBuildingAutomation* pNodeManager,
        OpcUa_UInt32 deviceAddress,
        BaCommunicationInterface *pCommIf);
    virtual ~ControllerObject(void);

    // Implement UaObject interface
    OpcUa_Byte eventNotifier() const;
    // Override UaObject method implementation
    MethodManager* getMethodManager(UaMethod* pMethod) const;

    // Implement MethodManager interface
    virtual UaStatus beginCall(
        MethodManagerCallback* pCallback,
        const ServiceContext&  serviceContext,
        OpcUa_UInt32           callbackHandle,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments);

    // Own synchronous call implementation the can be overridden in subclasses
    virtual UaStatus call( 
        UaMethod*             /*pMethod*/,
        const UaVariantArray& /*inputArguments*/,
        UaVariantArray&       /*outputArguments*/,
        UaStatusCodeArray&    /*inputArgumentResults*/,
        UaDiagnosticInfos&    /*inputArgumentDiag*/) { return OpcUa_BadMethodInvalid; }

protected:
    UaMutexRefCounted*        m_pSharedMutex;
    OpcUa_UInt32              m_deviceAddress;
    BaCommunicationInterface* m_pCommIf;

private:
    NmBuildingAutomation*      m_pNodeManager;
    UaMethodGeneric*           m_pMethodStart;
    UaMethodGeneric*           m_pMethodStop;
#if SUPPORT_Event_Subscription_Server_Facet
    OpcUa::OffNormalAlarmType* m_pStateOffNormalAlarm;
#endif // SUPPORT_Event_Subscription_Server_Facet
};

#endif
