#include "common/alignment/platform.hpp"

#include <sstream>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <random>
#include <algorithm>
#include <unistd.h>

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

#include "common/alignment/device.hpp"
#include "common/alignment/mpes.hpp"
#include "common/alignment/actuator.hpp"
#include "common/globalalignment/psdclass.hpp"

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
    {"Attempt to move out of Software Range", Device::ErrorState::FatalError},
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

    spdlog::info("{} : Creating Platform Object...", m_Identity);
    m_Errors.assign(getNumErrors(), false);
    if (!dbInfo.empty()) {
        setDBInfo(dbInfo);
    } else {
        spdlog::warn("Platform::Platform(): No DB info provided...");
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
        spdlog::error("{} : Platform::step() : Platform is off, motion aborted.", m_Identity);
        return inputSteps;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> stepsRemaining = __step(inputSteps);

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
        spdlog::error("{} : Platform::probeEndStopAll() : Encountered fatal error before starting. Motion aborted.",
                      m_Identity);
        return;
    }
    if (getDeviceState() == Device::DeviceState::Off) {
        spdlog::error("{} : Platform::probeEndStopAll() : Platform is off, motion aborted.", m_Identity);
        return;
    }

    if (direction != 1 && direction != -1) {
        spdlog::error(
            "{} : Platform::probeEndStopAll(): Invalid choice of direction {} (must be 1 for extend or -1 for retract).",
            direction);
        return;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    __probeEndStopAll(direction);
}

std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> PlatformBase::measureLengths()//public
{
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> currentLengths = __measureLengths();
    return currentLengths;
}

std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> PlatformBase::__measureLengths() {
    spdlog::debug("{} : Platform::measureLengths() : Measuring lengths of all actuators...", m_Identity);
    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> currentLengths{};
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        currentLengths[i] = m_Actuators.at(i)->measureLength();
    }
    return currentLengths;
}

std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM>
PlatformBase::moveToLengths(std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> targetLengths) {
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);

    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> currentLengths = __measureLengths();
    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> deltaLengths{};
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> stepsToTake{};

    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        deltaLengths[i] = targetLengths[i] - currentLengths[i];
        stepsToTake[i] = std::floor((deltaLengths[i] / (m_Actuators.at(i)->mmPerStep)) + 0.5);
    }
    __step(stepsToTake);
    currentLengths = __measureLengths();
    return currentLengths;
}

std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM>
PlatformBase::moveDeltaLengths(std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> deltaLengths) {
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> stepsToTake{};

    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        stepsToTake[i] = std::floor((deltaLengths[i] / (m_Actuators.at(i)->mmPerStep)) + 0.5);
    }
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> stepsRemaining = __step(stepsToTake);

    std::array<float, PlatformBase::NUM_ACTS_PER_PLATFORM> distancesFromTargets{};
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        distancesFromTargets[i] = stepsRemaining[i] * m_Actuators.at(i)->mmPerStep;
    }
    return distancesFromTargets;
}

bool PlatformBase::initialize() {
    spdlog::debug("{} : Initializing Platform object...", m_Identity);
    loadCBCParameters();
    return true;
}

bool PlatformBase::isOn() {
    return m_On;
}

void PlatformBase::emergencyStop() {
    spdlog::warn("{} : Platform::emergencyStop(): Stopping all actuator motion...", m_Identity);
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
            if (getErrorCodeDefinition(i).severity > state) {
                state = getErrorCodeDefinition(i).severity;
            }
        }
    }

    return state;
}

void PlatformBase::unsetError(int errorCode) {
    if (errorCode >= 0 && errorCode < 12) {
        spdlog::info("{} : Clearing corresponding errors for Actuator {}.", m_Identity, (errorCode / 2) + 1);
        m_Actuators.at(errorCode / 2)->clearErrors();
    }
    if (m_Errors.at(errorCode)) {
        spdlog::info("{} : Unsetting Error {} ({})", m_Identity, errorCode,
                     getErrorCodeDefinition(errorCode).description);
        m_Errors[errorCode] = false;
    }
}

