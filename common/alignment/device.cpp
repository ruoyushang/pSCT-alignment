#include "common/alignment/device.hpp"
#include <tuple>

#include "common/cbccode/cbc.hpp"

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

Device::Device(std::shared_ptr<CBC> pCBC, Device::Identity identity) : m_pCBC(std::move(pCBC)),
                                                                       m_Identity(std::move(identity)),
                                                                       m_state(Device::DeviceState::On) {}

void Device::setError(int errorCode) {
    if (!m_Errors.at(errorCode)) {
        std::cout << "Setting Error " << errorCode << " (" << getErrorCodeDefinitions()[errorCode].description
                  << ") for Device "
                  << m_Identity << std::endl;
        m_Errors[errorCode] = true;
        updateState();
    }
}

void Device::unsetError(int errorCode) {
    if (m_Errors.at(errorCode)) {
        std::cout << "Unsetting Error " << errorCode << " (" << getErrorCodeDefinitions()[errorCode].description
                  << ") for Device "
                  << m_Identity << std::endl;
        m_Errors[errorCode] = false;
        updateState();
    }
}

void Device::setState(Device::DeviceState state) {
    if (state > m_state) {
        std::cout << "Setting Device " << m_Identity << " status to " << deviceStateNames.at(state) << std::endl;
        m_state = state;
    }
}

void Device::updateState()//cycle through all errors and set status based on ones triggered.
{
    m_state = Device::DeviceState::On;
    for (int i = 0; i < getNumErrors(); i++) {
        if (m_Errors[i]) {
            setState(getErrorCodeDefinitions()[i].severity);
        }
    }
}

void Device::clearErrors() {
    std::cout << "Clearing All Errors for Device " << m_Identity << std::endl;
    for (int i = 0; i < getNumErrors(); i++) {
        m_Errors[i] = false;
    }
    updateState();
}
