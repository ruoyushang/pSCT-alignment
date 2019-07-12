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
    std::vector<Device::Identity> mpesIdentities;

    spdlog::trace("Connecting to DB {} at {} with user {}", DbInfo.dbname, dbAddress, DbInfo.user);
    try {
        sql::Driver *pSqlDriver = get_driver_instance(); // Need to use naked pointer here to avoid attempting to call protected destructor.
        std::unique_ptr<sql::Connection> pSqlConn = std::unique_ptr<sql::Connection>(
                pSqlDriver->connect(dbAddress, DbInfo.user, DbInfo.password));
        pSqlConn->setSchema(DbInfo.dbname);
        std::unique_ptr<sql::Statement> pSqlStmt = std::unique_ptr<sql::Statement>(pSqlConn->createStatement());
        std::unique_ptr<sql::ResultSet> pSqlResults;

        // Query DB for panel position and m_pCBC id by IP address
        std::string query = "SELECT mpcb_id, position FROM Opt_MPMMapping WHERE position='"
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
        query = "SELECT serial_number, position, port FROM Opt_ActuatorMapping WHERE panel=" +
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
        query = "SELECT serial_number, w_position, port, l_panel FROM Opt_MPESMapping WHERE w_panel=" +
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

    Device::Identity identity;
    identity.name = std::string("Panel_") + std::to_string(m_panelNum);
    identity.serialNumber = m_cbcID;
    identity.eAddress = std::to_string(0);
    identity.position = m_panelNum;

#ifdef SIMMODE
    spdlog::info("SIMMODE: Creating DummyPlatform object with identity {}...", identity);
    m_platform = std::dynamic_pointer_cast<PlatformBase>(std::make_shared<DummyPlatform>(identity, DbInfo));
#else
    spdlog::info("Creating Platform hardware interface with identity {}...", identity);
    m_platform = std::dynamic_pointer_cast<PlatformBase>(std::make_shared<Platform>(identity, DbInfo));
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

    std::map<OpcUa_UInt32, int> expectedDeviceCounts;
    expectedDeviceCounts[PAS_PanelType] = 1;
    expectedDeviceCounts[PAS_ACTType] = m_platform->getActuatorCount();
    expectedDeviceCounts[PAS_MPESType] = m_platform->getMPESCount();
    expectedDeviceCounts[PAS_PSDType] = 0;

    for (auto devCount: expectedDeviceCounts) {
        spdlog::info("Expecting {} {} hardware interfaces...", devCount.second, deviceTypes.at(devCount.first));

        // If the device type does not already exist in the m_pControllers map, add it
        m_pControllers[devCount.first] = std::map<Device::Identity, std::shared_ptr<PasControllerCommon>>();

        int failed = 0;
        std::shared_ptr<PasControllerCommon> pController;
        for (int i = 0; i < devCount.second; i++) {
            if (devCount.first == PAS_PanelType) {
                identity.name = std::string("Panel_") + std::to_string(m_panelNum);
                identity.serialNumber = m_cbcID;
                identity.eAddress = std::to_string(0);
                identity.position = m_panelNum;
                pController = std::dynamic_pointer_cast<PasControllerCommon>(
                    std::make_shared<PanelController>(identity, m_platform));
                spdlog::info("Added Panel controller with identity {}", identity);
            } else if (devCount.first == PAS_MPESType) {
                identity = mpesIdentities[i];
                pController = std::dynamic_pointer_cast<PasControllerCommon>(
                    std::make_shared<MPESController>(identity, m_platform));
                spdlog::info("Added MPES controller with identity {}", identity);
            } else if (devCount.first == PAS_ACTType) {
                identity = actuatorIdentities[i];
                pController = std::dynamic_pointer_cast<PasControllerCommon>(
                    std::make_shared<ActController>(identity, m_platform));
                spdlog::info("Added Actuator controller with identity {}", identity);
            }
#ifndef _AMD64
            else if (devCount.first == PAS_PSDType) {
                identity.name = std::string("PSD_0");
                identity.serialNumber = -1;
                identity.eAddress = "0";
                identity.position = -1;
                pController = std::dynamic_pointer_cast<PasControllerCommon>(std::make_shared<PSDController>(identity));
                spdlog::info("Added PSD controller with identity {}", identity);
            }
#endif
            else {
                spdlog::error("Invalid device type {} found", devCount.first);
            }

            if (pController->initialize()) {
                spdlog::debug("Successfully initialized {} controller with identity {}.",
                              deviceTypes.at(devCount.first), pController->getIdentity());
            } else {
                spdlog::error("Failed to initialize {} controller with identity {}.", deviceTypes.at(devCount.first),
                              pController->getIdentity());
                failed++;
            }
            m_pControllers[devCount.first].insert(std::make_pair(identity, pController));
        }
        spdlog::info("Successfully initialized {}/{} {} controllers.", devCount.second - failed, devCount.second, deviceTypes.at(devCount.first));
    }

    spdlog::info("Adding actuator and mpes controllers as children of the panel controller...");
    if (m_pControllers[PAS_PanelType].size() != 1) {
        spdlog::error("{} panel(s) found. There should be exactly 1 per server.", m_pControllers[PAS_PanelType].size());
        return OpcUa_Bad;
    }
    std::shared_ptr<PanelController> pPanel = std::dynamic_pointer_cast<PanelController>(
        m_pControllers[PAS_PanelType].begin()->second); // get the first panel and assign actuators to it
    for (const auto &actId : actuatorIdentities) {
        pPanel->addActuator(std::dynamic_pointer_cast<ActController>(m_pControllers[PAS_ACTType].at(actId)));
    }
    for (const auto &mpesId : mpesIdentities) {
        pPanel->addMPES(std::dynamic_pointer_cast<MPESController>(m_pControllers[PAS_MPESType].at(mpesId)));
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
