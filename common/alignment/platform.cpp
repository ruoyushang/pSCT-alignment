#ifdef DEBUG
#define DEBUG_MSG(str) do {std::cout << str << std::endl;} while (false)
#else
#define DEBUG_MSG(str) do {} while (false)
#endif

#define ERROR_MSG(str) do {std::cout << str << std::endl;} while (false)

/*
 * platform.cpp Platform Control
 */
#include "platform.hpp"
#include <sstream>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <iostream>
#include <algorithm>

using namespace std;

//Destructor
Platform::~Platform() 
{
/*
for (int i=0; i<6; i++)
{
delete actuator[i];
}
*/
for (auto& dev : m_vMPES)
    delete dev;
m_vMPES.clear();

cbc.driver.disableAll();
}

Platform::Platform()
{
DEBUG_MSG("Creating Emergency Platform Object.");
//CBC cbc;
for (int i=0; i<6; i++)
{
//actuator[i]=new Actuator(&cbc, i+1);
actuator[i]=Actuator(&cbc, i+1);
}
Initialize();
}

Platform::Platform(std::array<int, 6> ActuatorPorts, std::array<int, 6> ActuatorSerials)
{
DEBUG_MSG("Creating Platform Object with Actuator Serial Numbers and Ports");
//CBC cbc;
for (int i=0; i<6; i++)
{
//actuator[i]=new Actuator(&cbc, ActuatorPorts[i], ActuatorSerials[i]);
actuator[i]=Actuator(&cbc, ActuatorPorts[i], ActuatorSerials[i]);
}
Initialize();
}

Platform::Platform(int CBCSerial, std::array<int, 6> ActuatorPorts, std::array<int, 6> ActuatorSerials, Actuator::DBStruct InputDBInfo)
{
DEBUG_MSG("Creating Platform Object with Actuator Serial Numbers, Ports, and DB Information");
//CBC cbc;
for (int i=0; i<6; i++)
{
//actuator[i]=new Actuator(&cbc, ActuatorPorts[i], ActuatorSerials[i], InputDBInfo);
actuator[i]=Actuator(&cbc, ActuatorPorts[i], ActuatorSerials[i], InputDBInfo);
}
SetCBCSerialNumber(CBCSerial);
SetDB(InputDBInfo);
Initialize();
}

Platform::Platform(int CBCSerial, std::array<int, 6> ActuatorPorts, std::array<int, 6> ActuatorSerials, Actuator::DBStruct InputDBInfo, Actuator::ASFStruct InputASFInfo)
{
DEBUG_MSG("Creating Platform Object with Actuator Serial Numbers, Ports, DB Information and custom ASF paths");
//CBC cbc;
for (int i=0; i<6; i++)
{
//actuator[i]=new Actuator(&cbc, ActuatorPorts[i], ActuatorSerials[i], InputDBInfo, InputASFInfo);
actuator[i]=Actuator(&cbc, ActuatorPorts[i], ActuatorSerials[i], InputDBInfo, InputASFInfo);
}
SetCBCSerialNumber(CBCSerial);
SetDB(InputDBInfo);
Initialize();
}


