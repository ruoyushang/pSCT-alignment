#include "airconditionercontrollerobject.h"
#include "nmbuildingautomation.h"
#include "buildingautomationtypeids.h"
#include "opcua_analogitemtype.h"
#include "bacommunicationinterface.h"

AirConditionerControllerObject::AirConditionerControllerObject(
    const UaString& name,
    const UaNodeId& newNodeId,
    const UaString& defaultLocaleId,
    NmBuildingAutomation* pNodeManager,
    OpcUa_UInt32 deviceAddress,
    BaCommunicationInterface *pCommIf)
    : ControllerObject(name, newNodeId, defaultLocaleId, pNodeManager, deviceAddress, pCommIf)
{
    UaVariable*                  pInstanceDeclaration = NULL;
    OpcUa::AnalogItemType*       pAnalogItem          = NULL;
    BaUserData*                  pUserData            = NULL;
    UaStatus                     addStatus;
    // Method helpers
    UaPropertyMethodArgument*    pPropertyArg = NULL;
    UaUInt32Array                nullarray;
    OpcUa_Int16                  nsIdx = pNodeManager->getNameSpaceIndex();

    /**************************************************************
     * Create the AirConditionerController components
     **************************************************************/
    // Add Variable "Humidity"
    pInstanceDeclaration = pNodeManager->getInstanceDeclarationVariable(Ba_AirConditionerControllerType_Humidity);
    UA_ASSERT(pInstanceDeclaration!=NULL);
    // Create new variable and add it as component to this object
    pAnalogItem = new OpcUa::AnalogItemType(
        this,           // Parent node
        pInstanceDeclaration, // Instance declaration variable this variable instance is based on
        pNodeManager,   // Node manager responsible for this variable
        m_pSharedMutex);  // Shared mutex used across all variables of this object
    addStatus = pNodeManager->addNodeAndReference(this, pAnalogItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    UaRange tempRange(0, 100);
    pAnalogItem->setEURange(tempRange);
    UaEUInformation tempEUInformation("", -1, UaLocalizedText("en", "%"), UaLocalizedText("en", "Percent"));
    pAnalogItem->setEngineeringUnits(tempEUInformation);
    // Store information needed to access device
    pUserData = new BaUserData(OpcUa_False, deviceAddress, 3);
    pAnalogItem->setUserData(pUserData);
    // Change value handling to get read and write calls to the node manager
    pAnalogItem->setValueHandling(UaVariable_Value_Cache);

    // Add Variable "HumiditySetpoint"
    // Get the instance declaration node used as base for this variable instance
    pInstanceDeclaration = pNodeManager->getInstanceDeclarationVariable(Ba_AirConditionerControllerType_HumiditySetpoint);
    UA_ASSERT(pInstanceDeclaration!=NULL);
    // Create new variable and add it as component to this object
    pAnalogItem = new OpcUa::AnalogItemType(this, pInstanceDeclaration, pNodeManager, m_pSharedMutex);
    addStatus = pNodeManager->addNodeAndReference(this, pAnalogItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Set property values - the property nodes are already created
    pAnalogItem->setEURange(tempRange);
    pAnalogItem->setEngineeringUnits(tempEUInformation);
    // Store information needed to access device
    pUserData = new BaUserData(OpcUa_False, deviceAddress, 4);
    pAnalogItem->setUserData(pUserData);
    // Change value handling to get read and write calls to the node manager
    pAnalogItem->setValueHandling(UaVariable_Value_Cache);

    // Add Method "StartWithSetpoint"
    UaString sName = "StartWithSetpoint";
    UaString sNodeId = UaString("%1.%2").arg(newNodeId.toString()).arg(sName);
    m_pMethodStartWithSetpoint = new UaMethodGeneric(
        sName,
        UaNodeId(sNodeId, nsIdx),
        m_defaultLocaleId);
    addStatus = pNodeManager->addNodeAndReference(this, m_pMethodStartWithSetpoint, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Add Property "InputArguments"
    sName = "StartWithSetpoint";
    sNodeId = UaString("%1.%2").arg(m_pMethodStartWithSetpoint->nodeId().toString()).arg(sName);
    pPropertyArg = new UaPropertyMethodArgument(
        UaNodeId(sNodeId, nsIdx),             // NodeId of the property
        Ua_AccessLevel_CurrentRead,             // Access level of the property
        2,                                      // Number of arguments
        UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument TemperatureSetPoint
    pPropertyArg->setArgument(
        0,                       // Index of the argument
        "TemperatureSetPoint",   // Name of the argument
        UaNodeId(OpcUaId_Double),// Data type of the argument
        OpcUa_ValueRanks_Scalar, // Array rank of the argument
        nullarray,               // Array dimensions of the argument
        UaLocalizedText("en", "Controller set point for temperature")); // Description
    // Argument HumiditySetpoint
    pPropertyArg->setArgument(
        1,                       // Index of the argument
        "HumiditySetpoint",      // Name of the argument
        UaNodeId(OpcUaId_Double),// Data type of the argument
        OpcUa_ValueRanks_Scalar, // Array rank of the argument
        nullarray,               // Array dimensions of the argument
        UaLocalizedText("en", "Controller set point for humidity")); // Description
    // Add property to method
    addStatus = pNodeManager->addNodeAndReference(m_pMethodStartWithSetpoint, pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());
}

AirConditionerControllerObject::~AirConditionerControllerObject(void)
{
}

UaNodeId AirConditionerControllerObject::typeDefinitionId() const
{
    UaNodeId ret(Ba_AirConditionerControllerType, browseName().namespaceIndex());
    return ret;
}

UaStatus AirConditionerControllerObject::call(
    UaMethod*             pMethod,
    const UaVariantArray& inputArguments,
    UaVariantArray&       /*outputArguments*/,
    UaStatusCodeArray&    inputArgumentResults,
    UaDiagnosticInfos&    /*inputArgumentDiag*/)
{
    UaStatus ret;

    if(pMethod)
    {
        // Check if we have the start method
        if ( pMethod->nodeId() == m_pMethodStartWithSetpoint->nodeId())
        {
            // Number of input arguments must be 2
            if ( inputArguments.length() != 2 )
            {
                ret = OpcUa_BadInvalidArgument;
            }
            else
            {
                inputArgumentResults.create(2);
                // validate input parameters
                if ( inputArguments[0].Datatype != OpcUaType_Double )
                {
                    ret = OpcUa_BadInvalidArgument;
                    inputArgumentResults[0] = OpcUa_BadTypeMismatch;
                }
                if ( inputArguments[1].Datatype != OpcUaType_Double )
                {
                    ret = OpcUa_BadInvalidArgument;
                    inputArgumentResults[1] = OpcUa_BadTypeMismatch;
                }
                if (ret.isGood())
                {
                    UaVariant    vTemp;
                    OpcUa_Double value;

                    ret = m_pCommIf->setControllerState(
                        m_deviceAddress,
                        BaCommunicationInterface::Ba_ControllerState_On );
                    if ( ret.isGood() )
                    {
                        // Setting TemperatureSetPoint
                        vTemp = inputArguments[0];
                        vTemp.toDouble(value);
                        ret = m_pCommIf->setControllerData(
                            m_deviceAddress,
                            1,
                            value);
                        // Setting HumiditySetPoint
                        vTemp = inputArguments[1];
                        vTemp.toDouble(value);
                        ret = m_pCommIf->setControllerData(
                            m_deviceAddress,
                            4,
                            value);
                    }
                }
            }
        }
        else
        {
            ret = OpcUa_BadMethodInvalid;
        }
    }
    return ret;
}

