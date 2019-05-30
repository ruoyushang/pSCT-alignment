#include "client/pasnodemanager.hpp"

#include <iostream>
#include <memory>

#include "uaserver/opcua_analogitemtype.h"

#include "common/alignment/device.hpp"
#include "common/opcua/pasobject.hpp"
#include "common/opcua/passervertypeids.hpp"

#include "client/clienthelper.hpp"
#include "client/controllers/pascontroller.hpp"
#include "client/objects/mirrorobject.hpp"
#include "client/objects/panelobject.hpp"
#include "client/objects/edgeobject.hpp"
#include "client/objects/opttableobject.hpp"
#include "client/objects/positionerobject.hpp"
#include "client/pascommunicationinterface.hpp"
#include "client/pasobjectfactory.hpp"
#include "client/utilities/configuration.hpp"


PasNodeManager::PasNodeManager()
: PasNodeManagerCommon()
{
    std::cout << "PasNodeManager:: Creating new Node Manager...\n";
    m_pPositioner = std::make_shared<Client>(this);
}

PasNodeManager::~PasNodeManager()
{
    std::cout << "PasNodeManager:: Cleaning up...\n";
}

void PasNodeManager::setCommunicationInterface(std::unique_ptr<PasCommunicationInterface> &pCommIf)
{
    std::cout << "PasNodeManager:: Setting communication interface...\n";
    m_pCommIf = std::unique_ptr<PasComInterfaceCommon>(pCommIf.release());
}

void PasNodeManager::setConfiguration(std::shared_ptr<Configuration> pConfiguration)
{
    std::cout << "PasNodeManager:: Setting configuration...\n";
    m_pConfiguration = std::move(pConfiguration);

    m_pPositioner->setConfiguration(m_pConfiguration);

    std::cout << "PasNodeManager:: Attempting to create " << m_pConfiguration->getServers() << " clients\n\n";
    for (OpcUa_UInt32 i = 0; i < m_pConfiguration->getServers(); i++) {
        m_pClients.push_back(std::make_shared<Client>(this));
        m_pClients.back()->setConfiguration(m_pConfiguration);
    }
}