void Platform::LoadCBCParameters()
{
if (CBCSerialNumber==0)
{
DEBUG_MSG("Not loading CBC Parameters since no CBC Serial given");
return;
}
DEBUG_MSG("Loading CBC Parameters for Controller Board " << CBCSerialNumber);
if (DBFlag==false)
{
ERROR_MSG("Operable Error: Database Flag is not set!");
SetError(13);
return;
}
float CalibrationTemperature;
std::array<float, 6> EncoderVoltageSlope;
std::array<float, 6> EncoderVoltageOffset;
std::array<float, 6> EncoderTemperatureSlope;
std::array<float, 6> EncoderTemperatureOffset;
 try
{
  sql::Driver *driver;
  sql::Connection *con;
  sql::Statement *stmt;
  sql::ResultSet *res;
  driver = get_driver_instance();
  std::string hoststring="tcp://"+DBInfo.ip+":"+DBInfo.port;
  con = driver->connect(hoststring,DBInfo.user,DBInfo.password);
  con->setSchema(DBInfo.dbname);
  stmt = con->createStatement();

stringstream stmtvar;
stmtvar << "SELECT * FROM Opt_ControllerBoardCalibration WHERE serial_number=" << CBCSerialNumber << " ORDER BY start_date DESC LIMIT 1";
stmt->execute(stmtvar.str());
res = stmt->getResultSet();
while (res->next())
{
InternalTemperatureSlope=res->getDouble(5);
InternalTemperatureOffset=res->getDouble(6);
ExternalTemperatureSlope=res->getDouble(7);
ExternalTemperatureOffset=res->getDouble(8);
CalibrationTemperature=res->getDouble(9);
EncoderVoltageSlope[0]=res->getDouble(10);
EncoderVoltageOffset[0]=res->getDouble(11);
EncoderTemperatureSlope[0]=res->getDouble(12);
EncoderTemperatureOffset[0]=res->getDouble(13);
EncoderVoltageSlope[1]=res->getDouble(14);
EncoderVoltageOffset[1]=res->getDouble(15);
EncoderTemperatureSlope[1]=res->getDouble(16);
EncoderTemperatureOffset[1]=res->getDouble(17);
EncoderVoltageSlope[2]=res->getDouble(18);
EncoderVoltageOffset[2]=res->getDouble(19);
EncoderTemperatureSlope[2]=res->getDouble(20);
EncoderTemperatureOffset[2]=res->getDouble(21);
EncoderVoltageSlope[3]=res->getDouble(22);
EncoderVoltageOffset[3]=res->getDouble(23);
EncoderTemperatureSlope[3]=res->getDouble(24);
EncoderTemperatureOffset[3]=res->getDouble(25);
EncoderVoltageSlope[4]=res->getDouble(26);
EncoderVoltageOffset[4]=res->getDouble(27);
EncoderTemperatureSlope[4]=res->getDouble(28);
EncoderTemperatureOffset[4]=res->getDouble(29);
EncoderVoltageSlope[5]=res->getDouble(30);
EncoderVoltageOffset[5]=res->getDouble(31);
EncoderTemperatureSlope[5]=res->getDouble(32);
EncoderTemperatureOffset[5]=res->getDouble(33);
SynchronousRectification=res->getInt(34);
HighCurrent=res->getInt(35);
}

  delete res;
  delete stmt;
  delete con;

}
  catch (sql::SQLException &e) 
{
  cout << "# ERR: SQLException in " << __FILE__;
  cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
  cout << "# ERR: " << e.what();
  cout << " (MySQL error code: " << e.getErrorCode();
  cout << ", SQLState: " << e.getSQLState() << " )" << endl;
ERROR_MSG("Operable Error: SQL Exception caught for platform. Did not successfully communicate with database.");
SetError(14);
return;
}
cbc.adc.setEncoderTemperatureRef(CalibrationTemperature);
for (int i=0; i<6; i++)
{
cbc.adc.setEncoderTemperatureSlope(i+1, EncoderTemperatureSlope[i]);
cbc.adc.setEncoderTemperatureOffset(i+1, EncoderTemperatureOffset[i]);
cbc.adc.setEncoderVoltageSlope(i+1, EncoderVoltageSlope[i]);
cbc.adc.setEncoderVoltageOffset(i+1, EncoderVoltageOffset[i]);
}
if(HighCurrent)
{
SetHighCurrent();
}
else
{
UnsetHighCurrent();
}
if(SynchronousRectification)
{
SetSynchronousRectification();
}
else
{
UnsetSynchronousRectification();
}
return;
}

void Platform::SetDB(Actuator::DBStruct InputDBInfo)
{
DBInfo=InputDBInfo;
DBFlag=true;
}

void Platform::UnsetDB()
{
DBFlag=false;
}

