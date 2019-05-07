/**
 * @file pascontroller.hpp
 * @brief Header file for generic device controller class.
 */
#ifndef SERVER_PASCONTROLLER_HPP
#define SERVER_PASCONTROLLER_HPP

#include <chrono>
#include <map>
#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include "common/opcua/pascominterfacecommon.hpp"
#include "common/opcua/pascontrollercommon.hpp"
#include "common/alignment/device.hpp"

class Platform;

/// @brief Class representing a generic device controller.
class PasController : public PasControllerCommon {
    UA_DISABLE_COPY(PasController); // Disables copy construction and copy assignment.
public:
    /// @brief Instantiate a PasController object with a Platform object.
    /// @param ID The device index within its type.
    /// @param pPlatform Pointer to the platform object used to interface with the hardware.
    /// @param updateInterval Update interval in milliseconds.
    explicit PasController(Device::Identity identity,
                           std::shared_ptr<Platform> pPlatform = std::shared_ptr<Platform>(nullptr),
                           int updateInterval = 0) : PasControllerCommon(identity, updateInterval), m_pPlatform(std::move(pPlatform)) {}

    /// @brief Instantiate a PasController object without a Platform object.
    /// @param ID The device index within its type.
    /// @param updateInterval Update interval in milliseconds.
    PasController(Device::Identity identity, int updateInterval) : PasController(identity,
                                                                                 std::shared_ptr<Platform>(nullptr),
                                                                                 updateInterval) {}

    PasController(Device::Identity identity, int updateInterval) : PasController(identity,
                                                                                 std::shared_ptr<Platform>(nullptr),
                                                                                 updateInterval) {}

    /// @brief Initialize the controller object.
    /// @return 0 on success, -1 on failure.
    virtual bool initialize() { return true; } // only MPES overload this for now
    /// @brief Get the device ID/index.
    /// @return The device ID.
    Device::Identity getID() { return m_ID; }

    /// @brief Destroy a controller object.
    virtual ~PasController() {}

    /// @brief Get the device's state.
    /// @param state Variable to store the retrieved state value.
    /// @return OPC UA status code indicating success or failure.
    virtual UaStatus getState(Device::DeviceState &state) = 0;

    /// @brief Get data from a device variable.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Variable to store the retrieved data value.
    /// @return OPC UA status code indicating success or failure.
    virtual UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) = 0; // Pure virtual function

    /// @brief Set the device's state.
    /// @param state Value to set the device state to.
    /// @return OPC UA status code indicating success or failure.
    virtual UaStatus setState(Device::DeviceState state) = 0;

    /// @brief Set a device data variable.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Value to write to the selected data variable.
    /// @return OPC UA status code indicating success or failure.
    virtual UaStatus setData(OpcUa_UInt32 offset, UaVariant value) = 0; // Pure virtual function
    /// @brief Call a device method.
    /// @param offset A number used to uniquely identify the method to call.
    /// @param args Array of method arguments as UaVariants.
    /// @return OPC UA status code indicating success or failure.
    virtual UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args) = 0; // Pure virtual function

protected:
    /// @brief Pointer to the Platform object used to interface with hardware.
    std::shared_ptr<Platform> m_pPlatform;
};

#endif //SERVER_PASCONTROLLER_HPP
