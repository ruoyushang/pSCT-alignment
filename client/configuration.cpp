#include "configuration.h"
#include "mathtools.h"
#include "uasettings.h"
#include "uadir.h"
#include "uapkicertificate.h"
#include <vector>
#include <stdexcept>
#include <algorithm> // max, sort, accumulate

// MySQL C++ Connector includes
#include "mysql_driver.h"
#include "cppconn/statement.h"
#include "DBConfig.hpp"

using namespace UaClientSdk;

Configuration::Configuration()
{
    // initialize the list of panels to an empty one -- other devices don't need this
    m_DeviceList[PAS_PanelType] = {};
}

Configuration::~Configuration()
{
}

OpcUa_UInt32 Configuration::getServers() const
{
    try {
        return m_DeviceList.at(PAS_PanelType).size();
    }
    catch (std::out_of_range)
    {
        return 0;
    }
}

UaString Configuration::getDiscoveryUrl() const
{
    return m_discoveryUrl;
}

UaString Configuration::getPositionerUrl() const
{
    return m_positionerUrl;
}

UaString Configuration::getApplicationName() const
{
    return m_applicationName;
}

OpcUa_Boolean Configuration::getAutomaticReconnect() const
{
    return m_bAutomaticReconnect;
}

OpcUa_Boolean Configuration::getRetryInitialConnect() const
{
    return m_bRetryInitialConnect;
}

UaNodeIdArray Configuration::getNodesToRead() const
{
    return m_nodesToRead;
}

UaNodeIdArray Configuration::getNodesToWrite() const
{
    return m_nodesToWrite;
}

UaNodeIdArray Configuration::getNodesToMonitor() const
{
    return m_nodesToMonitor;
}

UaVariantArray Configuration::getWriteValues() const
{
    return m_writeValues;
}

UaStringArray Configuration::getDatabaseHost() const
{
    return m_databaseHost;
}
UaStringArray Configuration::getDatabaseUser() const
{
    return m_databaseUser;
}
UaStringArray Configuration::getDatabasePassword() const
{
    return m_databasePassword;
}
UaStringArray Configuration::getDatabaseName() const
{
    return m_databaseName;
}
std::vector<UaStringArray> Configuration::getDatabaseEntries() const
{
    return m_databaseEntries;
}