std::array<int, 6> Platform::Step(std::array<int,6> InputSteps)
{
    // mark this as busy
    m_State = PlatformState::Busy;

DEBUG_MSG("Stepping Platform steps: (" << InputSteps[0] << ", " << InputSteps[1] << ", " << InputSteps[2] << ", " << InputSteps[3] << ", " << InputSteps[4] << ", " << InputSteps[5] << ")");
if (DisallowMovement)
{
return InputSteps;
    // reset the state to Off
    m_State = PlatformState::Off;
}

//add check to make sure we dont step out of range.

std::array<int, 6> StepsRemaining;
std::array<int, 6> ActuatorIterations={0,0,0,0,0,0};
std::array<Actuator::Position, 6> FinalPosition;

for (int i=0; i<6; i++)
{
actuator[i].ReadStatusFromASF();
actuator[i].CheckCurrentPosition();
FinalPosition[i]=actuator[i].PredictPosition(actuator[i].CurrentPosition,-InputSteps[i]);//negative steps because positive step is extension of motor, negative steps increases counter since home is defined (0,0)
StepsRemaining[i]=-(actuator[i].CalculateStepsFromHome(FinalPosition[i])-actuator[i].CalculateStepsFromHome(actuator[i].CurrentPosition));

if (FinalPosition[i].Revolution < actuator[i].ExtendRevolutionLimit || FinalPosition[i].Revolution >= actuator[i].RetractRevolutionLimit)
{
ERROR_MSG("Operable Error: Attempting to move Actuator with Index " << i << " outside of software range.");
SetError(12);
ERROR_MSG("Attempting to move outside of Software Range(" << actuator[i].HomeLength-(actuator[i].RetractRevolutionLimit*actuator[i].StepsPerRevolution*actuator[i].mmPerStep) << "-" << actuator[i].HomeLength-(actuator[i].ExtendRevolutionLimit*actuator[i].StepsPerRevolution*actuator[i].mmPerStep) << "mm) for Actuator " << actuator[i].SerialNumber);
 
    // reset the state to Off
    m_State = PlatformState::Off;

return InputSteps;
}

if(StepsRemaining[i] != 0)
{
ActuatorIterations[i]=1+((std::abs(StepsRemaining[i])-1)/actuator[i].RecordingInterval);//simply integer division rounded up, hence the -1.
}
}

int IterationsRemaining=*std::max_element(ActuatorIterations.begin(), ActuatorIterations.end());
std::array<int, 6> StepsToTake={0,0,0,0,0,0};
int Sign;
int StepsMissed;

// m_State can be changed by an external caller during motion -- need to check this
cbc.driver.enableAll();
while(IterationsRemaining>1)
{
        // need to check the state and if needed, break TWICE --
        // once for the inner actuator loop and once for the outer cycle loop
        if (m_State == PlatformState::Off || m_State == PlatformState::Error)
            break;

for (int i=0; i<6; i++)
{
            if (m_State == PlatformState::Off || m_State == PlatformState::Error)
                break;


if(ActuatorIterations[i] > 1)
{
if (DisallowMovement)
{
cbc.driver.disableAll();
RecordActuatorsToDB();

    // reset the state to Off
    m_State = PlatformState::Off;

return StepsRemaining;
}
StepsToTake[i]=StepsToTake[i]+StepsRemaining[i]/IterationsRemaining;
if(std::abs(StepsToTake[i])>actuator[i].RecordingInterval)
{
Sign=(StepsToTake[i]>0)-(StepsToTake[i]<0);
StepsMissed=actuator[i].Step(Sign*actuator[i].RecordingInterval);
CheckActuatorErrorStatus(i);
StepsToTake[i]=StepsToTake[i]-(Sign*actuator[i].RecordingInterval)+StepsMissed;
StepsRemaining[i]=-(actuator[i].CalculateStepsFromHome(FinalPosition[i])-actuator[i].CalculateStepsFromHome(actuator[i].CurrentPosition));
ActuatorIterations[i]=1+((std::abs(StepsRemaining[i])-1)/actuator[i].RecordingInterval);
DEBUG_MSG("Steps Remaining for Actuator Index " << i << ": " << StepsRemaining[i]);
}
}
}
RecordActuatorsToDB();
IterationsRemaining=*std::max_element(ActuatorIterations.begin(), ActuatorIterations.end());
}

//Hysteresis Motion
if (m_State != PlatformState::Off && m_State != PlatformState::Error) {
for (int i=0; i<6; i++)
{
if (DisallowMovement)
{
cbc.driver.disableAll();
RecordActuatorsToDB();

    // reset the state to Off
    m_State = PlatformState::Off;

return StepsRemaining;
}
if(StepsRemaining[i] != 0)
{
actuator[i].HysteresisMotion(StepsRemaining[i]);
CheckActuatorErrorStatus(i);
StepsRemaining[i]=-(actuator[i].CalculateStepsFromHome(FinalPosition[i])-actuator[i].CalculateStepsFromHome(actuator[i].CurrentPosition));
}

}
}

cbc.driver.disableAll();
RecordActuatorsToDB();

    // reset the state to On
    m_State = PlatformState::On;

return StepsRemaining;
}

