#ifndef ALIGNMENT_ACTUATOR_HPP
#define ALIGNMENT_ACTUATOR_HPP

#include <array>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "common/alignment/device.hpp"

class Platform;

class ActuatorBase : public Device
{
public:

    struct Position {
        int revolution;
        int angle;
    };

    struct ASFInfo {
        ASFInfo() : directory("/.ASF/"), prefix(".ASF_"), suffix(".log") {}

        ASFInfo(std::string directory, std::string prefix, std::string suffix) : directory(std::move(directory)),
                                                                                 prefix(std::move(prefix)),
                                                                                 suffix(std::move(suffix)) {}

        std::string directory;
        std::string prefix;
        std::string suffix;
    };

    struct ActuatorStatus {
        Device::Date date;
        Position position;
        std::vector<int> errorCodes;
    };

    static const std::vector<Device::ErrorDefinition> ERROR_DEFINITIONS;

    Device::ErrorDefinition getErrorCodeDefinition(int errorCode) override {
        return ActuatorBase::ERROR_DEFINITIONS.at(errorCode);
    }

    int getNumErrors() override { return ActuatorBase::ERROR_DEFINITIONS.size(); }

    explicit ActuatorBase(Device::Identity identity, Device::DBInfo DBInfo = Device::DBInfo(),
                          const ASFInfo &ASFFileInfo = ASFInfo());

    ~ActuatorBase() = default;

    int getPortNumber() const { return std::stoi(m_Identity.eAddress); }
    int getSerialNumber() const { return (int) m_Identity.serialNumber; }

    float readVoltage();
    int readAngle();
    const Position &getCurrentPosition() { return m_CurrentPosition; }

    float measureLength();

    float moveToLength(float targetLength);
    float moveDeltaLength(float lengthToMove);

    virtual void probeHome() = 0;

    void probeEndStop(int direction);

    void findHomeFromEndStop(int direction);

    bool forceRecover();

    void clearErrors() override;

    int EndstopSearchStepsize{15};
    float dV{(m_VMax - m_VMin) / (StepsPerRevolution - 1)};
    float mmPerStep = 0.003048;
    int RecordingInterval{(StepsPerRevolution / 2) - 20};
    int RetractRevolutionLimit{100};
    int ExtendRevolutionLimit{0};
    int StepsPerRevolution = 200;
    float HomeLength = 476.2;

    int convertPositionToSteps(Position position);

    int step(int inputSteps);

    int performHysteresisMotion(int steps);
    Position predictNewPosition(Position position, int steps);

    void recoverPosition();

    void loadStatusFromASF();

    virtual int stepDriver(int inputSteps) = 0;

    virtual void turnOn() override = 0;

    virtual void turnOff() override = 0;

    bool initialize() override;

    virtual void emergencyStop();

protected:
    virtual bool isOn() override = 0;

    // Constants
    static const ASFInfo EMERGENCY_ASF_INFO;

    static constexpr const int NUM_ASF_HEADER_COLUMNS = 8;//yr,mo,day,hr,min,sec,rev,angle
    int NUM_ASF_COLUMNS{NUM_ASF_HEADER_COLUMNS + getNumErrors()};

    static constexpr const unsigned NUM_DB_HEADER_COLUMNS = 5; //id_increment, serial, start_date, rev, angle
    unsigned NUM_DB_COLUMNS{NUM_DB_HEADER_COLUMNS + getNumErrors()};

    Device::DBInfo m_DBInfo;
    bool m_keepStepping;

    static void copyFile(const std::string &srcFilePath, const std::string &destFilePath);

    std::string m_ASFPath;
    std::string m_OldASFPath;
    std::string m_NewASFPath;