void PlatformBase::clearActuatorErrors() {
    spdlog::info("{} : Clearing all Actuator-level errors for Platform...", m_Identity);
    for (int i = 0; i < 2 * PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        unsetError(i);
    }
}

void PlatformBase::clearPlatformErrors() {
    spdlog::info("{} : Clearing all Platform-level errors for Platform...", m_Identity);
    for (int i = 2 * PlatformBase::NUM_ACTS_PER_PLATFORM; i < getNumErrors(); i++) {
        unsetError(i);
    }
}

MPESBase::Position PlatformBase::readMPES(int idx) {
    spdlog::info("{} : Reading MPES {}...", m_Identity, idx);
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
        spdlog::error(
            "{} : Platform::loadCBCParameters() : Failed to load CBC Parameters from DB since no CBC Serial given.",
            m_Identity);
        return false;
    }
    spdlog::debug("{} : Platform::loadCBCParameters() : Loading CBC Parameters for Controller Board {}.", m_Identity,
                  getSerialNumber());
    if (m_DBInfo.empty())
    {
        spdlog::error("{} : Operable Error (13): Failed to load CBC parameters from DB because no DB info provided.",
                      m_Identity);
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
        sql::ResultSetMetaData *resmeta;

        driver = get_driver_instance();
        std::string dbAddress = "tcp://" + m_DBInfo.host + ":" + m_DBInfo.port;
        con = driver->connect(dbAddress, m_DBInfo.user, m_DBInfo.password);
        con->setSchema(m_DBInfo.dbname);
        stmt = con->createStatement();

        std::stringstream stmtvar;
        stmtvar << "SELECT * FROM Opt_ControllerBoardCalibration WHERE end_date is NULL and serial_number="
                << getSerialNumber()
                << " ORDER BY start_date DESC LIMIT 1";
        stmt->execute(stmtvar.str());
        res = stmt->getResultSet();
        
	resmeta = res->getMetaData();
        //check if number of results match what is expected. if not, set error(3)
        if (resmeta->getColumnCount() != NUM_DB_CB_COLUMNS) {
            spdlog::error(
                "{} : Operable Error (14): Number of columns in DB table ({}) did not equal the number expected ({}). DB table appears to have an incorrect structure.",
                m_Identity, resmeta->getColumnCount(), NUM_DB_CB_COLUMNS);
            setError(14);
            return false;
            }
        
	if (res->next())
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
	else {
            spdlog::error(
                "{} : Operable Error (14): No calibration data found in the CBC Calibration Table.",
                m_Identity);
            setError(14);
	    return false;
	}

        delete res;
        delete stmt;
        delete con;

    }
    catch (sql::SQLException &e)
    {
        spdlog::error("# ERR: SQLException in {}"
                      "({}) on line {}\n"
                      "# ERR: {}"
                      " (MySQL error code: {}"
                      ", SQLState: {})", __FILE__, __FUNCTION__, __LINE__, e.what(), e.getErrorCode(), e.getSQLState());
        spdlog::error(
            "{} : Operable Error (14): SQL Exception caught for platform. Did not successfully communicate with database.",
            m_Identity);
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

    spdlog::debug("{} : Platform::step() : Stepping platform ({}, {}, {}, {}, {}, {}) steps.",
                  m_Identity, inputSteps[0], inputSteps[1], inputSteps[2], inputSteps[3], inputSteps[4], inputSteps[5]);
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
            spdlog::error(
                "{} : Platform::step() : Operable Error (12): Attempting to move Actuator {} {}-{} mm outside of software range.",
                m_Identity, m_Actuators[i]->getIdentity(),
                m_Actuators[i]->HomeLength -
                (m_Actuators[i]->RetractRevolutionLimit *
                 m_Actuators[i]->StepsPerRevolution *
                 m_Actuators[i]->mmPerStep),
                m_Actuators[i]->HomeLength -
                (m_Actuators[i]->ExtendRevolutionLimit *
                 m_Actuators[i]->StepsPerRevolution *
                 m_Actuators[i]->mmPerStep));
            setError(12);

            return inputSteps;
        }

        if (StepsRemaining[i] != 0) {
            ActuatorIterations[i] = 1 + ((std::abs(StepsRemaining[i]) - 1) /
                                         m_Actuators[i]->RecordingInterval);//simply integer division rounded up, hence the -1.
        }
    }

    spdlog::debug("{} : Platform::step() : Actuator iterations: ({}, {}, {}, {}, {}, {}).",
                  m_Identity, ActuatorIterations[0], ActuatorIterations[1], ActuatorIterations[2],
                  ActuatorIterations[3], ActuatorIterations[4], ActuatorIterations[5]);

    int IterationsRemaining = *std::max_element(ActuatorIterations.begin(), ActuatorIterations.end());
    std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> StepsToTake = {0, 0, 0, 0, 0, 0};
    int Sign;
    int StepsMissed;

    m_pCBC->driver.enableAll();
    while (IterationsRemaining > 1) {
        spdlog::trace("{} : Platform::step() : Iterations remaining: {}",
                      m_Identity, IterationsRemaining);
        for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
            if (ActuatorIterations[i] > 1) {
                if (getDeviceState() == Device::DeviceState::Off || getErrorState() == Device::ErrorState::FatalError) {
                    m_pCBC->driver.disableAll();
                    spdlog::warn("{} : Platform::step() : Successfully stopped motion.", m_Identity);
                    return StepsRemaining;
                }
                StepsToTake[i] = StepsToTake[i] + StepsRemaining[i] / IterationsRemaining;
                spdlog::trace("{} : Platform::step() : Actuator {} stepsToTake = {}.", m_Identity,
                              m_Actuators[i]->getIdentity(), StepsToTake[i]);
                if (std::abs(StepsToTake[i]) > m_Actuators[i]->RecordingInterval) {
                    Sign = (StepsToTake[i] > 0) - (StepsToTake[i] < 0);
                    StepsMissed = m_Actuators[i]->step(Sign * m_Actuators[i]->RecordingInterval);
                    StepsToTake[i] = StepsToTake[i] - (Sign * m_Actuators[i]->RecordingInterval) + StepsMissed;
                    StepsRemaining[i] = -(m_Actuators[i]->convertPositionToSteps(FinalPosition[i]) -
                                          m_Actuators[i]->convertPositionToSteps(m_Actuators[i]->getCurrentPosition()));
                    ActuatorIterations[i] = 1 + ((std::abs(StepsRemaining[i]) - 1) / m_Actuators[i]->RecordingInterval);
                    spdlog::trace("{} : Platform::step() : Steps remaining for actuator {} : {}", m_Identity,
                                  m_Actuators[i]->getIdentity(), StepsRemaining[i]);
                }
            }
        }
        IterationsRemaining = *std::max_element(ActuatorIterations.begin(), ActuatorIterations.end());
    }

    //Hysteresis Motion
    if (getErrorState() != Device::ErrorState::FatalError) {
        spdlog::debug("{} : Platform::step() : Executing hysteresis motion...", m_Identity);
        for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
            if (getDeviceState() == Device::DeviceState::Off || getErrorState() == Device::ErrorState::FatalError) {
                m_pCBC->driver.disableAll();
                spdlog::warn("{} : Platform::step() : Successfully stopped hysteresis motion.", m_Identity);
                return StepsRemaining;
            }
            if (StepsRemaining[i] != 0) {
                m_Actuators[i]->performHysteresisMotion(StepsRemaining[i]);
                StepsRemaining[i] = -(m_Actuators[i]->convertPositionToSteps(FinalPosition[i]) -
                                      m_Actuators[i]->convertPositionToSteps(m_Actuators[i]->getCurrentPosition()));
            }

        }
    }

    m_pCBC->driver.disableAll();

    spdlog::debug("{} : Platform::step() : Finished stepping.", m_Identity);

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
                    spdlog::info("{} : Platform::probeEndStopAll() : Successfully stopped motion.", m_Identity);
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
        spdlog::error(
            "{} : Platform::findHomeFromEndStopAll() : Encountered fatal error before starting. Motion aborted.",
            m_Identity);
        return;
    }
    if (getDeviceState() == Device::DeviceState::Off) {
        spdlog::error("{} : Platform::findHomeFromEndStopAll() : Platform is off. Motion aborted.", m_Identity);
        return;
    }

    if (direction != 1 && direction != -1) {
        spdlog::error(
            "{} : Platform::findHomeFromEndStopAll() : Invalid choice of direction {} (must be 1 for extend or -1 for retract.).",
            m_Identity, direction);
        return;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    __probeEndStopAll(direction);

    if (getErrorState() == Device::ErrorState::FatalError) {
        spdlog::error(
            "{} : Platform::findHomeFromEndStopAll() : Failed to reach end stop due to fatal error in probeEndStopAll(). Find home aborted.",
            m_Identity);
        return;
    }
    else if (getDeviceState() == Device::DeviceState::Off) {
        spdlog::error("{} : Platform::findHomeFromEndStopAll() : Platform is off. Find home aborted.", m_Identity);
        return;
    }

    m_pCBC->driver.enableAll();
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++)
    {
        m_Actuators[i]->findHomeFromEndStop(direction);
    }
    m_pCBC->driver.disableAll();
}

