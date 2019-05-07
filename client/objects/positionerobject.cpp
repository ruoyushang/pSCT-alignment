/**
 * @file positionerobject.cpp
 * @brief Source file for PositionerObject class inheriting from the PasObject class.
 */

#include <iostream>
#include "positionerobject.hpp"
#include "pasnodemanager.hpp"
#include "passervertypeids.hpp"
#include "pascommunicationinterface.hpp"
#include "common/alignment/device.hpp"
#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> PositionerObject::VARIABLES = {
        {GLOB_PositionerType_isMoving,    std::make_tuple("isMoving", UaVariant(false), OpcUa_True,
                                                          Ua_AccessLevel_CurrentRead)},
        {GLOB_PositionerType_curAz,       std::make_tuple("Azimuth", UaVariant(0.0), OpcUa_False,
                                                          Ua_AccessLevel_CurrentRead)},
        {GLOB_PositionerType_curEl,       std::make_tuple("Elevation", UaVariant(0.0), OpcUa_False,
                                                          Ua_AccessLevel_CurrentRead)},
        {GLOB_PositionerType_inAz,        std::make_tuple("targetAzimuth", UaVariant(0.0), OpcUa_False,
                                                          Ua_AccessLevel_CurrentWrite)},
        {GLOB_PositionerType_inEl,        std::make_tuple("targetElevation", UaVariant(0.0), OpcUa_False,
                                                          Ua_AccessLevel_CurrentWrite)},
        {GLOB_PositionerType_EnergyLevel, std::make_tuple("EnergyLevel", UaVariant(0), OpcUa_False,
                                                          Ua_AccessLevel_CurrentRead)},
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> PositionerObject::ERRORS = {
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> PositionerObject::METHODS = {
    {GLOB_PositionerType_Initialize, {"Initialize", {}}},
    {GLOB_PositionerType_Move,       {"Move",       {}}},
    {GLOB_PositionerType_SetEnergy,  {"SetEnergy",  {}}},
    {GLOB_PositionerType_Stop,       {"Stop",       {}}},

};

/// @details Uses the PositionerType id number defined in passervertypeids.h
/// along with the namspace index to construct the UaNodeId for the Positioner
/// type node.
UaNodeId PositionerObject::typeDefinitionId() const
{
    UaNodeId ret(GLOB_PositionerType, browseName().namespaceIndex());
    return ret;
}
