#include "furnacecontrollerobject.h"
#include "nmbuildingautomation.h"
#include "buildingautomationtypeids.h"
#include "opcua_analogitemtype.h"

FurnaceControllerObject::FurnaceControllerObject(
    const UaString& name,
    const UaNodeId& newNodeId,
    const UaString& defaultLocaleId,
    NmBuildingAutomation* pNodeManager,
    OpcUa_UInt32 deviceAddress,
    BaCommunicationInterface *pCommIf)
: ControllerObject(name, newNodeId, defaultLocaleId, pNodeManager, deviceAddress, pCommIf)
{
    UaVariable*                  pInstanceDeclaration = NULL;
    OpcUa::DataItemType*         pDataItem            = NULL;
    BaUserData*                  pUserData            = NULL;
    UaStatus                     addStatus;

    /**************************************************************
     * Create the FurnaceController components
     **************************************************************/
    // Add Variable "GasFlow"
    // Get the instance declaration node used as base for this variable instance
    pInstanceDeclaration = pNodeManager->getInstanceDeclarationVariable(Ba_FurnaceControllerType_GasFlow);
    UA_ASSERT(pInstanceDeclaration!=NULL);
    pDataItem = new OpcUa::DataItemType(
        this,           // Parent node
        pInstanceDeclaration, // Instance declaration variable this variable instance is based on
        pNodeManager,   // Node manager responsible for this variable
        m_pSharedMutex);  // Shared mutex used across all variables of this object
    addStatus = pNodeManager->addNodeAndReference(this, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());
    // Store information needed to access device
    pUserData = new BaUserData(OpcUa_False, deviceAddress, 3);
    pDataItem->setUserData(pUserData);
    // Change value handling to get read and write calls to the node manager
    pDataItem->setValueHandling(UaVariable_Value_Cache);
}

FurnaceControllerObject::~FurnaceControllerObject(void)
{
}

UaNodeId FurnaceControllerObject::typeDefinitionId() const
{
    UaNodeId ret(Ba_FurnaceControllerType, browseName().namespaceIndex());
    return ret;
}