UaStatus PasNodeManager::afterStartUp()
{
    UaStatus ret;
    
    ret = createTypeNodes();
    UA_ASSERT(ret.isGood());
    ret = amendTypeNodes();
    UA_ASSERT(ret.isGood());

    // connect to positioner:
    std::cout << "PasNodeManager::afterStartUp(): Attempting to create controller for positioner...\n";
    Device::Device::Identity id;
    UaString positioner_address = m_pConfiguration->getPositionerUrl();
    m_pPositioner->setAddress(positioner_address);
    ret = m_pPositioner->connect();
    if (ret.isGood()) {
        id.eAddress = positioner_address.toUtf8();
        id.name = "Positioner";
        // add the positioner to the comm interface
        dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->addDevice(m_pPositioner, GLOB_PositionerType, id);
        std::cout << "PasNodeManager::afterStartUp(): Connected to positioner and added corresponding controller.\n";
    } else {
        std::cout << "PasNodeManager::afterStartUp(): Failed to connect to positioner at " << m_pConfiguration->getPositionerUrl().toUtf8()
            << ". Moving on..." << std::endl;
    }

    // connecting each client to its server -- loop through all panels!
    // this will add each panel to the communication interface; it will also browse the panels
    // and add the devices on them, such as MPES/ACT/PSD;
    // in the process, it will construct the edges out of the corresponding panels and sensors,
    // as well as the whole mirror(s). WOW
    // number of clients is the same as the number of servers/panels by our set up
    std::cout << "\nPasNodeManager::afterStartUp(): Connecting to all panel servers...\n";
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
            std::cout << "PasNodeManager::afterStartUp(): Successfully connected to Panel " << panelId << " and created controller.\n";
        } else
            std::cout << "PasNodeManager::afterStartUp(): Failed to connect to Panel " << panelId << ". Moving on..." << std::endl;

        ++client;
    }

    std::cout << "\nPasNodeManager::afterStartUp(): Now creating all OPC UA objects and folders...\n";

    UaFolder *pFolder = nullptr;
    PasObject *pObject = nullptr;
    std::shared_ptr<PasController> pController = nullptr;
    std::vector<std::shared_ptr<PasController>> pChildren;

    std::map<unsigned, UaFolder *> pDeviceFolders;
    std::map<std::shared_ptr<PasController>, PasObject *> pDeviceObjects;

    std::string deviceName;
    std::string folderName;
    unsigned deviceType;

    Device::Device::Identity identity;
    UaString sDeviceName;

    // Add folder for devices by category to object folder
    UaFolder *pDevicesByTypeFolder = new UaFolder("DevicesByType", UaNodeId("DevicesByType", getNameSpaceIndex()), m_defaultLocaleId);
    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pDevicesByTypeFolder, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    // Locate Positioner device
    OpcUa_UInt32 posCount = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getDeviceCount(GLOB_PositionerType);
    if (posCount == 1) {
        identity = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getValidDeviceIdentities(GLOB_PositionerType).at(0);
        sDeviceName = UaString(identity.name.c_str());

        std::cout << "PasNodeManager::afterStartUp(): Creating positioner OPC UA object...\n";
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
    else {
        std::cout << "WARNING: " << posCount << " positioner(s) added. There should be exactly 1. Skipping...\n" << std::endl;
    }

    std::cout << "PasNodeManager::afterStartUp(): Creating all other OPC UA device objects and adding to DevicesByType...\n";

    // First create all nodes and add object type references
    // Also add to device folder
    for (auto it=PasCommunicationInterface::deviceTypeNames.begin(); it!=PasCommunicationInterface::deviceTypeNames.end(); ++it) {
        deviceType = it->first;

        std::cout << "PasNodeManager::afterStartUp(): Creating " << it->second << " objects...\n";
        for (const auto &deviceId : dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getValidDeviceIdentities(
            deviceType)) {
            sDeviceName = UaString(deviceId.name.c_str());
            pController = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getDeviceFromId(deviceType, deviceId);
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

    std::cout << "\nPasNodeManager::afterStartUp(): Adding all parent-child references between objects...\n\n";

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
            std::cout << std::endl;
        }
    }

    std::cout << "\nPasNodeManager::afterStartUp(): Creating device tree...\n";

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

    return ret;
}

