/**
 * @file pasnodemanager.cpp
 * @brief Source file for the server-side node manager class.
 */

 #include "server/pasnodemanager.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "uabase/statuscode.h"
#include "uabase/uamutex.h"
#include "uabase/uaobjecttypes.h"

#include "uaserver/opcua_analogitemtype.h"
#include "uaserver/opcua_offnormalalarmtype.h"

#include "common/opcua/mpeseventdata.h"
#include "common/opcua/pasobject.h"
#include "common/opcua/passervertypeids.h"

#include "server/panelobject.hpp"
#include "server/pascommunicationinterface.hpp"


PasNodeManager::PasNodeManager()
: PasNodeManagerCommon()
{
    std::cout << "Created Node manager with NameSpaceIndex = " getNameSpaceIndex() << "\n";
}

/// @details Takes ownership of the heap-allocated PasCommunicationInterface by calling release on
/// the unique ptr and instantiating a new unique pointer.
void PasNodeManager::setCommunicationInterface(std::unique_ptr<PasCommunicationInterface> pCommIf)
{
    std::cout << "PasNodeManager: Setting communication interface\n";
    m_pCommIf = std::unique_ptr<PasComInterfaceCommon>(static_cast<PasComInterfaceCommon>(pCommIf.release())); // Note that we need to release the original unique pointer and make a new unique pointer
}

/// @details Creates all default and custom type nodes. Creates folders for MPES and Actuators.
/// Gets counts of devices from the PasCommunicationInterface and creates the corresponding object
/// OPC UA nodes and references in the server.
UaStatus PasNodeManager::afterStartUp()
{
    UaStatus ret;

    // NOTE: These naked pointers are safe, as the destructor of NodeManagerBase, from which
    // PasNodeManagerCommon and PasNodeManager inherit, will destroy all nodes (which are registered with
    // addNode or addNodeandReference)

    PasObject *pObject;
    PasController *pController;
    std::vector<PasController *> pChildren;

    std::map<OpcUa_UInt32, UaFolder *> pDeviceFolders;
    std::map<PasController *, PasObject *> pDeviceObjects;

    std::string deviceTypeName, folderName;

    std::vector<int> validDeviceAddresses;
    int eAddress;

    Identity identity;
    UaString sDeviceName;

    ret = createTypeNodes(); // create default type nodes
    UA_ASSERT(ret.isGood());
    ret = amendTypeNodes(); // add custom type nodes (in this case for the panel object type)
    UA_ASSERT(ret.isGood());

    //Create alarm area folders for the sensor objects and add them to the Server object
    UaAreaFolder* pAreaMPESFolder = new UaAreaFolder(
        "Area", UaNodeId("AreaMPESEvents", getNameSpaceIndex()), m_defaultLocaleId);
    ret = addNodeAndReference(OpcUaId_Server, pAreaMPESFolder, OpcUaId_HasNotifier);
    UA_ASSERT(ret.isGood());
    registerEventNotifier(OpcUaId_Server, pAreaMPESFolder->nodeId()); // Register event notifier tree

    // Add folder for devices by type
    UaFolder* pDevicesByTypeFolder = new UaFolder("DevicesByType", UaNodeId("DevicesByType", getNameSpaceIndex()), m_defaultLocaleId);
    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pDevicesByTypeFolder, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    // First create all nodes, add object type references, and add to relevant folder
    // Note that only the Communication Interface knows what types and numbers of devices we have
    OpcUa_UInt32 deviceType;
    for (auto it=PasCommunicationInterface::deviceTypeNames.begin(); it!=PasCommunicationInterface::deviceTypeNames.end(); ++it) {
        deviceTypeName = it->first;
        deviceType = it->second;
        validDeviceAddresses = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getValidDeviceAddresses(deviceType);

        for (auto it=validDeviceAddresses.begin(); it!=validDeviceAddresses.end(); ++it)
        {
            eAddress = *it;
            identity.eAddress = eAddress;
            sDeviceName = UaString(deviceTypeName.c_str()) + "_" + identity.eAddress.c_str();
            pController = dynamic_cast<PasCommunicationInterface *>(m_pCommIf)->getDevice(deviceType, eAddress);
            //If folder doesn't already exist, create a folder for each object type and add the folder to the DevicesByType folder
            if ( pDeviceFolders.find(deviceType) == pDeviceFolders.end() ) {
                folderName = deviceTypeName + "Folder";
                pDeviceFolders[deviceType] = new UaFolder(folderName.c_str()), UaNodeId(folderName.c_str(), getNameSpaceIndex()), m_defaultLocaleId);
                ret = addNodeAndReference(pDevicesByTypeFolder, pDeviceFolders[deviceType], OpcUaId_Organizes);
            }

            // Create OPC UA device object
            // Types hardcoded for now, in future consider creating a common factory class
            if (deviceType == PAS_ACTType):
                pObject = new ACTObject(sDeviceName, UaNodeId(sDeviceName, getNameSpaceIndex()), m_defaultLocaleId, dynamic_cast<PasNodeManagerCommon *>(this), identity, dynamic_cast<PasComInterfaceCommon *>(m_pCommIf));
            else if (deviceType == PAS_MPESType):
                pObject =  new MPESObject(sDeviceName, UaNodeId(sDeviceName, getNameSpaceIndex()), m_defaultLocaleId, dynamic_cast<PasNodeManagerCommon *>(this), identity, dynamic_cast<PasComInterfaceCommon *>(m_pCommIf));
            else if (deviceType == PAS_PanelType):
                pObject = new PanelObject(sDeviceName, UaNodeId(sDeviceName, getNameSpaceIndex()), m_defaultLocaleId, this, identity, m_pCommIf);
            else if (deviceType == PAS_PSDType):
                pObject = new PSDObject(sDeviceName, UaNodeId(sDeviceName, getNameSpaceIndex()), m_defaultLocaleId, dynamic_cast<PasNodeManagerCommon *>(this), identity, dynamic_cast<PasComInterfaceCommon *>(m_pCommIf));

            ret = addUaNode(pObject); // Create node
            UA_ASSERT(ret.isGood());

            ret = addUaReference(pObject->nodeId(), pObject->typeDefinitionId(), OpcUaId_HasTypeDefinition); // Add object type reference
            UA_ASSERT(ret.isGood());

            ret = addUaReference(pDeviceFolders[deviceType]->nodeId(), pObject->nodeId(), OpcUaId_HasComponent); // Add OpcUaId_HasComponent reference from the object type folder to the object.
            UA_ASSERT(ret.isGood());

            pDeviceObjects[pController] = pObject; // Add pointer to new object to pDeviceObjects map.

            if (deviceType == PAS_MPESType) {
              ret = addUaReference(pAreaMPESFolder, pMPES, OpcUaId_HasNotifier); // Add HasNotifier reference from alarm area to controller object
              UA_ASSERT(ret.isGood());
              ret = registerEventNotifier(pAreaMPESFolder->nodeId(), pMPES->nodeId());
              UA_ASSERT(ret.isGood());
            }
        }
    }

    std::map<PasController *, PasObject *> pRootDevices;
    pRootDevices.insert(pDeviceObjects.begin(), pDeviceObjects.end()); // Copy all elements in pDeviceObjects to pRootDevices

    // Loop through all created objects and add references to children
    UaString objectName;
    UaFolder* pFolder;
    for (auto it=pDeviceObjects.begin(); it!=pDeviceObjects.end(); ++it) {
        pController = it->first;
        pObject = it->second;

        objectName = pObject->nodeId().toString();

        // Check if object has children
        // Add folder for each type of child and add children to folder
        // Remove devices which are children from pRootDevices map
        if (dynamic_cast<PasCompositeController*>(pController)) {
            for (auto it=PasCommunicationInterface::deviceTypeNames.begin();
            it!=PasCommunicationInterface::deviceTypeNames.end(); ++it) {
                deviceType = it->first;
                deviceName = it->second;
                try {
                    pChildren = dynamic_cast<PasCompositeController*>(pController)->getChildren(deviceType);
                    if (!pChildren.empty()) {
                        pFolder = new UaFolder(UaString(deviceName.c_str()), UaNodeId(objectName + UaString(deviceName.c_str()), getNameSpaceIndex()), m_defaultLocaleId);
                        ret = addNodeAndReference(pObject->nodeId(), pFolder, OpcUaId_HasComponent);
                        UA_ASSERT(ret.isGood());
                        for (auto &child : pChildren) {
                            ret = addUaReference(pFolder->nodeId(), pDeviceObjects[child]->nodeId(), OpcUaId_HasComponent);
                            UA_ASSERT(ret.isGood());
                            pRootDevices.erase(child); // Remove any child from the list of root devices
                        }
                    }
                }
                catch (...) {
                    // Do nothing
                }
            }
        }
    }

    // Add folder for device tree to Objects folder
    UaFolder *pDeviceTreeFolder = new UaFolder("DeviceTree", UaNodeId("DeviceTree", getNameSpaceIndex()), m_defaultLocaleId);
    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pDeviceTreeFolder, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    // Add all root devices (devices with no parents) to the Device Tree Folder
    for (auto it=pRootDevices.begin(); it!=pRootDevices.end(); ++it) {
        pController = it->first;
        pObject = it->second;

        ret = addUaReference(pDeviceTreeFolder->nodeId(), pObject->nodeId(), OpcUaId_HasComponent);
        UA_ASSERT(ret.isGood());
    }

    return ret;
}

