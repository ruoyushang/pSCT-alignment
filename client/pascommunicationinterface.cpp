/******************************************************************************
** Project: P2PAS server
**
** Description: Communication interface to access the devices.
******************************************************************************/
#include "pascommunicationinterface.hpp"
#include "client/utilities/configuration.hpp"
#include "passervertypeids.hpp"
#include "client/controllers/pascontroller.hpp"
#include "client/controllers/mirrorcontroller.hpp"
#include "client/controllers/positionercontroller.hpp"
#include "client/controllers/panelcontroller.hpp"
#include "client/controllers/edgecontroller.hpp"
#include "client/controllers/mpescontroller.hpp"
#include "client/controllers/ccdcontroller.hpp"
#include "client/controllers/psdcontroller.hpp"
#include "client/controllers/actcontroller.hpp"
#include "uadatetime.h"
#include <dirent.h>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <stdexcept>

// to discover Aravis devices
#include <arv.h>

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
    {PAS_PSDType, "PSD"}
};

PasCommunicationInterface::PasCommunicationInterface() :
    m_pConfiguration(nullptr),
    m_stop(OpcUa_False)
{
}

PasCommunicationInterface::~PasCommunicationInterface()
{
    m_stop = OpcUa_True; // Signal Thread to stop
    std::cout << "Closed and cleaned up PasCommunicationInterface\n";
}