bool Platform::probeHomeAll()
{
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    spdlog::info("{} : Platform : Probing Home for all Actuators...", m_Identity);

    __probeEndStopAll(1);
    m_pCBC->driver.enableAll();
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++)
    {
        m_Actuators.at(i)->probeHome();
    }
    m_pCBC->driver.disableAll();

    return true;
}

float Platform::getInternalTemperature()
{
    spdlog::info("{} : Platform :: Reading Internal Temperature...", m_Identity);
    CBC::ADC::adcData temperatureADCData = m_pCBC->adc.readTemperatureVolts();
    return (m_InternalTemperatureSlope * temperatureADCData.voltage) + m_InternalTemperatureOffset;
}

float Platform::getExternalTemperature()
{
    spdlog::info("{} : Platform :: Reading External Temperature...", m_Identity);
    CBC::ADC::adcData temperatureADCData = m_pCBC->adc.readExternalTemp();
    return (m_ExternalTemperatureSlope * temperatureADCData.voltage) + m_ExternalTemperatureOffset;
}

void Platform::enableHighCurrent()
{
    spdlog::info("{} : Platform :: Enabling high current...", m_Identity);
    m_HighCurrent=true;
    m_pCBC->driver.enableHighCurrent();
}

void Platform::disableHighCurrent()
{
    spdlog::info("{} : Platform :: Disabling high current...", m_Identity);
    m_HighCurrent=false;
    m_pCBC->driver.disableHighCurrent();
}

