#include "common/opcua/pasobject.h"
#include "common/opcua/pasnodemanagercommon.h"
#include "common/opcua/passervertypeids.h"
#include "common/opcua/pascominterfacecommon.h"
#include "mpeseventdata.h"
#include "uaserver/methodhandleuanode.h"

#include <iostream>

#include "uabase/uamutex.h"

// ----------------------------------------------------------------
// PasObject implementation
const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> PasObject::VARIABLES;

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> PasObject::ERRORS;

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> PasObject::METHODS;


PasObject::PasObject(const UaString& name,
        const UaNodeId& newNodeId,
        const UaString& defaultLocaleId,
        PasNodeManagerCommon *pNodeManager,
        Identity identity,
        PasComInterfaceCommon *pCommIf) :
                  BaseObjectType(newNodeId, name, pNodeManager->getNameSpaceIndex(),
                         pNodeManager->getNodeManagerConfig()),
                  m_defaultLocaleId(defaultLocaleId),
                  m_pSharedMutex(NULL),
                  m_Identity(identity),
                  m_pCommIf(pCommIf),
                  m_pNodeManager(pNodeManager)
{
    UaStatus addStatus;
    UaString sName;
    UaString sNodeId;
    UaPropertyMethodArgument *pPropertyArg;
    UaUInt32Array nullarray;
    OpcUa_Int16 nsIdx = pNodeManager->getNameSpaceIndex();
    OpcUa::DataItemType *pDataItem;

    // Add all child variable nodes

    for (auto &kv : getVariableDefs()) {
        addVariable(pNodeManager, typeDefinitionId().identifierNumeric(), kv.first, std::get<2>(kv.second));
    }

    //Create the folder for the Errors
    UaFolder *pErrorFolder = new UaFolder("Errors", UaNodeId(
            (std::to_string(typeDefinitionId().identifierNumeric()) + "_" + identity.eAddress + "_errors").c_str(),
            nsIdx),
                                          m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, pErrorFolder, OpcUaId_Organizes);
    UA_ASSERT(addStatus.isGood());

    // Add all error variable nodes
    for (auto v : getErrorDefs()) {
        pDataItem = addVariable(pNodeManager, typeDefinitionId().identifierNumeric(), v.first, OpcUa_False, false);
        addStatus = pNodeManager->addUaReference(pErrorFolder->nodeId(), pDataItem->nodeId(), OpcUaId_Organizes);
    }

    // Add all child method nodes
    for (auto &m : getMethodDefs()) {
        sName = UaString(m.second.first.c_str());
        sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
        m_MethodMap[UaNodeId(sNodeId, nsIdx)] = std::make_pair(
                new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId), m.first);
        addStatus = pNodeManager->addNodeAndReference(this, m_MethodMap[UaNodeId(sNodeId, nsIdx)].first,
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
        addStatus = pNodeManager->addNodeAndReference(m_MethodMap[UaNodeId(sNodeId, nsIdx)].first, pPropertyArg,
                                                      OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());
    }
}

PasObject::~PasObject() {
    if (m_pSharedMutex) {
        m_pSharedMutex->releaseReference(); // Release our local reference
        m_pSharedMutex = NULL;
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

    OpcUa::MethodCallJob* pCallJob = new OpcUa::MethodCallJob;
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
    MethodHandleUaNode *pMethodHandleUaNode = static_cast<MethodHandleUaNode *>(pMethodHandle);
    UaMethod *pMethod = NULL;

    int numArgs;
    OpcUa_UInt32 methodTypeID;

    if (pMethodHandleUaNode) {
        pMethod = pMethodHandleUaNode->pUaMethod();

        if (m_MethodMap.find(pMethod->nodeId()) != m_MethodMap.end()) {
            methodTypeID = m_MethodMap[pMethod->nodeId()].second;
            numArgs = METHODS.at(methodTypeID).second.size();

            if (inputArguments.length() != numArgs)
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
                ret = m_pCommIf->OperateDevice(typeDefinitionId().identifierNumeric(), m_Identity, methodTypeID,
                                               inputArguments);
            }
        } else {
            ret = OpcUa_BadInvalidArgument;
        }
    } else {
        ret = OpcUa_BadInvalidArgument;
    }
}

