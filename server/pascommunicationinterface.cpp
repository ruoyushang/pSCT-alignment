/**
 * @file pascommunicationinterface.cpp
 * @brief Source file for panel server PAS communication interface
 */

#include "server/pascommunicationinterface.hpp"

#include <array>

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "common/alignment/actuator.hpp"
#include "common/alignment/platform.hpp"
#include "common/alignment/device.hpp"
#include "common/opcua/passervertypeids.hpp"

#include "server/controllers/pascontroller.hpp"
#include "server/controllers/actcontroller.hpp"
#include "server/controllers/mpescontroller.hpp"
#include "server/controllers/psdcontroller.hpp"
#include "server/controllers/panelcontroller.hpp"

#include "uabase/uadatetime.h"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

// MySQL C++ Connector includes
#include "mysql_driver.h"
#include "cppconn/statement.h"
#include <cppconn/exception.h>

const std::map<OpcUa_UInt32, std::string> PasCommunicationInterface::deviceTypes = {
        {PAS_PanelType, "Panel"},
        {PAS_MPESType,  "MPES"},
        {PAS_ACTType,   "ACT"},
        {PAS_PSDType,   "PSD"}
};

PasCommunicationInterface::~PasCommunicationInterface() {
    spdlog::info("Closing and cleaning up communication interface...");
}

