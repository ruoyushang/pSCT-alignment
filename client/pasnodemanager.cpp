#include "pascontroller.h"
#include "pasnodemanager.h"
#include "pasobject.h"
#include "mirrorobject.h"
#include "panelobject.h"
#include "edgeobject.h"
#include "gasobject.h"
#include "positioner.h"
#include "clienthelper.h"
#include "configuration.h"
#include "pascommunicationinterface.h"
#include "passervertypeids.h"
#include "components.h"
#include <iostream>
#include "uaserver/opcua_analogitemtype.h"

PasNodeManager::PasNodeManager()
: PasNodeManagerCommon()
{
    std::cout << "\nCreating new Node Manager\n";
    m_pPositioner = new Client(this);
}

PasNodeManager::~PasNodeManager()
{
    while (!m_pClient.empty())
    {
        delete m_pClient.back();
        m_pClient.pop_back();
    }
    delete m_pPositioner;
    m_pPositioner = nullptr;
    std::cout << "\nDeleted Clients\n";
}

void PasNodeManager::setCommunicationInterface(PasCommunicationInterface *pCommIf)
{
    std::cout << "PasNodeManager: Setting communication interface\n";
    // explicit cast
    m_pCommIf = static_cast<PasComInterfaceCommon *>(pCommIf);
}

void PasNodeManager::setConfiguration(Configuration *pConfiguration)
{
    std::cout << "PasNodeManager: Setting configuration\n";
    m_pConfiguration = pConfiguration;

    m_pPositioner->setConfiguration(m_pConfiguration);

    std::cout << "Will attempt to create " << m_pConfiguration->getServers() << " clients\n\n";
    for (OpcUa_UInt32 i = 0; i < m_pConfiguration->getServers(); i++)
    {
        m_pClient.push_back(new Client(this));
        m_pClient.back()->setConfiguration(m_pConfiguration);
    }
}

