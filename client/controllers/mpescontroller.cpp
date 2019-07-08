#include "client/controllers/mpescontroller.hpp"

#include <fstream>

#include "common/opcua/pasobject.hpp"
#include "common/alignment/mpes.hpp"
#include "common/utilities/DBConfig.hpp"

#include "client/clienthelper.hpp"

#include "mysql_driver.h"
#include "cppconn/statement.h"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

MPESController::MPESController(Device::Identity identity, Client *pClient, std::string mode) : PasController(
    std::move(identity), pClient), m_Mode(mode)
{
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

    spdlog::debug("{} : Initializing MPES Controller... (connecting to DB)", m_Identity);
    try {
        sql::Driver *sql_driver = get_driver_instance();
        sql::Connection *sql_conn = sql_driver->connect(db_address, db_user, db_password);
        sql_conn->setSchema(db_name);
        sql::Statement *sql_stmt = sql_conn->createStatement();
        sql::ResultSet *sql_results;

        std::string query;
        // obtain topological data
        query = "SELECT w_panel, l_panel FROM Opt_MPESMapping WHERE end_date is NULL and serial_number=" +
                to_string(m_Identity.serialNumber);
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
                to_string(m_Identity.serialNumber);
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

    spdlog::debug("{} : Done.", m_Identity);
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

    m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "State"}, &value);
    value.toInt32(v);

    state = static_cast<Device::DeviceState>(v);
    spdlog::trace("{} : Read device state => ({})", m_Identity, Device::deviceStateNames.at(state));

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
        switch (offset) {
            case PAS_MPESType_xCentroidAvg:
                status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "xCentroidAvg"}, &value);
                spdlog::trace("{} : Read xCentroid value => ({})", m_Identity, value[0].Value.Double);
                break;
            case PAS_MPESType_yCentroidAvg:
                status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "yCentroidAvg"}, &value);
                spdlog::trace("{} : Read yCentroid value => ({})", m_Identity, value[0].Value.Double);
                break;
            case PAS_MPESType_xCentroidSpotWidth:
                status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "xCentroidSpotWidth"}, &value);
                spdlog::trace("{} : Read xCentroidSpotWidth value => ({})", m_Identity, value[0].Value.Double);
                break;
            case PAS_MPESType_yCentroidSpotWidth:
                status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "xCentroidSpotWidth"}, &value);
                spdlog::trace("{} : Read yCentroidSpotWidth value => ({})", m_Identity, value[0].Value.Double);
                break;
            case PAS_MPESType_CleanedIntensity:
                status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "CleanedIntensity"}, &value);
                spdlog::trace("{} : Read CleanedIntensity value => ({})", m_Identity, value[0].Value.Double);
                break;
            case PAS_MPESType_xCentroidNominal:
                status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "xCentroidNominal"}, &value);
                spdlog::trace("{} : Read xCentroidNominal value => ({})", m_Identity, value[0].Value.Double);
                break;
            case PAS_MPESType_yCentroidNominal:
                status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "yCentroidNominal"}, &value);
                spdlog::trace("{} : Read yCentroidNominal value => ({})", m_Identity, value[0].Value.Double);
                break;
            case PAS_MPESType_Position:
                value.setInt32(m_Identity.position);
                spdlog::trace("{} : Read Position value => ({})", m_Identity, m_Identity.position);
                break;
            case PAS_MPESType_Serial:
                value.setInt32(m_Identity.serialNumber);
                spdlog::trace("{} : Read SerialNumber value => ({})", m_Identity, m_Identity.serialNumber);
                break;
            case PAS_MPESType_ErrorState:
                status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "ErrorState"}, &value);
                spdlog::trace("{} : Read ErrorState value => ({})", m_Identity, value[0].Value.UInt32);
                break;
            case PAS_MPESType_Exposure:
                status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "Exposure"}, &value);
                spdlog::trace("{} : Read Exposure value => ({})", m_Identity, value[0].Value.Int32);
                break;
            case PAS_MPESType_RawTimestamp:
                status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "RawTimestamp"}, &value);
                spdlog::trace("{} : Read Raw Timestamp value => ({})", m_Identity, value[0].Value.Int64);
                break;
            case PAS_MPESType_Timestamp:
                status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "Timestamp"}, &value);
                spdlog::trace("{} : Read Timestamp value => ({})", m_Identity, UaString(value[0].Value.String).toUtf8());
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
        std::vector<std::string> varsToRead = {m_pClient->getDeviceNodeId(m_Identity) + "." + varName};
        status = m_pClient->read(varsToRead, &value);
        spdlog::trace("{} : Read error {} value => ({})", m_Identity, offset, value[0].Value.Boolean);
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
        spdlog::info("{} : MPESController calling read()", m_Identity);
        status = read();
        if (status.isGood()) {
            spdlog::info("{}: Done reading webcam.", m_Identity);
        }
        else {
            return status;
        }

        MPESBase::Position data = getPosition();
        spdlog::info(
            "Reading MPES {}:\n"
            "x (nominal): {} ({})\n"
            "y (nominal): {} ({})\n"
            "xSpotWidth (nominal): {} ({})\n"
            "xSpotWidth (nominal): {} ({})\n"
            "Cleaned Intensity (nominal): {} ({})\n"
            "Exposure: {}\n"
            "Timestamp: {}\n",
            m_Identity,
            data.xCentroid, data.xNominal,
            data.yCentroid, data.yNominal,
            data.xSpotWidth, std::to_string(MPESBase::NOMINAL_SPOT_WIDTH),
            data.ySpotWidth, std::to_string(MPESBase::NOMINAL_SPOT_WIDTH),
            data.cleanedIntensity, std::to_string(MPESBase::NOMINAL_INTENSITY),
            data.exposure,
            std::ctime(&data.timestamp));

        if (m_Mode == "client") { // Record readings to database
            struct tm tstruct{};
            char buf[80];
            tstruct = *localtime(&data.timestamp);
            strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);

            UaString sql_stmt = UaString(
                "INSERT INTO Opt_MPESReadings (date, serial_number, xcoord, ycoord, x_SpotWidth, y_SpotWidth, intensity) VALUES  ('%1', '%2', '%3', '%4', '%5', '%6', '%7' );\n").arg(
                buf).arg(m_Identity.serialNumber).arg(data.xCentroid).arg(data.yCentroid).arg(
                data.xSpotWidth).arg(data.ySpotWidth).arg(data.cleanedIntensity);
            std::ofstream sql_file("MPES_readings.sql", std::ios_base::app);
            sql_file << sql_stmt.toUtf8();

            spdlog::trace("{} : Recorded MPES measurement SQL statement into MPES_readings.sql file: {} ", m_Identity,
                          sql_stmt.toUtf8());
        }
    } else if (offset == PAS_MPESType_SetExposure) {
        spdlog::info("{} : MPESController calling setExposure()", m_Identity);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("SetExposure"), args);
    } else if (offset == PAS_MPESType_ClearError) {
        spdlog::info("{} : MPESController calling clearError() for error {}", m_Identity, args[0].Value.Int32);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("ClearError"), args);
    } else if (offset == PAS_MPESType_ClearAllErrors) {
        spdlog::info("{} : MPESController calling clearAllErrors()", m_Identity);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("ClearAllErrors"));
    } else if (offset == PAS_MPESType_TurnOn) {
        spdlog::info("{} : MPESController calling turnOn()", m_Identity);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("TurnOn"));
    } else if (offset == PAS_MPESType_TurnOff) {
        spdlog::info("{} : MPESController calling turnOff()", m_Identity);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("TurnOff"));
    } else {
        status = OpcUa_BadInvalidArgument;
    }

    if (status == OpcUa_BadInvalidState) {
        spdlog::error("{} : MPESController::operate() : Device is in a bad state (busy, off, error) and "
                      "could not execute command. Check state and try again.", m_Identity);
    }

    return status;

}

