/******************************************************************************
** Project: MPES server
**
** Description: Communication interface to access the sensors.
******************************************************************************/
#include "pascommunicationinterface.h"
#include "passervertypeids.h"
#include "pascontroller.h"
#include "platform.hpp"
#include "components.h"
#include "uadatetime.h"
#include <string>
#include <array>
#include <iostream>

// MySQL C++ Connector includes
#include "mysql_driver.h"
#include "cppconn/statement.h"
#include <cppconn/exception.h>
#include "DBConfig.hpp"

using namespace std;
/* ----------------------------------------------------------------------------
    Begin Class    PasCommunicationInterface
    constructors / destructors
-----------------------------------------------------------------------------*/
// initialize the image directory path
//const char *PasCommunicationInterface::m_imageDir = "/home/root/opcua/sdk/mpesServer/testimages/";

PasCommunicationInterface::PasCommunicationInterface() :
    m_serverUrl(""),
    m_stop(OpcUa_False),
    m_platform(nullptr)
{
}

PasCommunicationInterface::~PasCommunicationInterface()
{
    // Signal  Thread to stop
    m_stop = OpcUa_True;

    // Wait until  Thread stopped
    // wait();
    // delete all the allocated objects by traversing the vectors of device types
    for (auto& devVector : m_pControllers) {
        for (auto& dev : devVector.second)
            delete dev;
        devVector.second.clear();
    }

    // delete platform object.
    if (m_platform)
    {
        delete m_platform;
        m_platform = NULL;
    }

    cout << "\nclosed and cleaned up CommIf\n";
}
/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       Initialize
    Description  Initialize the interface -- should be done outside of the constructor
