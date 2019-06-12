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


CCDController::CCDController(Device::Identity identity) : PasController(std::move(identity), nullptr, 1000) {
#ifndef SIMMODE  
    m_pCCD = std::unique_ptr<GASCCD>(new GASCCD());
#else 
    m_pCCD = std::unique_ptr<GASCCD>(new DummyGASCCD());
#endif
    m_pCCD->setConfig(m_ID.eAddress);
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

    if (!__expired())
        status = read(false);

    if (!m_updated)
        value.setDouble(0.);
    else
        value.setDouble(*(static_cast<const double *>(m_pCCD->getOutput() + dataoffset)));

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
            status = read();
            break;
        case PAS_CCDType_Start:
            status = OpcUa_BadInvalidArgument;
            break;
        case PAS_CCDType_Stop:
            status = OpcUa_BadInvalidArgument;
            break;
        default:
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
        std::cout << "Reading CCD " << m_ID.name.c_str() << std::endl;
        m_pCCD->update();
        m_updated = true;

        if (print) {
            std::cout << "Reading CCD " << m_ID << ":" << std::endl;
            std::cout << "x (nominal): " << *m_pCCD->getOutput() << " (" << *(m_pCCD->getOutput() + 6) << ")"
                      << std::endl;
            std::cout << "y (nominal): " << *(m_pCCD->getOutput() + 1) << " (" << *(m_pCCD->getOutput() + 7) << ")"
                      << std::endl;
            std::cout << "z (nominal): " << *(m_pCCD->getOutput() + 2) << " (" << *(m_pCCD->getOutput() + 8) << ")"
                      << std::endl;
            std::cout << "psi (nominal): " << *(m_pCCD->getOutput() + 3) << " (" << *(m_pCCD->getOutput() + 9) << ")"
                      << std::endl;
            std::cout << "theta (nominal): " << *(m_pCCD->getOutput() + 4) << " (" << *(m_pCCD->getOutput() + 10) << ")"
                      << std::endl;
            std::cout << "phi (nominal): " << *(m_pCCD->getOutput() + 5) << " (" << *(m_pCCD->getOutput() + 11) << ")"
                      << std::endl;
        }

        return OpcUa_Good;
    } else {
        m_updated = false;
        return OpcUa_Bad;
    }
}
