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


/// @details Calls update state before returning the current state.
UaStatus ActController::getState(Device::DeviceState &state) {
    UaMutexLocker lock(&m_mutex);
    state = _getState();
    return OpcUa_Good;
}

UaStatus ActController::setState(Device::DeviceState state) {
    UaMutexLocker lock(&m_mutex);

    switch (state) {
        case Device::DeviceState::On:
            m_pPlatform->getActuatorbyIdentity(m_ID)->turnOn();
            break;
        case Device::DeviceState::Off:
            m_pPlatform->getActuatorbyIdentity(m_ID)->turnOff();
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

/// @details If the offset given points to an error variable, internally calls getError. Locks the shared mutex while reading data.
UaStatus ActController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (ACTObject::VARIABLES.find(offset) != ACTObject::VARIABLES.end()) {
        switch (offset) {
            case PAS_ACTType_DeltaLength:
                value.setFloat(m_DeltaLength);
                break;
            case PAS_ACTType_CurrentLength:
                value.setFloat(m_pPlatform->getActuatorbyIdentity(m_ID)->measureLength());
		        break;
            case PAS_ACTType_TargetLength:
                value.setFloat(m_TargetLength);
                break;
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

    UaStatus status;
    UaVariantArray tempArgs;
    switch (offset) {
        case PAS_ACTType_MoveDeltaLength:
            if (!(_getState() == Device::DeviceState::On))
                return OpcUa_BadNothingToDo;
            status = moveDelta(args);
            break;
        case PAS_ACTType_MoveToLength:
            if (args.length() != 1) {
                return OpcUa_BadInvalidArgument;
            }
            status = moveToLength(args);
            break;
        case PAS_ACTType_ForceRecover:
            m_pPlatform->getActuatorbyIdentity(m_ID)->forceRecover();
            break;
        case PAS_ACTType_ClearError:
            if (args.length() != 1) {
                return OpcUa_BadInvalidArgument;
            }
            m_pPlatform->getActuatorbyIdentity(m_ID)->unsetError(args[0].Value.Int32);
            break;
        case PAS_ACTType_ClearAllErrors:
            m_pPlatform->getActuatorbyIdentity(m_ID)->clearErrors();
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/// @details Calls MoveDeltaLengths on the Platform object with the desired length change for this actuator and zero for all others.
// Applies lock to shared mutex to prevent other actions. Will fail unless device state is PAS_On.
UaStatus ActController::moveDelta(const UaVariantArray &args) {

    std::array<OpcUa_Float, 6> deltaL = {0., 0., 0., 0., 0., 0.}; // Set delta lengths to move to
    UaVariant length = UaVariant(args[0]);
    length.toFloat(deltaL[std::stoi(m_ID.eAddress)]);

    std::cout << "ActController :: Moving actuator " << m_ID << " by " << m_DeltaL << " mm." << std::endl;
    deltaL = m_pPlatform->moveDeltaLengths(deltaL);
    m_DeltaL = deltaL[std::stoi(m_ID.eAddress)];

    return OpcUa_Good;
}

UaStatus ActController::moveToLength(const UaVariantArray &args) {
    if (!(_getState() == Device::DeviceState::On))
        return OpcUa_BadNothingToDo;

    UaStatus status;

    UaVariantArray tempArgs;
    tempArgs.create(1);
    UaVariant(args[0]).toFloat(m_TargetLength);
    tempArgs[0] = UaVariant(m_TargetLength - m_pPlatform->getActuator(std::stoi(m_ID.eAddress))->measureLength())[0];
    status = moveDelta(tempArgs);

    return status;
}
