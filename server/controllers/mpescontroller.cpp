/**
 * @file mpescontroller.cpp
 * @brief Source file for mirror panel edge sensor device controller
 */

#include "server/controllers/mpescontroller.hpp"

#include <iostream>
#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"

#include "common/alignment/platform.hpp"
#include "common/opcua/pascominterfacecommon.hpp"
#include "common/opcua/passervertypeids.hpp"

/// @details Turns state to On.
MPESController::MPESController(Identity identity, std::shared_ptr<Platform> pPlatform) : PasController(identity,
                                                                                                       pPlatform) {
    m_state = PASState::On;
}

/// @details Turns state to Off.
MPESController::~MPESController() {
    m_state = PASState::Off;
}

/// @details Sets exposure for this MPES.
bool MPESController::initialize() {
    int ret = m_pPlatform->getMPESAt(std::stoi(m_ID.eAddress))->setExposure();
    if (ret > 0) {
        return true;
    } else {
        return false;
    }
}

/// @details If the MPES has not been read yet, calls read before retrieving data. Locks the shared mutex while reading data.
UaStatus MPESController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (!m_updated)
        status = read();

    const MPES::Position &position = m_pPlatform->getMPESAt(std::stoi(m_ID.eAddress))->getPosition();
    switch (offset) {
        case PAS_MPESType_xCentroidAvg:
            value.setFloat(position.xCentroid);
            break;
        case PAS_MPESType_yCentroidAvg:
            value.setFloat(position.yCentroid);
            break;
        case PAS_MPESType_xCentroidSpotWidth:
            value.setFloat(position.xSpotWidth);
            break;
        case PAS_MPESType_yCentroidSpotWidth:
            value.setFloat(position.ySpotWidth);
            break;
        case PAS_MPESType_CleanedIntensity:
            value.setFloat(position.CleanedIntensity);
            break;
        case PAS_MPESType_xCentroidNominal:
            value.setFloat(position.xNominal);
            break;
        case PAS_MPESType_yCentroidNominal:
            value.setFloat(position.yNominal);
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/// @details Locks the mutex while writing data.
UaStatus MPESController::setData(OpcUa_UInt32 offset, UaVariant value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    float v;
    value.toFloat(v);
    switch (offset) {
        case PAS_MPESType_xCentroidNominal:
            m_pPlatform->getMPESAt(std::stoi(m_ID.eAddress))->setxNominalPosition(v);
            break;
        case PAS_MPESType_yCentroidNominal:
            m_pPlatform->getMPESAt(std::stoi(m_ID.eAddress))->setyNominalPosition(v);
            break;
        default:
            status = OpcUa_BadNotWritable;
    }

    return status;
}

/// @details Locks the shared mutex while calling methods.
UaStatus MPESController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    switch (offset) {
        case PAS_MPESType_Start:
            status = OpcUa_BadNotImplemented;
            break;
        case PAS_MPESType_Stop:
            status = OpcUa_BadNotImplemented;
            break;
        case PAS_MPESType_Read:
            status = read();
            break;
        case PAS_MPESType_SetExposure:
            m_pPlatform->getMPESAt(std::stoi(m_ID.eAddress))->setExposure();
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }
    return status;
}

/// @details If state is On, calls the ReadMPES method through the Platform object and sets the m_updated flag to true.
/// Locks the shared mutex while reading.
UaStatus MPESController::read() {
    //UaMutexLocker lock(&m_mutex);
    if (m_state == PASState::On) {
        std::cout << "\nReading MPES " << m_ID << std::endl;
        m_pPlatform->ReadMPES(std::stoi(m_ID.eAddress));
        m_updated = true;
        return OpcUa_Good;
    } 
    else {
        m_updated = false;
        return OpcUa_Bad;
    }
}
