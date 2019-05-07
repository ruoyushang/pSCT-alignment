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

/// @details Uses hardcoded DB login info to access the device database and retrieve
/// all device mappings and positions. Then, initializes all corresponding controllers, adds all MPES to the platform object,
/// and attaches all actuator controllers to the corresponding panel controller.
UaStatus PasCommunicationInterface::initialize() {
    /// @warning Hardcoded database information. Should be moved to external configuration.
    Device::DBInfo DbInfo;
    DbInfo.host = "remus.ucsc.edu";
    DbInfo.port = "3406";
    DbInfo.user = "CTAreadonly";
    DbInfo.password = "readCTAdb";
    DbInfo.dbname = "CTAonline";
    std::string dbAddress = "tcp://" + DbInfo.host + ":" + DbInfo.port;

    UaStatus status;

    int cbcID = -1;
    int panelPosition = -1;
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
                            + m_panelNum + "'";
        pSqlStmt->execute(query);
        pSqlResults.reset(pSqlStmt->getResultSet());

        // Should only be one result FOR NOW -- IN THE FUTURE, NEED TO FIX THIS, SORTING BY DATE
        while (pSqlResults->next()) {
            cbcID = pSqlResults->getInt(1);
            panelPosition = pSqlResults->getInt(2);
        }
        if (cbcID == -1 || panelPosition == -1) {
            std::cout << "Error: Failed to get valid cbcID or panelPosition. Cannot initialize panel.\n";
            return OpcUa_Bad;
        }
        std::cout << "Will initialize Panel " << panelPosition << " with CBC " << cbcID << std::endl;

        // Query DB for actuator serials and ports
        query = "SELECT port, serial_number, position FROM Opt_ActuatorMapping WHERE panel=" +
                std::to_string(panelPosition);
        pSqlStmt->execute(query);
        pSqlResults.reset(pSqlStmt->getResultSet());
        while (pSqlResults->next()) {
            actPositionToPort[pSqlResults->getInt(3)] = pSqlResults->getInt(1);
            actPositionToSerial[pSqlResults->getInt(3)] = pSqlResults->getInt(2);
        }

        // Query DB for mpes serials and ports
        query = "SELECT port, serial_number, w_position FROM Opt_MPESMapping WHERE w_panel=" +
                std::to_string(panelPosition);
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

    m_platform = std::make_shared<Platform>(cbcID, actuatorPorts, actuatorSerials, DbInfo);

    // initialize the MPES in the positional order. Not strictly necessary, but keeps things tidy
    // addMPES(port, serial)
    for (auto mpes : mpesPositionToPort) {
        m_platform->addMPES(mpes.second, mpesPositionToSerial.at(mpes.first));
    }

    std::vector<int> mpesPositions;
    std::vector<int> actPositions;
    for (auto mpes : mpesPositionToPort) {
        mpesPositions.push_back(mpes.first);
    }
    for (auto act : actPositionToPort) {
        actPositions.push_back(act.first);
    }

    // initialize expected devices
    Identity identity;
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

        if (m_pControllers.find(devCount.first) == m_pControllers.end()) {
            m_pControllers[devCount.first] = std::map<Device::Identity, std::shared_ptr<PasControllerCommon>>();
        }

        int pos;
        int eAddress;
        int failed;
        std::shared_ptr<PasControllerCommon> pController;
        for (int i = 0; i < devCount.second; i++) {

            if (devCount.first == PAS_PanelType) {
                identity.name = std::string("Panel_") + std::to_string(cbcID);
                identity.serialNumber = cbcID;
                identity.eAddress = std::to_string(i);
                identity.position = i;
                pController.reset(new PanelController(identity, m_platform));
            } else if (devCount.first == PAS_MPESType) {
                pos = mpesPositions.at(i);
                identity.name = std::string("MPES_") + std::to_string(mpesPositionToSerial.at(pos));
                identity.serialNumber = mpesPositionToSerial.at(pos);
                identity.eAddress = std::to_string(mpesPositionToPort.at(pos));
                identity.position = pos;
                pController.reset(new MPESController(identity, m_platform));
            } else if (devCount.first == PAS_ACTType) {
                pos = actPositions.at(i);
                identity.name = std::string("ACT_") + std::to_string(actPositionToSerial.at(pos));
                identity.serialNumber = actPositionToSerial.at(pos);
                identity.eAddress = std::to_string(actPositionToPort.at(pos));
                identity.position = pos;
                pController.reset(new ActController(identity, m_platform));
            }
#ifndef _AMD64
            else if (devCount.first == PAS_PSDType) {
                identity.eAddress = std::to_string(i);
                pController.reset(new PSDController(identity));
            }
#endif
            else {
                std::cout << "PasCommunicationInterface:: Invalid device type found.\n";
                return OpcUa_Bad;
            }
            identity.eAddress = std::to_string(eAddress);
            if (pController->initialize() == 0) {
                m_pControllers.at(devCount.first).emplace(identity, pController);
            } else {
                std::cout << "Could not initialize " << deviceTypes.at(devCount.first)
                          << " with identity " << identity << std::endl;
            }
        }
        // update the number of devices to the ones initialized
        failed = devCount.second - m_pControllers.at(devCount.first).size();
        if (failed) {
            std::cout << "\n +++ WARNING +++ Failed to initialize " << failed << " "
                      << deviceTypes.at(devCount.first) << " devices!" << std::endl;
        }
    }

    std::cout << "Adding actuators to panel...\n";
    try {
        std::vector<std::shared_ptr<PasControllerCommon>> panels;
        for (const auto &panel : m_pControllers[PAS_PanelType]) {
            panels.push_back(panel.second);
        }
        std::shared_ptr<PanelController> pPanel = std::dynamic_pointer_cast<PanelController>(
                panels.at(0)); // get the first panel and assign actuators to it
        std::shared_ptr<ActController> pACT;
        for (const auto &act : m_pControllers.at(PAS_ACTType)) {
            pACT = std::dynamic_pointer_cast<ActController>(act.second);
            pPanel->addActuator(pACT);
        }
        std::shared_ptr<MPESController> pMPES;
        for (const auto &mpes : m_pControllers.at(PAS_MPESType)) {
            pMPES = std::dynamic_pointer_cast<MPESController>(mpes.second);
            pPanel->addMPES(pMPES);
        }
    }
    catch (std::out_of_range &e) {
    	std::cout << "Failed to add actuators to panel.\n";
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
