#include "client/objects/rangefinderobject.hpp"

#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

#include "common/opcua/passervertypeids.hpp"

#include "client/pascommunicationinterface.hpp"
#include "client/pasnodemanager.hpp"

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> RangefinderObject::VARIABLES = {
        {PAS_RangefinderType_State, std::make_tuple("State", UaVariant(0), OpcUa_True,
                                                   Ua_AccessLevel_CurrentRead)},
        {PAS_RangefinderType_ErrorState, std::make_tuple("ErrorState", UaVariant(0), OpcUa_True,
                                                        Ua_AccessLevel_CurrentRead)}
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> RangefinderObject::ERRORS = {
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> RangefinderObject::METHODS = {
        {PAS_RangefinderType_TurnOn, {"TurnOn", {}}},
        {PAS_RangefinderType_TurnOff, {"TurnOff", {}}},
};

