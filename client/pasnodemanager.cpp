#include "client/pasnodemanager.hpp"


#include <memory>
#include <sstream>
#include <objects/ccdobject.hpp>

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
#include "client/objects/focalplaneobject.hpp"
#include "client/objects/globalalignmentobject.hpp"
#include "client/objects/opticalalignmentobject.hpp"
#include "client/pascommunicationinterface.hpp"
#include "client/pasobjectfactory.hpp"
#include "client/utilities/configuration.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"


PasNodeManager::PasNodeManager(std::shared_ptr<Configuration> pConfiguration, std::string mode)
    : PasNodeManagerCommon(), m_Mode(std::move(mode)), m_pConfiguration(std::move(pConfiguration)),
      m_pPositioner(nullptr) {
    spdlog::debug("PasNodeManager:: Creating new Node Manager...");
    m_pPositioner = new Client(this);
    m_pPositioner->setConfiguration(m_pConfiguration);

    createClients();
}

PasNodeManager::~PasNodeManager()
{
    spdlog::debug("PasNodeManager:: Cleaning up...");
}

void PasNodeManager::createClients() {
    unsigned numServers = 0;

    // Create as many clients as needed (the number of servers)
    if (m_Mode == "client") {
        numServers = 4;
    } else if (m_Mode == "subclient") {
        numServers = m_pConfiguration->getDevices(PAS_PanelType).size();
    }

    spdlog::info("PasNodeManager:: Attempting to create {} clients...", numServers);
    for (OpcUa_UInt32 i = 0; i < numServers; i++) {
        m_pClients.push_back(new Client(this));
        m_pClients.back()->setConfiguration(m_pConfiguration);
    }
}

void PasNodeManager::setCommunicationInterface(std::shared_ptr<PasCommunicationInterface> &pCommIf)
{
    spdlog::debug("PasNodeManager:: Setting communication interface...");
    m_pCommIf = pCommIf;
}

