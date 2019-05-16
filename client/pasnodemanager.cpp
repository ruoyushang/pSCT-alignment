#include "client/controllers/pascontroller.hpp"
#include "pasnodemanager.hpp"
#include "pasobject.hpp"
#include "client/objects/mirrorobject.hpp"
#include "client/objects/panelobject.hpp"
#include "client/objects/edgeobject.hpp"
#include "client/objects/opttableobject.hpp"
#include "client/objects/positionerobject.hpp"
#include "clienthelper.hpp"
#include "client/utilities/configuration.hpp"
#include "pascommunicationinterface.hpp"
#include "pasobjectfactory.hpp"
#include "passervertypeids.hpp"
#include "common/alignment/device.hpp"
#include <iostream>
#include <memory>
#include "uaserver/opcua_analogitemtype.h"

PasNodeManager::PasNodeManager()
: PasNodeManagerCommon()
{
    std::cout << "\nCreating new Node Manager\n";
    m_pPositioner = std::unique_ptr<Client>(new Client(this));
}

PasNodeManager::~PasNodeManager()
{
    std::cout << "Closed and cleaned up PasNodeManager\n";
}

void PasNodeManager::setCommunicationInterface(std::unique_ptr<PasCommunicationInterface> &pCommIf)
{
    std::cout << "PasNodeManager: Setting communication interface\n";
    m_pCommIf = std::unique_ptr<PasComInterfaceCommon>(pCommIf.release());
}

