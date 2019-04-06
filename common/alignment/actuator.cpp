#ifdef DEBUG
#define DEBUG_MSG(str) do {std::cout << str << std::endl;} while (false)
#else
#define DEBUG_MSG(str) do {} while (false)
#endif

#define ERROR_MSG(str) do {std::cout << str << std::endl;} while (false)

#include "common/alignment/actuator.hpp"

#include <cmath>
#include <fstream>
#include <sstream>

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <mysql_connection.h>
#include <mysql_driver.h>

const int Actuator::NUM_ERROR_TYPES = 14;
const std::array<Platform::ErrorDefinition, NUM_ERROR_TYPES> Actuator::ERROR_DEFINITIONS = {
        {"Home Position is not calibrated",                                                                                                                           DeviceState::FatalError},//error 0
        {"DBInfo not set",                                                                                                                                            DeviceState::OperableError},//error 1
        {"MySQL Communication Error",                                                                                                                                 DeviceState::OperableError},//error 2
        {"DB Columns does not match what is expected",                                                                                                                DeviceState::FatalError},//error 3
        {"ASF File is Bad",                                                                                                                                           DeviceState::FatalError},//error 4
        {"ASF File entries does not match what is expected",                                                                                                          DeviceState::FatalError},//error 5
        {"DB recording more recent than ASF and has mismatch with measured angle",                                                                                    DeviceState::FatalError},//error 6
        {"Voltage Std Dev is entirely too high",                                                                                                                      DeviceState::FatalError},//error 7
        {"Actuator Missed too many steps",                                                                                                                            DeviceState::FatalError},//error 8
        {"Actuator position is too many steps away to recover safely",                                                                                                DeviceState::FatalError},//error 9
        {"Actuator position is recovering large amount of steps, should be ok",                                                                                       DeviceState::OperableError},//error 10
        {"Extend Stop Voltage is too close to the discontinuity. Possible 1 cycle uncertainty with calibrated data",                                                  DeviceState::OperableError},//error 11
        {"End stop is large number of steps away from what is expected. Possible uncertainty in home position",                                                       DeviceState::OperableError},//error 12
        {"Discrepancy between number of steps from extend stop and recorded number of steps from end stop is too high. Possible uncertainty in probed home position", DeviceState::OperableError}//error 13
};

const ASFStruct DEFAULT_ASF_INFO = {"/.ASF/", ".ASF_", ".log"};
const ASFStruct EMERGENCY_ASF_INFO = {"/.ASF/", ".ASF_Emergency_Port_", ".log"};

//Emergency constructor, pass port number and no serial and do not attempt to log actuator position information.
Actuator::Actuator(std::shared_ptr<CBC> pCBC, int USBPortNumber) : cbc(InputCBC) {
    DEBUG_MSG("Constructing Actuator with no Serial and Port " << USBPortNumber);
    setPortNumber(USBPortNumber);
    setSerialNumber(USBPortNumber);
    setError(0); // By default, home position not calibrated
    setError(1); // By default, DB info not set

    //Create custom emergency path since actuator serial is not passed.
    std::stringstream path;
    path << EMERGENCY_ASF_INFO.Directory << EMERGENCY_ASF_INFO.FilenamePrefix << USBPortNumber
         << EMERGENCY_ASF_INFO.FilenameSuffix;
    ASFFullPath = path.str();
    NewASFFullPath = ASFFullPath + ".new";
    OldASFFullPath = ASFFullPath + ".old";

    EncoderCalibration.resize(stepsPerRevolution);
    for (int i = 0; i < stepsPerRevolution; i++) {
        encoderCalibration[i] = VMin + (i * dV);
    }

    initialize();
}

Actuator::Actuator(CBC *InputCBC, int InputPortNumber, int InputActuatorSerial) : cbc(
        InputCBC)//Emergency constructor, should be used in emergency scenarios. serial number should be passed to keep track of actuator positions (so they dont default to 0).
{
    DEBUG_MSG("Constructing Actuator " << InputActuatorSerial << " with Port " << InputPortNumber);
    SetPortNumber(InputPortNumber);
    SetSerialNumber(InputActuatorSerial);
    setError(0); // By default, home position not calibrated
    setError(1); // By default, DB info not set

    SetASFFullPath(DefaultASFInfo);
    EncoderCalibration.resize(StepsPerRevolution);
    for (int i = 0; i < StepsPerRevolution; i++) {
        EncoderCalibration[i] = VMin + (i * dV);
    }
    Initialize();
}

Actuator::Actuator(CBC *InputCBC, int InputPortNumber, int InputActuatorSerial, DBStruct InputDBInfo) : cbc(
        InputCBC)//Normally used constructor WITHOUT ASFFileLocation. Uses Default ASFFileLocation information. Set it up accordingly.
{
    DEBUG_MSG("Constructing Actuator " << InputActuatorSerial << " with Port " << InputPortNumber
                                       << " and DB Information");
    SetPortNumber(InputPortNumber);
    SetSerialNumber(InputActuatorSerial);
    SetASFFullPath(DefaultASFInfo);
    SetDB(InputDBInfo);
    EncoderCalibration.resize(StepsPerRevolution);
    for (int i = 0; i < StepsPerRevolution; i++) {
        EncoderCalibration[i] = VMin + (i * dV);
    }
    Initialize();
}

