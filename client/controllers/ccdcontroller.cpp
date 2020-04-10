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
            double xval;
            xval = m_pCCD->m_pPosition.x;
            value.toDouble(xval);
            spdlog::trace("{} : Read x value => ({})", m_Identity, xval);
            break;
        case PAS_CCDType_yFromLED:
            double yval;
            yval = m_pCCD->m_pPosition.y;
            value.toDouble(yval);
            spdlog::trace("{} : Read y value => ({})", m_Identity, yval);
            break;
        case PAS_CCDType_zFromLED:
            double zval;
            zval = m_pCCD->m_pPosition.z;
            value.toDouble(zval);
            spdlog::trace("{} : Read z value => ({})", m_Identity, zval);
            break;
        case PAS_CCDType_thetaFromLED:
            double theta;
            theta = m_pCCD->m_pPosition.theta;
            value.toDouble(theta);
            spdlog::trace("{} : Read theta value => ({})", m_Identity, theta);
            break;
        case PAS_CCDType_phiFromLED:
            double phi;
            phi = m_pCCD->m_pPosition.phi;
            value.toDouble(phi);
            spdlog::trace("{} : Read phi value => ({})", m_Identity, phi);
            break;
        case PAS_CCDType_psiFromLED:
            double psi;
            psi = m_pCCD->m_pPosition.psi;
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
            m_pCCD->turnOn();
            status = OpcUa_Good;
            break;
        case PAS_CCDType_TurnOff:
            spdlog::info("{} : CCDController calling turnOff()", m_Identity);
            m_pCCD->turnOff();
            break;
        default:
            spdlog::error("{} : Invalid method call with offset {}", m_Identity, offset);
            status = OpcUa_BadInvalidArgument;
            break;
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
        spdlog::trace("{} : Updating CCD...", m_Identity.name);
        m_updated = m_pCCD->update();
        if (!m_updated){
            spdlog::error("{} : CCD is off, cannot read.", m_Identity.name);
            return OpcUa_Bad;
        }
        GASCCD::Position m_pPosition = m_pCCD->getPosition();

        if (print) {
            spdlog::info("{} : CCD results...", m_pCCD->getIdentity().name);
            spdlog::info("x: {}", m_pPosition.x);
            spdlog::info("y: {}", m_pPosition.y);
            spdlog::info("z: {}", m_pPosition.z);
            spdlog::info("psi: {}", m_pPosition.psi);
            spdlog::info("theta: {}", m_pPosition.theta);
            spdlog::info("phi: {}", m_pPosition.phi);
        }
        char buf[80];
        time_t t = time(0);
        struct tm *now = localtime(&t);
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %X", now);

        UaString sql_stmt = UaString(
                "%1, %2, %3, %4, %5, %6, %7, %8 \n").arg(buf).arg(m_pCCD->getIdentity().name.c_str()).arg(m_pPosition.x).arg(m_pPosition.y).arg(m_pPosition.z).arg(m_pPosition.psi).arg(m_pPosition.theta).arg(m_pPosition.phi);
        std::ofstream sql_file("CCD_readings.txt", std::ios_base::app);
        sql_file << sql_stmt.toUtf8();

        spdlog::trace("{} : Recorded MPES measurement SQL statement into CCD_readings.txt file: {} ", m_Identity.name,
                      sql_stmt.toUtf8());

        return OpcUa_Good;
    } else {
        m_updated = false;
        spdlog::error("{} : CCD is off, cannot read.", m_Identity.name);
        return OpcUa_Bad;
    }
}
