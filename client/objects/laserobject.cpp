#include "client/objects/laserobject.hpp"

#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

#include "common/opcua/passervertypeids.hpp"

#include "client/pascommunicationinterface.hpp"
#include "client/pasnodemanager.hpp"

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> LaserObject::VARIABLES = {
        {PAS_LaserType_State, std::make_tuple("State", UaVariant(0), OpcUa_True,
                                                   Ua_AccessLevel_CurrentRead)},
        {PAS_LaserType_ErrorState, std::make_tuple("ErrorState", UaVariant(0), OpcUa_True,
                                                        Ua_AccessLevel_CurrentRead)}
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> LaserObject::ERRORS = {
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> LaserObject::METHODS = {
        {PAS_LaserType_TurnOn, {"TurnOn", {}}},
        {PAS_LaserType_TurnOff, {"TurnOff", {}}},
};

