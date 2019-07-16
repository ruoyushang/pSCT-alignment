#include "common/opcua/pasobject.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

#include "uabase/uamutex.h"
#include "uaserver/methodhandleuanode.h"

#include "common/alignment/device.hpp"

#include "common/opcua/mpeseventdata.hpp"
#include "common/opcua/pascominterfacecommon.hpp"
#include "common/opcua/pasnodemanagercommon.hpp"
#include "common/opcua/passervertypeids.hpp"

// ----------------------------------------------------------------
// PasObject implementation
PasObject::PasObject(const UaString& name,
                     const UaNodeId& newNodeId,
                     const UaString& defaultLocaleId,
                     PasNodeManagerCommon *pNodeManager,
                     Device::Identity identity,
                     PasComInterfaceCommon *pCommIf) :
    BaseObjectType(newNodeId, name, pNodeManager->getNameSpaceIndex(),
                         pNodeManager->getNodeManagerConfig()),
    m_defaultLocaleId(defaultLocaleId),
    m_pSharedMutex(nullptr),
    m_Identity(std::move(identity)),
    m_pCommIf(pCommIf),
    m_pNodeManager(pNodeManager),
    m_newNodeId(newNodeId) {
}

void PasObject::initialize() {
    UaStatus addStatus;
    UaString sName;
    UaString sNodeId;
    UaPropertyMethodArgument *pPropertyArg;
    UaUInt32Array nullarray;
    OpcUa_Int16 nsIdx = m_pNodeManager->getNameSpaceIndex();
    OpcUa::DataItemType *pDataItem;

    spdlog::info("PasObject::Initialize(): initializing object w/ identity {}...", m_Identity);

    //spdlog::debug("Creating variables...");
    // Add all child variable nodes
    for (auto &kv : getVariableDefs()) {
        addVariable(m_pNodeManager, typeDefinitionId().identifierNumeric(), kv.first, std::get<2>(kv.second));
    }


    //spdlog::debug("Creating error variables...");
    //Create the folder for the Errors
    UaFolder *pErrorFolder = new UaFolder("Errors", UaNodeId(
            (std::to_string(typeDefinitionId().identifierNumeric()) + "_" + std::to_string(m_Identity.serialNumber) + "_errors").c_str(),
            nsIdx),
                                          m_defaultLocaleId);
    addStatus = m_pNodeManager->addNodeAndReference(this, pErrorFolder, OpcUaId_Organizes);
    UA_ASSERT(addStatus.isGood());

    // Add all error variable nodes
    for (const auto &v : getErrorDefs()) {
        pDataItem = addVariable(m_pNodeManager, typeDefinitionId().identifierNumeric(), v.first, OpcUa_False, OpcUa_False);
        addStatus = m_pNodeManager->addUaReference(pErrorFolder->nodeId(), pDataItem->nodeId(), OpcUaId_Organizes);
    }

    // Add all child method nodes
    //spdlog::debug("Creating method nodes...");
    for (auto &m : getMethodDefs()) {
        sName = UaString(m.second.first.c_str());
        sNodeId = UaString("%1.%2").arg(m_newNodeId.toString()).arg(sName);
        m_MethodMap[UaNodeId(sNodeId, nsIdx)] = std::make_pair(
                new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId), m.first);
        addStatus = m_pNodeManager->addNodeAndReference(this, m_MethodMap[UaNodeId(sNodeId, nsIdx)].first,
                                                        OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());

        // Add arguments
        pPropertyArg = new UaPropertyMethodArgument(
                UaNodeId((std::string(sNodeId.toUtf8()) + "_" + m.second.first + "_args").c_str(),
                         nsIdx), // NodeId of the property
                Ua_AccessLevel_CurrentRead,             // Access level of the property
                m.second.second.size(),                                      // Number of arguments
                UaPropertyMethodArgument::INARGUMENTS); // IN arguments
        for (size_t i = 0; i < m.second.second.size(); i++) {
            pPropertyArg->setArgument(
                    (OpcUa_UInt32) i,                       // Index of the argument
                    std::get<0>(m.second.second[i]).c_str(),   // Name of the argument
                    std::get<1>(m.second.second[i]),// Data type of the argument
                    -1,                      // Array rank of the argument
                    nullarray,               // Array dimensions of the argument
                    UaLocalizedText("en", (std::get<2>(m.second.second[i])).c_str())); // Description
        }
        addStatus = m_pNodeManager->addNodeAndReference(m_MethodMap[UaNodeId(sNodeId, nsIdx)].first, pPropertyArg,
                                                        OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());
    }
}

