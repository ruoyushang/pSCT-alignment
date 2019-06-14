#include "common/alignment/platform.hpp"

#include <sstream>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <iostream>
#include <random>
#include <algorithm>
#include <unistd.h>

#include "common/alignment/device.hpp"
#include "common/alignment/mpes.hpp"
#include "common/alignment/actuator.hpp"

// Hardcoded
const std::vector<Device::ErrorDefinition> PlatformBase::ERROR_DEFINITIONS = {
    {"Actuator 1 Operable Error",             Device::ErrorState::OperableError},
    {"Actuator 1 Fatal Error",                Device::ErrorState::FatalError},
    {"Actuator 2 Operable Error",             Device::ErrorState::OperableError},
    {"Actuator 2 Fatal Error",                Device::ErrorState::FatalError},
    {"Actuator 3 Operable Error",             Device::ErrorState::OperableError},
    {"Actuator 3 Fatal Error",                Device::ErrorState::FatalError},
    {"Actuator 4 Operable Error",             Device::ErrorState::OperableError},
    {"Actuator 4 Fatal Error",                Device::ErrorState::FatalError},
    {"Actuator 5 Operable Error",             Device::ErrorState::OperableError},
    {"Actuator 5 Fatal Error",                Device::ErrorState::FatalError},
    {"Actuator 6 Operable Error",             Device::ErrorState::OperableError},
    {"Actuator 6 Fatal Error",                Device::ErrorState::FatalError},
    {"Attempt to move out of Software Range", Device::ErrorState::OperableError},
    {"DBInfo not set",                        Device::ErrorState::OperableError},
    {"MySQL Communication Error",             Device::ErrorState::OperableError}
};

// Default calibration constants
const float PlatformBase::DEFAULT_INTERNAL_TEMPERATURE_SLOPE = 100.0;
const float PlatformBase::DEFAULT_INTERNAL_TEMPERATURE_OFFSET = -50.0;
const float PlatformBase::DEFAULT_EXTERNAL_TEMPERATURE_SLOPE = 44.444;
const float PlatformBase::DEFAULT_EXTERNAL_TEMPERATURE_OFFSET = -61.111;

PlatformBase::PlatformBase(Device::Identity identity, Device::DBInfo dbInfo) : Device::Device(std::move(identity)),
                                                                               m_On(true)
{
    std::cout << "Creating Platform Object..." << std::endl;
    m_Errors.assign(getNumErrors(), false);
    if (!dbInfo.empty()) {
        setDBInfo(dbInfo);
    } else {
        std::cout << "Platform::Platform(): No DB info provided..." << std::endl;
    }
}

void PlatformBase::setDBInfo(Device::DBInfo DBInfo) {
    m_DBInfo = std::move(DBInfo);
    if (m_Errors[13]) {
        unsetError(13);
    }
}

void PlatformBase::unsetDBInfo() {
    m_DBInfo = Device::DBInfo();
    if (!m_Errors[13]) {
        setError(13);
    }
}

std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM>
PlatformBase::step(std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> inputSteps) {
    if (getDeviceState() == Device::DeviceState::Off) {
        std::cout << m_Identity << " :: Platform::step() : Panel is off, motion aborted." << std::endl;
        return inputSteps;
    }

    setBusy();
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> stepsRemaining = __step(inputSteps);
    unsetBusy();

    return stepsRemaining;
}

void PlatformBase::checkActuatorStatus(int actuatorIdx) {
    Device::ErrorState actuatorStatus = m_Actuators.at(actuatorIdx)->getErrorState();
    if (actuatorStatus == Device::ErrorState::FatalError) {
        setError((2 * actuatorIdx) + 1);
    } else if (actuatorStatus == Device::ErrorState::OperableError) {
        setError(2 * actuatorIdx);
    }
}

void PlatformBase::probeEndStopAll(int direction) {
    if (getErrorState() == Device::ErrorState::FatalError) {
        std::cout << "Platform::probeEndStopAll() : Encountered fatal error before starting. Motion aborted."
                  << std::endl;
        return;
    }
    if (getDeviceState() == Device::DeviceState::Off) {
        std::cout << "Platform::probeEndStopAll() : Platform is off. Motion aborted." << std::endl;
        return;
    }

    if (direction != 1 && direction != -1) {
        std::cout << m_Identity << " :: Platform::probeEndStopAll(): Invalid choice of direction " << direction
                  << " (must be 1 for extend or -1 for retract.)\n";
        return;
    }

    setBusy();
    __probeEndStopAll(direction);
    unsetBusy();
}

std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> PlatformBase::measureLengths()//public
{
    setBusy();
    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> currentLengths = __measureLengths();
    unsetBusy();
    return currentLengths;
}

