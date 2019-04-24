/******************************************************************************
** Project: P2PAS Server
**
** Description: Common description of the communication interface to access devices.
** Abstract class to meant to be implemented on both sides.
******************************************************************************/
#ifndef COMMON_PASCOMUNICATIONINTERFACECOMMON_HPP
#define COMMON_PASCOMUNICATIONINTERFACECOMMON_HPP

#include "uabase.h"
#include "uavariant.h"
#include "statuscode.h"
#include "uaarraytemplates.h"

#include <map>

enum class PASState {
    On = 0,
    Off = 1,
    Busy = 2,
    FatalError = 3,
    OperableError = 4,
        GLOB_Positioner_Moving = 10,
        GLOB_Positioner_notMoving = 11
    };

struct Identity;

class PasComInterfaceCommon
{
public:
    PasComInterfaceCommon() = default;

    virtual ~PasComInterfaceCommon() = default;

    virtual UaStatus initialize() = 0;

    /// @brief Get the total number of devices of a given type.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @return The number of found device controllers.
    std::size_t getDeviceCount(OpcUa_UInt32 deviceType);

    /// @brief Get the identities of all devices of a given type.
    /// @param deviceType OPC UA type ID for the desired device object type.
    /// @return A vector of device Identities.
    std::vector<Identity> getValidDeviceIdentities(OpcUa_UInt32 deviceType);

    /* Get device status and data */
    virtual UaStatus getDeviceState(OpcUa_UInt32 type, const Identity &identity, PASState &state) = 0;

    virtual UaStatus getDeviceData(OpcUa_UInt32 type, const Identity &identity,
                                   OpcUa_UInt32 offset,
                                   UaVariant &value) = 0;

    /* Set device status and data*/
    virtual UaStatus setDeviceState(OpcUa_UInt32 type, const Identity &identity, PASState state) = 0;

    virtual UaStatus setDeviceData(OpcUa_UInt32 type, const Identity &identity,
                                   OpcUa_UInt32 offset,
                                   UaVariant value) = 0;

    virtual UaStatus operateDevice(OpcUa_UInt32 type, const Identity &identity,
                                   OpcUa_UInt32 offset,
                                   const UaVariantArray &args) = 0;
};

#endif //COMMON_PASCOMUNICATIONINTERFACECOMMON_HPP