void Platform::CheckActuatorErrorStatus(int ActuatorIndex)
{
Actuator::StatusModes ErrorStatus=actuator[ActuatorIndex].ErrorStatus;
if(ErrorStatus == Actuator::FatalError)
{
SetError((2*ActuatorIndex)+1);
}
else if(ErrorStatus == Actuator::OperableError)
{
SetError(2*ActuatorIndex);
}
return;
}

void Platform::CheckErrorStatus()//cycle through all errors and set status based on ones triggered.
{
ErrorStatus=Actuator::Healthy;
for (int i=0; i<6; i++)
{
CheckActuatorErrorStatus(i);
}
for(int i=12; i<NumberOfErrorCodes; i++)
{
if(PlatformErrors[i].Triggered==true)
{
SetStatus(PlatformErrors[i].ErrorType);
}
}
return;
}

void Platform::SetStatus(Actuator::StatusModes InputStatus)//private
{

if(InputStatus==Actuator::FatalError)
{
DEBUG_MSG("Disallowing Movement of Platform!");
DisallowMovement=true;
}

if (ErrorStatus == Actuator::FatalError)
{
return;
}
else
{
ErrorStatus = InputStatus;
}
return;
}

void Platform::SetError(int CodeNumber)//private
{
DEBUG_MSG("Setting Error " << CodeNumber << " (" << PlatformErrors[CodeNumber].ErrorDescription << ") for Platform");
PlatformErrors[CodeNumber].Triggered=true;
SetStatus(PlatformErrors[CodeNumber].ErrorType);
return;
}

void Platform::UnsetError(int CodeNumber)
{
DEBUG_MSG("Unsetting Error " << CodeNumber << " (" << PlatformErrors[CodeNumber].ErrorDescription << ") for Platform");
PlatformErrors[CodeNumber].Triggered=false;
CheckErrorStatus();
return;
}


void Platform::ProbeEndStopAll(int Direction)//pass +1 for extension stop, -1 for retraction stop. private 
{
if (DisallowMovement)
{
return;
}
std::array<int,6> SearchSteps;
std::array<float,6> VoltageBefore;
std::array<float,6> AbsDeltaVoltage;
std::array<float,6> VoltageAfter;

for (int i=0; i<6; i++)
{
SearchSteps[i]=Direction*actuator[i].EndstopSearchStepsize;
VoltageAfter[i]=actuator[i].MeasureVoltage();
//CheckActuatorErrorStatus(i);
if (DisallowMovement)
{
return;
}
}
float StoppedSteppingFactor=0.5;
std::array<bool,6> NotReachedStop={true,true,true,true,true,true};
cbc.driver.enableAll();

while( NotReachedStop[0] || NotReachedStop[1] || NotReachedStop[2] || NotReachedStop[3] || NotReachedStop[4] || NotReachedStop[5] )
{
for (int i=0; i<6; i++)
{
if (NotReachedStop[i])
{
VoltageBefore[i]=VoltageAfter[i];
cbc.driver.step(actuator[i].PortNumber, SearchSteps[i]);
VoltageAfter[i]=actuator[i].MeasureVoltage();
CheckActuatorErrorStatus(i);
if (DisallowMovement)
{
cbc.driver.disableAll();
return;
}
AbsDeltaVoltage[i]=std::fabs(VoltageAfter[i]-VoltageBefore[i]);
if (AbsDeltaVoltage[i]<std::fabs(actuator[i].dV*SearchSteps[i]*StoppedSteppingFactor))
{
NotReachedStop[i]=false;
}
}
}
}

cbc.driver.disableAll();
return;
}

void Platform::ProbeExtendStopAll()//public
{
DEBUG_MSG("Probing Extend Stop for All Actuators");
ProbeEndStopAll(1);
}

void Platform::ProbeRetractStopAll()//public
{
DEBUG_MSG("Probing Retract Stop for All Actuators");
ProbeEndStopAll(-1);
}