UaStatus PasNodeManager::afterStartUp()
{
    UaStatus ret;
    
    ret = createTypeNodes();
    UA_ASSERT(ret.isGood());
    ret = amendTypeNodes();
    UA_ASSERT(ret.isGood());

    // connect to each server
    spdlog::info("\nPasNodeManager::afterStartUp(): Connecting to all servers...");
    unsigned client = 0;
    for (const auto &address : m_pConfiguration->getServerAddresses()) {
        m_pClients.at(client)->setAddress(address);
        ret = m_pClients.at(client)->connect();
        if (ret.isGood()) {
            // add controllers for other devices in each server (this will only include ACT, MPES, and PSD controllers)
            ret = m_pClients.at(client)->browseAndAddDevices();
            spdlog::info("PasNodeManager::afterStartUp(): Successfully connected to server at {} and created all controllers.",address.toUtf8());
        } else
            spdlog::warn("PasNodeManager::afterStartUp(): Failed to connect to server at {}. Moving on...", address.toUtf8());

        ++client;
    }

    if (m_Mode == "client") {
        // connect to positioner
        spdlog::info("PasNodeManager::afterStartUp(): Attempting to create controller for positioner...");
        Device::Device::Identity id;
        UaString positioner_address = m_pConfiguration->getPositionerUrl();
        m_pPositioner->setAddress(positioner_address);
        id.eAddress = positioner_address.toUtf8();
        id.name = "Positioner";
        ret = m_pPositioner->connect();
        if (ret.isGood()) {
            // add the positioner to the comm interface
            dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->addDevice(m_pPositioner, GLOB_PositionerType,
                                                                                  id);
            spdlog::info("PasNodeManager::afterStartUp(): Connected to positioner and added corresponding controller.");
        } else {
            // add the positioner to the comm interface
            dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->addDevice(m_pPositioner, GLOB_PositionerType, id);
            spdlog::warn("PasNodeManager::afterStartUp(): Failed to connect to positioner server at {}. Moving on...",
                         m_pConfiguration->getPositionerUrl().toUtf8());
        }

        // Create all relevant mirror controllers
        spdlog::info("PasNodeManager::afterStartUp(): Creating all required mirror controllers...");
        dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->addMirrorControllers();

        // Create all relevant edge controllers
        spdlog::info("PasNodeManager::afterStartUp(): Creating all required edge controllers...");
        dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->addEdgeControllers();

        // Adding GlobalAlignment controller
        spdlog::info("PasNodeManager::afterStartUp(): Adding global alignment controller...");
        dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->addGlobalAlignmentController();

        // Create all relevant OptTable controllers
        spdlog::info("PasNodeManager::afterStartUp(): Creating all required OptTable controllers...");
        dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->addOpticalTableController();

        // Adding OpticalAlignment controller
        spdlog::info("PasNodeManager::afterStartUp(): Adding optical alignment controller...");
        dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->addOpticalAlignmentController();

        // Adding FocalPlane controller
        spdlog::info("PasNodeManager::afterStartUp(): Adding focal plane controller...");
        dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->addFocalPlaneController();

        Device::Identity rangefinderID = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getValidDeviceIdentities(PAS_RangefinderType).at(0);
        dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->addDevice(nullptr, PAS_RangefinderType, rangefinderID);

        Device::Identity LaserID = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getValidDeviceIdentities(PAS_LaserType).at(0);
        dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->addDevice(nullptr, PAS_LaserType, LaserID);

        Device::Identity positionerID = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getValidDeviceIdentities(GLOB_PositionerType).at(0);
        Device::Identity gaID = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getValidDeviceIdentities(PAS_GlobalAlignmentType).at(0);

    }

    // Finish controller initialization by adding parent-child relationships for all controllers
    spdlog::info("PasNodeManager::afterStartUp(): Finalizing all controller parent-child relationships...");
    dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->addParentChildRelations();

    spdlog::info("PasNodeManager::afterStartUp(): Now creating all OPC UA objects and folders...\n");

    UaFolder *pFolder = nullptr;
    PasObject *pObject = nullptr;
    std::shared_ptr<PasController> pController = nullptr;
    std::vector<std::shared_ptr<PasController>> pChildren;

    std::map<unsigned, UaFolder *> pDeviceFolders;
    std::map<std::shared_ptr<PasController>, PasObject *> pDeviceObjects;

    std::string deviceName;
    std::string folderName;
    unsigned deviceType;

    UaString sDeviceName;

    // Add folder for devices by category to object folder
    UaFolder *pDevicesByTypeFolder = new UaFolder("DevicesByType", UaNodeId("DevicesByType", getNameSpaceIndex()), m_defaultLocaleId);
    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pDevicesByTypeFolder, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    // Locate Positioner device
    OpcUa_UInt32 posCount = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getDeviceCount(GLOB_PositionerType);
    if (posCount == 1) {
     Device::Device::Identity identity = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getValidDeviceIdentities(GLOB_PositionerType).at(0);
        sDeviceName = UaString(identity.name.c_str());

        spdlog::info("PasNodeManager::afterStartUp(): Creating positioner OPC UA object...");
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
        spdlog::warn("{} positioner(s) added. There should be exactly 1. Skipping...", posCount);
    }

    spdlog::info(
        "PasNodeManager::afterStartUp(): Creating all other OPC UA device objects and adding to DevicesByType...");

    // First create all nodes and add object type references
    // Also add to device folder
    for (auto it=PasCommunicationInterface::deviceTypeNames.begin(); it!=PasCommunicationInterface::deviceTypeNames.end(); ++it) {
        deviceType = it->first;

        spdlog::info("PasNodeManager::afterStartUp(): Creating {} objects...\n", it->second);
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
                                               m_defaultLocaleId, this, deviceId,
                                               dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get()));

            // Create node
            ret = addUaNode(pObject);
            UA_ASSERT(ret.isGood());
            // Add object type reference

            ret = addUaReference(pObject->nodeId(), pObject->typeDefinitionId(), OpcUaId_HasTypeDefinition);
            UA_ASSERT(ret.isGood());

            // Add OpcUaId_HasComponent reference from the object type folder to
            // the object.
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

    spdlog::info("PasNodeManager::afterStartUp(): Adding all parent-child references between objects...\n\n");

    // Loop through all created objects and add references to children
    for (const auto &device : pDeviceObjects) {
        pController = device.first;
        pObject = device.second;

        objectName = pObject->nodeId().toString();

        // Check if object has children (is a composite controller)
        if (dynamic_cast<PasCompositeController *>(pController.get())) {
            std::string temp;
            std::ostringstream os(temp);
            os << "Parent: " << pController->getIdentity() << std::endl;
            for (const auto &devType: PasCommunicationInterface::deviceTypeNames) {
                deviceType = devType.first;
                deviceName = devType.second;
                try {
                    pChildren = dynamic_cast<PasCompositeController *>(pController.get())->getChildren(deviceType);
                    if (!pChildren.empty()) {
                        if (pChildren.size() > 1){
                            os << deviceName << std::endl;
                            pFolder = new UaFolder(UaString(deviceName.c_str()), UaNodeId(objectName + UaString(deviceName.c_str()), getNameSpaceIndex()), m_defaultLocaleId);
                            ret = addNodeAndReference(pObject->nodeId(), pFolder, OpcUaId_HasComponent);
                            UA_ASSERT(ret.isGood());
                            for ( auto &child : pChildren) {
                                os << child->getIdentity() << std::endl;
                                ret = addUaReference(pFolder->nodeId(), pDeviceObjects[child]->nodeId(), OpcUaId_HasComponent);
                                UA_ASSERT(ret.isGood());
                                // Remove any child from the list of root devices
                                pRootDevices.erase(child);
                            }
                        }
                        else{
                            os << deviceName << std::endl;
                            for ( auto &child : pChildren) {
                                os << child->getIdentity() << std::endl;
                                ret = addUaReference(pObject->nodeId(), pDeviceObjects[child]->nodeId(), OpcUaId_HasComponent);
                                UA_ASSERT(ret.isGood());
                                // Remove any child from the list of root devices
                                pRootDevices.erase(child);
                            }
                        }
                    }
                    else{
                        spdlog::debug("pChildren is empty for {}({})",deviceName, deviceType);
                    }
                }
                catch (...) {
                    // Do nothing
                }
            }
            os << std::endl;
            spdlog::info(os.str());
        }
    }

    spdlog::info("PasNodeManager::afterStartUp(): Creating device tree...");

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
    UaObjectTypeSimple *pFocalPlaneType = nullptr;
    UaObjectTypeSimple *pOpticalAlignmentType = nullptr;
    UaObjectTypeSimple *pGlobalAlignmentType = nullptr;
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
    pOptTableType = new UaObjectTypeSimple(
            "OptTable",    // Used as string in browse name and display name
            UaNodeId(PAS_OptTableType, getNameSpaceIndex()), // Numeric NodeId for types
            m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
            OpcUa_True);         // Abstract object type -> can not be instantiated
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pOptTableType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the OptTable Type Instance declaration
     ***************************************************************/
    // Register all variables
    for (auto v : OptTableObject::VARIABLES) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            std::get<3>(v.second), this);
        pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pOptTableType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all error variables
    for (auto v : OptTableObject::ERRORS) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            Ua_AccessLevel_CurrentRead, this);
        //pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Optional);
        addStatus = addNodeAndReference(pOptTableType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all methods
    for (auto m : OptTableObject::METHODS) {
        pMethod = new OpcUa::BaseMethod(UaNodeId(m.first, getNameSpaceIndex()), m.second.first.c_str(),
                                        getNameSpaceIndex());
        pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pOptTableType, pMethod, OpcUaId_HasComponent);
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
    * Create the CCD Type
    **************************************************************/
    // Add ObjectType "CCDType"
    pCCDType = new UaObjectTypeSimple(
            "CCDType",    // Used as string in browse name and display name
            UaNodeId(PAS_CCDType, getNameSpaceIndex()), // Numeric NodeId for types
            m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
            OpcUa_True);         // Abstract object type -> can not be instantiated
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pCCDType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the CCD Type Instance declaration
     ***************************************************************/
    // Register all variables
    for (auto v : CCDObject::VARIABLES) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            std::get<3>(v.second), this);
        pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pCCDType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all error variables
    for (auto v : CCDObject::ERRORS) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            Ua_AccessLevel_CurrentRead, this);
        //pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Optional);
        addStatus = addNodeAndReference(pCCDType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all methods
    for (auto m : CCDObject::METHODS) {
        pMethod = new OpcUa::BaseMethod(UaNodeId(m.first, getNameSpaceIndex()), m.second.first.c_str(),
                                        getNameSpaceIndex());
        pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pCCDType, pMethod, OpcUaId_HasComponent);
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
    * Create the Positioner Type
    **************************************************************/
    // Add ObjectType "PositionerType"
    UaObjectTypeSimple* pPositionerType = new UaObjectTypeSimple(
            "PositionerType",    // Used as string in browse name and display name
            UaNodeId(GLOB_PositionerType, getNameSpaceIndex()), // Numeric NodeId for types
            m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
            OpcUa_True);         // Abstract object type -> can not be instantiated
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pPositionerType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the Positioner Type Instance declaration
     ***************************************************************/
    // Register all variables
    for (auto v : PositionerObject::VARIABLES) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            std::get<3>(v.second), this);
        pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pPositionerType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all error variables
    for (auto v : PositionerObject::ERRORS) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            Ua_AccessLevel_CurrentRead, this);
        //pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Optional);
        addStatus = addNodeAndReference(pPositionerType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all methods
    for (auto m : PositionerObject::METHODS) {
        pMethod = new OpcUa::BaseMethod(UaNodeId(m.first, getNameSpaceIndex()), m.second.first.c_str(),
                                        getNameSpaceIndex());
        pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pPositionerType, pMethod, OpcUaId_HasComponent);
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
    * Create the Focal Plane Type
    **************************************************************/
    // Add ObjectType "FocalPlaneType"
    pFocalPlaneType = new UaObjectTypeSimple(
            "FocalPlaneType",    // Used as string in browse name and display name
            UaNodeId(PAS_FocalPlaneType, getNameSpaceIndex()), // Numeric NodeId for types
            m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
            OpcUa_True);         // Abstract object type -> can not be instantiated
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pFocalPlaneType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the Focal Plane Type Instance declaration
     ***************************************************************/
    // Register all variables
    for (auto v : FocalPlaneObject::VARIABLES) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            std::get<3>(v.second), this);
        pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pFocalPlaneType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all error variables
    for (auto v : FocalPlaneObject::ERRORS) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            Ua_AccessLevel_CurrentRead, this);
        //pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Optional);
        addStatus = addNodeAndReference(pFocalPlaneType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all methods
    for (auto m : FocalPlaneObject::METHODS) {
        pMethod = new OpcUa::BaseMethod(UaNodeId(m.first, getNameSpaceIndex()), m.second.first.c_str(),
                                        getNameSpaceIndex());
        pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pFocalPlaneType, pMethod, OpcUaId_HasComponent);
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
    * Create the Optical Alignment Type
    **************************************************************/
    // Add ObjectType "OpticalAlignmentType"
    pOpticalAlignmentType = new UaObjectTypeSimple(
            "OpticalAlignmentType",    // Used as string in browse name and display name
            UaNodeId(PAS_OpticalAlignmentType, getNameSpaceIndex()), // Numeric NodeId for types
            m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
            OpcUa_True);         // Abstract object type -> can not be instantiated
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pOpticalAlignmentType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the Optical Alignment Type Instance declaration
     ***************************************************************/
    // Register all variables
    for (auto v : OpticalAlignmentObject::VARIABLES) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            std::get<3>(v.second), this);
        pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pOpticalAlignmentType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all error variables
    for (auto v : OpticalAlignmentObject::ERRORS) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            Ua_AccessLevel_CurrentRead, this);
        //pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Optional);
        addStatus = addNodeAndReference(pOpticalAlignmentType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all methods
    for (auto m : OpticalAlignmentObject::METHODS) {
        pMethod = new OpcUa::BaseMethod(UaNodeId(m.first, getNameSpaceIndex()), m.second.first.c_str(),
                                        getNameSpaceIndex());
        pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pOpticalAlignmentType, pMethod, OpcUaId_HasComponent);
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
    * Create the Global Alignment Type
    **************************************************************/
    // Add ObjectType "GlobalAlignmentType"
    pGlobalAlignmentType = new UaObjectTypeSimple(
        "GlobalAlignmentType",    // Used as string in browse name and display name
        UaNodeId(PAS_GlobalAlignmentType, getNameSpaceIndex()), // Numeric NodeId for types
        m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
        OpcUa_True);         // Abstract object type -> can not be instantiated
    // Add new node to address space by creating a reference from BaseObjectType to this new node
    addStatus = addNodeAndReference(OpcUaId_BaseObjectType, pGlobalAlignmentType, OpcUaId_HasSubtype);
    UA_ASSERT(addStatus.isGood());

    /***************************************************************
     * Create the Global Alignment Type Instance declaration
     ***************************************************************/
    // Register all variables
    for (auto v : GlobalAlignmentObject::VARIABLES) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            std::get<3>(v.second), this);
        pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pGlobalAlignmentType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all error variables
    for (auto v : GlobalAlignmentObject::ERRORS) {
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            Ua_AccessLevel_CurrentRead, this);
        //pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Optional);
        addStatus = addNodeAndReference(pGlobalAlignmentType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all methods
    for (auto m : GlobalAlignmentObject::METHODS) {
        pMethod = new OpcUa::BaseMethod(UaNodeId(m.first, getNameSpaceIndex()), m.second.first.c_str(),
                                        getNameSpaceIndex());
        pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        addStatus = addNodeAndReference(pGlobalAlignmentType, pMethod, OpcUaId_HasComponent);
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
