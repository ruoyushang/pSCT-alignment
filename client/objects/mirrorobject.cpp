#include "client/objects/mirrorobject.hpp"

#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

#include "common/opcua/passervertypeids.hpp"

#include "client/pascommunicationinterface.hpp"
#include "client/pasnodemanager.hpp"


const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> MirrorObject::VARIABLES = {
    {PAS_MirrorType_State,    std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
    {PAS_MirrorType_Position, std::make_tuple("Position", UaVariant(0), OpcUa_False,
                                              Ua_AccessLevel_CurrentRead)},
    {PAS_MirrorType_x,        std::make_tuple("x", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
    {PAS_MirrorType_y,        std::make_tuple("y", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
    {PAS_MirrorType_z,        std::make_tuple("z", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
    {PAS_MirrorType_xRot,              std::make_tuple("xRot", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
    {PAS_MirrorType_yRot,              std::make_tuple("yRot", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
    {PAS_MirrorType_zRot,              std::make_tuple("zRot", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
    {PAS_MirrorType_sysOffsetsMPES_x1, std::make_tuple("sysOffsetMPESx1", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
    {PAS_MirrorType_sysOffsetsMPES_y1, std::make_tuple("sysOffsetMPESy1", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
    {PAS_MirrorType_sysOffsetsMPES_x2, std::make_tuple("sysOffsetMPESx2", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
    {PAS_MirrorType_sysOffsetsMPES_y2, std::make_tuple("sysOffsetMPESy2", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
    {PAS_MirrorType_sysOffsetsMPES_x3, std::make_tuple("sysOffsetMPESx3", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
    {PAS_MirrorType_sysOffsetsMPES_y3, std::make_tuple("sysOffsetMPESy3", UaVariant(0.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
    {PAS_MirrorType_SafetyRadius,      std::make_tuple("SafetyRadius", UaVariant(60.0), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)},
    {PAS_MirrorType_SelectedMPES,      std::make_tuple("SelectedMPES", UaVariant(""), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)},
    {PAS_MirrorType_SelectedPanels,    std::make_tuple("SelectedPanels", UaVariant(""), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)},
    {PAS_MirrorType_SelectedEdges,     std::make_tuple("SelectedEdges", UaVariant(""), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)},
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> MirrorObject::ERRORS = {
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> MirrorObject::METHODS = {
    {PAS_MirrorType_ReadPosition,        {"ReadPosition",        {}}},
    {PAS_MirrorType_ReadSensors,         {"ReadSensors",         {}}},
    {PAS_MirrorType_ReadSensorsParallel, {"ReadSensorsParallel", {}}},
    {PAS_MirrorType_SelectAll,           {"SelectAll",           {}}},
    {PAS_MirrorType_MoveToCoords,        {"MoveToCoords",        {
                                                             std::make_tuple("x",
                                                                             UaNodeId(OpcUaId_Double),
                                                                             "Target x-coordinate for the mirror (in mm)."),
                                                             std::make_tuple("y",
                                                                             UaNodeId(OpcUaId_Double),
                                                                             "Target y-coordinate for the mirror (in mm)."),
                                                             std::make_tuple("z",
                                                                             UaNodeId(OpcUaId_Double),
                                                                             "Target z-coordinate for the mirror (in mm)."),
                                                             std::make_tuple("xRot",
                                                                             UaNodeId(OpcUaId_Double),
                                                                             "Target x-rotation for the mirror (in rad)."),
                                                             std::make_tuple("yRot",
                                                                             UaNodeId(OpcUaId_Double),
                                                                             "Target y-rotation for the mirror (in rad)."),
                                                             std::make_tuple("zRot",
                                                                             UaNodeId(OpcUaId_Double),
                                                                             "Target z-rotation for the mirror (in rad)."),
                                                             std::make_tuple("Execute",
                                                                         UaNodeId(OpcUaId_Boolean),
                                                                         "Whether to actually execute the calculated motion."),
                                                         }}},
    {PAS_MirrorType_AlignSequential, {"AlignSequential", {
                                                             std::make_tuple("Start edge",
                                                                             UaNodeId(OpcUaId_UInt32),
                                                                             "Edge to start aligning from (eAddress)"),
                                                             std::make_tuple("Direction",
                                                                             UaNodeId(OpcUaId_UInt32),
                                                                             "Direction to align edges in (0 for +z rotation, 1 for -z rotation)"),
                                                         }}},
    {PAS_MirrorType_AlignSector,     {"AlignSector",     {
                                                            std::make_tuple("Align Fraction",
                                                                         UaNodeId(OpcUaId_Double),
                                                                         "Fraction of motion to carry out (between 0.0 and 1.0)"),
                                                            std::make_tuple("Execute",
                                                                         UaNodeId(OpcUaId_Boolean),
                                                                         "Whether to actually execute the calculated motion."),
                                                         }}},
    {PAS_MirrorType_GlobalAlign,     {"GlobalAlign",     {
                                                             std::make_tuple("Fixed Panel",
                                                                             UaNodeId(OpcUaId_UInt32),
                                                                             "Panel to fix when aligning (by position number)"),
                                                            std::make_tuple("Execute",
                                                                         UaNodeId(OpcUaId_Boolean),
                                                                         "Whether to actually execute the calculated motion."),
                                                         }}},
    {PAS_MirrorType_Stop,            {"Stop",            {}}}
};

