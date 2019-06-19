#include "configuration.hpp"
#include "common/simulatestewart/mathtools.hpp"
#include "uaclient/uaclientsdk.h"
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


Configuration::Configuration() : m_bAutomaticReconnect(OpcUa_True), m_bRetryInitialConnect(OpcUa_True)
{
}

OpcUa_UInt32 Configuration::getServers() const
{
    try {
        return m_DeviceIdentities.at(PAS_PanelType).size();
    }
    catch (std::out_of_range &e)
    {
        return 0;
    }
}

UaStatus Configuration::loadConnectionConfiguration(const UaString& sConfigurationFile)
{
    UaStatus result;
    UaVariant value;

    UaSettings *pSettings = new UaSettings(sConfigurationFile.toUtf16());

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
    const char *local_ip = getenv("LOCALIP");
    value = pSettings->value("DiscoveryURL", UaString("opc.tcp://%s:48010").arg(local_ip));
#else
    value = pSettings->value("DiscoveryURL", UaString("opc.tcp://172.17.0.201:48010"));
#endif
    m_discoveryUrl = value.toString();
    value = pSettings->value("PositionerURL", UaString("opc.tcp://127.0.0.1:4840"));
    m_positionerUrl = value.toString();

    // Read NamespaceArray
    OpcUa_UInt32 size = 0;
    m_namespaceArray.clear();
    pSettings->beginGroup("NSArray");
    value = pSettings->value("size", (OpcUa_UInt32)0);
    value.toUInt32(size);
    m_namespaceArray.resize(size);

    UaString sTempKey;
    for (OpcUa_UInt32 i = 0; i < size; i++)
    {
        sTempKey = UaString("NameSpaceUri%1").arg((int)i, 2, 10, UaChar('0'));
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        value.toString().copyTo(&m_namespaceArray[i]);
    }
    pSettings->endGroup();  // NSArray

    pSettings->endGroup(); // UaClientConfig

    delete pSettings;

    return result;
}

