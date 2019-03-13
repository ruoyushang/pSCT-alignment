/**
 * @file psdcontroller.cpp
 * @brief Source file for position sensitive device controller class
 */

#ifndef _AMD64

#include <chrono>
#include <iostream>

#include "uabase/statuscode.h"
#include "uabase/uamutex.h"
#include "uabase/uaobjecttypes.h"

#include "server/controllers/psdcontroller.hpp"

#include "common/globalalignment/psdclass.h"
#include "common/opcua/pascominterfacecommon.h"

/// @details By default, sets the update interval to 500 ms. Creates a new GASPSD object,
/// sets its port, and initializes. Sets state to On.
PSDController::PSDController(int ID) : PasController(ID, 500)
{
    m_pPSD = new GASPSD();
    m_pPSD->setPort();
    m_pPSD->Initialize();
    m_state = PASState::PAS_On;
}

/// @details Sets the device state to off.
PSDController::~PSDController()
{
    m_state = PASState::PAS_Off; // Set state to off
}

/// @details Calls GASPSD.getOutput() to read data. Locks the mutex to prevent concurrent actions while reading data.
UaStatusCode PSDController::getData(OpcUa_UInt32 offset, UaVariant& value)
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

/// @details No effect, PSDs have no writable data variables.
UaStatusCode PSDController::setData(OpcUa_UInt32 offset, UaVariant value)
{
    return OpcUa_BadNotWritable;
}

/// @details Locks the mutex to prevent concurrent actions while calling methods.
UaStatusCode PSDController::Operate(OpcUa_UInt32 offset, const UaVariantArray& args)
{
    UaMutexLocker lock(&m_mutex);

    UaStatus status;
    switch ( offset )
    {
        case 0:
            status = read();
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/// @details Calls update on the GASPSD object and updates the lastUpdateTime. Also locks the mutex while reading.
UaStatus PSDController::read()
{
    UaMutexLocker lock(&m_mutex);

    if ( m_state == PASState::PAS_On )
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
