/******************************************************************************
** Project: P2PAS server
**
** Description: Communication interface to access the devices.
******************************************************************************/
#include "client/pascommunicationinterface.hpp"

#include <dirent.h>
#include <iostream>
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

#include "client/utilities/configuration.hpp"

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

    // arv_update_device_list();
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
        for (const auto &identity : m_pConfiguration->getDevices(PAS_CCDType)) {
            try {
                if (serial2ip.at(std::to_string(identity.serialNumber)) != identity.eAddress) {
                    std::cout << " +++ WARNING +++ PasCommunicationInterface::Initialize(): "
                                 "mismatch in recorded config and actual IP assignment:" << std::endl;
                    std::cout << "        " << identity.serialNumber << " is assigned "
                              << identity.eAddress << ", but actually obtained "
                              << serial2ip[std::to_string(identity.serialNumber)] << std::endl;
                }
                addDevice(nullptr, PAS_CCDType, identity);
            }
            catch (std::out_of_range &e) {
                std::cout << " +++ WARNING +++ PasCommunicationInterface::Initialize(): CCD "
                          << identity.serialNumber << " with assigned IP " << identity.eAddress
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
PasCommunicationInterface::addDevice(Client *pClient, OpcUa_UInt32 deviceType,
                                     const Device::Identity &identity)
{
    // check if object already exists -- this way, passing the same object multiple times won't
    // actually add it multiple times
    Device::Identity id;

    // Found existing copy of device
    if (m_pControllers.count(deviceType) > 0 && m_pControllers.at(deviceType).count(identity) > 0) {
        std::cout << "PasCommunicationInterface::addDevice() : Device " << identity << " already exists. Moving on..."
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
            std::cout << "Device " << identity << ": failed to initialize." << std::endl;
            return identity;
        }

        m_pControllers[deviceType][identity] = pController;
        
        std::cout << "PasCommunicationInterface::addDevice() Added "
                  << PasCommunicationInterface::deviceTypeNames[deviceType] << " with identity " << identity << std::endl;

        // get the device's parents and create them.
        for (const auto &pair : m_pConfiguration->getParents(identity)) {
            OpcUa_UInt32 parentDeviceType = pair.first;
            std::set<Device::Identity> parents = pair.second;
            for (const auto &parentId : parents) {
                if (!parentId.eAddress.empty()) {
                    addDevice(pClient, parentDeviceType, parentId);
                    std::dynamic_pointer_cast<PasCompositeController>(
                        m_pControllers.at(parentDeviceType).at(parentId))->addChild(deviceType, pController);
                }
            }
        }
        return identity;
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
