/******************************************************************************
** Copyright (C) 2006-2015 Unified Automation GmbH. All Rights Reserved.
** Web: http://www.unifiedautomation.com
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Project: Sample OPC server
**
** Description: Simulation for the building automation the controllers.
******************************************************************************/
#include "bacontrollersimulation.h"
#include "stdlib.h"

/* ----------------------------------------------------------------------------
    Begin Class    BaAirConditionerSimulation
    constructors / destructors
-----------------------------------------------------------------------------*/
BaAirConditionerSimulation::BaAirConditionerSimulation()
{
    m_state               = BaCommunicationInterface::Ba_ControllerState_On;
    m_random = (1000 + (((double) rand() / (double) 2000) * 2000))/40000;
    m_temperature         = 72 + (m_random * 10);
    m_temperatureSetPoint = 72;
    m_powerConsumption    = 0;
    m_humidity            = 60 + (m_random * 5);
    m_humiditySetpoint    = 60;
}
BaAirConditionerSimulation::~BaAirConditionerSimulation()
{
}
/* ----------------------------------------------------------------------------
    Class        BaAirConditionerSimulation
    Method       getControllerState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatusCode BaAirConditionerSimulation::getControllerState(
    BaCommunicationInterface::ControllerState& state)
{
    UaMutexLocker lock(&m_mutex);
    state = m_state;
    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        BaAirConditionerSimulation
    Method       setControllerState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatusCode BaAirConditionerSimulation::setControllerState(
    BaCommunicationInterface::ControllerState state)
{
    if ( state == BaCommunicationInterface::Ba_ControllerState_Error )
    {
        return OpcUa_BadInvalidArgument;
    }
    if ( state == m_state )
    {
        return OpcUa_BadInvalidState;
    }
    UaMutexLocker lock(&m_mutex);
    m_state = state;
    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        BaAirConditionerSimulation
    Method       getControllerData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode BaAirConditionerSimulation::getControllerData(
    OpcUa_UInt32    offset,
    OpcUa_Double&   value)
{
    if ( offset >= 5 )
    {
        return OpcUa_BadInvalidArgument;
    }

    UaMutexLocker lock(&m_mutex);

    switch ( offset )
    {
    case 0:
        value = m_temperature;
        break;
    case 1:
        value = m_temperatureSetPoint;
        break;
    case 2:
        value = m_powerConsumption;
        break;
    case 3:
        value = m_humidity;
        break;
    case 4:
        value = m_humiditySetpoint;
        break;
    default:
        return OpcUa_BadInvalidArgument;
    }

    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        BaAirConditionerSimulation
    Method       setControllerData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode BaAirConditionerSimulation::setControllerData(
    OpcUa_UInt32    offset,
    OpcUa_Double    value)
{
    if ( offset >= 5 )
    {
        return OpcUa_BadInvalidArgument;
    }

    UaMutexLocker lock(&m_mutex);
    UaStatusCode  status;

    switch ( offset )
    {
    case 0:
        status = OpcUa_BadNotWritable;
        break;
    case 1:
        m_temperatureSetPoint = value;
        break;
    case 2:
        status = OpcUa_BadNotWritable;
        break;
    case 3:
        status = OpcUa_BadNotWritable;
        break;
    case 4:
        m_humiditySetpoint = value;
        break;
    default:
        status = OpcUa_BadInvalidArgument;
    }

    return status;
}
/* ----------------------------------------------------------------------------
    Class        BaAirConditionerSimulation
    Method       simulate
    Description  Simulate Controller data.
-----------------------------------------------------------------------------*/
void BaAirConditionerSimulation::simulate()
{
    UaMutexLocker lock(&m_mutex);

    if ( m_state == BaCommunicationInterface::Ba_ControllerState_On )
    {
        if ( m_temperature > m_temperatureSetPoint )
        {
            m_temperature = m_temperature - m_random;
            m_powerConsumption = m_random * 500;
            if ( m_humidity > m_humiditySetpoint )
            {
                m_humidity = m_humidity - (m_random/10);
            }
            else
            {
                m_humidity = m_humidity - (m_random / 100);
            }
        }
        else
        {
            m_temperature = m_temperature + m_random/5;
            m_powerConsumption = 0;
            m_humidity = m_humidity + (m_random / 100);
        }
    }
    else
    {
        m_powerConsumption = 0;
        if ( m_temperature <= (m_temperatureSetPoint * 1.1) )
        {
            m_temperature = m_temperature + m_random;
        }
        else
        {
            m_temperature = m_temperature - (m_random/10);
        }
        if ( m_humidity <= (m_humiditySetpoint * 1.1) )
        {
            m_humidity = m_humidity + (m_random / 100);
        }
        else
        {
            m_humidity = m_humidity - (m_random / 1000);
        }
    }

    if ( m_humidity > 95 )
    {
        m_humidity = 95 - (m_random * 5);
    }
}
/* ----------------------------------------------------------------------------
    Begin Class    BaAirConditionerSimulation
    constructors / destructors
-----------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------
    Begin Class    BaFurnaceSimulation
    constructors / destructors
-----------------------------------------------------------------------------*/
BaFurnaceSimulation::BaFurnaceSimulation()
{
    m_state               = BaCommunicationInterface::Ba_ControllerState_On;
    m_random = (1000 + (((double) rand() / (double) 2000) * 2000))/40000;
    m_temperature         = 72 - (m_random * 10);
    m_temperatureSetPoint = 72;
    m_powerConsumption    = 0;
    m_gasFlow             = 0;
}
BaFurnaceSimulation::~BaFurnaceSimulation()
{
}
/* ----------------------------------------------------------------------------
    Class        BaFurnaceSimulation
    Method       getControllerState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatusCode BaFurnaceSimulation::getControllerState(
    BaCommunicationInterface::ControllerState& state)
{
    UaMutexLocker lock(&m_mutex);
    state = m_state;
    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        BaFurnaceSimulation
    Method       setControllerState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatusCode BaFurnaceSimulation::setControllerState(
    BaCommunicationInterface::ControllerState state)
{
    if ( state == BaCommunicationInterface::Ba_ControllerState_Error )
    {
        return OpcUa_BadInvalidArgument;
    }
    if ( state == m_state )
    {
        return OpcUa_BadInvalidState;
    }
    UaMutexLocker lock(&m_mutex);
    m_state = state;
    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        BaFurnaceSimulation
    Method       getControllerData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode BaFurnaceSimulation::getControllerData(
    OpcUa_UInt32    offset,
    OpcUa_Double&   value)
{
    if ( offset >= 4 )
    {
        return OpcUa_BadInvalidArgument;
    }

    UaMutexLocker lock(&m_mutex);

    switch ( offset )
    {
    case 0:
        value = m_temperature;
        break;
    case 1:
        value = m_temperatureSetPoint;
        break;
    case 2:
        value = m_powerConsumption;
        break;
    case 3:
        value = m_gasFlow;
        break;
    default:
        return OpcUa_BadInvalidArgument;
    }

    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        BaFurnaceSimulation
    Method       setControllerData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode BaFurnaceSimulation::setControllerData(
    OpcUa_UInt32    offset,
    OpcUa_Double    value)
{
    if ( offset >= 4 )
    {
        return OpcUa_BadInvalidArgument;
    }

    UaMutexLocker lock(&m_mutex);
    UaStatusCode  status;

    switch ( offset )
    {
    case 0:
        status = OpcUa_BadNotWritable;
        break;
    case 1:
        m_temperatureSetPoint = value;
        break;
    case 2:
        status = OpcUa_BadNotWritable;
        break;
    case 3:
        status = OpcUa_BadNotWritable;
        break;
    default:
        status = OpcUa_BadInvalidArgument;
    }

    return status;
}
/* ----------------------------------------------------------------------------
    Class        BaFurnaceSimulation
    Method       simulate
    Description  Simulate Controller data.
-----------------------------------------------------------------------------*/
void BaFurnaceSimulation::simulate()
{
    UaMutexLocker lock(&m_mutex);

    if ( m_state == BaCommunicationInterface::Ba_ControllerState_On )
    {
        if ( m_temperature < m_temperatureSetPoint )
        {
            m_temperature = m_temperature + m_random;
            m_powerConsumption = m_random * 10;
        }
        else
        {
            m_temperature = m_temperature - m_random/5;
            m_powerConsumption = m_random;
        }
    }
    else
    {
        m_powerConsumption = 0;
        if ( m_temperature >= (m_temperatureSetPoint * 1.1) )
        {
            m_temperature = m_temperature - m_random/5;
        }
        else
        {
            m_temperature = m_temperature + (m_random/10);
        }
   }
}
/* ----------------------------------------------------------------------------
    Begin Class    BaFurnaceSimulation
    constructors / destructors
-----------------------------------------------------------------------------*/