OpcUa::DataItemType* PasObject::addVariable(PasNodeManagerCommon *pNodeManager, OpcUa_UInt32 ParentType, OpcUa_UInt32 VarType, OpcUa_Boolean isState, OpcUa_Boolean addReference)
{
    // Get the instance declaration node used as base for this variable instance
    UaVariable* pInstanceDeclaration = pNodeManager->getInstanceDeclarationVariable(VarType);
    UA_ASSERT(pInstanceDeclaration!=NULL);
    // Create new variable and add it as component to this object
    OpcUa::DataItemType* pDataItem = new OpcUa::DataItemType(this, pInstanceDeclaration, pNodeManager, m_pSharedMutex);

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
        {PAS_PanelType_State,          std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
        {PAS_PanelType_ExtTemperature, std::make_tuple("ExternalTemperature", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
        {PAS_PanelType_IntTemperature, std::make_tuple("InternalTemperature", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)}
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> MPESObject::ERRORS = {
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> PanelObject::METHODS = {
        {PAS_PanelType_MoveDeltaLengths, {"MoveDeltaLengths", {
                                                                      std::make_tuple("Delta Length 1",
                                                                                      UaNodeId(OpcUaId_Double),
                                                                                      "Desired change in length for Actuator 1 (in mm)."),
                                                                      std::make_tuple("Delta Length 2",
                                                                                      UaNodeId(OpcUaId_Double),
                                                                                      "Desired change in length for Actuator 2 (in mm)."),
                                                                      std::make_tuple("Delta Length 3",
                                                                                      UaNodeId(OpcUaId_Double),
                                                                                      "Desired change in length for Actuator 3 (in mm)."),
                                                                      std::make_tuple("Delta Length 4",
                                                                                      UaNodeId(OpcUaId_Double),
                                                                                      "Desired change in length for Actuator 4 (in mm)."),
                                                                      std::make_tuple("Delta Length 5",
                                                                                      UaNodeId(OpcUaId_Double),
                                                                                      "Desired change in length for Actuator 5 (in mm)."),
                                                                      std::make_tuple("Delta Length 6",
                                                                                      UaNodeId(OpcUaId_Double),
                                                                                      "Desired change in length for Actuator 6 (in mm)."),
                                                              }}
        },
        {PAS_PanelType_MoveToLengths,    {"MoveToLengths",    {
                                                                      std::make_tuple("Length Actuator 1",
                                                                                      UaNodeId(OpcUaId_Double),
                                                                                      "Target length for Actuator 1 (in mm)."),
                                                                      std::make_tuple("Length Actuator 2",
                                                                                      UaNodeId(OpcUaId_Double),
                                                                                      "Target length for Actuator 2 (in mm)."),
                                                                      std::make_tuple("Length Actuator 3",
                                                                                      UaNodeId(OpcUaId_Double),
                                                                                      "Target length for Actuator 3 (in mm)."),
                                                                      std::make_tuple("Length Actuator 4",
                                                                                      UaNodeId(OpcUaId_Double),
                                                                                      "Target length for Actuator 4 (in mm)."),
                                                                      std::make_tuple("Length Actuator 5",
                                                                                      UaNodeId(OpcUaId_Double),
                                                                                      "Target length for Actuator 5 (in mm)."),
                                                                      std::make_tuple("Length Actuator 6",
                                                                                      UaNodeId(OpcUaId_Double),
                                                                                      "Target length for Actuator 6 (in mm)."),
                                                              }}
        },
        {PAS_PanelType_Stop,             {"Stop",             {}}}
};


MPESObject::MPESObject(
    const UaString& name,
    const UaNodeId& newNodeId,
    const UaString& defaultLocaleId,
    PasNodeManagerCommon *pNodeManager,
    Identity identity,
    PasComInterfaceCommon *pCommIf)
: PasObject(name, newNodeId, defaultLocaleId, pNodeManager, identity, pCommIf)
  {
    // Use a mutex shared across all variables of this object
    m_pSharedMutex = new UaMutexRefCounted;

    OpcUa::DataItemType*         pDataItem            = NULL;
    UaStatus                     addStatus;
    // Method helper
    OpcUa_Int16                  nsIdx = pNodeManager->getNameSpaceIndex();

    /**************************************************************
     * Create the sensor components
     **************************************************************/
    // Add Variable "State"
    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_State, OpcUa_True);

    // Add state alarm condition "StateCondition" as component to the object
    m_pStateOffNormalAlarm = new OpcUa::OffNormalAlarmType(
        UaNodeId(UaString("%1.StateCondition").arg(this->nodeId().toString()), nsIdx),  // NodeId
        "StateCondition",           // Name of the node used for browse name and display name
        nsIdx,                      // Namespace index of the browse name
        pNodeManager,               // Node manager responsible for this node
        this->nodeId(),             // NodeId of the source node
        this->browseName().name()); // Name of the source node
    addStatus = pNodeManager->addNodeAndReference(this, m_pStateOffNormalAlarm, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Create HasEventSource reference from object to state variable
    addStatus = pNodeManager->addUaReference(this, pDataItem, OpcUaId_HasEventSource);
    UA_ASSERT(addStatus.isGood());
    // Create HasCondition reference from state variable to condition
    addStatus = pNodeManager->addUaReference(pDataItem, m_pStateOffNormalAlarm, OpcUaId_HasCondition);
    UA_ASSERT(addStatus.isGood());
    m_pStateOffNormalAlarm->setConditionName("StateCondition");
    // Set the initial states of the condition
    m_pStateOffNormalAlarm->setAckedState(OpcUa_True);
    m_pStateOffNormalAlarm->setEnabledState(OpcUa_True);
    /* =====================================================================*/

    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_xCentroidAvg);
    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_yCentroidAvg);
    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_xCentroidSD);
    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_yCentroidSD);
    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_CleanedIntensity);
    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_xCentroidNominal);
    pDataItem = addVariable(pNodeManager, PAS_MPESType, PAS_MPESType_yCentroidNominal);

    // Add Method "Start"
    UaString sName = "Start";
    UaString sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodStart = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodStart, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    sName = "Stop";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodStop = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodStop, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Read"
    sName = "Read";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodRead = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodRead, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "SetExposure"
    sName = "SetExposure";
    sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodSetExposure = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodSetExposure, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

}