void Platform::enableSynchronousRectification()//public
{
    spdlog::info("{} : Platform :: Enabling SR Mode...", m_Identity);
    m_SynchronousRectification=true;
    m_pCBC->driver.enableSR();
}

void Platform::disableSynchronousRectification()//public
{
    spdlog::info("{} : Platform :: Disabling SR Mode...", m_Identity);
    m_SynchronousRectification=false;
    m_pCBC->driver.disableSR();
}

void Platform::turnOn() {
    spdlog::info("{} : Platform :: Turning on power to platform...", m_Identity);
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    m_pCBC->powerUp();
    for (const auto& pMPES : m_MPES) {
    	pMPES->turnOn();
    }
    for (const auto& pActuator : m_Actuators) {
    	pActuator->turnOn();
    }
    m_On = true;
}

void Platform::turnOff() {
    spdlog::info("{} : Platform :: Turning off power to platform...", m_Identity);
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    m_pCBC->powerDown();
    m_On = false;
}

bool Platform::addMPES(const Device::Identity &identity)
{
    spdlog::info("{} : Platform::addMPES() : Adding MPES {} at USB {}.", m_Identity, identity, identity.eAddress);
    if (identity.serialNumber < 0 || std::stoi(identity.eAddress) < 0) {
        spdlog::error("{} : Platform::addMPES() : Failed to add MPES {}, invalid USB/serial number.", m_Identity,
                      identity);
        return false;
    }

    std::unique_ptr<MPES> newMPES = std::unique_ptr<MPES>(new MPES(m_pCBC, identity));

    if (newMPES->initialize()) {
        m_MPES.push_back(std::move(newMPES));
        m_MPESIdentityMap.insert(std::make_pair(identity, m_MPES.size() - 1));
        return true;
    }
    else {
        m_MPES.push_back(std::move(newMPES));
        m_MPESIdentityMap.insert(std::make_pair(identity, m_MPES.size() - 1));
        spdlog::warn("{} : Platform::addMPES() : Failed to initialize MPES {} at USB {}.", m_Identity, identity,
                     identity.eAddress);
        return false;
    }
}

