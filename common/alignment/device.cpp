#include "common/alignment/device.hpp"
#include <tuple>

#include "common/cbccode/cbc.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

const std::map<Device::ErrorState, std::string> Device::errorStateNames = {
    {Device::ErrorState::Nominal,       "Nominal"},
    {Device::ErrorState::OperableError, "OperableError"},
    {Device::ErrorState::FatalError,    "FatalError"},
};

const std::map<Device::DeviceState, std::string> Device::deviceStateNames = {
    {Device::DeviceState::Off,  "Off"},
    {Device::DeviceState::On,   "On"},
    {Device::DeviceState::Busy, "Busy"},
};


// definition of the comparison operator for identity struct
// does lexicographical comparison on the first non-default member:
// (comparison in the order of decreasing global uniqueness)
// name -> serial -> address -> positiion
bool Device::Identity::operator<(const Device::Identity &r) const {
    // this is problematic if an ordered map was created with ordering along one of the
    // members but new elements are added with a different ordering.
    // For instance, assume you create a map with the following keys:
    // {(11002, "169", "", 1113), (11018, "172", "", 1114), (12022, "171", "", 1126)}
    // and, not having the serial on hand, you want to find the value corresponding
    // to the key (-1, "171", "", 1126).
    // you expect the map to return the key corresponding to (12022, "171", "", 1126),
    // but it won't! it will treat your key as nonexistent, since it will reach
    // (11018, "172", "", 1114) and realize it's searched too far, since it will now
    // be comparing along the eAddress member, not the serialNumber
    if (!name.empty() && !r.name.empty()) return (name < r.name);
    if (serialNumber != -1 && r.serialNumber != -1) return (serialNumber < r.serialNumber);
    if (!eAddress.empty() && !r.eAddress.empty()) return (eAddress < r.eAddress);
    if (position != -1 && r.position != -1) return (position < r.position);

    // if failed to compare far, they don't have [overlapping] non-default members -- do
    // elementwise comparison
    return (*this != r) &&
           (std::tie(serialNumber, eAddress, name, position) <
            std::tie(r.serialNumber, r.eAddress, r.name, r.position));
}

// equal when the first non-defualt members are equal (even though others may differ),
bool Device::Identity::operator==(const Device::Identity &r) const {
    if (!name.empty() && !r.name.empty()) return (name == r.name);
    if (serialNumber != -1 && r.serialNumber != -1) return (serialNumber == r.serialNumber);
    if (!eAddress.empty() && !r.eAddress.empty()) return (eAddress == r.eAddress);
    if (position != -1 && r.position != -1) return (position == r.position);

    // if failed to compare so far, they don't have [overlapping] non-default members -- do
    // elementwise comparison
    return (serialNumber == r.serialNumber) && (eAddress == r.eAddress)
           && (name == r.name) && (position == r.position);
}

bool Device::Identity::operator!=(const Device::Identity &r) const { return !(*this == r); }

Device::Device(Device::Identity identity) : m_Identity(std::move(identity)), m_Busy(false) {}

void Device::setError(int errorCode) {
    if (!m_Errors.at(errorCode)) {
        spdlog::error("{} : Setting Error {} ({})", m_Identity, errorCode,
                      getErrorCodeDefinition(errorCode).description);
        m_Errors[errorCode] = true;
    }
}

void Device::unsetError(int errorCode) {
    if (m_Errors.at(errorCode)) {
        spdlog::info("{} : Unsetting Error {} ({})", m_Identity, errorCode,
                     getErrorCodeDefinition(errorCode).description);
        m_Errors[errorCode] = false;
    }
}

Device::DeviceState Device::getDeviceState() {
    if (isOn()) {
        if (isBusy()) {
            return DeviceState::Busy;
        } else {
            return DeviceState::On;
        }
    } else {
        return DeviceState::Off;
    }
}

Device::ErrorState Device::getErrorState() {
    Device::ErrorState state = Device::ErrorState::Nominal;
    for (int i = 0; i < getNumErrors(); i++) {
        if (m_Errors[i]) {
            if (getErrorCodeDefinition(i).severity > state) {
                state = getErrorCodeDefinition(i).severity;
            }
        }
    }

    return state;
}

void Device::clearErrors() {
    spdlog::info("{} : Clearing All Errors...", m_Identity);
    for (int i = 0; i < getNumErrors(); i++) {
        m_Errors[i] = false;
    }
}