std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> PlatformBase::__measureLengths() {
    std::cout << "Measuring Lengths of All Actuators" << std::endl;
    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> currentLengths{};
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        currentLengths[i] = m_Actuators.at(i)->measureLength();
    }
    return currentLengths;
}

std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM>
PlatformBase::moveToLengths(std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> targetLengths) {
    setBusy();

    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> currentLengths = __measureLengths();
    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> deltaLengths{};
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> stepsToTake{};

    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        deltaLengths[i] = targetLengths[i] - currentLengths[i];
        stepsToTake[i] = std::floor((deltaLengths[i] / (m_Actuators.at(i)->mmPerStep)) + 0.5);
    }
    __step(stepsToTake);
    currentLengths = __measureLengths();

    unsetBusy();
    return currentLengths;
}

std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM>
PlatformBase::moveDeltaLengths(std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> deltaLengths) {
    setBusy();
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> stepsToTake{};

    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        stepsToTake[i] = std::floor((deltaLengths[i] / (m_Actuators.at(i)->mmPerStep)) + 0.5);
    }
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> stepsRemaining = __step(stepsToTake);

    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> distancesFromTargets{};
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        distancesFromTargets[i] = stepsRemaining[i] * m_Actuators.at(i)->mmPerStep;
    }

    unsetBusy();
    return distancesFromTargets;
}

bool PlatformBase::initialize() {
    std::cout << "Initializing Platform class object..." << std::endl;
    loadCBCParameters();
    return true;
}

bool PlatformBase::isOn() {
    return m_On;
}

void PlatformBase::emergencyStop() {
    std::cout << m_Identity << " :: Platform::emergencyStop(): executing emergency stop...\n";
    m_On = false; // We temporarily set the state to off to indicate that the motion should be stopped.
    sleep(1);
    m_On = true;
}

Device::ErrorState PlatformBase::getErrorState() {
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        checkActuatorStatus(i);
    }
    Device::ErrorState state = Device::ErrorState::Nominal;
    for (int i = 0; i < getNumErrors(); i++) {
        if (m_Errors[i]) {
            if (getErrorCodeDefinitions()[i].severity > state) {
                state = getErrorCodeDefinitions()[i].severity;
            }
        }
    }

    return state;
}

void PlatformBase::unsetError(int errorCode) {
    if (m_Errors.at(errorCode)) {
        std::cout << m_Identity << " :: Unsetting Error " << errorCode << " ("
                  << getErrorCodeDefinitions()[errorCode].description
                  << ")\n";

        if (errorCode >= 0 && errorCode < 12) {
            std::cout << "Also clearing corresponding errors for Actuator " << (errorCode / 2) + 1 << "." << std::endl;
            m_Actuators.at(errorCode / 2)->clearErrors();
        }
        m_Errors[errorCode] = false;
    }
}

void PlatformBase::clearActuatorErrors() {
    for (int i = 0; i < 2 * PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        unsetError(i);
    }
}

void PlatformBase::clearPlatformErrors() {
    std::cout << "Clearing all Platform errors." << std::endl;
    for (int i = 12; i < getNumErrors(); i++) {
        unsetError(i);
    }
}

MPESBase::Position PlatformBase::readMPES(int idx) {
    std::cout << "Reading MPES." << std::endl;
    m_MPES.at(idx)->updatePosition();
    return m_MPES.at(idx)->getPosition();
}

#ifndef SIMMODE

#include "common/cbccode/cbc.hpp"

bool Platform::addActuators(std::array<Device::Identity, PlatformBase::NUM_ACTS_PER_PLATFORM> actuatorIdentities,
                            const ActuatorBase::ASFInfo &asfInfo) {
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        m_Actuators[i] = std::unique_ptr<ActuatorBase>(new Actuator(m_pCBC, actuatorIdentities[i], m_DBInfo, asfInfo));
        m_ActuatorIdentityMap.insert(std::make_pair(actuatorIdentities[i], i));
        m_Actuators[i]->initialize();
    }
}

