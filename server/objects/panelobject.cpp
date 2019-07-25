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
    {PAS_PanelType_Position,       std::make_tuple("Position", UaVariant(0), OpcUa_False, Ua_AccessLevel_CurrentRead)},
    {PAS_PanelType_Serial,         std::make_tuple("Serial", UaVariant(0), OpcUa_False, Ua_AccessLevel_CurrentRead)},
    {PAS_PanelType_ExtTemperature, std::make_tuple("ExternalTemperature", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
    {PAS_PanelType_IntTemperature, std::make_tuple("InternalTemperature", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
    {PAS_PanelType_ErrorState, std::make_tuple("ErrorState", UaVariant(0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)}

};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> PanelObject::ERRORS = {
    {PAS_PanelType_Error0,  std::make_tuple("[0] [Operable] Actuator 1 Operable Error", UaVariant(false), OpcUa_False)},
    {PAS_PanelType_Error1,  std::make_tuple("[1] [Fatal] Actuator 1 Fatal Error", UaVariant(false), OpcUa_False)},
    {PAS_PanelType_Error2,  std::make_tuple("[2] [Operable] Actuator 2 Operable Error", UaVariant(false), OpcUa_False)},
    {PAS_PanelType_Error3,  std::make_tuple("[3] [Fatal] Actuator 2 Fatal Error", UaVariant(false), OpcUa_False)},
    {PAS_PanelType_Error4,  std::make_tuple("[4] [Operable] Actuator 3 Operable Error", UaVariant(false), OpcUa_False)},
    {PAS_PanelType_Error5,  std::make_tuple("[5] [Fatal] Actuator 3 Fatal Error", UaVariant(false), OpcUa_False)},
    {PAS_PanelType_Error6,  std::make_tuple("[6] [Operable] Actuator 4 Operable Error", UaVariant(false), OpcUa_False)},
    {PAS_PanelType_Error7,  std::make_tuple("[7] [Fatal] Actuator 4 Fatal Error", UaVariant(false), OpcUa_False)},
    {PAS_PanelType_Error8,  std::make_tuple("[8] [Operable] Actuator 5 Operable Error", UaVariant(false), OpcUa_False)},
    {PAS_PanelType_Error9,  std::make_tuple("[9] [Fatal] Actuator 5 Fatal Error", UaVariant(false), OpcUa_False)},
    {PAS_PanelType_Error10, std::make_tuple("[10] [Operable] Actuator 6 Operable Error", UaVariant(false),
                                            OpcUa_False)},
    {PAS_PanelType_Error11, std::make_tuple("[11] [Fatal] Actuator 6 Fatal Error", UaVariant(false), OpcUa_False)},
    {PAS_PanelType_Error12, std::make_tuple("[12] [Fatal] Moved out of software range.", UaVariant(false),
                                            OpcUa_False)},
    {PAS_PanelType_Error13, std::make_tuple("[13] [Operable] DBInfo not set", UaVariant(false), OpcUa_False)},
    {PAS_PanelType_Error14, std::make_tuple("[14] [Operable] MySQL communication error", UaVariant(false), OpcUa_False)}
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> PanelObject::METHODS = {
    {PAS_PanelType_MoveDeltaLengths,    {"MoveDeltaLengths",    {
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
    {PAS_PanelType_MoveToLengths,       {"MoveToLengths",       {
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
    {PAS_PanelType_FindHome,            {"FindHome",            {
                                                                    std::make_tuple("Direction",
                                                                                    UaNodeId(OpcUaId_Int32),
                                                                                    "Direction to probe home (1 to extend, -1 to retract")
                                                                }}
    },
    {PAS_PanelType_ClearError,          {"ClearError",          {
                                                                    std::make_tuple("Error Num",
                                                                                    UaNodeId(OpcUaId_Int32),
                                                                                    "Number of the error to clear")
                                                                }}
    },
    {PAS_PanelType_ClearAllErrors,      {"ClearAllErrors",      {}}},
    {PAS_PanelType_ClearActuatorErrors, {"ClearActuatorErrors", {}}},
    {PAS_PanelType_ClearPlatformErrors, {"ClearPlatformErrors", {}}},
    {PAS_PanelType_TurnOn,              {"TurnOn",              {}}},
    {PAS_PanelType_TurnOff,             {"TurnOff",             {}}},
    {PAS_PanelType_Stop,                {"Stop",                {}}}
};
