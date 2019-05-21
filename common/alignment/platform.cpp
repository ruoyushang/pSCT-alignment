#ifdef DEBUG
#define DEBUG_MSG(str) do {std::cout << str << std::endl;} while (false)
#else
#define DEBUG_MSG(str) do {} while (false)
#endif

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

#include "common/alignment/device.hpp"
#include "common/cbccode/cbc.hpp"
#include "common/alignment/mpes.hpp"
#include "common/alignment/actuator.hpp"

// Hardcoded
const std::vector<Device::ErrorDefinition> Platform::ERROR_DEFINITIONS = {
        {"Actuator 0 Operable Error",             Device::DeviceState::OperableError},
        {"Actuator 0 Fatal Error",                Device::DeviceState::FatalError},
        {"Actuator 1 Operable Error",             Device::DeviceState::OperableError},
        {"Actuator 1 Fatal Error",                Device::DeviceState::FatalError},
        {"Actuator 2 Operable Error",             Device::DeviceState::OperableError},
        {"Actuator 2 Fatal Error",                Device::DeviceState::FatalError},
        {"Actuator 3 Operable Error",             Device::DeviceState::OperableError},
        {"Actuator 3 Fatal Error",                Device::DeviceState::FatalError},
        {"Actuator 4 Operable Error",             Device::DeviceState::OperableError},
        {"Actuator 4 Fatal Error",                Device::DeviceState::FatalError},
        {"Actuator 5 Operable Error",             Device::DeviceState::OperableError},
        {"Actuator 5 Fatal Error",                Device::DeviceState::FatalError},
        {"Attempt to move out of Software Range", Device::DeviceState::OperableError},
        {"DBInfo not set",                        Device::DeviceState::OperableError},
        {"MySQL Communication Error",             Device::DeviceState::OperableError}
};

// Default calibration constants
const float Platform::DEFAULT_INTERNAL_TEMPERATURE_SLOPE = 100.0;
const float Platform::DEFAULT_INTERNAL_TEMPERATURE_OFFSET = -50.0;
const float Platform::DEFAULT_EXTERNAL_TEMPERATURE_SLOPE = 44.444;
const float Platform::DEFAULT_EXTERNAL_TEMPERATURE_OFFSET = -61.111;

Platform::Platform() : Device::Device(std::make_shared<CBC>(), Device::Identity())
{
    DEBUG_MSG("Creating Platform object...");
    m_Errors.assign(getNumErrors(), false);
    Device::Identity act_identity;
    for (int i = 0; i < Platform::NUM_ACTS_PER_PLATFORM; i++)
    {
        #ifndef SIMMODE
        m_Actuators[i] = std::unique_ptr<Actuator>(new Actuator(m_pCBC, act_identity));
        #else
        m_Actuators[i] = std::unique_ptr<Actuator>(new DummyActuator(m_pCBC, act_identity));
        #endif
        m_ActuatorIdentityMap.insert(std::make_pair(act_identity, i));
    }
    initialize();
}

Platform::Platform(Device::Identity identity, std::array<int, Platform::NUM_ACTS_PER_PLATFORM> actuatorPorts,
                   std::array<int, Platform::NUM_ACTS_PER_PLATFORM> actuatorSerials, Device::DBInfo dbInfo,
                   const Actuator::ASFInfo &asfInfo) : Device::Device(std::make_shared<CBC>(), std::move(identity))
{
    DEBUG_MSG("Creating Platform Object with Actuator Serial Numbers, Ports, DB Information and custom ASF paths");
    m_Errors.assign(getNumErrors(), false);
    Device::Identity act_identity;
    for (int i = 0; i < Platform::NUM_ACTS_PER_PLATFORM; i++)
    {
        act_identity.name = std::string("ACT_") + std::to_string(actuatorSerials.at(i));
        act_identity.serialNumber = actuatorSerials.at(i);
        act_identity.eAddress = std::to_string(actuatorPorts.at(i));
        act_identity.position = i + 1;
        #ifndef SIMMODE
        m_Actuators[i] = std::unique_ptr<Actuator>(new Actuator(m_pCBC, act_identity, dbInfo, asfInfo));
        #else
        m_Actuators[i] = std::unique_ptr<Actuator>(new DummyActuator(m_pCBC, act_identity, dbInfo, asfInfo));
        #endif
        m_ActuatorIdentityMap.insert(std::make_pair(act_identity, i));
    }
    if (!dbInfo.empty()) {
        setDBInfo(dbInfo);
    } else {
        DEBUG_MSG("Platform::Platform(): No DB info provided...");
    }
    initialize();
}

