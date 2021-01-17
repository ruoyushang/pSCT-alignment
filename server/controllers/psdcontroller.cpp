/**
 * @file psdcontroller.cpp
 * @brief Source file for position sensitive device controller class
 */

#include <chrono>

#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uaserver/uaobjecttypes.h"

#include "server/controllers/psdcontroller.hpp"
#include "server/controllers/pascontroller.hpp"

#include "server/controllers/pascontroller.hpp"

#include "common/globalalignment/psdclass.hpp"
#include "common/opcua/pascominterfacecommon.hpp"
#include "common/opcua/pasobject.hpp"
#include "common/opcua/passervertypeids.hpp"

#include "common/alignment/device.hpp"
#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

/// @details By default, sets the update interval to 500 ms. Creates a new GASPSD object,
/// sets its port, and initializes. Sets its state to On.
PSDController::PSDController(Device::Identity identity, std::shared_ptr<PlatformBase> pPlatform)
        : PasController::PasController(std::move(identity),std::move(pPlatform), 500)
{
    spdlog::trace("{} : Creating PSD hardware interface... ", m_Identity);
}

/// @details Initialize this PSD.
bool PSDController::initialize() {
    spdlog::info("{} : Initializing PSD...", m_Identity);

    return true;
}

/// @details Locks the shared mutex while retrieving the state.
UaStatus PSDController::getState(Device::DeviceState &state) {
//    UaMutexLocker lock(&m_Mutex);
    state = _getDeviceState();
    spdlog::trace("{} : Read device state => ({})", m_Identity, Device::deviceStateNames.at(state));
    return OpcUa_Good;
}

/// @details Does not allow setting the state to error or setting the state to
/// its current value. Locks the shared mutex while setting the state.
UaStatus PSDController::setState(Device::DeviceState state) {
    return OpcUa_BadNotWritable;
}


/// @details Calls GASPSD.getOutput() to read data. Locks the shared mutex to prevent concurrent actions while reading data.
UaStatus PSDController::getData(OpcUa_UInt32 offset, UaVariant &value)
{
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (PSDObject::VARIABLES.find(offset) != PSDObject::VARIABLES.end()) {
        if (__expired()) { // if cached value expired, update it
            status = read();
        }
        switch (offset) {
            double tmp;
            case PAS_PSDType_x1:
                spdlog::trace("{} : Read x1 value => ({})", m_Identity, tmp);
                value.setDouble(m_pPlatform->getPSDbyIdentity(m_Identity)->getOutput(0));
                break;
            case PAS_PSDType_y1:
                spdlog::trace("{} : Read y1 value => ({})", m_Identity, tmp);
                value.setDouble(m_pPlatform->getPSDbyIdentity(m_Identity)->getOutput(1));
                break;
            case PAS_PSDType_x2:
                spdlog::trace("{} : Read x2 value => ({})", m_Identity, tmp);
                value.setDouble(m_pPlatform->getPSDbyIdentity(m_Identity)->getOutput(2));
                break;
            case PAS_PSDType_y2:
                spdlog::trace("{} : Read y2 value => ({})", m_Identity, tmp);
                value.setDouble(m_pPlatform->getPSDbyIdentity(m_Identity)->getOutput(3));
                break;
            case PAS_PSDType_dx1:
                spdlog::trace("{} : Read dx1 value => ({})", m_Identity, tmp);
                value.setDouble(m_pPlatform->getPSDbyIdentity(m_Identity)->getOutput(4));
                break;
            case PAS_PSDType_dy1:
                spdlog::trace("{} : Read dy1 value => ({})", m_Identity, tmp);
                value.setDouble(m_pPlatform->getPSDbyIdentity(m_Identity)->getOutput(5));
                break;
            case PAS_PSDType_dx2:
                spdlog::trace("{} : Read dx2 value => ({})", m_Identity, tmp);
                value.setDouble(m_pPlatform->getPSDbyIdentity(m_Identity)->getOutput(6));
                break;
            case PAS_PSDType_dy2:
                spdlog::trace("{} : Read dy2 value => ({})", m_Identity, tmp);
                value.setDouble(m_pPlatform->getPSDbyIdentity(m_Identity)->getOutput(7));
                break;
            case PAS_PSDType_Temp:
                spdlog::trace("{} : Read Temp value => ({})", m_Identity, tmp);
                value.setDouble(m_pPlatform->getPSDbyIdentity(m_Identity)->getOutput(8));
                break;
            default:
                status = OpcUa_BadInvalidArgument;
        }
    }
    else if (PSDObject::ERRORS.find(offset) != PSDObject::ERRORS.end()) {
        return getError(offset,value);
    } else {
        return OpcUa_BadInvalidArgument;
    }

    return status;
}

UaStatus PSDController::getError(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;
    bool errorStatus;

    OpcUa_UInt32 errorNum = offset - PAS_PSDType_Error0;
    if (errorNum >= 0 && errorNum < PSDObject::ERRORS.size()) {
        errorStatus = m_pPlatform->getPSDbyIdentity(m_Identity)->getError(int(errorNum));
        value.setBool(errorStatus);
        spdlog::trace("{} : Read error {} value => ({})", m_Identity, errorNum, errorStatus);
    } else {
        status = OpcUa_BadInvalidArgument;
    }
    return status;
}

/// @details Has no effect, as PSDs have no writable data variables.
UaStatus PSDController::setData(OpcUa_UInt32 offset, UaVariant value)
{
    return OpcUa_BadNotWritable;
}

/// @details Locks the shared mutex to prevent concurrent actions while calling methods.
UaStatus PSDController::operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    //UaMutexLocker lock(&m_mutex);
    if (_getDeviceState() == Device::DeviceState::Busy) {
        spdlog::error("{} : PSD is busy, operate call failed.", m_Identity);
        return OpcUa_BadInvalidState;
    }

    UaStatus status;
    switch (offset){
        case PAS_PSDType_Read :
            spdlog::trace("{} : PSDController calling read()", m_Identity);
            status = read();
            break;
        case PAS_PSDType_TurnOn :
            spdlog::trace("{} : PSDController calling turnOn()", m_Identity);
            m_pPlatform->getPSDbyIdentity(m_Identity)->turnOn();
            break;
        case PAS_PSDType_TurnOff :
            spdlog::trace("{} : PSDController calling TurnOff()", m_Identity);
            m_pPlatform->getPSDbyIdentity(m_Identity)->turnOff();
            break;
        default :
            spdlog::error("{} : Invalid method call with offset {}", m_Identity, offset);
            status = OpcUa_BadInvalidArgument;
            break;
    }


    return status;
}

/// @details Calls update on the GASPSD object and revises the last update time. Locks the shared mutex while reading.
UaStatus PSDController::read()
{
    //UaMutexLocker lock(&m_mutex);
    spdlog::debug("{}: errorState(): {}", m_Identity, int(_getErrorState()));

    if (_getErrorState() == Device::ErrorState::Nominal || _getErrorState() == Device::ErrorState::OperableError)
    {
        spdlog::trace("{} : Updating PSD data and lastUpdateTime... ", m_Identity);
        m_pPlatform->getPSDbyIdentity(m_Identity)->update();
        m_LastUpdateTime = std::chrono::system_clock::now();
        return OpcUa_Good;
    }
    else {
        spdlog::error("{} : Device is in fatal error state, cannot read. ", m_Identity);
        return OpcUa_BadInvalidState;
    }
}
