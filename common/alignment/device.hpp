#ifndef ALIGNMENT_DEVICE_HPP
#define ALIGNMENT_DEVICE_HPP

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
}

#endif //ALIGNMENT_DEVICE_HPP
