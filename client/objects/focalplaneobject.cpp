#include "client/objects/focalplaneobject.hpp"

#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

#include "common/opcua/passervertypeids.hpp"

#include "client/pascommunicationinterface.hpp"
#include "client/pasnodemanager.hpp"

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> FocalPlaneObject::VARIABLES = {
        {PAS_FocalPlaneType_State, std::make_tuple("State", UaVariant(0), OpcUa_True,
                                                   Ua_AccessLevel_CurrentRead)},
        {PAS_FocalPlaneType_ErrorState, std::make_tuple("ErrorState", UaVariant(0), OpcUa_True,
                                                        Ua_AccessLevel_CurrentRead)},
        {PAS_FocalPlaneType_ImageFile, std::make_tuple("ImageFile", UaVariant(""), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)},
        {PAS_FocalPlaneType_Verbosity, std::make_tuple("Verbosity", UaVariant(OpcUa_False), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)},
        {PAS_FocalPlaneType_Show, std::make_tuple("Show", UaVariant(OpcUa_True), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)},
        {PAS_FocalPlaneType_DETECT_MINAREA, std::make_tuple("DETECT_MINAREA", UaVariant(30) , OpcUa_False,
                                                            Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)},
        {PAS_FocalPlaneType_THRESH, std::make_tuple("THRESH", UaVariant(6), OpcUa_False,
                                                    Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)},
        {PAS_FocalPlaneType_DEBLEND_MINCONT, std::make_tuple("DEBLEND_MINCONT", UaVariant(0.01), OpcUa_False,
                                                             Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)},
        {PAS_FocalPlaneType_SEARCH_XS, std::make_tuple("SEARCH_XS", UaVariant("0 0"), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)},
        {PAS_FocalPlaneType_SEARCH_YS, std::make_tuple("SEARCH_YS", UaVariant("0 0"), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)},
        {PAS_FocalPlaneType_PatternRadius, std::make_tuple("PatternRadius", UaVariant(150), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead)},
        {PAS_FocalPlaneType_PhaseOffsetRad, std::make_tuple("PhaseOffsetRad", UaVariant(0), OpcUa_False,
                                                           Ua_AccessLevel_CurrentRead)},
        {PAS_FocalPlaneType_RingFrac, std::make_tuple("RingFrac", UaVariant(0), OpcUa_False,
                                                            Ua_AccessLevel_CurrentRead)},
        {PAS_FocalPlaneType_MinDist, std::make_tuple("MinDist", UaVariant(20), OpcUa_False,
                                                      Ua_AccessLevel_CurrentRead)},
        {PAS_FocalPlaneType_PatternCenter, std::make_tuple("PatternCenter", UaVariant("1910 1010"), OpcUa_False,
                                                     Ua_AccessLevel_CurrentRead)},
        {PAS_FocalPlaneType_RingTol, std::make_tuple("RingTol", UaVariant(0.1), OpcUa_False, Ua_AccessLevel_CurrentRead)},
        {PAS_FocalPlaneType_Exposure, std::make_tuple("Exposure", UaVariant(500000), OpcUa_False,
                                                       Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)},
        {PAS_FocalPlaneType_Gain, std::make_tuple("Gain", UaVariant(15), OpcUa_False,
                                                      Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)},
        {PAS_FocalPlaneType_FrameRate, std::make_tuple("Frame Rate", UaVariant(10), OpcUa_False,
                                                      Ua_AccessLevel_CurrentRead | Ua_AccessLevel_CurrentWrite)}
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> FocalPlaneObject::ERRORS = {
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> FocalPlaneObject::METHODS = {
        {PAS_FocalPlaneType_AnalyzePatternImage,     {"AnalyzePatternImage",     {std::make_tuple("ring_rad",
                                                                                                  UaNodeId(
                                                                                                          OpcUaId_Double),
                                                                                                  "Desired radius of ring pattern to analyze (in Px)."),
                                                                                         std::make_tuple(
                                                                                                 "phase_offset_rad",
                                                                                                 UaNodeId(
                                                                                                         OpcUaId_Double),
                                                                                                 "Phase angle to rotate ring, conterclockwise, in Radians. Default 0"),
                                                                                         std::make_tuple("ring_frac",
                                                                                                         UaNodeId(
                                                                                                                 OpcUaId_Double),
                                                                                                         "Fraction (1-frac, 1+frac)*radius that you accept a centroid as part of a ring pattern. "),
                                                                                         std::make_tuple("min_dist",
                                                                                                         UaNodeId(
                                                                                                                 OpcUaId_Int16),
                                                                                                         "Minimum distance we use to conclude a centroid is common in both images our default is 20 pixels"),
                                                                                         std::make_tuple("PatternCenter",
                                                                                                         UaNodeId(OpcUaId_String),
                                                                                                         "Center of pattern, written as a string like '1910 1010'"),
                                                                                         std::make_tuple("RingTol",
                                                                                                         UaNodeId(OpcUaId_Double),
                                                                                                         "Tolerance for how far to find panels between pattern positions. Default is 0.2"),
                                                                                 }}},
        {PAS_FocalPlaneType_AnalyzeSinglePanelImage, {"AnalyzeSinglePanelImage", {}}},
        {PAS_FocalPlaneType_SaveImage,               {"SaveImage",               {}}},
        {PAS_FocalPlaneType_SavePanelMotion,         {"SavePanelMotions",        {}}},
        {PAS_FocalPlaneType_Pattern2Center,          {"CalcPatternToCenter",     {std::make_tuple("sector",
                                                                                                  UaNodeId(
                                                                                                          OpcUaId_String),
                                                                                                  "Mirror sector to move to pattern, i.e. P1, P2 or S1."),
                                                                                         std::make_tuple("resp_file",
                                                                                                         UaNodeId(
                                                                                                                 OpcUaId_String),
                                                                                                         "Path to response matrix file for this sector"),
                                                                                         std::make_tuple("Radius",
                                                                                                         UaNodeId(
                                                                                                                 OpcUaId_Double),
                                                                                                         "Radius for this sector, in Pixels")
                                                                                 }}},
        {PAS_FocalPlaneType_Center2Pattern,          {"CalcCenterToPattern",     {std::make_tuple("sector",
                                                                                                  UaNodeId(
                                                                                                          OpcUaId_String),
                                                                                                  "Mirror sector to move to pattern, i.e. P1, P2 or S1."),
                                                                                         std::make_tuple("resp_file",
                                                                                                         UaNodeId(
                                                                                                                 OpcUaId_String),
                                                                                                         "Path to response matrix file for this sector"),
                                                                                         std::make_tuple("Radius",
                                                                                                         UaNodeId(
                                                                                                                 OpcUaId_Double),
                                                                                                         "Radius for this sector, in Pixels")
                                                                                 }}},
        {PAS_FocalPlaneType_Panel2Center,            {"CalcPanelToCenter",       {std::make_tuple("panel",
                                                                                                  UaNodeId(
                                                                                                          OpcUaId_Int16),
                                                                                                  "Panel to move"),
                                                                                         std::make_tuple("Current X",
                                                                                                         UaNodeId(
                                                                                                                 OpcUaId_Double),
                                                                                                         "Current X coordinate of panel"),
                                                                                         std::make_tuple("Current Y",
                                                                                                         UaNodeId(
                                                                                                                 OpcUaId_Double),
                                                                                                         "Current Y coordinate of panel"),
                                                                                         std::make_tuple(
                                                                                                 "RespFile",
                                                                                                 UaNodeId(OpcUaId_String),
                                                                                                 "Response Matrix file, in .yml format.")
                                                                                 }}},
        {PAS_FocalPlaneType_Panel2Pattern,           {"CalcPanelToPattern",      {std::make_tuple("panel",
                                                                                                  UaNodeId(
                                                                                                          OpcUaId_Int16),
                                                                                                  "Panel to move"),
                                                                                         std::make_tuple("Current X",
                                                                                                         UaNodeId(
                                                                                                                 OpcUaId_Double),
                                                                                                         "Current X coordinate of panel"),
                                                                                         std::make_tuple("Current Y",
                                                                                                         UaNodeId(
                                                                                                                 OpcUaId_Double),
                                                                                                         "Current Y coordinate of panel"),
                                                                                         std::make_tuple(
                                                                                                 "RespFile",
                                                                                                 UaNodeId(OpcUaId_String),
                                                                                                 "Response Matrix file, in .yml format.")
                                                                                 }}},
};