bool Platform::loadCBCParameters() {
    if (getSerialNumber() == 0)
    {
        std::cout << "Failed to load CBC Parameters from DB since no CBC Serial given" << std::endl;
        return false;
    }
    std::cout << "Loading CBC Parameters for Controller Board " << getSerialNumber() << std::endl;
    if (m_DBInfo.empty())
    {
        std::cout << "Operable Error: Failed to load CBC parameters from DB because no DB info provided." << std::endl;
        setError(13);
        return false;
    }

    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> EncoderVoltageSlope{};
    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> EncoderVoltageOffset{};
    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> EncoderTemperatureSlope{};
    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> EncoderTemperatureOffset{};
    try
    {
        sql::Driver *driver;
        sql::Connection *con;
        sql::Statement *stmt;
        sql::ResultSet *res;
        driver = get_driver_instance();
        std::string dbAddress = "tcp://" + m_DBInfo.host + ":" + m_DBInfo.port;
        con = driver->connect(dbAddress, m_DBInfo.user, m_DBInfo.password);
        con->setSchema(m_DBInfo.dbname);
        stmt = con->createStatement();

        std::stringstream stmtvar;
        stmtvar << "SELECT * FROM Opt_ControllerBoardCalibration WHERE serial_number=" << getSerialNumber()
                << " ORDER BY start_date DESC LIMIT 1";
        stmt->execute(stmtvar.str());
        res = stmt->getResultSet();
        while (res->next())
        {
            m_InternalTemperatureSlope = res->getDouble(5);
            m_InternalTemperatureOffset = res->getDouble(6);
            m_ExternalTemperatureSlope = res->getDouble(7);
            m_ExternalTemperatureOffset = res->getDouble(8);
            m_CalibrationTemperature = res->getDouble(9);
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
            m_SynchronousRectification = res->getInt(34);
            m_HighCurrent = res->getInt(35);
        }

        delete res;
        delete stmt;
        delete con;

    }
    catch (sql::SQLException &e)
    {
        std::cout << "# ERR: SQLException in " << __FILE__;
        std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
        std::cout << "# ERR: " << e.what();
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        std::cout
            << "Operable Error: SQL Exception caught for platform. Did not successfully communicate with database."
            << std::endl;
        setError(14);
        return false;
    }
    m_pCBC->adc.setEncoderTemperatureRef(m_CalibrationTemperature);
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++)
    {
        m_pCBC->adc.setEncoderTemperatureSlope(i+1, EncoderTemperatureSlope[i]);
        m_pCBC->adc.setEncoderTemperatureOffset(i+1, EncoderTemperatureOffset[i]);
        m_pCBC->adc.setEncoderVoltageSlope(i+1, EncoderVoltageSlope[i]);
        m_pCBC->adc.setEncoderVoltageOffset(i+1, EncoderVoltageOffset[i]);
    }
    if (m_HighCurrent)
    {
        enableHighCurrent();
    }
    else
    {
        disableHighCurrent();
    }
    if (m_SynchronousRectification)
    {
        enableSynchronousRectification();
    }
    else
    {
        disableSynchronousRectification();
    }
    return true;
}

