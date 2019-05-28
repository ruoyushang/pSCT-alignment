#include "client/controllers/mpescontroller.hpp"

#include <fstream>

#include "common/opcua/pasobject.hpp"
#include "common/utilities/DBConfig.hpp"

#include "client/clienthelper.hpp"

#include "mysql_driver.h"
#include "cppconn/statement.h"


float MPESController::kNominalIntensity = 150000.;
float MPESController::kNominalSpotWidth = 10.;
int MPESController::kMaxAttempts = 1;

MPESController::MPESController(Device::Identity identity, std::shared_ptr<Client> pClient) : PasController(
    std::move(identity),
    std::move(pClient)),
                                                                                             m_isVisible(false),
                                                                                             m_numAttempts(0) {
    // get the nominal aligned readings and response matrices from DB
    /* BEGIN DATABASE HACK */
    //std::string db_ip="172.17.10.10"; // internal ip
    DBConfig myConfig = DBConfig::getDefaultConfig();
    std::string db_ip = myConfig.getHost();
    std::string db_port = myConfig.getPort();
    std::string db_user = myConfig.getUser();
    std::string db_password = myConfig.getPassword();
    std::string db_name = myConfig.getDatabase();
    std::string db_address = "tcp://" + db_ip + ":" + db_port;

    //std::cout << "Initializing MPES " << m_ID.serialNumber << std::endl;
    try {
        sql::Driver *sql_driver = get_driver_instance();
        sql::Connection *sql_conn = sql_driver->connect(db_address, db_user, db_password);
        sql_conn->setSchema(db_name);
        sql::Statement *sql_stmt = sql_conn->createStatement();
        sql::ResultSet *sql_results;

        std::string query;
        // obtain topological data
        query = "SELECT w_panel, l_panel FROM Opt_MPESMapping WHERE end_date is NULL and serial_number=" +
                to_string(m_ID.serialNumber);
        sql_stmt->execute(query);
        sql_results = sql_stmt->getResultSet();
        while (sql_results->next()) {
            m_PanelSideMap[sql_results->getInt(1)] = 'w';
            m_PanelSideMap[sql_results->getInt(2)] = 'l';
        }

        // build query configuration -- obtain coord, nominal reading and response matrix elements
        query.clear();
        const char panelType[2] = {'w', 'l'};
        for (const auto &panel : panelType)
            for (int act = 1; act <= 6; act++)
                query += ", " + std::string(1, panel) + "_response_actuator" + to_string(act);
        query = "SELECT coord, nominal_reading" + query +
                " FROM Opt_MPESConfigurationAndCalibration WHERE end_date is NULL and serial_number=" +
                to_string(m_ID.serialNumber);
        sql_stmt->execute(query);
        sql_results = sql_stmt->getResultSet();

        while (sql_results->next()) {
            char coord = sql_results->getString(1)[0];
            m_AlignedReadings(int(coord - 'x')) = sql_results->getDouble(2);
            for (int panel = 0; panel < 2; panel++) {
                for (int act = 1; act <= 6; act++)
                    m_ResponseMatMap[panelType[panel]](int(coord - 'x'), act - 1) =
                            sql_results->getDouble(2 + panel * 6 + act);
            }
        }

        // close the connection!
        sql_conn->close();
        delete sql_conn;
        sql_driver->threadEnd();
    }
    catch (sql::SQLException &e) {
        std::cout << "# ERR: SQLException in " << __FILE__;
        std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
        std::cout << "# ERR: " << e.what();
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    }
    /* END DATABASE HACK */

    m_SystematicOffsets = Eigen::Vector2d::Zero();
}