UaStatus PasNodeManager::afterStartUp()
{
    UaStatus ret;
    std::cout << "\nPasNodeManager:afterStartUp()" << std::endl;

    ret = createTypeNodes();
    UA_ASSERT(ret.isGood());
    ret = amendTypeNodes();
    UA_ASSERT(ret.isGood());

    // connect to positioner:
    Identity id;
    UaString positioner_address = m_pConfiguration->getPositionerUrl();
    m_pPositioner->setAddress(positioner_address);
    ret = m_pPositioner->connect();
    if (ret.isGood()) {
        id.eAddress = positioner_address.toUtf8();
        id.name = "Positioner";
        // add the positioner to the comm interface
        dynamic_cast<PasCommunicationInterface *>(m_pCommIf)->addDevice(m_pPositioner, GLOB_PositionerType, id);
    }
    else {
        std::cout << "Failed to connect to " << m_pConfiguration->getPositionerUrl().toUtf8()
            << ". Moving on..." << std::endl;
    }

    // connecting each client to its server -- loop through all panels!
    // this will add each panel to the communication interface; it will also browse the panels
    // and add the devices on them, such as MPES/ACT/PSD;
    // in the process, it will construct the edges out of the corresponding panels and sensors,
    // as well as the whole mirror(s). WOW 
    // number of clients is the same as the number of servers/panels by our set up
    unsigned client = 0;
    for (const auto& panelId : m_pConfiguration->getDeviceList(PAS_PanelType)) {
        // set the address of the panel as the client helper address and connect to the server
        m_pClient.at(client)->setAddress(UaString(panelId.eAddress.c_str()));
        ret = m_pClient.at(client)->connect();
        if (ret.isGood()) {
            dynamic_cast<PasCommunicationInterface*>(m_pCommIf)->addDevice(m_pClient.at(client), PAS_PanelType, panelId);
            // add the nodes that are the result of browsing
            ret = m_pClient.at(client)->browseAndAddDevices();
        }
        else
            std::cout << "Failed to connect to " << panelId.eAddress << ". Moving on..." << std::endl;

        ++client;
    }

    UaFolder * pFolder = NULL;
    PasObject *pObject = NULL;
    PasController *pController = NULL;
    std::vector<PasController *>pChildren;

    std::map<unsigned, UaFolder *> pDeviceFolders;
    std::map<PasController *, PasObject *> pDeviceObjects;

    std::string deviceName;
    std::string folderName;
    unsigned deviceType;

    OpcUa_UInt32 count;
    Identity identity;
    UaString sDeviceName;

    // Locate Positioner device
    OpcUa_UInt32 posCount = m_pCommIf->getDevices(GLOB_PositionerType);
    if (posCount > 1){
        std::cout << "\n +++ WARNING +++ PasNodeManager: More than one positioner added??\n" << std::endl;
    }
    else if (posCount < 1) {
        std::cout << "\n +++ WARNING +++ PasNodeManager: Less than one positioner added??\n" << std::endl;
    }
    ret = m_pCommIf->getDeviceConfig(GLOB_PositionerType, 0, sDeviceName, identity);

    if (ret.isGood()) {
        //Create a folder for the positioner and add the folder to the ObjectsFolder
        PositionerObject *pPositioner = new PositionerObject(sDeviceName,
                UaNodeId(sDeviceName, getNameSpaceIndex()), m_defaultLocaleId, this, identity,
                dynamic_cast<PasCommunicationInterface*>(m_pCommIf));
        ret = addNodeAndReference(OpcUaId_ObjectsFolder, pPositioner, OpcUaId_Organizes);
        UA_ASSERT(ret.isGood());
        ret = addUaReference(pPositioner->nodeId(), pPositioner->typeDefinitionId(), OpcUaId_HasTypeDefinition);
        UA_ASSERT(ret.isGood());
    }

    // First create all nodes and add object type references
    // Also add to device folder
    for (auto it=PasCommunicationInterface::deviceTypeNames.begin(); it!=PasCommunicationInterface::deviceTypeNames.end(); ++it) {
        deviceType = it->first;
        count = dynamic_cast<PasCommunicationInterface *>(m_pCommIf)->getDevices(deviceType);

        for (unsigned i = 0; i < count; i++)
        {
            ret = m_pCommIf->getDeviceConfig(deviceType, i, sDeviceName, identity);
            pController = dynamic_cast<PasCommunicationInterface *>(m_pCommIf)->getDeviceFromId(deviceType, identity);
            //If folder doesn't already exist, create a folder for each object type and add the folder to the ObjectsFolder
            if ( pDeviceFolders.find(deviceType) == pDeviceFolders.end() ) {
                deviceName = PasCommunicationInterface::deviceTypeNames[deviceType];
                folderName = deviceName + "Folder";
                pDeviceFolders[deviceType] = new UaFolder(UaString(folderName.c_str()), UaNodeId(UaString(folderName.c_str()), getNameSpaceIndex()), m_defaultLocaleId);
                ret = addNodeAndReference(OpcUaId_ObjectsFolder, pDeviceFolders[deviceType], OpcUaId_Organizes);
            }

            // Create object
            pObject = PasObject::makeObject(deviceType, sDeviceName, UaNodeId(sDeviceName, getNameSpaceIndex()),
                    m_defaultLocaleId, this, identity,
                    dynamic_cast<PasCommunicationInterface*>(m_pCommIf));

            // Create node
            ret = addUaNode(pObject);
            UA_ASSERT(ret.isGood());
            // Add object type reference
            ret = addUaReference(pObject->nodeId(), pObject->typeDefinitionId(), OpcUaId_HasTypeDefinition);
            UA_ASSERT(ret.isGood());

            // Add OpcUaId_HasComponent reference from the object type folder to
            /// the object.
            ret = addUaReference(pDeviceFolders[deviceType]->nodeId(), pObject->nodeId(), OpcUaId_HasComponent);
            UA_ASSERT(ret.isGood());

            // Add pointer to new object to pDeviceObjects map.
            pDeviceObjects[pController] = pObject;
        }
    }

    // Loop through all created objects and add children
    for (std::map<PasController *, PasObject *>::iterator it=pDeviceObjects.begin(); it!=pDeviceObjects.end(); ++it) {
        pController = it->first;
        pObject = it->second;

        // Check if object has children (is a composite controller)
        if (dynamic_cast<PasCompositeController*>(pController)) {
            for (auto it=PasCommunicationInterface::deviceTypeNames.begin();
            it!=PasCommunicationInterface::deviceTypeNames.end(); ++it) {
                deviceType = it->first;
                deviceName = it->second;
                pChildren = dynamic_cast<PasCompositeController*>(pController)->getChildren(deviceType);
                if (!pChildren.empty()) {
                    pFolder = new UaFolder(UaString(deviceName.c_str()), UaNodeId(UaString(deviceName.c_str()), getNameSpaceIndex()), m_defaultLocaleId);
                    ret = addNodeAndReference(pObject->nodeId(), pFolder, OpcUaId_HasComponent);
                    UA_ASSERT(ret.isGood());
                    for ( auto &child : pChildren) {
                        ret = addUaReference(pFolder->nodeId(), pDeviceObjects[child]->nodeId(), OpcUaId_HasComponent);
                        UA_ASSERT(ret.isGood());
                    }
                }
            }
        }
    }
    return ret;
}

