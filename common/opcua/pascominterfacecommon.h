/******************************************************************************
** Project: P2PAS Server
**
** Description: Common description of the communication interface to access devices.
** Abstract class to meant to be implemented on both sides.
******************************************************************************/
#ifndef __PASCOMUNICATIONINTERFACECOMMON_H__
#define __PASCOMUNICATIONINTERFACECOMMON_H__

#include "uabase.h"
#include "uavariant.h"
#include "statuscode.h"
#include "uaarraytemplates.h"

#include "common/alignment/device.hpp"

struct Identity;

class PasComInterfaceCommon
{
public:
    PasComInterfaceCommon() {};
    virtual ~PasComInterfaceCommon() {};

    virtual UaStatus initialize() = 0;

    virtual OpcUa_Int32 getDevices(OpcUa_UInt32 deviceType) = 0;

    virtual UaStatusCode getDeviceConfig(OpcUa_UInt32 type, 
            OpcUa_UInt32 deviceIndex,
            UaString& sName,
            Identity& identity) = 0;

    /* Get device status and data */
    virtual UaStatus getDeviceState(OpcUa_UInt32 type, const Identity &identity, Device::DeviceState &state) = 0;

    virtual UaStatus getDeviceData(OpcUa_UInt32 type, const Identity &identity,
                                   OpcUa_UInt32 offset,
                                   UaVariant &value) = 0;

    /* Set device status and data*/
    virtual UaStatus setDeviceState(OpcUa_UInt32 type, const Identity &identity, Device::DeviceState state) = 0;

    virtual UaStatus setDeviceData(OpcUa_UInt32 type, const Identity &identity,
            OpcUa_UInt32 offset,
            UaVariant value) = 0;

    virtual UaStatus operateDevice(OpcUa_UInt32 type, const Identity &identity,
                                   OpcUa_UInt32 offset = 0,
                                   const UaVariantArray &args = UaVariantArray()) = 0;
};

#endif
