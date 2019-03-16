/**
 * @file pascommunicationinterface.hpp
 * @brief Header file for panel server PAS communication interface
 */
#ifndef __PASCOMMUNICATIONINTERFACE_H__
#define __PASCOMMUNICATIONINTERFACE_H__

#include "uabase/uabase.h"
#include "uabase/uastring.h"
#include "uabase/uathread.h"
#include "uabase/uamutex.h"
#include "uabase/statuscode.h"
#include "uabase/uaarraytemplates.h"

#include "common/opcua/pascominterfacecommon.h"
#include "common/opcua/passervertypeids.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class Platform;
class PasController;
struct Identity;

/// @brief Server communication interface to organize and interact with device controllers.
class PasCommunicationInterface : public PasComInterfaceCommon
{
    UA_DISABLE_COPY(PasCommunicationInterface); // Disables copy construction and copy assignment
public:

    /// @brief Instantiate a communication interface object.
    PasCommunicationInterface();
    /// @brief Destroy a communication interface object.
    ~PasCommunicationInterface();

    /// @brief Initialize all device controllers using information from the database.
    /// @return OPC UA status code indicating success/failure.
    UaStatusCode Initialize();
    /// @brief Set the panel number used when retreiving information from the database.
    /// @param panelNumber Position number of the panel for this server.
    void setPanelNumber(const std::string& panelNumber) { m_panelNum = panelNumber; }

    /// @brief Get the total number of devices of a given type.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @return The number of found device controllers.
    std::size_t getDeviceCount(OpcUa_UInt32 deviceType);

    /// @brief Get the eAddresses of all devices of a given type.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @return A vector of device eAddresses.
    std::vector<int> getValidDeviceAddresses(OpcUa_UInt32 deviceType);

    /// @brief Get the a pointer to the device controller for a specific device.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @param eAddress The electronic address (USB/serial port number) of the desired device.
    /// @return Pointer to the device controller.
    std::unique_ptr<PasController>& getDevice(OpcUa_UInt32 deviceType, int eAddress);

    /// @brief Get a device's state through its controller.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @param eAddress The electronic address (USB/serial port number) of the desired device.
    /// @param state Variable to store the retrieved state value.
    /// @return OPC UA status code indicating success/failure.
    UaStatusCode getDeviceState(
        OpcUa_UInt32 deviceType,
        int eAddress,
        PASState& state);

    /// @brief Get a device's data through its controller.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @param eAddress The electronic address (USB/serial port number) of the desired device.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Variable to store the retrieved data value.
    /// @return OPC UA status code indicating success/failure.
    UaStatusCode getDeviceData(
        OpcUa_UInt32 deviceType,
        int eAddress,
        OpcUa_UInt32 offset,
        UaVariant& value);

    /// @brief Set a device's state through its controller.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @param eAddress The electronic address (USB/serial port number) of the desired device.
    /// @param state State value to set the device state to.
    /// @return OPC UA status code indicating success/failure.
    UaStatusCode setDeviceState(
        OpcUa_UInt32 deviceType,
        int eAddress,
        PASState state);

    /// @brief Set a device's data through its controller.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @param eAddress The electronic address (USB/serial port number) of the desired device.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Value to write to the data variable.
    /// @return OPC UA status code indicating success/failure.
    UaStatusCode setDeviceData(
        OpcUa_UInt32 deviceType,
        int eAddress,
        OpcUa_UInt32 offset,
        UaVariant value);

    /// @brief Operate a device through its controller.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @param eAddress The electronic address (USB/serial port number) of the desired device.
    /// @param offset A number used to uniquely identify the method to call.
    /// @param args Array of method arguments as UaVariants.
    /// @return OPC UA status code indicating success/failure.
    UaStatusCode OperateDevice(
        OpcUa_UInt32 deviceType,
        int eAddress,
        OpcUa_UInt32 offset,
        const UaVariantArray& args);

    /// @brief Map of OPC UA type ID to device type name for all device types supported by the server.
    static const std::map<OpcUa_UInt32, std::string> deviceTypes;

    // Unused/temporary methods to match interface of virtual parent class

    /// @brief Temporary/unused, remove after reworking common/pascommmunicationinterfacecommon.h.
    UaStatusCode getDeviceConfig(
        OpcUa_UInt32 deviceType,
        OpcUa_UInt32 deviceIndex,
        UaString& sName,
        Identity& identity) { return OpcUa_BadNotImplemented; };

    /// @brief Temporary/unused, remove after reworking common/pascommmunicationinterfacecommon.h.
    OpcUa_Int32 getDevices(OpcUa_UInt32 deviceType) { return 0; };

    /// @brief Temporary/unused, remove after reworking common/pascommmunicationinterfacecommon.h.
    UaStatusCode getDeviceState(OpcUa_UInt32 type, const Identity& identity, PASState& state) { return OpcUa_BadNotImplemented; };

    /// @brief Temporary/unused, remove after reworking common/pascommmunicationinterfacecommon.h.
    UaStatusCode getDeviceData(OpcUa_UInt32 type, const Identity& identity,
            OpcUa_UInt32 offset,
            UaVariant &value) { return OpcUa_BadNotImplemented; };

    /// @brief Temporary/unused, remove after reworking common/pascommmunicationinterfacecommon.h.
    UaStatusCode setDeviceState(OpcUa_UInt32 type, const Identity& identity, PASState state) { return OpcUa_BadNotImplemented; };

    /// @brief Temporary/unused, remove after reworking common/pascommmunicationinterfacecommon.h.
    UaStatusCode setDeviceData(OpcUa_UInt32 type, const Identity& identity,
            OpcUa_UInt32 offset,
            UaVariant value) { return OpcUa_BadNotImplemented; };

    /// @brief Temporary/unused, remove after reworking common/pascommmunicationinterfacecommon.h.
    UaStatusCode OperateDevice(OpcUa_UInt32 type, const Identity& identity,
            OpcUa_UInt32 offset = 0,
            const UaVariantArray& args = UaVariantArray()) { return OpcUa_BadNotImplemented; };

private:
    /// @brief Shared mutex used to control multi thread access to controller.
    UaMutex m_mutex;

    /// @brief Position number of the panel. Used for device database lookup.
    std::string m_panelNum;

    /// @brief Map from OPC UA device type to eAddress to a unique pointer to the controller object.
    std::map<OpcUa_UInt32, std::map<int, std::shared_ptr<PasController>>> m_pControllers;

    /// @brief Flag indicating that the internal thread should be stopped.
    /// @warning Unused.
    OpcUa_Boolean m_stop;

    /// @brief Pointer to the platform object used by all devices to interface with the hardware.
    std::shared_ptr<Platform> m_platform;
};

#endif
