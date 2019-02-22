#include "pasnodemanager.h"
#include "passervertypeids.h"
#include "pasobject.h"
#include "panelobject.h"
#include "uaserver/opcua_analogitemtype.h"
#include "pascommunicationinterface.h"
#include "mpeseventdata.h"
#include "uaserver/opcua_offnormalalarmtype.h"

PasNodeManager::PasNodeManager()
: PasNodeManagerCommon()
{
    printf("\nCreated Node manager with NameSpaceIndex = %d\n", getNameSpaceIndex());
}

PasNodeManager::~PasNodeManager()
{
    m_pCommIf = nullptr;
}

void PasNodeManager::setCommunicationInterface(PasCommunicationInterface *pCommIf)
{
    printf("PasNodeManager: Setting communication interface\n");
    // explicit cast
    m_pCommIf = static_cast<PasComInterfaceCommon *>(pCommIf);
}

UaStatus PasNodeManager::afterStartUp()
{
    UaStatus ret;

    UA_ASSERT(ret.isGood());

    UaFolder      *pMPESFolder = NULL;
    UaFolder      *pACTFolder = NULL;
    MPESObject    *pMPES = NULL;
    ACTObject     *pACT  = NULL;
    PSDObject     *pPSD  = NULL;
    PanelObject   *pPanel  = NULL;
    UaString       sDeviceName;
    OpcUa_UInt32   i;
    Identity       identity;

    createTypeNodes();
    amendTypeNodes();

    /**************************************************************
     Create a folder for the sensor objects and add the folder to the ObjectsFolder
    ***************************************************************/
    pMPESFolder = new UaFolder("MPES", UaNodeId("MPES", getNameSpaceIndex()), m_defaultLocaleId);
    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pMPESFolder, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    /**************************************************************
     Create a folder for the actuator objects and add the folder to the ObjectsFolder
    ***************************************************************/
    pACTFolder = new UaFolder("ACT", UaNodeId("ACT", getNameSpaceIndex()), m_defaultLocaleId);
    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pACTFolder, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    /**************************************************************
     Create alarm area folders for the sensor objects and add them to the Server object
    ***************************************************************/
    UaAreaFolder* pAreaMPES = new UaAreaFolder(
        "Area", UaNodeId("AreaMPESEvents", getNameSpaceIndex()), m_defaultLocaleId);
    addNodeAndReference(OpcUaId_Server, pAreaMPES, OpcUaId_HasNotifier);
    // Register event notifier tree
    registerEventNotifier(OpcUaId_Server, pAreaMPES->nodeId());

    /**************************************************************
     * Create the Device Object Instances
     **************************************************************/
    OpcUa_UInt32 panelcount = m_pCommIf->getDevices(PAS_PanelType);
    OpcUa_UInt32 mpescount = m_pCommIf->getDevices(PAS_MPESType);
    OpcUa_UInt32 actcount = m_pCommIf->getDevices(PAS_ACTType);
    OpcUa_UInt32 psdcount = m_pCommIf->getDevices(PAS_PSDType);

    for ( i=0; i<panelcount; i++ )
    {
        ret = m_pCommIf->getDeviceConfig(PAS_PanelType, i, sDeviceName, identity);

        pPanel = new PanelObject(sDeviceName, UaNodeId(sDeviceName, getNameSpaceIndex()),
            m_defaultLocaleId, this, identity,
            static_cast<PasCommunicationInterface *>(m_pCommIf));
        ret = addNodeAndReference(OpcUaId_ObjectsFolder, pPanel, OpcUaId_Organizes);
        UA_ASSERT(ret.isGood());
        // Add HasTypeDefinition reference from object to PanelType
        ret = addUaReference(pPanel->nodeId(), pPanel->typeDefinitionId(), OpcUaId_HasTypeDefinition);
        UA_ASSERT(ret.isGood());
    }
    for ( i=0; i<mpescount; i++ )
    {
        ret = m_pCommIf->getDeviceConfig(PAS_MPESType, i, sDeviceName, identity);

        pMPES = new MPESObject(sDeviceName, UaNodeId(sDeviceName, getNameSpaceIndex()),
            m_defaultLocaleId, this, identity, m_pCommIf);
        ret = addNodeAndReference(pMPESFolder, pMPES, OpcUaId_Organizes);
        UA_ASSERT(ret.isGood());
        // Add HasNotifier reference from alarm area to controller object
        ret = addUaReference(pAreaMPES, pMPES, OpcUaId_HasNotifier);
        UA_ASSERT(ret.isGood());
        // Add HasTypeDefinition reference from object to MPESType
        ret = addUaReference(pMPES->nodeId(), pMPES->typeDefinitionId(), OpcUaId_HasTypeDefinition);
        UA_ASSERT(ret.isGood());
        // Register event notifier tree
        registerEventNotifier(pAreaMPES->nodeId(), pMPES->nodeId());
    }
    for ( i=0; i<actcount; i++ )
    {
        ret = m_pCommIf->getDeviceConfig(PAS_ACTType, i, sDeviceName, identity);

        pACT = new ACTObject(sDeviceName, UaNodeId(sDeviceName, getNameSpaceIndex()),
            m_defaultLocaleId, this, identity, m_pCommIf);
        ret = addNodeAndReference(pACTFolder, pACT, OpcUaId_Organizes);
        UA_ASSERT(ret.isGood());
        // Add HasTypeDefinition reference from object to ACTType 
        ret = addUaReference(pACT->nodeId(), pACT->typeDefinitionId(), OpcUaId_HasTypeDefinition);
        UA_ASSERT(ret.isGood());
    }
    for ( i=0; i<psdcount; i++ )
    {
        ret = m_pCommIf->getDeviceConfig(PAS_PSDType, i, sDeviceName, identity);

        pPSD = new PSDObject(sDeviceName, UaNodeId(sDeviceName, getNameSpaceIndex()),
            m_defaultLocaleId, this, identity,
            static_cast<PasCommunicationInterface *>(m_pCommIf));
        ret = addNodeAndReference(OpcUaId_ObjectsFolder, pPSD, OpcUaId_Organizes);
        UA_ASSERT(ret.isGood());
        ret = addUaReference(pPSD->nodeId(), pPSD->typeDefinitionId(), OpcUaId_HasTypeDefinition);
        UA_ASSERT(ret.isGood());
    }

    return ret;
}

