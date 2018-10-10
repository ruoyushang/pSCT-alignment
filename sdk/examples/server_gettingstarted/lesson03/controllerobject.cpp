#include "controllerobject.h"
#include "nmbuildingautomation.h"
#include "buildingautomationtypeids.h"
#include "opcua_analogitemtype.h"

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
  m_pCommIf(pCommIf)
{
    // Use a mutex shared across all variables of this object
    m_pSharedMutex = new UaMutexRefCounted;

    UaVariable*                  pInstanceDeclaration = NULL;
    OpcUa::BaseDataVariableType* pDataVariable        = NULL;
    OpcUa::DataItemType*         pDataItem            = NULL;
    OpcUa::AnalogItemType*       pAnalogItem          = NULL;
    BaUserData*                  pUserData            = NULL;
    UaStatus                     addStatus;

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
    return Ua_EventNotifier_None;
}
