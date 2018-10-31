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

/*!
 * The Actuator Class is responsible for the positioning of the Actuators. 
The class communicates with the online database to load and save positional information, 
as well as with the offline database to load and save calibration and configuration information.
 */

class Actuator 
{

public:

enum StatusModes {
Healthy=1,//Everything is good
OperableError=2,//Errors that the user should be aware of, but shouldn't interfere with normal operation of the actuator.
FatalError=3//Errors that the user should definitely be aware of and handle appropriately. The Actuator should not be able to move with a fatal error without first reconfiguring something.
};

struct ErrorInfo {
bool Triggered;
std::string ErrorDescription;
StatusModes ErrorType;
};

struct Position {
///@{
/*! @name Encoder Position
 */
/*! @brief Revolution count of encoder. */
int Revolution;
/*! @brief Angle count of encoder. */
int Angle;
///@}
};

struct DBStruct {
///@{
/*! @name Hardware Database Information
 */
/*! @brief IP of Database. */
std::string ip;
/*! @brief User of Database. */
std::string user;
/*! @brief Password of Database. */
std::string password;
/*! @brief Name of Sub-Database. */
std::string dbname;
/*! @brief Port Number of Database. */
std::string port;
///@}
};

struct ASFStruct {
///@{
/*! @name Actuator Status File Information
 */
/*! @brief Directory of Status File */
std::string Directory;
std::string FilenamePrefix;
std::string FilenameSuffix;
///@}
};


Actuator::ASFStruct DefaultASFInfo {"/.ASF/",".ASF_",".log"};

Actuator::ASFStruct EmergencyASFInfo {"/.ASF/",".ASF_Emergency_Port_",".log"};

struct RecordedPositionStruct {
int Revolution;
int Angle;
int Year;
int Month;
int Day;
int Hour;
int Minute;
int Second;
};

StatusModes ErrorStatus {OperableError};

std::string ASFFullPath;
CBC::ADC::adcData adcdata; /*!< typedef adcData instantiation. Struct that is the output when measuring encoder voltage. */
CBC* cbc;
RecordedPositionStruct RecordedPosition;
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
//int RevolutionRange{100};
int RetractRevolutionLimit{100};
int ExtendRevolutionLimit{0};
Position RetractStop{103,32};
Position ExtendStop{-3,89};
Position CurrentPosition{50,0};
int RecordingInterval{(StepsPerRevolution/2)-20};
float VMin{0.593};
float VMax{3.06};
float dV{(VMax-VMin)/(StepsPerRevolution-1)};
int HysteresisSteps{RecordingInterval-10};
float StdDevRemeasure{dV/2.0f};
int MaxVoltageMeasurementAttempts{20};
float StdDevMax{5.0f*dV};
int QuickAngleCheckRange{5};
int EndstopSearchStepsize{StepsPerRevolution/10};
int CyclesDefiningHome{3};
int MinimumMissedStepsToFlagError{5};
float TolerablePercentOfMissedSteps{0.1};
int ExtendStopToHomeStepsDeviation{StepsPerRevolution/4};
int FlaggedRecoverySteps{RecordingInterval/10};
int MaxRecoverySteps{RecordingInterval/2};
int EndStopRecoverySteps{StepsPerRevolution/4};


std::vector<ErrorInfo> ActuatorErrors{
{false, "Home Position is not calibrated", FatalError},//error 0
{false, "DBFlagNotSet", OperableError},//error 1
{false, "MySQL Communication Error", OperableError},//error 2
{false, "DB Columns does not match what is expected", FatalError},//error 3
{false, "ASF File is Bad", FatalError},//error 4
{false, "ASF File entries does not match what is expected", FatalError},//error 5
{false, "ASF and DB have mismatching data", OperableError},//error 6
{false, "Voltage Std Dev is entirely too high", FatalError},//error 7
{false, "Actuator Missed too many steps", FatalError},//error 8
{false, "Actuator position is too many steps away to recover safely", FatalError},//error 9
{false, "Actuator position is recovering large amount of steps, should be ok", OperableError},//error 10
{false, "Extend Stop Voltage is too close to the discontinuity. Possible 1 cycle uncertainty with calibrated data", OperableError},//error 11
{false, "End stop is large number of steps away from what is expected. Possible uncertainty in home position", OperableError},//error 12
{false, "Discrepancy between number of steps from extend stop and recorded number of steps from end stop is too high. Possible uncertainty in probed home position", OperableError}//error 13
};

int NumberOfIntsInASFHeader{8};//yr,mo,day,hr,min,sec,rev,angle
int NumberOfErrorCodes{(int) ActuatorErrors.size()};//change this to size of Error Struct?
//const static int NumberOfErrorCodes{ActuatorErrors.size()};//change this to size of Error Struct?
int NumberOfIntsInASF{NumberOfIntsInASFHeader+NumberOfErrorCodes};
std::vector<float> EncoderCalibration;//[StepsPerRevolution]
std::vector<bool> RecordedErrorCode;//[NumberOfErrorCodes]

//do checks on constructors to make sure numbers are okay. e.g. hysteresis steps are positive, recording interval is less than half of stepsperrevolution.


////////////This needs further thought. Each error should have an attached status affector (operable/fatal), such that when an error is cleared, the other errors can be checked to see what the resolved status is. Also important during initialization, since the status must be set depending on the error codes.

//what I need is a mapping. When I execute "SetErrorCode(ErrorCodes CodeNumber)" (e.g. SetErrorCode(VoltageTooHigh)), ErrorCodes[CodeNumber] should flip to true (e.g. VoltageTooHigh maps to the int 7, ErrorCodes[7] is set to true). But there should also be a built-in mapping where I can check for the associated StatusMode for that Error (e.g. VoltageTooHigh is a FatalError).

void ReadConfigurationAndCalibration();
//void SaveConfigurationAndCalibration();
void ReadStatusFromDB();
void RecordStatusToDB();
void ReadStatusFromASF();
void RecordStatusToASF();
float MeasureVoltage();
int MeasureAngle();
int QuickAngleCheck(Position ExpectedPosition);
int SlowAngleCheck(Position ExpectedPosition);
virtual int Step(int InputSteps);
Position PredictPosition(Position InputPosition, int InputSteps);
int HysteresisMotion(int InputSteps);
virtual void Initialize();
void SetCurrentPosition(Position InputPosition);
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
int CalculateStepsFromHome(Position InputPosition);
void SetError(int CodeNumber);
void UnsetError(int CodeNumber);
void SetStatus(StatusModes InputStatus);
void CheckErrorStatus();
bool Diagnostic();
void SetPosition(int Revolution, int Angle);
void ProbeHome();
void FindHomeFromEndStop(int Direction);
void FindHomeFromExtendStop();
void FindHomeFromRetractStop();
void ProbeEndStop(int Direction);
void ProbeExtendStop();
void ProbeRetractStop();
int GetPortNumber() const;
StatusModes GetStatus();
void ReadStatusFromDBAndASF();
void CreateDefaultASF();
//void LoadDefaults();
virtual float MeasureLength();
float MoveToLength(float TargetLength);
float MoveDeltaLength(float LengthToMove);
void ClearAllErrors();
void ReadStatus();
void Record();
void ForceRecover();
///////////////////////////////////////////////////////////////

/*
Actuator::ErrorInfo ActuatorErrors[NumberOfErrorCodes]={
{false, "DBFlagNotSet", OperableError},//error 0
{false, "MySQL Communication Error", OperableError},//error 1
{false, "DB Columns does not match what is expected", OperableError},//error 2
{false, "ASF File is Bad / DNE", OperableError},//error 3
{false, "ASF File entries does not match what is expected", FatalError},//error 4
{false, "ASF File is Bad / Cannot be written to.", FatalError},//error 5
{false, "Voltage Std Dev is entirely too high", FatalError},//error 6
{false, "Voltage Std Dev is a little high", OperableError},//error 7
{false, "Actuator Missed too many steps", FatalError},//error 8
{false, "Actuator position is too many steps away to recover safely", FatalError},//error 9
{false, "Actuator position is recovering large amount of steps, should be ok", OperableError},//error 10
{false, "Extend Stop Voltage is too close to the discontinuity. Possible 1 cycle uncertainty with calibrated data", OperableError},//error 11
....
{false, "End stop is large number of steps away from what is expected. Possible uncertainty in home position", OperableError},//error 12
{false, "Discrepancy between number of steps from extend stop and recorded number of steps from end stop is too high. Possible uncertainty in probed home position", OperableError},//error 13
{false, "ASF and DB have mismatching data", OperableError};//error 14
};
*/

