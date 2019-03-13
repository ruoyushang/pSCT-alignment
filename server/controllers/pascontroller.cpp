/**
 * @file pascontroller.cpp
 * @brief Source file for generic device controller class
 */

#ifndef _AMD64
    #include "server/controllers/psdcontroller.hpp"
#endif

#include "uabase/statuscode.h"
#include "uabase/uamutex.h"

#include "server/device_controller/pascontroller.hpp"

#include "common/opcua/passervertypeids.h"
#include "common/alignment/platform.hpp"


/// @details Locks the shared mutex while retrieving the state.
UaStatusCode PasController::getState(PASState& state)
{
    UaMutexLocker lock(&m_mutex);
    state = m_state;
    return OpcUa_Good;
}

/// #details Does not allow setting the state to error or setting the state to
/// its current value. Locks the shared mutex while setting the state.
UaStatusCode PasController::setState(PASState state)
{
    UaMutexLocker lock(&m_mutex);
    if ( state == PASState::FatalError || state == PASState::OperableError)
    {
        return OpcUa_BadInvalidArgument;
    }
    if ( state == m_state )
    {
        return OpcUa_BadInvalidState;
    }
    m_state = state;
    return OpcUa_Good;
}