/* ----------------------------------------------------------------------------
    Class        MPESController
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatus MPESController::getState(Device::DeviceState &state) {
    //UaMutexLocker lock(&m_mutex);    
    UaStatus status;
    UaVariant value;
    int v;
    
    m_pClient->read({m_ID.eAddress + "." + "State"}, &value);
    value.toInt32(v);

    state = static_cast<Device::DeviceState>(v);

    return status;
}

/* ----------------------------------------------------------------------------
    Class        MPESController
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatus MPESController::setState(Device::DeviceState state) {
    return OpcUa_BadNotWritable;
}

/* ----------------------------------------------------------------------------
    Class        MPESController
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatus MPESController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    UaStatus status;
    //UaMutexLocker lock(&m_mutex);

    if (MPESObject::ERRORS.count(offset) > 0) {
        return getError(offset, value);
    } else {
        if (offset == PAS_MPESType_xCentroidAvg || offset == PAS_MPESType_yCentroidAvg ||
            offset == PAS_MPESType_xCentroidSpotWidth ||
            offset == PAS_MPESType_yCentroidSpotWidth || offset == PAS_MPESType_xCentroidNominal ||
            offset == PAS_MPESType_yCentroidNominal ||
            offset == PAS_MPESType_CleanedIntensity) {
            status = read(false);
            if (status.isBad()) {
                std::cout << m_ID << " :: MPESController::getData() : Device is in a bad state (busy, off, error) and "
                                     "could not read data. Check state and try again. \n";
                return status;
            }
        }
        switch (offset) {
            case PAS_MPESType_xCentroidAvg:
                value.setFloat(m_data.xCentroid);
                break;
            case PAS_MPESType_yCentroidAvg:
                value.setFloat(m_data.yCentroid);
                break;
            case PAS_MPESType_xCentroidSpotWidth:
                value.setFloat(m_data.xSpotWidth);
                break;
            case PAS_MPESType_yCentroidSpotWidth:
                value.setFloat(m_data.ySpotWidth);
                break;
            case PAS_MPESType_CleanedIntensity:
                value.setFloat(m_data.cleanedIntensity);
                break;
            case PAS_MPESType_xCentroidNominal:
                value.setFloat(m_data.xNominal);
                break;
            case PAS_MPESType_yCentroidNominal:
                value.setFloat(m_data.yNominal);
                break;
            case PAS_MPESType_Position:
                value.setInt32(m_ID.position);
                break;
            case PAS_MPESType_Serial:
                value.setInt32(m_ID.serialNumber);
                break;
            case PAS_MPESType_ErrorState:
                status = m_pClient->read({m_ID.eAddress + "." + "ErrorState"}, &value);
                break;
            default:
                status = OpcUa_BadInvalidArgument;
                break;
        }
    }

    if (status == OpcUa_BadInvalidState) {
        std::cout << m_ID << " :: MPESController::getData() : Device is in a bad state (busy, off, error) and "
                             "could not read data. Check state and try again. \n";
    }

    return OpcUa_Good;
}

UaStatus MPESController::getError(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (MPESObject::ERRORS.count(offset) > 0) {
        std::string varName = std::get<0>(MPESObject::ERRORS.at(offset));
        std::vector<std::string> varsToRead = {m_ID.eAddress + "." + varName};
        status = m_pClient->read(varsToRead, &value);
    } else {
        status = OpcUa_BadInvalidArgument;
    }
    return status;
}

/* ----------------------------------------------------------------------------
    Class        MPESController
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatus MPESController::setData(OpcUa_UInt32 offset, UaVariant value) {
    return OpcUa_BadNotWritable;
}

/* ----------------------------------------------------------------------------
    Class        MPESController
    Method       Operate
    Description  run a method on the sensor
-----------------------------------------------------------------------------*/
UaStatus MPESController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    std::cout << m_ID << " :: MPESController::operate() : Calling method " << MPESObject::METHODS.at(offset).first
              << std::endl;

    if (offset == PAS_MPESType_Read) {
        status = read(true);
    } else if (offset == PAS_MPESType_SetExposure) {
        status = m_pClient->callMethod(m_ID.eAddress, UaString("SetExposure"), args);
    } else if (offset == PAS_MPESType_ClearError) {
        status = m_pClient->callMethod(m_ID.eAddress, UaString("ClearError"), args);
    } else if (offset == PAS_MPESType_ClearAllErrors) {
        status = m_pClient->callMethod(m_ID.eAddress, UaString("ClearAllErrors"));
    } else if (offset == PAS_MPESType_TurnOn) {
        status = m_pClient->callMethod(m_ID.eAddress, UaString("TurnOn"));
    } else if (offset == PAS_MPESType_TurnOff) {
        status = m_pClient->callMethod(m_ID.eAddress, UaString("TurnOff"));
    } else {
        status = OpcUa_BadInvalidArgument;
    }

    if (status == OpcUa_BadInvalidState) {
        std::cout << m_ID << " :: MPESController::operate() : Device is in a bad state (busy, off, error) and "
                             "could not execute command. Check state and try again. \n";
    }

    return status;

}

