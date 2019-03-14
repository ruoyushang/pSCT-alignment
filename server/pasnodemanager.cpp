/**
 * @file pasnodemanager.cpp
 * @brief Source file for the server-side node manager class.
 */

#include "server/pasnodemanager.hpp"

#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "uabase/statuscode.h"
#include "uabase/uamutex.h"

#include "uaserver/opcua_analogitemtype.h"
#include "uaserver/opcua_offnormalalarmtype.h"
#include "uaserver/uaobjecttypes.h"

#include "common/opcua/mpeseventdata.h"
#include "common/opcua/pasnodemanagercommon.h"
#include "common/opcua/pasobject.h"
#include "common/opcua/passervertypeids.h"

#include "server/objects/panelobject.hpp"
#include "server/pascommunicationinterface.hpp"


PasNodeManager::PasNodeManager() : PasNodeManagerCommon()
{
    std::cout << "Created Node manager with NameSpaceIndex = " << getNameSpaceIndex() << std::endl;
}

/// @details Takes ownership of the heap-allocated PasCommunicationInterface by calling release on
/// the unique ptr and instantiating a new unique pointer.
void PasNodeManager::setCommunicationInterface(std::unique_ptr<PasCommunicationInterface> &pCommIf)
{
    std::cout << "PasNodeManager: Setting communication interface\n";
    m_pCommIf = std::unique_ptr<PasCommunicationInterface>(pCommIf.release()); // Note that we need to release the original unique pointer and make a new unique pointer
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
    PanelObject *pPanel;
    std::set<PasObject *> pChildObjects;
   
    std::map<OpcUa_UInt32, UaFolder *> pDeviceTypeFolders;
    std::map<OpcUa_UInt32, UaFolder *> pChildFolders;
    
    std::string deviceTypeName, folderName;

    std::vector<int> validDeviceAddresses;

    Identity identity;
    
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
    UaString deviceName;
    for (auto pair : PasCommunicationInterface::deviceTypes) {
        deviceType = pair.first;
        deviceTypeName = pair.second;
        validDeviceAddresses = dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get())->getValidDeviceAddresses(deviceType);

        for (auto eAddress : validDeviceAddresses) {
            identity.eAddress = eAddress;
            deviceName = UaString(deviceTypeName.c_str()) + "_" + identity.eAddress.c_str();
            //If folder doesn't already exist, create a folder for each object type and add the folder to the DevicesByType folder
            if ( pDeviceTypeFolders.find(deviceType) == pDeviceTypeFolders.end() ) {
                folderName = deviceTypeName + "Folder";
                pDeviceTypeFolders[deviceType] = new UaFolder(folderName.c_str(), UaNodeId(folderName.c_str(), getNameSpaceIndex()), m_defaultLocaleId);
                ret = addNodeAndReference(pDevicesByTypeFolder, pDeviceTypeFolders[deviceType], OpcUaId_Organizes);
            }

            // Create OPC UA device object
            // Types hardcoded for now, in future consider creating a common factory class
            if (deviceType == PAS_ACTType) {
                pObject = new ACTObject(deviceName, UaNodeId(deviceName, getNameSpaceIndex()), m_defaultLocaleId, dynamic_cast<PasNodeManagerCommon *>(this), identity, m_pCommIf.get()); 
            }
            else if (deviceType == PAS_MPESType) {
                pObject =  new MPESObject(deviceName, UaNodeId(deviceName, getNameSpaceIndex()), m_defaultLocaleId, dynamic_cast<PasNodeManagerCommon *>(this), identity, m_pCommIf.get());
            }
            else if (deviceType == PAS_PanelType) {
                pObject = new PanelObject(deviceName, UaNodeId(deviceName, getNameSpaceIndex()), m_defaultLocaleId, this, identity, dynamic_cast<PasCommunicationInterface *>(m_pCommIf.get()));
            }
            else if (deviceType == PAS_PSDType) {
                pObject = new PSDObject(deviceName, UaNodeId(deviceName, getNameSpaceIndex()), m_defaultLocaleId, dynamic_cast<PasNodeManagerCommon *>(this), identity, m_pCommIf.get());
            }

            ret = addUaNode(pObject); // Create node
            UA_ASSERT(ret.isGood());

            ret = addUaReference(pObject->nodeId(), pObject->typeDefinitionId(), OpcUaId_HasTypeDefinition); // Add object type reference
            UA_ASSERT(ret.isGood());

            ret = addUaReference(pDeviceTypeFolders[deviceType]->nodeId(), pObject->nodeId(), OpcUaId_HasComponent); // Add OpcUaId_HasComponent reference from the object type folder to the object.
            UA_ASSERT(ret.isGood());
                
            if (deviceType == PAS_PanelType) {
                pPanel = dynamic_cast<PanelObject *>(pObject); // Keep track of the single panel object separately (as it is the parent of all others).
            }
            else {
                pChildObjects.insert(pObject); // Add pointer to new object to pChildObjects set.
            }
            
            if (deviceType == PAS_MPESType) {
                ret = addUaReference(pAreaMPESFolder, pObject, OpcUaId_HasNotifier); // Add HasNotifier reference from alarm area to controller object
                UA_ASSERT(ret.isGood());
                registerEventNotifier(pAreaMPESFolder->nodeId(), pObject->nodeId());
            }
        }
    }
 
    // Loop through all created objects and add as children of the panel
    for (auto p : pChildObjects) {
        deviceTypeName = pObject->typeDefinitionId().toString().toUtf8();
        deviceType = pObject->typeDefinitionId().identifierNumeric();
       
        //If folder doesn't already exist, create a folder for each object type and add the folder to the DevicesByType folder
        if ( pChildFolders.find(deviceType) == pChildFolders.end() ) {
            pChildFolders[deviceType] = new UaFolder(deviceTypeName.c_str(), UaNodeId((deviceTypeName + "_children").c_str(), getNameSpaceIndex()), m_defaultLocaleId);
            ret = addNodeAndReference(pPanel, pChildFolders[deviceType], OpcUaId_Organizes);
            UA_ASSERT(ret.isGood());
        }

        ret = addUaReference(pChildFolders[deviceType]->nodeId(), p->nodeId(), OpcUaId_HasComponent);
        UA_ASSERT(ret.isGood());
    }

    // Add folder for device tree to Objects folder
    UaFolder *pDeviceTreeFolder = new UaFolder("DeviceTree", UaNodeId("DeviceTree", getNameSpaceIndex()), m_defaultLocaleId);
    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pDeviceTreeFolder, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    // Add panel as root of device tree
    ret = addUaReference(pDeviceTreeFolder->nodeId(), pPanel->nodeId(), OpcUaId_HasComponent);
    UA_ASSERT(ret.isGood());

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
    for (auto v : PanelObject::variables) {
      pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
              std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second), Ua_AccessLevel_CurrentRead, this);
      pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
      status = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
      UA_ASSERT(status.isGood());
    }

    // Register all methods
    OpcUa::BaseMethod* pMethod;
    for (auto m : PanelObject::methods) {
      pMethod = new OpcUa::BaseMethod(UaNodeId(m.first, getNameSpaceIndex()), m.second.first.c_str(), getNameSpaceIndex());
      pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
      status = addNodeAndReference(pPanelType, pMethod, OpcUaId_HasComponent);
      UA_ASSERT(status.isGood());
    }

    return status;
}
