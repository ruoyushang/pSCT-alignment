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
#ifndef __BACONTROLLERSIMULATION_H__
#define __BACONTROLLERSIMULATION_H__

#include "uabase.h"
#include "uastring.h"
#include "statuscode.h"
#include "bacommunicationinterface.h"
#include "uamutex.h"

class BaControllerSimulation
{
    UA_DISABLE_COPY(BaControllerSimulation);
public:
    /* construction / destruction */
    BaControllerSimulation(){}
    virtual ~BaControllerSimulation(){}

    /* Get Controller status and data */
    virtual UaStatusCode getControllerState(
        BaCommunicationInterface::ControllerState& state) = 0;

    virtual UaStatusCode getControllerData(
        OpcUa_UInt32    offset,
        OpcUa_Double&   value) = 0;

    /* set Controller status and data */
    virtual UaStatusCode setControllerState(
        BaCommunicationInterface::ControllerState state) = 0;

    virtual UaStatusCode setControllerData(
        OpcUa_UInt32    offset,
        OpcUa_Double    value) = 0;

    virtual void simulate() = 0;

protected:
    UaMutex m_mutex;
    BaCommunicationInterface::ControllerState m_state;
    OpcUa_Double m_temperature;
    OpcUa_Double m_temperatureSetPoint;
    OpcUa_Double m_powerConsumption;
    OpcUa_Double m_random;
};

class BaAirConditionerSimulation: public BaControllerSimulation
{
    UA_DISABLE_COPY(BaAirConditionerSimulation);
public:
    /* construction / destruction */
    BaAirConditionerSimulation();
    virtual ~BaAirConditionerSimulation();

    /* Get Controller status and data */
    UaStatusCode getControllerState(
        BaCommunicationInterface::ControllerState& state);

    UaStatusCode getControllerData(
        OpcUa_UInt32    offset,
        OpcUa_Double&   value);

    /* set Controller status and data */
    UaStatusCode setControllerState(
        BaCommunicationInterface::ControllerState state);

    UaStatusCode setControllerData(
        OpcUa_UInt32    offset,
        OpcUa_Double    value);

    void simulate();
protected:
    OpcUa_Double m_humidity;
    OpcUa_Double m_humiditySetpoint;
};

class BaFurnaceSimulation: public BaControllerSimulation
{
    UA_DISABLE_COPY(BaFurnaceSimulation);
public:
    /* construction / destruction */
    BaFurnaceSimulation();
    virtual ~BaFurnaceSimulation();

    /* Get Controller status and data */
    UaStatusCode getControllerState(
        BaCommunicationInterface::ControllerState& state);

    UaStatusCode getControllerData(
        OpcUa_UInt32    offset,
        OpcUa_Double&   value);

    /* set Controller status and data */
    UaStatusCode setControllerState(
        BaCommunicationInterface::ControllerState state);

    UaStatusCode setControllerData(
        OpcUa_UInt32    offset,
        OpcUa_Double    value);

    void simulate();
protected:
    OpcUa_Double m_gasFlow;
};

#endif // #ifndef __BACONTROLLERSIMULATION_H__
