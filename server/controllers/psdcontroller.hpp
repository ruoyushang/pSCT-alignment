/**
 * @file psdcontroller.hpp
 * @brief Header file for position sensitive device controller class
 */
 
#ifndef _AMD64

#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uaobjecttypes.h"

class GASPSD;

/// @brief Controller class for a Position Sensitive Device
class PSDController : public PasController
{
    UA_DISABLE_COPY(PSDController);
public:
    /// @brief Instantiate a PSDController object.
    /// @param ID device ID.
    PSDController(int ID);
    /// @brief Destroy a PSDController object.
    ~PSDController();

    /// @brief Get the value of a PSD data variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getData(OpcUa_UInt32 offset, UaVariant& value);

    /// @brief Set the value of a PSD data variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus setData(OpcUa_UInt32 offset, UaVariant value);
    /// @brief Call a method on the PSD device.
    /// @return OPC UA status code indicating success or failure.
    UaStatus Operate(OpcUa_UInt32 offset, const UaVariantArray& args);

private:
    /// @brief The internal device state (on/off/busy).
    PASState m_state = PASState::PAS_Off;
    /// @brief Pointer to an interface object to interact with the PSD hardware.
    std::unique_ptr<GASPSD> m_pPSD;

    /// @brief Time interval after which internal data is considered to be expired.
    const int kUpdateInterval_ms;
    /// @brief Read all sensors and update internal data variables
    /// @return OPC UA status code indicating success or failure..
    UaStatus read();
};

#endif
