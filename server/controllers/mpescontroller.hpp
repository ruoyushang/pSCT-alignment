/**
 * @file MPEScontroller.hpp
 * @brief Header file for mirror panel edge sensor device controller.
 */

#ifndef SERVER_MPESCONTROLLER_HPP
#define SERVER_MPESCONTROLLER_HPP

#include "server/controllers/pascontroller.hpp"

#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"

#include "common/alignment/device.hpp"
#include "common/alignment/platform.hpp"

#include "common/opcua/pascominterfacecommon.hpp"


/// @brief Class representing a mirror panel edge sensor device controller.
class MPESController : public PasController {
    UA_DISABLE_COPY(MPESController); // Disables copy construction and copy assignment
public:
    /// @brief Instantiate an MPES device controller object.
    /// @param ID The integer index of the device within its type.
    /// @param pPlatform Platform object used to interface with hardware.
    MPESController(Device::Identity identity, std::shared_ptr<PlatformBase> pPlatform);

    /// @brief Initialize the MPES by setting its exposure.
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

    /// @brief Get the value of an MPES data variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    UaStatus getError(OpcUa_UInt32 offset, UaVariant &value);

    /// @brief Set the value of an MPES data variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    /// @brief Call a method on the MPES device.
    /// @return OPC UA status code indicating success or failure.
    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args) override;

private:
    Device::ErrorState _getErrorState() { return m_pPlatform->getMPESbyIdentity(m_Identity)->getErrorState(); }

    Device::DeviceState _getDeviceState() { return m_pPlatform->getMPESbyIdentity(m_Identity)->getDeviceState(); }

    /// @brief Update the MPES position data.
    UaStatus read();
};

#endif //SERVER_MPESCONTROLLER_HPP
