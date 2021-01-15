/**
 * @file rangefindercontroller.cpp
 * @brief Source file for position sensitive device controller class
 */

#include <chrono>

#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uaserver/uaobjecttypes.h"

#include "server/controllers/rangefindercontroller.hpp"
#include "server/controllers/pascontroller.hpp"

#include "server/controllers/pascontroller.hpp"

#include "common/globalalignment/rangefinderclass.hpp"
#include "common/opcua/pascominterfacecommon.hpp"
#include "common/opcua/pasobject.hpp"
#include "common/opcua/passervertypeids.hpp"

#include "common/alignment/device.hpp"
#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

/// @details By default, sets the update interval to 500 ms. Creates a new GASRangefinder object,
/// sets its port, and initializes. Sets its state to On.
RangefinderController::RangefinderController(Device::Identity identity, std::shared_ptr<PlatformBase> pPlatform)
        : PasController::PasController(std::move(identity),std::move(pPlatform), 500)
{
    spdlog::trace("{} : Creating Rangefinder hardware interface... ", m_Identity);
}

/// @details Initialize this Rangefinder.
bool RangefinderController::initialize() {
    spdlog::info("{} : Initializing Rangefinder...", m_Identity);

    return true;
}

/// @details Locks the shared mutex while retrieving the state.
UaStatus RangefinderController::getState(Device::DeviceState &state) {
//    UaMutexLocker lock(&m_Mutex);
    state = _getDeviceState();
    spdlog::trace("{} : Read device state => ({})", m_Identity, Device::deviceStateNames.at(state));
    return OpcUa_Good;
}

/// @details Does not allow setting the state to error or setting the state to
/// its current value. Locks the shared mutex while setting the state.
UaStatus RangefinderController::setState(Device::DeviceState state) {
    return OpcUa_BadNotWritable;
}


/// @details Calls GASRangefinder.getOutput() to read data. Locks the shared mutex to prevent concurrent actions while reading data.
UaStatus RangefinderController::getData(OpcUa_UInt32 offset, UaVariant &value)
{
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (RangeFinderObject::VARIABLES.find(offset) != RangeFinderObject::VARIABLES.end()) {
        if (__expired()) { // if cached value expired, update it
            status = read();
        }
        switch (offset) {
            double tmp;
            case PAS_RangefinderType_Temp:
                spdlog::trace("{} : Read Temp value => ({})", m_Identity, tmp);
//                value.setDouble(m_pPlatform->getRangefinderbyIdentity(m_Identity)->getOutput(8));
                break;
        }
    }
    else if (RangeFinderObject::ERRORS.find(offset) != RangeFinderObject::ERRORS.end()) {
        return getError(offset,value);
    } else {
        return OpcUa_BadInvalidArgument;
    }

    return status;
}

UaStatus RangefinderController::getError(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;
    bool errorStatus;

    OpcUa_UInt32 errorNum = offset - PAS_RangefinderType_Error0;
    if (errorNum >= 0 && errorNum < RangeFinderObject::ERRORS.size()) {
        errorStatus = m_pPlatform->getRangefinderbyIdentity(m_Identity)->getError(int(errorNum));
        value.setBool(errorStatus);
        spdlog::trace("{} : Read error {} value => ({})", m_Identity, errorNum, errorStatus);
    } else {
        status = OpcUa_BadInvalidArgument;
    }
    return status;
}

/// @details Has no effect, as Rangefinders have no writable data variables.
UaStatus RangefinderController::setData(OpcUa_UInt32 offset, UaVariant value)
{
    return OpcUa_BadNotWritable;
}

/// @details Locks the shared mutex to prevent concurrent actions while calling methods.
UaStatus RangefinderController::operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    //UaMutexLocker lock(&m_mutex);

    UaStatus status;

    if (_getDeviceState() == Device::DeviceState::Busy) {
        spdlog::error("{} : Rangefinder is busy, operate call failed.", m_Identity);
        return OpcUa_BadInvalidState;
    }

    switch (offset) {
        case PAS_RangefinderType_TurnOn :
            spdlog::trace("{} : RangefinderController calling turnOn()", m_Identity);
            m_pPlatform->getRangefinderbyIdentity(m_Identity)->turnOn();
            status = OpcUa_Good;
            break;
        case PAS_RangefinderType_TurnOff :
            spdlog::trace("{} : RangefinderController calling turnOff()", m_Identity);
            m_pPlatform->getRangefinderbyIdentity(m_Identity)->turnOff();
            status = OpcUa_Good;
            break;
        case PAS_RangefinderType_Read :
            spdlog::trace("{} : RangefinderController calling Read() - no effect", m_Identity);
            status = OpcUa_Good;
            break;
        default :
            spdlog::error("{} : Invalid method call with offset {}", m_Identity, offset);
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/// @details Calls update on the GASRangefinder object and revises the last update time. Locks the shared mutex while reading.
UaStatus RangefinderController::read()
{
    //UaMutexLocker lock(&m_mutex);
    spdlog::debug("{}: errorState(): {}", m_Identity, int(_getErrorState()));

    if (_getErrorState() == Device::ErrorState::Nominal || _getErrorState() == Device::ErrorState::OperableError)
    {
        spdlog::trace("{} : Updating Rangefinder data and lastUpdateTime... ", m_Identity);
        m_pPlatform->getRangefinderbyIdentity(m_Identity)->isOn();
        m_LastUpdateTime = std::chrono::system_clock::now();
        return OpcUa_Good;
    }
    else {
        spdlog::error("{} : Device is in fatal error state, cannot read. ", m_Identity);
        return OpcUa_BadInvalidState;
    }
}
