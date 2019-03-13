/**
 * @file pascommunicationinterface.cpp
 * @brief Source file for panel server PAS communication interface
 */

#include "server/pascommunicationinterface.hpp"

#include <array>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "common/alignment/actuator.hpp"
#include "common/alignment/platform.hpp"
#include "common/opcua/components.h"
#include "common/opcua/passervertypeids.h"

#include "server/controllers/pascontroller.hpp"

#include "uabase/uadatetime.h"

// MySQL C++ Connector includes
#include "mysql_driver.h"
#include "cppconn/statement.h"
#include <cppconn/exception.h>


PasCommunicationInterface::PasCommunicationInterface() :
    m_serverIP(""),
    m_stop(OpcUa_False),
    m_platform(nullptr)
{
}

PasCommunicationInterface::~PasCommunicationInterface()
{
    m_stop = OpcUa_True; // Signal thread to stop
    // wait(); // Wait until thread stopped

    std::cout << "\nClosed and cleaned up Communication Interface\n";
}

/// @details Uses hardcoded DB login info to access the device database and retrieve
/// all device mappings and positions. Then, initializes all corresponding controllers and attaches
/// all actuators to the corresponding panel.
UaStatusCode PasCommunicationInterface::Initialize()
{
    UaStatusCode status;

    /// @warning Hardcoded database information. Should be moved to external configuration.
    Actuator::DBStruct DbInfo;
    DbInfo.ip="remus.ucsc.edu";
    DbInfo.port="3406";
    DbInfo.user="CTAreadonly";
    DbInfo.password="readCTAdb";
    DbInfo.dbname="CTAonline";
    std::string dbAddress = "tcp://" + DbInfo.ip + ":" + DbInfo.port;

    int cbcID;
    int panelPosition;
    std::map<int, int> actPositionToSerial;
    std::map<int, int> actPositionToPort;
    std::map<int, int> mpesPositionToSerial;
    std::map<int, int> mpesPositionToPort;

    try {
        std::unique_ptr<sql::Driver> pSqlDriver = get_driver_instance();
        std::unique_ptr<sql::Connection> pSqlConn = pSqlDriver->connect(dbAddress, DbInfo.user, DbInfo.password);
        pSqlConn->setSchema(DbInfo.dbname);
        std::unique_ptr<sql::Statement> pSqlStmt = pSqlConn->createStatement();
        std::unique_ptr<sql::ResultSet> pSqlResults;

        // Query DB for panel position and cbc id by IP address
        std::string query = "SELECT mpcb_id, position FROM Opt_MPMMapping WHERE mpcb_ip_address='"
            + m_serverIP + "'";
        pSqlStmt->execute(query);
        pSqlResults = pSqlStmt->getResultSet();

        // Should only be one result FOR NOW -- IN THE FUTURE, NEED TO FIX THIS, SORTING BY DATE
        while (pSqlResults->next()) {
            cbcID = pSqlResults->getInt(1);
            panelPosition = pSqlResults->getInt(2);
        }
        std::cout << "Will initialize Panel " << panelPosition << " with CBC " << cbcID << std::endl;

        // Query DB for actuator serials and ports
        query = "SELECT port, serial_number, position FROM Opt_ActuatorMapping WHERE panel=" + to_string(panelPosition);
        pSqlStmt->execute(query);
        pSqlResults = pSqlStmt->getResultSet();
        while (pSqlResults->next()) {
            actPositionToPort[pSqlResults->getInt(3)] = pSqlResults->getInt(1);
            actPositionToSerial[pSqlResults->getInt(3)] = pSqlResults->getInt(2);
        }

        // Query DB for mpes serials and ports
        query = "SELECT port, serial_number, w_position FROM Opt_MPESMapping WHERE w_panel=" + to_string(panelPosition);
        pSqlStmt->execute(query);
        pSqlResults = pSqlStmt->getResultSet();
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
    std::array<int, 6> actuatorPorts;
    std::array<int, 6> actuatorSerials;
    for (auto act : actPositionToPort)
        actuatorPorts[act.first - 1] = act.second;
    for (auto act : actPositionToSerial)
        actuatorSerials[act.first - 1] = act.second;

    m_platform = new Platform(cbcID, actuatorPorts, actuatorSerials, DbInfo);

    // initialize the MPES in the positional order. Not strictly necessary, but keeps things tidy
    // addMPES(port, serial)
    for (auto mpes : mpesPositionToPort)
        m_platform->addMPES(mpes.second, mpesPositionToSerial.at(mpes.first));

    // Initialize expected devices
    std::map<OpcUa_UInt32, int> expectedDeviceCounts;
    expectedDeviceCounts[PAS_PanelType] = 1;
    expectedDeviceCounts[PAS_ACTType] = m_platform->getActuatorCount();
    expectedDeviceCounts[PAS_MPESType] = m_platform->getMPESCount();
    expectedDeviceCounts[PAS_PSDType] = 0;

    for (auto devCount: expectedDeviceCounts) {
        std::cout << "Expecting " << devCount.second << " "
                 << deviceTypes.at(devCount.first) << " devices.\n";
        if (d.second > 0)
            std::cout << "Attempting to create their virtual counterparts...\n";

        int failed = 0; // keep track of devices that fail to initialize
        std::shared_ptr<PasController> pController;
        for (int i = 0; i < d.second; i++) {

            int eAddress;
            if (devCount.first == PAS_PanelType) {
                pController = std::shared_ptr<PasController>(new PanelController(i, m_platform));
                eAddress = i;
            }
            else if (devCount.first == PAS_MPESType) {
                pController = std::shared_ptr<PasController>(new MPESController(i, m_platform));
                eAddress = m_platform->getMPESAt(i)->GetPortNumber();
            }
            else if (devCount.first == PAS_ACTType) {
                pController = std::shared_ptr<PasController>(new ActController(i, m_platform));
                eAddress = m_platform->getActuatorAt(i)->GetPortNumber();
            }
#ifndef _AMD64
            else if (devCount.first == PAS_PSDType) {
                pController = std::shared_ptr<PasController>(new PSDController(i));
                eAddress = i;
            }
#endif

            if (pController->Initialize()) {
                m_pControllers.at(devCount.first)[eAddress] = pController;
            }
            else {
                std::cout << "Could not Initialize " << deviceTypes.at(devCount.first)
                         << " at eAddress " << eAddress << std::endl;
                ++failed;
            }
        }
        // update the number of devices to the ones initialized
        m_DeviceCounts.at(devCount.first) -= failed;
        if (failed) {
            std::cout << "\n +++ WARNING +++ Failed to initialize " << failed << " "
                << m_DeviceTypeNames.at(devCount.first) << " devices!" << std::endl;
        }
    }

    try {
        std::shared_pointer<PanelController> pPanel = std::dynamic_pointer_cast<PanelController>(m_pControllers.at(PAS_PanelType).at(0)); // get the first panel and assign actuators to it
        std::shared_pointer<ActController> pACT;
        for (auto a : m_pControllers.at(PAS_ACTType)) {
            pACT = std::dynamic_pointer_cast<ActController>(a.second);
            pPanel->addActuator(pACT);
          }
    }
    catch (out_of_range) {
    }

    // start(); // start the thread mananged by this object
    return status;
}

/// @details Returns -1 on invalid device type ID.
std::size_t PasCommunicationInterface::getDeviceCount(OpcUa_UInt32 deviceType)
{
    try {
        return m_pController.at(deviceType).size();
    }
    catch (out_of_range) {
        return -1;
    }
}

std::vector<int> getValidDeviceAddresses(OpcUa_UInt32 deviceType)
{
  std::vector<int> validAddresses;
  for(auto it = m_pControllers.at(deviceType).begin(); it != m_pControllers.at(deviceType).end(); ++it) {
    validAddresses.push_back(it->first);
  }

  return validAddresses;
}

/**
/// @details Retreives the device index (on the platform) for the desired controller
/// and uses it to get the identity in
UaStatusCode PasCommunicationInterface::getDeviceInfo(
    OpcUa_UInt32 deviceType,
    int eAddress,
    UaString& sName,
    Identity& identity)
{
    int idx;
    try {
        idx = m_pControllers.at(deviceType).at(eAddress)->getId();
    }
    catch (out_of_range) {
        return OpcUa_BadInvalidArgument;
    }

    if ( deviceType == PAS_MPESType)
        identity.eAddress = to_string(m_platform->getMPESAt(idx)->GetPortNumber());
    else if (deviceType == PAS_ACTType)
        identity.eAddress = to_string(m_platform->getActuatorAt(idx)->GetPortNumber());
    else
        identity.eAddress = to_string(idx);

    std::map<OpcUa_UInt32, std::string> deviceTypeToName;
    for (auto it = deviceTypes.begin(); it != deviceTypes.end(); ++it)
    {
      deviceTypeToName[it->second] = it->first;
    }

    sName = UaString(deviceTypeToName.at(deviceType).c_str()) + "_" + identity.eAddress.c_str();

    return OpcUa_Good;
}
*/

UaStatusCode PasCommunicationInterface::getDeviceState(
    OpcUa_UInt32 deviceType,
    int eAddress,
    PASState& state)
{
    try {
        return m_pControllers.at(deviceType).at(eAddress)->getState(state);
    }
    catch (out_of_range) {
        return OpcUa_BadInvalidArgument;
    }
}

UaStatusCode PasCommunicationInterface::setDeviceState(
    OpcUa_UInt32 deviceType,
    int eAddress,
    PASState state)
{
    try {
        return m_pControllers.at(deviceType).at(eAddress)->setState(state);
    }
    catch (out_of_range) {
        return OpcUa_BadInvalidArgument;
    }
}

UaStatusCode PasCommunicationInterface::getDeviceData(
    OpcUa_UInt32 deviceType,
    int eAddress,
    OpcUa_UInt32 offset,
    UaVariant& value)
{
    try {
        return m_pControllers.at(deviceType).at(eAddress)->getData(offset, value);
    }
    catch (out_of_range) {
        return OpcUa_BadInvalidArgument;
    }
}

UaStatusCode PasCommunicationInterface::setDeviceData(
    OpcUa_UInt32 deviceType,
    int eAddress,
    OpcUa_UInt32 offset,
    UaVariant value)
{
    try {
        return m_pControllers.at(deviceType).at(eAddress)->setData(offset, value);
    }
    catch (out_of_range) {
        return OpcUa_BadInvalidArgument;
    }
}

UaStatusCode PasCommunicationInterface::OperateDevice(
    OpcUa_UInt32 deviceType,
    int eAddress,
    OpcUa_UInt32 offset,
    const UaVariantArray& args)
{
    try {
        return m_pControllers.at(deviceType).at(eAddress)->Operate(offset, args);
    }
    catch (out_of_range) {
        return OpcUa_BadInvalidArgument;
    }
}

/**
void PasCommunicationInterface::run()
{
    UaStatusCode status;
    UaDateTime last = UaDateTime::now();
    OpcUa_UInt32 i;
    OpcUa_UInt32 count = getCountSensors();

    OpcUa_Double curValue;

    while( m_stop == OpcUa_False )
    {
        if ( last.msecsTo(UaDateTime::now()) >= 1000 )
        {
            for ( i=0; i<count; i++ )
            {
                std::cout << "******** running MPES " << i+1 << " ********" << std::endl;
                Identity id;
                id.eAddress = to_string(m_platform->getMPESAt(i)->USBPortNumber);
                status = OperateDevice(PAS_MPESType, id);
                std::cout << "MPES " << i << " "
                << UaDateTime::now().toTime_t();
                for ( int j = 0; j < 5; j++ ) {
                    getDeviceData(PAS_MPESType, id, j, curValue);
                    std::cout << " " << curValue;
                }
                std::cout << endl;
            }
            last = UaDateTime::now();
        }
    }
} */
