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

#include "client/clienthelper.hpp"
#include "client/controllers/pascontroller.hpp"


CCDController::CCDController(Identity identity) : PasController(std::move(identity), nullptr) {
    m_pCCD = new GASCCD();
    m_pCCD->setConfig(m_ID.eAddress);
    m_pCCD->Initialize();

    m_ID.serialNumber = m_pCCD->getSerial();
    m_ID.eAddress = m_pCCD->getAddress();
    m_ID.name = m_pCCD->getName();

    m_state = Device::DeviceState::On;
}

CCDController::~CCDController() {
    m_state = Device::DeviceState::Off;
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
    if (state == Device::DeviceState::OperableError || state == Device::DeviceState::FatalError) {
        return OpcUa_BadInvalidArgument;
    }
    if (state == m_state) {
        return OpcUa_BadInvalidState;
    }
    //UaMutexLocker lock(&m_mutex);

    m_state = state;
    return OpcUa_Good;
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

    switch (offset) {
        case PAS_CCDType_xFromLED:
            status = read();
            break;
        case PAS_CCDType_yFromLED:
            status = read();
            break;
        case PAS_CCDType_zFromLED:
            status = read();
            break;
        case PAS_CCDType_thetaFromLED:
            status = read();
            break;
        case PAS_CCDType_phiFromLED:
            status = read();
            break;
        case PAS_CCDType_psiFromLED:
            status = read();
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }


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
    if ((offset > PAS_CCDType_xNominal) && (offset < PAS_CCDType_phiNominal)) {
        double val;
        value.toDouble(val);
        m_pCCD->setNominalValues(offset - PAS_CCDType_xNominal, val);
        return OpcUa_Good;
    }

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
            status = setState(Device::DeviceState::On);
            break;
        case PAS_CCDType_Stop:
            status = setState(Device::DeviceState::Off);
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
UaStatus CCDController::read() {
    //UaMutexLocker lock(&m_mutex);

    if (m_state == Device::DeviceState::On) {
        std::cout << "Reading CCD " << m_ID.name.c_str() << std::endl;
        m_pCCD->Update();
        m_updated = true;
        return OpcUa_Good;
    } else {
        m_updated = false;
        return OpcUa_Bad;
    }
}