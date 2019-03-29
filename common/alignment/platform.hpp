/**
 * @file platform.hpp
 * @brief Header file for the platform/controller board hardware interface class.
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <array>
#include <string>
#include <vector>

#ifdef _AMD64
#include "common/cbccode/dummycbc.hpp"
#warning "Compiling for AMD64 -- setting CBC to DummyCBC"
#define CBC DummyCBC
#else
#include "common/cbccode/cbc.hpp"
#endif

#include "common/alignment/actuator.hpp"
#include "common/alignment/mpesclass.hpp"


/// @brief Platform states used to disallow/stop motion based on errors.
enum class DeviceState {
    On = 0,
    Off = 1,
    Busy = 2,
    OperableError = 3, //Errors that the user should be aware of, but shouldn't interfere with normal operation of the actuator.
    FatalError = 4 //Errors that the user should definitely be aware of and handle appropriately. The Actuator should not be able to move with a fatal error without first reconfiguring something.
};

/// @brief Formal definition of a device error: description and severity.
struct ErrorDefinition {
    std::string description;
    DeviceState severity;
};

struct DBStruct {
    DBStruct() : ip(""), user(""), password(""), dbname(""), port("") {}
    std::string ip;
    std::string user;
    std::string password;
    std::string dbname;
    std::string port;
    bool empty() {
        return (ip.empty() && user.empty() && password.empty() && dbname.empty() && port.empty());
    }
};


class Platform
{
public:
    static const int NUM_ACTS_PER_PLATFORM;

    static const int NUM_ERROR_TYPES;
    static const std::array<ErrorDefinition, NUM_ERROR_TYPES> ERROR_DEFINITIONS;

    Platform();
    Platform(std::array<int,NUM_ACTS_PER_PLATFORM> actuatorPorts, std::array<int,NUM_ACTS_PER_PLATFORM> actuatorSerials);
    Platform(int CBCSerial, std::array<int,NUM_ACTS_PER_PLATFORM> actuatorPorts, std::array<int,NUM_ACTS_PER_PLATFORM> actuatorSerials, Actuator::DBStruct dbInfo);
    Platform(int CBCSerial, std::array<int,NUM_ACTS_PER_PLATFORM> actuatorPorts, std::array<int,NUM_ACTS_PER_PLATFORM> actuatorSerials, Actuator::DBStruct dbInfo, Actuator::ASFStruct asfInfo);
    ~Platform();

    bool loadCBCParameters();

    void setDBInfo(DBStruct DBInfo);
    void unsetDBInfo();

    float getInternalTemperature();
    float getExternalTemperature();

    std::array<int,NUM_ACTS_PER_PLATFORM> step(std::array<int,NUM_ACTS_PER_PLATFORM> InputSteps);
    bool GetActuatorStatus(int actuatorIdx, Actuator::StatusStruct & ActuatorStatus);
    void CheckActuatorErrorStatus(int actuatorIdx);

    void ProbeEndStopAll(int direction);
    void probeExtendStopAll() { probeEndStopAll(1); }
    void probeRetractStopAll() { probeEndStopAll(-1); }

    void findHomeFromEndStopAll(int direction);
    void findHomeFromExtendStopAll() { findHomeFromEndStopAll(1); }
    void findHomeFromRetractStopAll() { findHomeFromEndStopAll(-1); }

    bool probeHomeAll();

    std::array<float,NUM_ACTS_PER_PLATFORM> MeasureLengths();
    std::array<float,NUM_ACTS_PER_PLATFORM> MoveToLengths(std::array<float,NUM_ACTS_PER_PLATFORM> TargetLengths);
    std::array<float,NUM_ACTS_PER_PLATFORM> MoveDeltaLengths(std::array<float,NUM_ACTS_PER_PLATFORM> LengthsToMove);

    void setState(DeviceState state);
    void enableHighCurrent();
    void disableHighCurrent();
    void enableSynchronousRectification();
    void disableSynchronousRectification();
    void initialize();
    void recoverActuatorStatusFromDB(int actuatorIdx);

    void setCBCserial(int serial) { m_CBCserial = serial; }


    void clearActuatorAllErrors(int actuatorIdx) { m_Actuators.at(actuatorIdx)->clearAllErrors(); }
    void clearActuatorSingleError(int actuatorIdx, int errorCode) { m_Actuators.at(actuatorIdx)->unsetError(errorCode); }
    void clearAllPlatformErrors();
    void checkErrorStatus();

    void findHomeFromEndStop(int direction, int actuatorIdx);

    void findHomeFromExtendStop(int actuatorIdx) { findHomeFromEndStop(1, actuatorIdx); }
    void findHomeFromRetractStop(int actuatorIdx) { findHomeFromEndStop(-1, actuatorIdx); }
    void probeHome(int actuatorIdx) { m_Actuators.at(actuatorIdx)->probeHome(); }

    DeviceState getState() const { return m_State; }
    void setState(DeviceState state) { m_State = state; }

    int getActuatorCount() const { return m_Actuators.size(); }
    std::unique_ptr<Actuator>& getActuator(int idx) { return m_Actuators.at(idx) }

    int getMPESCount() const { return m_MPES.size(); }
    std::unique_ptr<MPES>& getMPES(int idx) { return m_MPES.at(idx) }


    /**
     * @brief This function adds MPES if they can be initialized at USB ports.
     In the Sim mode, MPES are added regardlessly.
     */
    bool addMPES(int USB, int serial);

    MPES::Position readMPES(int idx);

private:
    static const float DEFAULT_INTERNAL_TEMPERATURE_SLOPE;
    static const float DEFAULT_INTERNAL_TEMPERATURE_OFFSET;
    static const float DEFAULT_EXTERNAL_TEMPERATURE_SLOPE;
    static const float DEFAULT_EXTERNAL_TEMPERATURE_OFFSET;

    int m_CBCserial = 0;

    DBStruct m_DBInfo = DBStruct();

    bool m_HighCurrent = false;
    bool m_SynchronousRectification = true;

    DeviceState m_State = DeviceState::On;
    std::array<bool, NUM_ERROR_TYPES> m_Errors = { false };

    std::shared_ptr<CBC> m_pCBC;
    std::vector<std::unique_ptr<MPES>> m_MPES;
    std::array<std::unique_ptr<Actuator>, NUM_ACTS_PER_PLATFORM> m_Actuators;

    float m_InternalTemperatureSlope = DEFAULT_INTERNAL_TEMPERATURE_SLOPE;
    float m_InternalTemperatureOffset = DEFAULT_INTERNAL_TEMPERATURE_OFFSET;
    float m_ExternalTemperatureSlope = DEFAULT_EXTERNAL_TEMPERATURE_SLOPE;
    float m_ExternalTemperatureOffset = DEFAULT_EXTERNAL_TEMPERATURE_OFFSET;

    void setError(int errorCode);
    void unsetError(int errorCode);
};
#endif
