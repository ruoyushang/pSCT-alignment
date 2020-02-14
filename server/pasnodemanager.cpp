/**
 * @file pasnodemanager.cpp
 * @brief Source file for the server-side node manager class.
 */

#include "server/pasnodemanager.hpp"


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

#include "common/alignment/device.hpp"
#include "common/opcua/mpeseventdata.hpp"
#include "common/opcua/pascominterfacecommon.hpp"
#include "common/opcua/pasnodemanagercommon.hpp"
#include "common/opcua/pasobject.hpp"
#include "common/opcua/passervertypeids.hpp"

#include "server/objects/panelobject.hpp"
#include "server/pascommunicationinterface.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"


PasServerNodeManager::PasServerNodeManager() : PasNodeManagerCommon()
{
    spdlog::debug("Created Node manager with NameSpaceIndex = {}", getNameSpaceIndex());

    m_pCommIf = std::make_shared<PasServerCommunicationInterface>();
    spdlog::debug("Adding pNodeManager to pCommIf");
    m_pCommIf->setPasServerNodeManager((std::shared_ptr<PasServerNodeManager>) this);

    spdlog::debug("Trying to create the HistoryManager responsible for this NodeManager");
#if SUPPORT_Historical_Access
    // Create the HistoryManager responsible for this NodeManager
    m_pHistoryManagerCache = new HistoryManagerCache;
    // Register the HistoryManager with the NodeManager base class
    setHistoryManager(m_pHistoryManagerCache);
#endif // SUPPORT_Historical_Access
}

PasServerNodeManager::~PasServerNodeManager()
{
#if SUPPORT_Historical_Access
    setHistoryManager(NULL);
    delete m_pHistoryManagerCache;
#endif // SUPPORT_Historical_Access
}