PasObject::~PasObject() {
    if (m_pSharedMutex) {
        m_pSharedMutex->releaseReference(); // Release our local reference
        m_pSharedMutex = nullptr;
    }
}

OpcUa_Byte PasObject::eventNotifier() const
{
    return Ua_EventNotifier_SubscribeToEvents;
}


MethodManager* PasObject::getMethodManager(UaMethod* pMethod) const
{
    OpcUa_ReferenceParameter(pMethod);
    return (MethodManager*)this;
}


// generic begin call -- calls worker thread that needs to be implemented for device;
// that is, only <ObjectType>::call() method needs to be implemented for asynchronous calls
// to work through UaSession::beginCall() on the client side
UaStatus PasObject::beginCall(
    MethodManagerCallback* pCallback,
    const ServiceContext&  serviceContext,
    OpcUa_UInt32           callbackHandle,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments)
{
    UaStatus ret;

    auto pCallJob = new OpcUa::MethodCallJob;
    pCallJob->initialize(this, pCallback, serviceContext, callbackHandle, pMethodHandle, inputArguments);
    ret = NodeManagerRoot::CreateRootNodeManager()->pServerManager()->getThreadPool()->addJob(pCallJob);
    if ( ret.isBad() )
    {
        delete pCallJob;
    }

    return ret;
}

UaStatus PasObject::call(
        const ServiceContext &serviceContext,
        MethodHandle *pMethodHandle,
        const UaVariantArray &inputArguments,
        UaVariantArray &outputArguments,
        UaStatusCodeArray &inputArgumentResults,
        UaDiagnosticInfos &inputArgumentDiag) {
    UaStatus ret;
    auto pMethodHandleUaNode = dynamic_cast<MethodHandleUaNode *>(pMethodHandle);
    UaMethod *pMethod = nullptr;

    int numArgs;
    OpcUa_UInt32 methodTypeID;

    if (pMethodHandleUaNode) {
        pMethod = pMethodHandleUaNode->pUaMethod();

        if (m_MethodMap.find(pMethod->nodeId()) != m_MethodMap.end()) {
            methodTypeID = m_MethodMap[pMethod->nodeId()].second;
            numArgs = getMethodDefs().at(methodTypeID).second.size();

            if (inputArguments.length() != (unsigned) numArgs)
                ret = OpcUa_BadInvalidArgument;

            /**
             * Type checking
            for (int i = 0; i < METHODS.at(methodTypeID).second.size(); i++) {
                if ( inputArguments[i].Datatype != std::get<3>(METHODS.at(methodTypeID).second[i])) {
                    ret = OpcUa_BadInvalidArgument;
                    inputArgumentResults[i] = OpcUa_BadTypeMismatch;
                }
            }
            */
            if (ret.isGood()) {
                ret = m_pCommIf->operateDevice(typeDefinitionId().identifierNumeric(), m_Identity, methodTypeID,
                                               inputArguments);
            }
        } else {
            ret = OpcUa_BadInvalidArgument;
        }
    } else {
        ret = OpcUa_BadInvalidArgument;
    }

    return ret;
}

OpcUa::DataItemType* PasObject::addVariable(PasNodeManagerCommon *pNodeManager, OpcUa_UInt32 ParentType, OpcUa_UInt32 VarType, OpcUa_Boolean isState, OpcUa_Boolean addReference)
{
    // Get the instance declaration node used as base for this variable instance
    UaVariable* pInstanceDeclaration = pNodeManager->getInstanceDeclarationVariable(VarType);
    UA_ASSERT(pInstanceDeclaration!=NULL);
    // Create new variable and add it as component to this object
    auto pDataItem = new OpcUa::DataItemType(this, pInstanceDeclaration, pNodeManager, m_pSharedMutex);

    UaStatus addStatus;
    if (addReference)
        addStatus = pNodeManager->addNodeAndReference(this, pDataItem, OpcUaId_HasComponent);
    else
        addStatus = pNodeManager->addUaNode(pDataItem->getUaNode());
    UA_ASSERT(addStatus.isGood());

    // Store information needed to access device
    PasUserData* pUserData = new PasUserData(isState, ParentType, m_Identity, VarType);
    pDataItem->setUserData(pUserData);
    // Change value handling to get read and write calls to the node manager
    pDataItem->setValueHandling(UaVariable_Value_Cache);

    return pDataItem;
}


