#ifdef DEBUG
#define DEBUG_MSG(str) do {std::cout << str << std::endl;} while (false)
#else
#define DEBUG_MSG(str) do {} while (false)
#endif

#define ERROR_MSG(str) do {std::cout << str << std::endl;} while (false)

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

#include "common/cbccode/cbc.hpp"

const std::vector<Device::ErrorDefinition> Actuator::ERROR_DEFINITIONS = {
        {"Home position is not calibrated",                                                                                                                           Device::DeviceState::FatalError},//error 0
        {"DBInfo not set",                                                                                                                                            Device::DeviceState::OperableError},//error 1
        {"MySQL Communication Error",                                                                                                                                 Device::DeviceState::OperableError},//error 2
        {"DB Columns does not match what is expected",                                                                                                                Device::DeviceState::FatalError},//error 3
        {"ASF File is Bad",                                                                                                                                           Device::DeviceState::FatalError},//error 4
        {"ASF File entries does not match what is expected",                                                                                                          Device::DeviceState::FatalError},//error 5
        {"DB recording more recent than ASF and has mismatch with measured angle",                                                                                    Device::DeviceState::FatalError},//error 6
        {"Voltage Std Dev is entirely too high",                                                                                                                      Device::DeviceState::FatalError},//error 7
        {"Actuator Missed too many steps",                                                                                                                            Device::DeviceState::FatalError},//error 8
        {"Actuator position is too many steps away to recover safely",                                                                                                Device::DeviceState::FatalError},//error 9
        {"Actuator position is recovering large amount of steps, should be ok",                                                                                       Device::DeviceState::OperableError},//error 10
        {"Extend Stop Voltage is too close to the discontinuity. Possible 1 cycle uncertainty with calibrated data",                                                  Device::DeviceState::OperableError},//error 11
        {"End stop is large number of steps away from what is expected. Possible uncertainty in home position",                                                       Device::DeviceState::OperableError},//error 12
        {"Discrepancy between number of steps from extend stop and recorded number of steps from end stop is too high. Possible uncertainty in probed home position", Device::DeviceState::OperableError}//error 13
};

const Actuator::ASFInfo Actuator::EMERGENCY_ASF_INFO = {"/.ASF/", ".ASF_Emergency_Port_", ".log"};

Actuator::Actuator(std::shared_ptr<CBC> pCBC, Device::Identity identity, Device::DBInfo DBInfo,
                   const ASFInfo &ASFFileInfo) : Device::Device(std::move(pCBC), std::move(identity)),
                                                 m_ADCdata(CBC::ADC::adcData()) {
    m_Errors.assign(getNumErrors(), false);
    setError(0); // By default, home position not calibrated
    setError(1); // By default, DB info not set

    if (getSerialNumber() == -1) {
        m_Identity.serialNumber = std::stoi(m_Identity.eAddress);
        setASFInfo(EMERGENCY_ASF_INFO);
        DEBUG_MSG(
            "Actuator::Actuator(): Port: " << getPortNumber() << ". No serial number passed, creating emergency ASF");
    } else {
        setASFInfo(ASFFileInfo);
        DEBUG_MSG("Actuator::Actuator(): Port: " << getPortNumber() << ". Serial Num: " << getSerialNumber());
    }
    if (!DBInfo.empty()) {
        setDBInfo(DBInfo);
    } else {
        DEBUG_MSG("Actuator::Actuator(): No DB info provided...");
    }

    m_encoderScale.resize(StepsPerRevolution);
    for (int i = 0; i < StepsPerRevolution; i++) {
        m_encoderScale[i] = m_VMin + (i * dV);
    }

    initialize();
}

void Actuator::loadConfigurationAndCalibration() {
    DEBUG_MSG("Reading Configuration and Calibration Information from DB for Actuator " << getSerialNumber());
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
            std::cout << "# ERR: SQLException in " << __FILE__;
            std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
            std::cout << "# ERR: " << e.what();
            std::cout << " (MySQL error code: " << e.getErrorCode();
            std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
            std::cout << "Actuator Serial: " << getSerialNumber() << std::endl;
            ERROR_MSG("Operable Error: SQL Exception caught for Actuator " << getSerialNumber()
                                                                           << ". Did not successfully communicate with database.");
            setError(2);//operable
        }
    } else {
        ERROR_MSG("Operable Error: DBFlag is not set for Actuator " << getSerialNumber()
                                                                    << ". Cannot read Configuration and Calibration from DB.");

    }
    saveStatusToASF();
}