UaStatus PasCommunicationInterface::initialize()
{
    // initialize devices communicating directly with the Alignment server (p2pasclient)

    // currently, only GAS CCDs need to be intialized.
    // find what's on the network and compare it to what's expected
    //arv_update_device_list();
    unsigned n_ccds = 0; // arv_get_n_devices();

    std::string serial;
    std::string ip;
    std::map<std::string, std::string> serial2ip;
    for (unsigned i = 0; i < n_ccds; i++) {
        serial = "0"; //arv_get_device_serial_nbr(i);
        ip = "0.0.0.0"; //arv_get_device_address(i);
        serial2ip[serial] = ip;
    }

    try {
        for (const auto& CCD : m_pConfiguration->getDeviceList(PAS_CCDType)) {
            try {
                if (serial2ip.at(std::to_string(CCD.serialNumber)) != CCD.eAddress) {
                    std::cout << " +++ WARNING +++ PasCommunicationInterface::Initialize(): "
                                 "mismatch in recorded config and actual IP assignment:" << std::endl;
                    std::cout << "        " << CCD.serialNumber << " is assigned "
                              << CCD.eAddress << ", but actually obtained "
                              << serial2ip[std::to_string(CCD.serialNumber)] << std::endl;
                }
                addDevice(nullptr, PAS_CCDType, CCD);
            }
            catch (std::out_of_range &e) {
                std::cout << " +++ WARNING +++ PasCommunicationInterface::Initialize(): CCD "
                          << CCD.serialNumber << " with assigned IP " << CCD.eAddress
                          << " isn't found on the network. Check your connection and restart."
                          << " Skipping..." << std::endl;
            }
        }
    }
    catch (std::out_of_range &e) {
        std::cout << " +++ WARNING +++ PasCommunicationInterface::Initialize(): "
                     "no CCD configurations found." << std::endl;
    }

    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       addDevice
    Description  add a device of specified type
-----------------------------------------------------------------------------*/
const Device::Identity
PasCommunicationInterface::addDevice(const std::shared_ptr<Client> &pClient, OpcUa_UInt32 deviceType,
                                     const Device::Identity &identity)
{
    // check if object already exists -- this way, passing the same object multiple times won't
    // actually add it multiple times
    Device::Identity id;

    // Found existing copy of device
    if (m_DeviceIdentityMap.count(deviceType) > 0 && m_DeviceIdentityMap.at(deviceType).count(identity) > 0) {
        int index = m_DeviceIdentityMap.at(deviceType).at(identity);
        std::cout << "PasCommunicationInterface::addDevice() : Device " << identity << " already exists as "
                  << PasCommunicationInterface::deviceTypeNames.at(deviceType) << "[" << index << "]. Moving on..."
                  << std::endl;
        return identity;
    }
        // Didn't find existing copy, create new one
    else {
        std::shared_ptr<PasController> pController;
        // up-casting is implicit
        if (deviceType == PAS_MPESType)
            pController = std::make_shared<MPESController>(identity, pClient);
        else if (deviceType == PAS_ACTType)
            pController = std::make_shared<ActController>(identity, pClient);
        else if (deviceType == PAS_PanelType)
            pController = std::make_shared<PanelController>(identity, pClient);
        else if (deviceType == PAS_EdgeType)
            pController = std::make_shared<EdgeController>(identity);
        else if (deviceType == PAS_MirrorType)
            pController = std::make_shared<MirrorController>(identity);
        else if (deviceType == PAS_CCDType)
            pController = std::make_shared<CCDController>(identity);
        else if (deviceType == PAS_PSDType)
            pController = std::make_shared<PSDController>(identity, pClient);
        else if (deviceType == GLOB_PositionerType)
            pController = std::make_shared<PositionerController>(identity, pClient);
        else {
            return identity;
        }

        // Initialize this controller or return if unable to do so for whatever reason
        if (!pController->initialize()) {
            std::cout << "Device " << identity << ": failed to initialize." << std::endl;
            return identity;
        }

        m_pControllers[deviceType].push_back(pController);
        unsigned curindex = m_pControllers.at(deviceType).size() - 1;
        m_DeviceIdentityMap[deviceType][identity] = curindex;

        std::cout << "PasCommunicationInterface::addDevice() Added "
                  << PasCommunicationInterface::deviceTypeNames[deviceType] << "["
                  << curindex << "] with identity " << identity << std::endl;

        // get the device's parents and create them.
        // if added device is MPES, its parents are the w-side panel and the edge;
        // if added device is ACT, its parent is the panel;
        // if added device is Panel or Edge, its parent is the mirror.
        auto parentList = m_pConfiguration->getParents(deviceType, identity);
        if (!parentList.empty()) {
            // for each parent, create it, complete the ID and add the current device as its child,
            // and add the other co-parents to it -- this guarantees that panels get added to their edges
            for (auto parent : parentList) {
                // parent.first is type; parent.second is id
                if (!parent.second.eAddress.empty()) {
                    Device::Identity parentId = addDevice(pClient, parent.first, parent.second);
                    // update this parent with the full ID
                    parent = std::make_pair(parent.first, parentId);

                    // this is not necessarily the last element in the vector of controllers --
                    // need to find its real index
                    try {
                        int index = m_DeviceIdentityMap.at(parent.first).at(parentId);
                        // add the current controller
                        std::dynamic_pointer_cast<PasCompositeController>(
                            m_pControllers.at(parent.first).at(index))->addChild(
                            deviceType, pController);
                    }
                    catch (std::out_of_range &e) { /* no such device for whatever reason -- ignore */ }
                }
            }
            // added all parents -- now add co-parents as each other's children.
            // Note that only addition of panels to edges or the mirror proceeds; other additions get ignored
            std::cout << "Adding co-parents as children..." << std::endl;
            for (unsigned i = 0; i < parentList.size(); i++) {
                const auto &parent = parentList.at(i);
                int parentIdx;
                try {
                    parentIdx = m_DeviceIdentityMap.at(parent.first).at(parent.second);
                    for (unsigned j = 0; j < parentList.size(); j++) {
                        if (j != i) {
                            const auto &coparent = parentList.at(j);
                            int coparentIdx;
                            coparentIdx = m_DeviceIdentityMap.at(coparent.first).at(coparent.second);
                            std::shared_ptr<PasController> parentC = m_pControllers.at(parent.first).at(parentIdx);
                            std::shared_ptr<PasController> coparentC = m_pControllers.at(coparent.first).at(
                                coparentIdx);
                            std::dynamic_pointer_cast<PasCompositeController>(parentC)->addChild(coparent.first,
                                                                                                 coparentC);
                        }
                    }
                }
                catch (std::out_of_range &e) {/*ignore*/}
            }
        }

        return identity;
    }
}
/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       getDevices
    Description  Get available devices of requested type
-----------------------------------------------------------------------------*/
OpcUa_Int32 PasCommunicationInterface::getDevices(OpcUa_UInt32 deviceType)
{
    OpcUa_Int32 count;
    try {
        count = m_pControllers.at(deviceType).size();
    }
    catch (std::out_of_range &e)
    {
        count = 0;
    }

    return count;
}

/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       getDeviceConfig
    Description  Get configuration of a sensor.
-----------------------------------------------------------------------------*/
UaStatus PasCommunicationInterface::getDeviceConfig(
        OpcUa_UInt32 type,
        OpcUa_UInt32 deviceIndex,
        UaString& sName,
        Device::Identity &identity)
{

    OpcUa_UInt32 devCount = getDevices(type);

    if ( deviceIndex < devCount )
    {
        try {
            identity = m_pControllers.at(type).at(deviceIndex)->getId();
        }
        catch (std::out_of_range &e)
        {
            return OpcUa_BadInvalidArgument;
        }
        sName = UaString(identity.name.c_str());
    }
    else
        return OpcUa_BadInvalidArgument;

    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       getDeviceConfig
    Description  Get configuration of a sensor -- overloaded.
-----------------------------------------------------------------------------*/
UaStatus PasCommunicationInterface::getDeviceConfig(
        OpcUa_UInt32 type,
        OpcUa_UInt32 deviceIndex,
        Device::Identity &identity)
{
    UaString sDiscard;
    return getDeviceConfig(type, deviceIndex, sDiscard, identity);
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
    int index;
    // try accessing the map for this type
    try {
        m_DeviceIdentityMap.at(type);
    }
    catch (std::out_of_range &e)
    {
        return nullptr;
    }

    // try getting the device with this identity
    try {
        index = m_DeviceIdentityMap.at(type).at(identity);
    }
    catch (std::out_of_range &e)
    {
        return nullptr;
    }

    return m_pControllers.at(type).at(index);
}
/* ----------------------------------------------------------------------------
    End Class     PasCommunicationInterface
    constructors / destructors
------------------------------------------- ---------------------------------*/
