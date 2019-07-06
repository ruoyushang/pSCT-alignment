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
    spdlog::trace("{} : Creating PSD hardware interface... ", m_Identity);
#ifndef SIMMODE
    m_pPSD = std::unique_ptr<GASPSD>(new GASPSD(identity));
#else
    m_pPSD = std::unique_ptr<GASPSD>(new DummyGASPSD(identity));
#endif
    spdlog::trace("{} : Initializing PSD hardware interface... ", m_Identity);
    m_pPSD->initialize();
}

/// @details Locks the shared mutex while retrieving the state.
UaStatus PSDController::getState(Device::DeviceState &state) {
    UaMutexLocker lock(&m_Mutex);
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

    int dataoffset = offset - PAS_PSDType_x1;
    if ((dataoffset >= 9) || (dataoffset < 0)) { // GASPSD only has 9 data elements. In future this should not be hardcoded.
        return OpcUa_BadInvalidArgument;
      }

    if (__expired()) { // if cached value expired, update it
        status = read();
      }

    value.setDouble(m_pPSD->getOutput(offset));

    spdlog::trace("{} : Read data... offset=> {} value => ({})", m_Identity, offset, value[0].Value.Double);
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
        spdlog::trace("{} : PSDController calling read()", m_Identity);
        status = read();
    }
    else {
        spdlog::error("{} : Invalid method call with offset {}", m_Identity, offset);
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
        spdlog::trace("{} : Updating PSD data and lastUpdateTime... ", m_Identity);
        m_pPSD->update();
        m_LastUpdateTime = std::chrono::system_clock::now();
        return OpcUa_Good;
    }
    else {
        spdlog::error("{} : Device is in fatal error state, cannot read. ", m_Identity);
        return OpcUa_BadInvalidState;
    }
}
