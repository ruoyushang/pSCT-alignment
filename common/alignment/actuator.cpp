#ifdef DEBUG
#define DEBUG_MSG(str) do {std::cout << str << std::endl;} while (false)
#else
#define DEBUG_MSG(str) do {} while (false)
#endif

#define ERROR_MSG(str) do {std::cout << str << std::endl;} while (false)

#include <cmath>
#include "actuator.hpp"
#include <fstream>
#include <sstream>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

//Destructor
Actuator::~Actuator()
{
}

//Default Constructor
Actuator::Actuator() : cbc(0) {} //pass cbc pointer. In this case, pass zero. Actuator cannot move without properly passing cbc object.

//Emergency constructor, pass port number and no serial and do not attempt to log actuator position information.
Actuator::Actuator(CBC* InputCBC, int InputPortNumber) : cbc(InputCBC)
{
    DEBUG_MSG("Constructing Actuator with no Serial and Port " << PortNumber);
    SetPortNumber(InputPortNumber);
    SetSerialNumber(InputPortNumber);

    //Create custom emergency path since actuator serial is not passed.
    std::stringstream Path;
    Path << EmergencyASFInfo.Directory << EmergencyASFInfo.FilenamePrefix << PortNumber << EmergencyASFInfo.FilenameSuffix;
    ASFFullPath=Path.str();
    NewASFFullPath=ASFFullPath+".new";
    OldASFFullPath=ASFFullPath+".old";

    EncoderCalibration.resize(StepsPerRevolution);
    for (int i=0; i<StepsPerRevolution; i++)
    {
        EncoderCalibration[i]=VMin+(i*dV);
    }

    Initialize();
}

Actuator::Actuator(CBC* InputCBC, int InputPortNumber, int InputActuatorSerial) : cbc(InputCBC)//Emergency constructor, should be used in emergency scenarios. serial number should be passed to keep track of actuator positions (so they dont default to 0).
{
    DEBUG_MSG("Constructing Actuator " << InputActuatorSerial << " with Port " << InputPortNumber);
    SetPortNumber(InputPortNumber);
    SetSerialNumber(InputActuatorSerial);
    SetASFFullPath(DefaultASFInfo);
    EncoderCalibration.resize(StepsPerRevolution);
    for (int i=0; i<StepsPerRevolution; i++)
    {
        EncoderCalibration[i]=VMin+(i*dV);
    }
    Initialize();
}

Actuator::Actuator(CBC* InputCBC, int InputPortNumber, int InputActuatorSerial, DBStruct InputDBInfo) : cbc(InputCBC)//Normally used constructor WITHOUT ASFStruct. Uses Default ASFStruct information. Set it up accordingly.
{
    DEBUG_MSG("Constructing Actuator " << InputActuatorSerial << " with Port " << InputPortNumber << " and DB Information");
    SetPortNumber(InputPortNumber);
    SetSerialNumber(InputActuatorSerial);
    SetASFFullPath(DefaultASFInfo);
    SetDB(InputDBInfo);
    EncoderCalibration.resize(StepsPerRevolution);
    for (int i=0; i<StepsPerRevolution; i++)
    {
        EncoderCalibration[i]=VMin+(i*dV);
    }
    Initialize();
}

Actuator::Actuator(CBC* InputCBC, int InputPortNumber, int InputActuatorSerial, DBStruct InputDBInfo, ASFStruct InputASFInfo) : cbc(InputCBC)//Normally used constructor WITH ASFStruct. I envision this constructor being used only in special occasions, where the ASF path wants to be specifically defined. Set it up accordingly.
{
    DEBUG_MSG("Constructing Actuator " << InputActuatorSerial << " with Port " << InputPortNumber << " and DB and ASF Information");
    SetPortNumber(InputPortNumber);
    SetSerialNumber(InputActuatorSerial);
    SetASFFullPath(InputASFInfo);
    SetDB(InputDBInfo);
    EncoderCalibration.resize(StepsPerRevolution);
    for (int i=0; i<StepsPerRevolution; i++)
    {
        EncoderCalibration[i]=VMin+(i*dV);
    }
    Initialize();
}

void Actuator::ReadConfigurationAndCalibration()//needs to be fixed to new database structure
{
	DEBUG_MSG("Reading Configuration and Calibration Information from DB for Actuator " << SerialNumber);
	//check to make sure number of columns match what is expected.
	if(DBFlag)
	{
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

			std::stringstream stmtvar;
			stmtvar << "SELECT * FROM Opt_ActuatorConfigurationAndCalibration WHERE serial_number=" << SerialNumber << " ORDER BY start_date DESC LIMIT 1";
			stmt->execute(stmtvar.str());
			res = stmt->getResultSet();
			while (res->next())
			{
				mmPerStep=res->getDouble(4);
				StepsPerRevolution=res->getInt(5);
				HomeLength=res->getDouble(6);
				RetractStop.Revolution=res->getInt(7);
				RetractStop.Angle=res->getInt(8);
				ExtendStop.Revolution=res->getInt(9);
				ExtendStop.Angle=res->getInt(10);
				RecordingInterval=res->getInt(11);
				CalibrationTemperature=res->getDouble(12);
				HysteresisSteps=res->getInt(13);
				RetractRevolutionLimit=res->getInt(14);
				ExtendRevolutionLimit=res->getInt(15);
				MaxVoltageMeasurementAttempts=res->getInt(16);
				StdDevRemeasure=res->getDouble(17);
				StdDevMax=res->getDouble(18);
				QuickAngleCheckRange=res->getInt(19);
				EndstopSearchStepsize=res->getInt(20);
				CyclesDefiningHome=res->getInt(21);
				MinimumMissedStepsToFlagError=res->getInt(22);
				TolerablePercentOfMissedSteps=res->getDouble(23);
				ExtendStopToHomeStepsDeviation=res->getInt(24);
				FlaggedRecoverySteps=res->getInt(25);
				MaxRecoverySteps=res->getInt(26);
				EndStopRecoverySteps=res->getInt(27);
			}
			EncoderCalibration.resize(StepsPerRevolution);
			for (int i=0; i<StepsPerRevolution; i++)
			{
				stmtvar.str(std::string());
				stmtvar << "SELECT * FROM Opt_ActuatorMotorProfile WHERE (serial_number=" << SerialNumber << " and angle=" << i << ") ORDER BY start_date DESC LIMIT 1";
				stmt->execute(stmtvar.str());
				res = stmt->getResultSet();
				while (res->next())
				{
					EncoderCalibration[i]=res->getDouble(5);
				}
			}
			VMin=EncoderCalibration[0];
			VMax=EncoderCalibration[StepsPerRevolution-1];
			dV=(VMax-VMin)/(StepsPerRevolution-1);
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
			std::cout << "Actuator Serial: " << SerialNumber << std::endl;
			ERROR_MSG("Operable Error: SQL Exception caught for Actuator " << SerialNumber << ". Did not successfully communicate with database.");
			SetError(2);//operable
			RecordStatusToASF();
			return;
		}
	}
	else
	{
		ERROR_MSG("Operable Error: DBFlag is not set for Actuator " << SerialNumber << ". Cannot read Configuration and Calibration from DB.");
		SetError(1);//operable
		RecordStatusToASF();
	}
	return;
}