    // Calibration Parameters (defaults)
    float m_CalibrationTemperature = 22.0;
    float m_StoppedSteppingFactor = 0.5; //hardcoded? 1 means it always stops, 0 means it never stops.
    Position m_CurrentPosition{50, 0};
    Position m_RetractStopPosition{103, 32};
    Position m_ExtendStopPosition{-3, 89};
    float m_VMin{0.593};
    float m_VMax{3.06};
    int m_HysteresisSteps{RecordingInterval - 10};
    float m_StdDevRemeasure{dV / 2.0f};
    int m_MaxVoltageMeasurementAttempts{20};
    float m_StdDevMax{5.0f * dV};
    int m_QuickAngleCheckRange{5};
    int m_CyclesDefiningHome{3};
    int m_MinimumMissedStepsToFlagError{5};
    float m_TolerablePercentOfMissedSteps{0.1};
    int m_ExtendStopToHomeStepsDeviation{StepsPerRevolution / 4};
    int m_FlaggedRecoverySteps{RecordingInterval / 10};
    int m_MaxRecoverySteps{RecordingInterval / 2};
    int m_EndStopRecoverySteps{StepsPerRevolution / 4};
    std::vector<float> m_encoderScale;

    bool loadConfigurationAndCalibration();

    void createDefaultASF();

    void setASFInfo(const ASFInfo &ASFInfo);
    void setDBInfo(Device::DBInfo DBInfo);
    void unsetDBInfo();

    virtual bool readStatusFromDB(ActuatorStatus &status) = 0;

    virtual void loadStatusFromDB() = 0;

    virtual void saveStatusToDB() = 0;

    void recoverStatusFromDB();
    bool readStatusFromASF(ActuatorStatus &status);
    void saveStatusToASF();

    virtual int checkAngleQuick(Position expectedPosition);

    virtual int checkAngleSlow(Position expectedPosition);

    void setCurrentPosition(Position position) { m_CurrentPosition = position; }

    float __measureLength();

    virtual float __readVoltage() = 0;

    virtual int __step(int steps) = 0;

    virtual void __probeEndStop(int direction) = 0;

    virtual void __findHomeFromEndStop(int direction) = 0;
};

#ifndef SIMMODE

#include "common/cbccode/cbc.hpp"

class Actuator : public ActuatorBase {
public:
    Actuator(std::shared_ptr<CBC> pCBC, Device::Identity identity,
             Device::DBInfo DBInfo = Device::DBInfo(),
             const ASFInfo &ASFFileInfo = ActuatorBase::ASFInfo(std::string(getenv("HOME")) + "/.ASF/", ".ASF_",
                                                                ".log")) : ActuatorBase(
        std::move(identity), std::move(DBInfo), ASFFileInfo), m_pCBC(std::move(pCBC)), m_ADCdata() {}

    ~Actuator() = default;

    int stepDriver(int inputSteps) override;

    bool isOn() override;

    void turnOn() override;

    void turnOff() override;

    void probeHome() override;

protected:
    std::shared_ptr<CBC> m_pCBC;
    CBC::ADC::adcData m_ADCdata;

    bool readStatusFromDB(ActuatorStatus &status) override;

    void loadStatusFromDB() override;

    void saveStatusToDB() override;

    float __readVoltage() override;

    int __step(int steps) override;

    void __probeEndStop(int direction) override;

    void __findHomeFromEndStop(int direction) override;
};

#endif

class DummyActuator : public ActuatorBase
{
public:
    explicit DummyActuator(Device::Identity identity,
                           Device::DBInfo DBInfo = Device::DBInfo(), const ASFInfo &ASFFileInfo = ActuatorBase::ASFInfo())
        : ActuatorBase(std::move(identity), std::move(DBInfo), ASFFileInfo), m_On(true) {};

    int stepDriver(int inputSteps) override;

    bool isOn() override;
    void turnOn() override;
    void turnOff() override;

    void probeHome() override;

protected:
    bool m_On;

    // Do not write to DB when using simmode (to avoid risk of overwriting real data)
    bool readStatusFromDB(ActuatorStatus &status) override { return readStatusFromASF(status); };

    void loadStatusFromDB() override { return loadStatusFromASF(); };

    void saveStatusToDB() override { return saveStatusToASF(); };

    float __readVoltage() override;

    int __step(int steps) override;

    void __probeEndStop(int direction) override;

    void __findHomeFromEndStop(int direction) override;
};

#endif // ALIGNMENT_ACTUATOR_HPP
