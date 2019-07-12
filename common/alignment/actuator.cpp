#include "common/alignment/actuator.hpp"

#include <cmath>
#include <fstream>
#include <memory>
#include <sstream>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <mysql_connection.h>
#include <mysql_driver.h>

#include "common/alignment/platform.hpp"
#include "common/alignment/device.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"


const std::vector<Device::ErrorDefinition> ActuatorBase::ERROR_DEFINITIONS = {
    {"Home position is not calibrated",                                                                                                                           Device::ErrorState::FatalError},//error 0
    {"DBInfo not set",                                                                                                                                            Device::ErrorState::OperableError},//error 1
    {"MySQL Communication Error",                                                                                                                                 Device::ErrorState::OperableError},//error 2
    {"DB Columns does not match what is expected",                                                                                                                Device::ErrorState::FatalError},//error 3
    {"ASF File is Bad",                                                                                                                                           Device::ErrorState::FatalError},//error 4
    {"ASF File entries does not match what is expected",                                                                                                          Device::ErrorState::FatalError},//error 5
    {"DB recording more recent than ASF and has mismatch with measured angle",                                                                                    Device::ErrorState::FatalError},//error 6
    {"Voltage Std Dev is entirely too high",                                                                                                                      Device::ErrorState::FatalError},//error 7
    {"Actuator Missed too many steps",                                                                                                                            Device::ErrorState::FatalError},//error 8
    {"Actuator position is too many steps away to recover safely",                                                                                                Device::ErrorState::FatalError},//error 9
    {"Actuator position is recovering large amount of steps, should be ok",                                                                                       Device::ErrorState::OperableError},//error 10
    {"Extend Stop Voltage is too close to the discontinuity. Possible 1 cycle uncertainty with calibrated data",                                                  Device::ErrorState::OperableError},//error 11
    {"End stop is large number of steps away from what is expected. Possible uncertainty in home position",                                                       Device::ErrorState::OperableError},//error 12
    {"Discrepancy between number of steps from extend stop and recorded number of steps from end stop is too high. Possible uncertainty in probed home position", Device::ErrorState::OperableError}//error 13
};

const ActuatorBase::ASFInfo ActuatorBase::EMERGENCY_ASF_INFO = {"/.ASF/", ".ASF_Emergency_Port_", ".log"};

ActuatorBase::ActuatorBase(Device::Identity identity, Device::DBInfo DBInfo,
                           const ASFInfo &ASFFileInfo) : Device::Device(std::move(identity)),
                                                         m_keepStepping(true) {
    m_Errors.assign(getNumErrors(), false);
    setError(0); // By default, home position not calibrated
    setError(1); // By default, DB info not set

    if (getSerialNumber() == -1) {
        m_Identity.serialNumber = std::stoi(m_Identity.eAddress);
        setASFInfo(EMERGENCY_ASF_INFO);
        spdlog::warn("{} : Actuator: No serial number passed, creating emergency ASF.", m_Identity);
    } else {
        setASFInfo(ASFFileInfo);
    }
    if (!DBInfo.empty()) {
        setDBInfo(DBInfo);
    } else {
        spdlog::warn("{} : Actuator: No DB info provided.", m_Identity);
    }

    m_encoderScale.resize(StepsPerRevolution);
    for (int i = 0; i < StepsPerRevolution; i++) {
        m_encoderScale[i] = m_VMin + (i * dV);
    }
}

bool ActuatorBase::loadConfigurationAndCalibration() {
    spdlog::debug(
        "{} : ActuatorBase::loadConfigurationAndCalibration(): Reading Configuration and Calibration Information from DB...",
        m_Identity);
    //check to make sure number of columns match what is expected.
    if (!m_Errors[1]) {
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
            stmtvar << "SELECT * FROM Opt_ActuatorConfigurationAndCalibration WHERE serial_number=" << getSerialNumber()
                    << " ORDER BY start_date DESC LIMIT 1";
            stmt->execute(stmtvar.str());
            res = stmt->getResultSet();
            while (res->next()) {
                mmPerStep = res->getDouble(4);
                StepsPerRevolution = res->getInt(5);
                HomeLength = res->getDouble(6);
                m_RetractStopPosition.revolution = res->getInt(7);
                m_RetractStopPosition.angle = res->getInt(8);
                m_ExtendStopPosition.revolution = res->getInt(9);
                m_ExtendStopPosition.angle = res->getInt(10);
                RecordingInterval = res->getInt(11);
                m_CalibrationTemperature = res->getDouble(12);
                m_HysteresisSteps = res->getInt(13);
                RetractRevolutionLimit = res->getInt(14);
                ExtendRevolutionLimit = res->getInt(15);
                m_MaxVoltageMeasurementAttempts = res->getInt(16);
                m_StdDevRemeasure = res->getDouble(17);
                m_StdDevMax = res->getDouble(18);
                m_QuickAngleCheckRange = res->getInt(19);
                EndstopSearchStepsize = res->getInt(20);
                m_CyclesDefiningHome = res->getInt(21);
                m_MinimumMissedStepsToFlagError = res->getInt(22);
                m_TolerablePercentOfMissedSteps = res->getDouble(23);
                m_ExtendStopToHomeStepsDeviation = res->getInt(24);
                m_FlaggedRecoverySteps = res->getInt(25);
                m_MaxRecoverySteps = res->getInt(26);
                m_EndStopRecoverySteps = res->getInt(27);
            }
            m_encoderScale.resize(StepsPerRevolution);
            for (int i = 0; i < StepsPerRevolution; i++) {
                stmtvar.str(std::string());
                stmtvar << "SELECT * FROM Opt_ActuatorMotorProfile WHERE (serial_number=" << getSerialNumber()
                        << " and angle=" << i << ") ORDER BY start_date DESC LIMIT 1";
                stmt->execute(stmtvar.str());
                res = stmt->getResultSet();
                while (res->next()) {
                    m_encoderScale[i] = res->getDouble(5);
                }
            }
            m_VMin = m_encoderScale[0];
            m_VMax = m_encoderScale[StepsPerRevolution - 1];
            dV = (m_VMax - m_VMin) / (StepsPerRevolution - 1);
            delete res;
            delete stmt;
            delete con;
        }
        catch (sql::SQLException &e) {
            spdlog::error("# ERR: SQLException in {}"
                          "({}) on line {}\n"
                          "# ERR: {}"
                          " (MySQL error code: {}"
                          ", SQLState: {})", __FILE__, __FUNCTION__, __LINE__, e.what(), e.getErrorCode(),
                          e.getSQLState());
            spdlog::error("{} : Operable Error (2): SQL Exception, did not successfully communicate with database.",
                          m_Identity);
            setError(2);//operable
            return false;
        }
    } else {
        spdlog::error("{} : Operable Error (1): DBInfo not set, cannot read Configuration and Calibration from DB.",
                      m_Identity);
        setError(1);
        return false;
    }
    saveStatusToASF();
    return true;
}