MPESObject::~MPESObject(void)
{
    if ( m_pSharedMutex )
    {
        // Release our local reference
        m_pSharedMutex->releaseReference();
        m_pSharedMutex = NULL;
    }
}



UaNodeId MPESObject::typeDefinitionId() const
{
    UaNodeId ret(PAS_MPESType, browseName().namespaceIndex());
    return ret;
}


/*==========================================================================*/
UaStatus MPESObject::call(
    const ServiceContext&  serviceContext,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments,
    UaVariantArray&        outputArguments,
    UaStatusCodeArray&     inputArgumentResults,
    UaDiagnosticInfos&     inputArgumentDiag)
{
    UaStatus            ret;
    MethodHandleUaNode* pMethodHandleUaNode = static_cast<MethodHandleUaNode*>(pMethodHandle);
    UaMethod*           pMethod             = NULL;

    if(pMethodHandleUaNode)
    {
        pMethod = pMethodHandleUaNode->pUaMethod();

        if(pMethod)
        {
            // Check if we have the start method
            if ( pMethod->nodeId() == m_pMethodStart->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->setDeviceState(PAS_MPESType, m_Identity,
                                                    PASState::On);
            }
            // Check if we have the stop method
            else if ( pMethod->nodeId() == m_pMethodStop->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->setDeviceState(PAS_MPESType, m_Identity,
                                                    PASState::Off);
            }
            // Check if we have the read method
            else if ( pMethod->nodeId() == m_pMethodRead->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_MPESType, m_Identity, PAS_MPESType_Read);
            }
            // Check if we have the SetExposure method
            else if ( pMethod->nodeId() == m_pMethodSetExposure->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_MPESType, m_Identity, PAS_MPESType_SetExposure);
            }
            // Create Sensor Events if state change succeeded
            if ( ret.isGood() )
            {
                //--------------------------------------------------------
                // Create event
                MPESEventTypeData eventData(browseName().namespaceIndex());

                // Handle MPESEventType specific fields
                m_pCommIf->getDeviceData(PAS_MPESType, m_Identity, PAS_MPESType_xCentroidAvg, eventData.m_xCentroidAvg);
                m_pCommIf->getDeviceData(PAS_MPESType, m_Identity, PAS_MPESType_yCentroidAvg, eventData.m_yCentroidAvg);
                m_pCommIf->getDeviceData(PAS_MPESType, m_Identity, PAS_MPESType_xCentroidSD, eventData.m_xCentroidSD);
                m_pCommIf->getDeviceData(PAS_MPESType, m_Identity, PAS_MPESType_yCentroidSD, eventData.m_yCentroidSD);
                m_pCommIf->getDeviceData(PAS_MPESType, m_Identity, PAS_MPESType_CleanedIntensity, eventData.m_CleanedIntensity);
                PASState state;
                m_pCommIf->getDeviceState(PAS_MPESType, m_Identity, state);
                eventData.m_State.setUInt32(static_cast<unsigned>(state));

                // Fill all default event fields
                eventData.m_SourceNode.setNodeId(nodeId());
                eventData.m_SourceName.setString(browseName().toString());
                UaString messageText;
                if (state == PASState::Off)
                {
                    messageText = UaString("State of %1 changed to OFF").arg(browseName().toString());
                }
                else
                {
                    messageText = UaString("State of %1 changed to ON").arg(browseName().toString());
                }
                eventData.m_Message.setLocalizedText(UaLocalizedText("en", messageText));
                eventData.m_Severity.setUInt16(500);
                // Set timestamps and unique EventId
                eventData.prepareNewEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());

                // Fire the event
                m_pNodeManager->fireEvent(&eventData);

                //--------------------------------------------------------
                // Change state of alarm condition
                if (state == PASState::Off)
                {
                    m_pStateOffNormalAlarm->setAckedState(OpcUa_False);
                    m_pStateOffNormalAlarm->setActiveState(OpcUa_True);
                    m_pStateOffNormalAlarm->setRetain(OpcUa_True);
                }
                else
                {
                    m_pStateOffNormalAlarm->setAckedState(OpcUa_True);
                    m_pStateOffNormalAlarm->setActiveState(OpcUa_False);
                    m_pStateOffNormalAlarm->setRetain(OpcUa_False);
                }
                m_pStateOffNormalAlarm->setMessage(UaLocalizedText("en", messageText));
                m_pStateOffNormalAlarm->triggerEvent(UaDateTime::now(), UaDateTime::now(), UaByteString());
            }
        }
        else
        {
            assert(false);
            ret = OpcUa_BadInvalidArgument;
        }
    }
    else
    {
        assert(false);
        ret = OpcUa_BadInvalidArgument;
    }

    return ret;
}

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> ACTObject::VARIABLES = {
        {PAS_ACTType_State,         std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
        {PAS_ACTType_DeltaLength,   std::make_tuple("DeltaLength", UaVariant(0.0), OpcUa_False,
                                                    Ua_AccessLevel_CurrentRead)},
        {PAS_ACTType_TargetLength,  std::make_tuple("TargetLength", UaVariant(0.0), OpcUa_False,
                                                    Ua_AccessLevel_CurrentRead)},
        {PAS_ACTType_CurrentLength, std::make_tuple("CurrentLength", UaVariant(0.0), OpcUa_False,
                                                   Ua_AccessLevel_CurrentRead)}
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> ACTObject::ERRORS = {
        {PAS_ACTType_Error0,  std::make_tuple("Error0", UaVariant(false), OpcUa_False)},
        {PAS_ACTType_Error1,  std::make_tuple("Error1", UaVariant(false), OpcUa_False)},
        {PAS_ACTType_Error2,  std::make_tuple("Error2", UaVariant(false), OpcUa_False)},
        {PAS_ACTType_Error3,  std::make_tuple("Error3", UaVariant(false), OpcUa_False)},
        {PAS_ACTType_Error4,  std::make_tuple("Error4", UaVariant(false), OpcUa_False)},
        {PAS_ACTType_Error5,  std::make_tuple("Error5", UaVariant(false), OpcUa_False)},
        {PAS_ACTType_Error6,  std::make_tuple("Error6", UaVariant(false), OpcUa_False)},
        {PAS_ACTType_Error7,  std::make_tuple("Error7", UaVariant(false), OpcUa_False)},
        {PAS_ACTType_Error8,  std::make_tuple("Error8", UaVariant(false), OpcUa_False)},
        {PAS_ACTType_Error9,  std::make_tuple("Error9", UaVariant(false), OpcUa_False)},
        {PAS_ACTType_Error10, std::make_tuple("Error10", UaVariant(false), OpcUa_False)},
        {PAS_ACTType_Error11, std::make_tuple("Error11", UaVariant(false), OpcUa_False)},
        {PAS_ACTType_Error12, std::make_tuple("Error12", UaVariant(false), OpcUa_False)},
        {PAS_ACTType_Error13, std::make_tuple("Error13", UaVariant(false), OpcUa_False)}
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> ACTObject::METHODS = {
        {PAS_ACTType_Start,           {"Start", {}}},
        {PAS_ACTType_Stop,            {"Stop",  {}}},
        {PAS_ACTType_MoveDeltaLength, {"MoveDeltaLength", {std::make_tuple("DeltaLength", UaNodeId(OpcUaId_Double),
                                                                           "Desired change in length for the actuator (in mm).")}}},
        {PAS_ACTType_MoveToLength, {"MoveToLength", {std::make_tuple("TargetLength", UaNodeId(OpcUaId_Double),
                                                                     "Target length for the actuator to move to (in mm).")}}}
};

ACTObject::ACTObject(
    const UaString& name,
    const UaNodeId& newNodeId,
    const UaString& defaultLocaleId,
    PasNodeManagerCommon *pNodeManager,
    Identity identity,
    PasComInterfaceCommon *pCommIf)
: PasObject(name, newNodeId, defaultLocaleId, pNodeManager, identity, pCommIf)
{
    // Use a mutex shared across all variables of this object
    m_pSharedMutex = new UaMutexRefCounted();

    OpcUa::DataItemType *pDataItem;
    UaStatus                     addStatus;
    // Method helper
    OpcUa_Int16                  nsIdx = pNodeManager->getNameSpaceIndex();

    // Add all child variable nodes
    for (auto v : getVariableDefs()) {
        addVariable(pNodeManager, PAS_ACTType, v.first, std::get<2>(v.second));
    }

    //Create the folder for the Errors
    UaFolder *pErrorFolder = new UaFolder("Errors", UaNodeId(("ACT_" + identity.eAddress + "_errors").c_str(), nsIdx),
                                          m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, pErrorFolder, OpcUaId_Organizes);
    UA_ASSERT(addStatus.isGood());

    // Add all error variable nodes
    for (auto v : getErrorDefs()) {
        pDataItem = addVariable(pNodeManager, PAS_ACTType, v.first, OpcUa_False, false);
        addStatus = pNodeManager->addUaReference(pErrorFolder->nodeId(), pDataItem->nodeId(), OpcUaId_Organizes);
    }

    // Add all child method nodes
    UaString sName;
    UaString sNodeId;

    UaPropertyMethodArgument *pPropertyArg = NULL;
    UaUInt32Array nullarray;
    for (auto m: getMethodDefs()) {
        sName = UaString(m.second.first.c_str());
        sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
        m_MethodMap[UaNodeId(sNodeId, nsIdx)] = std::make_pair(
                new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId), m.first);
        addStatus = pNodeManager->addNodeAndReference(this, m_MethodMap[UaNodeId(sNodeId, nsIdx)].first,
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
        addStatus = pNodeManager->addNodeAndReference(m_MethodMap[UaNodeId(sNodeId, nsIdx)].first, pPropertyArg,
                                                      OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());
    }
}

ACTObject::~ACTObject(void)
{
    if ( m_pSharedMutex )
    {
        // Release our local reference
        m_pSharedMutex->releaseReference();
        m_pSharedMutex = NULL;
    }
}

UaNodeId ACTObject::typeDefinitionId() const
{
    UaNodeId ret(PAS_ACTType, browseName().namespaceIndex());
    return ret;
}

/*==========================================================================*/
UaStatus ACTObject::call(
    const ServiceContext&  serviceContext,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments,
    UaVariantArray&        outputArguments,
    UaStatusCodeArray&     inputArgumentResults,
    UaDiagnosticInfos&     inputArgumentDiag)
{
    UaStatus            ret;
    MethodHandleUaNode* pMethodHandleUaNode = static_cast<MethodHandleUaNode*>(pMethodHandle);
    UaMethod*           pMethod             = NULL;

    int numArgs;
    OpcUa_UInt32 methodTypeID;

    if(pMethodHandleUaNode)
    {
        pMethod = pMethodHandleUaNode->pUaMethod();

        if (m_MethodMap.find(pMethod->nodeId()) != m_MethodMap.end()) {
            methodTypeID = m_MethodMap[pMethod->nodeId()].second;
            numArgs = METHODS.at(methodTypeID).second.size();

            if (inputArguments.length() != numArgs) {
                ret = OpcUa_BadInvalidArgument;
                return ret;
            } else {
                for (size_t i = 0; i < numArgs; i++) {
                    // Type checking (currently not implemented)
                    //if ( inputArguments[i].Datatype !=  )
                    //{
                    //ret = OpcUa_BadInvalidArgument;
                    //inputArgumentResults[i] = OpcUa_BadTypeMismatch;
                    //}
                }
            }
            ret = m_pCommIf->OperateDevice(PAS_ACTType, m_Identity, methodTypeID, inputArguments);
        } else {
            ret = OpcUa_BadInvalidArgument;
        }
    } else {
        ret = OpcUa_BadInvalidArgument;
    }

    return ret;
}

// -------------------------------------------------------------------
// Specialization: PSDObject Implementation
PSDObject::PSDObject(
    const UaString& name,
    const UaNodeId& newNodeId,
    const UaString& defaultLocaleId,
    PasNodeManagerCommon *pNodeManager,
    Identity identity,
    PasComInterfaceCommon *pCommIf)
: PasObject(name, newNodeId, defaultLocaleId, pNodeManager, identity, pCommIf)
{
    // Use a mutex shared across all variables of this object
    m_pSharedMutex = new UaMutexRefCounted;

    UaStatus                     addStatus;
    // Method helper
    OpcUa_Int16                  nsIdx = pNodeManager->getNameSpaceIndex();
    OpcUa::DataItemType*         pDataItem            = NULL;

    /**************************************************************
     * Create the PSD components
     **************************************************************/
    // Add Variable "State"
    // Get the instance declaration node used as base for this variable instance
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_State, OpcUa_True);

    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_x1);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_y1);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_x2);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_y2);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_dx1);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_dy1);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_dx2);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_dy2);
    pDataItem = addVariable(pNodeManager, PAS_PSDType, PAS_PSDType_Temp);

    // Add Method "Update"
    UaString sName = "Update";
    UaString sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodUpdate = new UaMethodGeneric(sName, UaNodeId(sNodeId, nsIdx), m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodUpdate, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
}