//read all error codes from DB. Check size of error codes to make sure version is consistent. Adjust this function to the new database table structure.
bool Actuator::readStatusFromDB(ActuatorStatus &RecordedPosition)
{
    //check that the number of columns matches what is expected
    DEBUG_MSG("Reading Status from DB for Actuator " << getSerialNumber());
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
                ERROR_MSG("Fatal Error: DB Status number of arguments (" << resmeta->getColumnCount()
                                                                         << ") did not equal the number expected ("
                                                                         << NUM_DB_COLUMNS
                                                                         << "). Either DB or this code appears to have an incorrect structure.");
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
            std::cout << "# ERR: SQLException in " << __FILE__;
            std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
            std::cout << "# ERR: " << e.what();
            std::cout << " (MySQL error code: " << e.getErrorCode();
            std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
            std::cout << "Actuator Serial: " << getSerialNumber() << std::endl;
            ERROR_MSG("Operable Error: SQL Exception caught for Actuator " << getSerialNumber()
                                                                           << ". Did not successfully communicate with database.");

            //operable, If actuator status cannot be read, stil allow actuator to be moved. Local text file can still be used.
            setError(2);
            saveStatusToASF();
            return false;
        }

    } else {
        ERROR_MSG(
            "Operable Error: DBFlag is not set for Actuator " << getSerialNumber() << ". Cannot read Status from DB.");
        setError(1);//operable
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
    DEBUG_MSG("Recording Status to DB for Actuator " << getSerialNumber());
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
                std::cout << "# ERR: SQLException in " << __FILE__;
                std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
                std::cout << "# ERR: " << e.what();
                std::cout << " (MySQL error code: " << e.getErrorCode();
                std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
                std::cout << "Actuator Serial: " << getSerialNumber() << std::endl;
                ERROR_MSG("Operable Error: SQL Exception caught for Actuator " << getSerialNumber()
                                                                               << ". Did not successfully communicate with database.");
                setError(2);//operable, Local textfile can still be used.
            }
        } else {
            ERROR_MSG("Operable Error: DBFlag is not set for Actuator " << getSerialNumber()
                                                                        << ". Cannot record Status to DB.");
            setError(1);//operable
        }
    }
    saveStatusToASF();
}

