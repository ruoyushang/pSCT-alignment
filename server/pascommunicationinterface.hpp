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

    /// @brief Initialize all device controllers by reading from the database.
    /// @return A status code indicating success/failure.
    UaStatusCode Initialize();
    /// @brief Set the panel server IP used to retreive information from the database.
    /// @param serverIP External IP address of the panel/controller board.
    void setserverIP(const std::string& serverIP) { m_serverIP = serverIP; }

    /// @brief Get the number of found devices of a given type.
    std::size_t getDeviceCount(OpcUa_UInt32 deviceType);

    /// @brief Get the eAddresses of all existing devices of a given type.
    std::vector<int> getValidDeviceAddresses(OpcUa_UInt32 deviceType);

    /// @brief Get the PasController for a specific device.
    std::unique_ptr<PasController>& getDevice(OpcUa_UInt32 deviceType, int eAddress);

    /// @brief Get a device's state through its controller.
    UaStatusCode getDeviceState(
        OpcUa_UInt32 deviceType,
        int eAddress,
        PASState& state);

    /// @brief Get a device's data through its controller.
    UaStatusCode getDeviceData(
        OpcUa_UInt32 deviceType,
        int eAddress,
        OpcUa_UInt32 offset,
        UaVariant& value);

    /// @brief Set a device's state through its controller.
    UaStatusCode setDeviceState(
        OpcUa_UInt32 deviceType,
        int eAddress,
        PASState state);

    /// @brief Set a device's data through its controller.
    UaStatusCode setDeviceData(
        OpcUa_UInt32 deviceType,
        int eAddress,
        OpcUa_UInt32 offset,
        UaVariant value);

    /// @brief Operate a device through its controller.
    UaStatusCode OperateDevice(
        OpcUa_UInt32 deviceType,
        int eAddress,
        OpcUa_UInt32 offset = 0,
        const UaVariantArray& args = UaVariantArray());

    /// @brief Map of the OPC UA type ID for all supported device types to their device name.
    static const std::map<OpcUa_UInt32, std::string> deviceTypes;

    // Unused/temporary methods to match interface of virtual parent class
    
    /// @brief Get a device's name and identity information through its controller.
    /// Unused. Temporary, remove after reworking common/pascommmunicationinterfacecommon.h.
    UaStatusCode getDeviceConfig(
        OpcUa_UInt32 deviceType,
        OpcUa_UInt32 deviceIndex,
        UaString& sName,
        Identity& identity) { return OpcUa_BadNotImplemented; };

    OpcUa_Int32 getDevices(OpcUa_UInt32 deviceType) { return 0; };

    /* Get device status and data */
    UaStatusCode getDeviceState(OpcUa_UInt32 type, const Identity& identity, PASState& state) { return OpcUa_BadNotImplemented; };

    UaStatusCode getDeviceData(OpcUa_UInt32 type, const Identity& identity,
            OpcUa_UInt32 offset,
            UaVariant &value) { return OpcUa_BadNotImplemented; };

    /* Set device status and data*/
    UaStatusCode setDeviceState(OpcUa_UInt32 type, const Identity& identity, PASState state) { return OpcUa_BadNotImplemented; };

    UaStatusCode setDeviceData(OpcUa_UInt32 type, const Identity& identity,
            OpcUa_UInt32 offset,
            UaVariant value) { return OpcUa_BadNotImplemented; };

    UaStatusCode OperateDevice(OpcUa_UInt32 type, const Identity& identity,
            OpcUa_UInt32 offset = 0,
            const UaVariantArray& args = UaVariantArray()) { return OpcUa_BadNotImplemented; };



private:
    /// @brief Shared mutex used to control multi thread access to controller.
    UaMutex m_mutex;

    /// @brief IP address of the panel controller board. Used for device database lookup.
    std::string m_serverIP;

    /// @brief Map from OPC UA device type to eAddress to a unique pointer to the controller object.
    std::map<OpcUa_UInt32, std::map<int, std::shared_ptr<PasController>>> m_pControllers;

    /// @brief Flag indicating that the internal thread should be stopped.
    /// @warning Unused.
    OpcUa_Boolean m_stop;

    /// @brief Platform object used by all devices to interface directly with the hardware.
    std::shared_ptr<Platform> m_platform;
};

#endif