std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM>
Platform::__step(std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> inputSteps) {

    std::cout << "Stepping Platform steps: (" << inputSteps[0] << ", " << inputSteps[1] << ", " << inputSteps[2] << ", "
              << inputSteps[3] << ", " << inputSteps[4] << ", " << inputSteps[5] << ")" << std::endl;
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> StepsRemaining{};
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> ActuatorIterations = {0, 0, 0, 0, 0, 0};
    std::array<ActuatorBase::Position, PlatformBase::NUM_ACTS_PER_PLATFORM> FinalPosition{};

    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        m_Actuators[i]->loadStatusFromASF();
        m_Actuators[i]->recoverPosition();
        FinalPosition[i] = m_Actuators[i]->predictNewPosition(m_Actuators[i]->getCurrentPosition(),
                                                              -inputSteps[i]);//negative steps because positive step is extension of motor, negative steps increases counter since home is defined (0,0)
        StepsRemaining[i] = -(m_Actuators[i]->convertPositionToSteps(FinalPosition[i]) -
                              m_Actuators[i]->convertPositionToSteps(m_Actuators[i]->getCurrentPosition()));

        if (FinalPosition[i].revolution < m_Actuators[i]->ExtendRevolutionLimit ||
            FinalPosition[i].revolution >= m_Actuators[i]->RetractRevolutionLimit) {
            std::cout << "Operable Error: Attempting to move Actuator with Index " << i << " outside of software range."
                      << std::endl;
            setError(12);
            std::cout << "Attempting to move outside of Software Range(" << m_Actuators[i]->HomeLength -
                                                                            (m_Actuators[i]->RetractRevolutionLimit *
                                                                             m_Actuators[i]->StepsPerRevolution *
                                                                             m_Actuators[i]->mmPerStep) << "-"
                      << m_Actuators[i]->HomeLength -
                         (m_Actuators[i]->ExtendRevolutionLimit *
                          m_Actuators[i]->StepsPerRevolution *
                          m_Actuators[i]->mmPerStep)
                      << "mm) for Actuator "
                      << m_Actuators[i]->getSerialNumber() << std::endl;

            return inputSteps;
        }

        if (StepsRemaining[i] != 0) {
            ActuatorIterations[i] = 1 + ((std::abs(StepsRemaining[i]) - 1) /
                                         m_Actuators[i]->RecordingInterval);//simply integer division rounded up, hence the -1.
        }
    }

    int IterationsRemaining = *std::max_element(ActuatorIterations.begin(), ActuatorIterations.end());
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> StepsToTake = {0, 0, 0, 0, 0, 0};
    int Sign;
    int StepsMissed;

    m_pCBC->driver.enableAll();
    while (IterationsRemaining > 1) {
        for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
            if (ActuatorIterations[i] > 1) {
                if (getDeviceState() == Device::DeviceState::Off || getErrorState() == Device::ErrorState::FatalError) {
                    m_pCBC->driver.disableAll();
                    std::cout << m_Identity << " :: Platform::step() : successfully stopped motion.\n";
                    return StepsRemaining;
                }
                StepsToTake[i] = StepsToTake[i] + StepsRemaining[i] / IterationsRemaining;
                if (std::abs(StepsToTake[i]) > m_Actuators[i]->RecordingInterval) {
                    Sign = (StepsToTake[i] > 0) - (StepsToTake[i] < 0);
                    StepsMissed = m_Actuators[i]->step(Sign * m_Actuators[i]->RecordingInterval);
                    StepsToTake[i] = StepsToTake[i] - (Sign * m_Actuators[i]->RecordingInterval) + StepsMissed;
                    StepsRemaining[i] = -(m_Actuators[i]->convertPositionToSteps(FinalPosition[i]) -
                                          m_Actuators[i]->convertPositionToSteps(m_Actuators[i]->getCurrentPosition()));
                    ActuatorIterations[i] = 1 + ((std::abs(StepsRemaining[i]) - 1) / m_Actuators[i]->RecordingInterval);
                    std::cout << "Steps Remaining for Actuator Index " << i << ": " << StepsRemaining[i] << std::endl;
                }
            }
        }
        IterationsRemaining = *std::max_element(ActuatorIterations.begin(), ActuatorIterations.end());
    }

    //Hysteresis Motion
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        if (getDeviceState() == Device::DeviceState::Off || getErrorState() == Device::ErrorState::FatalError) {
            m_pCBC->driver.disableAll();
            std::cout << m_Identity << " :: Platform::step() : successfully stopped motion.\n";
            return StepsRemaining;
        }
        if (StepsRemaining[i] != 0) {
            m_Actuators[i]->performHysteresisMotion(StepsRemaining[i]);
            StepsRemaining[i] = -(m_Actuators[i]->convertPositionToSteps(FinalPosition[i]) -
                                  m_Actuators[i]->convertPositionToSteps(m_Actuators[i]->getCurrentPosition()));
        }

    }

    m_pCBC->driver.disableAll();
    return StepsRemaining;
}

void Platform::__probeEndStopAll(int direction) {
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> SearchSteps{};
    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> VoltageBefore{};
    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> AbsDeltaVoltage{};
    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> VoltageAfter{};

    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        SearchSteps[i] = direction * m_Actuators[i]->EndstopSearchStepsize;
        VoltageAfter[i] = m_Actuators[i]->readVoltage();
    }
    float StoppedSteppingFactor = 0.5;
    std::array<bool, PlatformBase::NUM_ACTS_PER_PLATFORM> NotReachedStop = {true, true, true, true, true, true};

    m_pCBC->driver.enableAll();
    while (NotReachedStop[0] || NotReachedStop[1] || NotReachedStop[2] || NotReachedStop[3] || NotReachedStop[4] ||
           NotReachedStop[5]) {
        for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
            if (NotReachedStop[i]) {
                VoltageBefore[i] = VoltageAfter[i];
                m_Actuators[i]->stepDriver(SearchSteps[i]);
                VoltageAfter[i] = m_Actuators[i]->readVoltage();
                if (getDeviceState() == Device::DeviceState::Off) {
                    std::cout << m_Identity << " :: Platform::probeEndStopAll() : successfully stopped motion.\n";
                    m_pCBC->driver.disableAll();
                    return;
                }
                AbsDeltaVoltage[i] = std::fabs(VoltageAfter[i] - VoltageBefore[i]);
                if (AbsDeltaVoltage[i] < std::fabs(m_Actuators[i]->dV * SearchSteps[i] * StoppedSteppingFactor)) {
                    NotReachedStop[i] = false;
                }
            }
        }
    }
    m_pCBC->driver.disableAll();
}

