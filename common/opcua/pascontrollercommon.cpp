#include "common/opcua/pascontrollercommon.hpp"

#include "passervertypeids.hpp"
#include "common/simulatestewart/stewartplatform.h"
#include "components.hpp"

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include <vector>
#include <set>
#include <map>
#include <chrono>
#include <iostream>

#include "common/opcua/pascominterfacecommon.hpp"
#include "pascominterfacecommon.hpp"

/// @details Locks the shared mutex while retrieving the state.
UaStatus PasControllerCommon::getState(PASState &state) {
    //UaMutexLocker lock(&m_mutex);
    state = m_state;
    return OpcUa_Good;
}

/// @details Does not allow setting the state to error or setting the state to
/// its current value. Locks the shared mutex while setting the state.
UaStatus PasControllerCommon::setState(PASState state) {
    //UaMutexLocker lock(&m_mutex);
    if (state == PASState::FatalError || state == PASState::OperableError) {
        return OpcUa_BadInvalidArgument;
    }
    if (state == m_state) {
        return OpcUa_BadInvalidState;
    }
    m_state = state;
    return OpcUa_Good;
}