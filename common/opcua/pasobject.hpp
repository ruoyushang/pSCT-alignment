#ifndef COMMON_PASOBJECT_HPP
#define COMMON_PASOBJECT_HPP

#include "uaserver/uaobjecttypes.h"
#include "uaserver/methodmanager.h"
#include "uaserver/opcua_offnormalalarmtype.h"
#include "uaserver/opcua_baseobjecttype.h"
#include "uaserver/opcua_analogitemtype.h"
#include "uaserver/nodemanagerroot.h"
#include "userdatabase.h"
#include "common/alignment/device.hpp"
#include "common/opcua/passervertypeids.hpp"

class PasNodeManager;
class PasCommunicationInterface;
class PasNodeManagerCommon;
class PasComInterfaceCommon;
class UaMethodGeneric;

class PasObject :  public OpcUa::BaseObjectType
{
    UA_DISABLE_COPY(PasObject);
public:
    PasObject(const UaString& name,
              const UaNodeId& newNodeId,
              const UaString& defaultLocaleId,
              PasNodeManagerCommon *pNodeManager,
              Device::Identity identity,
              PasComInterfaceCommon *pCommIf);

    ~PasObject() override;

    void initialize();

    // Implement UaObject interface
    OpcUa_Byte eventNotifier() const override;
    // Override UaObject method implementation
    MethodManager *getMethodManager(UaMethod *pMethod) const override;

    // Implement MethodManager interface
    // general beginCall that spawns a worker thread and calls call() on the desired method
    // from within the thread
    UaStatus beginCall(
        MethodManagerCallback* pCallback,
        const ServiceContext&  serviceContext,
        OpcUa_UInt32           callbackHandle,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments) override;

    /* Own synchronous call implementation that has to be implemented in subclasses */
    UaStatus call(
            const ServiceContext&  serviceContext,
            MethodHandle*          pMethodHandle,
            const UaVariantArray&  inputArguments,
            UaVariantArray&        outputArguments,
            UaStatusCodeArray&     inputArgumentResults,
            UaDiagnosticInfos &inputArgumentDiag) override;


    virtual const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>>
    getVariableDefs() = 0;

    virtual const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>>
    getErrorDefs() = 0;

    virtual const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>>
    getMethodDefs() = 0;

    Device::Identity getIdentity() { return m_Identity; }

protected: 
    // a function that's used very often
    OpcUa::DataItemType* addVariable(PasNodeManagerCommon *pNodeManager, OpcUa_UInt32 ParentType, OpcUa_UInt32 VarType, OpcUa_Boolean isState = OpcUa_False, OpcUa_Boolean addReference = OpcUa_True);

    /// @brief Map from the OPC UA node IDs for all supported method nodes to the corresponding method node and the OPC UA type ID for the method type node.
    std::map<UaNodeId, std::pair<UaMethodGeneric *, OpcUa_UInt32>> m_MethodMap;

    UaString                   m_defaultLocaleId;
    UaMutexRefCounted*         m_pSharedMutex;
    Device::Identity m_Identity;
    PasComInterfaceCommon*     m_pCommIf;
    PasNodeManagerCommon*      m_pNodeManager;
    UaNodeId m_newNodeId;

private:
};

class PasUserData : public UserDataBase
{
    UA_DISABLE_COPY(PasUserData);
public:
    PasUserData(OpcUa_Boolean isState, OpcUa_UInt32 type,
                Device::Identity identity,
                OpcUa_UInt32 variableOffset) :
        m_isState(isState), m_Type(type), m_Identity(std::move(identity)), m_variableOffset(variableOffset) {}

    /** Indicates if this is a state variable. */
    inline OpcUa_UInt32 isState() const { return m_isState; }
    /** Returns the device Identity. */
    inline Device::Identity DeviceId() const { return m_Identity; }
    /** Returns the device Type. */
    inline OpcUa_UInt32 DeviceType() const { return m_Type; }
    /** Returns the variable offset in the device. */
    inline OpcUa_UInt32 variableOffset() const { return m_variableOffset; }
    /** Update timestamp flag after write. */
    inline OpcUa_UInt32 updateTimestamp() const { return m_updateTimestamp; }
    inline void setUpdateTimestamp(OpcUa_Boolean updateTimestamp) { m_updateTimestamp = updateTimestamp; }

private:
    OpcUa_Boolean m_isState;
    OpcUa_UInt32  m_Type;
    Device::Identity m_Identity;
    OpcUa_UInt32  m_variableOffset;
    OpcUa_Boolean m_updateTimestamp;
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
        Device::Identity identity,
        PasComInterfaceCommon *pCommIf) : PasObject(name, newNodeId, defaultLocaleId, pNodeManager, std::move(identity),
                                                        pCommIf) { initialize(); }

    UaNodeId typeDefinitionId() const override { return UaNodeId(PAS_MPESType, browseName().namespaceIndex()); }

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>>
    getVariableDefs() override { return MPESObject::VARIABLES; }

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>>
    getErrorDefs() override { return MPESObject::ERRORS; }

    const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>>
    getMethodDefs() override { return MPESObject::METHODS; }

    /// @brief Map of OPC UA type ids for all child variables to their name, default value, is_state value, and access level.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> VARIABLES;

    /// @brief Map of OPC UA type ids for all child error variables to their name, default value, and is_state value.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> ERRORS;

    /// @brief Map of OPC UA type ids for all child methods to their name and number of arguments.
    static const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> METHODS;

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
        Device::Identity identity,
        PasComInterfaceCommon *pCommIf) : PasObject(name, newNodeId, defaultLocaleId, pNodeManager, std::move(identity),
                                                        pCommIf) { initialize(); }

    UaNodeId typeDefinitionId() const override { return UaNodeId(PAS_ACTType, browseName().namespaceIndex()); }

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>>
    getVariableDefs() override { return ACTObject::VARIABLES; }

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>>
    getErrorDefs() override { return ACTObject::ERRORS; }

    const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>>
    getMethodDefs() override { return ACTObject::METHODS; }

    /// @brief Map of OPC UA type ids for all child variables to their name, default value, is_state value, and access level.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> VARIABLES;

    /// @brief Map of OPC UA type ids for all child error variables to their name, default value, and is_state value.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> ERRORS;

    /// @brief Map of OPC UA type ids for all child methods to their name and number of arguments.
    static const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> METHODS;
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
        Device::Identity identity,
        PasComInterfaceCommon *pCommIf) : PasObject(name, newNodeId, defaultLocaleId, pNodeManager, std::move(identity),
                                                        pCommIf) { initialize(); }

    UaNodeId typeDefinitionId() const override { return UaNodeId(PAS_PSDType, browseName().namespaceIndex()); }

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>>
    getVariableDefs() override { return PSDObject::VARIABLES; }

    const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>>
    getErrorDefs() override { return PSDObject::ERRORS; }

    const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>>
    getMethodDefs() override { return PSDObject::METHODS; }

    /// @brief Map of OPC UA type ids for all child variables to their name, default value, is_state value, and access level.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> VARIABLES;

    /// @brief Map of OPC UA type ids for all child error variables to their name, default value, and is_state value.
    static const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> ERRORS;

    /// @brief Map of OPC UA type ids for all child methods to their name and number of arguments.
    static const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> METHODS;
};

#endif //COMMON_PASOBJECT_HPP