UaStatus Configuration::loadConnectionConfiguration(const UaString& sConfigurationFile)
{
    UaStatus result;
    UaVariant value;
    UaString sTempKey;
    OpcUa_UInt32 i = 0;
    OpcUa_UInt32 size = 0;
    UaSettings* pSettings = nullptr;
    pSettings = new UaSettings(sConfigurationFile.toUtf16());

    pSettings->beginGroup("UaClientConfig");

    // Certificate and trust list locations
    value = pSettings->value("CertificateTrustListLocation");
    m_certificateTrustListLocation = value.toString();
    value = pSettings->value("CertificateRevocationListLocation");
    m_certificateRevocationListLocation = value.toString();
    value = pSettings->value("IssuersCertificatesLocation");
    m_issuersCertificatesLocation = value.toString();
    value = pSettings->value("IssuersRevocationListLocation");
    m_issuersRevocationListLocation = value.toString();
    value = pSettings->value("ClientCertificate");
    m_clientCertificateFile = value.toString();
    value = pSettings->value("ClientPrivateKey");
    m_clientPrivateKeyFile = value.toString();

    // Application Name
    value = pSettings->value("ApplicationName", UaString());
    m_applicationName = value.toString();

    // Reconnection settings
    value = pSettings->value("AutomaticReconnect", UaVariant((OpcUa_Boolean)OpcUa_True));
    value.toBool(m_bAutomaticReconnect);
    value = pSettings->value("RetryInitialConnect", UaVariant((OpcUa_Boolean)OpcUa_False));
    value.toBool(m_bRetryInitialConnect);

    // Server URLs
#if SIMMODE
    const char *  local_ip = getenv("LOCALIP");
    value = pSettings->value("DiscoveryURL", UaString("opc.tcp://%s:48010").arg(local_ip));
#else
    value = pSettings->value("DiscoveryURL", UaString("opc.tcp://172.17.0.201.:48010"));
#endif
    m_discoveryUrl = value.toString();
    value = pSettings->value("PositionerURL", UaString("opc.tcp://127.0.0.1:4840"));
    m_positionerUrl = value.toString();

    // Read NamespaceArray
    m_namespaceArray.clear();
    pSettings->beginGroup("NSArray");
    value = pSettings->value("size", (OpcUa_UInt32)0);
    value.toUInt32(size);
    m_namespaceArray.resize(size);
    for (i=0; i<size;i++)
    {
        sTempKey = UaString("NameSpaceUri%1").arg((int)i, 2, 10, UaChar('0'));
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        value.toString().copyTo(&m_namespaceArray[i]);
    }
    pSettings->endGroup();  // NSArray

    pSettings->endGroup(); // UaClientConfig

    delete pSettings;
    pSettings = NULL;

    return result;

   /*
    pSettings->beginGroup("Server");
    value = pSettings->value("size", (OpcUa_UInt32)0);
    value.toUInt32(size);

    std::string sTempMap;
    std::string sSubMap;
    std::string USB;
    OpcUa_Int32 serial;
    OpcUa_Int32 position;
    int separator1, separator2;
    int subseparator;

    std::map<OpcUa_UInt32, std::string> typeNamesMap = {{PAS_MPESType, "MPES"}, {PAS_ACTType, "ACT"}, {PAS_PSDType, "PSD"}};

    for (i=0; i <size; i++)
    {
        sTempKey = UaString("Server%1").arg((int)i, 2, 10, UaChar('0'));
        pSettings->beginGroup(sTempKey.toUtf16());

        OpcUa_UInt32 panel_pos;
        OpcUa_UInt32 panelSerial;
        sTempKey = UaString("Position");
        value = pSettings->value(sTempKey.toUtf16(), OpcUa_UInt32(0));
        value.toUInt32(panel_pos);

        sTempKey = UaString("Serial");
        value = pSettings->value(sTempKey.toUtf16(), OpcUa_UInt32(0));
        value.toUInt32(panelSerial);

        sTempKey = UaString("Url");
        value = pSettings->value(sTempKey.toUtf16(), UaString("opc.tcp://localhost:48010"));
        std::string panel_address = value.toString().toUtf8();
        // add this to the device address map {type -> {serial -> address}}
        m_DeviceMap[PAS_PanelType][panelSerial] = panel_address;
        // also add this panel to the position map {type -> {serial -> position}}
        m_DevicePositionMap[PAS_PanelType][panelSerial] = panel_pos;
        m_PanelAddressMap[panel_pos] = panel_address;

        for (const auto& it_typeNameMap : typeNamesMap) {
            std::string urlmapstring = it_typeNameMap.second + "UrlMap";
            std::string posmapstring = it_typeNameMap.second + "PositionMap";

            value = pSettings->value(urlmapstring.c_str(), UaString(""));
            sTempMap = std::string(value.toString().toUtf8());
            separator1 = separator2 = 0;
            while (separator2 < int(sTempMap.length()) - 1)
            {
                separator2 = sTempMap.find(';', separator1);
                sSubMap = sTempMap.substr(separator1,separator2-separator1);
                subseparator = sSubMap.find(':');
                USB = sSubMap.substr(0,subseparator);
                serial = atoi(sSubMap.substr(subseparator+1, std::string::npos).c_str());
                // m_deviceSerialMap at (deviceType) at (panel address) at (usb number)
                m_DeviceSerialMap[it_typeNameMap.first]
                                        [UaString(panel_address.c_str())]
                                        [UaString(USB.c_str())] = serial;

                separator1 = separator2 + 1;
                printf("Added the following to %sUrlMap: USB=%s::serial=%d\n", it_typeNameMap.second.c_str(), USB.c_str(), serial);

                m_DeviceMap[it_typeNameMap.first][serial] = m_DeviceMap.at(PAS_PanelType).at(panelSerial);
                printf("Added the following to %s DeviceMap: %d -> %s\n", it_typeNameMap.second.c_str(), serial, m_DeviceMap.at(it_typeNameMap.first).at(serial).c_str());
            }

            value = pSettings->value(posmapstring.c_str(), UaString(""));
            sTempMap = std::string(value.toString().toUtf8());
            separator1 = separator2 = 0;
            while (separator2 < int(sTempMap.length()) - 1)
            {
                separator2 = sTempMap.find(';', separator1);
                sSubMap = sTempMap.substr(separator1,separator2-separator1);
                subseparator = sSubMap.find(':');
                serial = atoi(sSubMap.substr(0,subseparator).c_str());
                position = atoi(sSubMap.substr(subseparator+1, std::string::npos).c_str());
                m_DevicePositionMap[it_typeNameMap.first][serial] = position;
                separator1 = separator2 + 1;
                printf("Added the following to %sPositionMap: serial=%d::position=%d\n", it_typeNameMap.second.c_str(), serial, position);
            }
        } // for it_typeNameMap

        pSettings->endGroup(); // Server i
    }
    pSettings->endGroup(); // Server

    std::map<OpcUa_UInt32, std::string> deviceNamesMap = {{PAS_MPESType, "SensorEdgeMap"},
        {PAS_ACTType, "ActuatorEdgeMap"}, {PAS_CCDType, "CCDMap"}};
    for (const auto& it_deviceNamesMap : deviceNamesMap) {
        std::string eAddress;
        value = pSettings->value(it_deviceNamesMap.second.c_str(), UaString(""));
        sTempMap = std::string(value.toString().toUtf8());
        separator1 = separator2 = 0;
        while (separator2 < int(sTempMap.length()) - 1)
        {
            separator2 = sTempMap.find(';', separator1);
            sSubMap = sTempMap.substr(separator1,separator2-separator1);
            subseparator = sSubMap.find(':');
            serial = atoi(sSubMap.substr(0,subseparator).c_str());

            // special handling of MPES and ACT
            if (it_deviceNamesMap.first == PAS_MPESType || it_deviceNamesMap.first == PAS_ACTType) {
                eAddress = sSubMap.substr(subseparator+1, std::string::npos);
                eAddress.replace(eAddress.find(':'), 1, "+");
            }
            else {
                eAddress = sSubMap.substr(subseparator+1, std::string::npos);
            }

            m_DeviceMap[it_deviceNamesMap.first][serial] = eAddress;
            separator1 = separator2 + 1;
            printf("Added the following to %s: serial=%d::address=%s\n", it_deviceNamesMap.second.c_str(), serial, eAddress.c_str());
        }
    }

    // Read NodeIds to use for reading
    m_nodesToRead.clear();
    pSettings->beginGroup("NodesToRead");
    value = pSettings->value("size", (OpcUa_UInt32)0);
    value.toUInt32(size);
    m_nodesToRead.resize(size);
    for (i=0; i<size;i++)
    {
        sTempKey = UaString("Variable%1").arg((int)i, 2, 10, UaChar('0'));
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        UaNodeId::fromXmlString(value.toString()).copyTo(&m_nodesToRead[i]);
    }
    pSettings->endGroup();  // NodesToRead

    // Read NodeIds, DataTypes and Values to use for writing
    m_nodesToWrite.clear();
    pSettings->beginGroup("NodesToWrite");
    value = pSettings->value("size", (OpcUa_UInt32)0);
    value.toUInt32(size);
    // NodeIds
    m_nodesToWrite.resize(size);
    for (i=0; i<size;i++)
    {
        sTempKey = UaString("Variable%1").arg((int)i, 2, 10, UaChar('0'));
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        UaNodeId::fromXmlString(value.toString()).copyTo(&m_nodesToWrite[i]);
    }
    // DataTypes
    UaByteArray writeDataTypes;
    writeDataTypes.resize(size);
    for (i=0; i<size;i++)
    {
        sTempKey = UaString("DataType%1").arg((int)i, 2, 10, UaChar('0'));
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        value.toByte(byte);
        writeDataTypes[(int)i] = byte;
    }
    // Values
    m_writeValues.resize(size);
    for (i=0; i<size;i++)
    {
        sTempKey = UaString("Value%1").arg((int)i, 2, 10, UaChar('0'));
        value = pSettings->value(sTempKey.toUtf16());
        // convert the value to the correct type
        OpcUa_BuiltInType type = (OpcUa_BuiltInType)(char)writeDataTypes[(int)i];
        if (OpcUa_IsGood(value.changeType(type, OpcUa_False)))
        {
            value.copyTo(&m_writeValues[i]);
        }
        else
        {
            printf("Cannot convert variant value: %s\n", value.toString().toUtf8());
        }
    }
    pSettings->endGroup();  // NodesToWrite

    // Read NodeIds to use for monitoring
    m_nodesToMonitor.clear();
    pSettings->beginGroup("NodesToMonitor");
    value = pSettings->value("size", (OpcUa_UInt32)0);
    value.toUInt32(size);
    m_nodesToMonitor.resize(size);
    for (i=0; i<size;i++)
    {
        sTempKey = UaString("Variable%1").arg((int)i, 2, 10, UaChar('0'));
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        UaNodeId::fromXmlString(value.toString()).copyTo(&m_nodesToMonitor[i]);
    }
    pSettings->endGroup();  // NodesToMonitor

    // Read Database connection info
    pSettings->beginGroup("Database");
    value = pSettings->value("size", (OpcUa_UInt32)0);
    value.toUInt32(size);
    m_databaseHost.resize(size);
    m_databaseUser.resize(size);
    m_databaseName.resize(size);
    m_databasePassword.resize(size);
    UaStringArray databaseEntries;
    for (i = 0; i < size; i++)
    {
        sTempKey = UaString("Host%1").arg((int)i, 2, 10, UaChar('0'));
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        value.toString().copyTo(&m_databaseHost[i]);

        sTempKey = UaString("User%1").arg((int)i, 2, 10, UaChar('0'));
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        value.toString().copyTo(&m_databaseUser[i]);

        sTempKey = UaString("Password%1").arg((int)i, 2, 10, UaChar('0'));
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        value.toString().copyTo(&m_databasePassword[i]);

        sTempKey = UaString("Database%1").arg((int)i, 2, 10, UaChar('0'));
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        value.toString().copyTo(&m_databaseName[i]);

        // Read Database entries info
        databaseEntries.clear();
        sTempKey = UaString("Entries%1").arg((int)i, 2, 10, UaChar('0'));
        pSettings->beginGroup(sTempKey.toUtf16());
        value = pSettings->value("size", (OpcUa_UInt32)0);
        value.toUInt32(size_inner);
        databaseEntries.resize(size_inner);
        m_databaseEntries.push_back(databaseEntries);
        for (j = 0; j < size_inner; j++)
        {
            sTempKey = UaString("Entry%1").arg((int)j, 2, 10, UaChar('0'));
            value = pSettings->value(sTempKey.toUtf16(), UaString(""));
            value.toString().copyTo(&m_databaseEntries.at(i)[j]);
        }
        pSettings->endGroup(); // Entries
    }
    pSettings->endGroup(); // Database
    */
}

