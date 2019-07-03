#include "client/controllers/mpescontroller.hpp"

#include <fstream>

#include "common/opcua/pasobject.hpp"
#include "common/utilities/DBConfig.hpp"

#include "client/clienthelper.hpp"

#include "mysql_driver.h"
#include "cppconn/statement.h"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"


float MPESController::kNominalIntensity = 150000.;
float MPESController::kNominalSpotWidth = 10.;
int MPESController::kMaxAttempts = 1;

MPESController::MPESController(Device::Identity identity, Client *pClient) : PasController(
    std::move(identity), pClient),
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

    spdlog::debug("{} : Initializing MPES Controller... (connecting to DB)", m_ID);
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
        query = "SELECT coord" + query +
                " FROM Opt_MPESConfigurationAndCalibration WHERE end_date is NULL and serial_number=" +
                to_string(m_ID.serialNumber);
        sql_stmt->execute(query);
        sql_results = sql_stmt->getResultSet();

        while (sql_results->next()) {
            char coord = sql_results->getString(1)[0];
            for (int panel = 0; panel < 2; panel++) {
                for (int act = 1; act <= 6; act++)
                    m_ResponseMatMap[panelType[panel]](int(coord - 'x'), act - 1) =
                        sql_results->getDouble(1 + panel * 6 + act);
            }
        }

        // close the connection!
        sql_conn->close();
        delete sql_conn;
        sql_driver->threadEnd();
    }
    catch (sql::SQLException &e) {
        spdlog::error("# ERR: SQLException in {}"
                      "({}) on line {}\n"
                      "# ERR: {}"
                      " (MySQL error code: {}"
                      ", SQLState: {})", __FILE__, __FUNCTION__, __LINE__, e.what(), e.getErrorCode(), e.getSQLState());
    }

    m_SystematicOffsets = Eigen::Vector2d::Zero();

    spdlog::debug("{} : Done.", m_ID);
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

    m_pClient->read({m_pClient->getDeviceNodeId(m_ID) + "." + "State"}, &value);
    value.toInt32(v);

    state = static_cast<Device::DeviceState>(v);
    spdlog::trace("{} : Getting device state => ({})", m_ID, Device::deviceStateNames.at(state));

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
            offset == PAS_MPESType_yCentroidSpotWidth || 
            offset == PAS_MPESType_CleanedIntensity) {
            status = read(false);
            if (status.isBad()) {
                //spdlog::error("{} : MPESController::operate() : Device is in a bad state (busy, off, error) and "
                //              "could not read data. Check state and try again.", m_ID);
                return status;
            }
        }
        switch (offset) {
            case PAS_MPESType_xCentroidAvg:
                value.setFloat(m_data.xCentroid);
                spdlog::trace("{} : Getting xCentroid value => ({})", m_ID, m_data.xCentroid);
                break;
            case PAS_MPESType_yCentroidAvg:
                value.setFloat(m_data.yCentroid);
                spdlog::trace("{} : Getting yCentroid value => ({})", m_ID, m_data.yCentroid);
                break;
            case PAS_MPESType_xCentroidSpotWidth:
                value.setFloat(m_data.xSpotWidth);
                spdlog::trace("{} : Getting xCentroidSpotWidth value => ({})", m_ID, m_data.xSpotWidth);
                break;
            case PAS_MPESType_yCentroidSpotWidth:
                value.setFloat(m_data.ySpotWidth);
                spdlog::trace("{} : Getting yCentroidSpotWidth value => ({})", m_ID, m_data.ySpotWidth);
                break;
            case PAS_MPESType_CleanedIntensity:
                value.setFloat(m_data.cleanedIntensity);
                spdlog::trace("{} : Getting cleanedIntensity value => ({})", m_ID, m_data.cleanedIntensity);
                break;
            case PAS_MPESType_xCentroidNominal:
                value.setFloat(m_data.xNominal);
                spdlog::trace("{} : Getting xCentroidNominal value => ({})", m_ID, m_data.xNominal);
                break;
            case PAS_MPESType_yCentroidNominal:
                value.setFloat(m_data.yNominal);
                spdlog::trace("{} : Getting yCentroidNominal value => ({})", m_ID, m_data.yNominal);
                break;
            case PAS_MPESType_Position:
                value.setInt32(m_ID.position);
                spdlog::trace("{} : Getting Position value => ({})", m_ID, m_ID.position);
                break;
            case PAS_MPESType_Serial:
                value.setInt32(m_ID.serialNumber);
                spdlog::trace("{} : Getting SerialNumber value => ({})", m_ID, m_ID.serialNumber);
                break;
            case PAS_MPESType_ErrorState:
                status = m_pClient->read({m_pClient->getDeviceNodeId(m_ID) + "." + "ErrorState"}, &value);
                spdlog::trace("{} : Getting ErrorState value => ({})", m_ID, value[0].Value.UInt32);
                break;
            default:
                status = OpcUa_BadInvalidArgument;
                break;
        }
    }

    return OpcUa_Good;
}

