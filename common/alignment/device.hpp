#ifndef ALIGNMENT_DEVICE_HPP
#define ALIGNMENT_DEVICE_HPP

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "common/cbccode/cbc.hpp"


class Device {
public:
    /// @brief Platform states used to disallow/stop motion based on errors.
    enum class DeviceState {
        On = 0,
        OperableError = 1, //Errors that the user should be aware of, but shouldn't interfere with normal operation of the actuator.
        Busy = 2,
        FatalError = 3, //Errors that the user should definitely be aware of and handle appropriately. The Actuator should not be able to move with a fatal error without first reconfiguring something.
        Off = 4
    };
	
    std::map<DeviceState, std::string> deviceStateNames = {
	{DeviceState::On, "On"},
	{DeviceState::OperableError, "OperableError"},
	{DeviceState::Busy, "Busy"},
	{DeviceState::FatalError, "FatalError"},
	{DeviceState::Off, "Off"}
    };


/// @brief Formal definition of a device error: description and severity.
    struct ErrorDefinition {
        std::string description;
        DeviceState severity;
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

    Device(std::shared_ptr<CBC> pCBC, Identity identity);

    ~Device() = default;

    virtual std::vector <Device::ErrorDefinition> getErrorCodeDefinitions() = 0;
    int getNumErrors() { return (int)getErrorCodeDefinitions().size(); }

    bool getError(int errorCode) { return m_Errors[errorCode]; }

    virtual void clearErrors();

    virtual bool initialize() = 0;

    virtual Device::DeviceState getState() { return m_state; }

    virtual void turnOn() { setState(Device::DeviceState::On); }
    virtual void turnOff() { setState(Device::DeviceState::Off); }
    void setBusy() { setState(Device::DeviceState::Busy); }

    virtual void unsetError(int errorCode);

    void setError(int errorCode);

protected:
    std::shared_ptr<CBC> m_pCBC;
    Device::Identity m_Identity;

    Device::DeviceState m_state;
    std::vector<bool> m_Errors;

    void setState(Device::DeviceState state);
    void updateState();
};

inline std::ostream &operator<<(std::ostream &out, const Device::Identity &id) {
    out << "("
        << id.serialNumber << ", " << id.eAddress << ", " << id.name << ", " << id.position
        << ")";
    return out;
}

#endif //ALIGNMENT_DEVICE_HPP
