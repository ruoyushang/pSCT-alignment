#include "client/controllers/actcontroller.hpp"

#include <iostream>
#include <string>
#include <vector>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include "common/alignment/device.hpp"
#include "common/opcua/pascominterfacecommon.hpp"
#include "common/opcua/pasobject.hpp"
#include "common/opcua/passervertypeids.hpp"

#include "client/clienthelper.hpp"
#include "client/controllers/pascontroller.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

ActController::ActController(Device::Identity identity, Client *pClient) : PasController(
    std::move(identity), pClient) {
}

/* ----------------------------------------------------------------------------
    Class        ActController
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatus ActController::getState(Device::DeviceState &state) {
    //UaMutexLocker lock(&m_mutex);    
    UaStatus status;
    UaVariant value;
    int v;

    m_pClient->read({m_pClient->getDeviceNodeId(m_ID) + "." + "State"}, &value);
    value.toInt32(v);

    state = static_cast<Device::DeviceState>(v);
    spdlog::trace("{} : Read device state => ({})", m_ID, Device::deviceStateNames.at(state));

    return status;
}

/* ----------------------------------------------------------------------------
    Class        ActController
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatus ActController::setState(Device::DeviceState state) {
    return OpcUa_BadNotWritable;
}
/* ----------------------------------------------------------------------------
    Class        ActController
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
// Temporary - move ActController to common
UaStatus ActController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    std::string varName;

    if (ACTObject::ERRORS.count(offset) > 0) {
        return getError(offset, value);
    } else {
        switch (offset) {
            case PAS_ACTType_DeltaLength:
                varName = "DeltaLength";
                break;
            case PAS_ACTType_CurrentLength:
                varName = "CurrentLength";
                break;
            case PAS_ACTType_TargetLength:
                varName = "TargetLength";
                break;
            case PAS_ACTType_Position:
                varName = "Position";
                break;
            case PAS_ACTType_Serial:
                varName = "Serial";
                break;
            case PAS_ACTType_ErrorState:
                varName = "ErrorState";
                break;
            default:
                return OpcUa_BadInvalidArgument;
        }
        std::vector<std::string> varsToRead = {m_pClient->getDeviceNodeId(m_ID) + "." + varName};
        status = m_pClient->read(varsToRead, &value);
    }

    if (status == OpcUa_BadInvalidState) {
        spdlog::error(
            "{} : ActController::getData() : Device is in a bad state (busy, off, error) and could not read data. Check state and try again.",
            m_ID);
    }

    switch (offset) {
        case PAS_ACTType_DeltaLength:
            spdlog::trace("{} : Read deltaLength value => ({})", m_ID, value[0].Value.Float);
            break;
        case PAS_ACTType_CurrentLength:
            spdlog::trace("{} : Read currentLength value => ({})", m_ID, value[0].Value.Float);
            break;
        case PAS_ACTType_TargetLength:
            spdlog::trace("{} : Read targetLength value => ({})", m_ID, value[0].Value.Float);
            break;
        case PAS_ACTType_Position:
            spdlog::trace("{} : Read position value => ({})", m_ID, value[0].Value.Int32);
            break;
        case PAS_ACTType_Serial:
            spdlog::trace("{} : Read serial value => ({})", m_ID, value[0].Value.Int32);
            break;
        case PAS_ACTType_ErrorState:
            spdlog::trace("{} : Read errorState value => ({})", m_ID, value[0].Value.UInt32);
            break;
        default:
            return status;
    }

    return status;
}

// Temporary - move ActController to common.
UaStatus ActController::getError(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (ACTObject::ERRORS.count(offset) > 0) {
        std::string varName = std::get<0>(ACTObject::ERRORS.at(offset));
        std::vector<std::string> varsToRead = {m_pClient->getDeviceNodeId(m_ID) + "." + varName};
        status = m_pClient->read(varsToRead, &value);
        spdlog::trace("{} : Read error {} value => ({})", m_ID, offset, value[0].Value.Boolean);
    } else {
        status = OpcUa_BadInvalidArgument;
    }
    return status;
}

/* ----------------------------------------------------------------------------
    Class        ActController
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatus ActController::setData(OpcUa_UInt32 offset, UaVariant value) {
    return OpcUa_BadNotWritable;
}

/* ----------------------------------------------------------------------------
    Class        ActController
    Method       Operate
    Description  run a method on the actuator
-----------------------------------------------------------------------------*/
UaStatus ActController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    UaStatus status;

    // don't lock the object -- might want to change state while operating the device!
    // UaMutexLocker lock(&m_mutex);

    Device::DeviceState state;
    getState(state);

    Device::ErrorState errorState;
    UaVariant var;
    int i;
    getData(PAS_ACTType_ErrorState, var);
    var.toInt32(i);
    errorState = static_cast<Device::ErrorState>(i);

    switch (offset) {
        case PAS_ACTType_MoveDeltaLength:
            float deltaLength;
            UaVariant(args[0]).toFloat(deltaLength);

            // Check state manually beforehand
            if (state != Device::DeviceState::On || errorState == Device::ErrorState::FatalError) {
                spdlog::error(
                    "{} : ActController::getData() : Device is in a bad state (busy, off, error). MoveDeltaLength call aborted. Check state and try again.",
                    m_ID);
                status = OpcUa_BadInvalidState;
            } else {
                spdlog::info("{} : ActuatorController calling moveDeltaLength() with delta length {} mm", m_ID,
                             deltaLength);
                status = m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_ID), UaString("MoveDeltaLength"),
                                                    args);
            }
            break;
        case PAS_ACTType_MoveToLength:
            float targetLength;
            UaVariant(args[0]).toFloat(targetLength);

            spdlog::info("{} : ActuatorController calling moveToLength() with target length {}", m_ID, targetLength);

            // Check state manually beforehand
            if (state != Device::DeviceState::On || errorState == Device::ErrorState::FatalError) {
                spdlog::error(
                    "{} : ActController::getData() : Device is in a bad state (busy, off, error). MoveToLength call aborted. Check state and try again.",
                    m_ID);
                status = OpcUa_BadInvalidState;
            } else {
                spdlog::info("{} : ActuatorController calling moveToLength() with target length {} mm", m_ID,
                             targetLength);
                status = m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_ID), UaString("MoveToLength"), args);
            }
            break;
        case PAS_ACTType_ForceRecover:
            spdlog::info("{} : ActuatorController calling forceRecover()", m_ID);
            status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("ForceRecover"));
            break;
        case PAS_ACTType_ClearError:
            spdlog::info("{} : ActuatorController calling clearError() for error {}", m_ID, args[0].Value.Int32);
            status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("ClearError"), args);
            break;
        case PAS_ACTType_ClearAllErrors:
            spdlog::info("{} : ActuatorController calling clearAllErrors()", m_ID);
            status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("ClearAllErrors"));
            break;
        case PAS_ACTType_TurnOn:
            spdlog::info("{} : ActuatorController calling turnOn()", m_ID);
            status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("TurnOn"));
            break;
        case PAS_ACTType_TurnOff:
            spdlog::info("{} : ActuatorController calling turnOff()", m_ID);
            status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("TurnOff"));
            break;
        case PAS_ACTType_Stop:
            spdlog::info("{} : ActuatorController calling stop()", m_ID);
            status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("Stop"));
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }

    if (status == OpcUa_BadInvalidState) {
        spdlog::error(
            "{} : ActController::getData() : Device is in a bad state (busy, off, error) and could not call method. Check state and try again.",
            m_ID);
    }

    return status;
}