bool Platform::addPSD(const Device::Identity &identity)
{
    spdlog::info("{} : Platform::addPSD() : Adding PSD {} at USB {}.", m_Identity, identity, identity.eAddress);
    if (identity.serialNumber < 0 || std::stoi(identity.eAddress) < 0) {
        spdlog::error("{} : Platform::addPSD() : Failed to add PSD {}, invalid USB/serial number.", m_Identity,
                      identity);
        return false;
    }

    std::unique_ptr<GASPSD> newPSD = std::unique_ptr<GASPSD>(new GASPSD(identity));
    
    if (newPSD->initialize()) {
        m_PSD.push_back(std::move(newPSD));
        m_PSDIdentityMap.insert(std::make_pair(identity, m_PSD.size() - 1));
        return true;
    }
    else {
        m_PSD.push_back(std::move(newPSD));
        m_PSDIdentityMap.insert(std::make_pair(identity, m_PSD.size() - 1));
        spdlog::warn("{} : Platform::addPSD() : Failed to initialize PSD {} at USB {}.", m_Identity, identity,
                     identity.eAddress);
        return false;
    }
}

#endif

std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM>
DummyPlatform::__step(std::array<int, PlatformBase::NUM_ACTS_PER_PLATFORM> inputSteps) {

    spdlog::debug("{} : DummyPlatform::step() : Stepping platform ({}, {}, {}, {}, {}, {}) steps.",
                  m_Identity, inputSteps[0], inputSteps[1], inputSteps[2], inputSteps[3], inputSteps[4], inputSteps[5]);
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
            spdlog::error(
                "{} : DummyPlatform::step() : Operable Error (12): Attempting to move Actuator {} {}-{} mm outside of software range.",
                m_Identity, m_Actuators[i]->getIdentity(),
                m_Actuators[i]->HomeLength -
                (m_Actuators[i]->RetractRevolutionLimit *
                 m_Actuators[i]->StepsPerRevolution *
                 m_Actuators[i]->mmPerStep),
                m_Actuators[i]->HomeLength -
                (m_Actuators[i]->ExtendRevolutionLimit *
                 m_Actuators[i]->StepsPerRevolution *
                 m_Actuators[i]->mmPerStep));
            setError(12);
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
                    spdlog::info("{} : DummyPlatform::step() : Successfully stopped motion.", m_Identity);
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
                    spdlog::trace("{} : DummyPlatform::step() : Steps remaining for Actuator {} : {}.", m_Identity,
                                  m_Actuators[i]->getIdentity(), StepsRemaining[i]);
                }
            }
        }
        IterationsRemaining = *std::max_element(ActuatorIterations.begin(), ActuatorIterations.end());
    }

    //Hysteresis Motion
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        if (getDeviceState() == Device::DeviceState::Off || getErrorState() == Device::ErrorState::FatalError) {
            spdlog::info("{} : DummyPlatform::step() : Successfully stopped motion.", m_Identity);
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
    spdlog::info(
        "{} : DummyPlatform::findHomeFromEndStopAll() : probeEndStopAll() : Probing end stop in direction {} (nothing should happen)...\"",
        m_Identity, direction);
}

