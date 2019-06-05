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
            state = Device::DeviceState::Busy;
        else
            state = Device::DeviceState::On;
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

    std::string varToRead;

    switch (offset) {
        case GLOB_PositionerType_isMoving:
            varToRead = "is_moving";
            break;
        case GLOB_PositionerType_curAz:
            varToRead = "current_position.az";
            break;
        case GLOB_PositionerType_curEl:
            varToRead = "current_position.el";
            break;
        case GLOB_PositionerType_inAz:
            varToRead = "in_position.az";
            break;
        case GLOB_PositionerType_inEl:
            varToRead = "in_position.el";
            break;
        case GLOB_PositionerType_EnergyLevel:
            varToRead = "current_energy_level";
            break;
        default:
            return OpcUa_BadInvalidArgument;
    }

    std::vector<std::string> vec_curread{"ns=2;s=Application.USERVARGLOBAL_OPCUA." + varToRead};

    status = m_pClient->read(vec_curread, &value);
    if (status.isGood()) {
        if (offset == GLOB_PositionerType_isMoving)
            value.toBool(m_Data.isMoving);
        else if (offset == GLOB_PositionerType_curAz)
            value.toFloat(m_Data.curAz);
        else if (offset == GLOB_PositionerType_curEl)
            value.toFloat(m_Data.curEl);
        else if (offset == GLOB_PositionerType_inAz)
            value.toFloat(m_Data.inAz);
        else if (offset == GLOB_PositionerType_inEl)
            value.toFloat(m_Data.inEl);
        else if (offset == GLOB_PositionerType_EnergyLevel)
            value.toInt16(m_Data.energyLevel);
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
    std::string varToWrite;

    switch (offset) {
        case GLOB_PositionerType_inAz:
            varToWrite = "in_position.az";
            break;
        case GLOB_PositionerType_inEl:
            varToWrite = "in_position.el";
            break;
        default:
            return OpcUa_BadInvalidArgument;
    }

    std::vector<std::string> varstowrite{"ns=2;s=Application.USERVARGLOBAL_OPCUA." + varToWrite};

    if (offset == GLOB_PositionerType_inAz)
        value.setFloat(m_Data.inAz);
    else if (offset == GLOB_PositionerType_inEl)
        value.setFloat(m_Data.inEl);

    status = m_pClient->write(varstowrite, &value);

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

    UaVariant var;
    OpcUa_Int16 curEnergyLevel;
    std::vector<std::string> varstowrite{"ns=2;s=Application.USERVARGLOBAL_OPCUA.in_command"};
    UaVariant valstowrite;

    switch ( offset )
    {
        case GLOB_PositionerType_SetEnergy:
            // get current energy level and input toggled value before calling setEnergy level
            getData(GLOB_PositionerType_EnergyLevel, var);

            var.toInt16(curEnergyLevel);
            curEnergyLevel = (curEnergyLevel + 1) % 2;
            var.setInt16(curEnergyLevel);
            status = m_pClient->write({"ns=2;s=Application.USERVARGLOBAL_OPCUA.in_energy_level"},
                                      &var);

            valstowrite = UaVariant(OpcUa_Int16(7));
            break;
        case GLOB_PositionerType_Initialize:
            valstowrite = UaVariant(OpcUa_Int16(8));
            break;
        case GLOB_PositionerType_Move:
            valstowrite = UaVariant(OpcUa_Int16(13));
            break;
        case GLOB_PositionerType_Stop:
            valstowrite = UaVariant(OpcUa_Int16(14));
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }

    status = m_pClient->write(varstowrite, &valstowrite);
    UA_ASSERT(status.isGood());

    if (offset == GLOB_PositionerType_SetEnergy) {
        usleep(100000); // ???
    }

    return status;
}
/* ----------------------------------------------------------------------------
    End Class    PositionerController
==============================================================================*/
