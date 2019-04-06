#ifndef __ACTUATOR_H__
#define __ACTUATOR_H__

# ifndef _AMD64
#  include "common/cbccode/cbc.hpp"
# else

#  include "common/cbccode/dummycbc.hpp"

#  define CBC DummyCBC
# endif

# include <array>
# include <memory>
# include <string>
# include <vector>


class Actuator
{
public:

    struct Position {
        int Revolution;
        int Angle;
    };

    struct Date {
        int Year;
        int Month;
        int Day;
        int Hour;
        int Minute;
        int Second;
    };

    struct ASFFileLocation {
        std::string Directory;
        std::string FilenamePrefix;
        std::string FilenameSuffix;
    };

    struct ActuatorStatus {
        Date Date;
        Position Position;
        std::vector<int> ErrorCodes;
    };

    static const int NUM_ERROR_TYPES;
    static const std::array<Platform::ErrorDefinition, NUM_ERROR_TYPES> ERROR_DEFINITIONS;

    Actuator(std::shared_ptr<CBC> pCBC, int USBPortNumber);
    Actuator(std::shared_ptr<CBC> pCBC, int USBPortNumber, int serialNumber);
    Actuator(std::shared_ptr<CBC> pCBC, int USBPortNumber, int serialNumber, DBStruct DBInfo);

    Actuator(std::shared_ptr<CBC> pCBC, int USBPortNumber, int serialNumber, DBStruct DBInfo, ASFFileLocation ASFInfo);
    ~Actuator() {}

    static const int NumberOfIntsInASFHeader{8};//yr,mo,day,hr,min,sec,rev,angle
    static const int NumberOfColumnsInDBHeader{5}; //id_increment, serial, start_date, rev, angle
    static const int NumberOfIntsInASF{NumberOfIntsInASFHeader+NUM_ERROR_TYPES};
    int NumberOfColumnsInDB{NumberOfColumnsInDBHeader+NumberOfErrorCodes};
    std::vector<float> EncoderCalibration;

    //do checks on constructors to make sure numbers are okay. e.g. hysteresis steps are positive, recording interval is less than half of STEPS_PER_REVOLUTION.

    void ReadConfigurationAndCalibration();

    bool ReadStatusFromDB(ActuatorStatus &RecordedPosition);
    void LoadStatusFromDB();
    void RecordStatusToDB();

    bool ReadStatusFromASF(ActuatorStatus &RecordedPosition);
    void LoadStatusFromASF();
    void RecordStatusToASF();
    float MeasureVoltage();
    int MeasureAngle();

    int QuickAngleCheck(Position ExpectedPosition);

    int SlowAngleCheck(Position ExpectedPosition);
    virtual int Step(int InputSteps);

    Position PredictPosition(Position InputPosition, int InputSteps);
    int HysteresisMotion(int InputSteps);

    virtual void initialize();

    void setCurrentPosition(Position inputPosition) { currentPosition = inputPosition; }
    void CheckCurrentPosition();

    void SetASFFullPath(ASFFileLocation ASFInfoInfo);
    void SetDB(DBStruct DBInfoInfo);
    void UnsetDB();

    void setSerialNumber(int serialNumber) { m_SerialNumber = serialNumber; }
    void unsetSerialNumber() { m_SerialNumber = 0; }
    void setPortNumber(int portNumber) { m_PortNumber = portNumber; }
    void unsetPortNumber() { m_PortNumber = 0; }

    void SetMM_PER_STEP(float InputMM_PER_STEP);
    void SetSTEPS_PER_REVOLUTION(int InputSTEPS_PER_REVOLUTION);
    void SetRecordingInterval(int InputRecordingInterval);
    void SetHOME_LENGTH(float InputHOME_LENGTH, float InputCALIBRATION_TEMPERATURE);

    int CalculateStepsFromHome(Position InputPosition);
    void SetError(int CodeNumber);
    void UnsetError(int CodeNumber);
    void SetStatus(StatusModes InputStatus);
    void CheckErrorStatus();

