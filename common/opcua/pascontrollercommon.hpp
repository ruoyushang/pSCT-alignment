#ifndef COMMON_PASCONTROLLERCOMMON_HPP
#define COMMON_PASCONTROLLERCOMMON_HPP

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

#include "common/opcua/components.hpp"

class PasControllerCommon
{
    UA_DISABLE_COPY(PasControllerCommon);
public:

    /* construction / destruction */
    explicit PasControllerCommon(Identity identity, int updateInterval = 0) :
            m_state(PASState::On),
            m_ID(std::move(identity)),
            m_kUpdateInterval_ms(updateInterval) {};

    virtual ~PasControllerCommon() = default;

    const Identity& getId() const {return m_ID;}

    /* Get Controller status and data */
    virtual UaStatus getState(PASState &state);

    virtual UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) = 0;

    /* set Controller status and data */
    virtual UaStatus setState(PASState state);

    virtual UaStatus setData(OpcUa_UInt32 offset, UaVariant value) = 0;

    virtual UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args = UaVariantArray()) = 0;

    virtual bool initialize() { return true; }

protected:
    UaMutex m_mutex;
    PASState m_state;
    Identity m_ID;
    
    // update interval in milliseconds
    const int m_kUpdateInterval_ms;

    // be able to check if data has expired
    typedef std::chrono::system_clock TIME;
    bool __expired() const {return (std::chrono::duration_cast<std::chrono::milliseconds>(TIME::now() - m_lastUpdateTime).count() > m_kUpdateInterval_ms); }
    std::chrono::time_point<TIME> m_lastUpdateTime;
};

#endif
