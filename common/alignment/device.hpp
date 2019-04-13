#ifndef ALIGNMENT_DEVICE_HPP
#define ALIGNMENT_DEVICE_HPP

#include <iostream>
#include <string>

namespace Device {
    /// @brief Platform states used to disallow/stop motion based on errors.
    enum class DeviceState {
        On = 0,
        OperableError = 1, //Errors that the user should be aware of, but shouldn't interfere with normal operation of the actuator.
        Busy = 2,
        FatalError = 3, //Errors that the user should definitely be aware of and handle appropriately. The Actuator should not be able to move with a fatal error without first reconfiguring something.
        Off = 4
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
}

inline std::ostream &operator<<(std::ostream &out, const Device::Identity &id) {
    out << "("
        << id.serialNumber << ", " << id.eAddress << ", " << id.name << ", " << id.position
        << ")";
    return out;
}

#endif //ALIGNMENT_DEVICE_HPP