Actuator::Actuator(CBC *InputCBC, int InputPortNumber, int InputActuatorSerial, DBStruct InputDBInfo,
                   ASFFileLocation InputASFInfo) : cbc(
        InputCBC)//Normally used constructor WITH ASFFileLocation. I envision this constructor being used only in special occasions, where the ASF path wants to be specifically defined. Set it up accordingly.
{
    DEBUG_MSG("Constructing Actuator " << InputActuatorSerial << " with Port " << InputPortNumber
                                       << " and DB and ASF Information");
    SetPortNumber(InputPortNumber);
    SetSerialNumber(InputActuatorSerial);
    setError(0); // By default, home position not calibrated
    setError(1); // By default, DB info not set

    SetASFFullPath(InputASFInfo);
    SetDB(InputDBInfo);
    EncoderCalibration.resize(StepsPerRevolution);
    for (int i = 0; i < StepsPerRevolution; i++) {
        EncoderCalibration[i] = VMin + (i * dV);
    }
    Initialize();
}

void Actuator::readConfigurationAndCalibration() {
    DEBUG_MSG("Reading Configuration and Calibration Information from DB for Actuator " << m_SerialNumber);
    //check to make sure number of columns match what is expected.
    if (m_Errors[1] == true) {
        try {
            sql::Driver *driver;
            sql::Connection *con;
            sql::Statement *stmt;
            sql::ResultSet *res;

            driver = get_driver_instance();
            std::string hoststring = "tcp://" + m_DBInfo.ip + ":" + m_DBInfo.port;
            con = driver->connect(hoststring, m_DBInfo.user, m_DBInfo.password);
            con->setSchema(m_DBInfo.dbname);
            stmt = con->createStatement();

            std::stringstream stmtvar;
            stmtvar << "SELECT * FROM Opt_ActuatorConfigurationAndCalibration WHERE serial_number=" << SerialNumber
                    << " ORDER BY start_date DESC LIMIT 1";
            stmt->execute(stmtvar.str());
            res = stmt->getResultSet();
            while (res->next()) {
                mmPerStep = res->getDouble(4);
                StepsPerRevolution = res->getInt(5);
                HomeLength = res->getDouble(6);
                RetractStop.Revolution = res->getInt(7);
                RetractStop.Angle = res->getInt(8);
                ExtendStop.Revolution = res->getInt(9);
                ExtendStop.Angle = res->getInt(10);
                RecordingInterval = res->getInt(11);
                CalibrationTemperature = res->getDouble(12);
                HysteresisSteps = res->getInt(13);
                RetractRevolutionLimit = res->getInt(14);
                ExtendRevolutionLimit = res->getInt(15);
                MaxVoltageMeasurementAttempts = res->getInt(16);
                StdDevRemeasure = res->getDouble(17);
                StdDevMax = res->getDouble(18);
                QuickAngleCheckRange = res->getInt(19);
                EndstopSearchStepsize = res->getInt(20);
                CyclesDefiningHome = res->getInt(21);
                MinimumMissedStepsToFlagError = res->getInt(22);
                TolerablePercentOfMissedSteps = res->getDouble(23);
                ExtendStopToHomeStepsDeviation = res->getInt(24);
                FlaggedRecoverySteps = res->getInt(25);
                MaxRecoverySteps = res->getInt(26);
                EndStopRecoverySteps = res->getInt(27);
            }
            EncoderCalibration.resize(StepsPerRevolution);
            for (int i = 0; i < StepsPerRevolution; i++) {
                stmtvar.str(std::string());
                stmtvar << "SELECT * FROM Opt_ActuatorMotorProfile WHERE (serial_number=" << SerialNumber
                        << " and angle=" << i << ") ORDER BY start_date DESC LIMIT 1";
                stmt->execute(stmtvar.str());
                res = stmt->getResultSet();
                while (res->next()) {
                    EncoderCalibration[i] = res->getDouble(5);
                }
            }
            VMin = EncoderCalibration[0];
            VMax = EncoderCalibration[StepsPerRevolution - 1];
            dV = (VMax - VMin) / (StepsPerRevolution - 1);
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
            std::cout << "Actuator Serial: " << SerialNumber << std::endl;
            ERROR_MSG("Operable Error: SQL Exception caught for Actuator " << SerialNumber
                                                                           << ". Did not successfully communicate with database.");
            SetError(2);//operable
            RecordStatusToASF();
            return;
        }
    } else {
        ERROR_MSG("Operable Error: DBFlag is not set for Actuator " << SerialNumber
                                                                    << ". Cannot read Configuration and Calibration from DB.");
        SetError(1);//operable
        RecordStatusToASF();
    }
    return;
}

