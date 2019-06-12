#include "server/controllers/actcontroller.hpp"

#include <iostream>
#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"

#include "common/alignment/device.hpp"
#include "common/alignment/platform.hpp"
#include "common/opcua/pasobject.hpp"
#include "common/opcua/passervertypeids.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

/// @details Calls update state before returning the current state.
UaStatus ActController::getState(Device::DeviceState &state) {
    UaMutexLocker lock(&m_mutex);
    state = _getDeviceState();
    spdlog::trace("{} : Getting device state => ({})", m_ID, Device::deviceStateNames.at(state));
    return OpcUa_Good;
}

UaStatus ActController::setState(Device::DeviceState state) {
    return OpcUa_BadNotWritable;
}

/// @details If the offset given points to an error variable, internally calls getError. Locks the shared mutex while reading data.
UaStatus ActController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (ACTObject::VARIABLES.find(offset) != ACTObject::VARIABLES.end()) {
        switch (offset) {
            case PAS_ACTType_DeltaLength:
                spdlog::trace("{} : Getting DeltaLength value => ({})", m_ID, m_DeltaLength);
                value.setFloat(m_DeltaLength);
                break;
            case PAS_ACTType_CurrentLength: {
                float length = m_pPlatform->getActuatorbyIdentity(m_ID)->measureLength();
                spdlog::trace("{} : Getting CurrentLength value => ({})", m_ID, length);
                value.setFloat(length);
                break;
            }
            case PAS_ACTType_TargetLength:
                spdlog::trace("{} : Getting TargetLength value => ({})", m_ID, m_TargetLength);
                value.setFloat(m_TargetLength);
                break;
            case PAS_ACTType_Position:
                spdlog::trace("{} : Getting Position value => ({})", m_ID, m_ID.position);
                value.setInt32(m_ID.position);
                break;
            case PAS_ACTType_Serial:
                spdlog::trace("{} : Getting Serial value => ({})", m_ID, m_ID.serialNumber);
                value.setInt32(m_ID.serialNumber);
                break;
            case PAS_ACTType_ErrorState: {
                Device::ErrorState errorState = _getErrorState();
                spdlog::trace("{} : Getting ErrorState value => ({})", m_ID, static_cast<int>(errorState));
                value.setInt32(static_cast<int>(errorState));
                break;
            }
            default:
                status = OpcUa_BadInvalidArgument;
        }
    } else if (ACTObject::ERRORS.find(offset) != ACTObject::ERRORS.end()) {
        return getError(offset, value);
    } else {
        return OpcUa_BadInvalidArgument;
    }
    return status;
}

