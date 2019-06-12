/**
 * @file psdcontroller.cpp
 * @brief Source file for position sensitive device controller class
 */

#include <chrono>
#include <iostream>
#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uaserver/uaobjecttypes.h"

#include "server/controllers/psdcontroller.hpp"
#include "server/controllers/pascontroller.hpp"

#include "server/controllers/pascontroller.hpp"

#include "common/globalalignment/psdclass.h"
#include "common/opcua/pascominterfacecommon.hpp"
#include "common/opcua/pasobject.hpp"
#include "common/opcua/passervertypeids.hpp"

#include "common/alignment/device.hpp"
#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

/// @details By default, sets the update interval to 500 ms. Creates a new GASPSD object,
/// sets its port, and initializes. Sets its state to On.
PSDController::PSDController(Device::Identity identity) : PasController(std::move(identity), 500)
{
    spdlog::trace("{} : Creating PSD hardware interface... ", m_ID);
#ifndef SIMMODE
    m_pPSD = std::unique_ptr<GASPSD>(new GASPSD(identity));
#else
    m_pPSD = std::unique_ptr<GASPSD>(new DummyGASPSD(identity));
#endif
    spdlog::trace("{} : Initializing PSD hardware interface... ", m_ID);
    m_pPSD->initialize();
}

/// @details Locks the shared mutex while retrieving the state.
UaStatus PSDController::getState(Device::DeviceState &state) {
    UaMutexLocker lock(&m_mutex);
    state = _getDeviceState();
    spdlog::trace("{} : Getting device state => ({})", m_ID, Device::deviceStateNames.at(state));
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

    int dataoffset = offset - PAS_PSDType_x1;
    if ((dataoffset >= 9) || (dataoffset < 0)) { // GASPSD only has 9 data elements. In future this should not be hardcoded.
        return OpcUa_BadInvalidArgument;
      }

    if (__expired()) { // if cached value expired, update it
        status = read();
      }

    value.setDouble(m_pPSD->getOutput(offset));

    spdlog::trace("{} : Getting data... offset=> {} value => ({})", m_ID, offset, value[0].Value.Double);
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

    UaStatus status;
    if ( offset == PAS_PSDType_Read) {
        spdlog::trace("{} : PSD controller calling read()", m_ID);
        status = read();
    }
    else {
        spdlog::error("{} : Invalid method call with offset {}", m_ID, offset);
        status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/// @details Calls update on the GASPSD object and revises the last update time. Locks the shared mutex while reading.
UaStatus PSDController::read()
{
    //UaMutexLocker lock(&m_mutex);

    if (_getErrorState() == Device::ErrorState::Nominal || _getErrorState() == Device::ErrorState::OperableError)
    {
        spdlog::trace("{} : Updating PSD data and lastUpdateTime... ", m_ID);
        m_pPSD->update();
        m_lastUpdateTime = std::chrono::system_clock::now();
        return OpcUa_Good;
    }
    else {
        spdlog::error("{} : Device is in fatal error state, cannot read. ", m_ID);
        return OpcUa_BadInvalidState;
    }
}