/// @details Uses DB login info from environment to access the device database and retrieve
/// all device mappings and positions. Then, initializes all corresponding controllers, adds all MPES to the platform object,
/// and attaches all actuator controllers to the corresponding panel controller.
UaStatus PasCommunicationInterface::initialize() {
    UaStatus status;

    /// @remark DB login in uses variables preconfigured in MPCB environment variables.
    Device::DBInfo DbInfo;
    DBConfig myConfig = DBConfig::getDefaultConfig();
    DbInfo.host = myConfig.getHost();
    DbInfo.port = myConfig.getPort();
    DbInfo.user = myConfig.getUser();
    DbInfo.password = myConfig.getPassword();
    DbInfo.dbname = myConfig.getDatabase();
    std::string dbAddress = "tcp://" + DbInfo.host + ":" + DbInfo.port;

    std::array<Device::Identity, PlatformBase::NUM_ACTS_PER_PLATFORM> actuatorIdentities;
    std::vector <Device::Identity> mpesIdentities;

    spdlog::trace("Connecting to DB {} at {} with user {}", DbInfo.dbname, dbAddress, DbInfo.user);
    try {
        sql::Driver *pSqlDriver = get_driver_instance(); // Need to use naked pointer here to avoid attempting to call protected destructor.
        std::unique_ptr<sql::Connection> pSqlConn = std::unique_ptr<sql::Connection>(
                pSqlDriver->connect(dbAddress, DbInfo.user, DbInfo.password));
        pSqlConn->setSchema(DbInfo.dbname);
        std::unique_ptr<sql::Statement> pSqlStmt = std::unique_ptr<sql::Statement>(pSqlConn->createStatement());
        std::unique_ptr<sql::ResultSet> pSqlResults;

        // Query DB for panel position and m_pCBC id by IP address
        std::string query = "SELECT mpcb_id, position FROM Opt_MPMMapping WHERE end_date is NULL and position='"
                            + std::to_string(m_panelNum) + "'";
        pSqlStmt->execute(query);
        pSqlResults.reset(pSqlStmt->getResultSet());

        // Should only be one result FOR NOW -- IN THE FUTURE, NEED TO FIX THIS, SORTING BY DATE
        while (pSqlResults->next()) {
            m_cbcID = pSqlResults->getInt(1);
            m_panelNum = pSqlResults->getInt(2);
        }

        if (m_cbcID == -1) {
            spdlog::error("Error: Failed to get valid cbcID. Cannot initialize panel.");
            return OpcUa_Bad;
        } else if (m_panelNum == -1) {
            spdlog::error("Error: Failed to get valid panelNum. Cannot initialize panel.");
            return OpcUa_Bad;
        }

        spdlog::info("Initializing Panel {} with CBC {}...", m_panelNum, m_cbcID);

        // Query DB for actuator serials and ports
        query = "SELECT serial_number, position, port FROM Opt_ActuatorMapping WHERE end_date is NULL and panel=" +
                std::to_string(m_panelNum);
        pSqlStmt->execute(query);
        pSqlResults.reset(pSqlStmt->getResultSet());
        while (pSqlResults->next()) {
            Device::Identity actId;
            actId.serialNumber = pSqlResults->getInt(1);
            actId.position = pSqlResults->getInt(2);
            std::string port = std::to_string(pSqlResults->getInt(3));
            actId.eAddress = port;
            actId.name = std::string("ACT_") + std::to_string(actId.serialNumber);
            actuatorIdentities[actId.position - 1] = actId;
        }

        // Query DB for mpes serials and ports
        query =
            "SELECT serial_number, w_position, port, l_panel FROM Opt_MPESMapping WHERE end_date is NULL and w_panel=" +
            std::to_string(m_panelNum);
        pSqlStmt->execute(query);
        pSqlResults.reset(pSqlStmt->getResultSet());
        while (pSqlResults->next()) {
            Device::Identity mpesId;
            mpesId.serialNumber = pSqlResults->getInt(1);
            mpesId.position = pSqlResults->getInt(2);
            mpesId.eAddress = std::to_string(pSqlResults->getInt(3));
            mpesId.name = std::string("MPES_") + std::to_string(mpesId.serialNumber);
            std::string port = std::to_string(pSqlResults->getInt(3));
            std::string l_panelPos = std::to_string(pSqlResults->getInt(4));
            mpesIdentities.push_back(mpesId);
        }
    }
    catch (sql::SQLException &e) {
        spdlog::error("# ERR: SQLException in {}\n ({}) on line {}\n # ERR: {} (MySQL error code: {}, SQLState: {})",
                      __FILE__, __FUNCTION__, __LINE__, e.what(), e.getErrorCode(), e.getSQLState());
        return OpcUa_Bad;
    }

    Device::Identity panelId;
    panelId.name = std::string("Panel_") + std::to_string(m_panelNum);
    panelId.serialNumber = m_cbcID;
    panelId.eAddress = std::to_string(0);
    panelId.position = m_panelNum;

#ifdef SIMMODE
    spdlog::info("SIMMODE: Creating DummyPlatform object with identity {}...", panelId);
    m_platform = std::dynamic_pointer_cast<PlatformBase>(std::make_shared<DummyPlatform>(panelId, DbInfo));
#else
    spdlog::info("Creating Platform hardware interface with identity {}...", panelId);
    m_platform = std::dynamic_pointer_cast<PlatformBase>(std::make_shared<Platform>(panelId, DbInfo));
#endif

    m_platform->initialize();

    for (int i = 0; i < PlatformBase::NUM_ACTS_PER_PLATFORM; i++) {
        spdlog::info("Adding ACT hardware interface with identity {} as child of Platform...", actuatorIdentities[i]);
    }
    m_platform->addActuators(actuatorIdentities);

    // initialize the MPES in the positional order. Not strictly necessary, but keeps things tidy
    // addMPES(port, serial)
    for (const auto &mpesId : mpesIdentities) {
        spdlog::info("Adding MPES hardware interface with identity {} as child of Platform ...", mpesId);
        m_platform->addMPES(mpesId);
    }

    // initialize expected devices
    spdlog::info(
        "Initializing all hardware interfaces (Platform, ACT, MPES) and creating corresponding controllers...");

    std::map<OpcUa_UInt32, std::vector<Device::Identity>> allDevices;
    allDevices[PAS_PanelType] = {m_platform->getIdentity()};
    allDevices[PAS_ACTType] = {};
    for (int i = 0; i < m_platform->getActuatorCount(); i++) {
        allDevices[PAS_ACTType].push_back(m_platform->getActuator(i)->getIdentity());
    }

    allDevices[PAS_MPESType] = {};
    for (int i = 0; i < m_platform->getMPESCount(); i++) {
        allDevices[PAS_MPESType].push_back(m_platform->getMPES(i)->getIdentity());
    }

    allDevices[PAS_PSDType] = {};

    for (const auto &pair: allDevices) {
        int expectedDevices;
        if (pair.first == PAS_PanelType) {
            expectedDevices = 1;
        } else if (pair.first == PAS_MPESType) {
            expectedDevices = mpesIdentities.size();
        } else if (pair.first == PAS_ACTType) {
            expectedDevices = actuatorIdentities.size();
        } else if (pair.first == PAS_PSDType) {
            expectedDevices = 0;
        }

        spdlog::info("Expecting {} {} hardware interfaces...", expectedDevices, deviceTypes.at(pair.first));

        // If the device type does not already exist in the m_pControllers map, add it
        m_pControllers[pair.first] = std::map<Device::Identity, std::shared_ptr<PasControllerCommon>>();

        int failed = 0;
        std::shared_ptr<PasControllerCommon> pController;
        for (int i = 0; i < pair.second.size(); i++) {
            Device::Identity identity = pair.second.at(i);
            if (pair.first == PAS_PanelType) {
                pController = std::dynamic_pointer_cast<PasControllerCommon>(
                    std::make_shared<PanelController>(identity, m_platform));
                spdlog::info("Added Panel controller with identity {}", identity);
            } else if (pair.first == PAS_MPESType) {
                pController = std::dynamic_pointer_cast<PasControllerCommon>(
                    std::make_shared<MPESController>(identity, m_platform));
                spdlog::info("Added MPES controller with identity {}", identity);
            } else if (pair.first == PAS_ACTType) {
                pController = std::dynamic_pointer_cast<PasControllerCommon>(
                    std::make_shared<ActController>(identity, m_platform));
                spdlog::info("Added Actuator controller with identity {}", identity);
            }
#ifndef _AMD64
            else if (pair.first == PAS_PSDType) {
                pController = std::dynamic_pointer_cast<PasControllerCommon>(std::make_shared<PSDController>(identity));
                spdlog::info("Added PSD controller with identity {}", identity);
            }
#endif
            else {
                spdlog::error("Invalid device type {} found", pair.first);
            }

            if (pController->initialize()) {
                spdlog::debug("Successfully initialized {} controller with identity {}.",
                              deviceTypes.at(pair.first), pController->getIdentity());
            } else {
                spdlog::error("Failed to initialize {} controller with identity {}.", deviceTypes.at(pair.first),
                              pController->getIdentity());
                failed++;
            }
            m_pControllers[pair.first].insert(std::make_pair(identity, pController));
        }
        spdlog::info("Successfully initialized {}/{} {} controllers.", m_pControllers[pair.first].size(),
                     expectedDevices, deviceTypes.at(pair.first));
    }

    spdlog::info("Adding actuator and mpes controllers as children of the panel controller...");
    if (m_pControllers[PAS_PanelType].size() != 1) {
        spdlog::error("{} panel(s) found. There should be exactly 1 per server.", m_pControllers[PAS_PanelType].size());
        return OpcUa_Bad;
    }
    std::shared_ptr<PanelController> pPanel = std::dynamic_pointer_cast<PanelController>(
        m_pControllers[PAS_PanelType].begin()->second); // get the first panel and assign actuators to it in order
    for (const auto &actId : actuatorIdentities) {
        std::shared_ptr<ActController> pActuator = std::dynamic_pointer_cast<ActController>(
                getDevice(PAS_ACTType, actId));
        pPanel->addActuator(pActuator);
    }
    for (const auto &mpesId : mpesIdentities) {
        try {
            std::shared_ptr <MPESController> pMPES = std::dynamic_pointer_cast<MPESController>(
                    getDevice(PAS_MPESType, mpesId));
            pPanel->addMPES(pMPES);
        }
        catch (...) {
        }
    }

    // start(); // start the thread managed by this object
    return status;
}