UaStatus PasNodeManager::beforeShutDown()
{
    UaStatus ret;

    ret = m_pPositioner->disconnect();
    if (!ret.isGood())
        return ret;

    for (const auto &client : m_pClients) {
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
    // Register all variables
    for (auto v : PanelObject::VARIABLES) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            std::get<3>(v.second), this);
        pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all error variables
    for (auto v : PanelObject::ERRORS) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            Ua_AccessLevel_CurrentRead, this);
        //pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Optional);
        addStatus = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all methods
    for (auto m : PanelObject::METHODS) {
        pMethod = new OpcUa::BaseMethod(UaNodeId(m.first, getNameSpaceIndex()), m.second.first.c_str(),
                                        getNameSpaceIndex());
        pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pPanelType, pMethod, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());

        // Add arguments
        pPropertyArg = new UaPropertyMethodArgument(
            UaNodeId((std::to_string(m.first) + "_" + m.second.first + "_args").c_str(),
                     getNameSpaceIndex()), // NodeId of the property
            Ua_AccessLevel_CurrentRead,             // Access level of the property
            m.second.second.size(),                                      // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
        for (size_t i = 0; i < m.second.second.size(); i++) {
            pPropertyArg->setArgument(
                (OpcUa_UInt32) i,                       // Index of the argument
                std::get<0>(m.second.second[i]).c_str(),   // Name of the argument
                std::get<1>(m.second.second[i]),// Data type of the argument
                -1,                      // Array rank of the argument
                nullarray,               // Array dimensions of the argument
                UaLocalizedText("en", (std::get<2>(m.second.second[i])).c_str())); // Description
        }
        addStatus = addNodeAndReference(pMethod, pPropertyArg, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());
    }

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
    // Register all variables
    for (auto v : EdgeObject::VARIABLES) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            std::get<3>(v.second), this);
        pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pEdgeType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all error variables
    for (auto v : EdgeObject::ERRORS) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            Ua_AccessLevel_CurrentRead, this);
        //pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Optional);
        addStatus = addNodeAndReference(pEdgeType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all methods
    for (auto m : EdgeObject::METHODS) {
        pMethod = new OpcUa::BaseMethod(UaNodeId(m.first, getNameSpaceIndex()), m.second.first.c_str(),
                                        getNameSpaceIndex());
        pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pEdgeType, pMethod, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());

        // Add arguments
        pPropertyArg = new UaPropertyMethodArgument(
            UaNodeId((std::to_string(m.first) + "_" + m.second.first + "_args").c_str(),
                     getNameSpaceIndex()), // NodeId of the property
            Ua_AccessLevel_CurrentRead,             // Access level of the property
            m.second.second.size(),                                      // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
        for (size_t i = 0; i < m.second.second.size(); i++) {
            pPropertyArg->setArgument(
                (OpcUa_UInt32) i,                       // Index of the argument
                std::get<0>(m.second.second[i]).c_str(),   // Name of the argument
                std::get<1>(m.second.second[i]),// Data type of the argument
                -1,                      // Array rank of the argument
                nullarray,               // Array dimensions of the argument
                UaLocalizedText("en", (std::get<2>(m.second.second[i])).c_str())); // Description
        }
        addStatus = addNodeAndReference(pMethod, pPropertyArg, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());
    }

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
    // Register all variables
    for (auto v : MirrorObject::VARIABLES) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            std::get<3>(v.second), this);
        pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all error variables
    for (auto v : MirrorObject::ERRORS) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            Ua_AccessLevel_CurrentRead, this);
        //pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Optional);
        addStatus = addNodeAndReference(pMirrorType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all methods
    for (auto m : MirrorObject::METHODS) {
        pMethod = new OpcUa::BaseMethod(UaNodeId(m.first, getNameSpaceIndex()), m.second.first.c_str(),
                                        getNameSpaceIndex());
        pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pMirrorType, pMethod, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());

        // Add arguments
        pPropertyArg = new UaPropertyMethodArgument(
            UaNodeId((std::to_string(m.first) + "_" + m.second.first + "_args").c_str(),
                     getNameSpaceIndex()), // NodeId of the property
            Ua_AccessLevel_CurrentRead,             // Access level of the property
            m.second.second.size(),                                      // Number of arguments
            UaPropertyMethodArgument::INARGUMENTS); // IN arguments
        for (size_t i = 0; i < m.second.second.size(); i++) {
            pPropertyArg->setArgument(
                (OpcUa_UInt32) i,                       // Index of the argument
                std::get<0>(m.second.second[i]).c_str(),   // Name of the argument
                std::get<1>(m.second.second[i]),// Data type of the argument
                -1,                      // Array rank of the argument
                nullarray,               // Array dimensions of the argument
                UaLocalizedText("en", (std::get<2>(m.second.second[i])).c_str())); // Description
        }
        addStatus = addNodeAndReference(pMethod, pPropertyArg, OpcUaId_HasProperty);
        UA_ASSERT(addStatus.isGood());
    }

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

    for (const auto &id : dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getValidDeviceIdentities(
        PAS_ACTType)) {
        if (status.isGood()) {
            printf("Will try turning off %s\n", id.eAddress.c_str());
            status = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getDeviceFromId(PAS_ACTType, id)->operate(PAS_ACTType_TurnOff);
        } else
            printf("Problem turning off %s\n", id.eAddress.c_str());
    }

    sleep(3);
    for (const auto &id : dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getValidDeviceIdentities(
        PAS_ACTType)) {
        if (status.isGood()) {
            printf("Will try turning on %s again\n", id.eAddress.c_str());
            status = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getDeviceFromId(PAS_ACTType, id)->operate(PAS_ACTType_TurnOn);
        } else
            printf("Problem turning on %s\n", id.eAddress.c_str());
    }

    if (status.isGood())
        return 0;

    return -1;
}
