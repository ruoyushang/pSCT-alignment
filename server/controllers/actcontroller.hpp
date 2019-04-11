/**
 * @file actcontroller.hpp
 * @brief Header file for actuator device controller.
 */

#ifndef SERVER_CONTROLLERS_ACTCONTROLLER_HPP
#define SERVER_CONTROLLERS_ACTCONTROLLER_HPP

#include "server/controllers/pascontroller.hpp"

#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"

#include "common/alignment/platform.hpp"
#include "common/alignment/device.hpp"
#include "common/opcua/pascominterfacecommon.h"

/// @brief Class representing an actuator device controller.
class ActController : public PasController {
    UA_DISABLE_COPY(ActController); // Disables copy construction and copy assignment.
public:
    /// @brief Instantiate an actuator device controller object.
    /// @param ID The integer index of the device within its type.
    /// @param pPlatform Pointer to platform object used to interface directly with hardware.
    ActController(int ID, std::shared_ptr<Platform> pPlatform);

    /// @brief Destroy an actuator device controller object.
    ~ActController();

    /// @brief Update the controller's internal state to match the underlying Actuator object's state.
    /// @return OPC UA status code indicating success or failure.
    UaStatus updateState();

    /// @brief Get the internal state of the actuator device.
    /// @param state Variable to store the retrieved state value.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getState(Device::DeviceState &state);

    /// @brief Get the value of an actuator data variable.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Variable to store the retrieved data value.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value);

    /// @brief Get the value of an actuator error variable.
    /// @param offset A number used to uniquely identify the error variable to access.
    /// @param value Variable to store the retrieved error value.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getError(OpcUa_UInt32 offset, UaVariant &value);

    /// @brief Set the value of an actuator data variable.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Value to write to the selected data variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus setData(OpcUa_UInt32 offset, UaVariant value);

    /// @brief Set the value of an actuator error variable.
    /// @param offset A number used to uniquely identify the error variable to access.
    /// @param value Value to write to the selected error variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus setError(OpcUa_UInt32 offset, UaVariant value);

    /// @brief Call a method on the actuator device.
    /// @param offset A number used to uniquely identify the method to call.
    /// @param args Array of method arguments as UaVariants.
    /// @return OPC UA status code indicating success or failure.
    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args);

private:
    /// @brief The internal device state.
    Device::DeviceState m_state = Device::DeviceState::Off;
    /// @brief The distance between the current actuator length and the target length.
    OpcUa_Float m_DeltaL;

    /// @brief Move the panel a desired number of steps
    /// @param args Array of method arguments as UaVariants.
    UaStatus moveDelta(const UaVariantArray &args);
};

#endif //SERVER_CONTROLLERS_ACTCONTROLLER_HPP
