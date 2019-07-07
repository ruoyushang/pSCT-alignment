#ifndef ALIGNMENT_DEVICE_HPP
#define ALIGNMENT_DEVICE_HPP


#include <map>
#include <memory>
#include <iostream>
#include <string>
#include <vector>

#include "common/cbccode/cbc.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

class Device {
public:
    /// @brief Platform states used to disallow/stop motion based on errors.
    enum class ErrorState {
        Nominal = 0,
        OperableError = 1, //Errors that the user should be aware of, but shouldn't interfere with normal operation of the actuator.
        FatalError = 2, //Errors that the user should definitely be aware of and handle appropriately. The Actuator should not be able to move with a fatal error without first reconfiguring something.
    };

    enum class DeviceState {
        Off = 0,
        On = 1,
        Busy = 2,
    };

    static const std::map<ErrorState, std::string> errorStateNames;
    static const std::map<DeviceState, std::string> deviceStateNames;


/// @brief Formal definition of a device error: description and severity.
    struct ErrorDefinition {
        std::string description;
        ErrorState severity;
    };

    struct DBInfo {
        DBInfo() : host(""), user(""), password(""), dbname(""), port("") {}

        std::string host;
        std::string user;
        std::string password;
        std::string dbname;
        std::string port;

        bool empty() {
            return (host.empty() && user.empty() && password.empty() && dbname.empty() && port.empty());
        }
    };

    struct Date {
        int year;
        int month;
        int day;
        int hour;
        int minute;
        int second;
    };

    struct Identity {
        int serialNumber = -1; // engraved serial number
        std::string eAddress = "";     // electronic address: IP, USB port, serial port etc.
        std::string name = ""; // name of the device
        int position = -1;     // location within the immediate parent (see positioning schematic)
        // comparison operator to be able to construct associative maps with this struct as key
        bool operator<(const Identity &r) const;

        bool operator==(const Identity &r) const;

        bool operator!=(const Identity &r) const;
    };

    static Device::Identity parseIdentity(std::string identityString);

    explicit Device(Identity identity);
    ~Device() = default;

    virtual Device::ErrorDefinition getErrorCodeDefinition(int errorCode) = 0;

    int getNumErrors() { return m_Errors.size(); }

    bool getError(int errorCode) { return m_Errors[errorCode]; }

    virtual void setError(int errorCode);
    virtual void unsetError(int errorCode);
    virtual void clearErrors();

    virtual bool initialize() = 0;

    virtual Device::DeviceState getDeviceState();

    virtual Device::ErrorState getErrorState();

    virtual void turnOn() = 0;
    virtual void turnOff() = 0;

    Device::Identity getIdentity() { return m_Identity; }

protected:
    Device::Identity m_Identity;

    std::vector<bool> m_Errors;

    bool m_Busy;

    class CustomBusyLock {
    public:
        explicit CustomBusyLock(Device *device) : m_Device(device) {
            spdlog::trace("{} : Blocking task started.", m_Device->getIdentity());
            m_Device->m_Busy = true;
        }

        ~CustomBusyLock() {
            spdlog::trace("{} : Blocking task completed.", m_Device->getIdentity());
            m_Device->m_Busy = false;
        };

    private:
        Device *m_Device;
    };

    bool isBusy() { return m_Busy; }

    virtual bool isOn() = 0;
};

inline std::ostream& operator<<(std::ostream& out, const Device::Identity& id) {
    return out << "("
        << id.serialNumber << ", " << id.eAddress << ", " << id.name << ", " << id.position
        << ")";
}

#endif //ALIGNMENT_DEVICE_HPP
