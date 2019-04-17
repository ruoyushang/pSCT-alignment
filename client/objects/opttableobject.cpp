#include <iostream>
#include "opttableobject.hpp"
#include "pasnodemanager.h"
#include "passervertypeids.h"
#include "pascommunicationinterface.h"
#include "components.h"
#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> OptTableObject::VARIABLES = {
        {PAS_OptTableType_State, std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> OptTableObject::ERRORS = {
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> OptTableObject::METHODS = {
};

UaNodeId OptTableObject::typeDefinitionId() const
{
    UaNodeId ret(PAS_OptTableType, browseName().namespaceIndex());
    return ret;
}