//read all error codes from ASF. Check size of error codes to make sure version is consistent. Read whether Home is set or not.
bool ActuatorBase::readStatusFromASF(ActuatorStatus &RecordedPosition)
{
    spdlog::trace("{} : Reading Status from ASF File at {}.", m_Identity, m_ASFPath);
    std::ifstream ASF(m_ASFPath);
    if (!ASF.good())//if file does not exist (or possibly other file issues not expected..)
    {
        spdlog::warn("{} : ASF file at path {} was bad. Assuming it did not exist and will create a default ASF file.",
                     m_Identity, m_ASFPath);
        ASF.close();
        createDefaultASF();
        ASF.open(m_ASFPath);
        //if(ASF.bad())//check if ASF is good again. If not, set fatal error.
        if (!ASF.good())//check if ASF is good again. If not, set fatal error.
        {
            spdlog::error(
                "{} : Fatal Error (4): Creating ASF file at {} did not resolve problem. File appears corrupt.",
                m_Identity, m_ASFPath);
            setError(4);//fatal
            return false;
        }
    }
    std::string line;
    int word;
    getline(ASF, line);
    std::vector<int> ASFReadArray;
    ASFReadArray.reserve(NUM_ASF_COLUMNS);
    std::istringstream ss(line);
    while (ss.good()) {
        ss >> word;
        ASFReadArray.push_back(word);
    }
    if ((int) ASFReadArray.size() != NUM_ASF_COLUMNS) {
        spdlog::error(
            "{} : Fatal Error (5): Number of arguments in ASF file ({}) did not equal the number expected ({}). ASF File appears to have an incorrect structure.",
            m_Identity, ASFReadArray.size(), NUM_ASF_COLUMNS);
        setError(5);//fatal
        return false;
    }
    //following is ASF structure, hardcoded.
    RecordedPosition.date.year = ASFReadArray[0];
    RecordedPosition.date.month = ASFReadArray[1];
    RecordedPosition.date.day = ASFReadArray[2];
    RecordedPosition.date.hour = ASFReadArray[3];
    RecordedPosition.date.minute = ASFReadArray[4];
    RecordedPosition.date.second = ASFReadArray[5];
    RecordedPosition.position.revolution = ASFReadArray[6];
    RecordedPosition.position.angle = ASFReadArray[7];

    RecordedPosition.errorCodes.resize(getNumErrors());
    for (int i = 0; i < getNumErrors(); i++) {
        RecordedPosition.errorCodes[i] = ASFReadArray[NUM_ASF_HEADER_COLUMNS + i];
    }
    return true;
}

//read all error codes from ASF. Check size of error codes to make sure version is consistent. Read whether Home is set or not.
void ActuatorBase::loadStatusFromASF()
{
    ActuatorStatus RecordedPosition;
    if (readStatusFromASF(RecordedPosition)) {
        m_CurrentPosition.revolution = RecordedPosition.position.revolution;
        m_CurrentPosition.angle = RecordedPosition.position.angle;
        for (int i = 0; i < getNumErrors(); i++) {
            if (RecordedPosition.errorCodes[i]) {
                setError(i);
            } else {
                unsetError(i);
            }
        }
    }
}

void ActuatorBase::saveStatusToASF()//record all error codes to ASF.
{
    spdlog::trace("{} : Recording actuator status to ASF file at {}...", m_Identity, m_ASFPath);
    copyFile(m_ASFPath, m_OldASFPath);
    std::ofstream ASF(m_NewASFPath);

    if(ASF.bad())//or exist
    {
        spdlog::error("{} : Fatal Error (4): Cannot write to temporary ASF file ({}.log.new). ASF write failed.",
                      m_Identity, getSerialNumber());
        setError(4);//fatal
        return;
    }

    std::time_t now = std::time(0); //0 for UTC., unix timing will overflow in 2038.
    struct tm time = *gmtime(&now);
    ASF << time.tm_year + 1900 << " " << time.tm_mon + 1 << " " << time.tm_mday << " " << time.tm_hour << " "
        << time.tm_min << " " << time.tm_sec << " ";
    ASF << m_CurrentPosition.revolution << " " << m_CurrentPosition.angle;
    for (int i = 0; i < getNumErrors(); i++) {
        ASF << " " << (int)m_Errors[i];
    }
    ASF.close();
    copyFile(m_NewASFPath, m_ASFPath);
}

