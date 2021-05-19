#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

#include "uabase.h"
#include "uavariant.h"
#include "statuscode.h"
#include "uaarraytemplates.h"

#include "common/opcua/pascominterfacecommon.hpp"
#include "common/alignment/device.hpp"

/// @details Returns -1 on invalid device type ID.
std::size_t PasComInterfaceCommon::getDeviceCount(OpcUa_UInt32 deviceType) {
    if (m_pControllers.find(deviceType) != m_pControllers.end()) {
        return m_pControllers.at(deviceType).size();
    }
    else {
        return 0;
    }
}

std::vector<Device::Identity> PasComInterfaceCommon::getValidDeviceIdentities(OpcUa_UInt32 deviceType) {
    std::vector<Device::Identity> validIdentities;
    if (m_pControllers.find(deviceType) != m_pControllers.end()) {
        auto devices = m_pControllers.at(deviceType);
        for (auto &it : m_pControllers.at(deviceType)) {
            validIdentities.push_back(it.first);
        }
        return validIdentities;
    }
    else {
        return validIdentities;
    }
}
