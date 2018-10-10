#ifndef __CONTROLLEROBJECT_H__
#define __CONTROLLEROBJECT_H__

#include "uaobjecttypes.h"
#include "methodmanager.h"
#include "userdatabase.h"

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

    // Own synchronous call implementation that can be overridden in subclasses
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
    NmBuildingAutomation* m_pNodeManager;
    UaMethodGeneric*      m_pMethodStart;
    UaMethodGeneric*      m_pMethodStop;
};

class BaUserData : public UserDataBase
{
    UA_DISABLE_COPY(BaUserData);
public:
    BaUserData(
        OpcUa_Boolean   isState,
        OpcUa_UInt32    deviceAddress,
        OpcUa_UInt32    variableOffset)
    : m_isState(isState),
      m_deviceAddress(deviceAddress),
      m_variableOffset(variableOffset)
    {}
    virtual ~BaUserData(){}

    /** Indicates if this is a state variable. */
    inline OpcUa_UInt32 isState() const { return m_isState; }
    /** Returns the device address. */
    inline OpcUa_UInt32 deviceAddress() const { return m_deviceAddress; }
    /** Returns the variable offset in the device. */
    inline OpcUa_UInt32 variableOffset() const { return m_variableOffset; }

private:
    OpcUa_Boolean m_isState;
    OpcUa_UInt32  m_deviceAddress;
    OpcUa_UInt32  m_variableOffset;
};

#endif