/* ----------------------------------------------------------------------------
    Class        MPESController
    Method       read
    Description  Read Controller data.
-----------------------------------------------------------------------------*/
UaStatus MPESController::read() {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;
    status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("Read"), UaVariantArray());

    if (!status.isGood()) {
        spdlog::error("{} : MPESController::read() : Call to read webcam failed.", m_Identity);
        return status;
    }

    if (m_Mode == "client") {
        MPESBase::Position position = getPosition();
        int numAttempts = 1;
        if (fabs(position.cleanedIntensity - MPESBase::NOMINAL_INTENSITY) / MPESBase::NOMINAL_INTENSITY > 0.2) {
            if (numAttempts <= MPESBase::MAX_READ_ATTEMPTS) {
                spdlog::warn(
                    "{} : The image intensity ({}) differs from the nominal value ({}) by more than 20%. Will readjust exposure now.",
                    m_Identity, position.cleanedIntensity, std::to_string(MPESBase::NOMINAL_INTENSITY));
                operate(PAS_MPESType_SetExposure, UaVariantArray());
                numAttempts++;
                // read the sensor again
                status = read();
            }
        }
    }

    return status;
}

UaStatus MPESController::readAsync() {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;
    status = m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_Identity), UaString("Read"), UaVariantArray());

    return status;
}

