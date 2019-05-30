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

MPESController::MPESController(Device::Identity identity, std::shared_ptr<Platform> pPlatform)
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
                std::cout << "Error: Invalid coord " << coord << " (should be x or y)." << std::endl;
            }
        }
        // close the connection!
        sql_conn->close();
        delete sql_conn;
    }
    catch (sql::SQLException &e) {
        std::cout << "# ERR: SQLException in " << __FILE__;
        std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
        std::cout << "# ERR: " << e.what();
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    }
}

/// @details Locks the shared mutex while retrieving the state.
UaStatus MPESController::getState(Device::DeviceState &state) {
    UaMutexLocker lock(&m_mutex);
    state = _getDeviceState();
    return OpcUa_Good;
}

/// @details Does not allow setting the state to error or setting the state to
/// its current value. Locks the shared mutex while setting the state.
UaStatus MPESController::setState(Device::DeviceState state) {
    return OpcUa_BadNotWritable;
}
/// @details Sets exposure for this MPES.
bool MPESController::initialize() {
    if (_getDeviceState() == Device::DeviceState::On && _getErrorState() != Device::ErrorState::FatalError) {
        m_pPlatform->getMPESbyIdentity(m_ID)->setExposure();
        read();
        return true;
    } else {
        if (_getDeviceState() == Device::DeviceState::On) {
            std::cout << m_ID << " :: MPESController::initialize() : Failed to set exposure due to bad state."
                      << std::endl;
            return false;
        }
    }
}

/// @details If the MPES has not been read yet, calls read before retrieving data. Locks the shared mutex while reading data.
UaStatus MPESController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (MPESObject::VARIABLES.find(offset) != MPESObject::VARIABLES.end()) {
        if (offset != PAS_MPESType_Position && offset != PAS_MPESType_Serial && offset != PAS_MPESType_ErrorState &&
            !__expired()) {
            status = operate(PAS_MPESType_Read, UaVariantArray());
            if (status == OpcUa_BadInvalidState) {
                return status;
            }
        }
        const MPES::Position &position = m_pPlatform->getMPESbyIdentity(m_ID)->getPosition();
        switch (offset) {
            case PAS_MPESType_xCentroidAvg:
                value.setFloat(position.xCentroid);
                break;
            case PAS_MPESType_yCentroidAvg:
                value.setFloat(position.yCentroid);
                break;
            case PAS_MPESType_xCentroidSpotWidth:
                value.setFloat(position.xSpotWidth);
                break;
            case PAS_MPESType_yCentroidSpotWidth:
                value.setFloat(position.ySpotWidth);
                break;
            case PAS_MPESType_CleanedIntensity:
                value.setFloat(position.cleanedIntensity);
                break;
            case PAS_MPESType_xCentroidNominal:
                value.setFloat(position.xNominal);
                break;
            case PAS_MPESType_yCentroidNominal:
                value.setFloat(position.yNominal);
                break;
            case PAS_MPESType_Position:
                value.setInt32(m_ID.position);
                break;
            case PAS_MPESType_Serial:
                value.setInt32(m_ID.serialNumber);
                break;
            case PAS_MPESType_ErrorState:
                value.setInt32(static_cast<int>(_getErrorState()));
                break;
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
            m_pPlatform->getMPESbyIdentity(m_ID)->setxNominalPosition(v);
            break;
        case PAS_MPESType_yCentroidNominal:
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
        std::cout << m_ID << " :: MPESController::operate() : Device is busy. Operate call failed.\n";
        return OpcUa_BadInvalidState;
    }

    switch (offset) {
        case PAS_MPESType_TurnOn:
            m_pPlatform->getMPESbyIdentity(m_ID)->turnOn();
            initialize();
            break;
        case PAS_MPESType_TurnOff:
            m_pPlatform->getMPESbyIdentity(m_ID)->turnOff();
            break;
        case PAS_MPESType_Read:
            if (_getDeviceState() == Device::DeviceState::On && _getErrorState() != Device::ErrorState::FatalError) {
                status = read();
            } else {
                std::cout << m_ID << " :: MPESController::operate() : Device is off or has fatal error. Cannot read.\n";
                status = OpcUa_BadInvalidState;
            }
            break;
        case PAS_MPESType_SetExposure:
            if (_getDeviceState() == Device::DeviceState::On) {
                m_pPlatform->getMPESbyIdentity(m_ID)->setExposure();
            } else {
                std::cout << m_ID << " :: MPESController::operate() : Device is off. Cannot set exposure.\n";
                status = OpcUa_BadInvalidState;
            }
            break;
        case PAS_MPESType_ClearError:
            m_pPlatform->getMPESbyIdentity(m_ID)->unsetError(args[0].Value.Int32);
            break;
        case PAS_MPESType_ClearAllErrors:
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
    m_pPlatform->getMPESbyIdentity(m_ID)->updatePosition();
    return OpcUa_Good;
}
