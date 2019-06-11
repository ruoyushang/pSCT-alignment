#ifndef ALIGNMENT_ACTUATOR_HPP
#define ALIGNMENT_ACTUATOR_HPP

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "common/alignment/device.hpp"
#include "common/cbccode/cbc.hpp"

class Platform;


class Actuator : public Device
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

    std::vector<Device::ErrorDefinition> getErrorCodeDefinitions() override { return Actuator::ERROR_DEFINITIONS; }

    Actuator(std::shared_ptr<CBC> pCBC, Device::Identity identity,
             Device::DBInfo DBInfo = Device::DBInfo(), const ASFInfo &ASFFileInfo = Actuator::ASFInfo());

    ~Actuator() = default;

    int getPortNumber() const { return std::stoi(m_Identity.eAddress); }
    int getSerialNumber() const { return (int) m_Identity.serialNumber; }

    float readVoltage();
    int readAngle();
    const Position &getCurrentPosition() { return m_CurrentPosition; }

    virtual float measureLength();
    float moveToLength(float targetLength);
    float moveDeltaLength(float lengthToMove);

    virtual void probeHome();

    virtual void findHomeFromEndStop(int direction);

    virtual bool forceRecover();

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

    virtual int step(int inputSteps);

    virtual int stepDriver(int inputSteps);

    int performHysteresisMotion(int steps);

    void recoverPosition();

    Position predictNewPosition(Position position, int steps);

    void loadStatusFromASF();

    bool isOn() override;

    void turnOn() override;
    void turnOff() override;

    bool initialize() override;

    virtual void emergencyStop();

protected:
    bool m_keepStepping;

    static constexpr const int NUM_ASF_HEADER_COLUMNS = 8;//yr,mo,day,hr,min,sec,rev,angle
    int NUM_ASF_COLUMNS{NUM_ASF_HEADER_COLUMNS + getNumErrors()};

    static constexpr const unsigned NUM_DB_HEADER_COLUMNS = 5; //id_increment, serial, start_date, rev, angle
    unsigned NUM_DB_COLUMNS{NUM_DB_HEADER_COLUMNS + getNumErrors()};

    Device::DBInfo m_DBInfo;

    static void copyFile(const std::string &srcFilePath, const std::string &destFilePath);

    // Constants
    static const ASFInfo EMERGENCY_ASF_INFO;

    std::string m_ASFPath;
    std::string m_OldASFPath;
    std::string m_NewASFPath;

    CBC::ADC::adcData m_ADCdata;

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

    bool readStatusFromDB(ActuatorStatus &status);
    bool readStatusFromASF(ActuatorStatus &status);
    void loadStatusFromDB();
    void saveStatusToDB();
    void saveStatusToASF();
    void recoverStatusFromDB();

    int checkAngleQuick(Position expectedPosition);
    int checkAngleSlow(Position expectedPosition);

    void setCurrentPosition(Position position) { m_CurrentPosition = position; }

    void probeEndStop(int direction);

    virtual float __readVoltage();

    virtual int __step(int steps);

    virtual void __probeEndStop(int direction);
};

class DummyActuator : public Actuator
{
public:
    explicit DummyActuator(Device::Identity identity,
                           Device::DBInfo DBInfo = Device::DBInfo(), const ASFInfo &ASFFileInfo = Actuator::ASFInfo())
        : Actuator(nullptr, std::move(identity), std::move(DBInfo), ASFFileInfo), m_On(true) {};

    bool initialize() override;
    float measureLength() override;

    void probeHome() override;

    void findHomeFromEndStop(int direction) override;

    bool forceRecover() override;

    void clearErrors() override;

    int stepDriver(int inputSteps) override;

    bool isOn() override;

    void turnOn() override;

    void turnOff() override;

    void emergencyStop() override;

private:
    bool m_On;

    int __step(int steps) override;

    void __probeEndStop(int direction) override;
};

#endif // ALIGNMENT_ACTUATOR_HPP