char MPESController::getPanelSide(unsigned panelpos) {
    char panelside;
    try {
        panelside = m_PanelSideMap.at(panelpos);
    }
    catch (std::out_of_range &e) {
        //spdlog::error("MPESController::getPanelSide() : Invalid panel position {} (not connected to this MPES {}).",
        //              panelpos, m_ID);
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
    double xNominal;
    double yNominal;

    std::vector<std::string> varstoread{
        "xCentroidNominal",
        "yCentroidNominal"
    };
    std::transform(varstoread.begin(), varstoread.end(), varstoread.begin(),
                   [this](std::string &str) { return m_pClient->getDeviceNodeId(m_Identity) + "." + str; });
    UaVariant valstoread[2];

    m_pClient->read(varstoread, &valstoread[0]);

    valstoread[0].toDouble(xNominal);
    valstoread[1].toDouble(yNominal);

    alignedReadings(0) = xNominal;
    alignedReadings(1) = yNominal;

    return alignedReadings;
}

MPESBase::Position MPESController::getPosition() {
    UaStatus status;

    std::vector<std::string> varstoread{
        "xCentroidAvg",
        "yCentroidAvg",
        "xCentroidSpotWidth",
        "yCentroidSpotWidth",
        "CleanedIntensity",
        "xCentroidNominal",
        "yCentroidNominal",
        "Exposure",
        "RawTimestamp"
    };
    std::transform(varstoread.begin(), varstoread.end(), varstoread.begin(),
                   [this](std::string &str) { return m_pClient->getDeviceNodeId(m_Identity) + "." + str; });
    UaVariant valstoread[9];

    status = m_pClient->read(varstoread, &valstoread[0]);

    MPESBase::Position data;
    if (!status.isGood()) {
        spdlog::error("{} : MPESController::getPosition() : OPC UA read failed.", m_Identity);
        return data;
    }
    valstoread[0].toFloat(data.xCentroid);
    valstoread[1].toFloat(data.yCentroid);
    valstoread[2].toFloat(data.xSpotWidth);
    valstoread[3].toFloat(data.ySpotWidth);
    valstoread[4].toFloat(data.cleanedIntensity);
    valstoread[5].toFloat(data.xNominal);
    valstoread[6].toFloat(data.yNominal);
    valstoread[7].toInt32(data.exposure);
    valstoread[8].toInt64(data.timestamp);

    return data;
}
