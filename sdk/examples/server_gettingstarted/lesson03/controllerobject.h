#ifndef __CONTROLLEROBJECT_H__
#define __CONTROLLEROBJECT_H__

#include "uaobjecttypes.h"
#include "userdatabase.h"

class NmBuildingAutomation;
class BaCommunicationInterface;

class ControllerObject :
    public UaObjectBase
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

    OpcUa_Byte eventNotifier() const;

protected:
    UaMutexRefCounted*        m_pSharedMutex;
    OpcUa_UInt32              m_deviceAddress;
    BaCommunicationInterface* m_pCommIf;
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
