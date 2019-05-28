/******************************************************************************
** Project: pSCT Alignment system
**
** Description: implementation of Positioner.
******************************************************************************/
#include "client/controllers/positionercontroller.hpp"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "common/alignment/device.hpp"
#include "common/opcua/passervertypeids.hpp"

#include "client/clienthelper.hpp"


PositionerController::PositionerController(Device::Identity identity, std::shared_ptr<Client> pClient) : PasController(
    std::move(identity),
    std::move(pClient)), m_Data()
{
    m_state = Device::ErrorState::On;
    m_Data = {0.0, 0.0, 0.0, 0.0, OpcUa_False};
}

PositionerController::~PositionerController()
{
    m_pClient = nullptr;
    m_state = Device::ErrorState::On;
}


UaStatus PositionerController::getState(Device::DeviceState &state)
{
    UaStatus status;
    //UaMutexLocker lock(&m_mutex);
    
    UaVariant res;
    std::vector< std::string > vec_curread {"ns=2;s=Application.USERVARGLOBAL_OPCUA.is_moving"};
    status = m_pClient->read(vec_curread, &res);

    if (status.isGood()) {
        res.toBool(m_Data.isMoving);
        if (m_Data.isMoving)
            state = Device::ErrorState::Busy;
        else
            state = Device::ErrorState::On;
    }

    return status;
}

/* ----------------------------------------------------------------------------
    Class        PositionerController
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatus PositionerController::setState(Device::DeviceState state)
{
    return OpcUa_BadInvalidArgument;
}

/* ----------------------------------------------------------------------------
    Class        PositionerController
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatus PositionerController::getData(OpcUa_UInt32 offset, UaVariant &value)
{
    UaStatus status;
    //UaMutexLocker lock(&m_mutex);

    std::vector<std::string> varstoread {"is_moving", "current_position.az", "current_position.el",
                             "in_position.az", "in_position.el", "current_energy_level"};

    int dataoffset = offset - GLOB_PositionerType_isMoving;
    if (dataoffset > (int) varstoread.size())
        return OpcUa_BadInvalidArgument;

    std::vector<std::string> vec_curread {"ns=2;s=Application.USERVARGLOBAL_OPCUA." + varstoread[dataoffset]};

    status = m_pClient->read(vec_curread, &value);
    if (status.isGood()) {
        if (offset == GLOB_PositionerType_isMoving)
            value.toBool(m_Data.isMoving);
        else
            value.toFloat(*(reinterpret_cast<OpcUa_Float *>(&m_Data) + dataoffset));
    }

    return status;
}

/* ----------------------------------------------------------------------------
    Class        PositionerController
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
        value.toFloat(*(reinterpret_cast<OpcUa_Float *>(&m_Data) + dataoffset));
        status = m_pClient->write(varstowrite, &value);
    }

    return status;
}

/* ----------------------------------------------------------------------------
    Class        PositionerController
    Method       Operate
    Description  run a method on the sensor
-----------------------------------------------------------------------------*/
UaStatus PositionerController::operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    //UaMutexLocker lock(&m_mutex);
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
        case GLOB_PositionerType_Initialize:
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
    End Class    PositionerController
==============================================================================*/
