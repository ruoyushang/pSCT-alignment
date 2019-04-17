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
    std::cout << "assigned the name " << m_ID.name << " to this CCD" << std::endl;

    if (m_state != PASState::On)
        m_state = PASState::On;
}

CCDController::~CCDController() {
    m_state = PASState::Off;
}

/* ----------------------------------------------------------------------------
    Class        CCDController
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatus CCDController::getState(PASState &state) {
    UaMutexLocker lock(&m_mutex);
    state = m_state;
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        CCDController
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatus CCDController::setState(PASState state) {
    if (state == PASState::OperableError || state == PASState::FatalError) {
        return OpcUa_BadInvalidArgument;
    }
    if (state == m_state) {
        return OpcUa_BadInvalidState;
    }
    UaMutexLocker lock(&m_mutex);

    m_state = state;
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        CCDController
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatus CCDController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    UaMutexLocker lock(&m_mutex);
    int dataoffset = offset - PAS_CCDType_xFromLED;
    if (dataoffset >= 6)
        return OpcUa_BadInvalidArgument;

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
    int dataoffset = offset - PAS_CCDType_xFromLED;
    OpcUa_Double val;
    value.toDouble(val);
    if ((dataoffset > 5) && (dataoffset < 12)) {
        m_pCCD->setNominalValues(dataoffset - 6, val);
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
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (offset >= 1)
        return OpcUa_BadInvalidArgument;

    switch (offset) {
        case 0:
            status = read();
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
    UaMutexLocker lock(&m_mutex);

    if (m_state == PASState::On) {
        printf("\nReading CCD %s\n", m_ID.name.c_str());
        m_pCCD->Update();
        m_updated = true;
        return OpcUa_Good;
    } else
        m_updated = false;

    return OpcUa_Bad;
}