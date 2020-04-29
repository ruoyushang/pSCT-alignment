#include "client/objects/opttableobject.hpp"


#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

#include "client/pascommunicationinterface.hpp"
#include "client/pasnodemanager.hpp"

#include "common/alignment/device.hpp"
#include "common/opcua/passervertypeids.hpp"


const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> OptTableObject::VARIABLES = {
    {PAS_OptTableType_State, std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> OptTableObject::ERRORS = {
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> OptTableObject::METHODS = {
        {PAS_OptTableType_FindMatrix, {"FindMatrix", {       std::make_tuple("Step Size",
                                                                         UaNodeId(OpcUaId_Double),
                                                                         "Size of step to use when calculating response matrix (in mm)")}}},
        {PAS_OptTableType_Align, {"Align",      {
                                                        std::make_tuple("Panel To Move",
                                                                        UaNodeId(OpcUaId_UInt32),
                                                                        "Position # of the panel to move during alignment"),
                                                        std::make_tuple("Panel To Fix",
                                                                        UaNodeId(OpcUaId_UInt32),
                                                                        "Position # of the panel to fix during alignment"),
                                                        std::make_tuple("Align Fraction",
                                                                        UaNodeId(OpcUaId_Double),
                                                                        "Fraction of motion to carry out (between 0.0 and 1.0, default = 0.25)"),
                                                        std::make_tuple("Command",
                                                                        UaNodeId(OpcUaId_String),
                                                                        "Command to run (calculate, setAlignFrac, execute)."),
                                                }}},
        {PAS_OptTableType_StepAll, {"StepAll", {       std::make_tuple("Step Size",
                                                                   UaNodeId(OpcUaId_Double),
                                                                   "Size of step to use when calculating response matrix (in mm)")}}},
};