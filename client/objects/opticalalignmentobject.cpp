#include "opticalalignmentobject.hpp"

#include "uaserver/methodhandleuanode.h"
#include "uaserver/opcua_analogitemtype.h"

#include "client/pascommunicationinterface.hpp"
#include "client/pasnodemanager.hpp"

#include "common/alignment/device.hpp"
#include "common/opcua/passervertypeids.hpp"


const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean, OpcUa_Byte>> OpticalAlignmentObject::VARIABLES = {
        {PAS_OptTableType_State, std::make_tuple("State", UaVariant(0), OpcUa_True, Ua_AccessLevel_CurrentRead)},
};

const std::map<OpcUa_UInt32, std::tuple<std::string, UaVariant, OpcUa_Boolean>> OpticalAlignmentObject::ERRORS = {
};

const std::map<OpcUa_UInt32, std::pair<std::string, std::vector<std::tuple<std::string, UaNodeId, std::string>>>> OpticalAlignmentObject::METHODS = {
        {PAS_OpticalAlignmentType_GetResponseMatrix,            {"GetResponseMatrix",            {}}},
        {PAS_OpticalAlignmentType_MoveFocusToPattern,            {"MoveFocusToPattern",            {}}},
        {PAS_OpticalAlignmentType_MovePatternToFocus,            {"MovePatternToFocus",            {}}},
        {PAS_OpticalAlignmentType_CalibrateFirstOrderCorr,            {"CalibrateFirstOrderCorr",            {}}},
        {PAS_OpticalAlignmentType_MoveForCalibration,            {"MoveForCalibration",            {}}}
};