void DummyPlatform::findHomeFromEndStopAll(int direction) {
    if (getErrorState() == Device::ErrorState::FatalError) {
        spdlog::error(
            "{} : DummyPlatform::findHomeFromEndStopAll() : Encountered fatal error before starting. Motion aborted.",
            m_Identity);
        return;
    }
    if (getDeviceState() == Device::DeviceState::Off) {
        spdlog::error("{} : DummyPlatform::findHomeFromEndStopAll() : Platform is off. Motion aborted.", m_Identity);
        return;
    }

    if (direction != 1 && direction != -1) {
        spdlog::error(
            "{} : DummyPlatform::findHomeFromEndStopAll() : Invalid choice of direction {} (must be 1 for extend or -1 for retract.)",
            m_Identity, direction);
        return;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    __probeEndStopAll(direction);

    if (getErrorState() == Device::ErrorState::FatalError) {
        spdlog::error(
            "{} : DummyPlatform::findHomeFromEndStopAll() : Failed to reach end stop due to fatal error in probeEndStopAll(). Find home aborted.",
            m_Identity);
        return;
    } else if (getDeviceState() == Device::DeviceState::Off) {
        spdlog::error("{} : DummyPlatform::findHomeFromEndStopAll() : Platform is off. Find home aborted.", m_Identity);
        return;
    }

    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        m_Actuators[i]->findHomeFromEndStop(direction);
    }
}

bool DummyPlatform::probeHomeAll() {
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    spdlog::info("{} : DummyPlatform :: Probing Home for All Actuators", m_Identity);
    __probeEndStopAll(1);
    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        m_Actuators.at(i)->probeHome();
    }
    return true;
}

float DummyPlatform::getInternalTemperature() {
    spdlog::trace(
        "{} : DummyPlatform:: Reading Internal Temperature (will return normally distributed value w/ mean 20.0, stddev 2.0)",
        m_Identity);
    std::random_device rd{};
    std::mt19937 generator{rd()};

    std::normal_distribution<double> internalTempDistribution(20.0, 2.0);

    double newValue;
    newValue = internalTempDistribution(generator);
    return newValue;
}

float DummyPlatform::getExternalTemperature() {
    spdlog::trace(
        "{} : DummyPlatform:: Reading External Temperature (will return normally distributed value w/ mean 30.0, stddev 2.0)",
        m_Identity);
    std::random_device rd{};
    std::mt19937 generator{rd()};

    std::normal_distribution<double> externalTempDistribution(30.0, 2.0);

    double newValue;
    newValue = externalTempDistribution(generator);
    return newValue;
}

void DummyPlatform::enableHighCurrent() {
    spdlog::info("{} : DummyPlatform :: Enabling High Current", m_Identity);
    m_HighCurrent = true;
}

void DummyPlatform::disableHighCurrent() {
    spdlog::info("{} : DummyPlatform :: Disabling High Current", m_Identity);
    m_HighCurrent = false;
}

void DummyPlatform::enableSynchronousRectification()//public
{
    spdlog::info("{} : DummyPlatform :: Setting SR Mode", m_Identity);
    m_SynchronousRectification = true;
}

void DummyPlatform::disableSynchronousRectification()//public
{
    spdlog::info("{} : DummyPlatform :: Disabling SR Mode", m_Identity);
    m_SynchronousRectification = false;
}

void DummyPlatform::turnOn() {
    spdlog::info("{} : DummyPlatform::turnOn() : Turning on power to platform...", m_Identity);
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    for (const auto &pMPES : m_MPES) {
        pMPES->turnOn();
    }
    for (const auto &pActuator : m_Actuators) {
        pActuator->turnOn();
    }
    m_On = true;
}

