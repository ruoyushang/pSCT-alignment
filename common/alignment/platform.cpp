#ifdef DEBUG
#define DEBUG_MSG(str) do {std::cout << str << std::endl;} while (false)
#else
#define DEBUG_MSG(str) do {} while (false)
#endif

#define ERROR_MSG(str) do {std::cout << str << std::endl;} while (true)

#include "common/alignment/platform.hpp"

#include <sstream>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <iostream>
#include <algorithm>

// Hardcoded
const int Platform::NUM_ACTS_PER_PLATFORM = 6;

const int Platform::NUM_ERROR_TYPES = 15;
const std::array<ErrorDefinition, NUM_ERROR_TYPES> Platform::ERROR_DEFINITIONS = {
    {"Actuator 0 Operable Error", DeviceState::OperableError},
    {"Actuator 0 Fatal Error", DeviceState::FatalError},
    {"Actuator 1 Operable Error", DeviceState::OperableError},
    {"Actuator 1 Fatal Error", DeviceState::FatalError},
    {"Actuator 2 Operable Error", DeviceState::OperableError},
    {"Actuator 2 Fatal Error", DeviceState::FatalError},
    {"Actuator 3 Operable Error", DeviceState::OperableError},
    {"Actuator 3 Fatal Error", DeviceState::FatalError},
    {"Actuator 4 Operable Error", DeviceState::OperableError},
    {"Actuator 4 Fatal Error", DeviceState::FatalError},
    {"Actuator 5 Operable Error", DeviceState::OperableError},
    {"Actuator 5 Fatal Error", DeviceState::FatalError},
    {"Attempt to move out of Software Range", DeviceState::OperableError},
    {"DBInfo not set", DeviceState::OperableError},
    {"MySQL Communication Error", DeviceState::OperableError}
};

// Default calibration constants
static const float Platform::DEFAULT_INTERNAL_TEMPERATURE_SLOPE = 100.0;
static const float Platform::DEFAULT_INTERNAL_TEMPERATURE_OFFSET = -50.0;
static const float Platform::DEFAULT_EXTERNAL_TEMPERATURE_SLOPE = 44.444;
static const float Platform::DEFAULT_EXTERNAL_TEMPERATURE_OFFSET = -61.111;

Platform::Platform()
{
    DEBUG_MSG("Creating emergency Platform object.")
    m_pCBC = std::shared_ptr<CBC>(new CBC());
    for (int i = 0; i < NUM_ACTS_PER_PLATFORM; i++)
    {
        #ifndef SIMMODE
        actuator[i] = std::unique_ptr<Actuator>(new Actuator(&m_pCBC, i+1));
        #else
        actuator[i] = std::unique_ptr<Actuator>(new DummyActuator(&m_pCBC, i+1));
        #endif
    }
    initialize();
}

Platform::Platform(std::array<int, NUM_ACTS_PER_PLATFORM> actuatorPorts, std::array<int, NUM_ACTS_PER_PLATFORM> actuatorSerials)
{
    DEBUG_MSG("Creating Platform Object with Actuator Serial Numbers and Ports");
    m_pCBC = std::shared_ptr<CBC>(new CBC());
    for (int i = 0; i < NUM_ACTS_PER_PLATFORM; i++)
    {
        #ifndef SIMMODE
        actuator[i] = std::unique_ptr<Actuator>(new Actuator(&m_pCBC, actuatorPorts[i], actuatorSerials[i]));
        #else
        actuator[i] = std::unique_ptr<Actuator>(new DummyActuator(&m_pCBC, actuatorPorts[i], actuatorSerials[i]));
        #endif
    }
    initialize();
}

Platform::Platform(int CBCSerial, std::array<int, NUM_ACTS_PER_PLATFORM> actuatorPorts, std::array<int, NUM_ACTS_PER_PLATFORM> actuatorSerials, Platform::DBStruct dbInfo)
{
    DEBUG_MSG("Creating Platform Object with Actuator Serial Numbers, Ports, and DB Information");
    m_pCBC = std::shared_ptr<CBC>(new CBC());
    for (int i = 0; i < NUM_ACTS_PER_PLATFORM; i++)
    {
        #ifndef SIMMODE
        actuator[i]=new Actuator(&m_pCBC, actuatorPorts[i], actuatorSerials[i], dbInfo);
        #else
        actuator[i]=new DummyActuator(&m_pCBC, actuatorPorts[i], actuatorSerials[i], dbInfo);
        #endif
    }
    setCBCSerialNumber(CBCSerial);
    setDbInfo(dbInfo);
    initialize();
}