/* ----------------------------------------------------------------------------
    Class        MPESController
    Method       read
    Description  Read Controller data.
-----------------------------------------------------------------------------*/
UaStatus MPESController::read(bool print) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    std::vector<std::string> varstoread{
        "xCentroidAvg",
        "yCentroidAvg",
        "xCentroidSpotWidth",
        "yCentroidSpotWidth",
        "CleanedIntensity",
        "xCentroidNominal",
        "yCentroidNominal"};
    std::transform(varstoread.begin(), varstoread.end(), varstoread.begin(),
                   [this](std::string &str) { return m_ID.eAddress + "." + str; });
    UaVariant valstoread[7];

    status = m_pClient->read(varstoread, &valstoread[0]);

    for (unsigned i = 0; i < varstoread.size(); i++)
        valstoread[i].toFloat(*(reinterpret_cast<float *>(&m_data) + i));

    if (print) {
        std::cout << "Reading MPES " << m_ID << ":" << std::endl;
        std::cout << "x: " << m_data.xCentroid << std::endl;
        std::cout << "xNominal: " << m_data.xNominal << std::endl;
        std::cout << "y: " << m_data.yCentroid << std::endl;
        std::cout << "yNominal: " << m_data.yNominal << std::endl;
        std::cout << "xSpotWidth: " << m_data.xSpotWidth << std::endl;
        std::cout << "ySpotWidth: " << m_data.ySpotWidth << std::endl;
        std::cout << "Cleaned Intensity: " << m_data.cleanedIntensity << std::endl;
    }

    if (status == OpcUa_BadInvalidState) {
        if (m_numAttempts <= kMaxAttempts) {
            std::cout << m_ID << " :: MPESController::read() : Device is in a bad state (busy, off, error) and "
                                 "could not read. Waiting and re-trying... \n";
            sleep(1);
            m_numAttempts++;
            // read the sensor again
            status = read(print);
        } else {
            m_numAttempts = 0;
        }
    }

    if (m_data.xCentroid < 0.1 || m_data.yCentroid < 0.1)
        m_isVisible = false;
    else
        m_isVisible = true;

    if (m_isVisible) {
        if (m_data.xSpotWidth > kNominalSpotWidth) {
            std::cout << "+++ WARNING +++ The width of the image along the X axis for " << m_ID.name
                      << " is greater than 20px. Consider fixing things." << std::endl;
        }
        if (m_data.ySpotWidth > kNominalSpotWidth) {
            std::cout << "+++ WARNING +++ The width of the image along the Y axis for " << m_ID.name
                      << " is greater than 20px. Consider fixing things." << std::endl;
        }
        if (fabs(m_data.cleanedIntensity - kNominalIntensity) / kNominalIntensity > 0.2) {
            if (m_numAttempts <= kMaxAttempts) {
                std::cout << "+++ WARNING +++ The intensity of " << m_ID.name
                          << " differs from the magic value by more than 20%\n"
                          << "+++ WARNING +++ Will readjust the exposure now!" << std::endl;
                operate(PAS_MPESType_SetExposure);
                m_numAttempts++;
                // read the sensor again
                status = read(print);
            } else {
                m_numAttempts = 0;
            }
        }
    }

    time_t now = time(nullptr);
    struct tm tstruct{};
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);

    UaString sql_stmt = UaString(
        "INSERT INTO Opt_MPESReadings (date, serial_number, xcoord, ycoord, x_SpotWidth, y_SpotWidth, intensity) VALUES  ('%1', '%2', '%3', '%4', '%5', '%6', '%7' );\n").arg(
        buf).arg(m_ID.serialNumber).arg(m_data.xCentroid).arg(m_data.yCentroid).arg(
        m_data.xSpotWidth).arg(m_data.ySpotWidth).arg(m_data.cleanedIntensity);
    std::ofstream sql_file("MPES_readings.sql", std::ios_base::app);
    sql_file << sql_stmt.toUtf8();

    return status;
}

char MPESController::getPanelSide(unsigned panelpos) {
    char panelside;
    try {
        panelside = m_PanelSideMap.at(panelpos);
    }
    catch (std::out_of_range &e) {
        panelside = 0;
    }

    return panelside;
}