bool Actuator::ReadStatusFromDB(
        ActuatorStatus &RecordedPosition)//read all error codes from DB. Check size of error codes to make sure version is consistent. Adjust this function to the new database table structure.
{
    //check that the number of columns matches what is expected
    DEBUG_MSG("Reading Status from DB for Actuator " << SerialNumber);
    if (m_Errors[1] == false) {
        try {
            sql::Driver *driver;
            sql::Connection *con;
            sql::Statement *stmt;
            sql::ResultSet *res;
            sql::ResultSetMetaData *resmeta;

            driver = get_driver_instance();
            std::string hoststring = "tcp://" + m_DBInfo.ip + ":" + m_DBInfo.port;
            con = driver->connect(hoststring, m_DBInfo.user, m_DBInfo.password);
            con->setSchema(m_DBInfo.dbname);
            stmt = con->createStatement();

            std::stringstream stmtvar;
            stmtvar << "SELECT * FROM Opt_ActuatorStatus WHERE serial_number=" << SerialNumber
                    << " ORDER BY id DESC LIMIT 1";
            stmt->execute(stmtvar.str());
            res = stmt->getResultSet();
            resmeta = res->getMetaData();
            //check if number of results match what is expected. if not, set error(3)
            if (resmeta->getColumnCount() != NumberOfColumnsInDB) {
                ERROR_MSG("Fatal Error: DB Status number of arguments (" << resmeta->getColumnCount()
                                                                         << ") did not equal the number expected ("
                                                                         << NumberOfColumnsInDB
                                                                         << "). Either DB or this code appears to have an incorrect structure.");
                SetError(3);//fatal
                RecordStatusToASF();
                return false;
            }

            while (res->next()) {
                std::string date = res->getString(3);
                sscanf(date.c_str(), "%d-%d-%d %d:%d:%d", &RecordedPosition.Date.Year, &RecordedPosition.Date.Month,
                       &RecordedPosition.Date.Day, &RecordedPosition.Date.Hour, &RecordedPosition.Date.Minute,
                       &RecordedPosition.Date.Second);
                RecordedPosition.Position.Revolution = res->getInt(4);
                RecordedPosition.Position.Angle = res->getInt(5);
                RecordedPosition.ErrorCodes.resize(NUM_ERROR_TYPES);
                for (int i = 0; i < NUM_ERROR_TYPES; i++) {
                    RecordedPosition.ErrorCodes[i] = res->getInt(6 + i);
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
            std::cout << "Actuator Serial: " << SerialNumber << std::endl;
            ERROR_MSG("Operable Error: SQL Exception caught for Actuator " << SerialNumber
                                                                           << ". Did not successfully communicate with database.");
            SetError(
                    2);//operable, If actuator status cannot be read, stil allow actuator to be moved. Local text file can still be used.
            RecordStatusToASF();
            return false;
        }

    } else {
        ERROR_MSG("Operable Error: DBFlag is not set for Actuator " << SerialNumber << ". Cannot read Status from DB.");
        SetError(1);//operable
        RecordStatusToASF();
    }
    return true;
}

void
Actuator::loadStatusFromDB()//read all error codes from DB. Check size of error codes to make sure version is consistent. Adjust this function to the new database table structure.
{
    ActuatorStatus RecordedPosition;
    if (ReadStatusFromDB(RecordedPosition)) {
        currentPosition.Revolution = RecordedPosition.Position.Revolution;
        currentPosition.Angle = RecordedPosition.Position.Angle;
        for (int i = 0; i < NUM_ERROR_TYPES; i++) {
            if (RecordedPosition.ErrorCodes[i]) {
                SetError(i);
            }
        }
        if (ActuatorErrors[0].Triggered) {
            if (RecordedPosition.ErrorCodes[0] == false) {
                UnsetError(0);
            }
        }
        RecordStatusToASF();
        return;
    }
    return;
}

void Actuator::RecordStatusToDB()//record all error codes to DB. Adjust to new db table structure.
{
    DEBUG_MSG("Recording Status to DB for Actuator " << SerialNumber);
    ActuatorStatus RecordedPosition;
    if (ReadStatusFromASF(RecordedPosition)) {
        if (m_Errors[1] == false) {
            try {
                sql::Driver *driver;
                sql::Connection *con;
                sql::Statement *stmt;

                driver = get_driver_instance();
                std::string hoststring = "tcp://" + m_DBInfo.ip + ":" + m_DBInfo.port;
                con = driver->connect(hoststring, m_DBInfo.user, m_DBInfo.password);
                con->setSchema(m_DBInfo.dbname);
                stmt = con->createStatement();

                std::stringstream stmtvar;

                std::string datestring =
                        std::to_string(RecordedPosition.Date.Year) + "-" + std::to_string(RecordedPosition.Date.Month) +
                        "-" + std::to_string(RecordedPosition.Date.Day) + " " +
                        std::to_string(RecordedPosition.Date.Hour) + ":" +
                        std::to_string(RecordedPosition.Date.Minute) + ":" +
                        std::to_string(RecordedPosition.Date.Second);

                stmtvar << "INSERT INTO Opt_ActuatorStatus VALUES (null, " << SerialNumber << ", '" << datestring
                        << "', " << RecordedPosition.Position.Revolution << ", " << RecordedPosition.Position.Angle;

                for (int i = 0; i < NUM_ERROR_TYPES; i++) {
                    stmtvar << ", " << RecordedPosition.ErrorCodes[i];
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
                std::cout << "Actuator Serial: " << SerialNumber << std::endl;
                ERROR_MSG("Operable Error: SQL Exception caught for Actuator " << SerialNumber
                                                                               << ". Did not successfully communicate with database.");
                SetError(2);//operable, Local textfile can still be used.
                RecordStatusToASF();
                return;
            }
        } else {
            ERROR_MSG("Operable Error: DBFlag is not set for Actuator " << SerialNumber
                                                                        << ". Cannot record Status to DB.");
            SetError(1);//operable
            RecordStatusToASF();
        }
    }
    return;
}

bool Actuator::ReadStatusFromASF(
        ActuatorStatus &RecordedPosition)//read all error codes from ASF. Check size of error codes to make sure version is consistent. Read whether Home is set or not.
{
    DEBUG_MSG("Reading Status from ASF File with path " << ASFFullPath);
    std::ifstream ASF(ASFFullPath);
    //if(ASF.bad())//if file does not exist (or possibly other file issues not expected..)
    if (!ASF.good())//if file does not exist (or possibly other file issues not expected..)
    {
        ERROR_MSG("ASF file was bad for Actuator " << SerialNumber << " with ASF path " << ASFFullPath
                                                   << ". Assuming it did not exist and will create a default ASF file.");
        ASF.close();
        CreateDefaultASF();
        ASF.open(ASFFullPath);
        //if(ASF.bad())//check if ASF is good again. If not, set fatal error.
        if (!ASF.good())//check if ASF is good again. If not, set fatal error.
        {
            ERROR_MSG("Fatal Error: Creating ASF file for Actuator " << SerialNumber
                                                                     << " did not resolve problem. File appears corrupt.");
            SetError(4);//fatal
            return false;
        }
    }
    std::string line;
    int word;
    getline(ASF, line);
    std::vector<int> ASFReadArray;
    ASFReadArray.reserve(NumberOfIntsInASF);
    std::istringstream ss(line);
    while (ss.good()) {
        ss >> word;
        ASFReadArray.push_back(word);
    }
    if (ASFReadArray.size() != NumberOfIntsInASF) {
        ERROR_MSG("Fatal Error: ASF file (" << ASFFullPath << ") number of arguments (" << ASFReadArray.size()
                                            << ") did not equal the number expected (" << NumberOfIntsInASF
                                            << "). ASF File appears to have an incorrect structure.");
        SetError(5);//fatal
        return false;
    }
    //following is ASF structure, hardcoded.
    RecordedPosition.Date.Year = ASFReadArray[0];
    RecordedPosition.Date.Month = ASFReadArray[1];
    RecordedPosition.Date.Day = ASFReadArray[2];
    RecordedPosition.Date.Hour = ASFReadArray[3];
    RecordedPosition.Date.Minute = ASFReadArray[4];
    RecordedPosition.Date.Second = ASFReadArray[5];
    RecordedPosition.Position.Revolution = ASFReadArray[6];
    RecordedPosition.Position.Angle = ASFReadArray[7];

    RecordedPosition.ErrorCodes.resize(NUM_ERROR_TYPES);
    for (int i = 0; i < NUM_ERROR_TYPES; i++) {
        RecordedPosition.ErrorCodes[i] = ASFReadArray[NumberOfIntsInASFHeader + i];
    }
    return true;
}

void
Actuator::LoadStatusFromASF()//read all error codes from ASF. Check size of error codes to make sure version is consistent. Read whether Home is set or not.
{
    ActuatorStatus RecordedPosition;
    if (ReadStatusFromASF(RecordedPosition)) {
        currentPosition.Revolution = RecordedPosition.Position.Revolution;
        currentPosition.Angle = RecordedPosition.Position.Angle;
        for (int i = 0; i < NUM_ERROR_TYPES; i++) {
            if (RecordedPosition.ErrorCodes[i]) {
                SetError(i);
            }
        }
        if (ActuatorErrors[0].Triggered) {
            if (RecordedPosition.ErrorCodes[0] == false) {
                UnsetError(0);
            }
        }
    }
    return;
}

void Actuator::RecordStatusToASF()//record all error codes to ASF.
{
    //DEBUG_MSG("Recording Status for Actuator " << SerialNumber << " to ASF file with path " << ASFFullPath);
    copyFile(ASFFullPath, OldASFFullPath);
    std::ofstream ASF(NewASFFullPath);

    //if(ASF.bad())//or exist
    if (!ASF.good())//or exist
    {
        ERROR_MSG("Fatal Error: Cannot write to ASF" << SerialNumber << ".log.new, cannot record Status to ASF.");
        SetError(4);//fatal
        return;
    }

    time_t now = time(0);//0 for UTC., unix timing will overflow in 2038.
    struct tm *ptm;
    ptm = gmtime(&now);
    ASF << ptm->tm_year + 1900 << " " << ptm->tm_mon + 1 << " " << ptm->tm_mday << " " << ptm->tm_hour << " "
        << ptm->tm_min << " " << ptm->tm_sec << " ";
    ASF << currentPosition.Revolution << " " << currentPosition.Angle;
    for (int i = 0; i < NUM_ERROR_TYPES; i++) {
        ASF << " " << ActuatorErrors[i].Triggered;
    }
    ASF.close();
    copyFile(NewASFFullPath, ASFFullPath);
    return;
}

float Actuator::MeasureVoltage() {
    int MeasurementCount = 0;
    adcdata = cbc->adc.readEncoder(PortNumber);
    while ((adcdata.stddev > StdDevRemeasure) && (MeasurementCount < MaxVoltageMeasurementAttempts)) {
        adcdata = cbc->adc.readEncoder(PortNumber);
        MeasurementCount++;
    }
    if (adcdata.stddev > StdDevMax) {
        ERROR_MSG("Fatal Error: Actuator " << SerialNumber << " voltage measured (" << adcdata.voltage
                                           << ") has a standard deviation (" << adcdata.stddev
                                           << ") which is greater than the max std dev allowed of " << StdDevMax
                                           << " volts");
        setError(7);//fatal
        recordStatusToASF();
    }
    return adcdata.voltage;
}

int Actuator::measureAngle() {
    float voltage = measureVoltage();
    //find minimum deviation from measured voltage and array of voltages. index of this array is the Angle.
    float MinimumDifference = std::fabs(voltage - EncoderCalibration[0]);
    int Index = 0;
    float Deviation;
    for (int i = 1; i < StepsPerRevolution; i++) {
        Deviation = std::fabs(MeasuredVoltage - EncoderCalibration[i]);
        if (Deviation < MinimumDifference) {
            MinimumDifference = Deviation;
            Index = i;
        }
    }
    return index;
}

int Actuator::QuickAngleCheck(
        Position ExpectedPosition)//First attempts a quicker, less robust method of determining the current actuator angle. Defers to the slower method if it fails. Returns the number of missed steps. (e.g. returns +2 if expected position is 50,100 but measured angle is 102). This method ends when either range is exhausted (set by independent variable QuickAngleCheckSearchDeviation), or when it finds a voltage deviation less than dV/2. SlowAngleCheck has no such requirement, and finds minimum voltage scanning over range of actuator steps.
{
    int ExpectedAngle = ExpectedPosition.Angle;
    float MeasuredVoltage = MeasureVoltage();
    //search the Encoder_Calibration array at the expected angle index, and p/m two indices around this index (making sure we dont go out of range).
    int IndexDeviation = 0;
    float VoltageDeviation;
    VoltageDeviation = std::fabs(MeasuredVoltage - EncoderCalibration[ExpectedAngle]);
    if (VoltageDeviation < (dV / 2))//nearest step
    {
        return IndexDeviation;
    }
    IndexDeviation++;
    int EvaluatedIndex;
    while (IndexDeviation <= QuickAngleCheckRange) {

        EvaluatedIndex = ExpectedAngle + IndexDeviation;
        if (EvaluatedIndex >= StepsPerRevolution) {
            EvaluatedIndex = EvaluatedIndex % StepsPerRevolution;
        }
        VoltageDeviation = std::fabs(MeasuredVoltage - EncoderCalibration[EvaluatedIndex]);
        if (VoltageDeviation < (dV / 2))//nearest step
        {
            return IndexDeviation;
        }

        EvaluatedIndex = ExpectedAngle - IndexDeviation;
        if (EvaluatedIndex < 0) {
            EvaluatedIndex = EvaluatedIndex + StepsPerRevolution;
        }
        VoltageDeviation = std::fabs(MeasuredVoltage - EncoderCalibration[EvaluatedIndex]);
        if (VoltageDeviation < (dV / 2))//nearest step
        {
            return -IndexDeviation;
        }
        IndexDeviation++;

    }
    return SlowAngleCheck(ExpectedPosition);
}

int Actuator::SlowAngleCheck(Position ExpectedPosition) {
    int CurrentAngle = MeasureAngle();
    int IndexDeviation;
    int first_query = std::abs(ExpectedPosition.Angle - (CurrentAngle - StepsPerRevolution));
    int second_query = std::abs(ExpectedPosition.Angle - CurrentAngle);
    int third_query = std::abs(ExpectedPosition.Angle - (CurrentAngle + StepsPerRevolution));
    if ((first_query < second_query) && (first_query < third_query)) {
        IndexDeviation = -(ExpectedPosition.Angle - CurrentAngle + StepsPerRevolution);
    } else if ((third_query < first_query) && (third_query < second_query)) {
        IndexDeviation = -(ExpectedPosition.Angle - CurrentAngle - StepsPerRevolution);
    } else {
        IndexDeviation = -(ExpectedPosition.Angle - CurrentAngle);
    }
    return IndexDeviation;
}


int Actuator::Step(int InputSteps)//Positive Step is Extension of Motor
{
    DEBUG_MSG("Stepping Actuator " << SerialNumber << " " << InputSteps << " steps");
    if (ErrorStatus == DeviceState::FatalError)//don't move actuator if there's a fatal error.
    {
        return InputSteps;
    }
    LoadStatusFromASF();
    CheckCurrentPosition();
    Position FinalPosition = PredictPosition(currentPosition, -InputSteps);
    Position PredictedPosition;
    int MissedSteps;
    int StepsTaken;
    int Sign;
    int StepsRemaining = -(CalculateStepsFromHome(FinalPosition) - CalculateStepsFromHome(
            currentPosition));//negative because positive step is retraction, and (0,0) is defined as full extraction.

    if (InputSteps < 0) {
        Sign = -1;
    } else {
        Sign = 1;
    }

    int StepsToTake = Sign * RecordingInterval;
    bool KeepStepping = true;


    while ((KeepStepping == true) && (ErrorStatus != DeviceState::FatalError)) {
        if (std::abs(StepsRemaining) <= RecordingInterval) {
            StepsToTake = StepsRemaining;
            KeepStepping = false;
        }
        PredictedPosition = PredictPosition(currentPosition, -StepsToTake);
        cbc->driver.step(PortNumber, StepsToTake);
        MissedSteps = QuickAngleCheck(
                PredictedPosition);//negative*negative=positive sign because retraction is increasing internal counter and missed steps is negative by definition.
        if (m_Errors[7] == true)//if voltage measurement has issues.
        {
            return StepsRemaining;
        }

        StepsTaken = StepsToTake - MissedSteps;
        SetCurrentPosition(PredictPosition(currentPosition, -StepsTaken));

        //if( (std::abs(MissedSteps)/float(std::abs(StepsToTake)))>TolerablePercentOfMissedSteps && std::abs(MissedSteps)>MinimumMissedStepsToFlagError)//if the actuator misses a certain percent of steps AND misses more than a threshold number of steps.
        if (std::abs(MissedSteps) >
            std::max(int(TolerablePercentOfMissedSteps * std::abs(StepsToTake)), MinimumMissedStepsToFlagError)) {
            ERROR_MSG("Fatal Error: Actuator " << SerialNumber << " missed a large number of steps (" << MissedSteps
                                               << ").");
            SetError(8);//fatal
            RecordStatusToASF();
            return StepsRemaining;//quit, don't record or register steps attempted to be taken.
        }

        RecordStatusToASF();
        StepsRemaining = -(CalculateStepsFromHome(FinalPosition) - CalculateStepsFromHome(currentPosition));
    }
    return StepsRemaining;
}

float Actuator::measureLength() {
    DEBUG_MSG("Measuring Actuator Length for Actuator " << m_SerialNumber);
    loadStatusFromASF();
    checkCurrentPosition();
    int StepsFromHome = CalculateStepsFromHome(currentPosition);
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

Actuator::Position Actuator::predictPosition(Position InputPosition,
                                             int InputSteps)//Positive steps goes in positive direction of counter, which is *retraction* of actuator. (0,0 defined in extended state)
{
    Position PredictedPosition;
    int InputStepsFromHome = CalculateStepsFromHome(InputPosition);
    int PredictedStepsFromHome = InputStepsFromHome + InputSteps;
    PredictedPosition.Revolution = PredictedStepsFromHome / StepsPerRevolution;
    PredictedPosition.Angle = PredictedStepsFromHome % StepsPerRevolution;
    if (PredictedStepsFromHome < 0)//integer division rounds towards zero, we want floor behavior.
    {
        PredictedPosition.Revolution--;
        PredictedPosition.Angle += StepsPerRevolution;
    }
    return PredictedPosition;
}

int Actuator::performHysteresisMotion(int inputSteps) {
    DEBUG_MSG("Performing Hysteresis Motion of " << inputSteps << " for Actuator " << m_SerialNumber);
    int stepsRemaining = step(inputSteps - hysteresisSteps);
    return step(hysteresisSteps + stepsRemaining);
}

//Port, Serial, ASFPath, and sometimes DB are loaded. The rest of the loading needs to be designed here. Set Current Position
void Actuator::initialize() {
    //check if ASF file exists. if it doesn't, create it.
    DEBUG_MSG("Initializing Actuator " << m_SerialNumber);
    loadStatusFromASF();
    readConfigurationAndCalibration();
    checkCurrentPosition();
    checkErrorStatus();
}

void
Actuator::CheckCurrentPosition()//consolidates current position and recovers position (if not too far away). This is typically ran after reading status from ASF.
{
    DEBUG_MSG("Actuator: Checking current position...");
    int IndexDeviation = QuickAngleCheck(currentPosition);
    if (m_Errors[7] == true)//check for voltage issue
    {
        return;
    }
    if (IndexDeviation == 0) {
        return;
    }
    if (std::abs(IndexDeviation) < FlaggedRecoverySteps) {
        SetCurrentPosition(PredictPosition(currentPosition, IndexDeviation));
        RecordStatusToASF();
        return;
    } else if (std::abs(IndexDeviation) <
               MaxRecoverySteps)//If the difference between where we are and where we think we are is high, set an OperableError.
    {
        ERROR_MSG("Operable Error: Actuator " << SerialNumber << " is " << IndexDeviation
                                              << " steps away from the last believed position. This number is large enough to warrant an error, but below the set maximum number of recoverable steps ("
                                              << MaxRecoverySteps
                                              << "). Software will recover the position, but this error should not happen under normal conditions.");
        SetError(10);//operable
        SetCurrentPosition(PredictPosition(currentPosition, IndexDeviation));
        RecordStatusToASF();
        return;
    } else { //If the difference between where we are and where we think we are is extremely high, set a FatalError. Position is lost. (Do we want to set fatal error?? maybe we just recover and set homeisset=false.
        ERROR_MSG("Fatal Error: Actuator " << SerialNumber << " is " << IndexDeviation
                                           << " steps away from the last believed position. This number is above the set maximum number of recoverable steps ("
                                           << MaxRecoverySteps
                                           << "). Home position will likely need to be found again.");
        DEBUG_MSG("CurrentPosition: (" << currentPosition.Revolution << "," << currentPosition.Angle
                                       << "), Probable Position: ("
                                       << PredictPosition(currentPosition, IndexDeviation).Revolution << ", "
                                       << PredictPosition(currentPosition, IndexDeviation).Angle << ")");
        SetError(9);//fatal
        SetError(0);
        RecordStatusToASF();
        return;
    }
}

void Actuator::SetASFFullPath(ASFFileLocation InputASFInfo) {
    std::stringstream Path;
    Path << InputASFInfo.Directory << InputASFInfo.FilenamePrefix << SerialNumber << InputASFInfo.FilenameSuffix;
    ASFFullPath = Path.str();
    NewASFFullPath = ASFFullPath + ".new";
    OldASFFullPath = ASFFullPath + ".old";
}

void Actuator::setDB(DBStruct DBInfo) {
    m_DBInfo = DBInfo;
    unsetError(1);
}

void Actuator::unsetDB() {
    setError(1);
}

void Actuator::SetmmPerStep(float InputmmPerStep) {
    mmPerStep = InputmmPerStep;
}

void Actuator::SetStepsPerRevolution(int InputStepsPerRevolution) {
    StepsPerRevolution = InputStepsPerRevolution;
}

void Actuator::SetRecordingInterval(int InputRecordingInterval) {
    RecordingInterval = InputRecordingInterval;
}

void Actuator::SetHomeLength(float InputHomeLength, float InputCalibrationTemperature) {
    HomeLength = InputHomeLength;
    CalibrationTemperature = InputCalibrationTemperature;
}

int Actuator::CalculateStepsFromHome(Position InputPosition) {
    return InputPosition.Revolution * StepsPerRevolution + InputPosition.Angle;
}

void Actuator::SetError(int CodeNumber) {
    DEBUG_MSG("Setting Error " << CodeNumber << " (" << ActuatorErrors[CodeNumber].ErrorDescription << ") for Actuator "
                               << SerialNumber);
    ActuatorErrors[CodeNumber].Triggered = true;
    SetStatus(ActuatorErrors[CodeNumber].ErrorType);
    return;
}

void Actuator::UnsetError(int CodeNumber) {
    DEBUG_MSG(
            "Unsetting Error " << CodeNumber << "(" << ActuatorErrors[CodeNumber].ErrorDescription << ") for Actuator "
                               << SerialNumber);
    ActuatorErrors[CodeNumber].Triggered = false;
    CheckErrorStatus();
    return;
}

void Actuator::SetStatus(StatusModes InputStatus) {
    if (ErrorStatus == DeviceState::FatalError) {
        return;
    } else {
        DEBUG_MSG("Setting Actuator " << SerialNumber << " Status to " << InputStatus);
        ErrorStatus = InputStatus;
    }
    return;
}

void Actuator::CheckErrorStatus()//cycle through all errors and set status based on ones triggered.
{
    ErrorStatus = DeviceState::Healthy;
    for (int i = 0; i < NUM_ERROR_TYPES; i++) {
        if (ActuatorErrors[i].Triggered == true) {
            SetStatus(ActuatorErrors[i].ErrorType);
        }
    }
    return;
}

void Actuator::ProbeHome()//method used to define home.
{
    DEBUG_MSG("Probing Home for Actuator " << SerialNumber);
    ProbeExtendStop();

    float MeasuredVoltage = MeasureVoltage();
    if (m_Errors[7] == true) {
        return;
    }
    float ExtendStopVoltageMax = VMax - (StepsPerRevolution / 4) * dV;
    float ExtendStopVoltageMin = VMin + (StepsPerRevolution / 4) * dV;
    if (MeasuredVoltage > ExtendStopVoltageMax || MeasuredVoltage < ExtendStopVoltageMin) {
        ERROR_MSG("Operable Error: Actuator " << SerialNumber << " voltage at Extend Stop reads: " << MeasuredVoltage
                                              << ". Encoder should have been set during assembly to have a voltage in the mid-range, between "
                                              << ExtendStopVoltageMin << "-" << ExtendStopVoltageMax
                                              << " volts. Can possibly cause " << StepsPerRevolution
                                              << " step uncertainty in position.");
        SetError(11);//operable
        RecordStatusToASF();
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
        cbc->driver.step(PortNumber, -1);//step once, negative is retraction.
        StepsFromExtendStop++;
        VoltageAfter = MeasureVoltage();
        if (m_Errors[7] == true) {
            return;
        }
        if ((VoltageAfter - VoltageBefore) <
            0)//a negative step increases the voltage by dV. if we detect a voltage that is decreasing...
        {
            AbsDeltaVoltage = std::fabs(VoltageAfter - VoltageBefore);
            if (AbsDeltaVoltage > ((dV * StepsPerRevolution) / 2))//if we jump voltage greater than half of the range.
            {
                CurrentCyclesFromExtendStop++;
            } else//error must have occured.. probably stuck
            {
                ERROR_MSG("Fatal Error: Actuator " << SerialNumber
                                                   << " appears to be stuck at the end stop. Actuator is stepping just a couple of steps backwards instead of forwards.");
                SetError(0);
                //SetError(14);
                RecordStatusToASF();
                return;
            }
        }
        if (CurrentCyclesFromExtendStop == CyclesDefiningHome) {
            NotReachedHome = false;
        }
    }
    int RecordedStepsFromExtendStop = -1 * (CalculateStepsFromHome(ExtendStop));
    int StepsDeviationFromExtendStop = RecordedStepsFromExtendStop - StepsFromExtendStop;
    if (std::abs(StepsDeviationFromExtendStop) > ExtendStopToHomeStepsDeviation) {
        ERROR_MSG("Operable Error: Actuator " << SerialNumber << " is " << StepsDeviationFromExtendStop
                                              << " steps away from Recorded Extend Stop position.");
        SetError(
                13);//operable. if home is ill defined, we should still be able to move the actuator. Also, if internal position "ExtendStop" is not correct, we should still be able to move actuator.
    }
    //Actuator::Position
    Position HomePosition;
    HomePosition.Revolution = 0;
    HomePosition.Angle = 0;
    SetCurrentPosition(HomePosition);
    UnsetError(0);
    RecordStatusToASF();
    return;
}

void Actuator::FindHomeFromEndStop(int Direction)//use recorded extendstop and set actuator to that.
{
    //if direction=1, probeextendstop then set current position to recorded extendstop. compare the number of steps away from the recorded value, and report error if this number is too high.
    Position TargetPosition;
    if (Direction == 1) {
        TargetPosition = ExtendStop;
        ProbeExtendStop();
    } else if (Direction == -1) {
        TargetPosition = RetractStop;
        ProbeRetractStop();
    } else {
        return;
    }
    int IndexDeviation = SlowAngleCheck(TargetPosition);
    SetCurrentPosition(PredictPosition(TargetPosition, IndexDeviation));
    if (std::abs(IndexDeviation) > EndStopRecoverySteps) {
        ERROR_MSG("Operable Error: Actuator " << SerialNumber << " has End Stop which is " << IndexDeviation << " (mod "
                                              << StepsPerRevolution
                                              << ") steps away from recorded End Stop position. Home Position is possibly a cycle off! ProbeHome() needs to be called to more accurately calibrate Home Position.");
        SetError(12);//operable, we still want to move the actuator.
        RecordStatusToASF();
    } else {
        UnsetError(0);
        RecordStatusToASF();
        //Step here to check if we are stuck?
        int StepsRemaining = Step(-1 * Direction * RecordingInterval);
        if (std::abs(StepsRemaining) > (RecordingInterval / 2))//If we miss more than half of the steps
        {
            ERROR_MSG("Fatal Error: Actuator " << SerialNumber << " appears to be stuck at the end stop.");
            SetError(0);
            //SetError(14);
            RecordStatusToASF();
        } else {
            Step(1 * Direction * RecordingInterval);
        }
    }
    return;
}

void Actuator::probeEndStop(int direction) {
    setError(0); // Set home position not found error until probe home is complete

    int searchSteps = direction * endstopSearchStepsize;
    if (m_Errors[7] == true) {
        return;
    }

    bool atStop = false;

    float VoltageBefore = measureVoltage();
    float VoltageAfter = measureVoltage();
    float AbsDeltaVoltage;
    while (!atStop) {
        VoltageBefore = VoltageAfter;
        cbc->driver.step(PortNumber, SearchSteps);
        VoltageAfter = measureVoltage();
        if (m_Errors[7] == true) {
            return;
        }
        AbsDeltaVoltage = std::fabs(VoltageAfter - VoltageBefore);
        if (AbsDeltaVoltage < std::fabs(dV * SearchSteps * StoppedSteppingFactor)) {
            atStop = true;
        }
    }
    return;
}

void Actuator::createDefaultASF()//hardcoded structure of the ASF file (year,mo,day,hr,min,sec,rev,angle,errorcodes)
{
    DEBUG_MSG("Creating ASF File with location: " << ASFFullPath);
    copyFile(ASFFullPath, OldASFFullPath); // Create a copy of the current ASF file contents (the "old" ASF file)

    std::ofstream ASFfile(NewASFFullPath); // Write new default ASF file contents to a separate (the "new" ASF file)
    ASFfile << "2000 1 1 0 0 0 50 0"; // year month day hour minute second revolution angle
    for (int i = 0; i < NUM_ERROR_TYPES; i++) {
        if (i == 0) {
            ASFfile << " 1"; //set error code 0 to true, meaning home is not found.
        } else {
            ASFfile << " 0";
        }
    }
    ASFfile << std::endl;
    ASFfile.close();

    copyFile(NewASFFullPath, ASFFullPath); // Copy the "new" ASF file to the current ASF file location to overwrite
}

void Actuator::clearAllErrors() {
    DEBUG_MSG("Clearing All Errors for Actuator " << m_SerialNumber);
    for (int i = 0; i < NUM_ERROR_TYPES; i++) {
        m_Errors[i] = false;
    }
    checkErrorStatus();
    recordStatusToASF();
}

bool Actuator::forceRecover() {
    ERROR_MSG("Actuator: Force recovering...");
    int indexDeviation = quickAngleCheck(currentPosition);
    if (m_Errors[7] == true)//check for voltage issue
    {
        return false;
    }
    setCurrentPosition(predictPosition(currentPosition, indexDeviation));
    recordStatusToASF();
    return true;
}

void Actuator::copyFile(std::string srcFilePath, std::string destFilePath) {
    std::ifstream srcFile(srcFilePath, std::ios::binary);
    std::ofstream destFile(destFilePath, std::ios::binary);

    destFile << srcFile.rdbuf();

    srcFile.close();
    destFile.close();
}

int DummyActuator::step(int inputSteps)//Positive Step is Extension of Motor
{
    std::cout << "DummyActuator: Stepping Actuator " << m_SerialNumber << " " << inputSteps << " steps" << std::endl;
    if (m_State == DeviceState::FatalError) {
        ERROR_MSG("DummyActuator: Fatal error. Cannot step.");
        return inputSteps;
    }
    Position finalPosition = predictPosition(currentPosition, -inputSteps);
    setCurrentPosition(finalPosition); // Set current position to final position in SIMMODE
    int stepsRemaining = -(calculateStepsFromHome(finalPosition) - calculateStepsFromHome(
            currentPosition));//negative because positive step is retraction, and (0,0) is defined as full extraction.
    std::cout << "DummyActuator:: Steps Remaining = " << stepsRemaining << std::endl;
    return stepsRemaining;
}

float DummyActuator::measureLength() {
    DEBUG_MSG("Measuring Actuator Length for Dummy Actuator " << m_SerialNumber);
    int stepsFromHome = calculateStepsFromHome(currentPosition);
    float distanceFromHome = stepsFromHome * mmPerStep;
    float currentLength = homeLength - distanceFromHome;
    return currentLength;
}

void DummyActuator::initialize() {
    DEBUG_MSG("Initializing Dummy Actuator ...");
}