void Platform::findHomeFromEndStopAll(int direction) {
    if (getErrorState() == Device::ErrorState::FatalError) {
        std::cout << "Platform::findHomeFromEndStopAll() : Encountered fatal error before starting. Motion aborted."
                  << std::endl;
        return;
    }
    if (getDeviceState() == Device::DeviceState::Off) {
        std::cout << "Platform::findHomeFromEndStopAll() : Platform is off. Motion aborted." << std::endl;
        return;
    }

    if (direction != 1 && direction != -1) {
        std::cout << m_Identity << " :: Platform::findHomeFromEndStopAll(): Invalid choice of direction " << direction
                  << " (must be 1 for extend or -1 for retract.)\n";
        return;
    }

    setBusy();
    __probeEndStopAll(direction);

    if (getErrorState() == Device::ErrorState::FatalError) {
        std::cout << "Platform::findHomeFromEndStopAll() : Failed to reach end stop due to fatal error in probeEndStopAll(). Find home aborted."
                  << std::endl;
        unsetBusy();
        return;
    }
    else if (getDeviceState() == Device::DeviceState::Off) {
        std::cout << "Platform::findHomeFromEndStopAll() : Platform is off. Find home aborted." << std::endl;
        unsetBusy();
        return;
    }

    m_pCBC->driver.enableAll();
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++)
    {
        m_Actuators[i]->findHomeFromEndStop(direction);
    }
    m_pCBC->driver.disableAll();
    unsetBusy();
}

bool Platform::probeHomeAll()
{
    setBusy();
    std::cout << "Probing Home for All Actuators" << std::endl;
    __probeEndStopAll(1);
    m_pCBC->driver.enableAll();
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++)
    {
        m_Actuators.at(i)->probeHome();
    }
    m_pCBC->driver.disableAll();

    unsetBusy();
    return true;
}

float Platform::getInternalTemperature()
{
    std::cout << "Reading Internal Temperature" << std::endl;
    CBC::ADC::adcData temperatureADCData = m_pCBC->adc.readTemperatureVolts();
    return (m_InternalTemperatureSlope * temperatureADCData.voltage) + m_InternalTemperatureOffset;
}

float Platform::getExternalTemperature()
{
    std::cout << "Reading External Temperature" << std::endl;
    CBC::ADC::adcData temperatureADCData = m_pCBC->adc.readExternalTemp();
    return (m_ExternalTemperatureSlope * temperatureADCData.voltage) + m_ExternalTemperatureOffset;
}

void Platform::enableHighCurrent()
{
    std::cout << "Enabling High Current" << std::endl;
    m_HighCurrent=true;
    m_pCBC->driver.enableHighCurrent();
}

void Platform::disableHighCurrent()
{
    std::cout << "Disabling High Current" << std::endl;
    m_HighCurrent=false;
    m_pCBC->driver.disableHighCurrent();
}

void Platform::enableSynchronousRectification()//public
{
    std::cout << "Setting SR Mode" << std::endl;
    m_SynchronousRectification=true;
    m_pCBC->driver.enableSR();
}

void Platform::disableSynchronousRectification()//public
{
    std::cout << "Unsetting SR Mode" << std::endl;
    m_SynchronousRectification=false;
    m_pCBC->driver.disableSR();
}

void Platform::turnOn() {
    std::cout << m_Identity << " :: turning on...\n";
    setBusy();
    m_pCBC->powerUp();
    for (const auto& pMPES : m_MPES) {
    	pMPES->turnOn();
    }
    for (const auto& pActuator : m_Actuators) {
    	pActuator->turnOn();
    }
    m_On = true;
    unsetBusy();
}

void Platform::turnOff() {
    std::cout << m_Identity << " :: turning off...\n";
    m_pCBC->powerDown();
    m_On = false;
    unsetBusy();
}

bool Platform::addMPES(const Device::Identity &identity)
{
    std::cout << m_Identity << " :: Adding MPES " << identity.serialNumber << " at USB " << identity.eAddress
              << std::endl;
    if (identity.serialNumber < 0 || std::stoi(identity.eAddress) < 0) {
        std::cout << m_Identity << " :: Failed to add MPES, invalid USB/serial numbers (" << identity.eAddress << ", "
                  << identity.serialNumber << ")"
                  << std::endl;
        return false;
    }

    std::unique_ptr<MPES> newMPES = std::unique_ptr<MPES>(new MPES(m_pCBC, identity));
    
    if (newMPES->initialize()) {
        m_MPES.push_back(std::move(newMPES));
        m_MPESIdentityMap.insert(std::make_pair(identity, m_MPES.size() - 1));
        return true;
    }
    else {
        std::cout << m_Identity << " :: Failed to initialize MPES at USB " << identity.eAddress << std::endl;
        return false;
    }
}

#endif

