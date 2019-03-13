#ifndef ACTUATOR_H
#define ACTUATOR_H

#ifndef _AMD64
    #include "cbc.hpp"
#else
    #include "dummycbc.hpp"
    #define CBC DummyCBC
#endif

#include <vector>
#include <string>

class Actuator
{

public:

enum class StatusModes {
Healthy=1,//Everything is good
OperableError=2,//Errors that the user should be aware of, but shouldn't interfere with normal operation of the actuator.
FatalError=3//Errors that the user should definitely be aware of and handle appropriately. The Actuator should not be able to move with a fatal error without first reconfiguring something.
};

StatusModes ErrorStatus {OperableError};

struct ErrorInfo {
bool Triggered;
std::string ErrorDescription;
StatusModes ErrorType;
};

struct PositionStruct {
int Revolution;
int Angle;
};

struct DateStruct {
int Year;
int Month;
int Day;
int Hour;
int Minute;
int Second;
};

struct DBStruct {
std::string ip;
std::string user;
std::string password;
std::string dbname;
std::string port;
};

struct ASFStruct {
std::string Directory;
std::string FilenamePrefix;
std::string FilenameSuffix;
};


Actuator::ASFStruct DefaultASFInfo {"/.ASF/",".ASF_",".log"};

Actuator::ASFStruct EmergencyASFInfo {"/.ASF/",".ASF_Emergency_Port_",".log"};

std::string ASFFullPath;
std::string OldASFFullPath;
std::string NewASFFullPath;
CBC::ADC::adcData adcdata; 
CBC* cbc;
DBStruct DBInfo;
ASFStruct ASFInfo;
bool DBFlag{false};
bool VoltageError{false};
int PortNumber{0};
int SerialNumber{0};
int StepsPerRevolution{200};
float mmPerStep{0.003048};
float CalibrationTemperature{22.0};
float HomeLength{476.2};
int RetractRevolutionLimit{100};
int ExtendRevolutionLimit{0};
PositionStruct CurrentPosition{50,0};
PositionStruct RetractStop{103,32};
PositionStruct ExtendStop{-3,89};
int RecordingInterval{(StepsPerRevolution/2)-20};
float VMin{0.593};
float VMax{3.06};
float dV{(VMax-VMin)/(StepsPerRevolution-1)};
int HysteresisSteps{RecordingInterval-10};
float StdDevRemeasure{dV/2.0f};
int MaxVoltageMeasurementAttempts{20};
float StdDevMax{5.0f*dV};
int QuickAngleCheckRange{5};
int EndstopSearchStepsize{15};
int CyclesDefiningHome{3};
int MinimumMissedStepsToFlagError{5};
float TolerablePercentOfMissedSteps{0.1};
int ExtendStopToHomeStepsDeviation{StepsPerRevolution/4};
int FlaggedRecoverySteps{RecordingInterval/10};
int MaxRecoverySteps{RecordingInterval/2};
int EndStopRecoverySteps{StepsPerRevolution/4};


std::vector<ErrorInfo> ActuatorErrors{
{true, "Home Position is not calibrated", FatalError},//error 0
{false, "DBFlagNotSet", OperableError},//error 1
{false, "MySQL Communication Error", OperableError},//error 2
{false, "DB Columns does not match what is expected", FatalError},//error 3
{false, "ASF File is Bad", FatalError},//error 4
{false, "ASF File entries does not match what is expected", FatalError},//error 5
{false, "DB recording more recent than ASF and has mismatch with measured angle", FatalError},//error 6
{false, "Voltage Std Dev is entirely too high", FatalError},//error 7
{false, "Actuator Missed too many steps", FatalError},//error 8
{false, "Actuator position is too many steps away to recover safely", FatalError},//error 9
{false, "Actuator position is recovering large amount of steps, should be ok", OperableError},//error 10
{false, "Extend Stop Voltage is too close to the discontinuity. Possible 1 cycle uncertainty with calibrated data", OperableError},//error 11
{false, "End stop is large number of steps away from what is expected. Possible uncertainty in home position", OperableError},//error 12
{false, "Discrepancy between number of steps from extend stop and recorded number of steps from end stop is too high. Possible uncertainty in probed home position", OperableError}//error 13
};//do not simply add a new error to this vector without changing the ASF files. If ASF text files are saved with 13 errors and this code is updated to 14 errors, text files must also be updated to 14 errors. otherwise the number of arguments in ASF file won't match, and an error will occur.

int NumberOfIntsInASFHeader{8};//yr,mo,day,hr,min,sec,rev,angle
int NumberOfColumnsInDBHeader{5};//id_increment, serial, start_date, rev, angle
int NumberOfErrorCodes{(int) ActuatorErrors.size()};
int NumberOfIntsInASF{NumberOfIntsInASFHeader+NumberOfErrorCodes};
int NumberOfColumnsInDB{NumberOfColumnsInDBHeader+NumberOfErrorCodes};
std::vector<float> EncoderCalibration;

struct StatusStruct {
DateStruct Date;
PositionStruct Position;
std::vector<int> ErrorCodes;
};

//do checks on constructors to make sure numbers are okay. e.g. hysteresis steps are positive, recording interval is less than half of stepsperrevolution.

void ReadConfigurationAndCalibration();
bool ReadStatusFromDB(StatusStruct & RecordedPosition);
void LoadStatusFromDB();
void RecordStatusToDB();
bool ReadStatusFromASF(StatusStruct & RecordedPosition);
void LoadStatusFromASF();
void RecordStatusToASF();
float MeasureVoltage();
int MeasureAngle();
int QuickAngleCheck(PositionStruct ExpectedPosition);
int SlowAngleCheck(PositionStruct ExpectedPosition);
virtual int Step(int InputSteps);
PositionStruct PredictPosition(PositionStruct InputPosition, int InputSteps);
int HysteresisMotion(int InputSteps);
virtual void Initialize();
void SetCurrentPosition(PositionStruct InputPosition);
void CheckCurrentPosition();
void SetASFFullPath(ASFStruct InputASFInfo);
void SetDB(DBStruct InputDBInfo);
void UnsetDB();
void SetSerialNumber(int InputSerialNumber);
void UnsetSerialNumber();
void SetPortNumber(int InputPortNumber);
void UnsetPortNumber();
void SetmmPerStep(float InputmmPerStep);
void SetStepsPerRevolution(int InputStepsPerRevolution);
void SetRecordingInterval(int InputRecordingInterval);
void SetHomeLength(float InputHomeLength, float InputCalibrationTemperature);
int CalculateStepsFromHome(PositionStruct InputPosition);
void SetError(int CodeNumber);
void UnsetError(int CodeNumber);
void SetStatus(StatusModes InputStatus);
void CheckErrorStatus();
void ProbeHome();
void FindHomeFromEndStop(int Direction);
void FindHomeFromExtendStop();
void FindHomeFromRetractStop();
void ProbeEndStop(int Direction);
void ProbeExtendStop();
void ProbeRetractStop();
int GetPortNumber() const;
StatusModes GetStatus();
//void RecoverStatusFromDBAndASF();
void CreateDefaultASF();
virtual float MeasureLength();
float MoveToLength(float TargetLength);
float MoveDeltaLength(float LengthToMove);
void ClearAllErrors();
void ForceRecover();
void CopyFile(std::string srcfile, std::string destfile);
///////////////////////////////////////////////////////////////

Actuator();
Actuator(CBC* InputCBC, int InputPortNumber);
Actuator(CBC* InputCBC, int InputPortNumber, int InputActuatorSerial);
Actuator(CBC* InputCBC, int InputPortNumber, int InputActuatorSerial, DBStruct InputDB);
Actuator(CBC* InputCBC, int InputPortNumber, int InputActuatorSerial, DBStruct InputDB, ASFStruct InputASF);
~Actuator();

};

class DummyActuator : public Actuator
{
    public:
        DummyActuator(CBC* InputCBC, int InputPortNumber) : Actuator(InputCBC, InputPortNumber) {};
        DummyActuator(CBC* InputCBC, int InputPortNumber, int InputActuatorSerial) : Actuator(InputCBC, InputPortNumber, InputActuatorSerial) {};
        DummyActuator(CBC* InputCBC, int InputPortNumber, int InputActuatorSerial, DBStruct InputDB) : Actuator(InputCBC, InputPortNumber, InputActuatorSerial, InputDB) {};
        DummyActuator(CBC* InputCBC, int InputPortNumber, int InputActuatorSerial, DBStruct InputDB, ASFStruct InputASF) : Actuator(InputCBC, InputPortNumber, InputActuatorSerial, InputDB, InputASF) {};

        void Initialize();
        int Step(int InputSteps);
        float MeasureLength();

};

#endif
