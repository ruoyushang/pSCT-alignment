#include <string>
#include <vector>

#include "lasercontroller.hpp"
#include "panelcontroller.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

LaserController::LaserController(Device::Identity identity, Client *pClient)
        : PasController(std::move(identity), pClient, 500) {
    spdlog::debug("Created Laser controller.");
}

/* ----------------------------------------------------------------------------
    Class        LaserController
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatus LaserController::getState(Device::DeviceState &state) {
    //UaMutexLocker lock(&m_mutex);
    state = m_State;
    spdlog::trace("{} : Read device state => ({})", m_Identity, (int) state);
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        LaserController
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatus LaserController::setState(Device::DeviceState state) {
    m_State = state;
    spdlog::trace("{} : Setting device state => ({})", m_Identity, (int) state);
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        LaserController
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatus LaserController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    UaStatus status;
    //UaMutexLocker lock(&m_mutex);

    switch (offset) {
        case PAS_LaserType_Temp: {
            std::string string1;
            value.setString(string1.c_str());
            spdlog::trace("{} : Read Temp value => ({})", m_Identity, string1);
            break;
        }
        default:
            status = OpcUa_BadInvalidArgument;
            break;
    }
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        LaserController
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatus LaserController::setData(OpcUa_UInt32 offset, UaVariant value) {
    UaStatus status;

    switch (offset) {
        case PAS_LaserType_Temp: {
            std::string val;
            val = value.toString().toUtf8();
            spdlog::trace("{} : Setting temp value... value => ({})", m_Identity, val);
            break;
        }
        default:
            return OpcUa_BadNotWritable;
            break;
    }


    return OpcUa_Good;
}

UaStatus LaserController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    UaStatus status;
    //UaMutexLocker lock(&m_mutex);

    spdlog::trace("Offset: {}", offset);
    if (m_State == Device::DeviceState::Busy) {
        spdlog::error("{} : LaserController::operate() : Device is busy, method call aborted.", m_Identity);
        return OpcUa_BadInvalidState;
    }
    switch (offset) {
        case PAS_LaserType_TurnOn: {
            spdlog::info("LaserController::operate() :  Calling TurnOn...");
            break;
        }
        case PAS_LaserType_TurnOff: {
            spdlog::info("LaserController::operate() :  Calling TurnOff...");
            break;
        }
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