UaStatus PasNodeManager::beforeShutDown()
{
    UaStatus ret;

    return ret;
}

UaStatus PasNodeManager::amendTypeNodes()
{
    UaStatus ret;
    UaStatus addStatus;

    UaVariant                    defaultValue;
    UaObjectTypeSimple*          pPanelType = NULL;
    OpcUa::DataItemType*         pDataItem;
    // Method helpers
    OpcUa::BaseMethod*           pMethod = NULL;

    /**************************************************************
     * Create the Panel Type
     **************************************************************/
    // Add ObjectType "PanelType"
    pPanelType = new UaObjectTypeSimple(
        "PanelType",    // Used as string in browse name and display name
        UaNodeId(PAS_PanelType, getNameSpaceIndex()), // Numeric NodeId for types
        m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
        OpcUa_True);         // Abstract object type -> can not be instantiated
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pPanelType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the Panel Type Instance declaration
     ***************************************************************/
    // Add Variable "State" as BaseDataVariable
    defaultValue.setUInt32(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(PAS_PanelType_State, getNameSpaceIndex()), // NodeId of the Variable
        "State",                // Name of the Variable
        getNameSpaceIndex(),    // Namespace index of the browse name (same like NodeId)
        defaultValue,           // Initial value
        Ua_AccessLevel_CurrentRead, // Access level
        this);                  // Node manager for this variable
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    defaultValue.setFloat(0.);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_ExtTemperature, getNameSpaceIndex()),
            "ExternalTemperature", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    defaultValue.setFloat(0.);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_IntTemperature, getNameSpaceIndex()),
            "InternalTemperature", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "StepAll"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_PanelType_StepAll, getNameSpaceIndex()), "StepAll", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "MoveTo"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_PanelType_MoveTo_Acts, getNameSpaceIndex()), "MoveTo", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_PanelType_Stop, getNameSpaceIndex()), "Stop", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    return ret;
}