float ActuatorBase::readVoltage() {
    if (isBusy()) {
        spdlog::error("{} : Actuator::readVoltage() : Busy, cannot read encoder.", m_Identity);
        return 0.0;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    float voltage;
    voltage = __readVoltage();
    return voltage;
}

int ActuatorBase::readAngle() {
    float voltage = __readVoltage();
    //find minimum deviation from measured voltage and array of voltages. index of this array is the angle.
    float MinimumDifference = std::fabs(voltage - m_encoderScale[0]);
    int index = 0;
    float Deviation;
    for (int i = 1; i < StepsPerRevolution; i++) {
        Deviation = std::fabs(voltage - m_encoderScale[i]);
        if (Deviation < MinimumDifference) {
            MinimumDifference = Deviation;
            index = i;
        }
    }
    return index;
}

int ActuatorBase::step(int steps)//Positive Step is Extension of Motor
{
    if (getErrorState() == Device::ErrorState::FatalError)//don't move actuator if there's a fatal error.
    {
        spdlog::error("{} : Actuator::step() : Encountered fatal error before starting. Motion aborted.", m_Identity);
        return steps;
    }

    if (getDeviceState() == Device::DeviceState::Off) {
        spdlog::error("{} : Actuator::step() : Actuator is off, motion aborted. This should only occur when requesting "
                      " actuator motion from an individual actuator (not through the panel/platform). For actuators "
                      " attached to a platform this is unsafe and should not be done (instead, call step through the platform/panel)"
                      ". However, if you wish to step an isolated actuator, you can do so by manually powering on this actuator first.",
                      m_Identity);
        return steps;
    }

    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    int stepsRemaining = __step(steps);
    return stepsRemaining;
}

float ActuatorBase::measureLength() {
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    float currentLength = __measureLength();
    return currentLength;
}

float ActuatorBase::moveToLength(float targetLength) {
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);

    float currentLength = __measureLength();
    float lengthToMove = targetLength - currentLength;
    int stepsToMove = std::floor((lengthToMove / mmPerStep) + 0.5);
    __step(stepsToMove);
    currentLength = __measureLength();
    return currentLength;
}

float ActuatorBase::moveDeltaLength(float lengthToMove) {
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    int stepsToMove = std::floor((lengthToMove / mmPerStep) + 0.5);
    int stepsRemaining = __step(stepsToMove);
    float lengthRemaining = stepsRemaining * mmPerStep;
    return lengthRemaining;
}

ActuatorBase::Position ActuatorBase::predictNewPosition(Position position,
                                                        int steps)//Positive steps goes in positive direction of counter, which is *retraction* of actuator. (0,0 defined in extended state)
{
    Position PredictedPosition{};
    int InputStepsFromHome = convertPositionToSteps(position);
    int PredictedStepsFromHome = InputStepsFromHome + steps;
    PredictedPosition.revolution = PredictedStepsFromHome / StepsPerRevolution;
    PredictedPosition.angle = PredictedStepsFromHome % StepsPerRevolution;
    if (PredictedStepsFromHome < 0)//integer division rounds towards zero, we want floor behavior.
    {
        PredictedPosition.revolution--;
        PredictedPosition.angle += StepsPerRevolution;
    }
    return PredictedPosition;
}

int ActuatorBase::performHysteresisMotion(int steps) {
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);

    spdlog::trace("{} : Performing Hysteresis Motion of {} steps.", m_Identity, steps);
    int stepsRemaining = __step(steps - m_HysteresisSteps);
    stepsRemaining = __step(m_HysteresisSteps + stepsRemaining);

    return stepsRemaining;
}

//Port, Serial, ASFPath, and sometimes DB are loaded. The rest of the loading needs to be designed here. Set Current position
bool ActuatorBase::initialize() {
    spdlog::debug("{} : Initializing actuator...", m_Identity);
    loadStatusFromASF();
    loadConfigurationAndCalibration();
    recoverPosition();
    getErrorState();

    return true;
}

void
ActuatorBase::recoverPosition()//consolidates current position and recovers position (if not too far away). This is typically ran after reading status from ASF.
{
    spdlog::trace("{} : Recovering actuator position...", m_Identity);
    int indexDeviation = checkAngleQuick(m_CurrentPosition);
    if (m_Errors[7])//check for voltage issue
    {
        return;
    }
    if (indexDeviation == 0) {
        return;
    }
    if (std::abs(indexDeviation) < m_FlaggedRecoverySteps) {
        setCurrentPosition(predictNewPosition(m_CurrentPosition, indexDeviation));
        saveStatusToASF();
        return;
    } else if (std::abs(indexDeviation) <
               m_MaxRecoverySteps)//If the difference between where we are and where we think we are is high, set an OperableError.
    {
        spdlog::error(
            "{} : Operable Error (10): Actuator is {} steps away from the last believed position. This number is large enough to warrant an error, but below the set maximum number of recoverable steps ({}). Software will recover the position, but this error should not happen under normal conditions.",
            m_Identity, indexDeviation, m_MaxRecoverySteps);
        setError(10);//operable
        setCurrentPosition(predictNewPosition(m_CurrentPosition, indexDeviation));
        saveStatusToASF();
        return;
    } else { //If the difference between where we are and where we think we are is extremely high, set a FatalError. position is lost. (Do we want to set fatal error?? maybe we just recover and set homeisset=false.
        spdlog::error(
            "{} : Fatal Errors (9, 0): Actuator is {} steps away from the last believed position. This number is above the set maximum number of recoverable steps ({}). Home position will need to be found again.\n"
            "Current measured position: ({},{})\n"
            "Most probable (estimated) position: ({},{})",
            m_Identity, indexDeviation, m_MaxRecoverySteps,
            m_CurrentPosition.revolution, m_CurrentPosition.angle,
            predictNewPosition(m_CurrentPosition, indexDeviation).revolution,
            predictNewPosition(m_CurrentPosition, indexDeviation).angle);
        setError(9);//fatal
        setError(0);
        saveStatusToASF();
        return;
    }
}

