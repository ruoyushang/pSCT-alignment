/**
 * @file pascommunicationinterface.hpp
 * @brief Header file for panel server PAS communication interface
 */
#ifndef SERVER_PASCOMMUNICATIONINTERFACE_HPP
#define SERVER_PASCOMMUNICATIONINTERFACE_HPP

#include "uabase/uabase.h"
#include "uabase/uastring.h"
#include "uabase/uathread.h"
#include "uabase/uamutex.h"
#include "uabase/statuscode.h"
#include "uabase/uaarraytemplates.h"

#include "common/alignment/device.hpp"
#include "common/opcua/pascominterfacecommon.hpp"
#include "common/opcua/passervertypeids.hpp"
#include "common/utilities/DBConfig.hpp"

#include "server/pasnodemanager.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

class PlatformBase;
class PasController;

/// @brief Server communication interface to organize and interact with device controllers.
class PasServerCommunicationInterface : public PasComInterfaceCommon {
    UA_DISABLE_COPY(PasServerCommunicationInterface); // Disables copy construction and copy assignment
public:

    /// @brief Instantiate a communication interface object.
    PasServerCommunicationInterface() : m_panelNum(-1), m_cbcID(-1) {};

    /// @brief Destroy a communication interface object.
    ~PasServerCommunicationInterface() override;

    /// @brief Initialize all device controllers using information from the database.
    /// @return OPC UA status code indicating success/failure.
    UaStatus initialize() override;

    /// @brief Set the panel number used when retreiving information from the database.
    /// @param panelNumber Position number of the panel for this server.
    void setPanelNumber(int panelNumber) { m_panelNum = panelNumber; }

    /// @brief Get the a pointer to the device controller for a specific device.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @param identity The unique identity of the device.
    /// @return Pointer to the device controller.
    std::shared_ptr<PasControllerCommon> getDevice(OpcUa_UInt32 deviceType, const Device::Identity &identity) {
        return m_pControllers.at(deviceType).at(identity);
    }

    /// @brief Get a device's state through its controller.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @param identity The unique identity of the device.
    /// @param state Variable to store the retrieved state value.
    /// @return OPC UA status code indicating success/failure.
    UaStatus getDeviceState(
        OpcUa_UInt32 deviceType,
        const Device::Identity &identity,
        Device::DeviceState &state) override;

    /// @brief Get a device's data through its controller.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @param identity The unique identity of the device.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Variable to store the retrieved data value.
    /// @return OPC UA status code indicating success/failure.
    UaStatus getDeviceData(
        OpcUa_UInt32 deviceType,
        const Device::Identity &identity,
        OpcUa_UInt32 offset,
        UaVariant &value) override;

    /// @brief Set a device's state through its controller.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @param identity The unique identity of the device.
    /// @param state State value to set the device state to.
    /// @return OPC UA status code indicating success/failure.
    UaStatus setDeviceState(
        OpcUa_UInt32 deviceType,
        const Device::Identity &identity,
        Device::DeviceState state) override;

    /// @brief Set a device's data through its controller.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @param identity The unique identity of the device.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Value to write to the data variable.
    /// @return OPC UA status code indicating success/failure.
    UaStatus setDeviceData(
        OpcUa_UInt32 deviceType,
        const Device::Identity &identity,
        OpcUa_UInt32 offset,
        UaVariant value) override;

    /// @brief Operate a device through its controller.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @param identity The unique identity of the device.
    /// @param offset A number used to uniquely identify the method to call.
    /// @param args Array of method arguments as UaVariants.
    /// @return OPC UA status code indicating success/failure.
    UaStatus operateDevice(
        OpcUa_UInt32 deviceType,
        const Device::Identity &identity,
        OpcUa_UInt32 offset,
        const UaVariantArray &args) override;

    /// @brief Map of OPC UA type ID to device type name for all device types supported by the server.
    static const std::map<OpcUa_UInt32, std::string> deviceTypes;

    void setpNodeManager(PasNodeManager *pNodeManager);

private:

    /// @brief Position number of the panel. Used for device database lookup.
    int m_panelNum;

    /// @brief Controller board ID of the panel.
    int m_cbcID;

    /// @brief Pointer to the platform object used by all devices to interface with the hardware.
    std::shared_ptr<PlatformBase> m_platform;

    std::shared_ptr<PasNodeManager> m_pNodeManager;
};

#endif //SERVER_PASCOMMUNICATIONINTERFACE_HPP