bool Actuator::ReadStatusFromDB(StatusStruct & RecordedPosition)//read all error codes from DB. Check size of error codes to make sure version is consistent. Adjust this function to the new database table structure.
{
	//check that the number of columns matches what is expected
	DEBUG_MSG("Reading Status from DB for Actuator " << SerialNumber);
	if(DBFlag)
	{
		try {
			sql::Driver *driver;
			sql::Connection *con;
			sql::Statement *stmt;
			sql::ResultSet *res;
			sql::ResultSetMetaData *resmeta; 

			driver = get_driver_instance();
			std::string hoststring="tcp://"+DBInfo.ip+":"+DBInfo.port;
			con = driver->connect(hoststring,DBInfo.user,DBInfo.password);
			con->setSchema(DBInfo.dbname);
			stmt = con->createStatement();

			std::stringstream stmtvar;
			stmtvar << "SELECT * FROM Opt_ActuatorStatus WHERE serial_number=" << SerialNumber << " ORDER BY id DESC LIMIT 1";
			stmt->execute(stmtvar.str());
			res = stmt->getResultSet();
			resmeta = res->getMetaData();
			//check if number of results match what is expected. if not, set error(3)
			if(resmeta->getColumnCount() != NumberOfColumnsInDB)
			{
				ERROR_MSG("Fatal Error: DB Status number of arguments (" << resmeta->getColumnCount() << ") did not equal the number expected (" << NumberOfColumnsInDB << "). Either DB or this code appears to have an incorrect structure.");
				SetError(3);//fatal
				RecordStatusToASF();
				return false;
			}

			while (res->next())
			{
				std::string date=res->getString(3);
				sscanf(date.c_str(), "%d-%d-%d %d:%d:%d", &RecordedPosition.Date.Year, &RecordedPosition.Date.Month, &RecordedPosition.Date.Day, &RecordedPosition.Date.Hour, &RecordedPosition.Date.Minute, &RecordedPosition.Date.Second);
				RecordedPosition.Position.Revolution=res->getInt(4);
				RecordedPosition.Position.Angle=res->getInt(5);
				RecordedPosition.ErrorCodes.resize(NumberOfErrorCodes);
				for (int i=0; i<NumberOfErrorCodes; i++)
				{
					RecordedPosition.ErrorCodes[i]=res->getInt(6+i);
				}
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
			std::cout << "Actuator Serial: " << SerialNumber << std::endl;
			ERROR_MSG("Operable Error: SQL Exception caught for Actuator " << SerialNumber << ". Did not successfully communicate with database.");
			SetError(2);//operable, If actuator status cannot be read, stil allow actuator to be moved. Local text file can still be used.
			RecordStatusToASF();
			return false;
		}

	}
	else
	{
		ERROR_MSG("Operable Error: DBFlag is not set for Actuator " << SerialNumber << ". Cannot read Status from DB.");
		SetError(1);//operable
		RecordStatusToASF();
	}
	return true;
}

void Actuator::LoadStatusFromDB()//read all error codes from DB. Check size of error codes to make sure version is consistent. Adjust this function to the new database table structure.
{
	StatusStruct RecordedPosition;
	if(ReadStatusFromDB(RecordedPosition))
	{
		CurrentPosition.Revolution=RecordedPosition.Position.Revolution;
		CurrentPosition.Angle=RecordedPosition.Position.Angle;
		for (int i=0; i<NumberOfErrorCodes; i++)
		{
			if(RecordedPosition.ErrorCodes[i])
			{
				SetError(i);
			}
		}
		if(ActuatorErrors[0].Triggered)
		{
			if(RecordedPosition.ErrorCodes[0]==false)
			{
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
    StatusStruct RecordedPosition;
    if(ReadStatusFromASF(RecordedPosition))
    {
	    if(DBFlag)
	    {
		    try
		    {
			    sql::Driver *driver;
			    sql::Connection *con;
			    sql::Statement *stmt;

			    driver = get_driver_instance();
			    std::string hoststring="tcp://"+DBInfo.ip+":"+DBInfo.port;
			    con = driver->connect(hoststring,DBInfo.user,DBInfo.password);
			    con->setSchema(DBInfo.dbname);
			    stmt = con->createStatement();

			    std::stringstream stmtvar;
			    
			    std::string datestring=std::to_string(RecordedPosition.Date.Year)+"-"+std::to_string(RecordedPosition.Date.Month)+"-"+std::to_string(RecordedPosition.Date.Day)+" "+std::to_string(RecordedPosition.Date.Hour)+":"+std::to_string(RecordedPosition.Date.Minute)+":"+std::to_string(RecordedPosition.Date.Second);

			    stmtvar << "INSERT INTO Opt_ActuatorStatus VALUES (null, " << SerialNumber << ", '" << datestring << "', " << RecordedPosition.Position.Revolution << ", " << RecordedPosition.Position.Angle;

			    for (int i=0; i<NumberOfErrorCodes; i++)
			    {
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
			    ERROR_MSG("Operable Error: SQL Exception caught for Actuator " << SerialNumber << ". Did not successfully communicate with database.");
			    SetError(2);//operable, Local textfile can still be used.
			    RecordStatusToASF();
			    return;
		    }
	    }
	    else
	    {
		    ERROR_MSG("Operable Error: DBFlag is not set for Actuator " << SerialNumber << ". Cannot record Status to DB.");
		    SetError(1);//operable
		    RecordStatusToASF();
	    }
    }
    return;
}

bool Actuator::ReadStatusFromASF(StatusStruct & RecordedPosition)//read all error codes from ASF. Check size of error codes to make sure version is consistent. Read whether Home is set or not.
{
	//DEBUG_MSG("Reading Status from ASF File with path " << ASFFullPath);
	std::ifstream ASF(ASFFullPath);
	//if(ASF.bad())//if file does not exist (or possibly other file issues not expected..)
	if(!ASF.good())//if file does not exist (or possibly other file issues not expected..)
	{
		DEBUG_MSG("ASF file was bad for Actuator " << SerialNumber << " with ASF path " << ASFFullPath << ". Assuming it did not exist and will create a default ASF file.");
		ASF.close();
		CreateDefaultASF();
		ASF.open(ASFFullPath);    
		//if(ASF.bad())//check if ASF is good again. If not, set fatal error.
		if(!ASF.good())//check if ASF is good again. If not, set fatal error.
		{
			ERROR_MSG("Fatal Error: Creating ASF file for Actuator " << SerialNumber << " did not resolve problem. File appears corrupt.");
			SetError(4);//fatal
			return false;
		}
	}
	std::string line;
	int word;
	getline(ASF,line);
	std::vector<int> ASFReadArray;
	ASFReadArray.reserve(NumberOfIntsInASF);
	std::istringstream ss(line);
	while(ss.good())
	{
		ss >> word;
		ASFReadArray.push_back(word);
	}
	if(ASFReadArray.size() != NumberOfIntsInASF)
	{
		ERROR_MSG("Fatal Error: ASF file (" << ASFFullPath << ") number of arguments (" << ASFReadArray.size() << ") did not equal the number expected (" << NumberOfIntsInASF << "). ASF File appears to have an incorrect structure.");
		SetError(5);//fatal
		return false;
	}
	//following is ASF structure, hardcoded.
	RecordedPosition.Date.Year=ASFReadArray[0];
	RecordedPosition.Date.Month=ASFReadArray[1];
	RecordedPosition.Date.Day=ASFReadArray[2];
	RecordedPosition.Date.Hour=ASFReadArray[3];
	RecordedPosition.Date.Minute=ASFReadArray[4];
	RecordedPosition.Date.Second=ASFReadArray[5];
	RecordedPosition.Position.Revolution=ASFReadArray[6];
	RecordedPosition.Position.Angle=ASFReadArray[7];

	RecordedPosition.ErrorCodes.resize(NumberOfErrorCodes);
	for (int i=0; i<NumberOfErrorCodes; i++)
	{
		RecordedPosition.ErrorCodes[i]=ASFReadArray[NumberOfIntsInASFHeader+i];
	}
	return true;
}

void Actuator::LoadStatusFromASF()//read all error codes from ASF. Check size of error codes to make sure version is consistent. Read whether Home is set or not.
{
	StatusStruct RecordedPosition;
	if(ReadStatusFromASF(RecordedPosition))
	{
		CurrentPosition.Revolution=RecordedPosition.Position.Revolution;
		CurrentPosition.Angle=RecordedPosition.Position.Angle;
		for (int i=0; i<NumberOfErrorCodes; i++)
		{
			if(RecordedPosition.ErrorCodes[i])
			{
				SetError(i);
			}
		}
		if(ActuatorErrors[0].Triggered)
		{
			if(RecordedPosition.ErrorCodes[0]==false)
			{
				UnsetError(0);
			}
		}
	}
	return;
}

void Actuator::RecordStatusToASF()//record all error codes to ASF.
{
	//DEBUG_MSG("Recording Status for Actuator " << SerialNumber << " to ASF file with path " << ASFFullPath);
	CopyFile(ASFFullPath, OldASFFullPath);
	std::ofstream ASF(NewASFFullPath);

	//if(ASF.bad())//or exist
	if(!ASF.good())//or exist
	{
		ERROR_MSG("Fatal Error: Cannot write to ASF" << SerialNumber << ".log.new, cannot record Status to ASF.");
		SetError(4);//fatal
		return;
	}

	time_t now=time(0);//0 for UTC., unix timing will overflow in 2038.
	struct tm * ptm;
	ptm = gmtime(&now);
	ASF << ptm->tm_year+1900 << " " << ptm->tm_mon+1 << " " << ptm->tm_mday << " " << ptm->tm_hour << " " << ptm->tm_min << " " << ptm->tm_sec << " ";
	ASF << CurrentPosition.Revolution << " " << CurrentPosition.Angle;
	for (int i=0; i<NumberOfErrorCodes; i++)
	{
		ASF << " " << ActuatorErrors[i].Triggered;
	}
	ASF.close();
	CopyFile(NewASFFullPath, ASFFullPath);
	return;
}

float Actuator::MeasureVoltage()
{
    int MeasurementCount=0;
    adcdata=cbc->adc.readEncoder(PortNumber);
    while ( (adcdata.stddev > StdDevRemeasure) && (MeasurementCount < MaxVoltageMeasurementAttempts) )
    {
        adcdata=cbc->adc.readEncoder(PortNumber);
        MeasurementCount++;
    }
    if (adcdata.stddev > StdDevMax)
    {
        ERROR_MSG("Fatal Error: Actuator " << SerialNumber << " voltage measured (" << adcdata.voltage << ") has a standard deviation (" << adcdata.stddev << ") which is greater than the max std dev allowed of " << StdDevMax << " volts");
        VoltageError=true;
        SetError(7);//fatal
        RecordStatusToASF();
    }
    VoltageError=false;
    return adcdata.voltage;
}

int Actuator::MeasureAngle()
{
    float MeasuredVoltage=MeasureVoltage();
    //find minimum deviation from measured voltage and array of voltages. index of this array is the Angle.
    float MinimumDifference=std::fabs( MeasuredVoltage - EncoderCalibration[0] );
    int Index=0;
    float Deviation;
    for (int i=1; i<StepsPerRevolution; i++)
    {
        Deviation=std::fabs(MeasuredVoltage-EncoderCalibration[i]);
        if (Deviation < MinimumDifference)
        {
            MinimumDifference = Deviation;
            Index = i;
        }
    }
    return Index;
}

int Actuator::QuickAngleCheck(PositionStruct ExpectedPosition)//First attempts a quicker, less robust method of determining the current actuator angle. Defers to the slower method if it fails. Returns the number of missed steps. (e.g. returns +2 if expected position is 50,100 but measured angle is 102). This method ends when either range is exhausted (set by independent variable QuickAngleCheckSearchDeviation), or when it finds a voltage deviation less than dV/2. SlowAngleCheck has no such requirement, and finds minimum voltage scanning over range of actuator steps.
{
    int ExpectedAngle=ExpectedPosition.Angle;
    float MeasuredVoltage=MeasureVoltage();
    //search the Encoder_Calibration array at the expected angle index, and p/m two indices around this index (making sure we dont go out of range).
    int IndexDeviation=0;
    float VoltageDeviation;
    VoltageDeviation=std::fabs(MeasuredVoltage-EncoderCalibration[ExpectedAngle]);
    if (VoltageDeviation < (dV/2))//nearest step
    {
        return IndexDeviation;
    }
    IndexDeviation++;
    int EvaluatedIndex;
    while (IndexDeviation<=QuickAngleCheckRange)
    {

        EvaluatedIndex=ExpectedAngle+IndexDeviation;
        if(EvaluatedIndex>=StepsPerRevolution)
        {
            EvaluatedIndex=EvaluatedIndex%StepsPerRevolution;
        }
        VoltageDeviation=std::fabs(MeasuredVoltage-EncoderCalibration[EvaluatedIndex]);
        if (VoltageDeviation < (dV/2))//nearest step
        {
            return IndexDeviation;
        }

        EvaluatedIndex=ExpectedAngle-IndexDeviation;
        if(EvaluatedIndex<0)
        {
            EvaluatedIndex=EvaluatedIndex+StepsPerRevolution;
        }
        VoltageDeviation=std::fabs(MeasuredVoltage-EncoderCalibration[EvaluatedIndex]);
        if (VoltageDeviation < (dV/2))//nearest step
        {
            return -IndexDeviation;
        }
        IndexDeviation++;

    }
    return SlowAngleCheck(ExpectedPosition);
}

int Actuator::SlowAngleCheck(PositionStruct ExpectedPosition)
{
    int CurrentAngle=MeasureAngle();
    int IndexDeviation;
    int first_query=std::abs(ExpectedPosition.Angle - (CurrentAngle - StepsPerRevolution));
    int second_query=std::abs(ExpectedPosition.Angle - CurrentAngle);
    int third_query=std::abs(ExpectedPosition.Angle - (CurrentAngle + StepsPerRevolution));
    if ((first_query < second_query) && (first_query < third_query))
    {
        IndexDeviation=-(ExpectedPosition.Angle - CurrentAngle + StepsPerRevolution);
    }
    else if ((third_query < first_query) && (third_query < second_query))
    {
        IndexDeviation=-(ExpectedPosition.Angle - CurrentAngle - StepsPerRevolution);
    }
    else
    {
        IndexDeviation=-(ExpectedPosition.Angle - CurrentAngle);
    }
    return IndexDeviation;
}


int Actuator::Step(int InputSteps)//Positive Step is Extension of Motor
{
    DEBUG_MSG("Stepping Actuator " << SerialNumber << " " << InputSteps << " steps");
    if(ErrorStatus == StatusModes::FatalError)//don't move actuator if there's a fatal error.
    {
        return InputSteps;
    }
    LoadStatusFromASF();
    CheckCurrentPosition();
    PositionStruct FinalPosition=PredictPosition(CurrentPosition,-InputSteps);
    PositionStruct PredictedPosition;
    int MissedSteps;
    int StepsTaken;
    int Sign;
    int StepsRemaining=-( CalculateStepsFromHome(FinalPosition)-CalculateStepsFromHome(CurrentPosition) );//negative because positive step is retraction, and (0,0) is defined as full extraction.

    if (InputSteps < 0)
    {
        Sign=-1;
    }
    else
    {
        Sign=1;
    }

    int StepsToTake=Sign*RecordingInterval;
    bool KeepStepping=true;


    while ((KeepStepping==true) && (ErrorStatus != StatusModes::FatalError))
    {
        if(std::abs(StepsRemaining)<=RecordingInterval)
        {
            StepsToTake=StepsRemaining;
            KeepStepping=false;
        }
        PredictedPosition=PredictPosition(CurrentPosition,-StepsToTake);
        cbc->driver.step(PortNumber, StepsToTake);
        MissedSteps=QuickAngleCheck(PredictedPosition);//negative*negative=positive sign because retraction is increasing internal counter and missed steps is negative by definition.
        if(VoltageError)//if voltage measurement has issues.
        {
            return StepsRemaining;
        }

        StepsTaken=StepsToTake-MissedSteps;
        SetCurrentPosition(PredictPosition(CurrentPosition,-StepsTaken));

        //if( (std::abs(MissedSteps)/float(std::abs(StepsToTake)))>TolerablePercentOfMissedSteps && std::abs(MissedSteps)>MinimumMissedStepsToFlagError)//if the actuator misses a certain percent of steps AND misses more than a threshold number of steps.
        if( std::abs(MissedSteps) > std::max( int(TolerablePercentOfMissedSteps*std::abs(StepsToTake)) , MinimumMissedStepsToFlagError ) )
        {
            ERROR_MSG("Fatal Error: Actuator " << SerialNumber << " missed a large number of steps (" << MissedSteps << ").");
            SetError(8);//fatal
            RecordStatusToASF();
            return StepsRemaining;//quit, don't record or register steps attempted to be taken.
        }

        RecordStatusToASF();
        StepsRemaining=-(CalculateStepsFromHome(FinalPosition)-CalculateStepsFromHome(CurrentPosition));
    }
    return StepsRemaining;
}

float Actuator::MeasureLength()
{
    DEBUG_MSG("Measuring Actuator Length for Actuator " << SerialNumber);
    LoadStatusFromASF();
    CheckCurrentPosition();
    int StepsFromHome=CalculateStepsFromHome(CurrentPosition);
    float DistanceFromHome=StepsFromHome*mmPerStep;
    float CurrentLength=HomeLength-DistanceFromHome;
    return CurrentLength;
}

float Actuator::MoveToLength(float TargetLength)
{
    float CurrentLength=MeasureLength();
    float LengthToMove=TargetLength-CurrentLength;
    int StepsToTake=std::floor((LengthToMove/mmPerStep)+0.5);
    //int StepsRemaining=Step(StepsToTake);
    Step(StepsToTake);
    CurrentLength=MeasureLength();
    return CurrentLength;
}

float Actuator::MoveDeltaLength(float LengthToMove)
{
    float CurrentLength=MeasureLength();
    float TargetLength=CurrentLength+LengthToMove;
    CurrentLength=MoveToLength(TargetLength);
    float LengthRemaining=TargetLength-CurrentLength;
    return LengthRemaining;
}

Actuator::PositionStruct Actuator::PredictPosition(PositionStruct InputPosition, int InputSteps)//Positive steps goes in positive direction of counter, which is *retraction* of actuator. (0,0 defined in extended state)
{
    PositionStruct PredictedPosition;
    int InputStepsFromHome=CalculateStepsFromHome(InputPosition);
    int PredictedStepsFromHome=InputStepsFromHome+InputSteps;
    PredictedPosition.Revolution=PredictedStepsFromHome/StepsPerRevolution;
    PredictedPosition.Angle=PredictedStepsFromHome%StepsPerRevolution;
    if(PredictedStepsFromHome<0)//integer division rounds towards zero, we want floor behavior.
    {
        PredictedPosition.Revolution--;
        PredictedPosition.Angle+=StepsPerRevolution;
    }
    return PredictedPosition;
}

int Actuator::HysteresisMotion(int InputSteps)
{
    DEBUG_MSG("Performing Hysteresis Motion of " << InputSteps << " for Actuator " << SerialNumber);
    int StepsRemaining=Step(InputSteps-HysteresisSteps);
    return Step(HysteresisSteps+StepsRemaining);
}


void Actuator::Initialize()//Port, Serial, ASFPath, and sometimes DB are loaded. The rest of the loading needs to be designed here. Set Current Position
{
    //check if ASF file exists. if it doesn't, create it.
    DEBUG_MSG("Initializing Actuator " << SerialNumber);
    LoadStatusFromASF();
    ReadConfigurationAndCalibration();
    CheckCurrentPosition();
    CheckErrorStatus();
    //assert(RecordingInterval <= StepsPerRevolution/2);//to avoid ambiguity in revolution position
    //assert(HysteresisSteps <= RecordingInterval);//to make hysteresis steps one clean motion
    //assert((EndstopSearchStepsize >= 10) && (EndstopSearchStepsize <= StepsPerRevolution/2));//must be large enough to be twice greater than potential recoil steps (~5) but smaller than half a cycle to avoid ambiguity in revolution position.
}

void Actuator::SetCurrentPosition(PositionStruct InputPosition)//EXPERIENCED USE ONLY, should be private!
{
    CurrentPosition.Revolution=InputPosition.Revolution;
    CurrentPosition.Angle=InputPosition.Angle;
}

void Actuator::CheckCurrentPosition()//consolidates current position and recovers position (if not too far away). This is typically ran after reading status from ASF.
{
    int IndexDeviation=QuickAngleCheck(CurrentPosition);
    if(VoltageError)//check for voltage issue
    {
        return;
    }
    if (IndexDeviation==0)
    {
        return;
    }
    if (std::abs(IndexDeviation) < FlaggedRecoverySteps)
    {
        SetCurrentPosition(PredictPosition(CurrentPosition,IndexDeviation));
        RecordStatusToASF();
        return;
    }
    else if (std::abs(IndexDeviation) < MaxRecoverySteps)//If the difference between where we are and where we think we are is high, set an OperableError.
    {
        ERROR_MSG("Operable Error: Actuator " << SerialNumber << " is " << IndexDeviation << " steps away from the last believed position. This number is large enough to warrant an error, but below the set maximum number of recoverable steps (" << MaxRecoverySteps << "). Software will recover the position, but this error should not happen under normal conditions.");
        SetError(10);//operable
        SetCurrentPosition(PredictPosition(CurrentPosition,IndexDeviation));
        RecordStatusToASF();
        return;
    }
    else//If the difference between where we are and where we think we are is extremely high, set a FatalError. Position is lost. (Do we want to set fatal error?? maybe we just recover and set homeisset=false.
    {
        ERROR_MSG("Fatal Error: Actuator " << SerialNumber << " is " << IndexDeviation << " steps away from the last believed position. This number is above the set maximum number of recoverable steps (" << MaxRecoverySteps << "). Home position will likely need to be found again.");
        DEBUG_MSG("CurrentPosition: (" << CurrentPosition.Revolution << "," << CurrentPosition.Angle << "), Probable Position: (" << PredictPosition(CurrentPosition,IndexDeviation).Revolution << ", " << PredictPosition(CurrentPosition,IndexDeviation).Angle << ")");
        SetError(9);//fatal
        SetError(0);
        RecordStatusToASF();
        return;
    }
}

void Actuator::SetASFFullPath(ASFStruct InputASFInfo)
{
    std::stringstream Path;
    Path << InputASFInfo.Directory << InputASFInfo.FilenamePrefix << SerialNumber << InputASFInfo.FilenameSuffix;
    ASFFullPath=Path.str();
    NewASFFullPath=ASFFullPath+".new";
    OldASFFullPath=ASFFullPath+".old";
}

void Actuator::SetDB(DBStruct InputDBInfo)
{
    DBInfo=InputDBInfo;
    DBFlag=true;
}

void Actuator::UnsetDB()
{
    DBFlag=false;
}

void Actuator::SetSerialNumber(int InputSerialNumber)//Serial Number 0 is used for calibration.
{
    SerialNumber=InputSerialNumber;
}

void Actuator::UnsetSerialNumber()//Serial Number 0 is used for calibration.
{
    SerialNumber=0;
}

void Actuator::SetPortNumber(int InputPortNumber)//1-6
{
    PortNumber=InputPortNumber;
}

void Actuator::UnsetPortNumber()//0 is unused.
{
    PortNumber=0;
}

void Actuator::SetmmPerStep(float InputmmPerStep)
{
    mmPerStep=InputmmPerStep;
}

void Actuator::SetStepsPerRevolution(int InputStepsPerRevolution)
{
    StepsPerRevolution=InputStepsPerRevolution;
}

void Actuator::SetRecordingInterval(int InputRecordingInterval)
{
    RecordingInterval=InputRecordingInterval;
}

void Actuator::SetHomeLength(float InputHomeLength, float InputCalibrationTemperature)
{
    HomeLength=InputHomeLength;
    CalibrationTemperature=InputCalibrationTemperature;
}

int Actuator::CalculateStepsFromHome(PositionStruct InputPosition)
{
    return InputPosition.Revolution*StepsPerRevolution+InputPosition.Angle;
}

void Actuator::SetError(int CodeNumber)
{
    DEBUG_MSG("Setting Error " << CodeNumber << " (" << ActuatorErrors[CodeNumber].ErrorDescription << ") for Actuator " << SerialNumber);
    ActuatorErrors[CodeNumber].Triggered=true;
    SetStatus(ActuatorErrors[CodeNumber].ErrorType);
    return;
}

void Actuator::UnsetError(int CodeNumber)
{
    DEBUG_MSG("Unsetting Error " << CodeNumber << "(" << ActuatorErrors[CodeNumber].ErrorDescription << ") for Actuator " << SerialNumber);
    ActuatorErrors[CodeNumber].Triggered=false;
    CheckErrorStatus();
    return;
}

void Actuator::SetStatus(StatusModes InputStatus)
{
    if (ErrorStatus == StatusModes::FatalError)
    {
        return;
    }
    else
    {
        DEBUG_MSG("Setting Actuator " << SerialNumber << " Status to " << InputStatus);
        ErrorStatus = InputStatus;
    }
    return;
}

void Actuator::CheckErrorStatus()//cycle through all errors and set status based on ones triggered.
{
    ErrorStatus=StatusModes::Healthy;
    for (int i=0; i<NumberOfErrorCodes; i++)
    {
        if(ActuatorErrors[i].Triggered == true)
        {
            SetStatus(ActuatorErrors[i].ErrorType);
        }
    }
    return;
}

////////////////////////////
void Actuator::ProbeHome()//method used to define home.
{
	DEBUG_MSG("Probing Home for Actuator " << SerialNumber);
	ProbeExtendStop();

	float MeasuredVoltage=MeasureVoltage();
	if(VoltageError)
	{
		return;
	}
	float ExtendStopVoltageMax=VMax-(StepsPerRevolution/4)*dV;
	float ExtendStopVoltageMin=VMin+(StepsPerRevolution/4)*dV;
	if(MeasuredVoltage > ExtendStopVoltageMax || MeasuredVoltage < ExtendStopVoltageMin)
	{
		ERROR_MSG("Operable Error: Actuator " << SerialNumber << " voltage at Extend Stop reads: " << MeasuredVoltage << ". Encoder should have been set during assembly to have a voltage in the mid-range, between " << ExtendStopVoltageMin << "-" << ExtendStopVoltageMax << " volts. Can possibly cause " << StepsPerRevolution << " step uncertainty in position.");
		SetError(11);//operable
		RecordStatusToASF();
	}

	float VoltageBefore;
	float AbsDeltaVoltage;
	float VoltageAfter=MeasuredVoltage;
	if(VoltageError)
	{
		return;
	}
	int CurrentCyclesFromExtendStop=0;
	int StepsFromExtendStop=0;
	bool NotReachedHome=true;

	while(NotReachedHome)
	{
		VoltageBefore=VoltageAfter;
		cbc->driver.step(PortNumber, -1);//step once, negative is retraction.
		StepsFromExtendStop++;
		VoltageAfter=MeasureVoltage();
		if(VoltageError)
		{
			return;
		}
		if ((VoltageAfter-VoltageBefore)<0)//a negative step increases the voltage by dV. if we detect a voltage that is decreasing...
		{
			AbsDeltaVoltage=std::fabs(VoltageAfter-VoltageBefore);
			if (AbsDeltaVoltage>((dV*StepsPerRevolution)/2))//if we jump voltage greater than half of the range.
			{
				CurrentCyclesFromExtendStop++;
			}
			else//error must have occured.. probably stuck
			{
				ERROR_MSG("Fatal Error: Actuator " << SerialNumber << " appears to be stuck at the end stop. Actuator is stepping just a couple of steps backwards instead of forwards.");
				SetError(0);
				//SetError(14);
				RecordStatusToASF();
				return;
			}
		}
		if (CurrentCyclesFromExtendStop==CyclesDefiningHome)
		{
			NotReachedHome=false;
		}
	}
	int RecordedStepsFromExtendStop=-1*(CalculateStepsFromHome(ExtendStop));
	int StepsDeviationFromExtendStop=RecordedStepsFromExtendStop-StepsFromExtendStop;
	if (std::abs(StepsDeviationFromExtendStop) > ExtendStopToHomeStepsDeviation)
	{
		ERROR_MSG("Operable Error: Actuator " << SerialNumber << " is " << StepsDeviationFromExtendStop << " steps away from Recorded Extend Stop position.");
		SetError(13);//operable. if home is ill defined, we should still be able to move the actuator. Also, if internal position "ExtendStop" is not correct, we should still be able to move actuator.
	}
	//Actuator::PositionStruct
	PositionStruct HomePosition;
	HomePosition.Revolution=0;
	HomePosition.Angle=0;
	SetCurrentPosition(HomePosition);
	UnsetError(0);
	RecordStatusToASF();
	return;
}

void Actuator::FindHomeFromEndStop(int Direction)//use recorded extendstop and set actuator to that.
{
//if direction=1, probeextendstop then set current position to recorded extendstop. compare the number of steps away from the recorded value, and report error if this number is too high.
PositionStruct TargetPosition;
if(Direction == 1)
{
TargetPosition=ExtendStop;
ProbeExtendStop();
}
else if (Direction == -1)
{
TargetPosition=RetractStop;
ProbeRetractStop();
}
else
{
return;
}
int IndexDeviation=SlowAngleCheck(TargetPosition);
SetCurrentPosition(PredictPosition(TargetPosition,IndexDeviation));
if (std::abs(IndexDeviation) > EndStopRecoverySteps)
{
ERROR_MSG("Operable Error: Actuator " << SerialNumber << " has End Stop which is " << IndexDeviation << " (mod " << StepsPerRevolution << ") steps away from recorded End Stop position. Home Position is possibly a cycle off! ProbeHome() needs to be called to more accurately calibrate Home Position.");
SetError(12);//operable, we still want to move the actuator.
RecordStatusToASF();
}
else
{
UnsetError(0);
RecordStatusToASF();
//Step here to check if we are stuck?
int StepsRemaining=Step(-1*Direction*RecordingInterval);
if(std::abs(StepsRemaining) > (RecordingInterval/2))//If we miss more than half of the steps
{
ERROR_MSG("Fatal Error: Actuator " << SerialNumber << " appears to be stuck at the end stop.");
SetError(0);
//SetError(14);
RecordStatusToASF();
}
else
{
Step(1*Direction*RecordingInterval);
}
}
return;
}

void Actuator::FindHomeFromExtendStop()
{
    DEBUG_MSG("Finding Home from Extend Stop for Actuator " << SerialNumber);
    FindHomeFromEndStop(1);
}

void Actuator::FindHomeFromRetractStop()
{
    DEBUG_MSG("Finding Home from Retract Stop for Actuator " << SerialNumber);
    FindHomeFromEndStop(-1);
}

void Actuator::ProbeEndStop(int Direction)
{
    SetError(0);

    int SearchSteps=Direction*EndstopSearchStepsize;
    float StoppedSteppingFactor=0.5;//hardcoded? 1 means it always stops, 0 means it never stops.
    float VoltageBefore;
    float AbsDeltaVoltage;
    float VoltageAfter=MeasureVoltage();
    if(VoltageError)
    {
        return;
    }
    bool NotReachedStop=true;

    while( NotReachedStop )
    {
        VoltageBefore=VoltageAfter;
        cbc->driver.step(PortNumber, SearchSteps);
        VoltageAfter=MeasureVoltage();
        if(VoltageError)
        {
            return;
        }
        AbsDeltaVoltage=std::fabs(VoltageAfter-VoltageBefore);
        if (AbsDeltaVoltage < std::fabs(dV*SearchSteps*StoppedSteppingFactor))
        {
            NotReachedStop=false;
        }
    }
    return;
}

void Actuator::ProbeExtendStop()
{
    DEBUG_MSG("Probing Extend Stop for Actuator " << SerialNumber);
    ProbeEndStop(1);
}

void Actuator::ProbeRetractStop()
{
    DEBUG_MSG("Probing Retract Stop for Actuator " << SerialNumber);
    ProbeEndStop(-1);
}


/////////////////////////////
int Actuator::GetPortNumber() const
{
    return PortNumber;
}

Actuator::StatusModes Actuator::GetStatus()
{
    return ErrorStatus;
}

/*
void Actuator::RecoverStatusFromDBAndASF()//reads ASF and DB recordings and sets current position to the most recent recording.
{
DEBUG_MSG("Recovering Status from most recent recording between DB and ASF for Actuator " << SerialNumber);
RecordedPositionStruct ASFRecordedPosition;
RecordedPositionStruct DBRecordedPosition;
if(ReadStatusFromASF(ASFRecordedPosition))
{
if(ReadStatusFromDB(DBRecordedPosition))
{
if(ASFRecordedPosition.Date.Year < DBRecordedPosition.Date.Year)
{
LoadStatusFromASF();
CheckCurrentPosition();
}
else if(ASFRecordedPosition.Date.Month < DBRecordedPosition.Date.Month)
{
LoadStatusFromASF();
CheckCurrentPosition();
}
else if(ASFRecordedPosition.Date.Day < DBRecordedPosition.Date.Day)
{
LoadStatusFromASF();
CheckCurrentPosition();
}
else if(ASFRecordedPosition.Date.Hour < DBRecordedPosition.Date.Hour)
{
LoadStatusFromASF();
CheckCurrentPosition();
}
else if(ASFRecordedPosition.Date.Minute < DBRecordedPosition.Date.Minute)
{
LoadStatusFromASF();
CheckCurrentPosition();
}
else if(ASFRecordedPosition.Date.Second < DBRecordedPosition.Date.Second)
{
LoadStatusFromASF();
CheckCurrentPosition();
}
else
{
int CurrentAngle=MeasureAngle();
if(std::abs(CurrentAngle-ASFRecordedPosition.Position.Angle)>FlaggedRecoverySteps)
{
SetError(6);
RecordStatusToASF();
}
else
{
LoadStatusFromDB();
CheckCurrentPosition();
RecordStatusToASF();
}
}
}
}
return;
}
*/

void Actuator::CreateDefaultASF()//hardcoded structure of the ASF file (year,mo,day,hr,min,sec,rev,angle,errorcodes)
{
    DEBUG_MSG("Creating ASF File with location: " << ASFFullPath);
    //its possible this function is called if ASF is not good for other reason, like corrupted. Make sure this function takes that into account.
    CopyFile(ASFFullPath, OldASFFullPath);
    std::ofstream NewASF(NewASFFullPath);
    NewASF << "2000 1 1 0 0 0 50 0";//year month day hour minute second revolution angle
    NewASF << " 1";//set error code 0 to true, meaning home is not found.
    for (int i=1; i<NumberOfErrorCodes; i++)
    {
        NewASF << " 0";
    }
    NewASF << std::endl;
    NewASF.close();
    CopyFile(NewASFFullPath, ASFFullPath);
    //Set errors? home not found? ASF file created?
    return;
}

void Actuator::ClearAllErrors()
{
    DEBUG_MSG("Clearing All Errors for Actuator " << SerialNumber);
    for (int i=0; i<NumberOfErrorCodes; i++)
    {
        ActuatorErrors[i].Triggered=false;
    }
    CheckErrorStatus();
    RecordStatusToASF();
}

void Actuator::ForceRecover()
{
    int IndexDeviation=QuickAngleCheck(CurrentPosition);
    if(VoltageError)//check for voltage issue
    {
        return;
    }
    SetCurrentPosition(PredictPosition(CurrentPosition,IndexDeviation));
    RecordStatusToASF();
    return;
}

void Actuator::CopyFile(std::string srcfile, std::string destfile)
{
	//DEBUG_MSG("Copying " << srcfile << " to " << destfile);
	std::ifstream src(srcfile, std::ios::binary);
	std::ofstream dest(destfile, std::ios::binary);
	dest << src.rdbuf();
	src.close();
	dest.close();
	return;
}

int DummyActuator::Step(int InputSteps)//Positive Step is Extension of Motor
{
    std::cout << "SIMMODE: Stepping Actuator " << SerialNumber << " " << InputSteps << " steps" << std::endl;
    if(ErrorStatus == StatusModes::FatalError)//don't move actuator if there's a fatal error.
    {
        std::cout << "SIMMODE: Fatal error occurs!!!" << std::endl;
        return InputSteps;
    }
    PositionStruct FinalPosition=PredictPosition(CurrentPosition,-InputSteps);
    SetCurrentPosition(FinalPosition); // Set final position as current position in SIMMODE
    int MissedSteps;
    int StepsTaken;
    int Sign;
    int StepsRemaining=-( CalculateStepsFromHome(FinalPosition)-CalculateStepsFromHome(CurrentPosition) );//negative because positive step is retraction, and (0,0) is defined as full extraction.
    std::cout << "SIMMODE: StepsRemaining = " << StepsRemaining << std::endl;
    return StepsRemaining;
}
float DummyActuator::MeasureLength()
{
    //DEBUG_MSG("Measuring Actuator Length for Actuator " << SerialNumber);
    int StepsFromHome=CalculateStepsFromHome(CurrentPosition);
    float DistanceFromHome=StepsFromHome*mmPerStep;
    float CurrentLength=HomeLength-DistanceFromHome;
    return CurrentLength;
}
void DummyActuator::Initialize()//Port, Serial, ASFPath, and sometimes DB are loaded. The rest of the loading needs to be designed here. Set Current Position
{
    //check if ASF file exists. if it doesn't, create it.
    std::cout << "Initializing Dummy Actuator ..." << std::endl;
}