void ActuatorBase::recoverStatusFromDB() {
    spdlog::trace("{} : Recovering actuator status from DB...", m_Identity);
    loadStatusFromDB();
    recoverPosition();
    saveStatusToASF();
}

void ActuatorBase::setASFInfo(const ASFInfo &ASFInfo) {
    std::stringstream Path;
    Path << ASFInfo.directory << ASFInfo.prefix << getSerialNumber() << ASFInfo.suffix;
    m_ASFPath = Path.str();
    m_NewASFPath = m_ASFPath + ".new";
    m_OldASFPath = m_ASFPath + ".old";
}

void ActuatorBase::setDBInfo(Device::DBInfo DBInfo) {
    m_DBInfo = std::move(DBInfo);
    unsetError(1);
}

void ActuatorBase::unsetDBInfo() {
    m_DBInfo = Device::DBInfo();
    setError(1);
}

int ActuatorBase::convertPositionToSteps(Position position) {
    return position.revolution * StepsPerRevolution + position.angle;
}

void ActuatorBase::findHomeFromEndStop(int direction)//use recorded extendstop and set actuator to that.
{
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    __findHomeFromEndStop(direction);
}


int ActuatorBase::checkAngleQuick(
    Position ExpectedPosition)//First attempts a quicker, less robust method of determining the current actuator angle. Defers to the slower method if it fails. Returns the number of missed steps. (e.g. returns +2 if expected position is 50,100 but measured angle is 102). This method ends when either range is exhausted (set by independent variable QuickAngleCheckSearchDeviation), or when it finds a voltage deviation less than dV/2. checkAngleSlow has no such requirement, and finds minimum voltage scanning over range of actuator steps.
{
    int ExpectedAngle = ExpectedPosition.angle;
    float MeasuredVoltage = __readVoltage();
    //search the Encoder_Calibration array at the expected angle index, and p/m two indices around this index (making sure we dont go out of range).
    int IndexDeviation = 0;
    float VoltageDeviation;
    VoltageDeviation = std::fabs(MeasuredVoltage - m_encoderScale[ExpectedAngle]);
    if (VoltageDeviation < (dV / 2))//nearest step
    {
        return IndexDeviation;
    }
    IndexDeviation++;
    int EvaluatedIndex;
    while (IndexDeviation <= m_QuickAngleCheckRange) {
        EvaluatedIndex = ExpectedAngle + IndexDeviation;
        if (EvaluatedIndex >= StepsPerRevolution) {
            EvaluatedIndex = EvaluatedIndex % StepsPerRevolution;
        }
        VoltageDeviation = std::fabs(MeasuredVoltage - m_encoderScale[EvaluatedIndex]);
        if (VoltageDeviation < (dV / 2))//nearest step
        {
            return IndexDeviation;
        }

        EvaluatedIndex = ExpectedAngle - IndexDeviation;
        if (EvaluatedIndex < 0) {
            EvaluatedIndex = EvaluatedIndex + StepsPerRevolution;
        }
        VoltageDeviation = std::fabs(MeasuredVoltage - m_encoderScale[EvaluatedIndex]);
        if (VoltageDeviation < (dV / 2))//nearest step
        {
            return -IndexDeviation;
        }
        IndexDeviation++;
    }
    return checkAngleSlow(ExpectedPosition);
}

int ActuatorBase::checkAngleSlow(Position ExpectedPosition) {
    int CurrentAngle = readAngle();
    int IndexDeviation;
    int first_query = std::abs(ExpectedPosition.angle - (CurrentAngle - StepsPerRevolution));
    int second_query = std::abs(ExpectedPosition.angle - CurrentAngle);
    int third_query = std::abs(ExpectedPosition.angle - (CurrentAngle + StepsPerRevolution));
    if ((first_query < second_query) && (first_query < third_query)) {
        IndexDeviation = -(ExpectedPosition.angle - CurrentAngle + StepsPerRevolution);
    } else if ((third_query < first_query) && (third_query < second_query)) {
        IndexDeviation = -(ExpectedPosition.angle - CurrentAngle - StepsPerRevolution);
    } else {
        IndexDeviation = -(ExpectedPosition.angle - CurrentAngle);
    }
    return IndexDeviation;
}

float ActuatorBase::__measureLength() {
    spdlog::trace("{} : Measuring actuator length...", m_Identity);
    loadStatusFromASF();
    recoverPosition();
    int StepsFromHome = convertPositionToSteps(m_CurrentPosition);
    float DistanceFromHome = StepsFromHome * mmPerStep;
    float currentLength = HomeLength - DistanceFromHome;

    return currentLength;
}

