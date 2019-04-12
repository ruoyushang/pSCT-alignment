/**
 * @file panelcontroller.hpp
 * @brief Header file for panel device controller class.
 */

#ifndef SERVER_CONTROLLERS_PANELCONTROLLER_HPP
#define SERVER_CONTROLLERS_PANELCONTROLLER_HPP

#include <memory>
#include <vector>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include "common/alignment/device.hpp"
#include "server/controllers/pascontroller.hpp"

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
    PanelController(int ID, std::shared_ptr<Platform> pPlatform) : PasController(ID, std::move(pPlatform)) {}

    /// @brief Destroy a panel device controller object.
    ~PanelController() {}

    /// @brief Get the device's state.
    /// @param state Variable to store the retrieved state value.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getState(Device::DeviceState &state);

    /// @brief Set the device's state.
    /// @param state Value to set the device state to.
    /// @return OPC UA status code indicating success or failure.
    UaStatus setState(Device::DeviceState state);

    /// @brief Get the value of a panel data variable.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Variable to store the retrieved data value.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

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

private:
    Device::DeviceState _getState() { return m_pPlatform->getState(); }

    /// @brief A vector of all actuator controllers belonging to this panel.
    std::vector<std::shared_ptr<ActController>> m_pActuators;

    /// @brief A vector of all MPES controllers belonging to this panel.
    std::vector<std::shared_ptr<MPESController>> m_pMPES;
};

#endif //SERVER_CONTROLLERS_PANELCONTROLLER_HPP
