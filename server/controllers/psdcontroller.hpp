/**
 * @file psdcontroller.hpp
 * @brief Header file for position sensitive device controller class
 */

#ifndef __PSDCONTROLLER_H__
#define __PSDCONTROLLER_H__

#include <memory>

#include "uabase/statuscode.h"
#include "uaserver/uaobjecttypes.h"

class GASPSD;

/// @brief Class representing a device controller for a Position Sensitive Device.
class PSDController : public PasController {
    UA_DISABLE_COPY(PSDController);

public:
    /// @brief Instantiate a position sensitive device controller.
    /// @param ID The device ID (index) used to identify it within its parent panel/platform.
    PSDController(int ID);

    /// @brief Destroy a position sensitive device controller.
    ~PSDController();

    /// @brief Get the value of a PSD data variable.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Variable to store the retrieved data value.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value);

    /// @brief Set the value of a PSD data variable.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Value to write to the selected data variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus setData(OpcUa_UInt32 offset, UaVariant value);

    /// @brief Call a method on the PSD device.
    /// @param offset A number used to uniquely identify the method to call.
    /// @param args Array of method arguments as UaVariants.
    /// @return OPC UA status code indicating success or failure.
    UaStatus Operate(OpcUa_UInt32 offset, const UaVariantArray &args);

private:
    /// @brief The internal device state.
    PASState m_state = PASState::Off;
    /// @brief Pointer to the GASPSD object interfacing directly with thee PSD hardware.
    std::unique_ptr<GASPSD> m_pPSD;

    /// @brief Time interval after which internal data is considered to be expired.
    const int kUpdateInterval_ms;

    /// @brief Read all sensors and update internal data variables
    /// @return OPC UA status code indicating success or failure.
    UaStatus read();
};

#endif
