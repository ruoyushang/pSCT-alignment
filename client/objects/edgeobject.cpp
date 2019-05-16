#include "edgeobject.hpp"
#include "pasnodemanager.hpp"
#include "passervertypeids.hpp"
#include "pascommunicationinterface.hpp"
#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"


const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> EdgeObject::VARIABLES = {
    {PAS_EdgeType_State,    std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
    {PAS_EdgeType_Position, std::make_tuple("Position", UaVariant(0), OpcUa_False, Ua_AccessLevel_CurrentRead)}
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> EdgeObject::ERRORS = {
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> EdgeObject::METHODS = {
        {PAS_EdgeType_Align,      {"Align",      {
                                                         std::make_tuple("Panel To Move",
                                                                         UaNodeId(OpcUaId_UInt32),
                                                                         "Position # of the panel to move during alignment"),
                                                         std::make_tuple("Panel To Fix",
                                                                         UaNodeId(OpcUaId_UInt32),
                                                                         "Position # of the panel to fix during alignment"),
                                                         std::make_tuple("Align Fraction",
                                                                         UaNodeId(OpcUaId_Double),
                                                                         "Fraction of motion to carry out (between 0.0 and 1.0, default = 0.25)"),
                                                         std::make_tuple("Execute",
                                                                         UaNodeId(OpcUaId_Boolean),
                                                                         "Whether to actually execute the calculated motion."),
                                                }}
        },
        {PAS_EdgeType_FindMatrix, {"FindMatrix", {       std::make_tuple("Step Size",
                                                                         UaNodeId(OpcUaId_Double),
                                                                         "Size of step to use when calculating response matrix (in mm)")}}},
        {PAS_EdgeType_Read,       {"Read",       {}}},
        {PAS_EdgeType_Stop,       {"Stop",       {}}}
};

UaNodeId EdgeObject::typeDefinitionId() const {
    UaNodeId ret(PAS_EdgeType, browseName().namespaceIndex());
    return ret;
}
