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

//TODO refactor all the pNodeManager here so that operate() can fire event. Monitor this event with Client.

class PlatformBase;

/// @brief Class representing a generic device controller.
class PasController : public PasControllerCommon {
    UA_DISABLE_COPY(PasController); // Disables copy construction and copy assignment.
public:
    /// @brief Instantiate a PasController object with a Platform object.
    /// @param ID The device index within its type.
    /// @param pPlatform Pointer to the platform object used to interface with the hardware.
    /// @param updateInterval Update interval in milliseconds.
    explicit PasController(Device::Identity identity,
                           std::shared_ptr<PlatformBase> pPlatform = std::shared_ptr<PlatformBase>(nullptr),
                           int updateInterval = 0) : PasControllerCommon(std::move(identity), updateInterval),
                                                     m_pPlatform(std::move(pPlatform)) {}

    /// @brief Instantiate a PasController object without a Platform object.
    /// @param ID The device index within its type.
    /// @param updateInterval Update interval in milliseconds.
    PasController(Device::Identity identity, int updateInterval) : PasController(std::move(identity),
                                                                                 std::shared_ptr<PlatformBase>(nullptr),
                                                                                 updateInterval) {}

protected:
    /// @brief Pointer to the Platform object used to interface with hardware.
    std::shared_ptr<PlatformBase> m_pPlatform;
};

#endif //SERVER_PASCONTROLLER_HPP
