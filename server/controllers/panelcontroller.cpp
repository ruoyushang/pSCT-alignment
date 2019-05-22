#include "server/controllers/panelcontroller.hpp"

#include <array>
#include <iostream>
#include <memory>
#include <random>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"

#include "common/opcua/passervertypeids.hpp"
#include "common/opcua/pasobject.hpp"

#include "common/alignment/platform.hpp"

#include "server/objects/panelobject.hpp"

#include "server/controllers/actcontroller.hpp"
#include "server/controllers/mpescontroller.hpp"


/// @details Locks the shared mutex while retrieving the state.
UaStatus PanelController::getState(Device::DeviceState &state) {
    //UaMutexLocker lock(&m_mutex);
    state = _getState();
    return OpcUa_Good;
}

/// @details Does not allow setting the state to error or setting the state to
/// its current value. Locks the shared mutex while setting the state.
UaStatus PanelController::setState(Device::DeviceState state) {
    UaMutexLocker lock(&m_mutex);

    switch (state) {
        case Device::DeviceState::On:
            m_pPlatform->turnOn();
            break;
        case Device::DeviceState::Off:
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
    //UaMutexLocker lock(&m_mutex);

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
    if (PanelObject::VARIABLES.find(offset) != PanelObject::VARIABLES.end()) {
        if (offset == PAS_PanelType_ExtTemperature) {
            value.setFloat(m_pPlatform->getExternalTemperature());
        } else if (offset == PAS_PanelType_IntTemperature) {
            value.setFloat(m_pPlatform->getInternalTemperature());
        } else if (offset == PAS_PanelType_Position) {
        value.setInt32(m_ID.position);
    } else if (offset == PAS_PanelType_Serial) {
        value.setInt32(m_ID.serialNumber);
    }
    } else if (PanelObject::ERRORS.find(offset) != PanelObject::ERRORS.end()) {
        return getError(offset, value);
    }
    else {
        return OpcUa_BadInvalidArgument;
    }
#endif
    return OpcUa_Good;
}

UaStatus PanelController::getError(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;
    bool errorStatus;

    OpcUa_UInt32 errorNum = offset - PAS_PanelType_Error0;
    if (errorNum >= 0 && errorNum < PanelObject::ERRORS.size()) {
        errorStatus = m_pPlatform->getError(int(errorNum));
        value.setBool(errorStatus);
    } else {
        status = OpcUa_BadInvalidArgument;
    }
    return status;
}

/// @details This method does nothing as panels have no writeable variables.
UaStatus PanelController::setData(OpcUa_UInt32 offset, UaVariant value) {
    return OpcUa_BadNotImplemented;
}

/// @details Updates the state and then checks it before attempting to call any methods. When the state is FatalError or Busy, prevents panel operation.
/// If the stop method is called, sets the platform state to off in order to halt any motion.
UaStatus PanelController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    UaStatus status;

    Device::DeviceState state;
    getState(state);

    if (offset == PAS_PanelType_MoveToLengths || offset == PAS_PanelType_MoveToCoords || offset == PAS_PanelType_MoveDeltaLengths || offset == PAS_PanelType_FindHome) {
        if (m_state == Device::DeviceState::FatalError) {
            std::cout << "PanelController::operate(): Panel in fatal error state! "
                  << "Check what's wrong, fix it, and try again.\n";
            return OpcUa_BadInvalidState;
        }
        if (m_state == Device::DeviceState::Busy) {
            std::cout << "PanelController::operate() : Current state is 'Busy'! This won't do anything." << std::endl;
            return OpcUa_BadInvalidState;
        }
    }



    if (state == Device::DeviceState::FatalError) {
    }

    if (offset == PAS_PanelType_MoveDeltaLengths) {
        if (args.length() != 6) {
            return OpcUa_BadInvalidArgument;
        }
        if (_getState() == Device::DeviceState::Off)
            setState(Device::DeviceState::On);
        else if (_getState() == Device::DeviceState::Busy) {
            std::cout << "PanelController::operate(): Busy at the moment. "
                      << "Wait for the current operation to finish and try again.\n";
            return OpcUa_BadInvalidState;
        }

        setState(Device::DeviceState::Busy);

        std::array<float, 6> deltaLengths{};

        std::cout << "Received MoveDeltaLengths command with delta lengths: " << std::endl;
        UaVariant dL;
        float targetLength;
        for (int i = 0; i < 6; i++) {
            dL = UaVariant(args[i]);
            dL.toFloat(deltaLengths[i]);
            status = m_pActuators.at(i)->getData(PAS_ACTType_CurrentLength, dL);
            dL.toFloat(targetLength);
            targetLength += deltaLengths[i];
            m_pActuators.at(i)->setTargetLength(targetLength);
            std::cout << deltaLengths[i] << std::endl;
        }
        std::cout << std::endl << std::endl;

        deltaLengths = m_pPlatform->moveDeltaLengths(deltaLengths);
        // update missed lengths
        std::cout << "Missed targets by: " << std::endl;
        float remainingLength;
        for (int i = 0; i < 6; i++) {
            m_pActuators.at(i)->setDeltaLength(deltaLengths[i]);
            status = m_pActuators.at(i)->getData(PAS_ACTType_DeltaLength, dL);
            dL.toFloat(remainingLength);
            std::cout << remainingLength << std::endl;
        }
        std::cout << std::endl;
        status = OpcUa_Good;
    } else if (offset == PAS_PanelType_MoveToLengths) {
        if (args.length() != 6) {
            return OpcUa_BadInvalidArgument;
        }
        if (_getState() == Device::DeviceState::Off)
            setState(Device::DeviceState::On);
        else if (_getState() == Device::DeviceState::Busy) {
            std::cout << "PanelController::Operate(): Busy at the moment. "
                      << "Wait for the current operation to finish and try again.\n";
            return OpcUa_Good;
        }

        setState(Device::DeviceState::Busy); // set the state immeadiately

        std::array<float, 6> lengths{};
        UaVariant len;
        for (int i = 0; i < 6; i++) {
            len = UaVariant(args[i]);
            len.toFloat(lengths[i]);
            m_pActuators.at(i)->setTargetLength(lengths[i]);
            std::cout << lengths[i] << std::endl;
        }
        std::cout << std::endl << std::endl;

        lengths = m_pPlatform->moveToLengths(lengths);

        float l;
        std::cout << "Missed targets by: " << std::endl;
        for (int i = 0; i < 6; i++) {
            m_pActuators.at(i)->setDeltaLength(lengths[i]);
            status = m_pActuators.at(i)->getData(PAS_ACTType_DeltaLength, len);
            len.toFloat(l);
            std::cout << l << std::endl;
        }
        std::cout << std::endl;

        status = OpcUa_Good;
    } else if (offset == PAS_PanelType_Stop) {
        std::cout << "PanelController::operate(): Attempting to gracefully stop the motion.\n";
        status = setState(Device::DeviceState::Off);
    } else if (offset == PAS_PanelType_FindHome) {
        if (args.length() != 1) {
            return OpcUa_BadInvalidArgument;
        }
        m_pPlatform->findHomeFromEndStopAll(args[0].Value.Int32);
    } else if (offset == PAS_PanelType_ClearError) {
        if (args.length() != 1) {
            return OpcUa_BadInvalidArgument;
        }
        m_pPlatform->unsetError(args[0].Value.Int32);
    } else if (offset == PAS_PanelType_ClearAllErrors) {
        m_pPlatform->clearErrors();
    } else if (offset == PAS_PanelType_ClearActuatorErrors) {
        m_pPlatform->clearActuatorErrors();
    } else if (offset == PAS_PanelType_ClearPlatformErrors) {
        m_pPlatform->clearPlatformErrors();
    } else {
        status = OpcUa_BadInvalidArgument;
    }

    return status;
}

void PanelController::addActuator(const std::shared_ptr<ActController> &pActuator) {
    m_pActuators.push_back(pActuator);
}

void PanelController::addMPES(const std::shared_ptr<MPESController> &pMPES) {
    m_pMPES.push_back(pMPES);
}
