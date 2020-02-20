#include "client/controllers/ccdcontroller.hpp"


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
    spdlog::trace("{} : Creating CCD controller... ", m_Identity);
#ifndef SIMMODE  
    m_pCCD = std::unique_ptr<GASCCD>(new GASCCD(m_Identity));
#else 
    m_pCCD = std::unique_ptr<GASCCD>(new DummyGASCCD(m_Identity));
#endif
    m_pCCD->setConfig(m_Identity.eAddress);
    spdlog::trace("{} : Initializing CCD controller... ", m_Identity);
    m_pCCD->initialize();

    m_LastUpdateTime = TIME::now() - std::chrono::duration<int, std::ratio<1, 1000>>(m_kUpdateInterval_ms);
}

/* ----------------------------------------------------------------------------
    Class        CCDController
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatus CCDController::getState(Device::DeviceState &state) {
    //UaMutexLocker lock(&m_mutex);
    state = m_State;
    spdlog::trace("{} : Read device state => ({})", m_Identity, Device::deviceStateNames.at(state));
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

    if (__expired())
        status = read();

    switch (offset){
        case PAS_CCDType_xFromLED:
            status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "x"}, &value);
            double xval;
            value.toDouble(xval);
            spdlog::trace("{} : Read x value => ({})", m_Identity, xval);
            break;
        case PAS_CCDType_yFromLED:
            status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "y"}, &value);
            double yval;
            value.toDouble(yval);
            spdlog::trace("{} : Read y value => ({})", m_Identity, yval);
            break;
        case PAS_CCDType_zFromLED:
            status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "z"}, &value);
            double zval;
            value.toDouble(zval);
            spdlog::trace("{} : Read z value => ({})", m_Identity, zval);
            break;
        case PAS_CCDType_thetaFromLED:
            status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "theta"}, &value);
            double theta;
            value.toDouble(theta);
            spdlog::trace("{} : Read theta value => ({})", m_Identity, theta);
            break;
        case PAS_CCDType_phiFromLED:
            status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "phi"}, &value);
            double phi;
            value.toDouble(phi);
            spdlog::trace("{} : Read phi value => ({})", m_Identity, phi);
            break;
        case PAS_CCDType_psiFromLED:
            status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "psi"}, &value);
            double psi;
            value.toDouble(psi);
            spdlog::trace("{} : Read psi value => ({})", m_Identity, psi);
            break;
        default:
            status = OpcUa_BadInvalidArgument;
            break;
    }

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

    spdlog::trace("{} : Setting nominal value... offset=> {} value => ({})", m_Identity, nominalOffset, val);

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
            spdlog::trace("{} : CCDController calling read()", m_Identity);
            status = read();
            break;
        case PAS_CCDType_Start:
            spdlog::warn("{} : CCDController calling start() - Does nothing for now", m_Identity);
            spdlog::trace("{} : CCDController calling start()", m_Identity);
            status = OpcUa_BadInvalidArgument;
            break;
        case PAS_CCDType_Stop:
            spdlog::warn("{} : CCDController calling start() - Does nothing for now", m_Identity);
            spdlog::trace("{} : CCDController calling stop()", m_Identity);
            status = OpcUa_BadInvalidArgument;
            break;
        case PAS_CCDType_TurnOn:
            spdlog::info("{} : CCDController calling turnOn()", m_Identity);
            status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("TurnOn"));
        case PAS_CCDType_TurnOff:
            spdlog::info("{} : CCDController calling turnOff()", m_Identity);
            status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("TurnOff"));
        default:
            spdlog::error("{} : Invalid method call with offset {}", m_Identity, offset);
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

    if (m_State == Device::DeviceState::On) {
        spdlog::trace("{} : Reading CCD...", m_Identity.name);
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
        spdlog::error("{} : CCD is off, cannot read.", m_Identity.name);
        return OpcUa_Bad;
    }
}
