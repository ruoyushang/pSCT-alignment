//
// Created by Deivid Ribeiro on 4/10/20.
//

#include "globalalignmentobject.hpp"

#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

#include "client/pascommunicationinterface.hpp"
#include "client/pasnodemanager.hpp"

#include "common/alignment/device.hpp"
#include "common/opcua/passervertypeids.hpp"


const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> GlobalAlignmentObject::VARIABLES = {
        {PAS_OptTableType_State, std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> GlobalAlignmentObject::ERRORS = {
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> GlobalAlignmentObject::METHODS = {
        {PAS_GlobalAlignmentType_StartPSDTrack,            {"StartPSDTrack",            {}}},
        {PAS_GlobalAlignmentType_StopPSDTrack,            {"StopPSDTrack",            {}}}
};