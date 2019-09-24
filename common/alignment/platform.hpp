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

class PlatformBase : public Device
{
public:
    // Public constants
    static constexpr const int NUM_ACTS_PER_PLATFORM = 6;

    static constexpr const int NUM_DB_CB_COLUMNS = 36;

    static const std::vector<Device::ErrorDefinition> ERROR_DEFINITIONS;

    Device::ErrorDefinition getErrorCodeDefinition(int errorCode) override {
        return PlatformBase::ERROR_DEFINITIONS.at(errorCode);
    }

    int getNumErrors() override { return PlatformBase::ERROR_DEFINITIONS.size(); }

    PlatformBase(Device::Identity identity, Device::DBInfo dbInfo);

    virtual ~PlatformBase() = default;

    virtual bool addActuators(std::array<Device::Identity, PlatformBase::NUM_ACTS_PER_PLATFORM> actuatorIdentities,
                              const ActuatorBase::ASFInfo &asfInfo = ActuatorBase::ASFInfo()) = 0;

    bool initialize() override;

    void setDBInfo(Device::DBInfo DBInfo);
    void unsetDBInfo();

    int getSerialNumber() { return m_Identity.serialNumber; }

    virtual bool loadCBCParameters() = 0;

    // Platform settings and readings
    virtual void enableHighCurrent() = 0;

    virtual void disableHighCurrent() = 0;

    virtual void enableSynchronousRectification() = 0;

    virtual void disableSynchronousRectification() = 0;

    virtual float getInternalTemperature() = 0;

    virtual float getExternalTemperature() = 0;

    // General device methods

    int getActuatorCount() const { return m_Actuators.size(); }

    std::unique_ptr<ActuatorBase> &getActuator(int idx) { return m_Actuators.at(idx); }

    std::unique_ptr<ActuatorBase> &getActuatorbyIdentity(const Device::Identity &identity) {
        return m_Actuators.at(m_ActuatorIdentityMap.at(identity));
    }

    int getMPESCount() const { return m_MPES.size(); }

    std::unique_ptr<MPESBase> &getMPES(int idx) { return m_MPES.at(idx); }

    std::unique_ptr<MPESBase> &getMPESbyIdentity(const Device::Identity &identity) {
        return m_MPES.at(m_MPESIdentityMap.at(identity));
    }

    // Actuator-related methods
    void probeEndStopAll(int direction);

    virtual void findHomeFromEndStopAll(int direction) = 0;

    virtual bool probeHomeAll() = 0;

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
    virtual bool addMPES(const Device::Identity &identity) = 0;
    MPESBase::Position readMPES(int idx);

    virtual void emergencyStop();

    bool isOn() override;

    virtual void turnOn() override = 0;

    virtual void turnOff() override = 0;

protected:
    static const float DEFAULT_INTERNAL_TEMPERATURE_SLOPE;
    static const float DEFAULT_INTERNAL_TEMPERATURE_OFFSET;
    static const float DEFAULT_EXTERNAL_TEMPERATURE_SLOPE;
    static const float DEFAULT_EXTERNAL_TEMPERATURE_OFFSET;

    Device::DBInfo m_DBInfo;

    bool m_On;

    void checkActuatorStatus(int actuatorIdx);

    virtual std::array<int, NUM_ACTS_PER_PLATFORM> __step(std::array<int, NUM_ACTS_PER_PLATFORM> inputSteps) = 0;

    std::array<float, NUM_ACTS_PER_PLATFORM> __measureLengths();

    virtual void __probeEndStopAll(int direction) = 0;

    bool m_HighCurrent = false;
    bool m_SynchronousRectification = true;

    float m_CalibrationTemperature = 0.0;

    std::vector<std::unique_ptr<MPESBase>> m_MPES;
    std::array<std::unique_ptr<ActuatorBase>, NUM_ACTS_PER_PLATFORM> m_Actuators;

    std::map<Device::Identity, int> m_MPESIdentityMap;
    std::map<Device::Identity, int> m_ActuatorIdentityMap;

    float m_InternalTemperatureSlope = DEFAULT_INTERNAL_TEMPERATURE_SLOPE;
    float m_InternalTemperatureOffset = DEFAULT_INTERNAL_TEMPERATURE_OFFSET;
    float m_ExternalTemperatureSlope = DEFAULT_EXTERNAL_TEMPERATURE_SLOPE;
    float m_ExternalTemperatureOffset = DEFAULT_EXTERNAL_TEMPERATURE_OFFSET;
};

#ifndef SIMMODE

#include "common/cbccode/cbc.hpp"

class Platform : public PlatformBase {
public:
    Platform(Device::Identity identity, Device::DBInfo dbInfo) : PlatformBase(
        std::move(identity), std::move(dbInfo)), m_pCBC(std::make_shared<CBC>()) {}

    ~Platform() override { turnOff(); }

    bool addActuators(std::array<Device::Identity, PlatformBase::NUM_ACTS_PER_PLATFORM> actuatorIdentities,
                      const ActuatorBase::ASFInfo &asfInfo = ActuatorBase::ASFInfo()) override;

    bool loadCBCParameters() override;

    // Platform settings and readings
    void enableHighCurrent() override;

    void disableHighCurrent() override;

    void enableSynchronousRectification() override;

    void disableSynchronousRectification() override;

    float getInternalTemperature() override;

    float getExternalTemperature() override;

    void findHomeFromEndStopAll(int direction) override;

    bool probeHomeAll() override;

    // MPES functionality

    /**
     * @brief This function adds MPES if they can be initialized at USB ports.
     In the Sim mode, MPES are added regardlessly.
     */
    bool addMPES(const Device::Identity &identity) override;

    void turnOn() override;

    void turnOff() override;

private:
    std::shared_ptr<CBC> m_pCBC;

    std::array<int, NUM_ACTS_PER_PLATFORM> __step(std::array<int, NUM_ACTS_PER_PLATFORM> inputSteps) override;

    void __probeEndStopAll(int direction) override;
};

#endif

class DummyPlatform : public PlatformBase {
public:
    // Initialization methods
    explicit DummyPlatform(Device::Identity identity, Device::DBInfo dbInfo = Device::DBInfo()) : PlatformBase(
        std::move(identity), std::move(dbInfo)) {}

    bool addActuators(std::array<Device::Identity, PlatformBase::NUM_ACTS_PER_PLATFORM> actuatorIdentities,
                      const ActuatorBase::ASFInfo &asfInfo) override;

    bool loadCBCParameters() override;

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

private:
    std::array<int, NUM_ACTS_PER_PLATFORM> __step(std::array<int, NUM_ACTS_PER_PLATFORM> inputSteps) override;

    void __probeEndStopAll(int direction) override;
};

#endif // ALIGNMENT_PLATFORM_HPP
