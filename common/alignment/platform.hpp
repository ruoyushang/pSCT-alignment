#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _AMD64
    #include "dummycbc.hpp"
    #warning "Compiling for AMD64 -- setting CBC to DummyCBC"
    #define CBC DummyCBC
#else
    #include "cbc.hpp"
#endif

#include "actuator.hpp"
#include "mpesclass.hpp"
#include <vector>
#include <string>
#include <array>


// hack to be able to stop mid motion
/** 
 * @brief This is the platform class.
*/
enum class PlatformState {On = 0, Off = 1, Busy = 2, Error = 3};

class Platform
{
public:

Actuator::StatusModes ErrorStatus {Actuator::Healthy};

bool DisallowMovement{false};
CBC cbc;
std::array<Actuator,6> actuator;
bool DBFlag{false};
float InternalTemperatureSlope{100.0};
float InternalTemperatureOffset{-50.0};
float ExternalTemperatureSlope{44.444};
float ExternalTemperatureOffset{-61.111};
bool HighCurrent{false};
bool SynchronousRectification{true};
int CBCSerialNumber{0};
Actuator::DBStruct DBInfo;

std::vector<Actuator::ErrorInfo> PlatformErrors{
{false, "Actuator 0 Operable Error", Actuator::OperableError},
{false, "Actuator 0 Fatal Error", Actuator::FatalError},
{false, "Actuator 1 Operable Error", Actuator::OperableError},
{false, "Actuator 1 Fatal Error", Actuator::FatalError},
{false, "Actuator 2 Operable Error", Actuator::OperableError},
{false, "Actuator 2 Fatal Error", Actuator::FatalError},
{false, "Actuator 3 Operable Error", Actuator::OperableError},
{false, "Actuator 3 Fatal Error", Actuator::FatalError},
{false, "Actuator 4 Operable Error", Actuator::OperableError},
{false, "Actuator 4 Fatal Error", Actuator::FatalError},
{false, "Actuator 5 Operable Error", Actuator::OperableError},
{false, "Actuator 5 Fatal Error", Actuator::FatalError},
{false, "Attempt to move out of Software Range", Actuator::OperableError},//error 12
{false, "DBFlagNotSet", Actuator::OperableError},//error 13
{false, "MySQL Communication Error", Actuator::OperableError}//error 14
};

int NumberOfErrorCodes{(int) PlatformErrors.size()};

Platform();
~Platform();
Platform(std::array<int,6> ActuatorPorts, std::array<int,6> ActuatorSerials);
Platform(int CBCSerial, std::array<int,6> ActuatorPorts, std::array<int,6> ActuatorSerials, Actuator::DBStruct InputDBInfo);
Platform(int CBCSerial, std::array<int,6> ActuatorPorts, std::array<int,6> ActuatorSerials, Actuator::DBStruct InputDBInfo, Actuator::ASFStruct InputASFInfo);

void LoadCBCParameters();
void SetDB(Actuator::DBStruct InputDBInfo);
void UnsetDB();

float ReadInternalTemperature();
float ReadExternalTemperature();

std::array<int,6> Step(std::array<int,6> InputSteps);
//std::array<int,6> HysteresisMotion(std::array<int,6> StepsRemaining);
void CheckActuatorErrorStatus(int ActuatorIndex);
void CheckStatus();
void ProbeEndStopAll(int Direction);
void ProbeExtendStopAll();
void ProbeRetractStopAll();
void FindHomeFromEndStopAll(int Direction);
void FindHomeFromExtendStopAll();
void FindHomeFromRetractStopAll();
void ProbeHomeAll();
std::array<float,6> MeasureLengths();
std::array<float,6> MoveToLengths(std::array<float,6> TargetLengths);
std::array<float,6> MoveDeltaLengths(std::array<float,6> LengthsToMove);
void SetError(int CodeNumber);
void UnsetError(int CodeNumber);
void SetStatus(Actuator::StatusModes InputStatus);
void Record();
void SetHighCurrent();
void UnsetHighCurrent();
void SetSynchronousRectification();
void UnsetSynchronousRectification();
void Initialize();
void RecoverActuatorStatusFromDB(int ActuatorIndex);
void RecoverActuatorStatusFromDBAndASF(int ActuatorIndex);
void SetCBCSerialNumber(int InputSerial);
void ClearActuatorAllErrors(int ActuatorIndex);
void ClearActuatorSingleError(int ActuatorIndex, int CodeNumber);
void ClearPlatformAllErrors();
void ClearSinglePlatformError(int CodeNumber);
void RecordActuatorsToDB();
void CheckErrorStatus();
void FindHomeFromEndStop(int Direction, int ActuatorIndex);
void FindHomeFromExtendStop(int ActuatorIndex);
void FindHomeFromRetractStop(int ActuatorIndex);
void ProbeHome(int ActuatorIndex);

// additional interface needs
PlatformState getState() const { return m_State; };
void setState(PlatformState state) { m_State = state; };
int getActuatorCount() const {return actuator.size(); }
Actuator* getActuatorAt(int internal_idx);

// MPES functionality
int getMPESCount() {return m_vMPES.size(); }
MPES* getMPESAt(int internal_idx);


/** 
 * @brief This function adds MPES if they can be initialized at USB ports.
    In the Sim mode, MPES are added regardlessly.
*/
bool addMPES(int USB, int serial);

const MPES::Position& ReadMPES(int internal_idx);
private:
    std::vector<MPES *> m_vMPES;

    // internal state
    PlatformState m_State;
};
#endif