void DummyPlatform::emergencyStop() {
    spdlog::warn("{} : DummyPlatform::emergencyStop() : Emergency stopping all actuator motion...", m_Identity);
    m_On = false; // We temporarily set the state to off to indicate that the motion should be stopped.
    sleep(1);
    m_On = true;
}

void DummyPlatform::turnOff() {
    spdlog::info("{} : DummyPlatform::turnOff() : Turning off power to platform...", m_Identity);
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    m_On = false;
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
    spdlog::info("{} : DummyPlatform::addMPES() : Adding DummyMPES {} at USB {}.", m_Identity, identity,
                 identity.eAddress);
    if (identity.serialNumber < 0 || std::stoi(identity.eAddress) < 0) {
        spdlog::error("{} : DummyPlatform::addMPES() : Failed to add DummyMPES {}, invalid USB/serial number.",
                      m_Identity, identity);
        return false;
    }

    std::unique_ptr<MPESBase> newMPES = std::unique_ptr<MPESBase>(new DummyMPES(identity));

    if (newMPES->initialize()) {
        m_MPES.push_back(std::move(newMPES));
        m_MPESIdentityMap.insert(std::make_pair(identity, m_MPES.size() - 1));
        return true;
    } else {
        m_MPES.push_back(std::move(newMPES));
        m_MPESIdentityMap.insert(std::make_pair(identity, m_MPES.size() - 1));
        spdlog::warn("{} : DummyPlatform::addMPES() : Failed to initialize DummyMPES {} at USB {}.", m_Identity,
                     identity, identity.eAddress);
        return false;
    }
}

bool DummyPlatform::addPSD(const Device::Identity &identity) {
    spdlog::info("{} : DummyPlatform::addPSD() : Adding DummyGASPSD {} at USB {}.", m_Identity, identity,
                 identity.eAddress);
    if (identity.serialNumber < 0 || std::stoi(identity.eAddress) < 0) {
        spdlog::error("{} : DummyPlatform::addPSD() : Failed to add DummyGASPSD {}, invalid USB/serial number.",
                      m_Identity, identity);
        return false;
    }

    std::unique_ptr<GASPSD> newPSD = std::unique_ptr<GASPSD>(new DummyGASPSD(identity));

    if (newPSD->initialize() == true) {
        m_PSD.push_back(std::move(newPSD));
        m_PSDIdentityMap.insert(std::make_pair(identity, m_PSD.size() - 1));
        return true;
    } else {
        m_PSD.push_back(std::move(newPSD));
        m_PSDIdentityMap.insert(std::make_pair(identity, m_PSD.size() - 1));
        spdlog::warn("{} : DummyPlatform::addPSD() : Failed to initialize DummyGASPSD {} at USB {}.", m_Identity,
                     identity, identity.eAddress);
        return false;
    }
}

bool DummyPlatform::loadCBCParameters() {
    if (getSerialNumber() == 0) {
        spdlog::error(
            "{} : DummyPlatform::loadCBCParameters() : Failed to load CBC Parameters from DB since no CBC Serial given.",
            m_Identity);
        return false;
    }
    spdlog::debug("{} : DummyPlatform::loadCBCParameters() : Loading CBC Parameters for Controller Board {}.",
                  m_Identity, getSerialNumber());
    if (m_DBInfo.empty()) {
        spdlog::error("{} : Operable Error (13): Failed to load CBC parameters from DB because no DB info provided.",
                      m_Identity);
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
        stmtvar << "SELECT * FROM Opt_ControllerBoardCalibration WHERE end_date is NULL and serial_number="
                << getSerialNumber()
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
        spdlog::error("# ERR: SQLException in {}"
                      "({}) on line {}\n"
                      "# ERR: {}"
                      " (MySQL error code: {}"
                      ", SQLState: {})", __FILE__, __FUNCTION__, __LINE__, e.what(), e.getErrorCode(), e.getSQLState());
        spdlog::error("{} : Operable Error (14): SQL communication error.", m_Identity);
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



