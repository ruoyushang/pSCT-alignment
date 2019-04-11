/**
 * @file platform.hpp
 * @brief Header file for the platform/controller board hardware interface class.
 */

#ifndef ALIGNMENT_PLATFORM_HPP
#define ALIGNMENT_PLATFORM_HPP

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "common/alignment/device.hpp"
#include "common/alignment/mpes.hpp"
#include "common/alignment/actuator.hpp"

class CBC;

class Platform
{
public:
    // Public constants

    static constexpr const int NUM_ACTS_PER_PLATFORM = 6;

    static constexpr const int NUM_ERROR_TYPES = 15;
    static const std::array<Device::ErrorDefinition, NUM_ERROR_TYPES> ERROR_DEFINITIONS;

    // Initialization methods
    Platform();
    Platform(std::array<int,NUM_ACTS_PER_PLATFORM> actuatorPorts, std::array<int,NUM_ACTS_PER_PLATFORM> actuatorSerials);

    Platform(int CBCSerial, std::array<int, NUM_ACTS_PER_PLATFORM> actuatorPorts,
             std::array<int, NUM_ACTS_PER_PLATFORM> actuatorSerials, Device::DBInfo dbInfo);

    Platform(int CBCSerial, std::array<int, NUM_ACTS_PER_PLATFORM> actuatorPorts,
             std::array<int, NUM_ACTS_PER_PLATFORM> actuatorSerials, Device::DBInfo dbInfo,
             Actuator::ASFInfo asfInfo);
    ~Platform();

    void initialize();

    void turnOn() { setState(Device::DeviceState::On); }

    void turnOff() { setState(Device::DeviceState::Off); }

    void setCBCserial(int serial) { m_CBCserial = serial; }
    int getCBCserial() { return m_CBCserial; }

    void setDBInfo(Device::DBInfo DBInfo);
    void unsetDBInfo();

    bool loadCBCParameters();

    // Platform settings and readings
    void enableHighCurrent();
    void disableHighCurrent();

    void enableSynchronousRectification();
    void disableSynchronousRectification();

    float getInternalTemperature();
    float getExternalTemperature();

    // General device methods

    int getActuatorCount() const { return m_Actuators.size(); }

    std::unique_ptr<Actuator> &getActuator(int idx) { return m_Actuators.at(idx); }

    int getMPESCount() const { return m_MPES.size(); }

    std::unique_ptr<MPES> &getMPES(int idx) { return m_MPES.at(idx); }

    // Actuator-related methods

    void findHomeFromEndStop(int direction, int actuatorIdx);
    void findHomeFromExtendStop(int actuatorIdx) { findHomeFromEndStop(1, actuatorIdx); }
    void findHomeFromRetractStop(int actuatorIdx) { findHomeFromEndStop(-1, actuatorIdx); }

    void probeEndStopAll(int direction);
    void probeExtendStopAll() { probeEndStopAll(1); }
    void probeRetractStopAll() { probeEndStopAll(-1); }

    void findHomeFromEndStopAll(int direction);
    void findHomeFromExtendStopAll() { findHomeFromEndStopAll(1); }
    void findHomeFromRetractStopAll() { findHomeFromEndStopAll(-1); }

    bool probeHomeAll();

    std::array<int, NUM_ACTS_PER_PLATFORM> step(std::array<int, NUM_ACTS_PER_PLATFORM> inputSteps);
    std::array<float, NUM_ACTS_PER_PLATFORM> measureLengths();
    std::array<float, NUM_ACTS_PER_PLATFORM> moveToLengths(std::array<float, NUM_ACTS_PER_PLATFORM> targetLengths);
    std::array<float, NUM_ACTS_PER_PLATFORM> moveDeltaLengths(std::array<float, NUM_ACTS_PER_PLATFORM> lengthsToMove);

    // Error functionality
    const Device::DeviceState &getState();

    void clearActuatorErrors();

    void clearPlatformErrors();

    void updateState();

    // MPES functionality

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

    Device::DBInfo m_DBInfo = Device::DBInfo();

    bool m_HighCurrent = false;
    bool m_SynchronousRectification = true;

    Device::DeviceState m_State = Device::DeviceState::On;
    std::array<bool, NUM_ERROR_TYPES> m_Errors = { false };

    std::shared_ptr<CBC> m_pCBC;
    std::vector<std::unique_ptr<MPES>> m_MPES;
    std::array<std::unique_ptr<Actuator>, NUM_ACTS_PER_PLATFORM> m_Actuators;

    float m_InternalTemperatureSlope = DEFAULT_INTERNAL_TEMPERATURE_SLOPE;
    float m_InternalTemperatureOffset = DEFAULT_INTERNAL_TEMPERATURE_OFFSET;
    float m_ExternalTemperatureSlope = DEFAULT_EXTERNAL_TEMPERATURE_SLOPE;
    float m_ExternalTemperatureOffset = DEFAULT_EXTERNAL_TEMPERATURE_OFFSET;

    void setState(Device::DeviceState state);

    void setError(int errorCode);
    void unsetError(int errorCode);
};

#endif // ALIGNMENT_PLATFORM_HPP
