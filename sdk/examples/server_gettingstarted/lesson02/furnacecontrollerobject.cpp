#include "furnacecontrollerobject.h"
#include "nmbuildingautomation.h"
#include "buildingautomationtypeids.h"
#include "opcua_analogitemtype.h"

FurnaceControllerObject::FurnaceControllerObject(const UaString& name, const UaNodeId& newNodeId, const UaString& defaultLocaleId, NmBuildingAutomation* pNodeManager)
: ControllerObject(name, newNodeId, defaultLocaleId, pNodeManager)
{
    UaVariable*                  pInstanceDeclaration;
    OpcUa::DataItemType*         pDataItem;
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
}

FurnaceControllerObject::~FurnaceControllerObject(void)
{
}

UaNodeId FurnaceControllerObject::typeDefinitionId() const
{
    UaNodeId ret(Ba_FurnaceControllerType, browseName().namespaceIndex());
    return ret;
}