void PasNodeManager::setConfiguration(std::shared_ptr<Configuration> pConfiguration)
{
    std::cout << "PasNodeManager: Setting configuration\n";
    m_pConfiguration = std::move(pConfiguration);

    m_pPositioner->setConfiguration(m_pConfiguration);

    std::cout << "Will attempt to create " << m_pConfiguration->getServers() << " clients\n\n";
    for (OpcUa_UInt32 i = 0; i < m_pConfiguration->getServers(); i++)
    {
        m_pClients.push_back(std::unique_ptr<Client>(new Client(this)));
        m_pClients.back()->setConfiguration(m_pConfiguration);
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
    std::cout << "Attempting to create controller for positioner...\n";
    Device::Device::Identity id;
    UaString positioner_address = m_pConfiguration->getPositionerUrl();
    m_pPositioner->setAddress(positioner_address);
    ret = m_pPositioner->connect();
    if (ret.isGood()) {
        id.eAddress = positioner_address.toUtf8();
        id.name = "Positioner";
        // add the positioner to the comm interface
        dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->addDevice(m_pPositioner, GLOB_PositionerType, id);
        std::cout << "Found positioner and added corresponding controller.\n";
    }
    else {
        std::cout << "Failed to connect to positioner at " << m_pConfiguration->getPositionerUrl().toUtf8()
                  << ". Moving on..." << std::endl;
    }

    // connecting each client to its server -- loop through all panels!
    // this will add each panel to the communication interface; it will also browse the panels
    // and add the devices on them, such as MPES/ACT/PSD;
    // in the process, it will construct the edges out of the corresponding panels and sensors,
    // as well as the whole mirror(s). WOW
    // number of clients is the same as the number of servers/panels by our set up
    std::cout << "Looping through all panels, connecting to servers...\n";
    unsigned client = 0;
    for (const auto& panelId : m_pConfiguration->getDeviceList(PAS_PanelType)) {
        // set the address of the panel as the client helper address and connect to the server
        m_pClients.at(client)->setAddress(UaString(panelId.eAddress.c_str()));
        ret = m_pClients.at(client)->connect();
        if (ret.isGood()) {
            dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->addDevice(m_pClients.at(client), PAS_PanelType,
                                                                                  panelId);
            // add the nodes that are the result of browsing
            ret = m_pClients.at(client)->browseAndAddDevices();
            std::cout << "Successfully connected to Panel " << panelId << " and created controller.\n";
        }
        else
            std::cout << "Failed to connect to Panel " << panelId << ". Moving on..." << std::endl;

        ++client;
    }

    std::cout << "Done creating device controllers.\n";

    std::cout << "Now creating all OPC UA objects and folders...\n";

    UaFolder *pFolder = nullptr;
    PasObject *pObject = nullptr;
    std::shared_ptr<PasController> pController = nullptr;
    std::vector<std::shared_ptr<PasController>> pChildren;

    std::map<unsigned, UaFolder *> pDeviceFolders;
    std::map<std::shared_ptr<PasController>, PasObject *> pDeviceObjects;

    std::string deviceName;
    std::string folderName;
    unsigned deviceType;

    OpcUa_UInt32 count;
    Device::Device::Identity identity;
    UaString sDeviceName;

    // Add folder for devices by category to object folder
    UaFolder *pDevicesByTypeFolder = new UaFolder("DevicesByType", UaNodeId("DevicesByType", getNameSpaceIndex()), m_defaultLocaleId);
    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pDevicesByTypeFolder, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    // Locate Positioner device
    OpcUa_UInt32 posCount = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getDevices(GLOB_PositionerType);
    if (posCount != 1) {
        std::cout << "WARNING: " << posCount << " positioner(s) added. There should be exactly 1.\n" << std::endl;
    }
    ret = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getDeviceConfig(GLOB_PositionerType, 0, sDeviceName, identity);

    if (ret.isGood()) {
        std::cout << "Creating positioner OPC UA object...\n";
        //Create a folder for the positioner and add the folder to the ObjectsFolder
        PositionerObject *pPositioner = new PositionerObject(sDeviceName,
                                                             UaNodeId(sDeviceName, getNameSpaceIndex()),
                                                             m_defaultLocaleId, this, identity,
                                                             dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get()));
        ret = addNodeAndReference(OpcUaId_ObjectsFolder, pPositioner, OpcUaId_Organizes);
        UA_ASSERT(ret.isGood());
        ret = addUaReference(pPositioner->nodeId(), pPositioner->typeDefinitionId(), OpcUaId_HasTypeDefinition);
        UA_ASSERT(ret.isGood());
    }

    std::cout << "Creating all other OPC UA device objects and adding to DevicesByType...\n";

    // First create all nodes and add object type references
    // Also add to device folder
    for (auto it=PasCommunicationInterface::deviceTypeNames.begin(); it!=PasCommunicationInterface::deviceTypeNames.end(); ++it) {
        deviceType = it->first;
        count = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getDevices(deviceType);

        std::cout << "Creating " << it->second << " objects...\n";

        for (unsigned i = 0; i < count; i++)
        {
            ret = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getDeviceConfig(deviceType, i, sDeviceName, identity);
            pController = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getDeviceFromId(deviceType,
                                                                                                      identity);
            //If folder doesn't already exist, create a folder for each object type and add the folder to the DevicesByType folder
            if ( pDeviceFolders.find(deviceType) == pDeviceFolders.end() ) {
                deviceName = PasCommunicationInterface::deviceTypeNames[deviceType];
                folderName = deviceName + "Folder";
                pDeviceFolders[deviceType] = new UaFolder(UaString(folderName.c_str()), UaNodeId(UaString(folderName.c_str()), getNameSpaceIndex()), m_defaultLocaleId);
                ret = addNodeAndReference(pDevicesByTypeFolder, pDeviceFolders[deviceType], OpcUaId_Organizes);
            }

            // Create object
            pObject = PasObjectFactory::Create(deviceType, sDeviceName, UaNodeId(sDeviceName, getNameSpaceIndex()),
                                               m_defaultLocaleId, this, identity,
                                               dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get()));

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

    std::map<std::shared_ptr<PasController>, PasObject *> pRootDevices;

    for (const auto &kv : pDeviceObjects) {
        pRootDevices.insert(std::pair<std::shared_ptr<PasController>, PasObject *>(std::shared_ptr<PasController>(kv.first), kv.second));
    }

    UaString objectName;

    std::cout << "Adding all parent-child references between objects...\n";

    // Loop through all created objects and add references to children
    for (const auto &device : pDeviceObjects) {
        pController = device.first;
        pObject = device.second;

        objectName = pObject->nodeId().toString();

        // Check if object has children (is a composite controller)
        if (dynamic_cast<PasCompositeController *>(pController.get())) {
            std::cout << "Parent: " << pController->getId() << std::endl;
            for (const auto &devType: PasCommunicationInterface::deviceTypeNames) {
                deviceType = devType.first;
                deviceName = devType.second;
                try {
                    pChildren = dynamic_cast<PasCompositeController *>(pController.get())->getChildren(deviceType);
                    if (!pChildren.empty()) {
                        std::cout << deviceName << std::endl;
                        pFolder = new UaFolder(UaString(deviceName.c_str()), UaNodeId(objectName + UaString(deviceName.c_str()), getNameSpaceIndex()), m_defaultLocaleId);
                        ret = addNodeAndReference(pObject->nodeId(), pFolder, OpcUaId_HasComponent);
                        UA_ASSERT(ret.isGood());
                        for ( auto &child : pChildren) {
                            std::cout << child->getId() << std::endl;
                            ret = addUaReference(pFolder->nodeId(), pDeviceObjects[child]->nodeId(), OpcUaId_HasComponent);
                            UA_ASSERT(ret.isGood());
                            // Remove any child from the list of root devices
                            pRootDevices.erase(child);
                        }
                    }
                }
                catch (...) {
                    // Do nothing
                }
            }
            std::cout << std::endl << std::endl;
        }
    }

    std::cout << "Creating device tree...\n";

    // Add folder for device tree to Objects folder
    UaFolder *pDeviceTreeFolder = new UaFolder("DeviceTree", UaNodeId("DeviceTree", getNameSpaceIndex()), m_defaultLocaleId);
    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pDeviceTreeFolder, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    // Add all root devices (devices with no parents) to the Device Tree Folder
    for (const auto &r : pRootDevices) {
        pController = r.first;
        pObject = r.second;

        ret = addUaReference(pDeviceTreeFolder->nodeId(), pObject->nodeId(), OpcUaId_HasComponent);
        UA_ASSERT(ret.isGood());
    }

    std::cout << "PasNodeManager: Finished with AfterStartUp()\n";

    return ret;
}

UaStatus PasNodeManager::beforeShutDown()
{
    UaStatus ret;

    ret = m_pPositioner->disconnect();
    if (!ret.isGood())
        return ret;

    for (const auto &client : m_pClient)
    {
        ret = client->disconnect();
        if (!ret.isGood())
            continue;
    }

    return ret;
}

UaStatus PasNodeManager::amendTypeNodes()
{
    UaStatus ret;
    UaStatus addStatus;

    UaVariant                    defaultValue;
    UaObjectTypeSimple *pMirrorType = nullptr;
    UaObjectTypeSimple *pEdgeType = nullptr;
    UaObjectTypeSimple *pPanelType = nullptr;
    UaObjectTypeSimple *pOptTableType = nullptr;
    UaObjectTypeSimple *pCCDType = nullptr;
    OpcUa::DataItemType*         pDataItem;
    // Method helpers
    OpcUa::BaseMethod *pMethod = nullptr;
    UaPropertyMethodArgument *pPropertyArg = nullptr;
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
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_x, getNameSpaceIndex()),
                                        "x", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_y, getNameSpaceIndex()),
                                        "y", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_z, getNameSpaceIndex()),
                                        "z", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_xRot, getNameSpaceIndex()),
                                        "xRot", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_yRot, getNameSpaceIndex()),
                                        "yRot", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_zRot, getNameSpaceIndex()),
                                        "zRot", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
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

    defaultValue.setDouble(60.0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_PanelType_SafetyRadius, getNameSpaceIndex()),
            "SafetyRadius", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "MoveToActs"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_PanelType_MoveToLengths, getNameSpaceIndex()), "MoveToLengths",
                                    getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "MoveToActs"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_PanelType_MoveDeltaLengths, getNameSpaceIndex()), "MoveDeltaLengths",
                                    getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "MoveToCoords"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_PanelType_MoveToCoords, getNameSpaceIndex()), "MoveToCoords",
                                    getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Read"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_PanelType_ReadAll, getNameSpaceIndex()), "ReadPosition", getNameSpaceIndex());
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

    // Add Method "Stop"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_EdgeType_Stop, getNameSpaceIndex()), "Stop", getNameSpaceIndex());
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
     * Create the Mirror Type Instance declaration
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
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_x, getNameSpaceIndex()),
                                      "x", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_y, getNameSpaceIndex()),
                                        "y", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_z, getNameSpaceIndex()),
                                        "z", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_xRot, getNameSpaceIndex()),
                                        "xRot", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_yRot, getNameSpaceIndex()),
                                        "yRot", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_zRot, getNameSpaceIndex()),
                                        "zRot", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add systematic offsets
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_sysOffsetsMPES_x1, getNameSpaceIndex()),
            "sysOffsets_MPES_x1", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_sysOffsetsMPES_y1, getNameSpaceIndex()),
            "sysOffsets_MPES_y1", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_sysOffsetsMPES_x2, getNameSpaceIndex()),
            "sysOffsets_MPES_x2", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_sysOffsetsMPES_y2, getNameSpaceIndex()),
            "sysOffsets_MPES_y2", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_sysOffsetsMPES_x3, getNameSpaceIndex()),
            "sysOffsets_MPES_x3", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_sysOffsetsMPES_y3, getNameSpaceIndex()),
            "sysOffsets_MPES_y3", getNameSpaceIndex(), defaultValue,
            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    defaultValue.setString("");
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_SelectedEdges, getNameSpaceIndex()),
                                        "SelectedEdges", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite,
                                        this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    defaultValue.setString("");
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_SelectedMPES, getNameSpaceIndex()),
                                        "SelectedMPES", getNameSpaceIndex(), defaultValue,  Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite,
                                        this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    defaultValue.setString("");
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_SelectedPanels, getNameSpaceIndex()),
                                        "SelectedPanels", getNameSpaceIndex(), defaultValue,
                                         Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    //delete tempUIntArray;

    defaultValue.setDouble(60.0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(PAS_MirrorType_SafetyRadius, getNameSpaceIndex()),
            "SafetyRadius", getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Move"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_MirrorType_MoveToCoords, getNameSpaceIndex()), "MoveToCoords",
                                    getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "ReadPosition"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_MirrorType_ReadPosition, getNameSpaceIndex()), "ReadPosition",
                                    getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Align"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_MirrorType_AlignSequential, getNameSpaceIndex()), "AlignSequential",
                                    getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "Align"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_MirrorType_AlignSector, getNameSpaceIndex()), "AlignSector",
                                    getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "GlobalAlign"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_MirrorType_GlobalAlign, getNameSpaceIndex()), "GlobalAlign", getNameSpaceIndex());
    pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pMirrorType, pMethod, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Method "ReadAlignment"
    pMethod = new OpcUa::BaseMethod(UaNodeId(PAS_MirrorType_ReadSensors, getNameSpaceIndex()), "ReadSensors",
                                    getNameSpaceIndex());
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

    // Add Variable "curEl" as BaseDataVariable
    pDataItem = new OpcUa::DataItemType(UaNodeId(GLOB_PositionerType_curEl, getNameSpaceIndex()), "curEl",
    getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPositionerType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "curAz" as BaseDataVariable
    defaultValue.setFloat(0);
    pDataItem = new OpcUa::DataItemType(UaNodeId(GLOB_PositionerType_inAz, getNameSpaceIndex()), "inAz",
    getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    // Set Modelling Rule to Mandatory
    pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
    addStatus = addNodeAndReference(pPositionerType, pDataItem, OpcUaId_HasComponent);
    UA_ASSERT(addStatus.isGood());

    // Add Variable "curEl" as BaseDataVariable
    pDataItem = new OpcUa::DataItemType(UaNodeId(GLOB_PositionerType_inEl, getNameSpaceIndex()), "inEl",
    getNameSpaceIndex(), defaultValue, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite, this);
    // Set Modelling Rule to Mandatory
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
    pMethod = new OpcUa::BaseMethod(UaNodeId(GLOB_PositionerType_Initialize, getNameSpaceIndex()), "InitializeDrives",
                                    getNameSpaceIndex());
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

    OpcUa_Int32 actcount = m_pCommIf->getDeviceCount(PAS_ACTType);

    Device::Identity id;
    for (OpcUa_Int32 i = 0; i < actcount; i++)
    {
        status = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getDeviceConfig(PAS_ACTType, i, id);
        if (status.isGood())
        {
            printf("Will try changing state for %s\n", id.eAddress.c_str());
            status = m_pCommIf->setDeviceState(PAS_ACTType, id, Device::DeviceState::Off);
        }
        else
            printf("Problem changing state for %s\n", id.eAddress.c_str());
    }

    sleep(3);
    for (OpcUa_Int32 i = 0; i < actcount; i++)
    {
        status = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getDeviceConfig(PAS_ACTType, i, id);
        if (status.isGood())
        {
            printf("Will try changing state for %s again\n", id.eAddress.c_str());
            status = m_pCommIf->setDeviceState(PAS_ACTType, id, Device::DeviceState::On);
        }
        else
            printf("Problem changing state for %s\n", id.eAddress.c_str());
    }

    if (status.isGood())
        return 0;

    return -1;
}
