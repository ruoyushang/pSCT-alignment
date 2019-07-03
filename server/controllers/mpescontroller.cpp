/**
 * @file mpescontroller.cpp
 * @brief Source file for mirror panel edge sensor device controller
 */

#include "server/controllers/mpescontroller.hpp"

#include <iostream>
#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"

#include "common/alignment/platform.hpp"
#include "common/opcua/pascominterfacecommon.hpp"
#include "common/opcua/passervertypeids.hpp"
#include "common/opcua/pasobject.hpp"
#include "common/utilities/DBConfig.hpp"

#include "mysql_connection.h"
#include "mysql_driver.h"

#include "cppconn/driver.h"
#include "cppconn/exception.h"
#include "cppconn/resultset.h"
#include "cppconn/statement.h"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"


MPESController::MPESController(Device::Identity identity, std::shared_ptr<PlatformBase> pPlatform)
    : PasController::PasController(std::move(identity), std::move(pPlatform), 5000) {
    try {
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

        spdlog::trace("{} : Connecting to DB {} at {} with user {} for nominal readings", m_ID, db_name, db_address,
                      db_user);

        sql::Driver *sql_driver = get_driver_instance();
        sql::Connection *sql_conn = sql_driver->connect(db_address, db_user, db_password);
        sql_conn->setSchema(db_name);
        sql::Statement *sql_stmt = sql_conn->createStatement();
        sql::ResultSet *sql_results;

        std::string query =
            "SELECT coord, nominal_reading FROM Opt_MPESConfigurationAndCalibration WHERE end_date is NULL and serial_number=" +
            std::to_string(m_ID.serialNumber);
        sql_stmt->execute(query);
        sql_results = sql_stmt->getResultSet();

        while (sql_results->next()) {
            char coord = sql_results->getString(1)[0];
            if (coord == 'x') {
                m_pPlatform->getMPESbyIdentity(m_ID)->setxNominalPosition((float) sql_results->getDouble(2));
            } else if (coord == 'y') {
                m_pPlatform->getMPESbyIdentity(m_ID)->setyNominalPosition((float) sql_results->getDouble(2));
            } else {
                spdlog::error("Error: Invalid coord {} (should be x or y).", coord);
            }
        }
        // close the connection!
        sql_conn->close();
        delete sql_conn;
    }
    catch (sql::SQLException &e) {
        spdlog::error("# ERR: SQLException in {}\n ({}) on line {}\n # ERR: {} (MySQL error code: {}, SQLState: {})",
                      __FILE__, __FUNCTION__, __LINE__, e.what(), e.getErrorCode(), e.getSQLState());
    }

    // make sure things update on the first boot up
    // duration takes seconds -- hence the conversion with the 1/1000 ratio
    m_lastUpdateTime = TIME::now() - std::chrono::duration<int, std::ratio<1, 1000>>
            (m_kUpdateInterval_ms);
}

/// @details Locks the shared mutex while retrieving the state.
UaStatus MPESController::getState(Device::DeviceState &state) {
    UaMutexLocker lock(&m_mutex);
    state = _getDeviceState();
    spdlog::trace("{} : Read device state => ({})", m_ID, Device::deviceStateNames.at(state));
    return OpcUa_Good;
}

/// @details Does not allow setting the state to error or setting the state to
/// its current value. Locks the shared mutex while setting the state.
UaStatus MPESController::setState(Device::DeviceState state) {
    return OpcUa_BadNotWritable;
}
/// @details Sets exposure for this MPES.
bool MPESController::initialize() {
    spdlog::info("{} : Initializing MPES...", m_ID);
    m_pPlatform->getMPESbyIdentity(m_ID)->setExposure();
    return true;
}

/// @details If the MPES has not been read yet, calls read before retrieving data. Locks the shared mutex while reading data.
UaStatus MPESController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (MPESObject::VARIABLES.find(offset) != MPESObject::VARIABLES.end()) {
        if (offset != PAS_MPESType_Position && offset != PAS_MPESType_Serial && offset != PAS_MPESType_ErrorState &&
            offset != PAS_MPESType_xCentroidNominal && offset != PAS_MPESType_yCentroidNominal) {
            if (__expired()) {
                spdlog::info("{} : MPES data is expired, calling read()...", m_ID);
                status = operate(PAS_MPESType_Read, UaVariantArray());
            }
            if (status == OpcUa_BadInvalidState) {
                return status;
            }
        }
        const MPESBase::Position &position = m_pPlatform->getMPESbyIdentity(m_ID)->getPosition();
        switch (offset) {
            case PAS_MPESType_xCentroidAvg:
                spdlog::trace("{} : Read xCentroid value => ({})", m_ID, position.xCentroid);
                value.setFloat(position.xCentroid);
                break;
            case PAS_MPESType_yCentroidAvg:
                spdlog::trace("{} : Read yCentroid value => ({})", m_ID, position.yCentroid);
                value.setFloat(position.yCentroid);
                break;
            case PAS_MPESType_xCentroidSpotWidth:
                spdlog::trace("{} : Read xSpotWidth value => ({})", m_ID, position.xSpotWidth);
                value.setFloat(position.xSpotWidth);
                break;
            case PAS_MPESType_yCentroidSpotWidth:
                spdlog::trace("{} : Read ySpotWidth value => ({})", m_ID, position.ySpotWidth);
                value.setFloat(position.ySpotWidth);
                break;
            case PAS_MPESType_CleanedIntensity:
                spdlog::trace("{} : Read CleanedIntensity value => ({})", m_ID, position.cleanedIntensity);
                value.setFloat(position.cleanedIntensity);
                break;
            case PAS_MPESType_xCentroidNominal:
                spdlog::trace("{} : Read xCentroidNominal value => ({})", m_ID, position.xNominal);
                value.setFloat(position.xNominal);
                break;
            case PAS_MPESType_yCentroidNominal:
                spdlog::trace("{} : Read yCentroidNominal value => ({})", m_ID, position.yNominal);
                value.setFloat(position.yNominal);
                break;
            case PAS_MPESType_Position:
                spdlog::trace("{} : Read Position value => ({})", m_ID, m_ID.position);
                value.setInt32(m_ID.position);
                break;
            case PAS_MPESType_Serial:
                spdlog::trace("{} : Read Serial value => ({})", m_ID, m_ID.serialNumber);
                value.setInt32(m_ID.serialNumber);
                break;
            case PAS_MPESType_ErrorState: {
                Device::ErrorState errorState = _getErrorState();
                spdlog::trace("{} : Read ErrorState value => ({})", m_ID, static_cast<int>(errorState));
                value.setInt32(static_cast<int>(errorState));
                break;
            }
            default:
                break;
        }
    } else if (MPESObject::ERRORS.find(offset) != MPESObject::ERRORS.end()) {
        return getError(offset, value);
    } else {
        return OpcUa_BadInvalidArgument;
    }

    return status;
}