UaStatus PasNodeManager::beforeShutDown()
{
    UaStatus ret;
    return ret;
}

/// @details Adds a type node (instance declaration) for the custom Panel type with all
/// associated properties + variables + methods.
UaStatus PasNodeManager::amendTypeNodes()
{
    UaStatus status;

    // NOTE: No need to manually free allocated memory, as the NodeManagerBase destructor
    // will free all managed nodes. We need to use a naked pointer here because that is what the
    // NodeManager takes.

    // Add new ObjectType "PanelType" (inherits from BaseObjectType)
    UaObjectTypeSimple* pPanelType = new UaObjectTypeSimple(
        "PanelType",    // Used as string in browse name and display name
        UaNodeId(PAS_PanelType, getNameSpaceIndex()), // Numeric NodeId for types
        m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
        OpcUa_True);         // Abstract object type -> can not be instantiated
    status = addNodeAndReference(OpcUaId_BaseObjectType, pPanelType, OpcUaId_HasSubtype);
    UA_ASSERT(status.isGood());

    // Register all variables
    OpcUa::DataItemType* pDataItem;
    for (auto it = PanelObject::variables.begin(); it != PanelObject::variables.end(); ++it) {
      pDataItem = new OpcUa::DataItemType(UaNodeId(it->first, getNameSpaceIndex()),
              std::get<0>(it->second), getNameSpaceIndex(), std::get<1>(it->second), Ua_AccessLevel_CurrentRead, this);
      pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
      status = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
      UA_ASSERT(status.isGood());
    }

    // Register all methods
    OpcUa::BaseMethod* pMethod;
    for (auto it = PanelObject::methods.begin(); it != PanelObject::methods.end(); ++it) {
      pMethod = new OpcUa::BaseMethod(UaNodeId(it->first, getNameSpaceIndex()), it->second.first, getNameSpaceIndex());
      pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
      status = addNodeAndReference(pPanelType, pMethod, OpcUaId_HasComponent);
      UA_ASSERT(status.isGood());
    }

    return status;
}
