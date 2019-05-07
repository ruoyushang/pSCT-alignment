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

#include "common/opcua/pascominterfacecommon.hpp"


class Platform;

/// @brief Class representing a mirror panel edge sensor device controller.
class MPESController : public PasController {
    UA_DISABLE_COPY(MPESController); // Disables copy construction and copy assignment
public:
    /// @brief Instantiate an MPES device controller object.
    /// @param ID The integer index of the device within its type.
    /// @param pPlatform Platform object used to interface with hardware.
    MPESController(Identity identity, std::shared_ptr<Platform> pPlatform);

    /// @brief Destroy an MPES device controller object.
    ~MPESController() override;

    /// @brief Initialize the MPES by setting its exposure.
    /// #return 0 on success, -1 on failure.
    bool initialize() override;

    /// @brief Get the value of an MPES data variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    /// @brief Set the value of an MPES data variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    /// @brief Call a method on the MPES device.
    /// @return OPC UA status code indicating success or failure.
    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args) override;

private:
    Device::DeviceState _getState() { return m_pPlatform->getMPES(m_ID)->getState(); }

    /// @brief Whether the MPES data has been read at least once since initialization.
    bool m_updated = false;

    /// @brief Update the MPES position data.
    UaStatus read();
};

#endif //SERVER_MPESCONTROLLER_HPP