    void probeHome();

    void findHomeFromEndStop(int direction);
    void findHomeFromExtendStop() { findHomeFromEndStop(1); }
    void findHomeFromRetractStop(); { findHomeFromEndStop(-1); }

    void probeEndStop(int direction);
    void probeExtendStop() { probeEndStop(1); }
    void probeRetractStop() { probeEndStop(-1); }
    int getPortNumber() const { return m_PortNumber; }
    Platform::DeviceState getState() { return m_State; }
    void createDefaultASF();
    virtual float measureLength();
    float moveToLength(float targetLength);
    float moveDeltaLength(float lengthToMove);
    void clearAllErrors();
    bool forceRecover();

protected:
    Platform::DeviceState m_State = Platform::DeviceState::On;

    std::array<bool, NUM_ERROR_TYPES> m_Errors = { false };

    int m_PortNumber{0};
    int m_SerialNumber = 0;

    DBStruct m_DBInfo;
    ASFStruct m_ASFInfo;

    void copyFile(std::string srcFilePath, std::string destFilePath);

    // Constants

    static const ASFStruct DEFAULT_ASF_INFO;
    static const ASFStruct EMERGENCY_ASF_INFO;

    std::string ASFFullPath;
    std::string OldASFFullPath;
    std::string NewASFFullPath;

    CBC::ADC::adcData adcdata;
    std::shared_ptr<CBC> cbc;

    bool DBFlag{false};

    static const int STEPS_PER_REVOLUTION = 200;
    static const float MM_PER_STEP = 0.003048;
    static const float CALIBRATION_TEMPERATURE = 22.0;
    static const float HOME_LENGTH = 476.2;
    float StoppedSteppingFactor = 0.5; //hardcoded? 1 means it always stops, 0 means it never stops.
    int RetractRevolutionLimit{100};
    int ExtendRevolutionLimit{0};
    PositionStruct CurrentPosition{50,0};
    PositionStruct RetractStop{103,32};
    PositionStruct ExtendStop{-3,89};
    int RecordingInterval{(STEPS_PER_REVOLUTION/2)-20};
    float VMin{0.593};
    float VMax{3.06};
    float dV{(VMax-VMin)/(STEPS_PER_REVOLUTION-1)};
    int HysteresisSteps{RecordingInterval-10};
    float StdDevRemeasure{dV/2.0f};
    int MaxVoltageMeasurementAttempts{20};
    float StdDevMax{5.0f*dV};
    int QuickAngleCheckRange{5};
    int EndstopSearchStepsize{15};
    int CyclesDefiningHome{3};
    int MinimumMissedStepsToFlagError{5};
    float TolerablePercentOfMissedSteps{0.1};
    int ExtendStopToHomeStepsDeviation{STEPS_PER_REVOLUTION/4};
    int FlaggedRecoverySteps{RecordingInterval/10};
    int MaxRecoverySteps{RecordingInterval/2};
    int EndStopRecoverySteps{STEPS_PER_REVOLUTION/4};

};

class DummyActuator : public Actuator
{
public:
    DummyActuator(std::shared_ptr<CBC> pCBC, int USBPortNumber) : Actuator(pCBC, USBPortNumber) {};
    DummyActuator(std::shared_ptr<CBC> pCBC, int USBPortNumber, int serialNumber) : Actuator(pCBC, USBPortNumber, serialNumber) {};
    DummyActuator(std::shared_ptr<CBC> pCBC, int USBPortNumber, int serialNumber, DBStruct DBInfo) : Actuator(pCBC, USBPortNumber, serialNumber, DBInfo) {};

    DummyActuator(std::shared_ptr<CBC> pCBC, int USBPortNumber, int serialNumber, DBStruct DBInfo,
                  ASFFileLocation ASFInfo) : Actuator(pCBC, USBPortNumber, serialNumber, DBInfo, ASFInfo) {};

    void initialize() override;
    int step(int inputSteps) override;
    float measureLength() override;
};

#endif
