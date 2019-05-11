#include "client/controllers/mpescontroller.hpp"

#include "common/utilities/DBConfig.hpp"
#include "client/clienthelper.hpp"

#include "mysql_driver.h"
#include "cppconn/statement.h"

#include <fstream>

float MPESController::kNominalIntensity = 150000.;
float MPESController::kNominalCentroidSD = 20.;

MPESController::MPESController(Identity identity, std::shared_ptr<Client> pClient) : PasController(std::move(identity),
                                                                                                   pClient),
                                                                                     m_updated(false), m_isVisible(false) {
    m_state = PASState::On;
    m_Data = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

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

        // print out the loaded values
        //std::cout << "\t Aligned readings:\n" << m_AlignedReadings << std::endl;
        for (const auto &matrixPair : m_ResponseMatMap)
        {
            //std::cout << "\t " << matrixPair.first << "-side response matrix:\n" << matrixPair.second << std::endl;
        }
        // pass the aligned readings on to the server
        UaVariant value;
        value.setDouble(m_AlignedReadings(0));
        setData(PAS_MPESType_xCentroidNominal, value);
        value.setDouble(m_AlignedReadings(1));
        setData(PAS_MPESType_yCentroidNominal, value);

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

        // set state to error
        m_state = PASState::FatalError;
    }
    /* END DATABASE HACK */

    SystematicOffsets = Eigen::Vector2d::Zero();
}

MPESController::~MPESController() {
    m_pClient = nullptr;
    m_state = PASState::Off;
}

/* ----------------------------------------------------------------------------
    Class        MPESController
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatus MPESController::getState(PASState &state) {
    //UaMutexLocker lock(&m_mutex);
    state = m_state;
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        MPESController
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatus MPESController::setState(PASState state) {
    UaStatus status;

    if (state == m_state) {
        return OpcUa_BadInvalidState;
    }
    //UaMutexLocker lock(&m_mutex);

    m_state = state;
    if (state == PASState::Off)
        status = m_pClient->callMethod(m_ID.eAddress, UaString("Stop"));
    else if (state == PASState::On)
        status = m_pClient->callMethod(m_ID.eAddress, UaString("Start"));
    else
        status = OpcUa_BadInvalidArgument;

    return status;
}

/* ----------------------------------------------------------------------------
    Class        MPESController
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatus MPESController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    UaStatus status;
    //UaMutexLocker lock(&m_mutex);

    int dataoffset = offset - PAS_MPESType_xCentroidAvg;
    if ((dataoffset >= 7) || (dataoffset < 0))
        return OpcUa_BadInvalidArgument;

    if (!m_updated)
        status = read();

    // cast struct to double through reinterpret_cast!
    value.setDouble(*(reinterpret_cast<OpcUa_Double *>(&m_Data) + dataoffset));

    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        MPESController
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatus MPESController::setData(OpcUa_UInt32 offset, UaVariant value) {
    UaStatus status;

    int dataoffset = offset - PAS_MPESType_xCentroidAvg;
    if ((dataoffset > 6) | (dataoffset < 5))
        return OpcUa_BadNotWritable;

    std::string varstowrite[2]{".xCentroidNominal", ".yCentroidNominal"};
    vector<std::string> vec_curwrite{m_ID.eAddress + varstowrite[dataoffset - 5]};

    // set local variable
    value.toDouble(*(reinterpret_cast<OpcUa_Double *>(&m_Data) + dataoffset));
    // and write it to the server
    status = m_pClient->write(vec_curwrite, &value);

    return status;
}

/* ----------------------------------------------------------------------------
    Class        MPESController
    Method       Operate
    Description  run a method on the sensor
-----------------------------------------------------------------------------*/
UaStatus MPESController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (offset == PAS_MPESType_Read)
    {
        status = read();
        return status;
    }
    else if (offset == PAS_MPESType_SetExposure) {
        std::cout << "+++ Adjusting exposure for " << m_ID << std::endl;
        status = m_pClient->callMethod(m_ID.eAddress, UaString("SetExposure"));
        return status;
    } else
        return OpcUa_BadInvalidArgument;

}

/* ----------------------------------------------------------------------------
    Class        MPESController
    Method       read
    Description  Read Controller data.
-----------------------------------------------------------------------------*/
UaStatus MPESController::read(bool print) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    m_updated = false;

    std::cout << "calling read() on " << m_ID << std::endl;
    if (m_state == PASState::On) {
        // read the values on the server first
        status = __readRequest();

        m_isVisible = true;
        if (m_Data.xCentroidAvg < 0.1) m_isVisible = false;
        if (m_isVisible) {
            if (m_Data.xCentroidSpotWidth > kNominalCentroidSD) {
                std::cout << "+++ WARNING +++ The width of the image along the X axis for " << m_ID.name
                          << " is greater than 20px. Consider fixing things." << std::endl;
            }
            if (m_Data.yCentroidSpotWidth > kNominalCentroidSD) {
                std::cout << "+++ WARNING +++ The width of the image along the Y axis for " << m_ID.name
                          << " is greater than 20px. Consider fixing things." << std::endl;
            }

            if (fabs(m_Data.CleanedIntensity - kNominalIntensity) / kNominalIntensity > 0.2) {
                std::cout << "+++ WARNING +++ The intensity of " << m_ID.name
                          << " differs from the magic value by more than 20%\n"
                          << "+++ WARNING +++ Will readjust the exposure now!" << std::endl;
                operate(PAS_MPESType_SetExposure);
                // read the sensor again
                status = __readRequest();
            }
        }

        if (print) {
            std::cout << "MPES reading:" << std::endl;
            std::cout << "x: " << m_Data.xCentroidAvg << std::endl;
            std::cout << "xNominal: " << m_Data.xNominal << std::endl;
            std::cout << "y: " << m_Data.yCentroidAvg << std::endl;
            std::cout << "yNominal: " << m_Data.yNominal << std::endl;
            std::cout << "xSpotWidth: " << m_Data.xCentroidSpotWidth << std::endl;
            std::cout << "ySpotWidth: " << m_Data.yCentroidSpotWidth << std::endl;
            std::cout << "Cleaned Intensity: " << m_Data.CleanedIntensity << std::endl;
        }

        time_t now = time(nullptr);
        struct tm tstruct{};
        char buf[80];
        tstruct = *localtime(&now);
        strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);

        UaString sql_stmt = UaString(
                "INSERT INTO Opt_MPESReadings (date, serial_number, xcoord, ycoord, x_SpotWidth, y_SpotWidth, intensity) VALUES  ('%1', '%2', '%3', '%4', '%5', '%6', '%7' );\n").arg(
            buf).arg(m_ID.serialNumber).arg(m_Data.xCentroidAvg).arg(m_Data.yCentroidAvg).arg(
            m_Data.xCentroidSpotWidth).arg(m_Data.yCentroidSpotWidth).arg(m_Data.CleanedIntensity);
        std::ofstream sql_file("MPES_readings.sql", std::ios_base::app);
        sql_file << sql_stmt.toUtf8();
    }

    return status;
}

// a helper for the above that simply requests data from the server without performing any checks
UaStatus MPESController::__readRequest() {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    // read the values on the server first
    status = m_pClient->callMethod(m_ID.eAddress, UaString("Read"));
    if (!status.isGood()) return status;
    // get the updated values from the server

    // get all the updated values from the server
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
    if (status.isGood()) m_updated = true;

    for (unsigned i = 0; i < varstoread.size(); i++)
        valstoread[i].toDouble(*(reinterpret_cast<OpcUa_Double *>(&m_Data) + i));

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

