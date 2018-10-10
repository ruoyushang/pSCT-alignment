#include "controllerobject.h"
#include "nmbuildingautomation.h"
#include "buildingautomationtypeids.h"
#include "opcua_analogitemtype.h"
#include "bacommunicationinterface.h"
#include "methodhandleuanode.h"
#include "baeventdata.h"

ControllerObject::ControllerObject(
    const UaString& name,
    const UaNodeId& newNodeId,
    const UaString& defaultLocaleId,
    NmBuildingAutomation* pNodeManager,
    OpcUa_UInt32 deviceAddress,
    BaCommunicationInterface *pCommIf)
: UaObjectBase(name, newNodeId, defaultLocaleId),
  m_pSharedMutex(NULL),
  m_deviceAddress(deviceAddress),
  m_pCommIf(pCommIf),
  m_pNodeManager(pNodeManager)
{
    // Use a mutex shared across all variables of this object
    m_pSharedMutex = new UaMutexRefCounted;

    UaVariable*                  pInstanceDeclaration = NULL;
    OpcUa::BaseDataVariableType* pDataVariable        = NULL;
    OpcUa::DataItemType*         pDataItem            = NULL;
    OpcUa::AnalogItemType*       pAnalogItem          = NULL;
    BaUserData*                  pUserData            = NULL;
    UaStatus                     addStatus;
    // Method helper
    OpcUa_Int16                  nsIdx = pNodeManager->getNameSpaceIndex();

    /**************************************************************
     * Create the Controller components
     **************************************************************/
    // Add Variable "State"
    // Get the instance declaration node used as base for this variable instance
    pInstanceDeclaration = pNodeManager->getInstanceDeclarationVariable(Ba_ControllerType_State);
    UA_ASSERT(pInstanceDeclaration!=NULL);
    pDataVariable = new OpcUa::BaseDataVariableType(
        this,           // Parent node
        pInstanceDeclaration, // Instance declaration variable this variable instance is based on
        pNodeManager,   // Node manager responsible for this variable
        m_pSharedMutex);  // Shared mutex used across all variables of this object
    addStatus = pNodeManager->addNodeAndReference(this, pDataVariable, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Store information needed to access device
    pUserData = new BaUserData(OpcUa_True, deviceAddress, 0);
    pDataVariable->setUserData(pUserData);
    // Change value handling to get read and write calls to the node manager
    pDataVariable->setValueHandling(UaVariable_Value_Cache);

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
    addStatus = pNodeManager->addUaReference(this, pDataVariable, OpcUaId_HasEventSource);
    UA_ASSERT(addStatus.isGood());
    // Create HasCondition reference from state variable to condition
    addStatus = pNodeManager->addUaReference(pDataVariable, m_pStateOffNormalAlarm, OpcUaId_HasCondition);
    UA_ASSERT(addStatus.isGood());
    m_pStateOffNormalAlarm->setConditionName("StateCondition");
    m_pStateOffNormalAlarm->setAckedState(OpcUa_True);
    m_pStateOffNormalAlarm->setEnabledState(OpcUa_True);

    // Add Variable "Temperature"
    pInstanceDeclaration = pNodeManager->getInstanceDeclarationVariable(Ba_ControllerType_Temperature);
    UA_ASSERT(pInstanceDeclaration!=NULL);
    // Create new variable and add it as component to this object
    pAnalogItem = new OpcUa::AnalogItemType(this, pInstanceDeclaration, pNodeManager, m_pSharedMutex);
    addStatus = pNodeManager->addNodeAndReference(this, pAnalogItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    UaRange tempRange(0, 100);
    pAnalogItem->setEURange(tempRange);
    UaEUInformation tempEUInformation("", -1, UaLocalizedText("en", "\xc2\xb0\x46") /* °F */, UaLocalizedText("en", "Degrees Fahrenheit"));
    pAnalogItem->setEngineeringUnits(tempEUInformation);
    // Store information needed to access device
    pUserData = new BaUserData(OpcUa_False, deviceAddress, 0);
    pAnalogItem->setUserData(pUserData);
    // Change value handling to get read and write calls to the node manager
    pAnalogItem->setValueHandling(UaVariable_Value_Cache);

    // Add Variable "TemperatureSetPoint"
    // Get the instance declaration node used as base for this variable instance
    pInstanceDeclaration = pNodeManager->getInstanceDeclarationVariable(Ba_ControllerType_TemperatureSetPoint);
    UA_ASSERT(pInstanceDeclaration!=NULL);
    // Create new variable and add it as component to this object
    pAnalogItem = new OpcUa::AnalogItemType(this, pInstanceDeclaration, pNodeManager, m_pSharedMutex);
    addStatus = pNodeManager->addNodeAndReference(this, pAnalogItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    pAnalogItem->setEURange(tempRange);
    pAnalogItem->setEngineeringUnits(tempEUInformation);
    // Store information needed to access device
    pUserData = new BaUserData(OpcUa_False, deviceAddress, 1);
    pAnalogItem->setUserData(pUserData);
    // Change value handling to get read and write calls to the node manager
    pAnalogItem->setValueHandling(UaVariable_Value_Cache);

    // Add Variable "PowerConsumption"
    // Get the instance declaration node used as base for this variable instance
    pInstanceDeclaration = pNodeManager->getInstanceDeclarationVariable(Ba_ControllerType_PowerConsumption);
    UA_ASSERT(pInstanceDeclaration!=NULL);
    // Create new variable and add it as component to this object
    pDataItem = new OpcUa::DataItemType(this, pInstanceDeclaration, pNodeManager, m_pSharedMutex);
    addStatus = pNodeManager->addNodeAndReference(this, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Store information needed to access device
    pUserData = new BaUserData(OpcUa_False, deviceAddress, 2);
    pDataItem->setUserData(pUserData);
    // Change value handling to get read and write calls to the node manager
    pDataItem->setValueHandling(UaVariable_Value_Cache);

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
}

ControllerObject::~ControllerObject(void)
{
    if ( m_pSharedMutex )
    {
        // Release our local reference
        m_pSharedMutex->releaseReference();
        m_pSharedMutex = NULL;
    }
}

OpcUa_Byte ControllerObject::eventNotifier() const
{
    return Ua_EventNotifier_SubscribeToEvents;
}

MethodManager* ControllerObject::getMethodManager(UaMethod* pMethod) const
{
    OpcUa_ReferenceParameter(pMethod);
    return (MethodManager*)this;
}

UaStatus ControllerObject::beginCall(
    MethodManagerCallback* pCallback,
    const ServiceContext&  serviceContext,
    OpcUa_UInt32           callbackHandle,
    MethodHandle*          pMethodHandle,
    const UaVariantArray&  inputArguments)
{
    OpcUa_ReferenceParameter(serviceContext);
    UaStatus            ret;
    UaVariantArray      outputArguments;
    UaStatusCodeArray   inputArgumentResults;
    UaDiagnosticInfos   inputArgumentDiag;
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
                {
                    ret = OpcUa_BadInvalidArgument;
                }
                else
                {
                    ret = m_pCommIf->setControllerState(
                        m_deviceAddress,
                        BaCommunicationInterface::Ba_ControllerState_On );
                }
            }
            // Check if we have the stop method
            else if ( pMethod->nodeId() == m_pMethodStop->nodeId())
            {
                // Number of input arguments must be 0
                if ( inputArguments.length() > 0 )
                {
                    ret = OpcUa_BadInvalidArgument;
                }
                else
                {
                    ret = m_pCommIf->setControllerState(
                        m_deviceAddress,
                        BaCommunicationInterface::Ba_ControllerState_Off );
                }
            }
            else
            {
                ret = call(pMethod, inputArguments, outputArguments, inputArgumentResults, inputArgumentDiag);
            }

            // Create Controller Events if state change succeeded
            if ( ret.isGood() )
            {
                //--------------------------------------------------------
                // Create event
                ControllerEventTypeData eventData(browseName().namespaceIndex());

                // Handle ControllerEventType specific fields
                // Temperature
                OpcUa_Double value;
                m_pCommIf->getControllerData(m_deviceAddress, 0, value);
                eventData.m_Temperature.setDouble(value);
                // State
                BaCommunicationInterface::ControllerState state;
                m_pCommIf->getControllerState(m_deviceAddress, state);
                eventData.m_State.setUInt32(state);

                // Fill all default event fields
                eventData.m_SourceNode.setNodeId(nodeId());
                eventData.m_SourceName.setString(browseName().toString());
                UaString messageText;
                if ( state == BaCommunicationInterface::Ba_ControllerState_Off )
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
                if ( state == BaCommunicationInterface::Ba_ControllerState_Off )
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

            pCallback->finishCall(
                callbackHandle,
                inputArgumentResults,
                inputArgumentDiag,
                outputArguments,
                ret);

            ret = OpcUa_Good;
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