//read all error codes from ASF. Check size of error codes to make sure version is consistent. Read whether Home is set or not.
bool Actuator::readStatusFromASF(ActuatorStatus &RecordedPosition)
{
    DEBUG_MSG("Reading Status from ASF File with path " << m_ASFPath);
    std::ifstream ASF(m_ASFPath);
    //if(ASF.bad())//if file does not exist (or possibly other file issues not expected..)
    if (!ASF.good())//if file does not exist (or possibly other file issues not expected..)
    {
        ERROR_MSG("ASF file was bad for Actuator " << getSerialNumber() << " with ASF path " << m_ASFPath
                                                   << ". Assuming it did not exist and will create a default ASF file.");
        ASF.close();
        createDefaultASF();
        ASF.open(m_ASFPath);
        //if(ASF.bad())//check if ASF is good again. If not, set fatal error.
        if (!ASF.good())//check if ASF is good again. If not, set fatal error.
        {
            ERROR_MSG("Fatal Error: Creating ASF file for Actuator " << getSerialNumber()
                                                                     << " did not resolve problem. File appears corrupt.");
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
        ERROR_MSG("Fatal Error: ASF file (" << m_ASFPath << ") number of arguments (" << ASFReadArray.size()
                                            << ") did not equal the number expected (" << NUM_ASF_COLUMNS
                                            << "). ASF File appears to have an incorrect structure.");
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
void Actuator::loadStatusFromASF()
{
    ActuatorStatus RecordedPosition;
    if (readStatusFromASF(RecordedPosition)) {
        m_CurrentPosition.revolution = RecordedPosition.position.revolution;
        m_CurrentPosition.angle = RecordedPosition.position.angle;
        for (int i = 0; i < getNumErrors(); i++) {
            if (RecordedPosition.errorCodes[i]) {
                setError(i);
            }
        }
        if (m_Errors[0]) {
            if (!RecordedPosition.errorCodes[0]) {
                unsetError(0);
            }
        }
    }
}

void Actuator::saveStatusToASF()//record all error codes to ASF.
{
    DEBUG_MSG("Recording Status for Actuator " << getSerialNumber() << " to ASF file with path " << m_ASFPath);
    copyFile(m_ASFPath, m_OldASFPath);
    std::ofstream ASF(m_NewASFPath);

    //if(ASF.bad())//or exist
    if (!ASF.good())//or exist
    {
        ERROR_MSG("Fatal Error: Cannot write to ASF" << getSerialNumber() << ".log.new, cannot record Status to ASF.");
        setError(4);//fatal
        return;
    }

    time_t now = time(nullptr);//0 for UTC., unix timing will overflow in 2038.
    struct tm *ptm;
    ptm = gmtime(&now);
    ASF << ptm->tm_year + 1900 << " " << ptm->tm_mon + 1 << " " << ptm->tm_mday << " " << ptm->tm_hour << " "
        << ptm->tm_min << " " << ptm->tm_sec << " ";
    ASF << m_CurrentPosition.revolution << " " << m_CurrentPosition.angle;
    for (int i = 0; i < getNumErrors(); i++) {
        ASF << " " << m_Errors[i];
    }
    ASF.close();
    copyFile(m_NewASFPath, m_ASFPath);
}

float Actuator::readVoltage() {
    bool alreadyBusy = isBusy();
    if (!alreadyBusy) {
        setBusy();
    }
    int MeasurementCount = 0;
    m_ADCdata = m_pCBC->adc.readEncoder(getPortNumber());
    while ((m_ADCdata.stddev > m_StdDevRemeasure) && (MeasurementCount < m_MaxVoltageMeasurementAttempts)) {
        m_ADCdata = m_pCBC->adc.readEncoder(getPortNumber());
        MeasurementCount++;
    }
    if (m_ADCdata.stddev > m_StdDevMax) {
        ERROR_MSG("Fatal Error: Actuator " << getSerialNumber() << " voltage measured (" << m_ADCdata.voltage
                                           << ") has a standard deviation (" << m_ADCdata.stddev
                                           << ") which is greater than the max std dev allowed of " << m_StdDevMax
                                           << " volts");
        setError(7);//fatal
        saveStatusToASF();
    }
    if (!alreadyBusy) {
        unsetBusy();
    }
    return m_ADCdata.voltage;
}

int Actuator::readAngle() {
    float voltage = readVoltage();
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

int Actuator::checkAngleQuick(
        Position ExpectedPosition)//First attempts a quicker, less robust method of determining the current actuator angle. Defers to the slower method if it fails. Returns the number of missed steps. (e.g. returns +2 if expected position is 50,100 but measured angle is 102). This method ends when either range is exhausted (set by independent variable QuickAngleCheckSearchDeviation), or when it finds a voltage deviation less than dV/2. checkAngleSlow has no such requirement, and finds minimum voltage scanning over range of actuator steps.
{
    int ExpectedAngle = ExpectedPosition.angle;
    float MeasuredVoltage = readVoltage();
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

int Actuator::checkAngleSlow(Position ExpectedPosition) {
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


int Actuator::step(int steps)//Positive Step is Extension of Motor
{
    if (getState() != Device::DeviceState::On &&
        getState() != Device::DeviceState::OperableError)//don't move actuator if there's a fatal error.
    {
        return steps;
    }
    DEBUG_MSG("Stepping Actuator " << getSerialNumber() << " " << steps << " steps");
    bool alreadyBusy = isBusy();
    if (!alreadyBusy) {
        setBusy();
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
    bool KeepStepping = true;

    while ((KeepStepping) && (getState() != Device::DeviceState::FatalError)) {
        if (std::abs(StepsRemaining) <= RecordingInterval) {
            StepsToTake = StepsRemaining;
            KeepStepping = false;
        }
        PredictedPosition = predictNewPosition(m_CurrentPosition, -StepsToTake);
        m_pCBC->driver.step(getPortNumber(), StepsToTake);
        getErrorState();
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
            ERROR_MSG(
                "Fatal Error: Actuator " << getSerialNumber() << " missed a large number of steps (" << MissedSteps
                                         << ").");
            setError(8);//fatal
            saveStatusToASF();
            return StepsRemaining;//quit, don't record or register steps attempted to be taken.
        }

        saveStatusToASF();
        StepsRemaining = -(convertPositionToSteps(FinalPosition) - convertPositionToSteps(m_CurrentPosition));
    }
    if (!alreadyBusy) {
        unsetBusy();
    }
    return StepsRemaining;
}

float Actuator::measureLength() {
    DEBUG_MSG("Measuring Actuator Length for Actuator " << getSerialNumber());
    loadStatusFromASF();
    recoverPosition();
    int StepsFromHome = convertPositionToSteps(m_CurrentPosition);
    float DistanceFromHome = StepsFromHome * mmPerStep;
    float currentLength = HomeLength - DistanceFromHome;
    return currentLength;
}

float Actuator::moveToLength(float targetLength) {
    float currentLength = measureLength();
    float lengthToMove = targetLength - currentLength;
    int stepsToMove = std::floor((lengthToMove / mmPerStep) + 0.5);
    step(stepsToMove);
    currentLength = measureLength();
    return currentLength;
}

float Actuator::moveDeltaLength(float lengthToMove) {
    float currentLength = measureLength();
    float targetLength = currentLength + lengthToMove;
    currentLength = moveToLength(targetLength);
    float lengthRemaining = targetLength - currentLength;
    return lengthRemaining;
}

Actuator::Position Actuator::predictNewPosition(Position position,
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

int Actuator::performHysteresisMotion(int steps) {
    DEBUG_MSG("Performing Hysteresis Motion of " << steps << " for Actuator " << getSerialNumber());
    int stepsRemaining = step(steps - m_HysteresisSteps);
    return step(m_HysteresisSteps + stepsRemaining);
}

//Port, Serial, ASFPath, and sometimes DB are loaded. The rest of the loading needs to be designed here. Set Current position
bool Actuator::initialize() {
    //check if ASF file exists. if it doesn't, create it.
    DEBUG_MSG("Initializing Actuator " << getSerialNumber());
    loadStatusFromASF();
    loadConfigurationAndCalibration();
    recoverPosition();
    getErrorState();

    return true;
}

void
Actuator::recoverPosition()//consolidates current position and recovers position (if not too far away). This is typically ran after reading status from ASF.
{
    DEBUG_MSG("Actuator: Checking current position...");
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
        ERROR_MSG("Operable Error: Actuator " << getSerialNumber() << " is " << indexDeviation
                                              << " steps away from the last believed position. This number is large enough to warrant an error, but below the set maximum number of recoverable steps ("
                                              << m_MaxRecoverySteps
                                              << "). Software will recover the position, but this error should not happen under normal conditions.");
        setError(10);//operable
        setCurrentPosition(predictNewPosition(m_CurrentPosition, indexDeviation));
        saveStatusToASF();
        return;
    } else { //If the difference between where we are and where we think we are is extremely high, set a FatalError. position is lost. (Do we want to set fatal error?? maybe we just recover and set homeisset=false.
        ERROR_MSG("Fatal Error: Actuator " << getSerialNumber() << " is " << indexDeviation
                                           << " steps away from the last believed position. This number is above the set maximum number of recoverable steps ("
                                           << m_MaxRecoverySteps
                                           << "). Home position will likely need to be found again.");
        DEBUG_MSG("CurrentPosition: (" << m_CurrentPosition.revolution << "," << m_CurrentPosition.angle
                                       << "), Probable position: ("
                                       << predictNewPosition(m_CurrentPosition, indexDeviation).revolution << ", "
                                       << predictNewPosition(m_CurrentPosition, indexDeviation).angle << ")");
        setError(9);//fatal
        setError(0);
        saveStatusToASF();
        return;
    }
}

void Actuator::recoverStatusFromDB() {
    DEBUG_MSG("Recovering the status of Actuator " << getSerialNumber() << " from DB");
    loadStatusFromDB();
    recoverPosition();
    saveStatusToASF();
}

void Actuator::setASFInfo(const ASFInfo &ASFInfo) {
    std::stringstream Path;
    Path << ASFInfo.directory << ASFInfo.prefix << getSerialNumber() << ASFInfo.suffix;
    m_ASFPath = Path.str();
    m_NewASFPath = m_ASFPath + ".new";
    m_OldASFPath = m_ASFPath + ".old";
}

void Actuator::setDBInfo(Device::DBInfo DBInfo) {
    m_DBInfo = std::move(DBInfo);
    unsetError(1);
}

void Actuator::unsetDBInfo() {
    m_DBInfo = Device::DBInfo();
    setError(1);
}

int Actuator::convertPositionToSteps(Position position) {
    return position.revolution * StepsPerRevolution + position.angle;
}

void Actuator::probeHome()//method used to define home.
{
    setBusy();
    DEBUG_MSG("Probing Home for Actuator " << getSerialNumber());
    probeExtendStop();

    float MeasuredVoltage = readVoltage();
    if (m_Errors[7] == true) {
        return;
    }
    float ExtendStopVoltageMax = m_VMax - (StepsPerRevolution / 4.0) * dV;
    float ExtendStopVoltageMin = m_VMin + (StepsPerRevolution / 4.0) * dV;
    if (MeasuredVoltage > ExtendStopVoltageMax || MeasuredVoltage < ExtendStopVoltageMin) {
        ERROR_MSG(
            "Operable Error: Actuator " << getSerialNumber() << " voltage at Extend Stop reads: " << MeasuredVoltage
                                        << ". Encoder should have been set during assembly to have a voltage in the mid-range, between "
                                        << ExtendStopVoltageMin << "-" << ExtendStopVoltageMax
                                        << " volts. Can possibly cause " << StepsPerRevolution
                                        << " step uncertainty in position.");
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

    while (NotReachedHome) {
        VoltageBefore = VoltageAfter;
        m_pCBC->driver.step(getPortNumber(), -1);//step once, negative is retraction.
        StepsFromExtendStop++;
        VoltageAfter = readVoltage();
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
                ERROR_MSG("Fatal Error: Actuator " << getSerialNumber()
                                                   << " appears to be stuck at the end stop. Actuator is stepping just a couple of steps backwards instead of forwards.");
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
        ERROR_MSG("Operable Error: Actuator " << getSerialNumber() << " is " << StepsDeviationFromExtendStop
                                              << " steps away from Recorded Extend Stop position.");
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
    unsetBusy();
}

void Actuator::findHomeFromEndStop(int direction)//use recorded extendstop and set actuator to that.
{
    bool alreadyBusy = isBusy();
    if (!alreadyBusy) {
        setBusy();
    }
    //if direction=1, probeextendstop then set current position to recorded extendstop. compare the number of steps away from the recorded value, and report error if this number is too high.
    Position targetPosition{};
    if (direction == 1) {
        targetPosition = m_ExtendStopPosition;
        probeExtendStop();
    } else if (direction == -1) {
        targetPosition = m_RetractStopPosition;
        probeRetractStop();
    } else {
        return;
    }
    int indexDeviation = checkAngleSlow(targetPosition);
    setCurrentPosition(predictNewPosition(targetPosition, indexDeviation));
    if (std::abs(indexDeviation) > m_EndStopRecoverySteps) {
        ERROR_MSG(
            "Operable Error: Actuator " << getSerialNumber() << " has End Stop which is " << indexDeviation << " (mod "
                                        << StepsPerRevolution
                                        << ") steps away from recorded End Stop position. Home position is possibly a cycle off! ProbeHome() needs to be called to more accurately calibrate Home position.");
        setError(12);//operable, we still want to move the actuator.
        saveStatusToASF();
    } else {
        unsetError(0);
        saveStatusToASF();
        //Step here to check if we are stuck?
        int StepsRemaining = step(-1 * direction * RecordingInterval);
        if (std::abs(StepsRemaining) > (RecordingInterval / 2))//If we miss more than half of the steps
        {
            ERROR_MSG("Fatal Error: Actuator " << getSerialNumber() << " appears to be stuck at the end stop.");
            setError(0);
            //setError(14);
            saveStatusToASF();
        } else {
            step(1 * direction * RecordingInterval);
        }
    }
    if (!alreadyBusy) {
        unsetBusy();
    }
}

void Actuator::probeEndStop(int direction) {
    bool alreadyBusy = isBusy();
    if (!alreadyBusy) {
        setBusy();
    }
    setError(0); // Set home position not found error until probe home is complete

    int searchSteps = direction * EndstopSearchStepsize;
    if (m_Errors[7] == true) {
        return;
    }

    bool atStop = false;

    float VoltageBefore;
    float VoltageAfter = readVoltage();
    float AbsDeltaVoltage;
    while (!atStop) {
        VoltageBefore = VoltageAfter;
        m_pCBC->driver.step(getPortNumber(), searchSteps);
        VoltageAfter = readVoltage();
        if (m_Errors[7] == true) {
            return;
        }
        AbsDeltaVoltage = std::fabs(VoltageAfter - VoltageBefore);
        if (AbsDeltaVoltage < std::fabs(dV * searchSteps * m_StoppedSteppingFactor)) {
            atStop = true;
        }
    }
    if (!alreadyBusy) {
        unsetBusy();
    }
}

void Actuator::createDefaultASF()//hardcoded structure of the ASF file (year,mo,day,hr,min,sec,rev,angle,errorcodes)
{
    DEBUG_MSG("Creating ASF File with location: " << m_ASFPath);
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

void Actuator::clearErrors() {
    Device::clearErrors();
    saveStatusToASF();
}

bool Actuator::forceRecover() {
    ERROR_MSG("Actuator: Force recovering...");
    int indexDeviation = checkAngleQuick(m_CurrentPosition);
    if (m_Errors[7] == true)//check for voltage issue
    {
        return false;
    }
    setCurrentPosition(predictNewPosition(m_CurrentPosition, indexDeviation));
    saveStatusToASF();
    return true;
}

bool Actuator::isOn() {
    return m_pCBC->driver.isEnabled(getPortNumber());
}

void Actuator::turnOn() {
    m_pCBC->driver.enable(getPortNumber());    
    initialize();
}

void Actuator::turnOff() {
    saveStatusToASF();
    m_pCBC->driver.disable(getPortNumber());    
}

void Actuator::copyFile(const std::string &srcFilePath, const std::string &destFilePath) {
    std::ifstream srcFile(srcFilePath, std::ios::binary);
    std::ofstream destFile(destFilePath, std::ios::binary);

    destFile << srcFile.rdbuf();

    srcFile.close();
    destFile.close();
}

int DummyActuator::step(int steps)
{
    std::cout << "DummyActuator: Stepping Actuator " << getSerialNumber() << " " << steps << " steps" << std::endl;
    if (getState() != Device::DeviceState::On && getState() != Device::DeviceState::OperableError) {
        ERROR_MSG("DummyActuator: Fatal error. Cannot step.");
        return steps;
    }
    Position finalPosition = predictNewPosition(m_CurrentPosition, -steps);
    setCurrentPosition(finalPosition); // Set current position to final position (simulate motion)
    int stepsRemaining = -(convertPositionToSteps(finalPosition) - convertPositionToSteps(
            m_CurrentPosition));//negative because positive step is retraction, and (0,0) is defined as full extraction.
    std::cout << "DummyActuator:: Steps Remaining = " << stepsRemaining << std::endl;
    return stepsRemaining;
}

float DummyActuator::measureLength() {
    DEBUG_MSG("Measuring Actuator Length for Dummy Actuator " << getSerialNumber());
    int stepsFromHome = convertPositionToSteps(m_CurrentPosition);
    float distanceFromHome = stepsFromHome * mmPerStep;
    float currentLength = HomeLength - distanceFromHome;
    return currentLength;
}

bool DummyActuator::initialize() {
    DEBUG_MSG("Initializing Dummy Actuator ...");
    return true;
}
