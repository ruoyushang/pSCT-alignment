/**
 * @file panelcontroller.hpp
 * @brief Header file for panel device controller class.
 */

#ifndef SERVER_PANELCONTROLLER_HPP
#define SERVER_PANELCONTROLLER_HPP

#include <memory>
#include <vector>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include "common/alignment/device.hpp"
#include "common/opcua/pascominterfacecommon.hpp"

#include "server/controllers/actcontroller.hpp"
#include "server/controllers/pascontroller.hpp"
#include "server/controllers/mpescontroller.hpp"
#include "server/controllers/psdcontroller.hpp"

class ActController;

class MPESController;

class Platform;

/// @brief Class representing a panel device controller.
class PanelController : public PasController {
    UA_DISABLE_COPY(PanelController); // Disables copy construction and copy assignment.
public:
    /// @brief Instantiate a panel device controller object.
    /// @param ID The integer index of the device within its type.
    /// @param pPlatform Pointer to platform object used to interface directly with hardware.
    PanelController(Device::Identity identity, std::shared_ptr<PlatformBase> pPlatform) : PasController(
        std::move(identity),
        std::move(
                                                                                                        pPlatform)) {}

    /// @brief Get the device's state.
    /// @param state Variable to store the retrieved state value.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getState(Device::DeviceState &state) override;

    /// @brief Set the device's state.
    /// @param state Value to set the device state to.
    /// @return OPC UA status code indicating success or failure.
    UaStatus setState(Device::DeviceState state) override;

    /// @brief Get the value of a panel data variable.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Variable to store the retrieved data value.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    UaStatus getError(OpcUa_UInt32 offset, UaVariant &value);

    /// @brief Set the value of an panel data variable.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Value to write to the selected data variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    /// @brief Call a method on the panel device.
    /// @param offset A number used to uniquely identify the method to call.
    /// @param args Array of method arguments as UaVariants.
    /// @return OPC UA status code indicating success or failure.
    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args) override;

    /// @brief Add an actuator controller as a child of this panel controller.
    /// @param pActuator A pointer to an actuator controller.
    void addActuator(const std::shared_ptr<ActController> &pActuator);

    /// @brief Add an MPES controller as a child of this panel controller.
    /// @param pActuator A pointer to an MPES controller.
    void addMPES(const std::shared_ptr<MPESController> &pMPES);

    void addPSD(const std::shared_ptr<PSDController> &pPSD);

private:
    Device::ErrorState _getErrorState() { return m_pPlatform->getErrorState(); }

    Device::DeviceState _getDeviceState() { return m_pPlatform->getDeviceState(); }

    /// @brief A vector of all actuator controllers belonging to this panel.
    std::vector<std::shared_ptr<ActController>> m_pActuators;

    /// @brief A vector of all MPES controllers belonging to this panel.
    std::vector<std::shared_ptr<MPESController>> m_pMPES;

    std::vector<std::shared_ptr<PSDController>> m_pPSD;
};

#endif //SERVER_PANELCONTROLLER_HPP
