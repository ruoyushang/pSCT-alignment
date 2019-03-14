/**
 * @file actcontroller.hpp
 * @brief Header file for actuator device controller.
 */

#ifndef __ACTCONTROLLER_H__
#define __ACTCONTROLLER_H__

#include "server/controllers/pascontroller.hpp"

#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"

#include "common/alignment/platform.hpp"
#include "common/opcua/pascominterfacecommon.h"

/// @brief Controller class for an actuator device.
class ActController : public PasController
{
    UA_DISABLE_COPY(ActController); // Disables copy construction and copy assignment.
public:
    /// @brief Instantiate an actuator device controller object.
    /// @param ID The integer index of the device within its type.
    /// @param pPlatform Platform object used to interface with hardware.
    ActController(int ID, std::shared_ptr<Platform> pPlatform);
    /// @brief Destroy an actuator device controller object.
    ~ActController();

    UaStatus updateState();

    /// @brief Set the internal state of an actuator data variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getState(PASState &state);
    /// @brief Get the value of an actuator data variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getData(OpcUa_UInt32 offset, UaVariant& value);
    /// @brief Get the value of an actuator error variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getError(OpcUa_UInt32 offset, UaVariant& value);

    /// @brief Set the internal state of an actuator data variable.
    /// @return OPC UA status code indicating success or failure.
    //UaStatus setState(PASState state);
    /// @brief Set the value of an actuator data variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus setData(OpcUa_UInt32 offset, UaVariant value);
    /// @brief Set the value of an actuator error variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus setError(OpcUa_UInt32 offset, UaVariant value);
    /// @brief Call a method on the actuator device.
    /// @return OPC UA status code indicating success or failure.
    UaStatus Operate(OpcUa_UInt32 offset, const UaVariantArray& args);

private:
    /// @brief Device state.
    PASState m_state = PASState::Off;
    /// @brief
    OpcUa_Float m_DeltaL;
    /// @brief
    OpcUa_Float m_inLength;

    /// @brief
    UaStatus moveDelta(const UaVariantArray& args);
};

#endif