UaStatus PasNodeManager::beforeShutDown()
{
    UaStatus ret;

    ret = m_pPositioner->disconnect();
    if (!ret.isGood())
        return ret;

    for (OpcUa_UInt32 i = 0; i < m_pClient.size(); i++)
    {
        ret = m_pClient.at(i)->disconnect();
        if (!ret.isGood())
            break;
    }

    return ret;
}

UaStatus PasNodeManager::amendTypeNodes()
{
    UaStatus ret;
    UaStatus addStatus;

    UaVariant                    defaultValue;
    UaObjectTypeSimple*          pMirrorType = NULL;
    UaObjectTypeSimple*          pEdgeType = NULL;
    UaObjectTypeSimple*          pPanelType = NULL;
    UaObjectTypeSimple*          pOptTableType = NULL;
    UaObjectTypeSimple*          pCCDType = NULL;
    OpcUa::DataItemType*         pDataItem;
    // Method helpers
    OpcUa::BaseMethod*           pMethod = NULL;
    UaPropertyMethodArgument*    pPropertyArg = NULL;
    UaUInt32Array                nullarray;

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

    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_curCoords_x, getNameSpaceIndex()),
            "curCoords_x", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_curCoords_y, getNameSpaceIndex()),
            "curCoords_y", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_curCoords_z, getNameSpaceIndex()),
            "curCoords_z", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_curCoords_xRot, getNameSpaceIndex()),
            "curCoords_xRot", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_curCoords_yRot, getNameSpaceIndex()),
            "curCoords_yRot", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_curCoords_zRot, getNameSpaceIndex()),
            "curCoords_zRot", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());


    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_inCoords_x, getNameSpaceIndex()),
            "inCoords_x", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_inCoords_y, getNameSpaceIndex()),
            "inCoords_y", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_inCoords_z, getNameSpaceIndex()),
            "inCoords_z", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_inCoords_xRot, getNameSpaceIndex()),
            "inCoords_xRot", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_inCoords_yRot, getNameSpaceIndex()),
            "inCoords_yRot", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_inCoords_zRot, getNameSpaceIndex()),
            "inCoords_zRot", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    defaultValue.setFloat(0.);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_IntTemperature, getNameSpaceIndex()),
            "InternalTemperature", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_ExtTemperature, getNameSpaceIndex()),
            "ExternalTemperature", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());


    // Add Method "MoveToActs"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_PanelType_MoveTo_Acts, getNameSpaceIndex()), "MoveToActs", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "MoveToCoords"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_PanelType_MoveTo_Coords, getNameSpaceIndex()), "MoveToCoords", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Read"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_PanelType_Read, getNameSpaceIndex()), "Read", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_PanelType_Stop, getNameSpaceIndex()), "Stop", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    /**************************************************************
     * Create the Edge Type
     **************************************************************/
    // Add ObjectType "EdgeType"
    pEdgeType = new UaObjectTypeSimple(
        "EdgeType",    // Used as string in browse name and display name
        UaNodeId(PAS_EdgeType, getNameSpaceIndex()), // Numeric NodeId for types
        m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
        OpcUa_True);         // Abstract object type -> can not be instantiated
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pEdgeType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the Edge Type Instance declaration
     ***************************************************************/
    // Add Variable "State" as BaseDataVariable
    defaultValue.setUInt32(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(PAS_EdgeType_State, getNameSpaceIndex()), // NodeId of the Variable
        "State",                // Name of the Variable
        getNameSpaceIndex(),    // Namespace index of the browse name (same like NodeId)
        defaultValue,           // Initial value
        Ua_AccessLevel_CurrentRead, // Access level
        this);                  // Node manager for this variable
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pEdgeType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "StepSize" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_EdgeType_StepSize, getNameSpaceIndex()), "StepSize",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pEdgeType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_EdgeType_Stop, getNameSpaceIndex()), "Stop", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pEdgeType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Move"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_EdgeType_Move, getNameSpaceIndex()), "Move", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pEdgeType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "FindMatrix"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_EdgeType_FindMatrix, getNameSpaceIndex()), "FindMatrix", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pEdgeType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Read"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_EdgeType_Read, getNameSpaceIndex()), "Read", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pEdgeType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Align"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_EdgeType_Align, getNameSpaceIndex()), "Align", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pEdgeType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Property "InputArguments" -- takes two arguments, the panel to move and the panel
    // keep fixed during alignment
    pPropertyArg = new UaPropertyMethodArgument(
            UaNodeId(PAS_EdgeType_Align_InPanel, getNameSpaceIndex()), // NodeId of the property
            Ua_AccessLevel_CurrentRead,             // Access level of the property
            2,                                      // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument PanelPosition
    pPropertyArg->setArgument(
            0,                       // Index of the argument
            "movePanel",               // Name of the argument
            UaNodeId(OpcUaId_UInt32),// Data type of the argument
            -1,                      // Array rank of the argument
            nullarray,               // Array dimensions of the argument
            UaLocalizedText("en", "Position of the panel to move during alignment")); // Description
    pPropertyArg->setArgument(
            1,                       // Index of the argument
            "fixPanel",               // Name of the argument
            UaNodeId(OpcUaId_UInt32),// Data type of the argument
            -1,                      // Array rank of the argument
            nullarray,               // Array dimensions of the argument
            UaLocalizedText("en", "Position of the panel to keep fixed during alignment")); // Description

    // Add property to method
    addStatus = addNodeAndReference(pMethod, pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());

    /**************************************************************
     * Create the Mirror Type
     **************************************************************/
    // Add ObjectType "MirrorType"
    pMirrorType = new UaObjectTypeSimple(
        "MirrorType",    // Used as string in browse name and display name
        UaNodeId(PAS_MirrorType, getNameSpaceIndex()), // Numeric NodeId for types
        m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
        OpcUa_True);         // Abstract object type -> can not be instantiated
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pMirrorType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());


    /***************************************************************
     * Create the Panel Type Instance declaration
     ***************************************************************/
    // Add Variable "State" as BaseDataVariable
    defaultValue.setUInt32(0);
    pDataItem = new OpcUa::DataItemType(
        UaNodeId(PAS_MirrorType_State, getNameSpaceIndex()), // NodeId of the Variable
        "State",                // Name of the Variable
        getNameSpaceIndex(),    // Namespace index of the browse name (same like NodeId)
        defaultValue,           // Initial value
        Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, // Access level
        this);                  // Node manager for this variable
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_curCoords_x, getNameSpaceIndex()),
            "curCoords_x", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_curCoords_y, getNameSpaceIndex()),
            "curCoords_y", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_curCoords_z, getNameSpaceIndex()),
            "curCoords_z", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_curCoords_xRot, getNameSpaceIndex()),
            "curCoords_xRot", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_curCoords_yRot, getNameSpaceIndex()),
            "curCoords_yRot", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_curCoords_zRot, getNameSpaceIndex()),
            "curCoords_zRot", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_inCoords_x, getNameSpaceIndex()),
            "inCoords_x", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_inCoords_y, getNameSpaceIndex()),
            "inCoords_y", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_inCoords_z, getNameSpaceIndex()),
            "inCoords_z", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_inCoords_xRot, getNameSpaceIndex()),
            "inCoords_xRot", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_inCoords_yRot, getNameSpaceIndex()),
            "inCoords_yRot", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_inCoords_zRot, getNameSpaceIndex()),
            "inCoords_zRot", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // add the vars to store selected panels and edges. These are writeable STRINGS!
    defaultValue.setString("");
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_selectedPanels, getNameSpaceIndex()),
            "selectedPanels", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_selectedEdges, getNameSpaceIndex()),
            "selectedEdges", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_sysOffsets_x, getNameSpaceIndex()),
            "sysOffsets_x", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_sysOffsets_y, getNameSpaceIndex()),
            "sysOffsets_y", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_sysOffsets_z, getNameSpaceIndex()),
            "sysOffsets_z", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_sysOffsets_xRot, getNameSpaceIndex()),
            "sysOffsets_xRot", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_sysOffsets_yRot, getNameSpaceIndex()),
            "sysOffsets_yRot", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_sysOffsets_zRot, getNameSpaceIndex()),
            "sysOffsets_zRot", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Move"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_MirrorType_MoveTo_Coords, getNameSpaceIndex()), "Move", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "ReadPosition"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_MirrorType_ReadPos, getNameSpaceIndex()), "ReadPosition", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Align"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_MirrorType_Align, getNameSpaceIndex()), "Align", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Property "InputArguments" -- takes two arguments, the edge to start at and the
    // direction to move in
    pPropertyArg = new UaPropertyMethodArgument(
            UaNodeId(PAS_MirrorType_Align_InEdge, getNameSpaceIndex()), // NodeId of the property
            Ua_AccessLevel_CurrentRead,             // Access level of the property
            2,                                      // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
    // Argument PanelPosition
    pPropertyArg->setArgument(
            0,                       // Index of the argument
            "startEdge",             // Name of the argument
            UaNodeId(OpcUaId_String),      // Data type of the argument
            -1,                      // Array rank of the argument
            nullarray,               // Array dimensions of the argument
            UaLocalizedText("en", "The edge from which to start sequential alignment")); // Description
    pPropertyArg->setArgument(
            1,                       // Index of the argument
            "startEdge",               // Name of the argument
            UaNodeId(OpcUaId_UInt32),// Data type of the argument
            -1,                      // Array rank of the argument
            nullarray,               // Array dimensions of the argument
            UaLocalizedText("en", "Direction in which to move: 0 for +ZRot, 1 for -ZRot")); // Description

    // Add property to method
    addStatus = addNodeAndReference(pMethod, pPropertyArg, OpcUaId_HasProperty);
    UA_ASSERT(addStatus.isGood());


    // Add Method "ReadAlignment"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_MirrorType_ReadAlign, getNameSpaceIndex()), "ReadAlignment", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_MirrorType_Stop, getNameSpaceIndex()), "Stop", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    /**************************************************************
     * Create the Optical Table Type
     **************************************************************/
    // Add ObjectType "OptTableType"
    pOptTableType = new UaObjectTypeSimple("OptTableType", UaNodeId(PAS_OptTableType, getNameSpaceIndex()), m_defaultLocaleId, OpcUa_True);
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pOptTableType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the Optical Table Type Instance declaration
     ***************************************************************/
    // Add Variable "State" as BaseDataVariable
    defaultValue.setUInt32(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_OptTableType_State, getNameSpaceIndex()), "State",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pOptTableType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "StepSize" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_EdgeType_StepSize, getNameSpaceIndex()), "StepSize",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pOptTableType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "FindMatrix"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_OptTableType_FindMatrix, getNameSpaceIndex()), "FindMatrix", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pOptTableType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Align"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_OptTableType_Align, getNameSpaceIndex()), "Align", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pOptTableType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "StepAll"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_OptTableType_StepAll, getNameSpaceIndex()), "StepAll", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pOptTableType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    /**************************************************************
     * Create the CCD Type
     **************************************************************/
    // Add ObjectType "CCDType"
    pCCDType = new UaObjectTypeSimple("CCDType", UaNodeId(PAS_CCDType, getNameSpaceIndex()), m_defaultLocaleId, OpcUa_True);
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pCCDType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
    * Create the CCD Type Instance declaration
    ***************************************************************/
    // Add Variable "State" as BaseDataVariable
    defaultValue.setUInt32(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_CCDType_State, getNameSpaceIndex()), "State",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "xFromLED" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_CCDType_xFromLED, getNameSpaceIndex()), "xFromLED",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "yFromLED" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_CCDType_yFromLED, getNameSpaceIndex()), "yFromLED",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "zFromLED" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_CCDType_zFromLED, getNameSpaceIndex()), "zFromLED",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "psiFromLED" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_CCDType_psiFromLED, getNameSpaceIndex()), "psiFromLED",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "thetaFromLED" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_CCDType_thetaFromLED, getNameSpaceIndex()), "thetaFromLED",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "phiFromLED" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_CCDType_phiFromLED, getNameSpaceIndex()), "phiFromLED",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "xNominal" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_CCDType_xNominal, getNameSpaceIndex()), "xNominal",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "yNominal" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_CCDType_yNominal, getNameSpaceIndex()), "yNominal",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "zNominal" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_CCDType_zNominal, getNameSpaceIndex()), "zNominal",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "psiNominal" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_CCDType_psiNominal, getNameSpaceIndex()), "psiNominal",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "thetaNominal" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_CCDType_thetaNominal, getNameSpaceIndex()), "thetaNominal",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "phiNominal" as DataItem
    defaultValue.setDouble(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_CCDType_phiNominal, getNameSpaceIndex()), "phiNominal",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());


    // Add Method "Start"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_CCDType_Start, getNameSpaceIndex()), "Start", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_CCDType_Stop, getNameSpaceIndex()), "Stop", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Read"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_CCDType_Read, getNameSpaceIndex()), "Read", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pCCDType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());


    /**************************************************************
     * Create the Positioner Type
     **************************************************************/
    // Add ObjectType "PositionerType"
    UaObjectTypeSimple* pPositionerType = new UaObjectTypeSimple("PositionerType", UaNodeId(GLOB_PositionerType, getNameSpaceIndex()), m_defaultLocaleId, OpcUa_True);
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pPositionerType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
    * Create the Positioner Type Instance declaration
    ***************************************************************/
    // Add Variable "isMoving" as BaseDataVariable
    defaultValue.setBool(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(GLOB_PositionerType_isMoving, getNameSpaceIndex()), "isMoving",
    getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPositionerType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "curAz" as BaseDataVariable
    defaultValue.setFloat(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(GLOB_PositionerType_curAz, getNameSpaceIndex()), "curAz",
    getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPositionerType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "inAz" as DataItem
    pDataItem = new OpcUa::DataItemType(UaNodeId(GLOB_PositionerType_inAz, getNameSpaceIndex()), "inAz",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPositionerType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "curEl" as BaseDataVariable
    pDataItem = new OpcUa::DataItemType(UaNodeId(GLOB_PositionerType_curEl, getNameSpaceIndex()), "curEl",
    getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPositionerType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "inAz" as DataItem
    pDataItem = new OpcUa::DataItemType(UaNodeId(GLOB_PositionerType_inEl, getNameSpaceIndex()), "inEl",
        getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPositionerType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    defaultValue.setInt16(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(GLOB_PositionerType_EnergyLevel, getNameSpaceIndex()), "EnergyLevel",
    getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPositionerType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "ToggleEnergyLevel"
    pMethod = new OpcUa::BaseMethod(UaNodeId(GLOB_PositionerType_SetEnergy, getNameSpaceIndex()), "ToggleEnergyLevel", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPositionerType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Init"
    pMethod = new OpcUa::BaseMethod(UaNodeId(GLOB_PositionerType_Init, getNameSpaceIndex()), "InitializeDrives", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPositionerType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "MoveTo"
    pMethod = new OpcUa::BaseMethod(UaNodeId(GLOB_PositionerType_Move, getNameSpaceIndex()), "MoveTo", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPositionerType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Stop"
    pMethod = new OpcUa::BaseMethod(UaNodeId(GLOB_PositionerType_Stop, getNameSpaceIndex()), "Stop", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPositionerType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    return ret;
}

OpcUa_Int32 PasNodeManager::Panic()
{
    UaStatus status;
    
    OpcUa_Int32 actcount = dynamic_cast<PasCommunicationInterface*>(m_pCommIf)->getDevices(PAS_ACTType);

    Identity id;
    for (OpcUa_Int32 i = 0; i < actcount; i++)
    {
        status = dynamic_cast<PasCommunicationInterface*>(m_pCommIf)->getDeviceConfig(PAS_ACTType, i, id);
        if (status.isGood())
        {
            printf("Will try changing state for %s\n", id.eAddress.c_str());
            status = m_pCommIf->setDeviceState(PAS_ACTType, id, PASState::PAS_Off );
        }
        else 
            printf("Problem changing state for %s\n", id.eAddress.c_str());
    }

    sleep(3);
    for (OpcUa_Int32 i = 0; i < actcount; i++)
    {
        status = dynamic_cast<PasCommunicationInterface*>(m_pCommIf)->getDeviceConfig(PAS_ACTType, i, id);
        if (status.isGood())
        {
            printf("Will try changing state for %s again\n", id.eAddress.c_str());
            status = m_pCommIf->setDeviceState(PAS_ACTType, id, PASState::PAS_On );
        }
        else 
            printf("Problem changing state for %s\n", id.eAddress.c_str());
    }

    if (status.isGood())
        return 0;

    return -1;
}
