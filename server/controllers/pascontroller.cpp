/**
 * @file pascontroller.cpp
 * @brief Source file for generic device controller class.
 */

#include "server/controllers/pascontroller.hpp"

#include "uabase/statuscode.h"
#include "uabase/uamutex.h"

#include "common/opcua/passervertypeids.h"
#include "common/alignment/platform.hpp"

#ifndef _AMD64
#include "server/controllers/psdcontroller.hpp"
#endif

/// @details Locks the shared mutex while retrieving the state.
UaStatus PasController::getState(Device::DeviceState &state) {
    UaMutexLocker lock(&m_mutex);
    state = m_state;
    return OpcUa_Good;
}

/// @details Does not allow setting the state to error or setting the state to
/// its current value. Locks the shared mutex while setting the state.
UaStatus PasController::setState(Device::DeviceState state) {
    UaMutexLocker lock(&m_mutex);
    if (state == Device::DeviceState::FatalError || state == Device::DeviceState::OperableError) {
        return OpcUa_BadInvalidArgument;
    }
    if (state == m_state) {
        return OpcUa_BadInvalidState;
    }
    m_state = state;
    return OpcUa_Good;
}