Platform::Platform(int CBCSerial, std::array<int, NUM_ACTS_PER_PLATFORM> actuatorPorts, std::array<int, NUM_ACTS_PER_PLATFORM> actuatorSerials, Platform::DBStruct dbInfo, Platform::ASFStruct asfInfo)
{
    DEBUG_MSG("Creating Platform Object with Actuator Serial Numbers, Ports, DB Information and custom ASF paths");
    m_pCBC = std::shared_ptr<CBC>(new CBC());
    for (int i = 0 ; i < NUM_ACTS_PER_PLATFORM; i++)
    {
        #ifndef SIMMODE
        actuator[i]=new Actuator(&m_pCBC, actuatorPorts[i], actuatorSerials[i], dbInfo, asfInfo);
        #else
        actuator[i]=new DummyActuator(&m_pCBC, actuatorPorts[i], ActuatorSerials[i], dbInfo, asfInfo);
        #endif
    }
    setCBCSerialNumber(CBCSerial);
    setDbInfo(dbInfo);
    initialize();
}

Platform::~Platform()
{
    m_pCBC->driver.disableAll();
}

bool Platform::loadCBCcalibrationParams()
{
    if (CBCSerialNumber==0)
    {
        DEBUG_MSG("Failed to load CBC Parameters from DB since no CBC Serial given");
        return false;
    }
    DEBUG_MSG("Loading CBC Parameters for Controller Board " << CBCSerialNumber);
    if (m_DBInfo.empty())
    {
        ERROR_MSG("Operable Error: Failed to load CBC parameters from DB because no DB info provided.");
        setError(13);
        return false;
    }
    float CalibrationTemperature;
    std::array<float, NUM_ACTS_PER_PLATFORM> EncoderVoltageSlope;
    std::array<float, NUM_ACTS_PER_PLATFORM> EncoderVoltageOffset;
    std::array<float, NUM_ACTS_PER_PLATFORM> EncoderTemperatureSlope;
    std::array<float, NUM_ACTS_PER_PLATFORM> EncoderTemperatureOffset;
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
            m_HighCurrent=res->getInt(35);
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
    m_pCBC->adc.setEncoderTemperatureRef(CalibrationTemperature);
    for (int i=0; i<NUM_ACTS_PER_PLATFORM; i++)
    {
        m_pCBC->adc.setEncoderTemperatureSlope(i+1, EncoderTemperatureSlope[i]);
        m_pCBC->adc.setEncoderTemperatureOffset(i+1, EncoderTemperatureOffset[i]);
        m_pCBC->adc.setEncoderVoltageSlope(i+1, EncoderVoltageSlope[i]);
        m_pCBC->adc.setEncoderVoltageOffset(i+1, EncoderVoltageOffset[i]);
    }
    if(m_HighCurrent)
    {
        enableHighCurrent();
    }
    else
    {
        disableHighCurrent();
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

void Platform::setDBInfo(DBStruct DBInfo) {
    m_DBInfo = DBInfo;
    if (m_Errors[13]) {
        unsetError(13);
    }
}

void Platform::UnsetDbInfo()
{
    m_DBInfo = DBStruct();
}

std::array<int, NUM_ACTS_PER_PLATFORM> Platform::step(std::array<int,NUM_ACTS_PER_PLATFORM> inputSteps)
{
    m_State = DeviceState::Busy; // mark platform as busy

    DEBUG_MSG("Stepping Platform steps: (" << inputSteps[0] << ", " << inputSteps[1] << ", " << inputSteps[2] << ", " << inputSteps[3] << ", " << inputSteps[4] << ", " << inputSteps[5] << ")");
    if (m_State == DeviceState::FatalError)
    {
        m_State = DeviceState::Off; // Don't move, turn platform off
        return inputSteps;
    }

    std::array<int, NUM_ACTS_PER_PLATFORM> StepsRemaining;
    std::array<int, NUM_ACTS_PER_PLATFORM> ActuatorIterations={0,0,0,0,0,0};
    std::array<Actuator::PositionStruct, NUM_ACTS_PER_PLATFORM> FinalPosition;

    for (int i=0; i<NUM_ACTS_PER_PLATFORM; i++)
    {
        actuator[i]->LoadStatusFromASF();
        actuator[i]->CheckCurrentPosition();
        FinalPosition[i]=actuator[i]->PredictPosition(actuator[i]->CurrentPosition,-inputSteps[i]);//negative steps because positive step is extension of motor, negative steps increases counter since home is defined (0,0)
        StepsRemaining[i]=-(actuator[i]->CalculateStepsFromHome(FinalPosition[i])-actuator[i]->CalculateStepsFromHome(actuator[i]->CurrentPosition));

        if (FinalPosition[i].Revolution < actuator[i]->ExtendRevolutionLimit || FinalPosition[i].Revolution >= actuator[i]->RetractRevolutionLimit)
        {
            ERROR_MSG("Operable Error: Attempting to move Actuator with Index " << i << " outside of software range.");
            SetError(12);
            ERROR_MSG("Attempting to move outside of Software Range(" << actuator[i]->HomeLength-(actuator[i]->RetractRevolutionLimit*actuator[i]->StepsPerRevolution*actuator[i]->mmPerStep) << "-" << actuator[i]->HomeLength-(actuator[i]->ExtendRevolutionLimit*actuator[i]->StepsPerRevolution*actuator[i]->mmPerStep) << "mm) for Actuator " << actuator[i]->SerialNumber);

            // reset the state to Off
            m_State = PlatformState::Off;

            return inputSteps;
        }

        if(StepsRemaining[i] != 0)
        {
            ActuatorIterations[i]=1+((std::abs(StepsRemaining[i])-1)/actuator[i]->RecordingInterval);//simply integer division rounded up, hence the -1.
        }
    }

    int IterationsRemaining=*std::max_element(ActuatorIterations.begin(), ActuatorIterations.end());
    std::array<int, NUM_ACTS_PER_PLATFORM> StepsToTake={0,0,0,0,0,0};
    int Sign;
    int StepsMissed;

    // m_State can be changed by an external caller during motion -- need to check this
    m_pCBC->driver.enableAll();
    while(IterationsRemaining>1)
    {
        // need to check the state and if needed, break TWICE --
        // once for the inner actuator loop and once for the outer cycle loop
        if (m_State == PlatformState::Off || m_State == PlatformState::FatalError)
        break;

        for (int i=0; i<NUM_ACTS_PER_PLATFORM; i++)
        {
            if (m_State == PlatformState::Off || m_State == PlatformState::FatalError)
            break;


            if(ActuatorIterations[i] > 1)
            {
                if (status == DeviceStatus::FatalError)
                {
                    m_pCBC->driver.disableAll();

                    // reset the state to Off
                    m_State = PlatformState::Off;

                    return StepsRemaining;
                }
                StepsToTake[i]=StepsToTake[i]+StepsRemaining[i]/IterationsRemaining;
                if(std::abs(StepsToTake[i])>actuator[i]->RecordingInterval)
                {
                    Sign=(StepsToTake[i]>0)-(StepsToTake[i]<0);
                    StepsMissed=actuator[i]->Step(Sign*actuator[i]->RecordingInterval);
                    checkActuatorErrors(i);
                    StepsToTake[i]=StepsToTake[i]-(Sign*actuator[i]->RecordingInterval)+StepsMissed;
                    StepsRemaining[i]=-(actuator[i]->CalculateStepsFromHome(FinalPosition[i])-actuator[i]->CalculateStepsFromHome(actuator[i]->CurrentPosition));
                    ActuatorIterations[i]=1+((std::abs(StepsRemaining[i])-1)/actuator[i]->RecordingInterval);
                    DEBUG_MSG("Steps Remaining for Actuator Index " << i << ": " << StepsRemaining[i]);
                }
            }
        }
        IterationsRemaining=*std::max_element(ActuatorIterations.begin(), ActuatorIterations.end());
    }

    //Hysteresis Motion
    if (m_State != PlatformState::Off && m_State != PlatformState::FatalError) {
        for (int i=0; i<NUM_ACTS_PER_PLATFORM; i++)
        {
            if (status == DeviceStatus::FatalError)
            {
                m_pCBC->driver.disableAll();

                // reset the state to Off
                m_State = PlatformState::Off;

                return StepsRemaining;
            }
            if(StepsRemaining[i] != 0)
            {
                actuator[i]->HysteresisMotion(StepsRemaining[i]);
                checkActuatorErrors(i);
                StepsRemaining[i]=-(actuator[i]->CalculateStepsFromHome(FinalPosition[i])-actuator[i]->CalculateStepsFromHome(actuator[i]->CurrentPosition));
            }

        }
    }

    m_pCBC->driver.disableAll();

    // reset the state to On
    m_State = PlatformState::On;

    return StepsRemaining;
}

bool Platform::getActuatorStatus(int actuatorIdx, Actuator::StatusStruct & actuatorStatus)
{
    return m_Actuators.at(actuatorIdx)->readStatusFromASF(actuatorStatus);
}

void Platform::checkActuatorErrors(int actuatorIdx)
{
    DeviceState actuatorState = m_Actuators.at(actuatorIdx)->getState();
    if(actuatorState == DeviceState::FatalError)
    {
        setError((2*actuatorIdx)+1);
    }
    else if(actuatorState == DeviceState::OperableError)
    {
        setError(2*actuatorIdx);
    }
}

void Platform::updateErrorState()
{
    m_State = DeviceState::On;
    for (int i = 0; i < NUM_ACTS_PER_PLATFORM; i++)
    {
        checkActuatorErrors(i);
    }
}

void Platform::setState(DeviceState state)
{
    if (state == DeviceState::FatalError) {
        DEBUG_MSG("Fatal Error set! Disallowing Movement of Platform...");
    }
    if (state > m_State && m_State != DeviceState::Off && m_State != DeviceState::Busy) {
        m_State = state;
    }
}

void Platform::setError(int errorCode)
{
    DEBUG_MSG("Setting Error " << errorCode << " (" << Platform::ERROR_DEFINITIONS[errorCode].description << ") for Platform");
    m_Errors[errorCode] = true;
    setState(Platform::ERROR_DEFINITIONS[errorCode].severity);
}

void Platform::unsetError(int errorCode)
{
    DEBUG_MSG("Unsetting Error " << errorCode << " (" << Platform::ERROR_DEFINITIONS[errorCode].description << ") for Platform");
    m_Errors[errorCode] = false;
    updateErrorState();
}

void Platform::probeEndStopAll(int direction)
{
    if (status == DeviceStatus::FatalError)
    {
        return;
    }
    std::array<int,NUM_ACTS_PER_PLATFORM> SearchSteps;
    std::array<float,NUM_ACTS_PER_PLATFORM> VoltageBefore;
    std::array<float,NUM_ACTS_PER_PLATFORM> AbsDeltaVoltage;
    std::array<float,NUM_ACTS_PER_PLATFORM> VoltageAfter;

    for (int i=0; i<NUM_ACTS_PER_PLATFORM; i++)
    {
        SearchSteps[i]=Direction*actuator[i]->EndstopSearchStepsize;
        VoltageAfter[i]=actuator[i]->MeasureVoltage();
        //checkActuatorErrors(i);
        if (status == DeviceStatus::FatalError)
        {
            return;
        }
    }
    float StoppedSteppingFactor=0.5;
    std::array<bool,NUM_ACTS_PER_PLATFORM> NotReachedStop={true,true,true,true,true,true};
    m_pCBC->driver.enableAll();

    while( NotReachedStop[0] || NotReachedStop[1] || NotReachedStop[2] || NotReachedStop[3] || NotReachedStop[4] || NotReachedStop[5] )
    {
        for (int i=0; i<NUM_ACTS_PER_PLATFORM; i++)
        {
            if (NotReachedStop[i])
            {
                VoltageBefore[i]=VoltageAfter[i];
                m_pCBC->driver.step(actuator[i]->PortNumber, SearchSteps[i]);
                VoltageAfter[i]=actuator[i]->MeasureVoltage();
                checkActuatorErrors(i);
                if (status == DeviceStatus::FatalError)
                {
                    m_pCBC->driver.disableAll();
                    return;
                }
                AbsDeltaVoltage[i]=std::fabs(VoltageAfter[i]-VoltageBefore[i]);
                if (AbsDeltaVoltage[i]<std::fabs(actuator[i]->dV*SearchSteps[i]*StoppedSteppingFactor))
                {
                    NotReachedStop[i]=false;
                }
            }
        }
    }

    m_pCBC->driver.disableAll();
    return;
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

    updateErrorState();
    if (status == DeviceStatus::FatalError)
    {
        return;
    }

    m_pCBC->driver.enableAll();

    for (int i=0; i<NUM_ACTS_PER_PLATFORM; i++)
    {
        if(Direction == 1)
        {
            actuator[i]->FindHomeFromExtendStop();
        }
        else if(Direction == -1)
        {
            actuator[i]->FindHomeFromRetractStop();
        }
        else
        {
            return;
        }
        checkActuatorErrors(i);
        if (status == DeviceStatus::FatalError)
        {
            m_pCBC->driver.disableAll();
            return;
        }
    }
    m_pCBC->driver.disableAll();
    return;
}

bool Platform::probeHomeAll()
{
    DEBUG_MSG("Probing Home for All Actuators");
    probeExtendStopAll();
    if (m_State == DeviceState::FatalError)
    {
        ERROR_MSG("Platform:: Cannot probe home, Platform has a Fatal Error")
        return false;
    }

    m_pCBC->driver.enableAll();
    for (int i = 0; i < NUM_ACTS_PER_PLATFORM; i++)
    {
        m_Actuators.at(i)->probeHome();
        checkActuatorErrors(i);
        if (m_State == DeviceStatus::FatalError)
        {
            ERROR_MSG("Platform:: Actuator " << i << " encountered a fatal error after probing home. Stopping probeHomeAll...")
            m_pCBC->driver.disableAll();
            return false;
        }
    }
    m_pCBC->driver.disableAll();
    return true;
}


std::array<float, NUM_ACTS_PER_PLATFORM> Platform::measureLengths()//public
{
    DEBUG_MSG("Measuring Lengths of All Actuators");
    std::array<float, NUM_ACTS_PER_PLATFORM> currentLengths;
    for (int i = 0; i < NUM_ACTS_PER_PLATFORM; i++)
    {
        currentLengths[i] = m_Actuators.at(i)->measureLength();
    }
    return currentLengths;
}

std::array<float, NUM_ACTS_PER_PLATFORM> Platform::moveToLengths(std::array<float, NUM_ACTS_PER_PLATFORM> targetLengths)
{
    std::array<float, NUM_ACTS_PER_PLATFORM> currentLengths = measureLengths();
    std::array<float, NUM_ACTS_PER_PLATFORM> deltaLengths;
    std::array<int, NUM_ACTS_PER_PLATFORM> stepsToTake;

    for (int i = 0; i < NUM_ACTS_PER_PLATFORM; i++)
    {
        deltaLengths[i] = targetLengths[i] - currentLengths[i];
        stepsToTake[i] = std::floor((deltaLengths[i]/(m_Actuators.at(i)->mmPerStep)) + 0.5);
    }
    step(stepsToTake);
    currentLengths = measureLengths();
    return currentLengths;
}

std::array<float, NUM_ACTS_PER_PLATFORM> Platform::moveDeltaLengths(std::array<float, NUM_ACTS_PER_PLATFORM> deltaLengths)
{
    std::array<float, NUM_ACTS_PER_PLATFORM> currentLengths = measureLengths();
    std::array<float, NUM_ACTS_PER_PLATFORM> targetLengths;

    for (int i = 0; i < NUM_ACTS_PER_PLATFORM; i++)
    {
        targetLengths[i] = currentLengths[i] + deltaLengths[i];
    }

    currentLengths = moveToLengths(targetLengths);

    std::array<float, NUM_ACTS_PER_PLATFORM> distancesFromTargets;
    for (int i = 0; i < NUM_ACTS_PER_PLATFORM; i++)
    {
        distancesFromTargets[i] = targetLengths[i] - currentLengths[i];
    }
    return distancesFromTargets;
}

float Platform::getInternalTemperature()
{
    DEBUG_MSG("Reading Internal Temperature");
    CBC::ADC::adcData temperatureADCData = m_pCBC->adc.readTemperatureVolts();
    return (m_InternalTemperatureSlope * temperatureADCData.voltage) + m_InternalTemperatureOffset;
}

float Platform::getExternalTemperature()
{
    DEBUG_MSG("Reading External Temperature");
    CBC::ADC::adcData temperatureADCData = m_pCBC->adc.readExternalTemp();
    return (m_ExternalTemperatureSlope * temperatureADCData.voltage) + m_ExternalTemperatureOffset;
}

void Platform::enableHighCurrent()
{
    DEBUG_MSG("Enabling High Current");
    m_HighCurrent=true;
    m_pCBC->driver.enableHighCurrent();
}

void Platform::disableHighCurrent()
{
    DEBUG_MSG("Disabling High Current");
    m_HighCurrent=false;
    m_pCBC->driver.disableHighCurrent();
}

void Platform::enableSynchronousRectification()//public
{
    DEBUG_MSG("Setting SR Mode");
    m_SynchronousRectification=true;
    m_pCBC->driver.enableSR();
}

void Platform::disableSynchronousRectification()//public
{
    DEBUG_MSG("Unsetting SR Mode");
    m_SynchronousRectification=false;
    m_pCBC->driver.disableSR();
}

void Platform::initialize()
{
    DEBUG_MSG("Initializing Platform class object...");
    loadCBCcalibrationParams();
    updateErrorState();
}

void Platform::recoverActuatorStatusFromDB(int actuatorIdx)
{
    DEBUG_MSG("Recovering the status of Actuator " << actuatorIdx << " from DB");
    m_Actuators.at(actuatorIdx)->loadStatusFromDB();
    m_Actuators.at(actuatorIdx)->checkCurrentPosition();
    m_Actuators.at(actuatorIdx)->recordStatusToASF();
    checkActuatorErrors(actuatorIdx);
}

void Platform::clearAllPlatformErrors()
{
    DEBUG_MSG("Clearing all Platform errors.");
    for (int i = 0; i < NUM_ERROR_TYPES; i++)
    {
        m_Errors[i] = false;
    }
    m_State = DeviceState::On;
    updateErrorState(); // Update error status to reflect actuator errors
}

void Platform::findHomeFromEndStop(int direction, int actuatorIdx)
{
    DEBUG_MSG("Finding home from end stop.");
    if(direction == 1) {
        m_Actuators.at(actuatorIdx)->findHomeFromExtendStop();
    }
    else if(direction == -1) {
        m_Actuators.at(actuatorIdx)->findHomeFromRetractStop();
    }
    else {
        ERROR_MSG("Invalid direction given for findHomeFromEndStop.");
    }
}

bool Platform::addMPES(int USB, int serial)
{
    DEBUG_MSG("Adding MPES " << serial << " at USB " << USB);
    if (serial < 0 || USB < 0) {
        ERROR_MSG("Platform:: Failed to add MPES, invalid USB/serial numbers (" << USB << ", " << serial << ")");
        return false;
    }

    #ifndef SIMMODE
    std::unique_ptr<MPES> newMPES = std::unique_ptr<MPES>(new MPES(&m_pCBC, USB, serial));
    #else
    std::unique_ptr<MPES> newMPES = std::unique_ptr<MPES>(new DummyMPES(&m_pCBC, USB, serial));
    #endif

    if (newMPES->initialize()) {
        m_vMPES.push_back(std::move(newMPES));
        return true;
    }
    else {
        ERROR_MSG("Platform:: Failed to initialize MPES at USB " << USB);
        return false;
    }
}

MPES::Position Platform::readMPES(int idx)
{
    DEBUG_MSG("Reading MPES.");
    m_vMPES.at(idx)->measurePosition();
    return m_vMPES.at(internal_idx)->getPosition();
}