std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM>
DummyPlatform::__step(std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> inputSteps) {

    std::cout << "Stepping Platform steps: (" << inputSteps[0] << ", " << inputSteps[1] << ", " << inputSteps[2] << ", "
              << inputSteps[3] << ", " << inputSteps[4] << ", " << inputSteps[5] << ")" << std::endl;
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> StepsRemaining{};
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> ActuatorIterations = {0, 0, 0, 0, 0, 0};
    std::array<ActuatorBase::Position, PlatformBase::NUM_ACTS_PER_PLATFORM> FinalPosition{};

    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        FinalPosition[i] = m_Actuators[i]->predictNewPosition(m_Actuators[i]->getCurrentPosition(),
                                                              -inputSteps[i]);//negative steps because positive step is extension of motor, negative steps increases counter since home is defined (0,0)
        StepsRemaining[i] = -(m_Actuators[i]->convertPositionToSteps(FinalPosition[i]) -
                              m_Actuators[i]->convertPositionToSteps(m_Actuators[i]->getCurrentPosition()));

        if (FinalPosition[i].revolution < m_Actuators[i]->ExtendRevolutionLimit ||
            FinalPosition[i].revolution >= m_Actuators[i]->RetractRevolutionLimit) {
            std::cout << "Operable Error: Attempting to move Actuator with Index " << i << " outside of software range."
                      << std::endl;
            setError(12);
            std::cout << "Attempting to move outside of Software Range(" << m_Actuators[i]->HomeLength -
                                                                            (m_Actuators[i]->RetractRevolutionLimit *
                                                                             m_Actuators[i]->StepsPerRevolution *
                                                                             m_Actuators[i]->mmPerStep) << "-"
                      << m_Actuators[i]->HomeLength -
                         (m_Actuators[i]->ExtendRevolutionLimit *
                          m_Actuators[i]->StepsPerRevolution *
                          m_Actuators[i]->mmPerStep)
                      << "mm) for Actuator "
                      << m_Actuators[i]->getSerialNumber() << std::endl;

            return inputSteps;
        }

        if (StepsRemaining[i] != 0) {
            ActuatorIterations[i] = 1 + ((std::abs(StepsRemaining[i]) - 1) /
                                         m_Actuators[i]->RecordingInterval);//simply integer division rounded up, hence the -1.
        }
    }

    int IterationsRemaining = *std::max_element(ActuatorIterations.begin(), ActuatorIterations.end());
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> StepsToTake = {0, 0, 0, 0, 0, 0};
    int Sign;
    int StepsMissed;

    while (IterationsRemaining > 1) {
        for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
            if (ActuatorIterations[i] > 1) {
                if (getDeviceState() == Device::DeviceState::Off || getErrorState() == Device::ErrorState::FatalError) {
                    std::cout << m_Identity << " :: Platform::step() : successfully stopped motion.\n";
                    return StepsRemaining;
                }
                StepsToTake[i] = StepsToTake[i] + StepsRemaining[i] / IterationsRemaining;
                if (std::abs(StepsToTake[i]) > m_Actuators[i]->RecordingInterval) {
                    Sign = (StepsToTake[i] > 0) - (StepsToTake[i] < 0);
                    StepsMissed = m_Actuators[i]->step(Sign * m_Actuators[i]->RecordingInterval);
                    StepsToTake[i] = StepsToTake[i] - (Sign * m_Actuators[i]->RecordingInterval) + StepsMissed;
                    StepsRemaining[i] = -(m_Actuators[i]->convertPositionToSteps(FinalPosition[i]) -
                                          m_Actuators[i]->convertPositionToSteps(m_Actuators[i]->getCurrentPosition()));
                    ActuatorIterations[i] = 1 + ((std::abs(StepsRemaining[i]) - 1) / m_Actuators[i]->RecordingInterval);
                    std::cout << "Steps Remaining for Actuator Index " << i << ": " << StepsRemaining[i] << std::endl;
                }
            }
        }
        IterationsRemaining = *std::max_element(ActuatorIterations.begin(), ActuatorIterations.end());
    }

    //Hysteresis Motion
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        if (getDeviceState() == Device::DeviceState::Off || getErrorState() == Device::ErrorState::FatalError) {
            std::cout << m_Identity << " :: Platform::step() : successfully stopped motion.\n";
            return StepsRemaining;
        }
        if (StepsRemaining[i] != 0) {
            m_Actuators[i]->performHysteresisMotion(StepsRemaining[i]);
            StepsRemaining[i] = -(m_Actuators[i]->convertPositionToSteps(FinalPosition[i]) -
                                  m_Actuators[i]->convertPositionToSteps(m_Actuators[i]->getCurrentPosition()));
        }

    }

    return StepsRemaining;
}

void DummyPlatform::__probeEndStopAll(int direction) {
    std::cout << "DummyPlatform::probeEndStopAll() : Probing end stop in direction " << direction
              << "(nothing should happen)..."
              << std::endl;
}