void Platform::FindHomeFromEndStopAll(int Direction)//private
{
if(Direction == 1)
{
ProbeExtendStopAll();
}
else if (Direction == -1)
{
ProbeRetractStopAll();
}
else
{
return;
}

CheckErrorStatus();
if (DisallowMovement)
{
return;
}

cbc.driver.enableAll();

for (int i=0; i<6; i++)
{
if(Direction == 1)
{
actuator[i].FindHomeFromExtendStop();
}
else if(Direction == -1)
{
actuator[i].FindHomeFromRetractStop();
}
else
{
return;
}
CheckActuatorErrorStatus(i);
if (DisallowMovement)
{
cbc.driver.disableAll();
return;
}
}
cbc.driver.disableAll();
RecordActuatorsToDB();
return;
}



void Platform::FindHomeFromExtendStopAll()//public
{
DEBUG_MSG("Finding Home From Extend Stop for All Actuators");
FindHomeFromEndStopAll(1);
return;
}

void Platform::FindHomeFromRetractStopAll()//public
{
DEBUG_MSG("Finding Home From Retract Stop for All Actuators");
FindHomeFromEndStopAll(-1);
return;
}

void Platform::ProbeHomeAll()//public
{
DEBUG_MSG("Probing Home for All Actuators");
ProbeExtendStopAll();
if (DisallowMovement)
{
return;
}

cbc.driver.enableAll();
for (int i=0; i<6; i++)
{
actuator[i].ProbeHome();
CheckActuatorErrorStatus(i);
if (DisallowMovement)
{
cbc.driver.disableAll();
return;
}
}
cbc.driver.disableAll();
RecordActuatorsToDB();
return;
}


std::array<float,6> Platform::MeasureLengths()//public
{
DEBUG_MSG("Measuring Lengths of All Actuators");
std::array<float,6> CurrentLengths;
for (int i=0; i<6; i++)
{
CurrentLengths[i]=actuator[i].MeasureLength();
//CheckActuatorErrorStatus(i);
}
return CurrentLengths;
}

std::array<float, 6> Platform::MoveToLengths(std::array<float, 6> TargetLengths)//public
{
std::array<float, 6> CurrentLengths=MeasureLengths();
std::array<float, 6> LengthsToMove;
std::array<int, 6> StepsToTake;
for (int i=0; i<6; i++)
{
LengthsToMove[i]=TargetLengths[i]-CurrentLengths[i];
StepsToTake[i]=std::floor((LengthsToMove[i]/actuator[i].mmPerStep)+0.5);
}
std::array<int, 6> StepsRemaining=Step(StepsToTake);
CurrentLengths=MeasureLengths();
return CurrentLengths;
}

std::array<float, 6> Platform::MoveDeltaLengths(std::array<float, 6> LengthsToMove)//public
{
std::array<float, 6> CurrentLengths=MeasureLengths();
std::array<float, 6> TargetLengths;
for (int i=0; i<6; i++)
{
TargetLengths[i]=CurrentLengths[i]+LengthsToMove[i];
}
CurrentLengths=MoveToLengths(TargetLengths);
std::array<float, 6> LengthsRemaining;
for(int i=0; i<6; i++)
{
LengthsRemaining[i]=TargetLengths[i]-CurrentLengths[i];
}
return LengthsRemaining;
}

float Platform::ReadInternalTemperature()//public
{
DEBUG_MSG("Reading Internal Temperature");
CBC::ADC::adcData TemperatureADCData=cbc.adc.readTemperatureVolts();
return (InternalTemperatureSlope*TemperatureADCData.voltage)+InternalTemperatureOffset;
}

float Platform::ReadExternalTemperature()//public
{
DEBUG_MSG("Reading External Temperature");
CBC::ADC::adcData TemperatureADCData=cbc.adc.readExternalTemp();
return (ExternalTemperatureSlope*TemperatureADCData.voltage)+ExternalTemperatureOffset;
}

void Platform::RecordActuatorsToDB()
{
//DEBUG_MSG("Recording All Actuators To DB");
for (int i=0; i<6; i++)
{
//actuator[i].RecordStatusToDB();//uncomment this when db can be used locally.
}
}

void Platform::SetHighCurrent()//public
{
DEBUG_MSG("Setting High Current");
HighCurrent=true;
cbc.driver.enableHighCurrent();
}