UaStatus Configuration::loadDeviceConfiguration(const std::vector<std::string>& positionlist)
{
    // read device configuration from the database and load it into the internal maps
    /* *********************************************************/
    /* INITIAL DATABASE HACK JOB -- NEEDS TO HAVE ITS OWN CLASS */
    //string db_ip="172.17.10.10"; // internal ip
    DBConfig myconfig = DBConfig::getDefaultConfig();
    std::string db_ip=myconfig.getHost();
    std::string db_port=std::to_string(myconfig.getPort());
    std::string db_user=myconfig.getUser();
    std::string db_password=myconfig.getPassword();
    std::string db_name=myconfig.getDatabase();
    std::string db_address = "tcp://" + db_ip + ":" + db_port;

    try {
        sql::Driver *sql_driver = get_driver_instance();
        sql::Connection *sql_conn = sql_driver->connect(db_address, db_user, db_password);
        sql_conn->setSchema(db_name);
        sql::Statement *sql_stmt = sql_conn->createStatement();
        sql::ResultSet *sql_results;

        // get panel IP and serial from position
        for (const auto& position : positionlist) {
            Identity panelId;
            panelId.position = atoi(position.c_str());
            std::string query = "SELECT serial_number, mpcb_ip_address FROM Opt_MPMMapping WHERE end_date is NULL and position='" + position + "'";
            sql_stmt->execute(query);
            sql_results = sql_stmt->getResultSet();
            // should only be one result FOR NOW -- IN THE FUTURE, NEED TO FIX THIS, SORTING BY DATE
            while (sql_results->next()) {
#if SIMMODE
                const char* local_ip = getenv("LOCALIP");
                panelId.eAddress = "opc.tcp://"+std::string(local_ip)+":"+std::string(position.c_str());
#else
                panelId.eAddress = "opc.tcp://" + sql_results->getString(2) + ":4840";
#endif
                panelId.serialNumber = sql_results->getInt(1);
            }
            // add to the list of devices
            m_DeviceList[PAS_PanelType].push_back(panelId);
            std::cout << " +++ Configuration: added Panel "
                << panelId << " to Device List" << std::endl;

            // add to the position map {type -> {serial -> position}}
            m_DevicePositionMap[PAS_PanelType][panelId.serialNumber] = panelId.position;
            // add to the address map
            m_PanelAddressMap[panelId.position] = panelId.eAddress;

            // get the panel's actuators and add them to all the needed maps
            query = "SELECT serial_number, position, port FROM Opt_ActuatorMapping WHERE end_date is NULL and panel=" + std::to_string(panelId.position);
            sql_stmt->execute(query);
            sql_results = sql_stmt->getResultSet();
            while (sql_results->next()) {
                Identity actId;
                actId.serialNumber = sql_results->getInt(1);
                actId.position = sql_results->getInt(2);
                std::string port = std::to_string(sql_results->getInt(3));

                // add to the list of devices
                m_DeviceList[PAS_ACTType].push_back(actId);
                std::cout << " +++ Configuration: added Actuator "
                    << actId << " to Device List" << std::endl;

                // m_deviceSerialMap at (deviceType) at (panel address) at (port number)
                m_DeviceSerialMap[PAS_ACTType][UaString(panelId.eAddress.c_str())]
                                        [UaString(port.c_str())] = actId.serialNumber;

                // add to the position map {type -> {serial -> position}}
                m_DevicePositionMap[PAS_ACTType][actId.serialNumber] = actId.position;

                // add the actuator and this panel to the parents map
                m_ParentMap[PAS_ACTType][actId.serialNumber] = std::to_string(panelId.position);
                std::cout << " +++ Configuration: added parent " << panelId.position
                    << " of Actuator " << actId.serialNumber << std::endl;
            }

            // get the panel's mpes and add them to all the needed maps
            query = "SELECT serial_number, w_position, port, l_panel FROM Opt_MPESMapping WHERE end_date is NULL and w_panel=" + std::to_string(panelId.position);
            sql_stmt->execute(query);
            sql_results = sql_stmt->getResultSet();
            while (sql_results->next()) {
                Identity mpesId;
                mpesId.serialNumber = sql_results->getInt(1);
                mpesId.position = sql_results->getInt(2);
                std::string port = std::to_string(sql_results->getInt(3));
                std::string l_panelPos = std::to_string(sql_results->getInt(4));

                // add to the list of devices
                m_DeviceList[PAS_MPESType].push_back(mpesId);
                std::cout << " +++ Configuration: added MPES "
                    << mpesId << " to Device List" << std::endl;

                // m_deviceSerialMap at (deviceType) at (panel address) at (port number)
                m_DeviceSerialMap[PAS_MPESType][UaString(panelId.eAddress.c_str())]
                                        [UaString(port.c_str())] = mpesId.serialNumber;

                // add to the position map {type -> {serial -> position}}
                m_DevicePositionMap[PAS_MPESType][mpesId.serialNumber] = mpesId.position;

                // add the sensor and its panels to the parents map
                m_ParentMap[PAS_MPESType][mpesId.serialNumber] =
                    "w" + std::to_string(panelId.position) + "l" + l_panelPos;
                std::cout << " +++ Configuration: added parent "
                    << m_ParentMap.at(PAS_MPESType).at(mpesId.serialNumber)
                    << " of Sensor " << mpesId.serialNumber << std::endl;
            }
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
    /* END OF DATABASE HACK JOB */
    /* *********************************************************/

    return OpcUa_Good;
}


UaStatus Configuration::setupSecurity(SessionSecurityInfo& sessionSecurityInfo)
{
    UaStatus result;

    // create directories
    UaDir dirHelper("");
    UaUniString usClientCertificatePath(dirHelper.filePath(UaDir::fromNativeSeparators(m_clientCertificateFile.toUtf16())));
    dirHelper.mkpath(usClientCertificatePath);
    UaUniString usPrivateKeyPath(dirHelper.filePath(UaDir::fromNativeSeparators(m_clientPrivateKeyFile.toUtf16())));
    dirHelper.mkpath(usPrivateKeyPath);
    UaUniString usTrustListLocationPath(dirHelper.filePath(UaDir::fromNativeSeparators(m_certificateTrustListLocation.toUtf16())));
    dirHelper.mkpath(usTrustListLocationPath);
    UaUniString usRevocationListPath(dirHelper.filePath(UaDir::fromNativeSeparators(m_certificateRevocationListLocation.toUtf16())));
    dirHelper.mkpath(usRevocationListPath);

    // try to load the client certificate
    UaPkiCertificate clientCertificate = UaPkiCertificate::fromDERFile(m_clientCertificateFile);

    // certificate doesn't exists - we create a new one
    if (clientCertificate.isNull())
    {
        // Create the certificate
        UaPkiRsaKeyPair keyPair ( 1024 );
        UaPkiIdentity   identity;

        UaString sNodeName;
        char szHostName[256];
        if ( 0 == UA_GetHostname(szHostName, 256) )
        {
            sNodeName = szHostName;
        }

        identity.commonName       = UaString("Client_Cpp_SDK@%1").arg(sNodeName);
        identity.organization     = "Organization";
        identity.organizationUnit = "Unit";
        identity.locality         = "LocationName";
        identity.state            = "State";
        identity.country          = "DE";
        identity.domainComponent  = sNodeName;

        UaPkiCertificateInfo info;
        info.URI = UaString("urn:%1:%2:%3").arg(sNodeName).arg(COMPANY_NAME).arg(PRODUCT_NAME);
        info.DNSNames.create(1);
        sNodeName.copyTo(&info.DNSNames[0]);
        info.validTime = 3600*24*365*5; // 5 years in seconds

        // create a self signed certificate
        UaPkiCertificate cert ( info, identity, keyPair );

        // save public key to file
        cert.toDERFile ( m_clientCertificateFile.toUtf8() );

        // save private key to file
        keyPair.toPEMFile ( m_clientPrivateKeyFile.toUtf8(), 0 );
    }

    // initialize the PKI provider for using OpenSSL
    result = sessionSecurityInfo.initializePkiProviderOpenSSL(
        m_certificateRevocationListLocation,
        m_certificateTrustListLocation,
        m_issuersRevocationListLocation,
        m_issuersCertificatesLocation);
    if (result.isBad())
    {
        printf("*******************************************************\n");
        printf("** setupSecurity failed!\n");
        printf("** Could not initialize PKI\n");
        printf("*******************************************************\n");
        return result;
    }

    // load client certificate and private key
    result = sessionSecurityInfo.loadClientCertificateOpenSSL(
        m_clientCertificateFile,
        m_clientPrivateKeyFile);
    /*********************************************************************/
    if (result.isBad())
    {
        printf("*******************************************************\n");
        printf("** setupSecurity failed!\n");
        printf("** Could not load Client certificate\n");
        printf("** Connect will work only without security\n");
        printf("*******************************************************\n");
        return result;
    }

    return result;
}

UaStatus Configuration::updateNamespaceIndexes(const UaStringArray& namespaceArray)
{
    UaStatus result;
    OpcUa_UInt32 i, j;
    OpcUa_UInt32 size;

    // create mapping table
    size = m_namespaceArray.length();
    UaInt16Array mappingTable;
    mappingTable.resize(size);

    // fill mapping table
    for (i = 0; i < m_namespaceArray.length(); i++)
    {
        mappingTable[i] = (OpcUa_UInt16)i;
        // find namespace uri
        for (j = 0; j < namespaceArray.length(); j++)
        {
            UaString string1(m_namespaceArray[i]);
            UaString string2(namespaceArray[j]);
            if (string1 == string2)
            {
                mappingTable[i] = (OpcUa_UInt16)j;
                break;
            }
        }
    }

    // replace namespace index in NodeIds
    // NodesToRead
    for (i = 0; i < m_nodesToRead.length(); i++)
    {
        m_nodesToRead[i].NamespaceIndex = mappingTable[m_nodesToRead[i].NamespaceIndex];
    }
    // NodeToWrite
    for (i = 0; i < m_nodesToWrite.length(); i++)
    {
        m_nodesToWrite[i].NamespaceIndex = mappingTable[m_nodesToWrite[i].NamespaceIndex];
    }
    // NodesToMonitor
    for (i = 0; i < m_nodesToMonitor.length(); i++)
    {
        m_nodesToMonitor[i].NamespaceIndex = mappingTable[m_nodesToMonitor[i].NamespaceIndex];
    }

    // update namespace array
    m_namespaceArray = namespaceArray;
    return result;
}


OpcUa_Int32 Configuration::getDeviceSerial(const UaString& address, OpcUa_UInt32 deviceType, const UaString& sPort)
{
    OpcUa_Int32 serial;
    try
    {
        serial = m_DeviceSerialMap.at(deviceType).at(address).at(sPort);
    }
    catch (std::out_of_range)
    {
        serial = -1;
    }

    return serial;
}

OpcUa_Int32 Configuration::getDevicePosition(OpcUa_UInt32 deviceType, OpcUa_UInt32 serial)
{
    int pos;
    try
    {
        pos = m_DevicePositionMap.at(deviceType).at(serial);
    }
    catch (std::out_of_range)
    {
        pos = -1;
    }

    return pos;
}

// need to pass around the whole object!
// can't return this by reference, since we're creating the objects right here
std::vector< std::pair<OpcUa_UInt32, Identity> > Configuration::getParents(OpcUa_UInt32 deviceType, const Identity& id)
{
    // for actuators, the parent is the ACT's panel. ACT's parent's address in the map
    // is the panel position. so pretty straighforward.
    if (deviceType == PAS_ACTType) {
        Identity parentId;
        parentId.position = std::stoi(m_ParentMap.at(deviceType).at(id.serialNumber));
        // need to fill in the address of the parent as well!
        parentId.eAddress = m_PanelAddressMap.at(parentId.position);
        return {{PAS_PanelType, parentId}};
    }
    // for the MPES, the parent is the MPES's w-panel and the edge.
    // the edge is the alphabetic concatenation of the panels, "panel1+panel2[+panel3]"
    // the MPES's parent address in the map is "w"+"panel1"+"l"+"panel2",
    // so also pretty straighforward
    else if (deviceType == PAS_MPESType) {
        unsigned w_pos, l_pos;
        Identity w_panelId, edgeId;
        std::string parentAddress = m_ParentMap.at(deviceType).at(id.serialNumber);

        // w panel should be first in the address, but let's be safe in case someone forgets
        // and changes that convention;
        // assume that both of the id's are present though -- the program will throw an exception
        // if that's not the case
        auto w_idx = parentAddress.find("w");
        auto l_idx = parentAddress.find("l");
        if (w_idx < l_idx) { // w first
            w_pos = stoi(parentAddress.substr(w_idx + 1, l_idx - w_idx));
            l_pos = stoi(parentAddress.substr(l_idx + 1));
        }
        else { // l first
            l_pos = stoi(parentAddress.substr(l_idx + 1, w_idx - l_idx));
            w_pos = stoi(parentAddress.substr(w_idx + 1));
        }

        w_panelId.position = w_pos;
        w_panelId.eAddress = m_PanelAddressMap.at(w_pos);

        // return the edge only if the l_panel is actually present; otherwise, only the w_panel
        try {
            std::string l_panel_address = m_PanelAddressMap.at(l_pos);
        }
        catch (std::out_of_range) {
            return {{PAS_PanelType, w_panelId}};
        }

        // special treatment of the P1-P2/S1-S2 edge, as this edge is defined by 3 panels.
        // this edge is characterized by the fact that the last two digits of the outer ring
        // position  are exactly 2*<inner> or 2*<inner> - 1, where <inner> are the last two
        // digits of the inner ring position
        unsigned third_pos = 0;
        unsigned larger = std::max(w_pos, l_pos);
        unsigned smaller = std::min(w_pos, l_pos);
        if ( (larger % 100) == 2*(smaller % 100) || (larger % 100) == 2*(smaller % 100) - 1) {
            // get the third panel -- it's the neighbor of the panel with the larger position:
            // if this value is even, the third panel is one less;
            // if this value is odd, the third panel is one more:
            third_pos = larger;
            third_pos += (larger % 2) ? 1 : -1;

            // check that this panel is actually present
            try {
                std::string third_panel_address = m_PanelAddressMap.at(third_pos);
            }
            catch (std::out_of_range) {
                third_pos = 0;
            }
        }

        // find the mirror this sensor is on
        Identity mirrorId;
        mirrorId.position = SCTMath::Mirror(w_pos);
        // the mirror has no Address or serial, so we set these to be equal to the position
        mirrorId.serialNumber = mirrorId.position;
        mirrorId.eAddress = std::to_string(mirrorId.position);

        std::vector<unsigned> panel_positions {w_pos, l_pos};
        if (third_pos) panel_positions.push_back(third_pos);
        edgeId.eAddress = SCTMath::GetEdgeFromPanels(panel_positions);

        return {{PAS_PanelType, w_panelId}, {PAS_EdgeType, edgeId}, {PAS_MirrorType, mirrorId}};
    }
    // For a panel, the parent is the mirror it's on
    else if (deviceType == PAS_PanelType) {
        Identity mirrorId;
        // the position is the first digit of the panel's position
        mirrorId.position = SCTMath::Mirror(id.position);
        // the mirror has no Address or serial, so we set these to be equal to the position
        mirrorId.serialNumber = mirrorId.position;
        mirrorId.eAddress = std::to_string(mirrorId.position);

        return {{PAS_MirrorType, mirrorId}};
    }
    // For an edge, the parent is the mirror it's on
    else if (deviceType == PAS_EdgeType) {
        Identity mirrorId;
        // the position is the first digit of the position of either of the panels of the edge;
        // the mirror has no Address or serial, so we set these to be equal to the position
        mirrorId.position = SCTMath::Mirror(SCTMath::GetPanelsFromEdge(id.eAddress, 1).at(0));
        mirrorId.serialNumber = mirrorId.position;
        mirrorId.eAddress = std::to_string(mirrorId.position);

        return {{PAS_MirrorType, mirrorId}};
    }

    return {{}};
}
