/******************************************************************************
** Copyright (C) 2006-2015 Unified Automation GmbH. All Rights Reserved.
** Web: http://www.unifiedautomation.com
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** Project: Sample OPC server
**
** Description: Communication interface to access the controllers.
******************************************************************************/
#include "bacommunicationinterface.h"
#include "bacontrollersimulation.h"
#include "uadatetime.h"

#define NUMBER_AIRCONDITIONER 10
#define NUMBER_FURNACES 10

/* ----------------------------------------------------------------------------
    Begin Class    BaCommunicationInterface
    constructors / destructors
-----------------------------------------------------------------------------*/
BaCommunicationInterface::BaCommunicationInterface()
{
    m_stop = OpcUa_False;
    m_arrayDevices.create(NUMBER_AIRCONDITIONER + NUMBER_FURNACES);

    OpcUa_UInt32 i;
    OpcUa_UInt32 index = 0;

    for ( i=0; i<NUMBER_AIRCONDITIONER; i++ )
    {
        m_arrayDevices[index] = new BaAirConditionerSimulation;
        index++;
    }
    for ( i=0; i<NUMBER_FURNACES; i++ )
    {
        m_arrayDevices[index] = new BaFurnaceSimulation;
        index++;
    }

    start();
}
BaCommunicationInterface::~BaCommunicationInterface()
{
    // Signal Simulation Thread to stop
    m_stop = OpcUa_True;
    // Wait until Simulation Thread stopped
    wait();
}
/* ----------------------------------------------------------------------------
    Class        BaCommunicationInterface
    Method       getCountControllers
    Description  Get available controllers.
-----------------------------------------------------------------------------*/
OpcUa_UInt32 BaCommunicationInterface::getCountControllers()
{
    return NUMBER_AIRCONDITIONER + NUMBER_FURNACES;
}
/* ----------------------------------------------------------------------------
    Class        BaCommunicationInterface
    Method       getControllerConfig
    Description  Get configuration of a controller.
-----------------------------------------------------------------------------*/
UaStatusCode BaCommunicationInterface::getControllerConfig(
    OpcUa_UInt32    controllerIndex,
    ControllerType& type,
    UaString&       sName,
    OpcUa_UInt32&   address)
{
    if ( controllerIndex >= NUMBER_AIRCONDITIONER + NUMBER_FURNACES )
    {
        return OpcUa_BadInvalidArgument;
    }

    char name[100];

    if ( controllerIndex < NUMBER_AIRCONDITIONER )
    {
        type = AIR_CONDITIONER;
        address = controllerIndex + 1;
        sprintf( name, "AirConditioner_%d", address);
        sName = name;
    }
    else
    {
        type = FURNACE;
        address = controllerIndex + 1;
        sprintf( name, "Furnace_%d", address - NUMBER_AIRCONDITIONER);
        sName = name;
    }

    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        BaCommunicationInterface
    Method       getControllerState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatusCode BaCommunicationInterface::getControllerState(
    OpcUa_UInt32     address,
    ControllerState& state)
{
    if ( address > NUMBER_AIRCONDITIONER + NUMBER_FURNACES )
    {
        return OpcUa_BadInvalidArgument;
    }

    return m_arrayDevices[address-1]->getControllerState(state);
}
/* ----------------------------------------------------------------------------
    Class        BaCommunicationInterface
    Method       setControllerState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatusCode BaCommunicationInterface::setControllerState(
    OpcUa_UInt32    address,
    ControllerState state)
{
    if ( address > NUMBER_AIRCONDITIONER + NUMBER_FURNACES )
    {
        return OpcUa_BadInvalidArgument;
    }

    return m_arrayDevices[address-1]->setControllerState(state);
}
/* ----------------------------------------------------------------------------
    Class        BaCommunicationInterface
    Method       getControllerData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode BaCommunicationInterface::getControllerData(
    OpcUa_UInt32    address,
    OpcUa_UInt32    offset,
    OpcUa_Double&   value)
{
    if ( address > NUMBER_AIRCONDITIONER + NUMBER_FURNACES )
    {
        return OpcUa_BadInvalidArgument;
    }

    return m_arrayDevices[address-1]->getControllerData(offset, value);
}
/* ----------------------------------------------------------------------------
    Class        BaCommunicationInterface
    Method       setControllerData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatusCode BaCommunicationInterface::setControllerData(
    OpcUa_UInt32    address,
    OpcUa_UInt32    offset,
    OpcUa_Double    value)
{
    if ( address > NUMBER_AIRCONDITIONER + NUMBER_FURNACES )
    {
        return OpcUa_BadInvalidArgument;
    }

    return m_arrayDevices[address-1]->setControllerData(offset, value);
}
/* ----------------------------------------------------------------------------
    Class        BaCommunicationInterface
    Method       run
    Description  Simulation Thread main function.
-----------------------------------------------------------------------------*/
void BaCommunicationInterface::run()
{
    UaDateTime lastSimulation = UaDateTime::now();
    while( m_stop == OpcUa_False )
    {
        msleep(100);
        if ( lastSimulation.msecsTo(UaDateTime::now()) >= 1000 )
        {
            OpcUa_UInt32 i;
            OpcUa_UInt32 count = m_arrayDevices.length();
            for ( i=0; i<count; i++ )
            {
                m_arrayDevices[i]->simulate();
            }
            lastSimulation = UaDateTime::now();
        }
    }
}
/* ----------------------------------------------------------------------------
    Begin Class    BaCommunicationInterface
    constructors / destructors
-----------------------------------------------------------------------------*/