void ActuatorBase::probeEndStop(int direction) {
    if (direction != 1 && direction != -1) {
        spdlog::error(
            "{} : Actuator::probeEndStop(): Invalid choice of direction {} (must be 1 for extend or -1 for retract).",
            m_Identity, direction);
        return;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    __probeEndStop(direction);
}

void ActuatorBase::createDefaultASF()//hardcoded structure of the ASF file (year,mo,day,hr,min,sec,rev,angle,errorcodes)
{
    spdlog::trace("{} : Creating default ASF file at {}...", m_Identity, m_ASFPath);
    copyFile(m_ASFPath, m_OldASFPath); // Create a copy of the current ASF file contents (the "old" ASF file)

    std::ofstream ASFfile(m_NewASFPath); // Write new default ASF file contents to a separate (the "new" ASF file)
    ASFfile << "2000 1 1 0 0 0 50 0"; // year month day hour minute second revolution angle
    for (int i = 0; i < getNumErrors(); i++) {
        if (i == 0) {
            ASFfile << " 1"; //set error code 0 to true, meaning home is not found.
        } else {
            ASFfile << " 0";
        }
    }
    ASFfile << std::endl;
    ASFfile.close();

    copyFile(m_NewASFPath, m_ASFPath); // Copy the "new" ASF file to the current ASF file location to overwrite
}

void ActuatorBase::clearErrors() {
    Device::clearErrors();
    saveStatusToASF();
}

bool ActuatorBase::forceRecover() {
    spdlog::debug("{} : Force recovering actuator position...", m_Identity);
    int indexDeviation = checkAngleQuick(m_CurrentPosition);
    if (m_Errors[7])//check for voltage issue
    {
        return false;
    }
    setCurrentPosition(predictNewPosition(m_CurrentPosition, indexDeviation));
    saveStatusToASF();
    return true;
}

void ActuatorBase::emergencyStop() {
    spdlog::warn("{} : Emergency stopping actuator motion...", m_Identity);
    m_keepStepping = false;
}

void ActuatorBase::copyFile(const std::string &srcFilePath, const std::string &destFilePath) {
    std::ifstream srcFile(srcFilePath, std::ios::binary);
    std::ofstream destFile(destFilePath, std::ios::binary);

    destFile << srcFile.rdbuf();

    srcFile.close();
    destFile.close();
}


#ifndef SIMMODE

#include "common/cbccode/cbc.hpp"

//read all error codes from DB. Check size of error codes to make sure version is consistent. Adjust this function to the new database table structure.
bool Actuator::readStatusFromDB(ActuatorStatus &RecordedPosition) {
    //check that the number of columns matches what is expected
    spdlog::trace("{} : Reading actuator status from DB...", m_Identity);
    if (!m_Errors[1]) {
        try {
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
            stmtvar << "SELECT * FROM Opt_ActuatorStatus WHERE serial_number=" << getSerialNumber()
                    << " ORDER BY id DESC LIMIT 1";
            stmt->execute(stmtvar.str());
            res = stmt->getResultSet();
            resmeta = res->getMetaData();
            //check if number of results match what is expected. if not, set error(3)
            if (resmeta->getColumnCount() != NUM_DB_COLUMNS) {
                spdlog::error(
                    "{} : Fatal Error (3): Number of columns in DB table ({}) did not equal the number expected ({}). DB table appears to have an incorrect structure.",
                    m_Identity, resmeta->getColumnCount(), NUM_DB_COLUMNS);
                setError(3);//fatal
                saveStatusToASF();
                return false;
            }

            while (res->next()) {
                std::string date = res->getString(3);
                sscanf(date.c_str(), "%d-%d-%d %d:%d:%d", &RecordedPosition.date.year, &RecordedPosition.date.month,
                       &RecordedPosition.date.day, &RecordedPosition.date.hour, &RecordedPosition.date.minute,
                       &RecordedPosition.date.second);
                RecordedPosition.position.revolution = res->getInt(4);
                RecordedPosition.position.angle = res->getInt(5);
                RecordedPosition.errorCodes.resize(getNumErrors());
                for (int i = 0; i < getNumErrors(); i++) {
                    RecordedPosition.errorCodes[i] = res->getInt(6 + i);
                }
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
                          ", SQLState: {})", __FILE__, __FUNCTION__, __LINE__, e.what(), e.getErrorCode(),
                          e.getSQLState());
            spdlog::error("{} : Operable Error (2): SQL Exception, did not successfully communicate with database.",
                          m_Identity);
            //operable, If actuator status cannot be read, stil allow actuator to be moved. Local text file can still be used.
            setError(2);
            saveStatusToASF();
            return false;
        }

    } else {
        spdlog::error("{} : Operable Error (1): DBFlag is not set. Cannot read status from DB.", m_Identity);
        setError(1);//operable
        return false;
    }
    saveStatusToASF();
    return true;
}

//read all error codes from DB. Check size of error codes to make sure version is consistent. Adjust this function to the new database table structure.
void Actuator::loadStatusFromDB() {
    ActuatorStatus recordedStatus;
    if (readStatusFromDB(recordedStatus)) {
        m_CurrentPosition.revolution = recordedStatus.position.revolution;
        m_CurrentPosition.angle = recordedStatus.position.angle;
        for (int i = 0; i < getNumErrors(); i++) {
            if (recordedStatus.errorCodes[i]) {
                setError(i);
            }
        }
        if (m_Errors[0]) {
            if (!recordedStatus.errorCodes[0]) {
                unsetError(0);
            }
        }
        saveStatusToASF();
    }
}

void Actuator::saveStatusToDB()//record all error codes to DB. Adjust to new db table structure.
{
    spdlog::trace("{} : Recording actuator status to DB...", m_Identity);
    ActuatorStatus statusToSave;
    if (readStatusFromASF(statusToSave)) {
        if (m_Errors[1] == false) {
            try {
                sql::Driver *driver;
                sql::Connection *con;
                sql::Statement *stmt;

                driver = get_driver_instance();
                std::string dbAddress = "tcp://" + m_DBInfo.host + ":" + m_DBInfo.port;
                con = driver->connect(dbAddress, m_DBInfo.user, m_DBInfo.password);
                con->setSchema(m_DBInfo.dbname);
                stmt = con->createStatement();

                std::stringstream stmtvar;

                std::string datestring =
                    std::to_string(statusToSave.date.year) + "-" + std::to_string(statusToSave.date.month) +
                    "-" + std::to_string(statusToSave.date.day) + " " +
                    std::to_string(statusToSave.date.hour) + ":" +
                    std::to_string(statusToSave.date.minute) + ":" +
                    std::to_string(statusToSave.date.second);

                stmtvar << "INSERT INTO Opt_ActuatorStatus VALUES (null, " << getSerialNumber() << ", '" << datestring
                        << "', " << statusToSave.position.revolution << ", " << statusToSave.position.angle;

                for (int i = 0; i < getNumErrors(); i++) {
                    stmtvar << ", " << statusToSave.errorCodes[i];
                }
                stmtvar << ")";

                stmt->execute(stmtvar.str());
                stmtvar.str(std::string());

                delete stmt;
                delete con;

            }
            catch (sql::SQLException &e) {
                spdlog::error("# ERR: SQLException in {}"
                              "({}) on line {}\n"
                              "# ERR: {}"
                              " (MySQL error code: {}"
                              ", SQLState: {})", __FILE__, __FUNCTION__, __LINE__, e.what(), e.getErrorCode(),
                              e.getSQLState());
                spdlog::error("{} : Operable Error (2): SQL Exception, did not successfully communicate with database.",
                              m_Identity);
                setError(2);//operable, Local textfile can still be used.
            }
        } else {
            spdlog::error("{} : Operable Error (1): DBFlag is not set. Cannot record status to DB.", m_Identity);
            setError(1);//operable
        }
    }
    saveStatusToASF();
}

float Actuator::__readVoltage() {
    int MeasurementCount = 0;
    m_ADCdata = m_pCBC->adc.readEncoder(getPortNumber());
    while ((m_ADCdata.stddev > m_StdDevRemeasure) && (MeasurementCount < m_MaxVoltageMeasurementAttempts)) {
        m_ADCdata = m_pCBC->adc.readEncoder(getPortNumber());
        MeasurementCount++;
    }
    if (m_ADCdata.stddev > m_StdDevMax) {
        spdlog::error(
            "{} : Fatal Error (7): Actuator voltage measured ({}) has a standard deviation ({}) which is greater than the max std dev allowed ({}).",
            m_Identity, m_ADCdata.voltage, m_ADCdata.stddev, m_StdDevMax);
        setError(7);//fatal
        saveStatusToASF();
    }
    return m_ADCdata.voltage;
}

void Actuator::probeHome()//method used to define home.
{
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    spdlog::trace("{} : Probing Home position...", m_Identity);

    __probeEndStop(1);

    float MeasuredVoltage = __readVoltage();
    if (m_Errors[7] == true) {
        return;
    }
    float ExtendStopVoltageMax = m_VMax - (StepsPerRevolution / 4.0) * dV;
    float ExtendStopVoltageMin = m_VMin + (StepsPerRevolution / 4.0) * dV;
    if (MeasuredVoltage > ExtendStopVoltageMax || MeasuredVoltage < ExtendStopVoltageMin) {
        spdlog::error(
            "{} : Operable Error (11): Actuator voltage at Extend Stop reads {}. Encoder should have been set during assembly to have a voltage in the range ({} - {}). Could possibly cause a {} step uncertainty in position.",
            m_Identity, MeasuredVoltage, ExtendStopVoltageMin, ExtendStopVoltageMax, StepsPerRevolution);
        setError(11);//operable
        saveStatusToASF();
    }

    float VoltageBefore;
    float AbsDeltaVoltage;
    float VoltageAfter = MeasuredVoltage;
    if (m_Errors[7] == true) {
        return;
    }
    int CurrentCyclesFromExtendStop = 0;
    int StepsFromExtendStop = 0;
    bool NotReachedHome = true;
    m_keepStepping = true;

    while (NotReachedHome && m_keepStepping) {
        VoltageBefore = VoltageAfter;
        m_pCBC->driver.step(getPortNumber(), -1);//step once, negative is retraction.
        StepsFromExtendStop++;
        VoltageAfter = __readVoltage();
        if (m_Errors[7] == true) {
            return;
        }
        if ((VoltageAfter - VoltageBefore) <
            0)//a negative step increases the voltage by dV. if we detect a voltage that is decreasing...
        {
            AbsDeltaVoltage = std::fabs(VoltageAfter - VoltageBefore);
            if (AbsDeltaVoltage >
                ((dV * StepsPerRevolution) / 2))//if we jump voltage greater than half of the range.
            {
                CurrentCyclesFromExtendStop++;
            } else//error must have occured.. probably stuck
            {
                spdlog::error(
                    "{} : Fatal Error (0): Actuator appears to be stuck at the end stop. Home position not set.",
                    m_Identity);
                setError(0);
                //setError(14);
                saveStatusToASF();
                return;
            }
        }
        if (CurrentCyclesFromExtendStop == m_CyclesDefiningHome) {
            NotReachedHome = false;
        }
    }
    int RecordedStepsFromExtendStop = -1 * (convertPositionToSteps(m_ExtendStopPosition));
    int StepsDeviationFromExtendStop = RecordedStepsFromExtendStop - StepsFromExtendStop;
    if (std::abs(StepsDeviationFromExtendStop) > m_ExtendStopToHomeStepsDeviation) {
        spdlog::error(
            "{} : Operable Error (13): Actuator has stopped at {} steps away from the last recorded end stop position. Home position may be ill-defined.",
            m_Identity, StepsDeviationFromExtendStop);
        setError(
            13);//operable. if home is ill defined, we should still be able to move the actuator. Also, if internal position "ExtendStop" is not correct, we should still be able to move actuator.
    }
    //Actuator::position
    Position HomePosition{};
    HomePosition.revolution = 0;
    HomePosition.angle = 0;
    setCurrentPosition(HomePosition);
    unsetError(0);
    saveStatusToASF();
}

int Actuator::__step(int steps) {
    spdlog::trace("{} : Stepping actuator {} steps...", m_Identity, steps);

    if (getErrorState() == Device::ErrorState::FatalError) {
        spdlog::trace("{}: Fatal error, disallowing motion.", m_Identity);
    }

    loadStatusFromASF();
    recoverPosition();
    Position FinalPosition = predictNewPosition(m_CurrentPosition, -steps);
    Position PredictedPosition{};
    int MissedSteps;
    int StepsTaken;
    int Sign;
    int StepsRemaining = -(convertPositionToSteps(FinalPosition) - convertPositionToSteps(
        m_CurrentPosition));//negative because positive step is retraction, and (0,0) is defined as full extraction.

    if (steps < 0) {
        Sign = -1;
    } else {
        Sign = 1;
    }

    int StepsToTake = Sign * RecordingInterval;
    m_keepStepping = true;

    while ((m_keepStepping) && (getErrorState() != Device::ErrorState::FatalError) &&
           (getDeviceState() != Device::DeviceState::Off)) {
        if (std::abs(StepsRemaining) <= RecordingInterval) {
            StepsToTake = StepsRemaining;
            m_keepStepping = false;
        }
        PredictedPosition = predictNewPosition(m_CurrentPosition, -StepsToTake);
        m_pCBC->driver.step(getPortNumber(), StepsToTake);
        MissedSteps = checkAngleQuick(
            PredictedPosition);//negative*negative=positive sign because retraction is increasing internal counter and missed steps is negative by definition.
        if (m_Errors[7] == true)//if voltage measurement has issues.
        {
            return StepsRemaining;
        }

        StepsTaken = StepsToTake - MissedSteps;
        setCurrentPosition(predictNewPosition(m_CurrentPosition, -StepsTaken));

        //if( (std::abs(MissedSteps)/float(std::abs(StepsToTake)))>TolerablePercentOfMissedSteps && std::abs(MissedSteps)>MinimumMissedStepsToFlagError)//if the actuator misses a certain percent of steps AND misses more than a threshold number of steps.
        if (std::abs(MissedSteps) >
            std::max(int(m_TolerablePercentOfMissedSteps * std::abs(StepsToTake)), m_MinimumMissedStepsToFlagError)) {
            spdlog::error("{} : Fatal Error (8): Actuator has missed a large number of steps ({})", m_Identity,
                          MissedSteps);
            setError(8);//fatal
            saveStatusToASF();
            return StepsRemaining;//quit, don't record or register steps attempted to be taken.
        }

        saveStatusToASF();
        StepsRemaining = -(convertPositionToSteps(FinalPosition) - convertPositionToSteps(m_CurrentPosition));
    }

    return StepsRemaining;
}

void Actuator::__findHomeFromEndStop(int direction) {
    //if direction=1, probeextendstop then set current position to recorded extendstop. compare the number of steps away from the recorded value, and report error if this number is too high.
    Position targetPosition{};
    if (direction == 1) {
        targetPosition = m_ExtendStopPosition;
    } else if (direction == -1) {
        targetPosition = m_RetractStopPosition;
    } else {
        spdlog::error(
            "{} : Actuator::findHomeFromEndStop() : Invalid choice of direction {} (must be 1 for extend or -1 for retract.)",
            m_Identity, direction);
        return;
    }

    __probeEndStop(direction);

    int indexDeviation = checkAngleSlow(targetPosition);
    setCurrentPosition(predictNewPosition(targetPosition, indexDeviation));
    if (std::abs(indexDeviation) > m_EndStopRecoverySteps) {
        spdlog::error(
            "{} : Operable Error (12): Actuator's new end stop is {} (mod {}) steps away from the last recorded end stop position. Home position is possibly a cycle off! ProbeHome() needs to be called to more accurately calibrate Home position.",
            m_Identity, indexDeviation, StepsPerRevolution);
        setError(12);//operable, we still want to move the actuator.
        saveStatusToASF();
    } else {
        unsetError(0);
        saveStatusToASF();
        //Step here to check if we are stuck?
        int StepsRemaining = __step(-1 * direction * RecordingInterval);
        if (std::abs(StepsRemaining) > (RecordingInterval / 2))//If we miss more than half of the steps
        {

            spdlog::error("{} : Fatal Error (0): Actuator appears to be stuck at the end stop. Home position not set.",
                          m_Identity);
            setError(0);
            //setError(14);
            saveStatusToASF();
        } else {
            __step(1 * direction * RecordingInterval);
        }
    }
}

void Actuator::__probeEndStop(int direction) {
    setError(0); // Set home position not found error until probe home is complete

    int searchSteps = direction * EndstopSearchStepsize;
    if (m_Errors[7] == true) {
        return;
    }

    bool atStop = false;
    m_keepStepping = true;

    float VoltageBefore;
    float VoltageAfter = __readVoltage();
    float AbsDeltaVoltage;
    while (!atStop && m_keepStepping) {
        VoltageBefore = VoltageAfter;
        m_pCBC->driver.step(getPortNumber(), searchSteps);
        VoltageAfter = __readVoltage();
        if (m_Errors[7] == true) {
            return;
        }
        AbsDeltaVoltage = std::fabs(VoltageAfter - VoltageBefore);
        if (AbsDeltaVoltage < std::fabs(dV * searchSteps * m_StoppedSteppingFactor)) {
            atStop = true;
        }
    }
}

int Actuator::stepDriver(int inputSteps) {
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    m_pCBC->driver.step(getPortNumber(), inputSteps);
}

bool Actuator::isOn() {
    return m_pCBC->driver.isEnabled(getPortNumber());
}

void Actuator::turnOn() {
    spdlog::debug("{} : Actuator : Turning on power...", m_Identity);
    m_pCBC->driver.enable(getPortNumber());    
    initialize();
}

void Actuator::turnOff() {
    spdlog::debug("{} : DummyActuator : Turning off power...", m_Identity);
    saveStatusToASF();
    m_pCBC->driver.disable(getPortNumber());    
}

#endif

int DummyActuator::__step(int steps)
{
    spdlog::trace("{} : DummyActuator : Stepping actuator {} steps...", m_Identity, steps);
    Position finalPosition = predictNewPosition(m_CurrentPosition, -steps);

    int direction;

    if (steps > 0) {
        direction = 1;
    } else {
        direction = -1;
    }

    int numFullCycles = steps / 80;
    int remainingSteps = steps % 80;

    for (int i = 0; i < numFullCycles; i++) {
        sleep(1);
        m_CurrentPosition.angle += 80 * direction;
        if (m_CurrentPosition.angle > 200) {
            m_CurrentPosition.revolution += 1;
            m_CurrentPosition.angle -= 200;
        } else if (m_CurrentPosition.angle < 0) {
            m_CurrentPosition.revolution -= 1;
            m_CurrentPosition.angle += 200;
        }
    }

    //step remaining steps
    sleep(1);
    m_CurrentPosition.angle += remainingSteps;
    if (m_CurrentPosition.angle > 200) {
        m_CurrentPosition.revolution += 1;
        m_CurrentPosition.angle -= 200;
    } else if (m_CurrentPosition.angle < 0) {
        m_CurrentPosition.revolution -= 1;
        m_CurrentPosition.angle += 200;
    }

    remainingSteps = -(convertPositionToSteps(finalPosition) - convertPositionToSteps(
            m_CurrentPosition));//negative because positive step is retraction, and (0,0) is defined as full extraction.

    spdlog::trace("{}: DummyActuator::step() : New length => {} ", m_Identity, measureLength());
    return remainingSteps;
}

int DummyActuator::stepDriver(int inputSteps) {
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    __step(inputSteps);
}

float DummyActuator::__readVoltage() {
    return m_encoderScale[m_CurrentPosition.angle];
}

bool DummyActuator::isOn() {
    return m_On;
}

void DummyActuator::turnOn() {
    spdlog::debug("{} : DummyActuator : Turning on power...", m_Identity);
    initialize();
    m_On = true;
}

void DummyActuator::turnOff() {
    spdlog::debug("{} : DummyActuator : Turning off power...", m_Identity);
    m_On = false;
}

void DummyActuator::__findHomeFromEndStop(int direction) {
    //if direction=1, probeextendstop then set current position to recorded extendstop. compare the number of steps away from the recorded value, and report error if this number is too high.
    Position targetPosition{};
    if (direction == 1) {
        targetPosition = m_ExtendStopPosition;
    } else if (direction == -1) {
        targetPosition = m_RetractStopPosition;
    } else {
        spdlog::error(
            "{} : DummyActuator::findHomeFromEndStop(): Invalid choice of direction {} (must be 1 for extend or -1 for retract).",
            m_Identity, direction);
        return;
    }

    __probeEndStop(direction);

    int indexDeviation = checkAngleSlow(targetPosition);
    setCurrentPosition(predictNewPosition(targetPosition, indexDeviation));
    if (std::abs(indexDeviation) > m_EndStopRecoverySteps) {
        spdlog::error(
            "{} : Operable Error (12): DummyActuator's new end stop is {} (mod {}) steps away from the last recorded end stop position. Home position is possibly a cycle off! ProbeHome() needs to be called to more accurately calibrate Home position.",
            m_Identity, indexDeviation, StepsPerRevolution);
        setError(12);//operable, we still want to move the actuator.
        saveStatusToASF();
    } else {
        unsetError(0);
        saveStatusToASF();
        //Step here to check if we are stuck?
        int StepsRemaining = __step(-1 * direction * RecordingInterval);
        if (std::abs(StepsRemaining) > (RecordingInterval / 2))//If we miss more than half of the steps
        {
            spdlog::error("{} : Fatal Error (0): Actuator appears to be stuck at the end stop. Home position not set.",
                          m_Identity);
            setError(0);
            //setError(14);
            saveStatusToASF();
        } else {
            __step(1 * direction * RecordingInterval);
        }
    }
}

void DummyActuator::probeHome() {
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    spdlog::debug("{} : Probing Home for Dummy Actuator {}...", m_Identity);
    __probeEndStop(1);

    float MeasuredVoltage = __readVoltage();
    float ExtendStopVoltageMax = m_VMax - (StepsPerRevolution / 4.0) * dV;
    float ExtendStopVoltageMin = m_VMin + (StepsPerRevolution / 4.0) * dV;
    if (MeasuredVoltage > ExtendStopVoltageMax || MeasuredVoltage < ExtendStopVoltageMin) {
        spdlog::error(
            "{} : Operable Error (11): DummyActuator voltage at Extend Stop reads {}. Encoder should have been set during assembly to have a voltage in the range ({} - {}). Could possibly cause a {} step uncertainty in position.",
            m_Identity, MeasuredVoltage, ExtendStopVoltageMin, ExtendStopVoltageMax, StepsPerRevolution);
        setError(11);//operable
        saveStatusToASF();
    }

    //Actuator::position
    Position HomePosition{};
    HomePosition.revolution = 0;
    HomePosition.angle = 0;
    setCurrentPosition(HomePosition);
    unsetError(0);
    saveStatusToASF();
}

void DummyActuator::__probeEndStop(int direction) {
    setError(0); // Set home position not found error until probe home is complete

    int searchSteps = direction * EndstopSearchStepsize;
    if (m_Errors[7] == true) {
        return;
    }

    bool atStop = false;
    m_keepStepping = true;

    __step(searchSteps);

    float VoltageBefore;
    float VoltageAfter = __readVoltage();
    float AbsDeltaVoltage;
    while (!atStop && m_keepStepping) {
        VoltageBefore = VoltageAfter;
        __step(searchSteps);
        VoltageAfter = __readVoltage();
        if (m_Errors[7] == true) {
            return;
        }
        AbsDeltaVoltage = std::fabs(VoltageAfter - VoltageBefore);
        if (AbsDeltaVoltage < std::fabs(dV * searchSteps * m_StoppedSteppingFactor)) {
            atStop = true;
        }
    }
}
