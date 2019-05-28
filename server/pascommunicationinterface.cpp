/**
 * @file pascommunicationinterface.cpp
 * @brief Source file for panel server PAS communication interface
 */

#include "server/pascommunicationinterface.hpp"

#include <array>
#include <iostream>
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

    std::map<int, int> actPositionToSerial;
    std::map<int, int> actPositionToPort;
    std::map<int, int> mpesPositionToSerial;
    std::map<int, int> mpesPositionToPort;

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
        if (m_cbcID == -1 || m_panelNum == -1) {
            std::cout << "Error: Failed to get valid cbcID or panelPosition. Cannot initialize panel.\n";
            return OpcUa_Bad;
        }
        std::cout << "Will initialize Panel " << m_panelNum << " with CBC " << m_cbcID << std::endl;

        // Query DB for actuator serials and ports
        query = "SELECT port, serial_number, position FROM Opt_ActuatorMapping WHERE panel=" +
                std::to_string(m_panelNum);
        pSqlStmt->execute(query);
        pSqlResults.reset(pSqlStmt->getResultSet());
        while (pSqlResults->next()) {
            actPositionToPort[pSqlResults->getInt(3)] = pSqlResults->getInt(1);
            actPositionToSerial[pSqlResults->getInt(3)] = pSqlResults->getInt(2);
        }

        // Query DB for mpes serials and ports
        query = "SELECT port, serial_number, w_position FROM Opt_MPESMapping WHERE w_panel=" +
                std::to_string(m_panelNum);
        pSqlStmt->execute(query);
        pSqlResults.reset(pSqlStmt->getResultSet());
        while (pSqlResults->next()) {
            mpesPositionToPort[pSqlResults->getInt(3)] = pSqlResults->getInt(1);
            mpesPositionToSerial[pSqlResults->getInt(3)] = pSqlResults->getInt(2);
        }
    }
    catch (sql::SQLException &e) {
        std::cout << "# ERR: SQLException in " << __FILE__;
        std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
        std::cout << "# ERR: " << e.what();
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;

        return OpcUa_Bad;
    }

    // construct arrays of the actuator ports and serial numbers
    // ordered by position
    std::array<int, 6> actuatorPorts{};
    std::array<int, 6> actuatorSerials{};
    for (auto act : actPositionToPort)
        actuatorPorts[act.first - 1] = act.second;
    for (auto act : actPositionToSerial)
        actuatorSerials[act.first - 1] = act.second;

    Device::Identity identity;
    identity.name = std::string("Panel_") + std::to_string(m_panelNum);
    identity.serialNumber = m_cbcID;
    identity.eAddress = std::to_string(0);
    identity.position = m_panelNum;

    m_platform = std::make_shared<Platform>(identity, actuatorPorts, actuatorSerials, DbInfo);

    std::vector<int> mpesPositions;
    std::vector<int> actPositions;
    for (auto mpes : mpesPositionToPort) {
        mpesPositions.push_back(mpes.first);
    }
    for (auto act : actPositionToPort) {
        actPositions.push_back(act.first);
    }

    // initialize the MPES in the positional order. Not strictly necessary, but keeps things tidy
    // addMPES(port, serial)
    int pos;
    for (auto mpes : mpesPositionToPort) {
	identity.name = std::string("MPES_") + std::to_string(mpesPositionToSerial.at(mpes.first));
	identity.serialNumber = mpesPositionToSerial.at(mpes.first);
	identity.eAddress = std::to_string(mpes.second);
	identity.position = mpes.first;
        m_platform->addMPES(identity);
    }

    // initialize expected devices
    std::map<OpcUa_UInt32, int> expectedDeviceCounts;
    expectedDeviceCounts[PAS_PanelType] = 1;
    expectedDeviceCounts[PAS_ACTType] = m_platform->getActuatorCount();
    expectedDeviceCounts[PAS_MPESType] = m_platform->getMPESCount();
    expectedDeviceCounts[PAS_PSDType] = 0;

    for (auto devCount: expectedDeviceCounts) {
        std::cout << "Expecting " << devCount.second << " "
                  << deviceTypes.at(devCount.first) << " devices.\n";
        if (devCount.second > 0)
            std::cout << "Attempting to create their virtual counterparts...\n";

        // If the device type does not already exist in the m_pControllers map, add it
        m_pControllers[devCount.first] = std::map<Device::Identity, std::shared_ptr<PasControllerCommon>>();

        int failed;
        std::shared_ptr<PasControllerCommon> pController;
        for (int i = 0; i < devCount.second; i++) {
            if (devCount.first == PAS_PanelType) {
                identity.name = std::string("Panel_") + std::to_string(m_panelNum);
                identity.serialNumber = m_cbcID;
                identity.eAddress = std::to_string(0);
                identity.position = m_panelNum;
                pController = std::dynamic_pointer_cast<PasControllerCommon>(
                    std::make_shared<PanelController>(identity, m_platform));
            } else if (devCount.first == PAS_MPESType) {
                pos = mpesPositions.at(i);
                std::cout << pos << std::endl;
                identity.name = std::string("MPES_") + std::to_string(mpesPositionToSerial.at(pos));
                std::cout << mpesPositionToSerial.at(pos) << std::endl;
                identity.serialNumber = mpesPositionToSerial.at(pos);
                std::cout << mpesPositionToPort.at(pos) << std::endl;
                identity.eAddress = std::to_string(mpesPositionToPort.at(pos));
                identity.position = pos;
                pController = std::dynamic_pointer_cast<PasControllerCommon>(
                    std::make_shared<MPESController>(identity, m_platform));
            } else if (devCount.first == PAS_ACTType) {
                pos = actPositions.at(i);
                identity.name = std::string("ACT_") + std::to_string(actPositionToSerial.at(pos));
                identity.serialNumber = actPositionToSerial.at(pos);
                identity.eAddress = std::to_string(actPositionToPort.at(pos));
                identity.position = pos;
                pController = std::dynamic_pointer_cast<PasControllerCommon>(
                    std::make_shared<ActController>(identity, m_platform));
            }
#ifndef _AMD64
            else if (devCount.first == PAS_PSDType) {
                identity.name = std::string("PSD_0");
                identity.serialNumber = -1;
                identity.eAddress = "0";
                identity.position = -1;
                pController = std::dynamic_pointer_cast<PasControllerCommon>(std::make_shared<PSDController>(identity));
            }
#endif
            else {
                std::cout << "PasCommunicationInterface:: Invalid device type found.\n";
            }
            if (pController->initialize()) {
                m_pControllers[devCount.first].insert(std::make_pair(identity, pController));
            } else {
                std::cout << "Could not initialize " << deviceTypes.at(devCount.first)
                          << " with identity " << identity << std::endl;
            }
        }
        // update the number of devices to the ones initialized
        failed = devCount.second - m_pControllers[devCount.first].size();
        if (failed) {
            std::cout << "\n +++ WARNING +++ Failed to initialize " << failed << " "
                      << deviceTypes.at(devCount.first) << " devices!" << std::endl;
        }
    }

    std::cout << "Adding actuators and MPES children to panel...\n";
    try {
        if (m_pControllers[PAS_PanelType].size() != 1) {
            std::cout << "Error: " << m_pControllers[PAS_PanelType].size()
                      << " panel(s) found. There should be exactly 1 per server." << std::endl;
            return OpcUa_Bad;
        }
        std::shared_ptr<PanelController> pPanel = std::dynamic_pointer_cast<PanelController>(
            m_pControllers[PAS_PanelType].begin()->second); // get the first panel and assign actuators to it
        for (const auto &act : m_pControllers[PAS_ACTType]) {
            pPanel->addActuator(std::dynamic_pointer_cast<ActController>(act.second));
        }
        for (const auto &mpes : m_pControllers[PAS_MPESType]) {
            pPanel->addMPES(std::dynamic_pointer_cast<MPESController>(mpes.second));
        }
    }
    catch (std::out_of_range &e) {
        std::cout << "Failed to add actuators/MPES to panel.\n";
    }

    // start(); // start the thread managed by this object
    return status;
}

UaStatus PasCommunicationInterface::getDeviceState(
    OpcUa_UInt32 deviceType,
    const Device::Identity &identity,
    Device::ErrorState &state) {
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
    Device::ErrorState state) {
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
        std::cout << "failed to find controller " << deviceType << " " << identity << std::endl;
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