void DummyPlatform::findHomeFromEndStopAll(int direction) {
    if (getErrorState() == Device::ErrorState::FatalError) {
        std::cout
            << "DummyPlatform::findHomeFromEndStopAll() : Encountered fatal error before starting. Motion aborted."
            << std::endl;
        return;
    }
    if (getDeviceState() == Device::DeviceState::Off) {
        std::cout << "DummyPlatform::findHomeFromEndStopAll() : Platform is off. Motion aborted." << std::endl;
        return;
    }

    if (direction != 1 && direction != -1) {
        std::cout << m_Identity << " :: DummyPlatform::findHomeFromEndStopAll(): Invalid choice of direction "
                  << direction
                  << " (must be 1 for extend or -1 for retract.)\n";
        return;
    }

    setBusy();
    __probeEndStopAll(direction);

    if (getErrorState() == Device::ErrorState::FatalError) {
        std::cout
            << "DummyPlatform::findHomeFromEndStopAll() : Failed to reach end stop due to fatal error in probeEndStopAll(). Find home aborted."
            << std::endl;
        unsetBusy();
        return;
    } else if (getDeviceState() == Device::DeviceState::Off) {
        std::cout << "DummyPlatform::findHomeFromEndStopAll() : Platform is off. Find home aborted." << std::endl;
        unsetBusy();
        return;
    }

    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        m_Actuators[i]->findHomeFromEndStop(direction);
    }
    unsetBusy();
}

bool DummyPlatform::probeHomeAll() {
    setBusy();
    std::cout << "DummyPlatform :: Probing Home for All Actuators" << std::endl;
    __probeEndStopAll(1);
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        m_Actuators.at(i)->probeHome();
    }
    unsetBusy();
    return true;
}

float DummyPlatform::getInternalTemperature() {
    std::cout
        << "DummyPlatform:: Reading External Temperature (will return normally distributed value w/ mean 20.0, stddev 2.0)"
        << std::endl;
    std::random_device rd{};
    std::mt19937 generator{rd()};

    std::normal_distribution<double> internalTempDistribution(20.0, 2.0);

    double newValue;
    newValue = internalTempDistribution(generator);
    return newValue;
}

float DummyPlatform::getExternalTemperature() {
    std::cout
        << "DummyPlatform:: Reading External Temperature (will return normally distributed value w/ mean 30.0, stddev 2.0)"
        << std::endl;
    std::random_device rd{};
    std::mt19937 generator{rd()};

    std::normal_distribution<double> externalTempDistribution(30.0, 2.0);

    double newValue;
    newValue = externalTempDistribution(generator);
    return newValue;
}

void DummyPlatform::enableHighCurrent() {
    std::cout << "DummyPlatform :: Enabling High Current" << std::endl;
    m_HighCurrent = true;
}

void DummyPlatform::disableHighCurrent() {
    std::cout << "DummyPlatform :: Disabling High Current" << std::endl;
    m_HighCurrent = false;
}

void DummyPlatform::enableSynchronousRectification()//public
{
    std::cout << "DummyPlatform :: Setting SR Mode" << std::endl;
    m_SynchronousRectification = true;
}

void DummyPlatform::disableSynchronousRectification()//public
{
    std::cout << "DummyPlatform :: Unsetting SR Mode" << std::endl;
    m_SynchronousRectification = false;
}

void DummyPlatform::turnOn() {
    std::cout << m_Identity << " DummyPlatform :: turning on...\n";
    setBusy();
    for (const auto &pMPES : m_MPES) {
        pMPES->turnOn();
    }
    for (const auto &pActuator : m_Actuators) {
        pActuator->turnOn();
    }
    m_On = true;
    unsetBusy();
}

void DummyPlatform::emergencyStop() {
    std::cout << m_Identity
              << " :: DummyPlatform::emergencyStop(): executing emergency stop (should have no effect)...\n";
    m_On = false; // We temporarily set the state to off to indicate that the motion should be stopped.
    sleep(1);
    m_On = true;
}

void DummyPlatform::turnOff() {
    std::cout << m_Identity << " Dummy Platform :: turning off...\n";
    m_On = false;
    unsetBusy();
}

bool DummyPlatform::addActuators(std::array<Device::Identity, PlatformBase::NUM_ACTS_PER_PLATFORM> actuatorIdentities,
                                 const ActuatorBase::ASFInfo &asfInfo) {
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        m_Actuators[i] = std::unique_ptr<ActuatorBase>(new DummyActuator(actuatorIdentities[i], m_DBInfo, asfInfo));
        m_ActuatorIdentityMap.insert(std::make_pair(actuatorIdentities[i], i));
        m_Actuators[i]->initialize();
    }
}