PSDObject::~PSDObject(void)
{
    if ( m_pSharedMutex )
    {
        // Release our local reference
        m_pSharedMutex->releaseReference();
        m_pSharedMutex = NULL;
    }
}

UaNodeId PSDObject::typeDefinitionId() const
{
    UaNodeId ret(PAS_PSDType, browseName().namespaceIndex());
    return ret;
}

/*==========================================================================*/
UaStatus PSDObject::call(
    const ServiceContext&  serviceContext,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments,
    UaVariantArray&        outputArguments,
    UaStatusCodeArray&     inputArgumentResults,
    UaDiagnosticInfos&     inputArgumentDiag)
{
    UaStatus            ret;
    MethodHandleUaNode* pMethodHandleUaNode = static_cast<MethodHandleUaNode*>(pMethodHandle);
    UaMethod*           pMethod             = NULL;

    if(pMethodHandleUaNode)
    {
        pMethod = pMethodHandleUaNode->pUaMethod();

        if(pMethod)
        {
            // Check if we have the step() method
            if ( pMethod->nodeId() == m_pMethodUpdate->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                    ret = OpcUa_BadInvalidArgument;
                else
                    ret = m_pCommIf->OperateDevice(PAS_PSDType, m_Identity);
            }
        }
        else
        {
            assert(false);
            ret = OpcUa_BadInvalidArgument;
        }
    }
    else
    {
        assert(false);
        ret = OpcUa_BadInvalidArgument;
    }

    return ret;
}