/// @details Creates all default and custom type nodes. Creates folders for MPES and Actuators.
/// Gets counts of devices from the PasServerCommunicationInterface and creates the corresponding object
/// OPC UA nodes and references in the server.
UaStatus PasServerNodeManager::afterStartUp()
{
    UaStatus ret;
    spdlog::debug("Creating OPC UA nodes for all devices found in communication interface...");

    // NOTE: These naked pointers are safe, as the destructor of NodeManagerBase, from which
    // PasNodeManagerCommon and PasServerNodeManager inherit, will destroy all nodes (which are registered with
    // addNode or addNodeandReference)

    PasObject *pObject;
    PanelObject *pPanel = nullptr;
    std::set<PasObject *> pChildObjects;

    std::map<OpcUa_UInt32, UaFolder *> pDeviceTypeFolders;
    std::map<OpcUa_UInt32, UaFolder *> pChildFolders;

    std::string deviceTypeName, folderName;

    std::vector<Device::Identity> validDeviceIdentities;

    ret = createTypeNodes(); // create default type nodes
    UA_ASSERT(ret.isGood());
    ret = amendTypeNodes(); // add custom type nodes (in this case for the panel object type)
    UA_ASSERT(ret.isGood());

    //Create alarm area folders for the sensor objects and add them to the Server object
    UaAreaFolder *pAreaMPESFolder = new UaAreaFolder(
            "Area", UaNodeId("AreaMPESEvents", getNameSpaceIndex()), m_defaultLocaleId);
    ret = addNodeAndReference(OpcUaId_Server, pAreaMPESFolder, OpcUaId_HasNotifier);
    UA_ASSERT(ret.isGood());
    registerEventNotifier(OpcUaId_Server, pAreaMPESFolder->nodeId()); // Register event notifier tree

    // Add folder for devices by type
    spdlog::debug("Creating DevicesByType OPC UA folder object...");
    UaFolder *pDevicesByTypeFolder = new UaFolder("DevicesByType", UaNodeId("DevicesByType", getNameSpaceIndex()),
                                                  m_defaultLocaleId);
    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pDevicesByTypeFolder, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    // First create all nodes, add object type references, and add to relevant folder
    // Note that only the Communication Interface knows what types and numbers of devices we have
    OpcUa_UInt32 deviceType;
    UaString deviceName;
    for (const auto &pair : PasServerCommunicationInterface::deviceTypes) {
        deviceType = pair.first;
        deviceTypeName = pair.second;
        spdlog::debug("Creating {} OPC UA device objects...", deviceTypeName);
        validDeviceIdentities = dynamic_cast<PasServerCommunicationInterface *>(m_pCommIf.get())->getValidDeviceIdentities(
                deviceType);

        for (const auto &identity : validDeviceIdentities) {
            //If folder doesn't already exist, create a folder for each object type and add the folder to the DevicesByType folder
            if (pDeviceTypeFolders.find(deviceType) == pDeviceTypeFolders.end()) {
                spdlog::debug("No folder found for device type {}. Creating...", deviceTypeName);
                folderName = deviceTypeName + "Folder";
                pDeviceTypeFolders[deviceType] = new UaFolder(folderName.c_str(),
                                                              UaNodeId(folderName.c_str(), getNameSpaceIndex()),
                                                              m_defaultLocaleId);
                ret = addNodeAndReference(pDevicesByTypeFolder, pDeviceTypeFolders[deviceType], OpcUaId_Organizes);
            }

            // Create OPC UA device object
            // Types hardcoded for now, in future consider creating a common factory class
            if (deviceType == PAS_ACTType) {
                deviceName = UaString((deviceTypeName + "_" + std::to_string(identity.serialNumber)).c_str());
                spdlog::debug("Creating OPC UA actuator object with identity {}...", identity);
                pObject = new ACTObject(deviceName, UaNodeId(deviceName, getNameSpaceIndex()), m_defaultLocaleId,
                                        dynamic_cast<PasNodeManagerCommon *>(this), identity, m_pCommIf.get());
            } else if (deviceType == PAS_MPESType) {
                deviceName = UaString((deviceTypeName + "_" + std::to_string(identity.serialNumber)).c_str());
                spdlog::debug("Creating OPC UA MPES object with identity {}...", identity);
                pObject = new MPESObject(deviceName, UaNodeId(deviceName, getNameSpaceIndex()), m_defaultLocaleId,
                                         dynamic_cast<PasNodeManagerCommon *>(this), identity,dynamic_cast<PasComInterfaceCommon *>(m_pCommIf.get()));
                (dynamic_cast<PasNodeManagerCommon *>(this))->__updatepCommIf(std::dynamic_pointer_cast<PasComInterfaceCommon>(m_pCommIf));
            } else if (deviceType == PAS_PanelType) {
                deviceName = UaString((deviceTypeName + "_" + std::to_string(identity.position)).c_str());
                spdlog::debug("Creating OPC UA panel object with identity {}...", identity);
                pObject = new PanelObject(deviceName, UaNodeId(deviceName, getNameSpaceIndex()), m_defaultLocaleId,
                                          this, identity, dynamic_cast<PasServerCommunicationInterface *>(m_pCommIf.get()));
            } else if (deviceType == PAS_PSDType) {
                deviceName = UaString((deviceTypeName + "_" + std::to_string(identity.serialNumber)).c_str());
                spdlog::debug("Creating OPC UA PSD object with identity {}...", identity);
                pObject = new PSDObject(deviceName, UaNodeId(deviceName, getNameSpaceIndex()), m_defaultLocaleId,
                                        dynamic_cast<PasNodeManagerCommon *>(this), identity, m_pCommIf.get());
            } else {
                return OpcUa_Bad;
            }

            ret = addUaNode(pObject); // Create node
            UA_ASSERT(ret.isGood());

            ret = addUaReference(pObject->nodeId(), pObject->typeDefinitionId(),
                                 OpcUaId_HasTypeDefinition); // Add object type reference
            UA_ASSERT(ret.isGood());

            ret = addUaReference(pDeviceTypeFolders[deviceType]->nodeId(), pObject->nodeId(),
                                 OpcUaId_HasComponent); // Add OpcUaId_HasComponent reference from the object type folder to the object.
            UA_ASSERT(ret.isGood());

            if (deviceType == PAS_PanelType) {
                if (!pPanel) {
                    pPanel = dynamic_cast<PanelObject *>(pObject); // Keep track of the single panel object separately (as it is the parent of all others).
                } else {
                    spdlog::warn(
                        "More than one panel found in communication interface! There should only be one per server.");
                }
            } else {
                pChildObjects.insert(pObject); // Add pointer to new object to pChildObjects set.
            }

            if (deviceType == PAS_MPESType) {
                ret = addUaReference(pAreaMPESFolder, pObject,
                                     OpcUaId_HasNotifier); // Add HasNotifier reference from alarm area to controller object
                UA_ASSERT(ret.isGood());
                registerEventNotifier(pAreaMPESFolder->nodeId(), pObject->nodeId());
            }
        }
    }

    // Check at least one panel has been found
    if (!pPanel) {
        spdlog::error("Error: No panel found. There should always be one in the server device list.");
        return OpcUa_Bad;
    }
    spdlog::debug("Panel object (parent) for this server: {}", pPanel->getIdentity());

    // Loop through all created objects and add as children of the panel
    for (auto p : pChildObjects) {
        deviceType = p->typeDefinitionId().identifierNumeric();
        deviceTypeName = PasServerCommunicationInterface::deviceTypes.at(p->typeDefinitionId().identifierNumeric());

        //If folder doesn't already exist, create a folder for each object type and add the folder to the DevicesByType folder
        if (pChildFolders.find(deviceType) == pChildFolders.end()) {
            spdlog::debug("No child folder found for device type {}. Creating as child of panel...", deviceTypeName);
            pChildFolders[deviceType] = new UaFolder(deviceTypeName.c_str(),
                                                     UaNodeId((deviceTypeName + "_children").c_str(),
                                                              getNameSpaceIndex()), m_defaultLocaleId);
            ret = addNodeAndReference(pPanel, pChildFolders[deviceType], OpcUaId_Organizes);
            UA_ASSERT(ret.isGood());
        }
        spdlog::debug("Adding OPC UA device object w/ identity {} as child of panel...", p->getIdentity());
        ret = addUaReference(pChildFolders[deviceType]->nodeId(), p->nodeId(), OpcUaId_HasComponent);
        UA_ASSERT(ret.isGood());
    }

    // Add folder for device tree to Objects folder
    spdlog::debug("Adding device tree folder to OPC UA interface...");
    UaFolder *pDeviceTreeFolder = new UaFolder("DeviceTree", UaNodeId("DeviceTree", getNameSpaceIndex()),
                                               m_defaultLocaleId);
    ret = addNodeAndReference(OpcUaId_ObjectsFolder, pDeviceTreeFolder, OpcUaId_Organizes);
    UA_ASSERT(ret.isGood());

    // Add panel as root of device tree
    spdlog::debug("Adding panel object as root of device tree...");
    ret = addUaReference(pDeviceTreeFolder->nodeId(), pPanel->nodeId(), OpcUaId_HasComponent);
    UA_ASSERT(ret.isGood());

    return ret;
}

