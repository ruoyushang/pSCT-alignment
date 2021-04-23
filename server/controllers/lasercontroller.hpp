/**
 * @file lasercontroller.hpp
 * @brief Header file for position sensitive device controller class
 */

#ifndef SERVER_LASERCONTROLLER_HPP
#define SERVER_LASERCONTROLLER_HPP

#include <memory>

#include "uabase/statuscode.h"
#include "uaserver/uaobjecttypes.h"

#include "common/alignment/device.hpp"
#include "common/globalalignment/laserclass.h"
#include "common/alignment/platform.hpp"

#include "server/controllers/pascontroller.hpp"


/// @brief Class representing a device controller for a Position Sensitive Device.
class LaserController : public PasController
{
    UA_DISABLE_COPY(LaserController);

public:
    /// @brief Instantiate a position sensitive device controller.
    /// @param ID The device ID (index) used to identify it within its parent panel/platform.
    LaserController(Device::Identity identity, std::shared_ptr<PlatformBase> pPlatform);

    /// @brief Initialize the Laser.
    /// #return 0 on success, -1 on failure.
    bool initialize() override;

    /// @brief Get the device's state.
    /// @param state Variable to store the retrieved state value.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getState(Device::DeviceState &state) override;

    /// @brief Set the device's state.
    /// @param state Value to set the device state to.
    /// @return OPC UA status code indicating success or failure.
    UaStatus setState(Device::DeviceState state) override;

    /// @brief Get the value of a Laser data variable.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Variable to store the retrieved data value.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    /// @brief Set the value of a Laser data variable.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Value to write to the selected data variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus getError(OpcUa_UInt32 offset, UaVariant &value);

    /// @brief Call a method on the Laser device.
    /// @param offset A number used to uniquely identify the method to call.
    /// @param args Array of method arguments as UaVariants.
    /// @return OPC UA status code indicating success or failure.
    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args) override;

private:
    Device::ErrorState _getErrorState() { return Device::ErrorState::Nominal; }

    Device::DeviceState _getDeviceState() { return Device::DeviceState::On; }
    /// @brief Pointer to the GASLaser object interfacing directly with thee Laser hardware.

    /// @brief Read all sensors and update internal data variables
    /// @return OPC UA status code indicating success or failure.
    UaStatus read();
};

#endif //SERVER_LASERCONTROLLER_HPP