UaStatus ActController::getError(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;
    bool errorStatus;

    OpcUa_UInt32 errorNum = offset - PAS_ACTType_Error0;
    if (errorNum >= 0 && errorNum < ACTObject::ERRORS.size()) {
        errorStatus = m_pPlatform->getActuatorbyIdentity(m_ID)->getError(int(errorNum));
        value.setBool(errorStatus);
        spdlog::trace("{} : Getting error {} value => ({})", m_ID, errorNum, errorStatus);
    } else {
        status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/// @details Locks the shared mutex while writing data.
UaStatus ActController::setData(OpcUa_UInt32 offset, UaVariant value) {
    //UaMutexLocker lock(&m_mutex);
    return OpcUa_BadNotWritable;
}

/// @details Does nothing, as errors are not currently user-writeable.
UaStatus ActController::setError(OpcUa_UInt32 offset, UaVariant value) {
    return OpcUa_BadNotWritable;
}

/// @details Locks shared mutex while operating device.
UaStatus ActController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    //UaMutexLocker lock(&m_mutex); // Lock the object to prevent other actions while operating.

    if (_getDeviceState() == Device::DeviceState::Busy && offset != PAS_ACTType_TurnOff) {
        spdlog::error("{} : Actuator controller is busy, operate call failed. Wait and try again.", m_ID);
        return OpcUa_BadInvalidState;
    }

    UaStatus status;
    UaVariantArray tempArgs;
    switch (offset) {
        case PAS_ACTType_MoveDeltaLength:
            spdlog::info("{} : Actuator controller calling moveDeltaLength with delta length {}", m_ID,
                         args[0].Value.Float);
            if (_getDeviceState() != Device::DeviceState::On) {
                spdlog::error("{} : Actuator controller is off, operate call failed. Turn on and try again.", m_ID);
                status = OpcUa_BadInvalidState;
            } else if (_getErrorState() == Device::ErrorState::FatalError) {
                spdlog::error(
                    "{} : Actuator controller is in fatal error state, operate call failed. Fix/clear errors and try again.",
                    m_ID);
                status = OpcUa_BadInvalidState;
            } else {
                status = moveDelta(args[0].Value.Float);
            }
            break;
        case PAS_ACTType_MoveToLength:
            spdlog::info("{} : Actuator controller calling moveToLength with target length {}", m_ID,
                         args[0].Value.Float);
            if (_getDeviceState() != Device::DeviceState::On) {
                spdlog::error("{} : Actuator controller is off, operate call failed. Turn on and try again.", m_ID);
                status = OpcUa_BadInvalidState;
            } else if (_getErrorState() == Device::ErrorState::FatalError) {
                spdlog::error(
                    "{} : Actuator controller is in fatal error state, operate call failed. Fix/clear errors and try again.",
                    m_ID);
                status = OpcUa_BadInvalidState;
            } else {
                status = moveToLength(args[0].Value.Float);
            }
            break;
        case PAS_ACTType_ForceRecover:
            spdlog::info("{} : Actuator controller calling forceRecover()", m_ID);
            m_pPlatform->getActuatorbyIdentity(m_ID)->forceRecover();
            break;
        case PAS_ACTType_ClearError:
            spdlog::info("{} : Actuator controller calling clearError() for error {}", m_ID, args[0].Value.Int32);
            m_pPlatform->getActuatorbyIdentity(m_ID)->unsetError(args[0].Value.Int32);
            break;
        case PAS_ACTType_ClearAllErrors:
            spdlog::info("{} : Actuator controller calling clearAllErrors()", m_ID);
            m_pPlatform->getActuatorbyIdentity(m_ID)->clearErrors();
            break;
        case PAS_ACTType_TurnOn:
            spdlog::info("{} : Actuator controller calling turnOn()", m_ID);
            if (_getDeviceState() == Device::DeviceState::Off) {
                m_pPlatform->getActuatorbyIdentity(m_ID)->turnOn();
                initialize();
            } else {
                spdlog::trace("{} : Device is already on, nothing to do...", m_ID);
            }
            break;
        case PAS_ACTType_TurnOff:
            spdlog::info("{} : Actuator controller calling turnOff()", m_ID);
            if (_getDeviceState() == Device::DeviceState::On) {
                m_pPlatform->getActuatorbyIdentity(m_ID)->turnOff();
            } else {
                spdlog::trace("{} : Device is already off, nothing to do...", m_ID);
            }
            break;
        case PAS_ACTType_Stop:
            spdlog::info("{} : Actuator controller calling stop()...", m_ID);
            m_pPlatform->getActuatorbyIdentity(m_ID)->emergencyStop();
            break;
        default:
            spdlog::error("{} : Invalid method call with offset {}", m_ID, offset);
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/// @details Calls MoveDeltaLengths on the Platform object with the desired length change for this actuator and zero for all others.
// Applies lock to shared mutex to prevent other actions. Will fail unless device state is PAS_On.
UaStatus ActController::moveDelta(float deltaLength) {

    std::array<OpcUa_Float, 6> deltaLengths = {0., 0., 0., 0., 0., 0.}; // Set delta lengths to move to
    deltaLengths[m_ID.position] = deltaLength;

    spdlog::trace("{} : Setting target length to {}", m_ID, m_pPlatform->measureLengths()[m_ID.position] + deltaLength);
    m_TargetLength = m_pPlatform->measureLengths()[m_ID.position] + deltaLength;

    deltaLengths = m_pPlatform->moveDeltaLengths(deltaLengths);
    spdlog::trace("{} : Setting remaining length (deltaLength) to {}", m_ID, deltaLengths[m_ID.position]);
    m_DeltaL = deltaLengths[m_ID.position];

    return OpcUa_Good;
}

UaStatus ActController::moveToLength(float targetLength) {

    UaStatus status;
    std::array<OpcUa_Float, 6> targetLengths = {0., 0., 0., 0., 0., 0.}; // Set target lengths to move to
    targetLengths[m_ID.position] = targetLength;

    spdlog::trace("{} : Setting target length to {}", m_ID, targetLength);
    m_TargetLength = targetLength;
    std::array<OpcUa_Float, 6> finalLengths = m_pPlatform->moveToLengths(targetLengths);

    spdlog::trace("{} : Setting remaining length (deltaLength) to {}", m_ID,
                  targetLength - finalLengths[m_ID.position]);
    m_DeltaL = targetLength - finalLengths[m_ID.position];

    return status;
}
