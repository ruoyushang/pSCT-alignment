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

/// @details By default, sets the update interval to 500 ms. Creates a new GASPSD object,
/// sets its port, and initializes. Sets its state to On.
PSDController::PSDController(Device::Identity identity) : PasController(std::move(identity), 500)
{
    m_pPSD = std::unique_ptr<GASPSD>(new GASPSD());
    m_pPSD->setPort();
    m_pPSD->Initialize();
}

/// @details Locks the shared mutex while retrieving the state.
UaStatus PSDController::getState(Device::DeviceState &state) {
    UaMutexLocker lock(&m_mutex);
    state = _getState();
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
        status = read();
    }
    else {
        status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/// @details Calls update on the GASPSD object and revises the last update time. Locks the shared mutex while reading.
UaStatus PSDController::read()
{
    //UaMutexLocker lock(&m_mutex);

    if ( _getState() == Device::DeviceState::On )
    {
        std::cout << "\nReading PSD " <<  m_ID << std::endl;
        m_pPSD->Update();
        m_lastUpdateTime = std::chrono::system_clock::now();
        return OpcUa_Good;
    }
    else {
        return OpcUa_BadOutOfService;
    }
}