-----------------------------------------------------------------------------*/
UaStatusCode PasCommunicationInterface::Initialize()
{
    UaStatusCode status;

    int cbc_id = 0;
    int panel_position = 0;
    map<int, int> actuator_positionSerialMap;
    map<int, int> actuator_positionPortMap;
    map<int, int> mpes_positionSerialMap;
    map<int, int> mpes_positionPortMap;

    /* *********************************************************/
    /* INITIAL DATABASE HACK JOB -- NEEDS TO HAVE ITS OWN CLASS */
    Actuator::DBStruct HDB_ID;
    DBConfig myconfig = DBConfig::getDefaultConfig();
    HDB_ID.ip=myconfig.getHost();
    HDB_ID.port=std::to_string(myconfig.getPort());
    HDB_ID.user=myconfig.getUser();
    HDB_ID.password=myconfig.getPassword();
    HDB_ID.dbname=myconfig.getDatabase();
    string db_address = "tcp://" + HDB_ID.ip + ":" + HDB_ID.port;

    try {
        sql::Driver *sql_driver = get_driver_instance();
        sql::Connection *sql_conn = sql_driver->connect(db_address, HDB_ID.user, HDB_ID.password);
        sql_conn->setSchema(HDB_ID.dbname);
        sql::Statement *sql_stmt = sql_conn->createStatement();
        sql::ResultSet *sql_results;

        // get panel position and cbc id by IP address
        string query = "SELECT mpcb_id, position FROM Opt_MPMMapping WHERE end_date is NULL and mpcb_ip_address='"
            + m_serverUrl + "'";
        sql_stmt->execute(query);
        sql_results = sql_stmt->getResultSet();
        // should only be one result FOR NOW -- IN THE FUTURE, NEED TO FIX THIS, SORTING BY DATE
        while (sql_results->next()) {
            cbc_id = sql_results->getInt(1);
            panel_position = sql_results->getInt(2);
        }
        cout << "Will initialiaze Panel " << panel_position << " with CBC " << cbc_id << endl;

        // get actuator serials and ports
        query = "SELECT port, serial_number, position FROM Opt_ActuatorMapping WHERE end_date is NULL and panel=" + to_string(panel_position);
        sql_stmt->execute(query);
        sql_results = sql_stmt->getResultSet();
        while (sql_results->next()) {
            actuator_positionPortMap[sql_results->getInt(3)] = sql_results->getInt(1);
            actuator_positionSerialMap[sql_results->getInt(3)] = sql_results->getInt(2);
        }

        // get mpes serials and ports
        query = "SELECT port, serial_number, w_position FROM Opt_MPESMapping WHERE end_date is NULL and w_panel=" + to_string(panel_position);
        sql_stmt->execute(query);
        sql_results = sql_stmt->getResultSet();
        while (sql_results->next()) {
            mpes_positionPortMap[sql_results->getInt(3)] = sql_results->getInt(1);
            mpes_positionSerialMap[sql_results->getInt(3)] = sql_results->getInt(2);
        }
    }
    catch (sql::SQLException &e) {
        cout << "# ERR: SQLException in " << __FILE__;
        cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
        cout << "# ERR: " << e.what();
        cout << " (MySQL error code: " << e.getErrorCode();
        cout << ", SQLState: " << e.getSQLState() << " )" << endl;

        return OpcUa_Bad;
    }
    /* END OF DATABASE HACK JOB */
    /* *********************************************************/

    // construct the panel with the actuators in the position order
    // the map is sorted by the first index, position
    array<int, 6> actuator_ports;
    array<int, 6> actuator_serials;
    for (auto act : actuator_positionPortMap)
        actuator_ports[act.first - 1] = act.second;
    for (auto act : actuator_positionSerialMap)
        actuator_serials[act.first - 1] = act.second;

    m_platform = new Platform(cbc_id, actuator_ports, actuator_serials, HDB_ID);
    // initialize the MPES in the positional order. Not strictly necessary, but keeps things tidy
    for (auto mpes : mpes_positionPortMap)
        m_platform->addMPES(mpes.second, mpes_positionSerialMap.at(mpes.first));

    // count the devices
    m_DeviceCounts[PAS_PanelType] = 1; // one panel, this one
    m_DeviceCounts[PAS_ACTType] = m_platform->getActuatorCount();
    m_DeviceCounts[PAS_MPESType] = m_platform->getMPESCount();
    m_DeviceCounts[PAS_PSDType] = 0; // hard-coded for now

    PasController *pController;
    for (const auto& devCount: m_DeviceCounts) {
        cout << "Expecting " << devCount.second << " "
                 << m_DeviceTypeNames.at(devCount.first) << " devices" << endl;
        if ( devCount.second > 0)
            cout << "Attempting to create their virtual counterparts..." << endl;

        int failed = 0; // keep track of devices that fail to initialize
        for (unsigned i = 0; i < devCount.second; i++) {
            // i keeps track of the "internal index", and we intialize all objects
            // with this internal index as the ID. this allows us not to have any extra maps
            pController = nullptr;

            int USB = 0;
            if (devCount.first == PAS_PanelType) {
                pController = static_cast<PasController *>(new PasPanel(i, m_platform));
                USB = i;
            }
            else if (devCount.first == PAS_MPESType) {
                pController = static_cast<PasController *>(new PasMPES(i, m_platform));
                USB = m_platform->getMPESAt(i)->GetPortNumber();
            }
            else if (devCount.first == PAS_ACTType) {
                pController = static_cast<PasController *>(new PasACT(i, m_platform));
                USB = m_platform->getActuatorAt(i)->GetPortNumber();
            }
#ifndef _AMD64
            else if (devCount.first == PAS_PSDType) {
                pController = static_cast<PasController *>(new PasPSD(i));
                USB = i;
            }
#endif

            // commenting this out so that the devices are there even if they don't get
            // initialized -- we'd like to keep them in case they manage to initialize at some
            // point AFTER the regular startup procedure
//            if (pController->Initialize()) {
                m_pControllers[devCount.first].push_back(pController);
                m_DeviceUsbMap[devCount.first][USB] = m_pControllers.at(devCount.first).size() - 1;
//           }
//           else {
           if (!pController->Initialize()) {
                cout << "Could not Initialize " << m_DeviceTypeNames.at(devCount.first)
                         << " at USB " << USB << endl;
//                delete pController;
                ++failed;
            }
        }
        // update the number of devices to the ones initialized
//        m_DeviceCounts.at(devCount.first) -= failed;
        if (failed) {
            cout << "\n +++ WARNING +++ Failed to initialize " << failed << " "
                << m_DeviceTypeNames.at(devCount.first) << " devices!" << endl;
        }
    }

    try {
        // get the first panel and assign actuators to it
        PasPanel *pPanel = static_cast<PasPanel *>(m_pControllers.at(PAS_PanelType).at(0));
        for (const auto& act : m_pControllers.at(PAS_ACTType))
            pPanel->addActuator(static_cast<PasACT *>(act));
    }
    catch (out_of_range) {
        // nothing to do
    }

    // start(); // start the thread mananged by this object
    return status;
}
/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       getDevices
    Description  Get available devices of requested type
-----------------------------------------------------------------------------*/
OpcUa_Int32 PasCommunicationInterface::getDevices(OpcUa_UInt32 deviceType)
{
    try {
        return m_DeviceCounts.at(deviceType);
    }
    catch (out_of_range) {
        return 0;
    }
}
/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       getDeviceConfig
    Description  Get configuration of a sensor.