void Platform::UnsetHighCurrent()//public
{
DEBUG_MSG("Unsetting High Current");
HighCurrent=false;
cbc.driver.disableHighCurrent();
}

void Platform::SetSynchronousRectification()//public
{
DEBUG_MSG("Setting SR Mode");
SynchronousRectification=true;
cbc.driver.enableSR();
}

void Platform::UnsetSynchronousRectification()//public
{
DEBUG_MSG("Unsetting SR Mode");
SynchronousRectification=false;
cbc.driver.disableSR();
}

void Platform::Initialize()//
{
DEBUG_MSG("Initializing Platform..");
LoadCBCParameters();
CheckErrorStatus();
//any asserts?
}

void Platform::RecoverActuatorStatusFromDB(int ActuatorIndex)
{
DEBUG_MSG("Recovering Actuator Status with Index " << ActuatorIndex << " from Database");
actuator[ActuatorIndex].ReadStatusFromDB();
actuator[ActuatorIndex].CheckCurrentPosition();
actuator[ActuatorIndex].RecordStatusToASF();
CheckActuatorErrorStatus(ActuatorIndex);
}

void Platform::RecoverActuatorStatusFromDBAndASF(int ActuatorIndex)
{
DEBUG_MSG("Recovering Actuator Status with Index " << ActuatorIndex << " from Database and ASF");
actuator[ActuatorIndex].ReadStatusFromDBAndASF();
actuator[ActuatorIndex].CheckCurrentPosition();
actuator[ActuatorIndex].RecordStatusToASF();
CheckActuatorErrorStatus(ActuatorIndex);
}

void Platform::SetCBCSerialNumber(int InputSerial)
{
CBCSerialNumber=InputSerial;
}

void Platform::ClearActuatorAllErrors(int ActuatorIndex)
{
actuator[ActuatorIndex].ClearAllErrors();
}

void Platform::ClearActuatorSingleError(int ActuatorIndex, int CodeNumber)
{
actuator[ActuatorIndex].UnsetError(CodeNumber);
}

void Platform::ClearPlatformAllErrors()
{
ErrorStatus=Actuator::Healthy;
for (int i=0; i<PlatformErrors.size(); i++)
{
PlatformErrors[i].Triggered=false;
}
CheckErrorStatus();
DisallowMovement=false;
}

void Platform::FindHomeFromEndStop(int Direction, int ActuatorIndex)
{
if(Direction==1)
{
actuator[ActuatorIndex].FindHomeFromExtendStop();
}
else if(Direction == -1)
{
actuator[ActuatorIndex].FindHomeFromRetractStop();
}
}

void Platform::FindHomeFromExtendStop(int ActuatorIndex)
{
FindHomeFromEndStop(1, ActuatorIndex);
}

void Platform::FindHomeFromRetractStop(int ActuatorIndex)
{
FindHomeFromEndStop(-1, ActuatorIndex);
}

void Platform::ProbeHome(int ActuatorIndex)
{
actuator[ActuatorIndex].ProbeHome();
}

Actuator* Platform::getActuatorAt(int internal_idx)
{
    try {
        return &actuator.at(internal_idx);
    }
    catch (out_of_range) {
        return nullptr;
    }
}

MPES* Platform::getMPESAt(int internal_idx)
{
    try {
        return m_vMPES.at(internal_idx);
    }
    catch (out_of_range) {
        return nullptr;
    }
}

bool Platform::addMPES(int USB, int serial)
{
    if (serial < 0)
        return false;

    cout << "Adding MPES "<< serial << " at USB " << USB << "..." << endl;

#ifndef SIMMODE
    MPES *newMPES = new MPES(&cbc, USB, serial);
#else
    MPES *newMPES = new DummyMPES(&cbc, USB, serial);
#endif
    if (!newMPES->Initialize()) {
        cout << "+++ Platform: Couldn't initialize MPES at USB " << USB << endl;
        delete newMPES;
        return false;
    }
    else
        m_vMPES.push_back(newMPES);

    return true;
}

// MPES functionality
const MPES::Position& Platform::ReadMPES(int internal_idx)
{
    m_vMPES.at(internal_idx)->MeasurePosition();
    return m_vMPES.at(internal_idx)->getPosition();
}
