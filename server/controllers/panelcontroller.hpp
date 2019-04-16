/**
 * @file panelcontroller.hpp
 * @brief Header file for panel device controller class.
 */

#ifndef __PANELCONTROLLER_H__
#define __PANELCONTROLLER_H__

#include <memory>
#include <vector>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include "common/opcua/pascominterfacecommon.h"

#include "server/controllers/actcontroller.hpp"
#include "server/controllers/pascontroller.hpp"

/// @brief Class representing a panel device controller.
class PanelController : public PasController {
    UA_DISABLE_COPY(PanelController); // Disables copy construction and copy assignment.
public:
    /// @brief Instantiate a panel device controller object.
    /// @param ID The integer index of the device within its type.
    /// @param pPlatform Pointer to platform object used to interface directly with hardware.
    PanelController(int ID, std::shared_ptr<Platform> pPlatform);

    /// @brief Destroy a panel device controller object.
    ~PanelController();

    /// @brief Get the internal state of the panel device.
    /// @param state Variable to store the retrieved state value.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getState(PASState &state);

    /// @brief Get the value of a panel data variable.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Variable to store the retrieved data value.
    /// @return OPC UA status code indicating success or failure.
    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value);

    /// @brief Set the internal state of the panel device.
    /// @param state Value to set the state to.
    /// @return OPC UA status code indicating success or failure.
    UaStatus setState(PASState state);

    /// @brief Set the value of an panel data variable.
    /// @param offset A number used to uniquely identify the data variable to access.
    /// @param value Value to write to the selected data variable.
    /// @return OPC UA status code indicating success or failure.
    UaStatus setData(OpcUa_UInt32 offset, UaVariant value);

    /// @brief Call a method on the panel device.
    /// @param offset A number used to uniquely identify the method to call.
    /// @param args Array of method arguments as UaVariants.
    /// @return OPC UA status code indicating success or failure.
    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args);

    /// @brief Update the controller's internal state to match the underlying Platform object's state.
    /// @return OPC UA status code indicating success or failure.
    UaStatus updateState();

    /// @brief Add an actuator controller as a child of this panel controller.
    /// @param pActuator A pointer to an actuator controller.
    void addActuator(std::shared_ptr<ActController> pActuator) { m_pActuators.push_back(pActuator); };

private:
    /// @brief The internal state of the panel device.
    PASState m_state = PASState::Off;

    /// @brief A vector of all actuator controllers belonging to this panel.
    std::vector<std::shared_ptr<ActController>> m_pActuators;
};

#endif
