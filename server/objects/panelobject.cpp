#include "server/objects/panelobject.hpp"

#include <map>
#include <string>
#include <tuple>

#include "uabase/uavariant.h"

#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

#include "common/opcua/passervertypeids.hpp"
#include "common/alignment/device.hpp"

#include "server/pasnodemanager.hpp"
#include "server/pascommunicationinterface.hpp"

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> PanelObject::VARIABLES = {
        {PAS_PanelType_State,          std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
        {PAS_PanelType_ExtTemperature, std::make_tuple("ExternalTemperature", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
        {PAS_PanelType_IntTemperature, std::make_tuple("InternalTemperature", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)}
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> PanelObject::ERRORS = {
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> PanelObject::METHODS = {
        {PAS_PanelType_MoveDeltaLengths, {"MoveDeltaLengths", {
                                                                      std::make_tuple("Delta Length 1",
                                                                                      UaNodeId(OpcUaId_Float),
                                                                                      "Desired change in length for Actuator 1 (in mm)."),
                                                                      std::make_tuple("Delta Length 2",
                                                                                      UaNodeId(OpcUaId_Float),
                                                                                      "Desired change in length for Actuator 2 (in mm)."),
                                                                      std::make_tuple("Delta Length 3",
                                                                                      UaNodeId(OpcUaId_Float),
                                                                                      "Desired change in length for Actuator 3 (in mm)."),
                                                                      std::make_tuple("Delta Length 4",
                                                                                      UaNodeId(OpcUaId_Float),
                                                                                      "Desired change in length for Actuator 4 (in mm)."),
                                                                      std::make_tuple("Delta Length 5",
                                                                                      UaNodeId(OpcUaId_Float),
                                                                                      "Desired change in length for Actuator 5 (in mm)."),
                                                                      std::make_tuple("Delta Length 6",
                                                                                      UaNodeId(OpcUaId_Float),
                                                                                      "Desired change in length for Actuator 6 (in mm)."),
                                                              }}
        },
        {PAS_PanelType_MoveToLengths, {"MoveToLengths", {
                                                                      std::make_tuple("Length Actuator 1",
                                                                                      UaNodeId(OpcUaId_Float),
                                                                                      "Target length for Actuator 1 (in mm)."),
                                                                      std::make_tuple("Length Actuator 2",
                                                                                      UaNodeId(OpcUaId_Float),
                                                                                      "Target length for Actuator 2 (in mm)."),
                                                                      std::make_tuple("Length Actuator 3",
                                                                                      UaNodeId(OpcUaId_Float),
                                                                                      "Target length for Actuator 3 (in mm)."),
                                                                      std::make_tuple("Length Actuator 4",
                                                                                      UaNodeId(OpcUaId_Float),
                                                                                      "Target length for Actuator 4 (in mm)."),
                                                                      std::make_tuple("Length Actuator 5",
                                                                                      UaNodeId(OpcUaId_Float),
                                                                                      "Target length for Actuator 5 (in mm)."),
                                                                      std::make_tuple("Length Actuator 6",
                                                                                      UaNodeId(OpcUaId_Float),
                                                                                      "Target length for Actuator 6 (in mm)."),
                                                              }}
        },
        {PAS_PanelType_Stop,          {"Stop",          {}}}
};

UaNodeId PanelObject::typeDefinitionId() const {
    UaNodeId ret(PAS_PanelType, browseName().namespaceIndex());
    return ret;
}
