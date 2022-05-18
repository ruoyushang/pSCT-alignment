#include <string>
#include <vector>

#include "rangefindercontroller.hpp"
#include "panelcontroller.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

RangefinderController::RangefinderController(Device::Identity identity, Client *pClient)
        : PasController(std::move(identity), pClient, 500) {
    spdlog::debug("Created Rangefinder controller.");
}

/* ----------------------------------------------------------------------------
    Class        RangefinderController
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatus RangefinderController::getState(Device::DeviceState &state) {
    //UaMutexLocker lock(&m_mutex);
    state = m_State;
    spdlog::trace("{} : Read device state => ({})", m_Identity, (int) state);
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        RangefinderController
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatus RangefinderController::setState(Device::DeviceState state) {
    m_State = state;
    spdlog::trace("{} : Setting device state => ({})", m_Identity, (int) state);
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        RangefinderController
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatus RangefinderController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    UaStatus status;
    //UaMutexLocker lock(&m_mutex);

    switch (offset) {
        case PAS_RangefinderType_Temp: {
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
    Class        RangefinderController
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatus RangefinderController::setData(OpcUa_UInt32 offset, UaVariant value) {
    UaStatus status;

    switch (offset) {
        case PAS_RangefinderType_Temp: {
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

UaStatus RangefinderController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    UaStatus status;
    //UaMutexLocker lock(&m_mutex);

    spdlog::trace("Offset: {}", offset);
    if (m_State == Device::DeviceState::Busy) {
        spdlog::error("{} : RangefinderController::operate() : Device is busy, method call aborted.", m_Identity);
        return OpcUa_BadInvalidState;
    }
    switch (offset) {
        case PAS_RangefinderType_TurnOn: {
            spdlog::info("RangefinderController::operate() :  Calling TurnOn...");
            break;
        }
        case PAS_RangefinderType_TurnOff: {
            spdlog::info("RangefinderController::operate() :  Calling TurnOff...");
            break;
        }
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

