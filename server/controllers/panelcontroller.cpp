#include "server/controllers/panelcontroller.hpp"

#include <array>
#include <sstream>
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
//TODO refactor all the pNodeManager here so that operate() can fire event. Monitor this event with Client.

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
            spdlog::trace("{} : Read ErrorState value => ({})", m_Identity, Device::errorStateNames.at(errorState));
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


        std::array<float, 6> initialLengths{};
        std::array<float, 6> deltaLengths{};
        std::array<float, 6> targetLengths{};
        std::ostringstream os;
        for (int i = 0; i < 6; i++) {
            UaVariant(args[i]).toFloat(deltaLengths[i]);
            initialLengths[i] = m_pActuators.at(i)->getCurrentLength();
            targetLengths[i] = initialLengths[i] + deltaLengths[i];
            m_pActuators.at(i)->setTargetLength(targetLengths[i]);
            os << m_pActuators.at(i)->getIdentity() << " : " << initialLengths[i] << " + " << deltaLengths[i] << " => " << targetLengths[i] << std::endl;
        }

        spdlog::info(
            "{} : PanelController calling moveDeltaLengths\n ActuatorId : CurrentLength + Delta Length => TargetLength\n\n{}",
            m_Identity, os.str());
        
        deltaLengths = m_pPlatform->moveDeltaLengths(deltaLengths);

        std::array<float, 6> finalLengths{};
        os.str("");
        for (int i = 0; i < 6; i++) {
            m_pActuators.at(i)->setDeltaLength(deltaLengths[i]);
            finalLengths[i] = m_pActuators.at(i)->getCurrentLength();
            os << m_pActuators.at(i)->getIdentity() << " : " << finalLengths[i] << " (" << deltaLengths[i] << ")" << std::endl;
        }

        spdlog::info(
            "{} : Lengths after moveDeltaLengths (Remaining distance to target):\n{}",
            m_Identity, os.str());
    
    } else if (offset == PAS_PanelType_MoveToLengths) {
        if (_getDeviceState() == Device::DeviceState::Off)
            setState(Device::DeviceState::On);

        std::array<float, 6> initialLengths{};
        std::array<float, 6> targetLengths{};
        std::ostringstream os;
        for (int i = 0; i < 6; i++) {
            UaVariant(args[i]).toFloat(targetLengths[i]);
            initialLengths[i] = m_pActuators.at(i)->getCurrentLength();
            m_pActuators.at(i)->setTargetLength(targetLengths[i]);
            os << m_pActuators.at(i)->getIdentity() << " : " << initialLengths[i] << " => " << targetLengths[i] << std::endl;
        }

        spdlog::info(
            "{} : PanelController calling moveToLengths:\n ActuatorId : CurrentLength => TargetLength\n\n{}",
            m_Identity, os.str());
        
        std::array<float, 6> finalLengths = m_pPlatform->moveToLengths(targetLengths);
        os.str("");
        for (int i = 0; i < 6; i++) {
            m_pActuators.at(i)->setDeltaLength(targetLengths[i] - finalLengths[i]);
            os << m_pActuators.at(i)->getIdentity() << " : " << finalLengths[i] << " (" << targetLengths[i] - finalLengths[i] << ")" << std::endl;          
        }

        spdlog::info(
            "{} : Lengths after moveToLengths (Remaining distance to target):\n{}",
            m_Identity, os.str());
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
    spdlog::info("{} : Panel controller added actuator {} controller as Actuator child {}...", m_Identity,
                 pActuator->getIdentity(), m_pActuators.size());
}

void PanelController::addMPES(const std::shared_ptr<MPESController> &pMPES) {
    m_pMPES.push_back(pMPES);
    spdlog::info("{} : Panel controller added MPES {} controller as MPES child {}...", m_Identity, pMPES->getIdentity(),
                 m_pMPES.size());
}
