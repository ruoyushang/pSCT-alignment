#ifndef ALIGNMENT_ACTUATOR_HPP
#define ALIGNMENT_ACTUATOR_HPP

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "common/alignment/platform.hpp"
#include "common/alignment/device.hpp"
#include "common/cbccode/cbc.hpp"


class Actuator
{
public:

    struct Position {
        int revolution;
        int angle;
    };

    struct ASFInfo {
        ASFInfo() : directory("/.ASF/"), prefix(".ASF_"), suffix(".log") {}

        ASFInfo(std::string directory, std::string prefix, std::string suffix) : directory(directory),
                                                                                 prefix(prefix),
                                                                                 suffix(suffix) {}

        std::string directory;
        std::string prefix;
        std::string suffix;
    };

    struct ActuatorStatus {
        Device::Date date;
        Position position;
        std::vector<int> errorCodes;
    };

    static constexpr const int NUM_ERROR_TYPES = 14;
    static const std::array<Device::ErrorDefinition, NUM_ERROR_TYPES> ERROR_DEFINITIONS;

    Actuator(std::shared_ptr<CBC> pCBC, int USBPortNumber, int serialNumber = -1,
             Device::DBInfo DBInfo = Device::DBInfo(), ASFInfo ASFFileInfo = Actuator::ASFInfo());
    ~Actuator() {}

    int getPortNumber() const { return m_PortNumber; }
    int getSerialNumber() const { return m_SerialNumber; }

    float readVoltage();
    int readAngle();
    const Position &getCurrentPosition() { return m_CurrentPosition; }

    virtual float measureLength();
    float moveToLength(float targetLength);
    float moveDeltaLength(float lengthToMove);

    Device::DeviceState getState() { return m_State; }

    void probeHome();
    void findHomeFromExtendStop() { findHomeFromEndStop(1); }
    void findHomeFromRetractStop() { findHomeFromEndStop(-1); }
    void probeExtendStop() { probeEndStop(1); }
    void probeRetractStop() { probeEndStop(-1); }

    bool getError(int errorCode) { return m_Errors[errorCode]; }
    void clearErrors();
    bool forceRecover();

    void turnOn() { setState(Device::DeviceState::On); };

    void turnOff() { setState(Device::DeviceState::Off); };

protected:
    static constexpr const int NUM_ASF_HEADER_COLUMNS = 8;//yr,mo,day,hr,min,sec,rev,angle
    static const int NUM_ASF_COLUMNS{NUM_ASF_HEADER_COLUMNS + NUM_ERROR_TYPES};

    static constexpr const int NUM_DB_HEADER_COLUMNS = 5; //id_increment, serial, start_date, rev, angle
    static const int NUM_DB_COLUMNS{NUM_DB_HEADER_COLUMNS + NUM_ERROR_TYPES};

    Device::DeviceState m_State = Device::DeviceState::On;
    std::array<bool, NUM_ERROR_TYPES> m_Errors = { false };

    int m_PortNumber = -1;
    int m_SerialNumber = -1;

    Device::DBInfo m_DBInfo;
    ASFInfo m_ASFInfo;

    void copyFile(std::string srcFilePath, std::string destFilePath);

    // Constants
    static const ASFInfo EMERGENCY_ASF_INFO;

    std::string m_ASFPath;
    std::string m_OldASFPath;
    std::string m_NewASFPath;

    CBC::ADC::adcData m_ADCdata;
    std::shared_ptr<CBC> m_pCBC;

    // Calibration Parameters (defaults)
    int m_StepsPerRevolution = 200;
    float m_mmPerStep = 0.003048;
    float m_CalibrationTemperature = 22.0;
    float m_HomeLength = 476.2;
    float m_StoppedSteppingFactor = 0.5; //hardcoded? 1 means it always stops, 0 means it never stops.
    int m_RetractRevolutionLimit{100};
    int m_ExtendRevolutionLimit{0};
    Position m_CurrentPosition{50, 0};
    Position m_RetractStopPosition{103, 32};
    Position m_ExtendStopPosition{-3, 89};
    int m_RecordingInterval{(m_StepsPerRevolution / 2) - 20};
    float m_VMin{0.593};
    float m_VMax{3.06};
    float m_dV{(m_VMax - m_VMin) / (m_StepsPerRevolution - 1)};
    int m_HysteresisSteps{m_RecordingInterval - 10};
    float m_StdDevRemeasure{m_dV / 2.0f};
    int m_MaxVoltageMeasurementAttempts{20};
    float m_StdDevMax{5.0f * m_dV};
    int m_QuickAngleCheckRange{5};
    int m_EndstopSearchStepsize{15};
    int m_CyclesDefiningHome{3};
    int m_MinimumMissedStepsToFlagError{5};
    float m_TolerablePercentOfMissedSteps{0.1};
    int m_ExtendStopToHomeStepsDeviation{m_StepsPerRevolution / 4};
    int m_FlaggedRecoverySteps{m_RecordingInterval / 10};
    int m_MaxRecoverySteps{m_RecordingInterval / 2};
    int m_EndStopRecoverySteps{m_StepsPerRevolution / 4};
    std::vector<float> m_encoderScale;

    virtual void initialize();
    void loadConfigurationAndCalibration();

    void setSerialNumber(int serialNumber) { m_SerialNumber = serialNumber; }
    void unsetSerialNumber() { m_SerialNumber = 0; }

    void setPortNumber(int portNumber) { m_PortNumber = portNumber; }
    void unsetPortNumber() { m_PortNumber = 0; }

    void createDefaultASF();

    void setASFInfo(ASFInfo ASFInfo);
    void setDBInfo(Device::DBInfo DBInfo);
    void unsetDBInfo();

    bool readStatusFromDB(ActuatorStatus &status);
    bool readStatusFromASF(ActuatorStatus &status);
    void loadStatusFromDB();
    void saveStatusToDB();
    void loadStatusFromASF();
    void saveStatusToASF();
    void recoverStatusFromDB();

    void setState(Device::DeviceState state);

    void updateState();

    void setError(int errorCode);
    void unsetError(int errorCode);

    int checkAngleQuick(Position expectedPosition);
    int checkAngleSlow(Position expectedPosition);

    void setCurrentPosition(Position position) { m_CurrentPosition = position; }
    Position predictNewPosition(Position position, int steps);

    int convertPositionToSteps(Position position);

    virtual int step(int inputSteps);
    int performHysteresisMotion(int steps);

    void findHomeFromEndStop(int direction);

    void probeEndStop(int direction);

    void recoverPosition();
};

class DummyActuator : public Actuator
{
public:
    DummyActuator(std::shared_ptr<CBC> pCBC, int USBPortNumber) : Actuator(pCBC, USBPortNumber) {};
    DummyActuator(std::shared_ptr<CBC> pCBC, int USBPortNumber, int serialNumber) : Actuator(pCBC, USBPortNumber, serialNumber) {};

    DummyActuator(std::shared_ptr<CBC> pCBC, int USBPortNumber, int serialNumber, Device::DBInfo DBInfo) : Actuator(
            pCBC, USBPortNumber, serialNumber, DBInfo) {};

    DummyActuator(std::shared_ptr<CBC> pCBC, int USBPortNumber, int serialNumber, Device::DBInfo DBInfo,
                  ASFInfo ASFInfo) : Actuator(pCBC, USBPortNumber, serialNumber, DBInfo, ASFInfo) {};

    void initialize() override;
    int step(int steps) override;
    float measureLength() override;
};

#endif // ALIGNMENT_ACTUATOR_HPP
