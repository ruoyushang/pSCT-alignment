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

    Platform(Device::Identity identity, Device::DBInfo dbInfo, const Actuator::ASFInfo &asfInfo);
    ~Platform();

    virtual bool addActuators(std::array<Device::Identity, Platform::NUM_ACTS_PER_PLATFORM> actuatorIdentities,
                              Actuator::ASFInfo asfInfo = Actuator::ASFInfo());

    bool initialize() override;

    void setDBInfo(Device::DBInfo DBInfo);
    void unsetDBInfo();

    int getSerialNumber() { return m_Identity.serialNumber; }

    bool loadCBCParameters();

    // Platform settings and readings
    virtual void enableHighCurrent();

    virtual void disableHighCurrent();

    virtual void enableSynchronousRectification();

    virtual void disableSynchronousRectification();

    virtual float getInternalTemperature();

    virtual float getExternalTemperature();

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
    void probeEndStopAll(int direction);

    virtual void findHomeFromEndStopAll(int direction);

    virtual bool probeHomeAll();

    std::array<int, NUM_ACTS_PER_PLATFORM> step(std::array<int, NUM_ACTS_PER_PLATFORM> inputSteps);
    std::array<float, NUM_ACTS_PER_PLATFORM> measureLengths();
    std::array<float, NUM_ACTS_PER_PLATFORM> moveToLengths(std::array<float, NUM_ACTS_PER_PLATFORM> targetLengths);
    std::array<float, NUM_ACTS_PER_PLATFORM> moveDeltaLengths(std::array<float, NUM_ACTS_PER_PLATFORM> lengthsToMove);

    // Error functionality
    void unsetError(int errorCode) override;
    void clearActuatorErrors();
    void clearPlatformErrors();

    Device::ErrorState getErrorState() override;

    // MPES functionality

    /**
     * @brief This function adds MPES if they can be initialized at USB ports.
     In the Sim mode, MPES are added regardlessly.
     */
    virtual bool addMPES(const Device::Identity &identity);
    MPES::Position readMPES(int idx);

    virtual void emergencyStop();

    bool isOn() override;

    void turnOn() override;

    void turnOff() override;

protected:
    static const float DEFAULT_INTERNAL_TEMPERATURE_SLOPE;
    static const float DEFAULT_INTERNAL_TEMPERATURE_OFFSET;
    static const float DEFAULT_EXTERNAL_TEMPERATURE_SLOPE;
    static const float DEFAULT_EXTERNAL_TEMPERATURE_OFFSET;

    Device::DBInfo m_DBInfo;

    bool m_On;

    void checkActuatorStatus(int actuatorIdx);

    virtual std::array<int, NUM_ACTS_PER_PLATFORM> __step(std::array<int, NUM_ACTS_PER_PLATFORM> inputSteps);

    std::array<float, NUM_ACTS_PER_PLATFORM> __measureLengths();

    virtual void __probeEndStopAll(int direction);

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

class DummyPlatform : public Platform 
{
    public:
    // Initialization methods
    DummyPlatform(Device::Identity identity, Device::DBInfo dbInfo = Device::DBInfo(),
                  const Actuator::ASFInfo &asfInfo = Actuator::ASFInfo()) : Platform(identity, dbInfo,
                                                                                     asfInfo) {}

    ~DummyPlatform() = default;

    bool initialize() override;

    // Platform settings and readings
    void enableHighCurrent() override;

    void disableHighCurrent() override;

    void enableSynchronousRectification() override;

    void disableSynchronousRectification() override;

    float getInternalTemperature() override;

    float getExternalTemperature() override;

    // General device methods

    // Actuator-related methods
    void findHomeFromEndStopAll(int direction) override;

    bool probeHomeAll() override;

    // MPES functionality

    /**
     * @brief This function adds MPES if they can be initialized at USB ports.
     In the Sim mode, MPES are added regardlessly.
     */
    bool addMPES(const Device::Identity &identity) override;

    void emergencyStop() override;

    void turnOn() override;

    void turnOff() override;

    bool addActuators(std::array<Device::Identity, Platform::NUM_ACTS_PER_PLATFORM> actuatorIdentities,
                      Actuator::ASFInfo asfInfo) override;

private:
    std::array<int, NUM_ACTS_PER_PLATFORM> __step(std::array<int, NUM_ACTS_PER_PLATFORM> inputSteps) override;

    void __probeEndStopAll(int direction) override;
};

#endif // ALIGNMENT_PLATFORM_HPP