UaStatus PasServerNodeManager::beforeShutDown()
{
    spdlog::debug("Shutting down pasNodeManager...");
    UaStatus ret;
    return ret;
}

/// @details Adds a type node (instance declaration) for the custom Panel type with all
/// associated properties, variables, and methods.
UaStatus PasServerNodeManager::amendTypeNodes()
{
    UaStatus status;

    spdlog::debug("Adding custom node type declarations...");
    spdlog::debug("Adding custom node type declaration for the Panel type...");

    // NOTE: No need to manually free allocated memory, as the NodeManagerBase destructor
    // will free all managed nodes. We need to use a naked pointer here because that is what the
    // NodeManager takes.

    // Add new ObjectType "PanelType" (inherits from BaseObjectType)
    UaObjectTypeSimple *pPanelType = new UaObjectTypeSimple(
            "PanelType",    // Used as string in browse name and display name
            UaNodeId(PAS_PanelType, getNameSpaceIndex()), // Numeric NodeId for types
            m_defaultLocaleId,   // Defaul LocaleId for UaLocalizedText strings
            OpcUa_True);         // Abstract object type -> can not be instantiated
    status = addNodeAndReference(OpcUaId_BaseObjectType, pPanelType, OpcUaId_HasSubtype);
    UA_ASSERT(status.isGood());

    // Register all variables
    spdlog::debug("Adding all variables to panel type definition...");
    OpcUa::DataItemType *pDataItem;
    for (auto &v : PanelObject::VARIABLES) {
        spdlog::trace("Adding variable {} to panel type definition...", std::get<0>(v.second));
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            std::get<3>(v.second), this);
        pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        status = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(status.isGood());
    }

    // Register all error variables
    spdlog::debug("Adding all error variables to panel type definition...");
    for (auto v : PanelObject::ERRORS) {
        spdlog::trace("Adding error variable {} to panel type definition...", std::get<0>(v.second));
        pDataItem = new OpcUa::DataItemType(UaNodeId(v.first, getNameSpaceIndex()),
                                            std::get<0>(v.second).c_str(), getNameSpaceIndex(), std::get<1>(v.second),
                                            Ua_AccessLevel_CurrentRead, this);
        //pDataItem->setModellingRuleId(OpcUaId_ModellingRule_Optional);
        status = addNodeAndReference(pPanelType, pDataItem, OpcUaId_HasComponent);
        UA_ASSERT(addStatus.isGood());
    }

    // Register all methods
    OpcUa::BaseMethod *pMethod;
    spdlog::debug("Adding all methods to panel type definition...");
    for (const auto &m : PanelObject::METHODS) {
        spdlog::trace("Adding method {} to panel type definition...", m.first);
        pMethod = new OpcUa::BaseMethod(UaNodeId(m.first, getNameSpaceIndex()), m.second.first.c_str(),
                                        getNameSpaceIndex());
        pMethod->setModellingRuleId(OpcUaId_ModellingRule_Mandatory);
        status = addNodeAndReference(pPanelType, pMethod, OpcUaId_HasComponent);
        UA_ASSERT(status.isGood());
    }

    return status;
}


