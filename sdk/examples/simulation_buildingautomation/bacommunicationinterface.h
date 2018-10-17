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
#ifndef __BACOMMUNICATIONINTERFACE_H__
#define __BACOMMUNICATIONINTERFACE_H__

#include "uabase.h"
#include "uastring.h"
#include "statuscode.h"
#include "uaarraytemplates.h"
#include "uathread.h"

class BaControllerSimulation;

class BaCommunicationInterface: public UaThread
{
    UA_DISABLE_COPY(BaCommunicationInterface);
public:
    // ControllerType enumeration
    enum ControllerType
    {
        AIR_CONDITIONER,
        FURNACE
    };
    enum ControllerState
    {
        Ba_ControllerState_Off,
        Ba_ControllerState_On,
        Ba_ControllerState_Error
    };

    /* construction / destruction */
    BaCommunicationInterface();
    virtual ~BaCommunicationInterface();

    /* Get available controllers and their configuration */
    OpcUa_UInt32 getCountControllers();
    
    UaStatusCode getControllerConfig(
        OpcUa_UInt32    controllerIndex,
        ControllerType& type,
        UaString&       sName,
        OpcUa_UInt32&   address);

    /* Get Controller status and data */
    UaStatusCode getControllerState(
        OpcUa_UInt32     address,
        ControllerState& state);

    UaStatusCode getControllerData(
        OpcUa_UInt32    address,
        OpcUa_UInt32    offset,
        OpcUa_Double&   value);

    /* Set Controller status and data */
    UaStatusCode setControllerState(
        OpcUa_UInt32     address,
        ControllerState  state);

    UaStatusCode setControllerData(
        OpcUa_UInt32    address,
        OpcUa_UInt32    offset,
        OpcUa_Double    value);

private:
    // Simulation Thread main function
    void run();

    UaObjectPointerArray<BaControllerSimulation> m_arrayDevices;
    OpcUa_Boolean m_stop;
};

#endif // #ifndef __BACOMMUNICATIONINTERFACE_H__