UaStatus PasCommunicationInterface::getDeviceState(
    OpcUa_UInt32 deviceType,
    const Device::Identity &identity,
    Device::DeviceState &state) {
    try {
        return m_pControllers.at(deviceType).at(identity)->getState(state);
    }
    catch (std::out_of_range &e) {
        return OpcUa_BadInvalidArgument;
    }
}

UaStatus PasCommunicationInterface::setDeviceState(
    OpcUa_UInt32 deviceType,
    const Device::Identity &identity,
    Device::DeviceState state) {
    try {
        return m_pControllers.at(deviceType).at(identity)->setState(state);
    }
    catch (std::out_of_range &e) {
        return OpcUa_BadInvalidArgument;
    }
}

UaStatus PasCommunicationInterface::getDeviceData(
    OpcUa_UInt32 deviceType,
    const Device::Identity &identity,
    OpcUa_UInt32 offset,
    UaVariant &value) {
    try {
        return m_pControllers.at(deviceType).at(identity)->getData(offset, value);
    }
    catch (std::out_of_range &e) {
        return OpcUa_BadInvalidArgument;
    }
}

UaStatus PasCommunicationInterface::setDeviceData(
    OpcUa_UInt32 deviceType,
    const Device::Identity &identity,
    OpcUa_UInt32 offset,
    UaVariant value) {
    try {
        return m_pControllers.at(deviceType).at(identity)->setData(offset, value);
    }
    catch (std::out_of_range &e) {
        return OpcUa_BadInvalidArgument;
    }
}

UaStatus PasCommunicationInterface::operateDevice(
    OpcUa_UInt32 deviceType,
    const Device::Identity &identity,
    OpcUa_UInt32 offset,
    const UaVariantArray &args) {
    try {
        return m_pControllers.at(deviceType).at(identity)->operate(offset, args);
    }
    catch (std::out_of_range &e) {
        return OpcUa_BadInvalidArgument;
    }
}
