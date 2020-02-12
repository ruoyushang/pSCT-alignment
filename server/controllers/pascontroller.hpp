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
#include "server/pasnodemanager.hpp"

//TODO refactor all the pNodeManager here so that operate() can fire event. Monitor this event with Client.

class PlatformBase;
class PasServerNodeManager;

/// @brief Class representing a generic device controller.
class PasControllerServer : public PasControllerCommon {
    UA_DISABLE_COPY(PasControllerServer); // Disables copy construction and copy assignment.
public:
    /// @brief Instantiate a PasControllerServer object with a Platform object.
    /// @param ID The device index within its type.
    /// @param pPlatform Pointer to the platform object used to interface with the hardware.
    /// @param pNodeManager Pointer to the pNodeManager object used to interface with the server.
    /// @param pCommIf Pointer to the pCommIf object used to communicate with the device.
    /// @param updateInterval Update interval in milliseconds.
    explicit PasControllerServer(Device::Identity identity,
                                 std::shared_ptr<PlatformBase> pPlatform = std::shared_ptr<PlatformBase>(nullptr),
                                 const std::shared_ptr<PasServerNodeManager>& pNodeManager = std::shared_ptr<PasServerNodeManager>(),
                                 const std::shared_ptr<PasServerCommunicationInterface>& pCommIf = std::shared_ptr<PasServerCommunicationInterface>(),
                                 int updateInterval = 0) : PasControllerCommon(std::move(identity), updateInterval),
                                                     m_pPlatform(std::move(pPlatform)) {}

    /// @brief Instantiate a PasControllerServer object without a Platform object.
    /// @param ID The device index within its type.
    /// @param updateInterval Update interval in milliseconds.
    PasControllerServer(Device::Identity identity, int updateInterval) : PasControllerServer(std::move(identity),
                                                                                             std::shared_ptr<PlatformBase>(nullptr),
                                                                                             std::shared_ptr<PasServerNodeManager>(),
                                                                                             std::shared_ptr<PasServerCommunicationInterface>(),
                                                                                             updateInterval) {}

protected:
    /// @brief Pointer to the Platform object used to interface with hardware.
    std::shared_ptr<PlatformBase> m_pPlatform;
    std::shared_ptr<PasServerNodeManager> m_pNodeManager;
    std::shared_ptr<PasServerCommunicationInterface> m_pCommIf;

//private:
//#if SUPPORT_Event_Subscription_Server_Facet
//    OpcUa::OffNormalAlarmType* m_pStateOffNormalAlarm;
//#endif // SUPPORT_Event_Subscription_Server_Facet
};

#endif //SERVER_PASCONTROLLER_HPP
