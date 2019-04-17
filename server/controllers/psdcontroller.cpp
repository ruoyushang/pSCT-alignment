/**
 * @file psdcontroller.cpp
 * @brief Source file for position sensitive device controller class
 */

#ifndef _AMD64

#include <chrono>
#include <iostream>
#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"

#include "uaserver/uaobjecttypes.h"

#include "server/controllers/psdcontroller.hpp"

#include "server/controllers/pascontroller.hpp"

#include "common/globalalignment/psdclass.h"
#include "common/opcua/pascominterfacecommon.h"
#include "common/opcua/pasobject.h"
#include "common/opcua/passervertypeids.h"


/// @details By default, sets the update interval to 500 ms. Creates a new GASPSD object,
/// sets its port, and initializes. Sets its state to On.
PSDController::PSDController(int ID) : PasController(ID, 500)
{
    m_pPSD = std::unique_ptr<GASPSD>(new GASPSD());
    m_pPSD->setPort();
    m_pPSD->Initialize();
    m_state = PASState::On;
}

/// @details Sets the device state to off.
PSDController::~PSDController()
{
    m_state = PASState::Off;
}

/// @details Calls GASPSD.getOutput() to read data. Locks the shared mutex to prevent concurrent actions while reading data.
UaStatus PSDController::getData(OpcUa_UInt32 offset, UaVariant &value)
{
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    int dataoffset = offset - PAS_PSDType_x1;
    if ((dataoffset >= 9) || (dataoffset < 0)) { // GASPSD only has 9 data elements. In future this should not be hardcoded.
        return OpcUa_BadInvalidArgument;
      }

    if (_expired()) { // if cached value expired, update it
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
    UaMutexLocker lock(&m_mutex);

    UaStatus status;
    switch ( offset )
    {
        case PAS_PSDType_Read:
            status = read();
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/// @details Calls update on the GASPSD object and revises the last update time. Locks the shared mutex while reading.
UaStatus PSDController::read()
{
    UaMutexLocker lock(&m_mutex);

    if ( m_state == PASState::On )
    {
        std::cout << "\nReading PSD " <<  m_ID << std::endl;
        m_pPSD->Update();
        m_lastUpdateTime = std::chrono::system_clock::now();
        return OpcUa_Good;
    }
    else
        return OpcUa_BadOutOfService;

    return OpcUa_BadUnexpectedError;
}

#endif
