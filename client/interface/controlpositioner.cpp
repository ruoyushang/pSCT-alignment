/******************************************************************************
** Project: pSCT Alignment system
**
** Description: implementation of Positioner.
******************************************************************************/
#include "controlpositioner.h"
#include "passervertypeids.h"
#include "clienthelper.h"
#include "common/alignment/device.hpp"
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
ControlPositioner::ControlPositioner(Identity identity, Client *pClient) : PasController(identity, pClient)
{
    m_state = Device::DeviceState::On;
} 

ControlPositioner::~ControlPositioner()
{
    m_pClient = nullptr;
    m_state = Device::DeviceState::On;
}

/* ----------------------------------------------------------------------------
    Class        ControlPositioner
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatus ControlPositioner::getState(Device::DeviceState &state)
{
    UaStatus status;
    UaMutexLocker lock(&m_mutex);
    
    UaVariant res;
    std::vector< std::string > vec_curread {"ns=2;s=Application.USERVARGLOBAL_OPCUA.is_moving"};
    status = m_pClient->read(vec_curread, &res);

    if (status.isGood()) {
        res.toBool(data.isMoving);
        if (data.isMoving)
            state = Device::DeviceState::Busy;
        else
            state = Device::DeviceState::On;
    }

    return status;
}

/* ----------------------------------------------------------------------------
    Class        ControlPositioner
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatus ControlPositioner::setState(Device::DeviceState state)
{
    return OpcUa_BadInvalidArgument;
}
/* ----------------------------------------------------------------------------
    Class        ControlPositioner
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatus ControlPositioner::getData(OpcUa_UInt32 offset, UaVariant &value)
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
UaStatus ControlPositioner::setData(
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
    Method       operate
    Description  run a method on the sensor
-----------------------------------------------------------------------------*/
UaStatus ControlPositioner::operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    UaMutexLocker lock(&m_mutex);
    UaStatus status;
    
               
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
