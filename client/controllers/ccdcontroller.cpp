#include "client/controllers/ccdcontroller.hpp"

#include <iostream>
#include <string>
#include <vector>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include "common/globalalignment/ccdclass.h"
#include "common/opcua/pascominterfacecommon.hpp"
#include "common/opcua/pasobject.hpp"
#include "common/opcua/passervertypeids.hpp"
#include "common/alignment/device.hpp"

#include "client/clienthelper.hpp"
#include "client/controllers/pascontroller.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"


CCDController::CCDController(Device::Identity identity) : PasController(std::move(identity), nullptr, 1000) {
    spdlog::trace("{} : Creating CCD controller... ", m_ID);
#ifndef SIMMODE  
    m_pCCD = std::unique_ptr<GASCCD>(new GASCCD());
#else 
    m_pCCD = std::unique_ptr<GASCCD>(new DummyGASCCD());
#endif
    m_pCCD->setConfig(m_ID.eAddress);
    spdlog::trace("{} : Initializing CCD controller... ", m_ID);
    m_pCCD->initialize();

    m_lastUpdateTime = TIME::now() - std::chrono::duration<int, std::ratio<1, 1000>>(m_kUpdateInterval_ms);
}

/* ----------------------------------------------------------------------------
    Class        CCDController
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatus CCDController::getState(Device::DeviceState &state) {
    //UaMutexLocker lock(&m_mutex);
    state = m_state;
    spdlog::trace("{} : Getting device state => ({})", m_ID, Device::deviceStateNames.at(state));
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        CCDController
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatus CCDController::setState(Device::DeviceState state) {
    return OpcUa_BadNotWritable;
}

/* ----------------------------------------------------------------------------
    Class        CCDController
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatus CCDController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    UaStatus status;
    //UaMutexLocker lock(&m_mutex);
    int dataoffset = offset - PAS_CCDType_xFromLED;
    if (dataoffset >= 6)
        return OpcUa_BadInvalidArgument;

    if (__expired())
        status = read(false);

    if (!m_updated)
        value.setDouble(0.);
    else
        value.setDouble(*(static_cast<const double *>(m_pCCD->getOutput() + dataoffset)));

    spdlog::trace("{} : Getting data... offset=> {} value => ({})", m_ID, offset, value[0].Value.Double);

    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        CCDController
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatus CCDController::setData(OpcUa_UInt32 offset, UaVariant value) {
    UaStatus status;

    double val;
    int nominalOffset;

    switch (offset) {
        case PAS_CCDType_xNominal:
            value.toDouble(val);
            nominalOffset = 0;
            break;
        case PAS_CCDType_yNominal:
            value.toDouble(val);
            nominalOffset = 1;
            break;
        case PAS_CCDType_zNominal:
            value.toDouble(val);
            nominalOffset = 2;
            break;
        case PAS_CCDType_thetaNominal:
            value.toDouble(val);
            nominalOffset = 3;
            break;
        case PAS_CCDType_phiNominal:
            value.toDouble(val);
            nominalOffset = 4;
            break;
        case PAS_CCDType_psiNominal:
            value.toDouble(val);
            nominalOffset = 5;
            break;
        default:
            status = OpcUa_BadNotWritable;
    }

    m_pCCD->setNominalValues(nominalOffset, val);

    spdlog::trace("{} : Setting nominal value... offset=> {} value => ({})", m_ID, nominalOffset, val);

    return OpcUa_BadNotWritable;
}

/* ----------------------------------------------------------------------------
    Class        CCDController
    Method       Operate
    Description  run a method on the sensor
-----------------------------------------------------------------------------*/
UaStatus CCDController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    switch (offset) {
        case PAS_CCDType_Read:
            spdlog::trace("{} : CCD controller calling read()", m_ID);
            status = read();
            break;
        case PAS_CCDType_Start:
            spdlog::trace("{} : CCD controller calling start()", m_ID);
            status = OpcUa_BadInvalidArgument;
            break;
        case PAS_CCDType_Stop:
            spdlog::trace("{} : CCD controller calling stop()", m_ID);
            status = OpcUa_BadInvalidArgument;
            break;
        default:
            spdlog::error("{} : Invalid method call with offset {}", m_ID, offset);
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/* ----------------------------------------------------------------------------
    Class        CCDController
    Method       read
    Description  Read Controller data.
-----------------------------------------------------------------------------*/
UaStatus CCDController::read(bool print) {
    //UaMutexLocker lock(&m_mutex);

    if (m_state == Device::DeviceState::On) {
        spdlog::trace("{} : Reading CCD...", m_ID);
        m_pCCD->update();
        m_updated = true;

        if (print) {
            spdlog::info("x (nominal): {} ({})", *m_pCCD->getOutput(), *(m_pCCD->getOutput() + 6));
            spdlog::info("y (nominal): {} ({})", *(m_pCCD->getOutput() + 1), *(m_pCCD->getOutput() + 7));
            spdlog::info("z (nominal): {} ({})", *(m_pCCD->getOutput() + 2), *(m_pCCD->getOutput() + 8));
            spdlog::info("psi (nominal): {} ({})", *(m_pCCD->getOutput() + 3), *(m_pCCD->getOutput() + 9));
            spdlog::info("theta (nominal): {} ({})", *(m_pCCD->getOutput() + 4), *(m_pCCD->getOutput() + 10));
            spdlog::info("phi (nominal): {} ({})", *(m_pCCD->getOutput() + 5), *(m_pCCD->getOutput() + 11));
        }

        return OpcUa_Good;
    } else {
        m_updated = false;
        spdlog::error("{} : CCD is off, cannot read.", m_ID);
        return OpcUa_Bad;
    }
}