 /*!
 *
 * Default constructor, used ideally only during calibration of Actuator. Allows for calibration functions to be performed, as well as (inaccurate) motion of Actuator. 
 *
 * @param cbc 			Instance of CBC class being used is passed to Actuator Class
 */
Actuator();

/*!
 *
 * Constructor used for communication with database, but not the local text file.
 *
 * @param cbc    		Instance of CBC class being used is passed to Actuator Class
 * @param input_Actuator_ID     Unique ID of Actuator to be used
 * @param input_HDB_ID          DBstruct filetype of Hardware Database (offline database)
 * @param input_SDB_ID          DBstruct filetype of Software Database (online database)
*/
Actuator(CBC* InputCBC, int InputPortNumber);
Actuator(CBC* InputCBC, int InputPortNumber, int InputActuatorSerial);
Actuator(CBC* InputCBC, int InputPortNumber, int InputActuatorSerial, DBStruct InputDB);
Actuator(CBC* InputCBC, int InputPortNumber, int InputActuatorSerial, DBStruct InputDB, ASFStruct InputASF);

/*!
 *
 * Constructor used for communication with database AND local text file.
 *
 * @param cbc		        Instance of CBC class being used is passed to Actuator Class
 * @param input_Actuator_ID	Unique ID of Actuator to be used
 * @param input_HDB_ID		DBstruct filetype of Hardware Database (offline database)
 * @param input_ASF_ID		ASFstruct filetype of local Actuator Status File
 */

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
