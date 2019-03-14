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
#include "common/opcua/pascominterfacecommon.h"
#include "common/opcua/passervertypeids.h"

/// @details Turns state to On.
MPESController::MPESController(int ID, std::shared_ptr<Platform> pPlatform) : PasController(ID, pPlatform)
{
    m_state = PASState::On;
}

/// @details Turns state to Off.
MPESController::~MPESController()
{
    m_state = PASState::Off;
}

/// @details Sets exposure for this MPES.
int MPESController::Initialize()
{   int ret = m_pPlatform->getMPESAt(m_ID)->setExposure();
    if (ret > 0) {
      return 0;
    }
    else {
      return -1;
    }
}

/// @details If the MPES has not been read yet, calls read before retrieving data. Locks the mutex while reading data.
UaStatus MPESController::getData(OpcUa_UInt32 offset, UaVariant& value)
{
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (!m_updated)
        status = read();

    int dataOffset = offset - PAS_MPESType_xCentroidAvg;
    const MPES::Position& position = m_pPlatform->getMPESAt(m_ID)->getPosition();
    float data;
    switch ( dataOffset )
    {
        case 0:
            value.setFloat(position.xCenter);
            break;
        case 1:
            value.setFloat(position.yCenter);
            break;
        case 2:
            value.setFloat(position.xStdDev);
            break;
        case 3:
            value.setFloat(position.yStdDev);
            break;
        case 4:
            value.setFloat(position.CleanedIntensity);
            break;
        case 5:
            value.setFloat(position.xNominal);
            break;
        case 6:
            value.setFloat(position.yNominal);
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return OpcUa_Good;
}

/// @details Locks the mutex while writing data.
UaStatus MPESController::setData(OpcUa_UInt32 offset, UaVariant value)
{
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    int dataOffset = offset - PAS_MPESType_xCentroidAvg;
    float v;
    value.toFloat(v);
    switch ( dataOffset )
    {
        case 5:
            m_pPlatform->getMPESAt(m_ID)->setxNominalPosition(v);
            break;
        case 6:
            m_pPlatform->getMPESAt(m_ID)->setyNominalPosition(v);
            break;
        default:
            status = OpcUa_BadNotWritable;
    }

    return status;
}

/// @details Locks the mutex while calling methods.
UaStatus MPESController::Operate(OpcUa_UInt32 offset, const UaVariantArray& args)
{
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    switch ( offset )
    {
        case PAS_MPESType_Read:
            status = read();
            break;
        case PAS_MPESType_SetExposure:
            m_pPlatform->getMPESAt(m_ID)->setExposure();
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/// @details If state is On, calls the ReadMPES method through the Platform object and sets the m_updated flag to true.
OpcUa_Int32 MPESController::read()
{
    UaMutexLocker lock(&m_mutex);

    if ( m_state == PASState::On )
    {
        std::cout << "\nReading MPES " << m_ID << std::endl;
        m_pPlatform->ReadMPES(m_ID);
        m_updated = true;
        return 0;
    }
    else
        m_updated = false;

    return -1;
}