UaStatus Configuration::loadDeviceConfiguration(const std::vector<std::string> &positionList)
{
    // read device configuration from the database and load it into the internal maps
    DBConfig myConfig = DBConfig::getDefaultConfig();
    std::string db_ip = myConfig.getHost();
    std::string db_port = myConfig.getPort();
    std::string db_user = myConfig.getUser();
    std::string db_password = myConfig.getPassword();
    std::string db_name = myConfig.getDatabase();
    std::string db_address = "tcp://" + db_ip + ":" + db_port;

    m_DeviceIdentities[PAS_PanelType] = {}; // initialize the list of panels to an empty one -- other devices don't need this

    try {
        sql::Driver *sql_driver = get_driver_instance();
        sql::Connection *sql_conn = sql_driver->connect(db_address, db_user, db_password);
        sql_conn->setSchema(db_name);
        sql::Statement *sql_stmt = sql_conn->createStatement();
        sql::ResultSet *sql_results;
        std::string query;

        // get panel IP and serial from position
        for (const auto &position : positionList) {
            Device::Identity panelId;
            panelId.position = std::stoi(position);

            query = "SELECT serial_number, mpcb_ip_address FROM Opt_MPMMapping WHERE position='" + position + "'";
            sql_stmt->execute(query);
            sql_results = sql_stmt->getResultSet();
            // should only be one result FOR NOW -- IN THE FUTURE, NEED TO FIX THIS, SORTING BY DATE
            while (sql_results->next()) {
#if SIMMODE
                const char* local_ip = getenv("LOCALIP");
                panelId.eAddress = "opc.tcp://"+std::string(local_ip)+":" + position;
#else
                panelId.eAddress = "opc.tcp://" + sql_results->getString(2) + ":4840";
#endif
                panelId.serialNumber = sql_results->getInt(1);
                panelId.name = std::string("Panel_") + std::to_string(panelId.position);

                // add to the list of devices
                m_DeviceIdentities[PAS_PanelType].insert(panelId);
                std::cout << "    Configuration::loadDeviceConfiguration(): added Panel " << panelId << " to Device List" << std::endl;

                // add to other maps
                m_DeviceSerialMap[PAS_PanelType][panelId.serialNumber] = panelId;
                m_PanelPositionMap[panelId.position] = panelId;
                m_PanelAddressMap[panelId.eAddress] = panelId;
            }

            // get the panel's actuators and add them to all the needed maps
            query = "SELECT serial_number, position, port FROM Opt_ActuatorMapping WHERE end_date is NULL and panel=" + std::to_string(panelId.position);
            sql_stmt->execute(query);
            sql_results = sql_stmt->getResultSet();
            while (sql_results->next()) {
                Device::Identity actId;
                actId.serialNumber = sql_results->getInt(1);
                actId.position = sql_results->getInt(2);
                std::string port = std::to_string(sql_results->getInt(3));
                actId.eAddress = port;
                actId.name = std::string("ACT_") + std::to_string(actId.serialNumber);

                m_DeviceIdentities[PAS_ACTType].insert(actId);
                std::cout << "    Configuration::loadDeviceConfiguration(): added Actuator " << actId << " to Device List" << std::endl;

                m_DeviceSerialMap[PAS_ACTType][actId.serialNumber] = actId;

                // add the actuator and this panel to the parents map
                m_ChildMap[panelId][PAS_ACTType].insert(actId);
                m_ParentMap[actId][PAS_PanelType].insert(panelId);
                std::cout << "    Configuration::loaDeviceConfiguration(): added Panel " << panelId.position << " as parent of Actuator "
                          << actId.serialNumber << std::endl;
            }

            // get the panel's mpes and add them to all the needed maps
            query = "SELECT serial_number, w_position, port, l_panel FROM Opt_MPESMapping WHERE end_date is NULL and w_panel=" + std::to_string(panelId.position);
            sql_stmt->execute(query);
            sql_results = sql_stmt->getResultSet();
            while (sql_results->next()) {
                Device::Identity mpesId;
                mpesId.serialNumber = sql_results->getInt(1);
                mpesId.position = sql_results->getInt(2);
                mpesId.eAddress = std::to_string(sql_results->getInt(3));
                mpesId.name = std::string("MPES_") + std::to_string(mpesId.serialNumber);
                std::string port = std::to_string(sql_results->getInt(3));
                std::string l_panelPos = std::to_string(sql_results->getInt(4));

                // add to the list of devices
                m_DeviceIdentities[PAS_MPESType].insert(mpesId);
                std::cout << "    Configuration::loadDeviceConfiguration(): added MPES " << mpesId << " to Device List" << std::endl;

                m_DeviceSerialMap[PAS_MPESType][mpesId.serialNumber] = mpesId;

                // add the sensor and its panels to the parents map
                m_ChildMap[panelId][PAS_MPESType].insert(mpesId);
                m_ParentMap[mpesId][PAS_PanelType].insert(panelId);
                m_MPES_SideMap[mpesId]["w"] = panelId;
                std::cout << "    Configuration::loadDeviceConfiguration(): added Panel " << panelId
                          << " as w parent of MPES " << mpesId << std::endl;
            }
        }
        //Get laser-side panel for all MPES
        for (const auto &mpesId : m_DeviceIdentities.at(PAS_MPESType)) {
            query = "SELECT l_panel FROM Opt_MPESMapping WHERE end_date is NULL and serial_number=" + std::to_string(mpesId.serialNumber);
            sql_stmt->execute(query);
            sql_results = sql_stmt->getResultSet();
            while (sql_results->next()) {
                int lPanelPosition = sql_results->getInt(1);

                // get corresponding laser-side panel if it exists
                if (m_PanelPositionMap.find(lPanelPosition) != m_PanelPositionMap.end()) {
                    Device::Identity lPanelId = m_PanelPositionMap.at(lPanelPosition);

                    m_MPES_SideMap[mpesId]["l"] = lPanelId;
                    // Could optionally add laser-side panel as a parent here.
                }
                else {
                    continue;
                }
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

    std::cout << "Configuration::loadDeviceConfiguration(): Adding missing parents..." << std::endl;
    bool result = addMissingParents();
    if (!result) {
        return OpcUa_Bad;
    }

    return OpcUa_Good;
}


UaStatus Configuration::setupSecurity(UaClientSdk::SessionSecurityInfo &sessionSecurityInfo)
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
        keyPair.toPEMFile(m_clientPrivateKeyFile.toUtf8(), nullptr);
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

bool Configuration::addMissingParents() {
    for (const auto &panelId : m_DeviceIdentities.at(PAS_PanelType)) {
        // Add mirror as parent to all panels
        Device::Identity mirrorId = getMirrorId(panelId.position);
        if (m_DeviceIdentities[PAS_MirrorType].find(mirrorId) == m_DeviceIdentities[PAS_MirrorType].end()) {
            std::cout << "Configuration::addMissingParents(): Added Mirror " << mirrorId << " to Device List" << std::endl;
            m_DeviceIdentities[PAS_MirrorType].insert(mirrorId);
        }
        m_ChildMap[mirrorId][PAS_PanelType].insert(panelId);
        m_ParentMap[panelId][PAS_MirrorType].insert(mirrorId);
    }
    for (const auto &mpesId : m_DeviceIdentities.at(PAS_MPESType)) {
        if (m_ParentMap.at(mpesId).at(PAS_PanelType).size() != 1) {
            std::cout << " ERROR:: Configuration::createMissingParents(): MPES " << mpesId << " has "
                      << m_ParentMap.at(mpesId).at(PAS_PanelType).size()
                      << " parent panels (should only have 1). Aborting...\n";
            return false;
        }
        Device::Identity w_panelId = *m_ParentMap.at(mpesId).at(PAS_PanelType).begin();

        // add the edge as a parent only if the l_panel (and possibly third panel) are actually present
        if (m_MPES_SideMap.at(mpesId).find("l") != m_MPES_SideMap.at(mpesId).end()) {
            Device::Identity l_panelId = m_MPES_SideMap.at(mpesId).at("l");
            Device::Identity edgeId;
            std::vector<int> panelPositions{w_panelId.position, l_panelId.position};
            int thirdPanelPosition = getThirdPanelPosition(w_panelId.position, l_panelId.position);

            if (thirdPanelPosition > 0 &&
                m_PanelPositionMap.find(thirdPanelPosition) != m_PanelPositionMap.end()) {
                panelPositions.push_back(thirdPanelPosition);
            }
            edgeId.eAddress = SCTMath::GetEdgeFromPanels(panelPositions);
            edgeId.name = std::string("Edge_") + edgeId.eAddress;

            if (m_DeviceIdentities[PAS_EdgeType].find(edgeId) == m_DeviceIdentities[PAS_EdgeType].end()) {
                std::cout << "Configuration::addMissingParents(): Added Edge " << edgeId << " to Device List" << std::endl;
                m_DeviceIdentities[PAS_EdgeType].insert(edgeId);
            }
            m_ChildMap[edgeId][PAS_MPESType].insert(mpesId);
            m_ParentMap[mpesId][PAS_EdgeType].insert(edgeId);
        }

        // Add mirror as parent to all MPES
        Device::Identity mirrorId = getMirrorId(w_panelId.position);
        if (m_DeviceIdentities[PAS_MirrorType].find(mirrorId) == m_DeviceIdentities[PAS_MirrorType].end()) {
            std::cout << "Configuration::addMissingParents(): Added Mirror " << mirrorId << " to Device List" << std::endl;
            m_DeviceIdentities[PAS_MirrorType].insert(mirrorId);
        }
        m_ChildMap[mirrorId][PAS_MPESType].insert(mpesId);
        m_ParentMap[mpesId][PAS_MirrorType].insert(mirrorId);
    }
    if (m_DeviceIdentities.find(PAS_EdgeType) != m_DeviceIdentities.end()) {
        for (const auto &edgeId : m_DeviceIdentities.at(PAS_EdgeType)) {
            // Add mirror as parent to all edges
            Device::Identity mirrorId = getMirrorId(SCTMath::GetPanelsFromEdge(edgeId.eAddress, 1).at(0));
            if (m_DeviceIdentities[PAS_MirrorType].find(mirrorId) == m_DeviceIdentities[PAS_MirrorType].end()) {
                std::cout << "Configuration::addMissingParents(): Added Mirror " << mirrorId << " to Device List" << std::endl;
                m_DeviceIdentities[PAS_MirrorType].insert(mirrorId);
            }
            m_ChildMap[mirrorId][PAS_EdgeType].insert(edgeId);
            m_ParentMap[edgeId][PAS_MirrorType].insert(mirrorId);
        }
    }
    for (const auto &mpesId : m_DeviceIdentities.at(PAS_MPESType)) {
        if (m_ParentMap.at(mpesId).find(PAS_EdgeType) != m_ParentMap.at(mpesId).end()) {
            for (const auto &edgeParentId : m_ParentMap.at(mpesId).at(PAS_EdgeType)) {
                //add the parent edges and panels as each others' parents
                for (const auto &panelParentId : m_ParentMap.at(mpesId).at(PAS_PanelType)) {
                    m_ChildMap[edgeParentId][PAS_PanelType].insert(panelParentId);
                    m_ParentMap[panelParentId][PAS_EdgeType].insert(edgeParentId);
                    m_ChildMap[panelParentId][PAS_EdgeType].insert(edgeParentId);
                    m_ParentMap[edgeParentId][PAS_PanelType].insert(panelParentId);
                }
            }
        }
    }
}

Device::Identity Configuration::getMirrorId(int mirrorNum) {
    Device::Identity mirrorId;
    mirrorId.position = SCTMath::Mirror(mirrorNum);
    mirrorId.serialNumber = mirrorId.position;
    mirrorId.eAddress = std::to_string(mirrorId.position);

    if (mirrorId.position == 1) {
        mirrorId.name = "PrimaryMirror";
    } else if (mirrorId.position == 2) {
        mirrorId.name = "SecondaryMirror";
    } else if (mirrorId.position == 3) {
        mirrorId.name = "TestMirror";
    } else {
        mirrorId.name = "UnknownMirror";
    }

    return mirrorId;
}

int Configuration::getThirdPanelPosition(int wPanelPosition, int lPanelPosition) {
    // special treatment of the P1-P2/S1-S2 edge, as this edge is defined by 3 panels.
    // this edge is characterized by the fact that the last two digits of the outer ring
    // position  are exactly 2*<inner> or 2*<inner> - 1, where <inner> are the last two
    // digits of the inner ring position
    int third_pos = 0;
    int larger = std::max(wPanelPosition, lPanelPosition);
    int smaller = std::min(wPanelPosition, lPanelPosition);
    if ((larger % 100) == 2 * (smaller % 100) || (larger % 100) == 2 * (smaller % 100) - 1) {
        // get the third panel -- it's the neighbor of the panel with the larger position:
        // if this value is even, the third panel is one less;
        // if this value is odd, the third panel is one more:
        third_pos = larger;
        third_pos += (larger % 2) ? 1 : -1;

        return third_pos;
    } else {
        return -1;
    }
}
