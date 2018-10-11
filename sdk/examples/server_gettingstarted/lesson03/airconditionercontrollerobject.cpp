#include "airconditionercontrollerobject.h"
#include "nmbuildingautomation.h"
#include "buildingautomationtypeids.h"
#include "opcua_analogitemtype.h"

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
}

AirConditionerControllerObject::~AirConditionerControllerObject(void)
{
}

UaNodeId AirConditionerControllerObject::typeDefinitionId() const
{
    UaNodeId ret(Ba_AirConditionerControllerType, browseName().namespaceIndex());
    return ret;
}

