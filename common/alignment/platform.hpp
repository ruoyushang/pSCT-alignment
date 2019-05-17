/**
 * @file platform.hpp
 * @brief Header file for the platform/controller board hardware interface class.
 */

#ifndef ALIGNMENT_PLATFORM_HPP
#define ALIGNMENT_PLATFORM_HPP

#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "common/alignment/device.hpp"
#include "common/alignment/mpes.hpp"
#include "common/alignment/actuator.hpp"

#include "common/cbccode/cbc.hpp"

class Platform : public Device
{
public:
    // Public constants
    static constexpr const int NUM_ACTS_PER_PLATFORM = 6;

    static const std::vector<Device::ErrorDefinition> ERROR_DEFINITIONS;
    
    std::vector<Device::ErrorDefinition> getErrorCodeDefinitions() override { return Platform::ERROR_DEFINITIONS; }

    // Initialization methods
    Platform();
    Platform(std::array<int,NUM_ACTS_PER_PLATFORM> actuatorPorts, std::array<int,NUM_ACTS_PER_PLATFORM> actuatorSerials);

    Platform(Device::Identity identity, std::array<int, NUM_ACTS_PER_PLATFORM> actuatorPorts,
             std::array<int, NUM_ACTS_PER_PLATFORM> actuatorSerials, Device::DBInfo dbInfo);

    Platform(Device::Identity identity, std::array<int, NUM_ACTS_PER_PLATFORM> actuatorPorts,
             std::array<int, NUM_ACTS_PER_PLATFORM> actuatorSerials, Device::DBInfo dbInfo,
             const Actuator::ASFInfo &asfInfo);
    ~Platform();

    bool initialize() override;

    void setDBInfo(Device::DBInfo DBInfo);
    void unsetDBInfo();

    int getSerialNumber() { return m_Identity.serialNumber; }

    bool loadCBCParameters();

    Device::DeviceState getState() override;

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

    std::unique_ptr<Actuator> &getActuatorbyIdentity(const Device::Identity &identity) {
        return m_Actuators.at(m_ActuatorIdentityMap.at(identity));
    }

    int getMPESCount() const { return m_MPES.size(); }

    std::unique_ptr<MPES> &getMPES(int idx) { return m_MPES.at(idx); }

    std::unique_ptr<MPES> &getMPESbyIdentity(const Device::Identity &identity) {
        return m_MPES.at(m_MPESIdentityMap.at(identity));
    }

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
    void clearActuatorErrors();
    void clearPlatformErrors();

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

    Device::DBInfo m_DBInfo = Device::DBInfo();

    void checkActuatorStatus(int actuatorIdx);

    bool m_HighCurrent = false;
    bool m_SynchronousRectification = true;

    float m_CalibrationTemperature = 0.0;

    std::vector<std::unique_ptr<MPES>> m_MPES;
    std::array<std::unique_ptr<Actuator>, NUM_ACTS_PER_PLATFORM> m_Actuators;

    std::map<Device::Identity, int> m_MPESIdentityMap;
    std::map<Device::Identity, int> m_ActuatorIdentityMap;

    float m_InternalTemperatureSlope = DEFAULT_INTERNAL_TEMPERATURE_SLOPE;
    float m_InternalTemperatureOffset = DEFAULT_INTERNAL_TEMPERATURE_OFFSET;
    float m_ExternalTemperatureSlope = DEFAULT_EXTERNAL_TEMPERATURE_SLOPE;
    float m_ExternalTemperatureOffset = DEFAULT_EXTERNAL_TEMPERATURE_OFFSET;
};

#endif // ALIGNMENT_PLATFORM_HPP
