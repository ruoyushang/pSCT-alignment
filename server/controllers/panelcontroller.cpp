#include "server/controllers/panelcontroller.hpp"

#include <array>
#include <iostream>
#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"

#include "common/opcua/passervertypeids.h"

// @details Sets the state to On.
PanelController::PanelController(int ID, std::shared_ptr<Platform> pPlatform) : PasController(ID,
                                                                                              std::move(pPlatform)) {
    m_state = Device::DeviceState::On;
}

// @details Sets the state to Off.
PanelController::~PanelController() {
    m_state = Device::DeviceState::Off;
}

// @details Locks the shared mutex while reading the state.
UaStatus PanelController::getState(Device::DeviceState &state) {
    UaMutexLocker lock(&m_mutex);
    updateState();
    state = m_state;
    return OpcUa_Good;
}

// @details Locks the shared mutex while updating the state.
UaStatus PanelController::updateState() {
    UaMutexLocker lock(&m_mutex);
    // update internal state to match the underlying platform object
    m_state = m_pPlatform->getState();
    return OpcUa_Good;
}

// @details Locks the shared mutex while setting the state. Only the On and Off states are allowed to be set manually.
UaStatus PanelController::setState(Device::DeviceState state) {
    UaMutexLocker lock(&m_mutex);

    switch (state) {
        case Device::DeviceState::On:
            m_state = state;
            m_pPlatform->turnOn();
            break;
        case Device::DeviceState::Off:
            m_state = state;
            m_pPlatform->turnOff();
            break;
        case Device::DeviceState::FatalError:
            return OpcUa_BadInvalidArgument;
        case Device::DeviceState::OperableError:
            return OpcUa_BadInvalidArgument;
        case Device::DeviceState::Busy:
            return OpcUa_BadInvalidArgument;
        default:
            return OpcUa_BadInvalidArgument;
    }

    return OpcUa_Good;
}

/// @details Locks the shared mutex while setting the state. In SIMMODE,
/// when reading temperature values, dummy values are generated from a normal distribution.
UaStatus PanelController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    UaMutexLocker lock(&m_mutex);

#ifdef SIMMODE
    std::random_device rd{};
    std::mt19937 generator{rd()};

    std::normal_distribution<double> internalTempDistribution(30.0, 2.0);
    std::normal_distribution<double> externalTempDistribution(20.0, 2.0);

    double newValue;
    if (offset == PAS_PanelType_ExtTemperature) {
        newValue = externalTempDistribution(generator);
        value.setFloat(newValue);
    } else if (offset == PAS_PanelType_IntTemperature) {
        newValue = internalTempDistribution(generator);
        value.setFloat(newValue);
    } else {
        return OpcUa_BadInvalidArgument;
    }

#else
    if (offset == PAS_PanelType_ExtTemperature) {
        value.setFloat(m_pPlatform->ReadExternalTemperature());
    }
    else if (offset == PAS_PanelType_IntTemperature) {
        value.setFloat(m_pPlatform->ReadInternalTemperature());
    }
    else {
        return OpcUa_BadInvalidArgument;
    }
#endif
    return OpcUa_Good;
}

/// @details This method does nothing as panels have no writeable variables.
UaStatus PanelController::setData(OpcUa_UInt32 offset, UaVariant value) {
    return OpcUa_BadNotImplemented;
}

/// @details Updates the state and then checks it before attempting to call any methods. When the state is FatalError or Busy, prevents panel operation.
/// If the stop method is called, sets the platform state to off in order to halt any motion.
UaStatus PanelController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    UaStatus status;

    updateState(); // Update the current state

    if (m_state == Device::DeviceState::FatalError) {
        std::cout << "PasPanel::operate(): Panel in fatal error state! "
                  << "Check what's wrong, fix it, and try again.\n";
        return OpcUa_BadInvalidState;
    }

    if (offset == PAS_PanelType_StepAll) {
        if (m_state == Device::DeviceState::Off)
            setState(Device::DeviceState::On);
        else if (m_state == Device::DeviceState::Busy) {
            std::cout << "PanelController::operate(): Busy at the moment. "
                      << "Wait for the current operation to finish and try again.\n";
            return OpcUa_Good;
        }

        m_state = Device::DeviceState::Busy; // set the state immadiately

        std::array<float, 6> deltaLengths{};

        UaVariant vTemp;
        for (int i = 0; i < 6; i++) {
            m_pActuators.at(i)->getData(PAS_ACTType_Steps, vTemp);
            vTemp.toFloat(deltaLengths[i]);
        }
        deltaLengths = m_pPlatform->moveDeltaLengths(deltaLengths);
        // update missed lengths
        for (int i = 0; i < 6; i++) {
            vTemp.setFloat(deltaLengths[i]);
            m_pActuators.at(i)->setData(PAS_ACTType_Steps, vTemp);
        }

        status = OpcUa_Good;
    } else if (offset == PAS_PanelType_MoveTo_Acts) {
        if (m_state == Device::DeviceState::Off)
            setState(Device::DeviceState::On);
        else if (m_state == Device::DeviceState::Busy) {
            std::cout << "PanelController::operate(): Busy at the moment. "
                      << "Wait for the current operation to finish and try again.\n";
            return OpcUa_Good;
        }

        m_state = Device::DeviceState::Busy; // set the state immeadiately

        std::array<float, 6> lengths{};
        UaVariant vTemp;
        for (int i = 0; i < 6; i++) {
            m_pActuators.at(i)->getData(PAS_ACTType_inLength_mm, vTemp);
            vTemp.toFloat(lengths[i]);
        }
        lengths = m_pPlatform->moveToLengths(lengths);

        status = OpcUa_Good;
    } else if (offset == PAS_PanelType_Stop) {
        std::cout << "PanelController::operate(): Attempting to gracefully stop the motion.\n";
        status = setState(Device::DeviceState::Off);
    } else
        status = OpcUa_BadInvalidArgument;

    updateState(); // update the state again

    return status;
}