void PasServerNodeManager::setPanelNumber(int panelNumber)
{
    m_panelNum = panelNumber;
    spdlog::debug("Setting panel number {} for pCommIf", m_panelNum);
    m_pCommIf->setPanelNumber(m_panelNum);
    spdlog::debug("Set panel number {}", m_pCommIf->getPanelNumber());
    UaStatus status;
    status = m_pCommIf->initialize();
    if (status.isGood()){
        spdlog::debug("Initialized pCommIf");
    }
    else{
        spdlog::warn("Could not initialize pCommIf");
    }
}

// SamplingOnRequestExample change begin
/** Overwrite of base class function to get informed by NodeManagerBase about a change in monitoring
 */

void PasServerNodeManager::variableCacheMonitoringChanged(UaVariableCache* pVariable, TransactionType transactionType)
{
    // Just make sure only handle the right variables
    if ( pVariable->valueHandling() != (UaVariable_Value_CacheIsSource | UaVariable_Value_CacheIsUpdatedOnRequest) )
    {
        return;
    }

    // Get fastest requested sampling interval requested by a client
    // Can be used to change polling rate to device if fastest rate changed
    OpcUa_Double fastedRequestedRate = pVariable->getMinSamplingInterval();
    OpcUa_ReferenceParameter(fastedRequestedRate);
    // This is not used in this example

    if ( (transactionType == IOManager::TransactionMonitorBegin) && (pVariable->signalCount() == 1) )
    {
        // The first monitored item was created for variable (pVariable)
        // Lock access to variable list
        UaMutexLocker lock(&m_mutexMonitoredVariables);

        // Add to map and set changed flag
        m_mapMonitoredVariables[pVariable] = pVariable;
        m_changedMonitoredVariables = true;
        // Increment reference counter for the entry in the map
        pVariable->addReference();
    }
    else if ( (transactionType == IOManager::TransactionMonitorStop) && (pVariable->signalCount() == 0) )
    {
        // The last monitored item was removed for variable (pVariable)
        // Lock access to variable list
        UaMutexLocker lock(&m_mutexMonitoredVariables);

        // Add to map and set changed flag
        std::map<UaVariableCache*, UaVariableCache*>::iterator it = m_mapMonitoredVariables.find(pVariable);
        if ( it != m_mapMonitoredVariables.end() )
        {
            m_mapMonitoredVariables.erase(it);
            m_changedMonitoredVariables = true;
            // Decrement reference counter since we removed the entry from the map
            pVariable->releaseReference();
        }
    }
}

