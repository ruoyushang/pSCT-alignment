#include "airconditionercontrollerobject.h"
#include "nmbuildingautomation.h"
#include "buildingautomationtypeids.h"
#include "opcua_analogitemtype.h"

AirConditionerControllerObject::AirConditionerControllerObject(const UaString& name, const UaNodeId& newNodeId, const UaString& defaultLocaleId, NmBuildingAutomation* pNodeManager)
: ControllerObject(name, newNodeId, defaultLocaleId, pNodeManager)
{
    UaVariable*                  pInstanceDeclaration;
    OpcUa::AnalogItemType*       pAnalogItem;
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
}

AirConditionerControllerObject::~AirConditionerControllerObject(void)
{
}

UaNodeId AirConditionerControllerObject::typeDefinitionId() const
{
    UaNodeId ret(Ba_AirConditionerControllerType, browseName().namespaceIndex());
    return ret;
}

