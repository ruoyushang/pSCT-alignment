#ifndef __PASOBJECT_H__
#define __PASOBJECT_H__

#include "uaserver/uaobjecttypes.h"
#include "uaserver/methodmanager.h"
#include "uaserver/opcua_offnormalalarmtype.h"
#include "uaserver/opcua_baseobjecttype.h"
#include "uaserver/opcua_analogitemtype.h"
#include "uaserver/nodemanagerroot.h"
#include "userdatabase.h"
#include "components.h"

class PasNodeManagerCommon;
class PasComInterfaceCommon;
class UaMethodGeneric;
struct Identity;

class MirrorObject : public PasObject;
class PanelObject : public PasObject;
class EdgeObject : public PasObject;
class CCDObject : public PasObject;

class PasObject :  public OpcUa::BaseObjectType
{
    UA_DISABLE_COPY(PasObject);
public:
    PasObject(const UaString& name,
            const UaNodeId& newNodeId,
            const UaString& defaultLocaleId,
            PasNodeManagerCommon *pNodeManager,
            Identity identity,
            PasComInterfaceCommon *pCommIf);
    virtual ~PasObject(void);
    virtual UaNodeId typeDefinitionId() const = 0;

    // Implement UaObject interface
    OpcUa_Byte eventNotifier() const;
    // Override UaObject method implementation
    MethodManager* getMethodManager(UaMethod* pMethod) const;

    // Implement MethodManager interface
    // general beginCall that spawns a worker thread and calls call() on the desired method
    // from within the thread
    virtual UaStatus beginCall(
        MethodManagerCallback* pCallback,
        const ServiceContext&  serviceContext,
        OpcUa_UInt32           callbackHandle,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments);

    /* Own synchronous call implementation that has to be implemented in subclasses */
    virtual UaStatus call(
        const ServiceContext&  serviceContext,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments,
        UaVariantArray&        outputArguments,
        UaStatusCodeArray&     inputArgumentResults,
        UaDiagnosticInfos&     inputArgumentDiag) = 0;

    // Factory pattern
    static PasObject * makeObject(
            unsigned deviceType,
            const UaString& name,
            const UaNodeId& newNodeId,
            const UaString& defaultLocaleId,
            PasNodeManagerCommon *pNodeManager,
            Identity identity,
            PasComInterfaceCommon *pCommIf);

protected:
    // a function that's used very often
    OpcUa::DataItemType* addVariable(PasNodeManagerCommon *pNodeManager, OpcUa_UInt32 ParentType, OpcUa_UInt32 VarType, OpcUa_Boolean isState = OpcUa_False);

    UaString                   m_defaultLocaleId;
    UaMutexRefCounted*         m_pSharedMutex;
    Identity                   m_Identity;
    PasComInterfaceCommon*     m_pCommIf;
    PasNodeManagerCommon*      m_pNodeManager;
    OpcUa::OffNormalAlarmType* m_pStateOffNormalAlarm;

};

class PasUserData : public UserDataBase
{
    UA_DISABLE_COPY(PasUserData);
public:
    PasUserData(OpcUa_Boolean isState, OpcUa_UInt32 type,
            Identity identity,
            OpcUa_UInt32 variableOffset) :
    m_isState(isState), m_Type(type), m_Identity(identity), m_variableOffset(variableOffset) {}

    virtual ~PasUserData(){}

    /** Indicates if this is a state variable. */
    inline OpcUa_UInt32 isState() const { return m_isState; }
    /** Returns the device Identity. */
    inline Identity DeviceId() const { return m_Identity; }
    /** Returns the device Type. */
    inline OpcUa_UInt32 DeviceType() const { return m_Type; }
    /** Returns the variable offset in the device. */
    inline OpcUa_UInt32 variableOffset() const { return m_variableOffset; }

private:
    OpcUa_Boolean m_isState;
    OpcUa_UInt32  m_Type;
    Identity      m_Identity;
    OpcUa_UInt32  m_variableOffset;
};


class MPESObject : public PasObject
{
    UA_DISABLE_COPY(MPESObject);
public:
    MPESObject(
        const UaString& name,
        const UaNodeId& newNodeId,
        const UaString& defaultLocaleId,
        PasNodeManagerCommon* pNodeManager,
        Identity identity,
        PasComInterfaceCommon *pCommIf);
    virtual ~MPESObject(void);

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

class ACTObject : public PasObject
{
    UA_DISABLE_COPY(ACTObject);
public:
    ACTObject(
        const UaString& name,
        const UaNodeId& newNodeId,
        const UaString& defaultLocaleId,
        PasNodeManagerCommon* pNodeManager,
        Identity identity,
        PasComInterfaceCommon *pCommIf);
    virtual ~ACTObject(void);

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
    UaMethodGeneric* m_pMethodStep;
};


class PSDObject : public PasObject
{
    UA_DISABLE_COPY(PSDObject);
public:
    PSDObject(
        const UaString& name,
        const UaNodeId& newNodeId,
        const UaString& defaultLocaleId,
        PasNodeManagerCommon* pNodeManager,
        Identity identity,
        PasComInterfaceCommon *pCommIf);
    virtual ~PSDObject(void);

    UaStatus call(
        const ServiceContext&  serviceContext,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments,
        UaVariantArray&        outputArguments,
        UaStatusCodeArray&     inputArgumentResults,
        UaDiagnosticInfos&     inputArgumentDiag);

    UaNodeId typeDefinitionId() const;

private:
    UaMethodGeneric* m_pMethodUpdate;
};

#endif