/** Main method of worker thread for internal sampling
 */

void PasServerNodeManager::run()
{
    UaStatus     ret;
    OpcUa_UInt32 i;
    OpcUa_UInt32 count;
    OpcUa_DateTime nullTime;
    OpcUa_DateTime_Initialize(&nullTime);

    std::map<UaVariableCache*, UaVariableCache*>::iterator it;

    spdlog::debug("Begin worker thread for pNodeManager");

    while ( m_stopThread == false )
    {
        // Lock access to variable list
        UaMutexLocker lock(&m_mutexMonitoredVariables);
        // Check if the list was changed
        if ( m_changedMonitoredVariables )
        {
            // Update list for sampling
            // First release reference for all variables in array
            count = m_arrayMonitoredVariables.length();
            for ( i=0; i<count; i++ )
            {
                // Check if the variable is still used
                it = m_mapMonitoredVariables.find((UaVariableCache*)m_arrayMonitoredVariables[i]);
                if ( it == m_mapMonitoredVariables.end() )
                {
                    // Change value of variable to bad status BadWaitingForInitialData - it is not longer used
                    // This makes sure we do not deliver an old value when the monitoring is activated later for this variable
                    UaDataValue badStatusValue;
                    badStatusValue.setStatusCode(OpcUa_BadWaitingForInitialData);
                    m_arrayMonitoredVariables[i]->setValue(NULL, badStatusValue, OpcUa_False);
                }
                // Decrement reference counter for the variable - we removed it from the list
                m_arrayMonitoredVariables[i]->releaseReference();
            }
            // And clear old array
            m_arrayMonitoredVariables.clear();

            // Create the new array and increment reference counter for added variables
            it = m_mapMonitoredVariables.begin();
            m_arrayMonitoredVariables.create((OpcUa_UInt32) m_mapMonitoredVariables.size());
            count = m_arrayMonitoredVariables.length();
            for ( i=0; i<count; i++ )
            {
                m_arrayMonitoredVariables[i] = it->first;
                // Increment reference counter - it was added to the list
                m_arrayMonitoredVariables[i]->addReference();
                it++;
            }

            // Reset the change flag
            m_changedMonitoredVariables = false;
        }
        lock.unlock();

        // Check if we have anything to sample
        if ( m_arrayMonitoredVariables.length() > 0 )
        {
            UaDataValueArray results;
            count = m_arrayMonitoredVariables.length();

            // Call readValues to update variable values
            ret = readValues(m_arrayMonitoredVariables, results);
            if ( ret.isGood() )
            {
                // Update values
                for ( i=0; i<count; i++ )
                {
                    // The Read sets a new source timestamp
                    // -> we should only update source timestamp after a write
                    // -> or after value change (detected and set by UaVariable::setValue()
                    PasUserData* pUserData = (PasUserData*)m_arrayMonitoredVariables[i]->getUserData();
                    if ( pUserData && pUserData->updateTimestamp() != OpcUa_False )
                    {
                        // Reset update flag and keep source timestamp from read
                        pUserData->setUpdateTimestamp(OpcUa_False);
                    }
                    else
                    {
                        // Reset source timestamp to do the check for change in UaVariable::setValue()
                        results[i].setSourceTimestamp(nullTime);
                    }
                    m_arrayMonitoredVariables[i]->setValue(NULL, results[i], OpcUa_True);
                }
            }
            else
            {
                // Set bad status for all variables
                UaDataValue badStatusValue;
                badStatusValue.setStatusCode(ret.statusCode());
                for ( i=0; i<count; i++ )
                {
                    m_arrayMonitoredVariables[i]->setValue(NULL, badStatusValue, OpcUa_True);
                }
            }
        }

        UaThread::msleep(500);
    }
}
// SamplingOnRequestExample change end