Platform::~Platform()
{
    m_pCBC->driver.disableAll();
}

bool Platform::loadCBCParameters() {
    if (getSerialNumber() == 0)
    {
        DEBUG_MSG("Failed to load CBC Parameters from DB since no CBC Serial given");
        return false;
    }
    DEBUG_MSG("Loading CBC Parameters for Controller Board " << getSerialNumber());
    if (m_DBInfo.empty())
    {
        std::cout << "Operable Error: Failed to load CBC parameters from DB because no DB info provided." << std::endl;
        setError(13);
        return false;
    }

    std::array<float, Platform::NUM_ACTS_PER_PLATFORM> EncoderVoltageSlope{};
    std::array<float, Platform::NUM_ACTS_PER_PLATFORM> EncoderVoltageOffset{};
    std::array<float, Platform::NUM_ACTS_PER_PLATFORM> EncoderTemperatureSlope{};
    std::array<float, Platform::NUM_ACTS_PER_PLATFORM> EncoderTemperatureOffset{};
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
    for (int i = 0; i < Platform::NUM_ACTS_PER_PLATFORM; i++)
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

void Platform::setDBInfo(Device::DBInfo DBInfo) {
    m_DBInfo = std::move(DBInfo);
    if (m_Errors[13]) {
        unsetError(13);
    }
}

void Platform::unsetDBInfo() {
    m_DBInfo = Device::DBInfo();
    if (!m_Errors[13]) {
        setError(13);
    }
}

std::array<int, Platform::NUM_ACTS_PER_PLATFORM>
Platform::step(std::array<int, Platform::NUM_ACTS_PER_PLATFORM> inputSteps) {
    m_state = Device::DeviceState::Busy; // mark platform as busy

    DEBUG_MSG("Stepping Platform steps: (" << inputSteps[0] << ", " << inputSteps[1] << ", " << inputSteps[2] << ", " << inputSteps[3] << ", " << inputSteps[4] << ", " << inputSteps[5] << ")");
    if (m_state == Device::DeviceState::FatalError)
    {
        m_state = Device::DeviceState::Off; // Don't move, turn platform off
        return inputSteps;
    }

    std::array<int, Platform::NUM_ACTS_PER_PLATFORM> StepsRemaining{};
    std::array<int, Platform::NUM_ACTS_PER_PLATFORM> ActuatorIterations = {0, 0, 0, 0, 0, 0};
    std::array<Actuator::Position, Platform::NUM_ACTS_PER_PLATFORM> FinalPosition{};

    for (int i = 0; i < Platform::NUM_ACTS_PER_PLATFORM; i++)
    {
        m_Actuators[i]->loadStatusFromASF();
        m_Actuators[i]->recoverPosition();
        FinalPosition[i] = m_Actuators[i]->predictNewPosition(m_Actuators[i]->getCurrentPosition(),
                                                           -inputSteps[i]);//negative steps because positive step is extension of motor, negative steps increases counter since home is defined (0,0)
        StepsRemaining[i] = -(m_Actuators[i]->convertPositionToSteps(FinalPosition[i]) -
                              m_Actuators[i]->convertPositionToSteps(m_Actuators[i]->getCurrentPosition()));

        if (FinalPosition[i].revolution < m_Actuators[i]->ExtendRevolutionLimit ||
            FinalPosition[i].revolution >= m_Actuators[i]->RetractRevolutionLimit)
        {
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

            // reset the state to Off
            m_state = Device::DeviceState::Off;

            return inputSteps;
        }

        if(StepsRemaining[i] != 0)
        {
            ActuatorIterations[i] = 1 + ((std::abs(StepsRemaining[i]) - 1) /
                                         m_Actuators[i]->RecordingInterval);//simply integer division rounded up, hence the -1.
        }
    }

    int IterationsRemaining=*std::max_element(ActuatorIterations.begin(), ActuatorIterations.end());
    std::array<int, Platform::NUM_ACTS_PER_PLATFORM> StepsToTake = {0, 0, 0, 0, 0, 0};
    int Sign;
    int StepsMissed;

    // m_state can be changed by an external caller during motion -- need to check this
    m_pCBC->driver.enableAll();
    while(IterationsRemaining>1)
    {
        // need to check the state and if needed, break TWICE --
        // once for the inner actuator loop and once for the outer cycle loop
        if (m_state == Device::DeviceState::Off || m_state == Device::DeviceState::FatalError) {
            break;
        }

        for (int i = 0; i < Platform::NUM_ACTS_PER_PLATFORM; i++)
        {
            if (m_state == Device::DeviceState::Off || m_state == Device::DeviceState::FatalError) {
                break;
            }

            if(ActuatorIterations[i] > 1)
            {
                if (getState() == Device::DeviceState::FatalError)
                {
                    m_pCBC->driver.disableAll();

                    // reset the state to Off
                    m_state = Device::DeviceState::Off;

                    return StepsRemaining;
                }
                StepsToTake[i]=StepsToTake[i]+StepsRemaining[i]/IterationsRemaining;
                if (std::abs(StepsToTake[i]) > m_Actuators[i]->RecordingInterval)
                {
                    Sign=(StepsToTake[i]>0)-(StepsToTake[i]<0);
                    StepsMissed = m_Actuators[i]->step(Sign * m_Actuators[i]->RecordingInterval);
                    checkActuatorStatus(i);
                    StepsToTake[i] = StepsToTake[i] - (Sign * m_Actuators[i]->RecordingInterval) + StepsMissed;
                    StepsRemaining[i] = -(m_Actuators[i]->convertPositionToSteps(FinalPosition[i]) -
                                          m_Actuators[i]->convertPositionToSteps(m_Actuators[i]->getCurrentPosition()));
                    ActuatorIterations[i] = 1 + ((std::abs(StepsRemaining[i]) - 1) / m_Actuators[i]->RecordingInterval);
                    DEBUG_MSG("Steps Remaining for Actuator Index " << i << ": " << StepsRemaining[i]);
                }
            }
        }
        IterationsRemaining=*std::max_element(ActuatorIterations.begin(), ActuatorIterations.end());
    }

    //Hysteresis Motion
    if (m_state != Device::DeviceState::Off && m_state != Device::DeviceState::FatalError) {
        for (int i = 0; i < Platform::NUM_ACTS_PER_PLATFORM; i++)
        {
            if (m_state == Device::DeviceState::FatalError)
            {
                m_pCBC->driver.disableAll();

                // reset the state to Off
                m_state = Device::DeviceState::Off;

                return StepsRemaining;
            }
            if(StepsRemaining[i] != 0)
            {
                m_Actuators[i]->performHysteresisMotion(StepsRemaining[i]);
                checkActuatorStatus(i);
                StepsRemaining[i] = -(m_Actuators[i]->convertPositionToSteps(FinalPosition[i]) -
                                      m_Actuators[i]->convertPositionToSteps(m_Actuators[i]->getCurrentPosition()));
            }

        }
    }

    m_pCBC->driver.disableAll();

    // reset the state to On
    m_state = Device::DeviceState::On;

    return StepsRemaining;
}

void Platform::checkActuatorStatus(int actuatorIdx) {
    Device::DeviceState actuatorStatus = m_Actuators.at(actuatorIdx)->getState();
    if (actuatorStatus == Device::DeviceState::FatalError)
    {
        setError((2*actuatorIdx)+1);
    } else if (actuatorStatus == Device::DeviceState::OperableError)
    {
        setError(2*actuatorIdx);
    }
}

Device::DeviceState Platform::getState() {
    m_state = Device::DeviceState::On;
    for (int i = 0; i < Platform::NUM_ACTS_PER_PLATFORM; i++)
    {
        checkActuatorStatus(i);
    }
    return m_state;
}

void Platform::probeEndStopAll(int direction)
{
    if (m_state == Device::DeviceState::FatalError)
    {
        return;
    }
    std::array<int, Platform::NUM_ACTS_PER_PLATFORM> SearchSteps{};
    std::array<float, Platform::NUM_ACTS_PER_PLATFORM> VoltageBefore{};
    std::array<float, Platform::NUM_ACTS_PER_PLATFORM> AbsDeltaVoltage{};
    std::array<float, Platform::NUM_ACTS_PER_PLATFORM> VoltageAfter{};

    for (int i = 0; i < Platform::NUM_ACTS_PER_PLATFORM; i++)
    {
        SearchSteps[i] = direction * m_Actuators[i]->EndstopSearchStepsize;
        VoltageAfter[i] = m_Actuators[i]->readVoltage();
        //checkActuatorStatus(i);
        if (m_state == Device::DeviceState::FatalError)
        {
            return;
        }
    }
    float StoppedSteppingFactor=0.5;
    std::array<bool, Platform::NUM_ACTS_PER_PLATFORM> NotReachedStop = {true, true, true, true, true, true};
    m_pCBC->driver.enableAll();

    while( NotReachedStop[0] || NotReachedStop[1] || NotReachedStop[2] || NotReachedStop[3] || NotReachedStop[4] || NotReachedStop[5] )
    {
        for (int i = 0; i < Platform::NUM_ACTS_PER_PLATFORM; i++)
        {
            if (NotReachedStop[i])
            {
                VoltageBefore[i]=VoltageAfter[i];
                m_pCBC->driver.step(m_Actuators[i]->getPortNumber(), SearchSteps[i]);
                VoltageAfter[i] = m_Actuators[i]->readVoltage();
                checkActuatorStatus(i);
                if (m_state == Device::DeviceState::FatalError)
                {
                    m_pCBC->driver.disableAll();
                    return;
                }
                AbsDeltaVoltage[i]=std::fabs(VoltageAfter[i]-VoltageBefore[i]);
                if (AbsDeltaVoltage[i] < std::fabs(m_Actuators[i]->dV * SearchSteps[i] * StoppedSteppingFactor))
                {
                    NotReachedStop[i]=false;
                }
            }
        }
    }

    m_pCBC->driver.disableAll();
}

void Platform::findHomeFromEndStopAll(int direction) {
    if (direction == 1)
    {
        probeExtendStopAll();
    } else if (direction == -1)
    {
        probeRetractStopAll();
    }
    else
    {
        return;
    }

    updateState();
    if (m_state == Device::DeviceState::FatalError)
    {
        return;
    }

    m_pCBC->driver.enableAll();

    for (int i = 0; i < Platform::NUM_ACTS_PER_PLATFORM; i++)
    {
        if (direction == 1)
        {
            m_Actuators[i]->findHomeFromExtendStop();
        } else if (direction == -1)
        {
            m_Actuators[i]->findHomeFromRetractStop();
        }
        else
        {
            return;
        }
        checkActuatorStatus(i);
        if (m_state == Device::DeviceState::FatalError)
        {
            m_pCBC->driver.disableAll();
            return;
        }
    }
    m_pCBC->driver.disableAll();
}

bool Platform::probeHomeAll()
{
    DEBUG_MSG("Probing Home for All Actuators");
    probeExtendStopAll();
    if (m_state == Device::DeviceState::FatalError)
    {
        std::cout << "Platform:: Cannot probe home, Platform has a Fatal Error" << std::endl;
        return false;
    }

    m_pCBC->driver.enableAll();
    for (int i = 0; i < Platform::NUM_ACTS_PER_PLATFORM; i++)
    {
        m_Actuators.at(i)->probeHome();
        checkActuatorStatus(i);
        if (m_state == Device::DeviceState::FatalError)
        {
            std::cout << "Platform:: Actuator " << i
                      << " encountered a fatal error after probing home. Stopping probeHomeAll..." << std::endl;
            m_pCBC->driver.disableAll();
            return false;
        }
    }
    m_pCBC->driver.disableAll();
    return true;
}


std::array<float, Platform::NUM_ACTS_PER_PLATFORM> Platform::measureLengths()//public
{
    DEBUG_MSG("Measuring Lengths of All Actuators");
    std::array<float, Platform::NUM_ACTS_PER_PLATFORM> currentLengths{};
    for (int i = 0; i < Platform::NUM_ACTS_PER_PLATFORM; i++)
    {
        currentLengths[i] = m_Actuators.at(i)->measureLength();
    }
    return currentLengths;
}

std::array<float, Platform::NUM_ACTS_PER_PLATFORM>
Platform::moveToLengths(std::array<float, Platform::NUM_ACTS_PER_PLATFORM> targetLengths) {
    std::array<float, Platform::NUM_ACTS_PER_PLATFORM> currentLengths = measureLengths();
    std::array<float, Platform::NUM_ACTS_PER_PLATFORM> deltaLengths{};
    std::array<int, Platform::NUM_ACTS_PER_PLATFORM> stepsToTake{};

    for (int i = 0; i < Platform::NUM_ACTS_PER_PLATFORM; i++)
    {
        deltaLengths[i] = targetLengths[i] - currentLengths[i];
        stepsToTake[i] = std::floor((deltaLengths[i]/(m_Actuators.at(i)->mmPerStep)) + 0.5);
    }
    step(stepsToTake);
    currentLengths = measureLengths();
    return currentLengths;
}

std::array<float, Platform::NUM_ACTS_PER_PLATFORM>
Platform::moveDeltaLengths(std::array<float, Platform::NUM_ACTS_PER_PLATFORM> deltaLengths) {
    std::array<float, Platform::NUM_ACTS_PER_PLATFORM> currentLengths = measureLengths();
    std::array<float, Platform::NUM_ACTS_PER_PLATFORM> targetLengths{};

    for (int i = 0; i < Platform::NUM_ACTS_PER_PLATFORM; i++)
    {
        targetLengths[i] = currentLengths[i] + deltaLengths[i];
    }

    currentLengths = moveToLengths(targetLengths);

    std::array<float, Platform::NUM_ACTS_PER_PLATFORM> distancesFromTargets{};
    for (int i = 0; i < Platform::NUM_ACTS_PER_PLATFORM; i++)
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

bool Platform::initialize()
{
    DEBUG_MSG("Initializing Platform class object...");
    loadCBCParameters();
    updateState();

    return true;
}

void Platform::clearActuatorErrors() {
    for (auto &act : m_Actuators) {
        act->clearErrors();
    }
    for (int i = 0; i < 12; i++)
        unsetError(i);
    updateState();
}

void Platform::clearPlatformErrors()
{
    DEBUG_MSG("Clearing all Platform errors.");
    for (int i = 12; i < getNumErrors(); i++)
    {
        unsetError(i);
    }
    updateState(); // Update error status to reflect actuator errors
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
        std::cout << "Invalid direction given for findHomeFromEndStop." << std::endl;
    }
}

bool Platform::addMPES(Device::Identity identity)
{
    DEBUG_MSG("Adding MPES " << identity.serialNumber << " at USB " << identity.eAddress);
    if (identity.serialNumber < 0 || std::stoi(identity.eAddress) < 0) {
        std::cout << "Platform:: Failed to add MPES, invalid USB/serial numbers (" << identity.eAddress << ", " << identity.serialNumber << ")"
                  << std::endl;
        return false;
    }

    #ifndef SIMMODE
    std::unique_ptr<MPES> newMPES = std::unique_ptr<MPES>(new MPES(m_pCBC, identity));
    #else
    std::unique_ptr<MPES> newMPES = std::unique_ptr<MPES>(new DummyMPES(m_pCBC, identity));
    #endif

    if (newMPES->initialize()) {
        m_MPES.push_back(std::move(newMPES));
        m_MPESIdentityMap.insert(std::make_pair(identity, m_MPES.size() - 1));
        return true;
    }
    else {
        std::cout << "Platform:: Failed to initialize MPES at USB " << identity.eAddress << std::endl;
        return false;
    }
}

MPES::Position Platform::readMPES(int idx)
{
    DEBUG_MSG("Reading MPES.");
    m_MPES.at(idx)->updatePosition();
    return m_MPES.at(idx)->getPosition();
}