-----------------------------------------------------------------------------*/
UaStatusCode PasCommunicationInterface::getDeviceConfig(
    OpcUa_UInt32    type,
    OpcUa_UInt32    deviceIndex,
    UaString&       sName,
    Identity&   identity)
{
    int realid;
    try {
        realid = m_pControllers.at(type).at(deviceIndex)->getId();
    }
    catch (out_of_range) {
        return OpcUa_BadInvalidArgument;
    }

    if ( type == PAS_MPESType)
        identity.eAddress = to_string(m_platform->getMPESAt(realid)->GetPortNumber());
    else if (type == PAS_ACTType)
        identity.eAddress = to_string(m_platform->getActuatorAt(realid)->GetPortNumber());
    else
        identity.eAddress = to_string(realid);

    sName = UaString(m_DeviceTypeNames.at(type).c_str()) + "_" + identity.eAddress.c_str();

    return OpcUa_Good;
}
/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       getDeviceState
    Description  Get Device status.
-----------------------------------------------------------------------------*/
UaStatusCode PasCommunicationInterface::getDeviceState(
    OpcUa_UInt32 type,
    const Identity& identity,
    PASState&    state)
{
    OpcUa_Int32 address = stoi(identity.eAddress);
    try {
        OpcUa_Int32 deviceIndex = m_DeviceUsbMap.at(type).at(address);
        return m_pControllers.at(type).at(deviceIndex)->getState(state);
    }
    catch (out_of_range) {
        return OpcUa_BadInvalidArgument;
    }
}
/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       setSensorState
    Description  Set Sensor status.
-----------------------------------------------------------------------------*/
UaStatusCode PasCommunicationInterface::setDeviceState(
    OpcUa_UInt32 type,
    const Identity& identity,
    PASState state)
{
    OpcUa_Int32 address = stoi(identity.eAddress);
    try {
        OpcUa_Int32 deviceIndex = m_DeviceUsbMap.at(type).at(address);
        return m_pControllers.at(type).at(deviceIndex)->setState(state);
    }
    catch (out_of_range) {
        return OpcUa_BadInvalidArgument;
    }
}
/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       getDeviceData
    Description  Get Sensor data.
-----------------------------------------------------------------------------*/
UaStatusCode PasCommunicationInterface::getDeviceData(
    OpcUa_UInt32 type,
    const Identity& identity,
    OpcUa_UInt32 offset,
    UaVariant& value)
{
    OpcUa_Int32 address = stoi(identity.eAddress);
    try {
        OpcUa_UInt32 deviceIndex = m_DeviceUsbMap.at(type).at(address);
        return m_pControllers.at(type).at(deviceIndex)->getData(offset, value);
    }
    catch (out_of_range) {
        return OpcUa_BadInvalidArgument;
    }
}
/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       setSensorData
    Description  Set Sensor data.
-----------------------------------------------------------------------------*/
UaStatusCode PasCommunicationInterface::setDeviceData(
    OpcUa_UInt32 type,
    const Identity& identity,
    OpcUa_UInt32 offset,
    UaVariant value)
{
    OpcUa_Int32 address = stoi(identity.eAddress);
    try {
        OpcUa_Int32 deviceIndex = m_DeviceUsbMap.at(type).at(address);
        return m_pControllers.at(type).at(deviceIndex)->setData(offset, value);
    }
    catch (out_of_range) {
        return OpcUa_BadInvalidArgument;
    }
}

/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       OperateDevice
    Description  Run a method on a device.
-----------------------------------------------------------------------------*/
UaStatusCode PasCommunicationInterface::OperateDevice(
    OpcUa_UInt32 type,
    const Identity& identity,
    OpcUa_UInt32 offset,
    const UaVariantArray& args)
{
    OpcUa_Int32 address = stoi(identity.eAddress);
    try {
        OpcUa_Int32 deviceIndex = m_DeviceUsbMap.at(type).at(address);
        return m_pControllers.at(type).at(deviceIndex)->Operate(offset, args);
    }
    catch (out_of_range) {
        return OpcUa_BadInvalidArgument;
    }
}

/* ----------------------------------------------------------------------------
    Class        PasCommunicationInterface
    Method       run
    Description   Thread main function.
-----------------------------------------------------------------------------*
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
                cout << "******** running MPES " << i+1 << " ********" << endl;
                Identity id;
                id.eAddress = to_string(m_platform->getMPESAt(i)->USBPortNumber);
                status = OperateDevice(PAS_MPESType, id);
                cout << "MPES " << i << " " 
                << UaDateTime::now().toTime_t(); 
                for ( int j = 0; j < 5; j++ ) {
                    getDeviceData(PAS_MPESType, id, j, curValue);
                    cout << " " << curValue;
                }
                cout << endl;
            }
            last = UaDateTime::now();
        }
    }
} */
/* ----------------------------------------------------------------------------
    End Class     PasCommunicationInterface
    constructors / destructors
------------------------------------------- ---------------------------------*/
