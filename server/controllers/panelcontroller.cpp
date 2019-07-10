#include "server/controllers/panelcontroller.hpp"

#include <array>

#include <memory>

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

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

/// @details Locks the shared mutex while retrieving the state.
UaStatus PanelController::getState(Device::DeviceState &state) {
    //UaMutexLocker lock(&m_mutex);
    state = _getDeviceState();
    spdlog::trace("{} : Read device state => ({})", m_Identity, Device::deviceStateNames.at(state));
    return OpcUa_Good;
}

/// @details Does not allow setting the state to error or setting the state to
/// its current value. Locks the shared mutex while setting the state.
UaStatus PanelController::setState(Device::DeviceState state) {
    return OpcUa_BadNotWritable;
}

/// @details Locks the shared mutex while setting the state. In SIMMODE,
/// when reading temperature values, dummy values are generated from a normal distribution.
UaStatus PanelController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    if (PanelObject::VARIABLES.find(offset) != PanelObject::VARIABLES.end()) {
        if (offset == PAS_PanelType_ExtTemperature) {
            float externalTemperature = m_pPlatform->getExternalTemperature();
            spdlog::trace("{} : Read externalTemp value => ({})", m_Identity, externalTemperature);
            value.setFloat(externalTemperature);
        } else if (offset == PAS_PanelType_IntTemperature) {
            float internalTemperature = m_pPlatform->getInternalTemperature();
            spdlog::trace("{} : Read externalTemp value => ({})", m_Identity, internalTemperature);
            value.setFloat(internalTemperature);
        } else if (offset == PAS_PanelType_Position) {
            spdlog::trace("{} : Read Position value => ({})", m_Identity, m_Identity.position);
            value.setInt32(m_Identity.position);
        } else if (offset == PAS_PanelType_Serial) {
            spdlog::trace("{} : Read Serial value => ({})", m_Identity, m_Identity.serialNumber);
            value.setInt32(m_Identity.serialNumber);
        } else if (offset == PAS_PanelType_ErrorState) {
            Device::ErrorState errorState = _getErrorState();
            spdlog::trace("{} : Read ErrorState value => ({})", m_Identity, static_cast<int>(errorState));
            value.setInt32(static_cast<int>(errorState));
        }
    } else if (PanelObject::ERRORS.find(offset) != PanelObject::ERRORS.end()) {
        return getError(offset, value);
    }
    else {
        return OpcUa_BadInvalidArgument;
    }
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
        spdlog::trace("{} : Read error {} value => ({})", m_Identity, errorNum, errorStatus);
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

    if (_getDeviceState() == Device::DeviceState::Busy && offset != PAS_PanelType_Stop) {
        spdlog::error("{} : Panel controller is busy, operate call failed. Wait and try again.", m_Identity);
        return OpcUa_BadInvalidState;
    }

    if (offset == PAS_PanelType_MoveDeltaLengths) {
        if (args.length() != 6) {
            return OpcUa_BadInvalidArgument;
        }
        if (_getDeviceState() == Device::DeviceState::Off)
            setState(Device::DeviceState::On);

        std::array<float, 6> deltaLengths{};
        float targetLength;
        for (int i = 0; i < 6; i++) {
            UaVariant(args[i]).toFloat(deltaLengths[i]);
            targetLength =  m_pActuators.at(i)->measureLength() + deltaLengths(i);
            m_pActuators.at(i)->setTargetLength(targetLength);
        }

        spdlog::info("{} : PanelController calling moveDeltaLength with delta lengths:\n{}\n{}\n{}\n{}\n{}\n{}\n",
                     m_Identity,
                     deltaLengths[0],
                     deltaLengths[1],
                     deltaLengths[2],
                     deltaLengths[3],
                     deltaLengths[4],
                     deltaLengths[5]);

        deltaLengths = m_pPlatform->moveDeltaLengths(deltaLengths);

        std::array<float, 6> finalLengths{};
        for (int i = 0; i < 6; i++) {
            m_pActuators.at(i)->setDeltaLength(deltaLengths[i]);
            finalLengths[i] = m_pActuators.at(i)->measureLength();
        }

        spdlog::info(
            "{} : Lengths after moveDeltaLengths (distance from target):\n{} ({})\n{} ({})\n{} ({})\n{} ({})\n{} ({})\n{} ({})\n",
            m_Identity,
            finalLengths[0], deltaLengths[0],
            finalLengths[1], deltaLengths[1],
            finalLengths[2], deltaLengths[2],
            finalLengths[3], deltaLengths[3],
            finalLengths[4], deltaLengths[4],
            finalLengths[5], deltaLengths[5]);

    } else if (offset == PAS_PanelType_MoveToLengths) {
        if (_getDeviceState() == Device::DeviceState::Off)
            setState(Device::DeviceState::On);

        std::array<float, 6> targetLengths{};
        for (int i = 0; i < 6; i++) {
            UaVariant(args[i]).toFloat(targetLengths[i]);
            m_pActuators.at(i)->setTargetLength(targetLengths[i]);
        }

        spdlog::info("{} : PanelController calling moveToLengths with target lengths:\n{}\n{}\n{}\n{}\n{}\n{}\n",
                     m_Identity,
                     targetLengths[0],
                     targetLengths[1],
                     targetLengths[2],
                     targetLengths[3],
                     targetLengths[4],
                     targetLengths[5]);

        std::array<float, 6> finalLengths = m_pPlatform->moveToLengths(targetLengths);

        spdlog::info(
            "{} : Lengths after moveDeltaLengths (distance from target):\n{} ({})\n{} ({})\n{} ({})\n{} ({})\n{} ({})\n{} ({})\n",
            m_Identity,
            finalLengths[0], finalLengths[0] - targetLengths[0],
            finalLengths[1], finalLengths[1] - targetLengths[1],
            finalLengths[2], finalLengths[2] - targetLengths[2],
            finalLengths[3], finalLengths[3] - targetLengths[3],
            finalLengths[4], finalLengths[4] - targetLengths[4],
            finalLengths[5], finalLengths[5] - targetLengths[5]);

        for (int i = 0; i < 6; i++) {
            m_pActuators.at(i)->setDeltaLength(finalLengths[i] - targetLengths[i]);
        }
    } else if (offset == PAS_PanelType_Stop) {
        spdlog::info("{} : PanelController calling stop()", m_Identity);
        m_pPlatform->emergencyStop();
    } else if (offset == PAS_PanelType_TurnOn) {
        spdlog::info("{} : PanelController calling turnOn()", m_Identity);
        if (_getDeviceState() == Device::DeviceState::Off) {
            m_pPlatform->turnOn();
        } else {
            spdlog::trace("{} : Device is already on, nothing to do...", m_Identity);
        }
    } else if (offset == PAS_PanelType_TurnOff) {
        spdlog::info("{} : PanelController calling turnOff()", m_Identity);
        if (_getDeviceState() == Device::DeviceState::On) {
            m_pPlatform->turnOff();
        } else {
            spdlog::trace("{} : Device is already off, nothing to do...", m_Identity);
        }
    } else if (offset == PAS_PanelType_FindHome) {
        int direction;
        UaVariant(args[0]).toInt32(direction);
        spdlog::info("{} : PanelController calling findHome() with direction {}", m_Identity, direction);
        m_pPlatform->findHomeFromEndStopAll(direction);
    } else if (offset == PAS_PanelType_ClearError) {
        int errorCode;
        UaVariant(args[0]).toInt32(errorCode);
        spdlog::info("{} : PanelController calling clearError() for error {}", m_Identity, errorCode);
        m_pPlatform->unsetError(errorCode);
    } else if (offset == PAS_PanelType_ClearAllErrors) {
        spdlog::info("{} : PanelController calling clearAllErrors()", m_Identity);
        m_pPlatform->clearActuatorErrors();
        m_pPlatform->clearPlatformErrors();
    } else if (offset == PAS_PanelType_ClearActuatorErrors) {
        spdlog::info("{} : PanelController calling clearActuatorErrors()", m_Identity);
        m_pPlatform->clearActuatorErrors();
    } else if (offset == PAS_PanelType_ClearPlatformErrors) {
        spdlog::info("{} : PanelController calling clearPlatformErrors()", m_Identity);
        m_pPlatform->clearPlatformErrors();
    } else {
        status = OpcUa_BadInvalidArgument;
    }

    return status;
}

void PanelController::addActuator(const std::shared_ptr<ActController> &pActuator) {
    m_pActuators.push_back(pActuator);
    spdlog::trace("{} : Panel controller added actuator {} controller as child...", m_Identity,
                  pActuator->getIdentity());
}

void PanelController::addMPES(const std::shared_ptr<MPESController> &pMPES) {
    m_pMPES.push_back(pMPES);
    spdlog::trace("{} : Panel controller added MPES {} controller as child...", m_Identity, pMPES->getIdentity());
}