const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> MPESObject::VARIABLES = {
    {PAS_MPESType_State,              std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
    {PAS_MPESType_Position,           std::make_tuple("Position", UaVariant(0), OpcUa_False,
                                                      Ua_AccessLevel_CurrentRead)},
    {PAS_MPESType_Serial,             std::make_tuple("Serial", UaVariant(0), OpcUa_False, Ua_AccessLevel_CurrentRead)},
    {PAS_MPESType_xCentroidAvg,       std::make_tuple("xCentroidAvg", UaVariant(0.0), OpcUa_False,
                                                        Ua_AccessLevel_CurrentRead)},
    {PAS_MPESType_yCentroidAvg,       std::make_tuple("yCentroidAvg", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
    {PAS_MPESType_xCentroidSpotWidth, std::make_tuple("xCentroidSpotWidth", UaVariant(0.0), OpcUa_False,
                                                        Ua_AccessLevel_CurrentRead)},
    {PAS_MPESType_yCentroidSpotWidth,      std::make_tuple("yCentroidSpotWidth", UaVariant(0.0), OpcUa_False,
                                                        Ua_AccessLevel_CurrentRead)},
    {PAS_MPESType_CleanedIntensity, std::make_tuple("CleanedIntensity", UaVariant(0.0), OpcUa_False,
                                                        Ua_AccessLevel_CurrentRead)},
    {PAS_MPESType_xCentroidNominal, std::make_tuple("xCentroidNominal", UaVariant(0.0), OpcUa_False,
                                                        Ua_AccessLevel_CurrentRead)},
    {PAS_MPESType_yCentroidNominal, std::make_tuple("yCentroidNominal", UaVariant(0.0), OpcUa_False,
                                                        Ua_AccessLevel_CurrentRead)},
    {PAS_MPESType_Exposure,         std::make_tuple("Exposure", UaVariant(0), OpcUa_False,
                                                    Ua_AccessLevel_CurrentRead)},
    {PAS_MPESType_Timestamp,        std::make_tuple("Timestamp", UaVariant(""), OpcUa_False,
                                                    Ua_AccessLevel_CurrentRead)},
    {PAS_MPESType_RawTimestamp,     std::make_tuple("RawTimestamp", UaVariant(0), OpcUa_False,
                                                    Ua_AccessLevel_CurrentRead)},
    {PAS_MPESType_ErrorState,       std::make_tuple("ErrorState", UaVariant(0), OpcUa_False,
                                                        Ua_AccessLevel_CurrentRead)},
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> MPESObject::ERRORS = {
    {PAS_MPESType_Error0, std::make_tuple("[0] Bad connection. No device found", UaVariant(false), OpcUa_False)},
    {PAS_MPESType_Error1, std::make_tuple("[1] Intermittent connection, possible select timeout or other error.",
                                          UaVariant(false), OpcUa_False)},
    {PAS_MPESType_Error2, std::make_tuple("[2] Cannot find laser spot", UaVariant(false), OpcUa_False)},
    {PAS_MPESType_Error3, std::make_tuple("[3] Too bright (missing tube)", UaVariant(false), OpcUa_False)},
    {PAS_MPESType_Error4, std::make_tuple("[4] Too bright", UaVariant(false), OpcUa_False)},
    {PAS_MPESType_Error5, std::make_tuple("[5] Very uneven spot, likely near edge", UaVariant(false), OpcUa_False)},
    {PAS_MPESType_Error6, std::make_tuple("[6] Uneven spot", UaVariant(false), OpcUa_False)},
    {PAS_MPESType_Error7, std::make_tuple("[7] Large intensity deviation", UaVariant(false), OpcUa_False)},
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> MPESObject::METHODS = {
    {PAS_MPESType_TurnOn,         {"TurnOn",         {}}},
    {PAS_MPESType_TurnOff,        {"TurnOff",        {}}},
    {PAS_MPESType_Read,           {"Read",           {}}},
    {PAS_MPESType_SetExposure,    {"SetExposure",    {}}},
    {PAS_MPESType_ClearError,     {"ClearError",     {std::make_tuple("ErrorNum", UaNodeId(OpcUaId_Int32),
                                                                      "Number of error to clear")}}},
    {PAS_MPESType_ClearAllErrors, {"ClearAllErrors", {}}}
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> MPESObject::EVENTS = {
    {PAS_MPESEventType_State,              std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
    {PAS_MPESEventType_xCentroidAvg,       std::make_tuple("xCentroidAvg", UaVariant(0.0), OpcUa_False,
                                                      Ua_AccessLevel_CurrentRead)},
    {PAS_MPESEventType_yCentroidAvg,       std::make_tuple("yCentroidAvg", UaVariant(0.0), OpcUa_False,
                                                      Ua_AccessLevel_CurrentRead)},
    {PAS_MPESEventType_xCentroidSpotWidth, std::make_tuple("xCentroidSpotWidth", UaVariant(0.0), OpcUa_False,
                                                      Ua_AccessLevel_CurrentRead)},
    {PAS_MPESEventType_yCentroidSpotWidth,      std::make_tuple("yCentroidSpotWidth", UaVariant(0.0), OpcUa_False,
                                                           Ua_AccessLevel_CurrentRead)},
    {PAS_MPESEventType_CleanedIntensity, std::make_tuple("CleanedIntensity", UaVariant(0.0), OpcUa_False,
                                                    Ua_AccessLevel_CurrentRead)}
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> ACTObject::VARIABLES = {
    {PAS_ACTType_State,         std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
    {PAS_ACTType_Position,      std::make_tuple("Position", UaVariant(0), OpcUa_False, Ua_AccessLevel_CurrentRead)},
    {PAS_ACTType_Serial,        std::make_tuple("Serial", UaVariant(0), OpcUa_False, Ua_AccessLevel_CurrentRead)},
    {PAS_ACTType_DeltaLength,   std::make_tuple("DeltaLength", UaVariant(0.0), OpcUa_False,
                                                    Ua_AccessLevel_CurrentRead)},
    {PAS_ACTType_TargetLength,  std::make_tuple("TargetLength", UaVariant(0.0), OpcUa_False,
                                                    Ua_AccessLevel_CurrentRead)},
    {PAS_ACTType_CurrentLength, std::make_tuple("CurrentLength", UaVariant(0.0), OpcUa_False,
                                                   Ua_AccessLevel_CurrentRead)},
    {PAS_ACTType_ErrorState, std::make_tuple("ErrorState", UaVariant(0), OpcUa_False,
                                                   Ua_AccessLevel_CurrentRead)}

};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> ACTObject::ERRORS = {
    {PAS_ACTType_Error0,  std::make_tuple("[0] Home position not calibrated", UaVariant(false), OpcUa_False)},
    {PAS_ACTType_Error1,  std::make_tuple("[1] DBInfo not set", UaVariant(false), OpcUa_False)},
    {PAS_ACTType_Error2,  std::make_tuple("[2] MySQL Communication Error", UaVariant(false), OpcUa_False)},
    {PAS_ACTType_Error3,  std::make_tuple("[3] DB columns do not match what is expected", UaVariant(false),
                                          OpcUa_False)},
    {PAS_ACTType_Error4,  std::make_tuple("[4] ASF File is Bad", UaVariant(false), OpcUa_False)},
    {PAS_ACTType_Error5,  std::make_tuple("[5] ASF File entries do not match what is expected", UaVariant(false),
                                          OpcUa_False)},
    {PAS_ACTType_Error6,  std::make_tuple("[6] Actuator is not stepping",
                                          UaVariant(false), OpcUa_False)},
    {PAS_ACTType_Error7,  std::make_tuple("[7] Voltage stddev too high", UaVariant(false), OpcUa_False)},
    {PAS_ACTType_Error8,  std::make_tuple("[8] Actuator missed steps while stepping", UaVariant(false), OpcUa_False)},
    {PAS_ACTType_Error9,  std::make_tuple("[9] Actuator position discrepancy, too far away to recover safely",
                                          UaVariant(false),
                                          OpcUa_False)},
    {PAS_ACTType_Error10, std::make_tuple("[10] Actuator position discrepancy, small enough to recover automatically",
                                          UaVariant(false),
                                          OpcUa_False)},
    {PAS_ACTType_Error11, std::make_tuple("[11] Extend Stop Voltage too close to discontinuity", UaVariant(false),
                                          OpcUa_False)},
    {PAS_ACTType_Error12, std::make_tuple("[12] End stop differs from expected location", UaVariant(false),
                                          OpcUa_False)},
    {PAS_ACTType_Error13, std::make_tuple("[13] Probe home distance differs from expected", UaVariant(false),
                                          OpcUa_False)}
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> ACTObject::METHODS = {
    {PAS_ACTType_TurnOn,          {"TurnOn",          {}}},
    {PAS_ACTType_TurnOff,         {"TurnOff",         {}}},
    {PAS_ACTType_MoveDeltaLength, {"MoveDeltaLength", {std::make_tuple("DeltaLength", UaNodeId(OpcUaId_Double),
                                                                           "Desired change in length for the actuator (in mm).")}}},
    {PAS_ACTType_MoveToLength,   {"MoveToLength",   {std::make_tuple("TargetLength", UaNodeId(OpcUaId_Double),
                                                                     "Target length for the actuator to move to (in mm).")}}},
    {PAS_ACTType_ForceRecover,   {"ForceRecover",   {}}},
    {PAS_ACTType_ClearError,     {"ClearError",     {     std::make_tuple("ErrorNum", UaNodeId(OpcUaId_Int32),
                                                                           "Number of error to clear")}}},
    {PAS_ACTType_ClearAllErrors, {"ClearAllErrors", {}}},
    {PAS_ACTType_Stop,           {"Stop",           {}}}
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> PSDObject::VARIABLES = {
        {PAS_PSDType_State, std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
        {PAS_PSDType_x1,    std::make_tuple("x1", UaVariant(0.0), OpcUa_False,
                                            Ua_AccessLevel_CurrentRead)},
        {PAS_PSDType_y1,    std::make_tuple("y1", UaVariant(0.0), OpcUa_False,
                                            Ua_AccessLevel_CurrentRead)},
        {PAS_PSDType_x2,    std::make_tuple("x2", UaVariant(0.0), OpcUa_False,
                                            Ua_AccessLevel_CurrentRead)},
        {PAS_PSDType_y2,    std::make_tuple("y2", UaVariant(0.0), OpcUa_False,
                                            Ua_AccessLevel_CurrentRead)},
        {PAS_PSDType_dx1,   std::make_tuple("x1", UaVariant(0.0), OpcUa_False,
                                            Ua_AccessLevel_CurrentRead)},
        {PAS_PSDType_dy1,   std::make_tuple("y1", UaVariant(0.0), OpcUa_False,
                                            Ua_AccessLevel_CurrentRead)},
        {PAS_PSDType_dx2,   std::make_tuple("x2", UaVariant(0.0), OpcUa_False,
                                            Ua_AccessLevel_CurrentRead)},
        {PAS_PSDType_dy2,   std::make_tuple("y2", UaVariant(0.0), OpcUa_False,
                                            Ua_AccessLevel_CurrentRead)},
        {PAS_PSDType_Temp,  std::make_tuple("Temperature", UaVariant(0.0), OpcUa_False,
                                            Ua_AccessLevel_CurrentRead)}
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> PSDObject::ERRORS = {
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> PSDObject::METHODS = {
        {PAS_PSDType_Read, {"Read", {}}}
};

UaStatus MPESObject::beginCall(
        MethodManagerCallback* pCallback,
        const ServiceContext&  serviceContext,
        OpcUa_UInt32           callbackHandle,
        MethodHandle*          pMethodHandle,
        const UaVariantArray&  inputArguments)
{
    UaStatus            ret;
    UaVariantArray      outputArguments;
    UaStatusCodeArray   inputArgumentResults;
    UaDiagnosticInfos   inputArgumentDiag;

    auto pCallJob = new OpcUa::MethodCallJob;
    pCallJob->initialize(this, pCallback, serviceContext, callbackHandle, pMethodHandle, inputArguments);
    ret = NodeManagerRoot::CreateRootNodeManager()->pServerManager()->getThreadPool()->addJob(pCallJob);
    if ( ret.isBad() )
    {
        delete pCallJob;
    }
    else{
        // Create event
        MPESEventTypeData eventData(m_pNodeManager->getNameSpaceIndex());

//        // Handle ControllerEventType specific fields
//        auto pUserData = (PasUserData *) getUserData();

//        // CleanedIntensity
//        UaVariant varValue;
//        OpcUa_Double value;
//        m_pCommIf->getDeviceData(PAS_MPESType_CleanedIntensity, pUserData->DeviceId(), pUserData->variableOffset(), varValue);
//        varValue.toDouble(value);
//        eventData.m_CleanedIntensity.setDouble(value);

//        // State
//        Device::DeviceState state;
//        m_pCommIf->getDeviceState(PAS_MPESEventType_State, pUserData->DeviceId(), state);
//        eventData.m_State.setDouble(value);

        // Fill all default event fields
        eventData.m_SourceNode.setNodeId(nodeId());
        eventData.m_SourceName.setString(browseName().toString());
        eventData.m_Severity.setUInt16(500);
        // Set timestamps and unique EventId
        eventData.prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
        // Fire the event
        m_pNodeManager->fireEvent(&eventData);
    }

    return ret;
}