/******************************************************************************
** Project: pSCT Alignment system
**
** Description: implementation of Positioner.
******************************************************************************/
#include "positionercontroller.hpp"
#include "passervertypeids.h"
#include "clienthelper.h"
#include "components.h"
#include "stdlib.h"
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <unistd.h>

/* ----------------------------------------------------------------------------
    Begin Class    ControlPositioner
    constructors / destructors
-----------------------------------------------------------------------------*/
PositionerController::PositionerController(Identity identity, Client *pClient) : PasController(identity, pClient)
{
    m_state = PASState::GLOB_Positioner_notMoving;
}

PositionerController::~PositionerController()
{
    m_pClient = nullptr;
    m_state = PASState::GLOB_Positioner_notMoving;
}
/* ----------------------------------------------------------------------------
    Class        ControlPositioner
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatus PositionerController::getState(PASState &state)
{
    UaStatus status;
    UaMutexLocker lock(&m_mutex);
    
    UaVariant res;
    std::vector< std::string > vec_curread {"ns=2;s=Application.USERVARGLOBAL_OPCUA.is_moving"};
    status = m_pClient->read(vec_curread, &res);

    if (status.isGood()) {
        res.toBool(data.isMoving);
        if (data.isMoving)
            state = PASState::GLOB_Positioner_Moving;
        else 
            state = PASState::GLOB_Positioner_notMoving;
    }

    return status;
}
/* ----------------------------------------------------------------------------
    Class        ControlPositioner
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatus PositionerController::setState(PASState state)
{
    return OpcUa_BadInvalidArgument;
}
/* ----------------------------------------------------------------------------
    Class        ControlPositioner
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatus PositionerController::getData(OpcUa_UInt32 offset, UaVariant &value)
{
    UaStatus status;
    UaMutexLocker lock(&m_mutex);

    std::vector<std::string> varstoread {"is_moving", "current_position.az", "current_position.el",
                             "in_position.az", "in_position.el", "current_energy_level"};

    int dataoffset = offset - GLOB_PositionerType_isMoving;
    if ( dataoffset > varstoread.size() )
        return OpcUa_BadInvalidArgument;

    std::vector<std::string> vec_curread {"ns=2;s=Application.USERVARGLOBAL_OPCUA." + varstoread[dataoffset]};

    status = m_pClient->read(vec_curread, &value);
    if (status.isGood()) {
        if (offset == GLOB_PositionerType_isMoving)
            value.toBool(data.isMoving);
        else
            value.toFloat(*(reinterpret_cast<OpcUa_Float *>(&data) + dataoffset));
    }

    return status;
}
/* ----------------------------------------------------------------------------
    Class        ControlPositioner
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatus PositionerController::setData(
        OpcUa_UInt32 offset,
        UaVariant value)
{
    UaStatus status;

    int dataoffset = offset - GLOB_PositionerType_isMoving;
    std::string positions[2] {"az", "el"};
    std::vector<std::string> varstowrite {"ns=2;s=Application.USERVARGLOBAL_OPCUA.in_position." + positions[dataoffset-3]};

    if ( (dataoffset > 4) || (dataoffset < 3))
        return OpcUa_BadNotWritable;
    else 
    {
        value.toFloat(*(reinterpret_cast<OpcUa_Float *>(&data) + dataoffset));
        status = m_pClient->write(varstowrite, &value);
    }

    return status;
}
/* ----------------------------------------------------------------------------
    Class        ControlPositioner
    Method       Operate
    Description  run a method on the sensor
-----------------------------------------------------------------------------*/
UaStatusCode PositionerController::Operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    UaMutexLocker lock(&m_mutex);
    UaStatusCode  status;
    
               
    switch ( offset )
    {
        case GLOB_PositionerType_SetEnergy:
            {
                // get current energy level and toggle it
                UaVariant energy_level;
                getData(GLOB_PositionerType_EnergyLevel, energy_level);
                OpcUa_Int16 curEnergyLevel;
                energy_level.toInt16(curEnergyLevel);
                curEnergyLevel = (curEnergyLevel + 1) % 2;
                energy_level.setInt16(curEnergyLevel);

                status = m_pClient->write({"ns=2;s=Application.USERVARGLOBAL_OPCUA.in_energy_level"},
                                                &energy_level);

                UaVariant set_energy_level = UaVariant(OpcUa_Int16(7));
                status = m_pClient->write({"ns=2;s=Application.USERVARGLOBAL_OPCUA.in_command"},
                                                &set_energy_level);
                usleep(100000);
            }
        case GLOB_PositionerType_Init:
            {
                /* INITIALIZE */
                UaVariant init_drive_system = UaVariant(OpcUa_Int16(8));
                status = m_pClient->write({"ns=2;s=Application.USERVARGLOBAL_OPCUA.in_command"},
                                                &init_drive_system);
                UA_ASSERT(status.isGood());
                /* DONE */
                break;
            }
        case GLOB_PositionerType_Move:
            {
                /* MOVE TO */
                std::vector<std::string> varstowrite {"ns=2;s=Application.USERVARGLOBAL_OPCUA.in_command"};
                UaVariant valstowrite(OpcUa_Int16(13));
                status = m_pClient->write(varstowrite, &valstowrite);
                UA_ASSERT(status.isGood());
                break;
            }
        case GLOB_PositionerType_Stop:
            {
                /* STOP */
                std::vector<std::string> varstowrite {"ns=2;s=Application.USERVARGLOBAL_OPCUA.in_command"};
                UaVariant valstowrite(OpcUa_Int16(14));
                status = m_pClient->write(varstowrite, &valstowrite);
                UA_ASSERT(status.isGood());
                break;
            }

        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}
/* ----------------------------------------------------------------------------
    End Class    ControlPositioner
==============================================================================*/