bool DummyPlatform::addMPES(const Device::Identity &identity) {
    std::cout << m_Identity << " :: Adding DummyMPES " << identity.serialNumber << " at USB " << identity.eAddress
              << std::endl;
    if (identity.serialNumber < 0 || std::stoi(identity.eAddress) < 0) {
        std::cout << m_Identity << " :: Failed to add DummyMPES, invalid USB/serial numbers (" << identity.eAddress
                  << ", "
                  << identity.serialNumber << ")"
                  << std::endl;
        return false;
    }

    std::unique_ptr<MPESBase> newMPES = std::unique_ptr<MPESBase>(new DummyMPES(identity));

    if (newMPES->initialize()) {
        m_MPES.push_back(std::move(newMPES));
        m_MPESIdentityMap.insert(std::make_pair(identity, m_MPES.size() - 1));
        return true;
    } else {
        std::cout << m_Identity << " :: Failed to initialize MPES at USB " << identity.eAddress << std::endl;
        return false;
    }
}

bool DummyPlatform::loadCBCParameters() {
    if (getSerialNumber() == 0) {
        std::cout << "Failed to load CBC Parameters from DB since no CBC Serial given" << std::endl;
        return false;
    }
    std::cout << "Loading CBC Parameters for Controller Board " << getSerialNumber() << std::endl;
    if (m_DBInfo.empty()) {
        std::cout << "Operable Error: Failed to load CBC parameters from DB because no DB info provided." << std::endl;
        setError(13);
        return false;
    }

    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> EncoderVoltageSlope{};
    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> EncoderVoltageOffset{};
    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> EncoderTemperatureSlope{};
    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> EncoderTemperatureOffset{};
    try {
        sql::Driver *driver;
        sql::Connection *con;
        sql::Statement *stmt;
        sql::ResultSet *res;
        driver = get_driver_instance();
        std::string dbAddress = "tcp://" + m_DBInfo.host + ":" + m_DBInfo.port;
        con = driver->connect(dbAddress, m_DBInfo.user, m_DBInfo.password);
        con->setSchema(m_DBInfo.dbname);
        stmt = con->createStatement();

        std::stringstream stmtvar;
        stmtvar << "SELECT * FROM Opt_ControllerBoardCalibration WHERE serial_number=" << getSerialNumber()
                << " ORDER BY start_date DESC LIMIT 1";
        stmt->execute(stmtvar.str());
        res = stmt->getResultSet();
        while (res->next()) {
            m_InternalTemperatureSlope = res->getDouble(5);
            m_InternalTemperatureOffset = res->getDouble(6);
            m_ExternalTemperatureSlope = res->getDouble(7);
            m_ExternalTemperatureOffset = res->getDouble(8);
            m_CalibrationTemperature = res->getDouble(9);
            EncoderVoltageSlope[0] = res->getDouble(10);
            EncoderVoltageOffset[0] = res->getDouble(11);
            EncoderTemperatureSlope[0] = res->getDouble(12);
            EncoderTemperatureOffset[0] = res->getDouble(13);
            EncoderVoltageSlope[1] = res->getDouble(14);
            EncoderVoltageOffset[1] = res->getDouble(15);
            EncoderTemperatureSlope[1] = res->getDouble(16);
            EncoderTemperatureOffset[1] = res->getDouble(17);
            EncoderVoltageSlope[2] = res->getDouble(18);
            EncoderVoltageOffset[2] = res->getDouble(19);
            EncoderTemperatureSlope[2] = res->getDouble(20);
            EncoderTemperatureOffset[2] = res->getDouble(21);
            EncoderVoltageSlope[3] = res->getDouble(22);
            EncoderVoltageOffset[3] = res->getDouble(23);
            EncoderTemperatureSlope[3] = res->getDouble(24);
            EncoderTemperatureOffset[3] = res->getDouble(25);
            EncoderVoltageSlope[4] = res->getDouble(26);
            EncoderVoltageOffset[4] = res->getDouble(27);
            EncoderTemperatureSlope[4] = res->getDouble(28);
            EncoderTemperatureOffset[4] = res->getDouble(29);
            EncoderVoltageSlope[5] = res->getDouble(30);
            EncoderVoltageOffset[5] = res->getDouble(31);
            EncoderTemperatureSlope[5] = res->getDouble(32);
            EncoderTemperatureOffset[5] = res->getDouble(33);
            m_SynchronousRectification = res->getInt(34);
            m_HighCurrent = res->getInt(35);
        }

        delete res;
        delete stmt;
        delete con;

    }
    catch (sql::SQLException &e) {
        std::cout << "# ERR: SQLException in " << __FILE__;
        std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
        std::cout << "# ERR: " << e.what();
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        std::cout
            << "Operable Error: SQL Exception caught for platform. Did not successfully communicate with database."
            << std::endl;
        setError(14);
        return false;
    }
    if (m_HighCurrent) {
        enableHighCurrent();
    } else {
        disableHighCurrent();
    }
    if (m_SynchronousRectification) {
        enableSynchronousRectification();
    } else {
        disableSynchronousRectification();
    }
    return true;
}



