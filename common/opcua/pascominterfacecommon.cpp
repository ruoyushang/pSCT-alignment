#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

#include "uabase.h"
#include "uavariant.h"
#include "statuscode.h"
#include "uaarraytemplates.h"

#include "common/opcua/pascominterfacecommon.hpp"

/// @details Returns -1 on invalid device type ID.
std::size_t PasComInterfaceCommon::getDeviceCount(OpcUa_UInt32 deviceType) {
    try {
        return m_pControllers.at(deviceType).size();
    }
    catch (std::out_of_range &e) {
        return -1;
    }
}

std::vector<Identity> PasComInterfaceCommon::getValidDeviceIdentities(OpcUa_UInt32 deviceType) {
    std::vector <Identity> validIdentities;
    for (auto &it : m_pControllers.at(deviceType)) {
        validIdentities.push_back(it.first);
    }

    return validIdentities;
}
