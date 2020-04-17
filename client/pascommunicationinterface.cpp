/******************************************************************************
** Project: P2PAS server
**
** Description: Communication interface to access the devices.
******************************************************************************/
#include "client/pascommunicationinterface.hpp"

#include <dirent.h>

#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "uabase/uadatetime.h"

#include "common/opcua/passervertypeids.hpp"

#include "client/controllers/actcontroller.hpp"
#include "client/controllers/ccdcontroller.hpp"
#include "client/controllers/edgecontroller.hpp"
#include "client/controllers/mirrorcontroller.hpp"
#include "client/controllers/mpescontroller.hpp"
#include "client/controllers/positionercontroller.hpp"
#include "client/controllers/panelcontroller.hpp"
#include "client/controllers/pascontroller.hpp"
#include "client/controllers/psdcontroller.hpp"
#include "client/controllers/focalplanecontroller.hpp"
#include "client/controllers/opticalalignmentcontroller.hpp"
#include "client/controllers/globalalignmentcontroller.hpp"

#include "client/utilities/configuration.hpp"

#include <arv.h>

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

/* ----------------------------------------------------------------------------
    Begin Class    PasCommunicationInterface
    constructors / destructors
-----------------------------------------------------------------------------*/
// initialize the image directory path
//const char *PasCommunicationInterface::m_imageDir = "/home/root/opcua/sdk/mpesServer/testimages/";

std::map<OpcUa_UInt32, std::string> PasCommunicationInterface::deviceTypeNames{
    {PAS_MirrorType, "Mirror"},
    {PAS_PanelType, "Panel"},
    {PAS_EdgeType, "Edge"},
    {PAS_MPESType, "MPES"},
    {PAS_ACTType, "ACT"},
    {PAS_CCDType, "CCD"},
    {PAS_PSDType, "PSD"},
    {PAS_FocalPlaneType, "FocalPlane"},
    {PAS_OpticalAlignmentType, "OpticalAlignment"},
    {PAS_GlobalAlignmentType, "GlobalAlignment"},
    {GLOB_PositionerType, "Positioner"}
};

PasCommunicationInterface::PasCommunicationInterface() :
    m_pConfiguration(nullptr),
    m_stop(OpcUa_False)
{
}

PasCommunicationInterface::~PasCommunicationInterface()
{
    m_stop = OpcUa_True; // Signal Thread to stop
    spdlog::info("Closed and cleaned up PasCommunicationInterface");
}

UaStatus PasCommunicationInterface::initialize()
{
    initializeCCDs();
    return OpcUa_Good;
}