UaStatus MPESController::getError(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;
    bool errorStatus;

    OpcUa_UInt32 errorNum = offset - PAS_MPESType_Error0;
    if (errorNum >= 0 && errorNum < MPESObject::ERRORS.size()) {
        errorStatus = m_pPlatform->getMPESbyIdentity(m_ID)->getError(int(errorNum));
        value.setBool(errorStatus);
        spdlog::trace("{} : Read error {} value => ({})", m_ID, errorNum, errorStatus);
    } else {
        status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/// @details Locks the mutex while writing data.
UaStatus MPESController::setData(OpcUa_UInt32 offset, UaVariant value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    float v;
    value.toFloat(v);
    switch (offset) {
        case PAS_MPESType_xCentroidNominal:
            spdlog::trace("{} : Setting xCentroidNominal to {}.", m_ID, v);
            m_pPlatform->getMPESbyIdentity(m_ID)->setxNominalPosition(v);
            break;
        case PAS_MPESType_yCentroidNominal:
            spdlog::trace("{} : Setting yCentroidNominal to {}.", m_ID, v);
            m_pPlatform->getMPESbyIdentity(m_ID)->setyNominalPosition(v);
            break;
        default:
            status = OpcUa_BadNotWritable;
    }

    return status;
}

/// @details Locks the shared mutex while calling methods.
UaStatus MPESController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (_getDeviceState() == Device::DeviceState::Busy) {
        spdlog::error("{} : MPES is busy, operate call failed.", m_ID);
        return OpcUa_BadInvalidState;
    }

    switch (offset) {
        case PAS_MPESType_TurnOn:
            spdlog::info("{} : MPES controller calling turnOn()", m_ID);
            if (_getDeviceState() == Device::DeviceState::Off) {
                m_pPlatform->getMPESbyIdentity(m_ID)->turnOn();
                initialize();
            } else {
                spdlog::trace("{} : Device is already on, nothing to do...", m_ID);
            }
            break;
        case PAS_MPESType_TurnOff:
            spdlog::info("{} : MPES controller calling turnOff()", m_ID);
            if (_getDeviceState() == Device::DeviceState::On) {
                m_pPlatform->getMPESbyIdentity(m_ID)->turnOff();
            } else {
                spdlog::trace("{} : Device is already off, nothing to do...", m_ID);
            }
            break;
        case PAS_MPESType_Read:
            spdlog::info("{} : MPES controller calling read()", m_ID);
            if (_getDeviceState() == Device::DeviceState::On && _getErrorState() != Device::ErrorState::FatalError) {
                status = read();
            } else {
                spdlog::error("{} : MPES is off/in fatal error state, unable to read.", m_ID);
                status = OpcUa_BadInvalidState;
            }
            break;
        case PAS_MPESType_SetExposure:
            spdlog::info("{} : MPES controller calling setExposure()", m_ID);
            if (_getDeviceState() == Device::DeviceState::On) {
                m_pPlatform->getMPESbyIdentity(m_ID)->setExposure();
            } else {
                spdlog::error("{} : MPES is off, unable to set exposure.", m_ID);
                status = OpcUa_BadInvalidState;
            }
            break;
        case PAS_MPESType_ClearError:
            spdlog::info("{} : MPES controller calling clearError() for error {}", m_ID, args[0].Value.Int32);
            m_pPlatform->getMPESbyIdentity(m_ID)->unsetError(args[0].Value.Int32);
            break;
        case PAS_MPESType_ClearAllErrors:
            spdlog::info("{} : MPES controller calling clearErrors()", m_ID);
            m_pPlatform->getMPESbyIdentity(m_ID)->clearErrors();
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }
    return status;
}

/// @details If state is On, calls the ReadMPES method through the Platform object and sets the m_updated flag to true.
/// Locks the shared mutex while reading.
UaStatus MPESController::read() {
    //UaMutexLocker lock(&m_mutex);
    spdlog::trace("{} : Updating MPES position data (reading webcam) ... ", m_ID);
    m_pPlatform->getMPESbyIdentity(m_ID)->updatePosition();
    m_lastUpdateTime = TIME::now();
    return OpcUa_Good;
}
