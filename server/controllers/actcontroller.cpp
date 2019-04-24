#include "server/controllers/actcontroller.hpp"

#include <iostream>
#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"

#include "common/opcua/pasobject.hpp"
#include "common/opcua/passervertypeids.hpp"

/// @details Sets state to On, inLength to current length, and DeltaL to 0.
ActController::ActController(int ID, std::shared_ptr<Platform> pPlatform) : PasController(ID, std::move(pPlatform)),
                                                                            m_state(PASState::On), m_DeltaLength(0.0) {}

/// @details Sets state to off.
ActController::~ActController() {
    m_state = PASState::Off; // NOTE: This shouldn't do anything, as the object is destroyed anyways.
}

/// @details Calls update state before returning the current state.
UaStatus ActController::getState(PASState &state) {
    //UaMutexLocker lock(&m_mutex);
    updateState();
    state = m_state;
    return OpcUa_Good;
}

UaStatus ActController::updateState() {
    //UaMutexLocker lock(&m_mutex);
    // update internal state to match teh underlying platform object
    switch (m_pPlatform->getActuatorAt(m_ID)->GetStatus()) {
        case Actuator::StatusModes::Healthy :
            m_state = PASState::On;
            break;
        case Actuator::StatusModes::OperableError :
            m_state = PASState::OperableError;
            break;
        case Actuator::StatusModes::FatalError :
            m_state = PASState::FatalError;
            break;
        default :
            return OpcUa_BadInvalidState;
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
                value.setFloat(m_pPlatform->getActuatorAt(m_ID)->MeasureLength());
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
        errorStatus = m_pPlatform->getActuatorAt(m_ID)->ActuatorErrors[int(errorNum)].Triggered;
        value.setBool(errorStatus);
    } else {
        status = OpcUa_BadInvalidArgument;
    }
    return status;
}

/// @details Locks the shared mutex while writing data.
UaStatus ActController::setData(OpcUa_UInt32 offset, UaVariant value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

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
            if (args.length() != 1) {
                return OpcUa_BadInvalidArgument;
            }
            status = moveDelta(args);
            break;
        case PAS_ACTType_MoveToLength:
            if (args.length() != 1) {
                return OpcUa_BadInvalidArgument;
            }
            status = moveToLength(args);
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/// @details Calls MoveDeltaLengths on the Platform object with the desired length change for this actuator and zero for all others.
// Applies lock to shared mutex to prevent other actions. Will fail unless device state is PAS_On.
UaStatus ActController::moveDelta(const UaVariantArray &args) {
    if (!(m_state == PASState::On))
        return OpcUa_BadNothingToDo;

    std::array<OpcUa_Float, 6> deltaL = {0., 0., 0., 0., 0., 0.}; // Set delta lengths to move to
    UaVariant length = UaVariant(args[0]);
    length.toFloat(deltaL[m_ID]);

    std::cout << "ActController :: Moving actuator " << m_ID << " by " << m_DeltaLength << " mm." << std::endl;
    deltaL = m_pPlatform->MoveDeltaLengths(deltaL);
    m_DeltaLength = deltaL[m_ID];

    return OpcUa_Good;
}

UaStatus ActController::moveToLength(const UaVariantArray &args) {
    if (!(m_state == PASState::On))
        return OpcUa_BadNothingToDo;

    UaStatus status;

    UaVariantArray tempArgs;
    tempArgs.create(1);
    UaVariant(args[0]).toFloat(m_TargetLength);
    tempArgs[0] = UaVariant(m_TargetLength - m_pPlatform->getActuatorAt(m_ID)->MeasureLength())[0];
    status = moveDelta(tempArgs);

    return status;
}
