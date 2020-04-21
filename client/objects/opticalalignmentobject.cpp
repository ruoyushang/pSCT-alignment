#include "opticalalignmentobject.hpp"

#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

#include "common/opcua/passervertypeids.hpp"

#include "client/pascommunicationinterface.hpp"
#include "client/pasnodemanager.hpp"

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> OpticalAlignmentObject::VARIABLES = {
        {PAS_OpticalAlignmentType_State, std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
        {PAS_OpticalAlignmentType_ErrorState, std::make_tuple("ErrorState", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
        {PAS_OpticalAlignmentType_ImageParamSet, std::make_tuple("ImageParamSet", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)}
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> OpticalAlignmentObject::ERRORS = {
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> OpticalAlignmentObject::METHODS = {
        {PAS_OpticalAlignmentType_GetResponseMatrix,       {"GetResponseMatrix",       {}}},
        {PAS_OpticalAlignmentType_StopProcess,             {"StopProcessing",       {}}},
        {PAS_OpticalAlignmentType_MoveFocusToPattern,      {"MoveFocusToPattern",      {}}},
        {PAS_OpticalAlignmentType_MovePatternToFocus,      {"MovePatternToFocus",      {}}},
        {PAS_OpticalAlignmentType_CalibrateFirstOrderCorr, {"CalibrateFirstOrderCorr", {std::make_tuple("CenterX",
                                                                                                        UaNodeId(OpcUaId_Double),
                                                                                                        "Target Focal Point x coordinate"),
                                                                                               std::make_tuple(
                                                                                                       "CenterY",
                                                                                                       UaNodeId(OpcUaId_Double),
                                                                                                       "Target Focal Point y coordinate"),
                                                                                               std::make_tuple(
                                                                                                       "ShowPlot",
                                                                                                       UaNodeId(OpcUaId_Int16),
                                                                                                       "Show Plot in between panel motions (1 True, 0 False)"),
                                                                                               std::make_tuple(
                                                                                                       "OffsetLimit",
                                                                                                       UaNodeId(OpcUaId_Double),
                                                                                                       "Max pixel distance between current and target panel position."),
                                                                                               std::make_tuple(
                        "RespFile",
                        UaNodeId(OpcUaId_String),
                        "Response Matrix file, in .yml format.")}}},
        {PAS_OpticalAlignmentType_MoveForCalibration,      {"MoveForCalibration",      {}}}
};