UaStatus MPESController::getError(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (MPESObject::ERRORS.count(offset) > 0) {
        std::string varName = std::get<0>(MPESObject::ERRORS.at(offset));
        std::vector<std::string> varsToRead = {m_pClient->getDeviceNodeId(m_ID) + "." + varName};
        status = m_pClient->read(varsToRead, &value);
        spdlog::trace("{} : Getting error {} value => ({})", m_ID, offset, value[0].Value.Boolean);
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

    if (offset == PAS_MPESType_Read) {
        spdlog::info("{} : MPES controller calling read()", m_ID);
        status = read(true);
    } else if (offset == PAS_MPESType_SetExposure) {
        spdlog::info("{} : MPES controller calling setExposure()", m_ID);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("SetExposure"), args);
    } else if (offset == PAS_MPESType_ClearError) {
        spdlog::info("{} : MPES controller calling clearError() for error {}", m_ID, args[0].Value.Int32);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("ClearError"), args);
    } else if (offset == PAS_MPESType_ClearAllErrors) {
        spdlog::info("{} : MPES controller calling clearAllErrors()", m_ID);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("ClearAllErrors"));
    } else if (offset == PAS_MPESType_TurnOn) {
        spdlog::info("{} : MPES controller calling turnOn()", m_ID);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("TurnOn"));
    } else if (offset == PAS_MPESType_TurnOff) {
        spdlog::info("{} : MPES controller calling turnOff()", m_ID);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("TurnOff"));
    } else {
        status = OpcUa_BadInvalidArgument;
    }

    if (status == OpcUa_BadInvalidState) {
        spdlog::error("{} : MPESController::operate() : Device is in a bad state (busy, off, error) and "
                      "could not execute command. Check state and try again.", m_ID);
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
    status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("Read"), UaVariantArray());

    if (!status.isGood()) {
        return status;
    }

    std::vector<std::string> varstoread{
        "xCentroidAvg",
        "yCentroidAvg",
        "xCentroidSpotWidth",
        "yCentroidSpotWidth",
        "CleanedIntensity",
        "xCentroidNominal",
        "yCentroidNominal"
    };
    std::transform(varstoread.begin(), varstoread.end(), varstoread.begin(),
                   [this](std::string &str) { return m_pClient->getDeviceNodeId(m_ID) + "." + str; });
    UaVariant valstoread[7];

    status = m_pClient->read(varstoread, &valstoread[0]);
    m_numAttempts = 1;

    if (!status.isGood()) {
        spdlog::error("{} : MPES Read failed", m_ID);
        return status;
    }

    for (unsigned i = 0; i < varstoread.size(); i++)
        valstoread[i].toFloat(*(reinterpret_cast<float *>(&m_data) + i));

    if (print) {
        spdlog::info(
            "Reading MPES {}:\n"
            "x (nominal): {} ({})\n"
            "y (nominal): {} ({})\n"
            "xSpotWidth (nominal): {} ({})\n"
            "xSpotWidth (nominal): {} ({})\n"
            "Cleaned Intensity (nominal): {} ({})\n",
            m_ID,
            m_data.xCentroid, m_data.xNominal,
            m_data.yCentroid, m_data.yNominal,
            m_data.xSpotWidth, kNominalSpotWidth,
            m_data.ySpotWidth, kNominalSpotWidth,
            m_data.cleanedIntensity, kNominalIntensity);
    }

    if (isVisible()) {
        if (m_data.xSpotWidth > kNominalSpotWidth) {
            spdlog::warn(
                "{} : The width of the image along the X axis ({}) is greater than the nominal limit ({}). Consider fixing things.",
                m_ID, m_data.xSpotWidth, kNominalSpotWidth);
        }
        if (m_data.ySpotWidth > kNominalSpotWidth) {
            spdlog::warn(
                "{} : The width of the image along the Y axis ({}) is greater than the nominal limit ({}). Consider fixing things.",
                m_ID, m_data.ySpotWidth, kNominalSpotWidth);
        }
        if (fabs(m_data.cleanedIntensity - kNominalIntensity) / kNominalIntensity > 0.2) {
            if (m_numAttempts <= kMaxAttempts) {
                spdlog::warn(
                    "{} : The image intensity ({}) differs from the nominal value ({}) by more than 20%. Will readjust exposure now.",
                    m_ID, m_data.cleanedIntensity, kNominalIntensity);
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

    spdlog::trace("{} : Recorded MPES measurement SQL statement into MPES_readings.sql file: {} ", m_ID,
                  sql_stmt.toUtf8());

    return status;
}

char MPESController::getPanelSide(unsigned panelpos) {
    char panelside;
    try {
        panelside = m_PanelSideMap.at(panelpos);
    }
    catch (std::out_of_range &e) {
        spdlog::error("MPESController::getPanelSide() : Invalid panel position {} (not connected to this MPES {}).",
                      panelpos, m_ID);
        panelside = 0;
    }

    return panelside;
}

bool MPESController::isVisible() {
    UaVariant temp;
    OpcUa_Boolean missingLaserError;
    getError(PAS_MPESType_Error2, temp);
    temp.toBool(missingLaserError);

    return !missingLaserError;
}

Eigen::Vector2d MPESController::getAlignedReadings() {
    Eigen::Vector2d alignedReadings;

    alignedReadings(0) = m_data.xNominal;
    alignedReadings(1) = m_data.yNominal;

    return alignedReadings;
};