UaStatus PasCommunicationInterface::initializeCCDs()
{
    // initialize devices communicating directly with the Alignment server (p2pasclient)

    // currently, only GAS CCDs need to be intialized.
    // find what's on the network and compare it to what's expected

    arv_update_device_list();
    unsigned n_ccds = 0; // arv_get_n_devices();

//    std::string serial;
//    std::string ip;
//    std::map<std::string, std::string> serial2ip;
//    for (unsigned i = 0; i < n_ccds; i++) {
//        serial = "0"; //arv_get_device_serial_nbr(i);
//        ip = "0.0.0.0"; //arv_get_device_address(i);
//        serial2ip[serial] = ip;
//    }
    spdlog::info("PasCommunicationInterface::Initialize(): setting up CCD connection.");

    try {
        for (const auto &identity : m_pConfiguration->getDevices(PAS_CCDType)) {
            spdlog::info("{}",identity);
            try {
//                if (serial2ip.at(std::to_string(identity.serialNumber)) != identity.eAddress) {
//                    spdlog::warn(
//                        "PasCommunicationInterface::Initialize(): mismatch in recorded config and actual IP assignment. {} is assigned {}, but actually obtained {}.",
//                        identity.serialNumber, identity.eAddress, serial2ip[std::to_string(identity.serialNumber)]);
//                }
                addDevice(nullptr, PAS_CCDType, identity);
            }
            catch (std::out_of_range &e) {
                spdlog::warn(
                    "PasCommunicationInterface::Initialize(): CCD {} with assigned IP {} isn't found on the network. Check your connection and try again. Skipping...",
                    identity.serialNumber, identity.eAddress);
            }
        }
    }
    catch (std::out_of_range &e) {
        spdlog::warn("PasCommunicationInterface::Initialize(): no CCD configurations found.");
    }

    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       addDevice
    Description  add a device of specified type
-----------------------------------------------------------------------------*/
const Device::Identity
PasCommunicationInterface::addDevice(Client *pClient, OpcUa_UInt32 deviceType,
                                     const Device::Identity &identity, std::string mode)
{
    // check if object already exists -- this way, passing the same object multiple times won't
    // actually add it multiple times
    Device::Identity id;

    // Found existing copy of device
    if (m_pControllers.count(deviceType) > 0 && m_pControllers.at(deviceType).count(identity) > 0) {
        //spdlog::debug("PasCommunicationInterface::addDevice() : Device {} already exists. Moving on...", identity);
        return identity;
    } else { // Didn't find existing copy, create new one
        std::shared_ptr<PasController> pController;
        // up-casting is implicit
        if (deviceType == PAS_MPESType)
            pController = std::make_shared<MPESController>(identity, pClient, mode);
        else if (deviceType == PAS_ACTType)
            pController = std::make_shared<ActController>(identity, pClient);
        else if (deviceType == PAS_PanelType)
            pController = std::make_shared<PanelController>(identity, pClient, mode);
        else if (deviceType == PAS_EdgeType)
            pController = std::make_shared<EdgeController>(identity);
        else if (deviceType == PAS_MirrorType)
            pController = std::make_shared<MirrorController>(identity, mode);
        else if (deviceType == PAS_CCDType)
            pController = std::make_shared<CCDController>(identity);
        else if (deviceType == PAS_PSDType)
            pController = std::make_shared<PSDController>(identity, pClient);
        else if (deviceType == PAS_FocalPlaneType)
            pController = std::make_shared<FocalPlaneController>(identity, pClient);
        else if (deviceType == PAS_GlobalAlignmentType)
            pController = std::make_shared<GlobalAlignmentController>(identity, pClient);
        else if (deviceType == PAS_OpticalAlignmentType)
            pController = std::make_shared<OpticalAlignmentController>(identity, pClient);
        else if (deviceType == GLOB_PositionerType) {
#if SIMMODE
            pController = std::dynamic_pointer_cast<PositionerController>(std::make_shared<DummyPositionerController>(identity));
#else
            pController = std::make_shared<PositionerController>(identity, pClient);
#endif
        }
        else {
            return identity;
        }

        // Initialize this controller or return if unable to do so for whatever reason
        if (!pController->initialize()) {
            spdlog::error("Device {} failed to initialize.", identity);
            return identity;
        }

        m_pControllers[deviceType][identity] = pController;

        spdlog::info("PasCommunicationInterface::addDevice() Added {} with identity {}.",
                  PasCommunicationInterface::deviceTypeNames[deviceType], identity);

        return identity;
    }
}

void PasCommunicationInterface::addEdgeControllers() {
    for (const auto &edgeId : m_pConfiguration->getDevices(PAS_EdgeType)) {
        bool addEdge = true;
        // Check if all panels in edge exist
        auto children = m_pConfiguration->getChildren(edgeId);
        if (children.find(PAS_PanelType) != children.end()) {
            try {
                for (const auto &panelChildId : m_pConfiguration->getChildren(edgeId).at(PAS_PanelType)) {
                    if (m_pControllers.at(PAS_PanelType).find(panelChildId) ==
                        m_pControllers.at(PAS_PanelType).end()) {
                        // Child panel not found
                        //std::debug("Could not find panel {} as child of Edge {} (likely server failed to connect). Edge controller not created...", panelChildId, edgeId);
                        addEdge = false;
                        break;
                    }
                }
            }
            catch (std::out_of_range oor){
                spdlog::warn("No Panels found.");
                addEdge = false;
            }
            if (addEdge) {
                addDevice(nullptr, PAS_EdgeType, edgeId);
            } else {
                spdlog::warn(
                        "Could not find any panel children of Edge {} (likely server failed to connect). Edge controllers not created...",
                        edgeId);
            }
        }
    }
}

void PasCommunicationInterface::addOpticalAlignmentController() {
    for (const auto &oaID:  m_pConfiguration->getDevices(PAS_OpticalAlignmentType)){
        spdlog::debug("Adding Optical Alignment device...");
        spdlog::trace("Found ID: {}", oaID);
        addDevice(nullptr, PAS_OpticalAlignmentType, oaID);
    }
}

void PasCommunicationInterface::addGlobalAlignmentController() {
    for (const auto &gaID:  m_pConfiguration->getDevices(PAS_GlobalAlignmentType)){
        bool addGA = false;
        spdlog::debug("Adding Global Alignment device...");
        spdlog::trace("Found ID: {}", gaID);
        for (const auto &ccdId : m_pConfiguration->getChildren(gaID).at(PAS_CCDType)) {
            if (m_pControllers.at(PAS_CCDType).find(ccdId) !=
                m_pControllers.at(PAS_CCDType).end()) {
                // Child panel found
                addGA = true;
                break;
            }
        }
        if (addGA) {
            addDevice(nullptr, PAS_GlobalAlignmentType, gaID);
        } else {
            spdlog::warn(
                    "Could not find any CCD children of GA {} (likely server failed to connect). GA controller not created...",
                    gaID);
        }
    }
}

void PasCommunicationInterface::addFocalPlaneController() {
    for (const auto &fpID:  m_pConfiguration->getDevices(PAS_FocalPlaneType)){
        spdlog::debug("Adding Focal Plane device...");
        spdlog::trace("Found ID: {}", fpID);
        addDevice(nullptr, PAS_FocalPlaneType, fpID);
    }
}

void PasCommunicationInterface::addMirrorControllers() {
    for (const auto &mirrorId : m_pConfiguration->getDevices(PAS_MirrorType)) {
        bool addMirror = false;
        // Check if at least one panel in the mirror exists
        try {
            for (const auto &panelChildId : m_pConfiguration->getChildren(mirrorId).at(PAS_PanelType)) {
                if (m_pControllers.at(PAS_PanelType).find(panelChildId) !=
                    m_pControllers.at(PAS_PanelType).end()) {
                    // Child panel found
                    addMirror = true;
                    break;
                }
            }
        }
        catch (std::out_of_range oor){
            spdlog::warn("No Panels found.");
        }

        if (addMirror) {
            addDevice(nullptr, PAS_MirrorType, mirrorId);
        } else {
            spdlog::warn(
                "Could not find any panel children of Mirror {} (likely server failed to connect). Mirror controller not created...",
                mirrorId);
        }
    }
}

void PasCommunicationInterface::addParentChildRelations() {
    for (const auto &t : m_pControllers) {
        for (const auto &device : m_pControllers.at(t.first)) {
            Device::Identity childIdentity = device.first;
            std::shared_ptr<PasControllerCommon> childController = device.second;
            for (const auto &pair : m_pConfiguration->getParents(childIdentity)) {
                OpcUa_UInt32 parentDeviceType = pair.first;
                std::set<Device::Identity> parents = pair.second;
                for (const auto &parentId : parents) {
                    // If found controller for the desired parent, add it as a parent
                    if (m_pControllers[parentDeviceType].find(parentId) !=
                        m_pControllers[parentDeviceType].end()) {
                        std::dynamic_pointer_cast<PasCompositeController>(
                            m_pControllers.at(parentDeviceType).at(parentId))->addChild(t.first,
                                                                                        std::dynamic_pointer_cast<PasController>(
                                                                                            childController));
                    }
                }
            }
        }
    }
}

/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       getDeviceState
    Description  Get Device status.
-----------------------------------------------------------------------------*/
UaStatus PasCommunicationInterface::getDeviceState(
    OpcUa_UInt32 deviceType,
    const Device::Identity &identity,
    Device::DeviceState &state) {
    if (getDeviceFromId(deviceType, identity) != nullptr)
        return getDeviceFromId(deviceType, identity)->getState(state);

    return OpcUa_BadInvalidArgument;
}
/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       setSensorState
    Description  Set Sensor status.
-----------------------------------------------------------------------------*/
UaStatus PasCommunicationInterface::setDeviceState(
    OpcUa_UInt32 deviceType,
    const Device::Identity &identity,
    Device::DeviceState state) {
    if (getDeviceFromId(deviceType, identity) != nullptr)
        return getDeviceFromId(deviceType, identity)->setState(state);

    return OpcUa_BadInvalidArgument;
}
/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       getDeviceData
    Description  Get Sensor data.
-----------------------------------------------------------------------------*/
UaStatus PasCommunicationInterface::getDeviceData(
        OpcUa_UInt32 deviceType,
        const Device::Identity &identity,
        OpcUa_UInt32 offset,
        UaVariant &value) {
    if (getDeviceFromId(deviceType, identity) != nullptr)
        return getDeviceFromId(deviceType, identity)->getData(offset, value);

    return OpcUa_BadInvalidArgument;
}
/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       setSensorData
    Description  Set Sensor data.
-----------------------------------------------------------------------------*/
UaStatus PasCommunicationInterface::setDeviceData(
        OpcUa_UInt32 type,
        const Device::Identity &identity,
        OpcUa_UInt32 offset,
        UaVariant value)
{
    UaStatus status;
    if (getDeviceFromId(type, identity) != nullptr)
    {
        status = getDeviceFromId(type, identity)->setData(offset, value);
    }
    else
        status = OpcUa_BadInvalidArgument;

    return status;
}
/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       OperateDevice
    Description  Run a method on a device.
-----------------------------------------------------------------------------*/
UaStatus PasCommunicationInterface::operateDevice(
        OpcUa_UInt32 type, const Device::Identity &identity,
        OpcUa_UInt32 offset, const UaVariantArray &args)
{
    if (getDeviceFromId(type, identity) != nullptr)
        return getDeviceFromId(type, identity)->operate(offset, args);

    return OpcUa_BadInvalidArgument;
}
/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       getDeviceFromId
    Description  Return a device with the specified id.
-----------------------------------------------------------------------------*/
std::shared_ptr<PasController> PasCommunicationInterface::getDeviceFromId(OpcUa_UInt32 type,
                                                                          const Device::Identity &identity)
{
    try {
        return std::dynamic_pointer_cast<PasController>(m_pControllers.at(type).at(identity));
    }
    catch (std::out_of_range &e)
    {
        return nullptr;
    }
}
/* ----------------------------------------------------------------------------
    End Class     PasCommunicationInterface
    constructors / destructors
------------------------------------------- ---------------------------------*/
