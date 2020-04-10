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

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

const std::map<std::string, std::string> Configuration::SUBCLIENTS = {
    {"primary_upper", "opc.tcp://127.0.0.1:48011"},
    {"primary_lower", "opc.tcp://127.0.0.1:48012"},
    {"secondary",     "opc.tcp://127.0.0.1:48013"},
    {"other",         "opc.tcp://127.0.0.1:48014"}
};

const std::map<int, std::tuple<std::string, std::string>> Configuration::CCDs = {
        {21, std::make_tuple("T01S1","/home/ctauser/repos/bryan/pSCT-alignment/common/globalalignment/ccd/50mm-nosnT01S1.dat")},
        {22, std::make_tuple("T01S2","/home/ctauser/repos/bryan/pSCT-alignment/common/globalalignment/ccd/50mm-nosnT01S2.dat")},
        {23, std::make_tuple("T01S3","/home/ctauser/repos/bryan/pSCT-alignment/common/globalalignment/ccd/50mm-nosnT01S3.dat")},
        {11, std::make_tuple("T01P1","/home/ctauser/repos/bryan/pSCT-alignment/common/globalalignment/ccd/50mm-nosnT01P1.dat")},
        {12, std::make_tuple("T01P2","/home/ctauser/repos/bryan/pSCT-alignment/common/globalalignment/ccd/50mm-nosnT01P2.dat")},
        {13, std::make_tuple("T01P3","/home/ctauser/repos/bryan/pSCT-alignment/common/globalalignment/ccd/50mm-nosnT01P3.dat")}
};


Configuration::Configuration(std::string mode) : m_Mode(std::move(mode)), m_bAutomaticReconnect(OpcUa_True),
                                                 m_bRetryInitialConnect(OpcUa_True)
{
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
    value = pSettings->value("PositionerURL", UaString("opc.tcp://172.17.3.3:4840"));
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

        std::vector<std::string> positions;

        // If empty list of panels passed, assume all panels.
        if (positionList.empty()) {
            query = "SELECT position FROM Opt_MPMMapping";
            sql_stmt->execute(query);
            sql_results = sql_stmt->getResultSet();
            // should only be one result FOR NOW -- IN THE FUTURE, NEED TO FIX THIS, SORTING BY DATE
            while (sql_results->next()) {
                positions.push_back(sql_results->getString(1));
            }
        } else {
            positions = positionList;
        }

        // get panel IP and serial from position
        for (const auto &position : positions) {
            Device::Identity panelId;
            panelId.position = std::stoi(position);

            query = "SELECT serial_number, mpcb_ip_address FROM Opt_MPMMapping WHERE end_date is NULL and position='" +
                    position + "'";
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
                spdlog::info("Configuration::loadDeviceConfiguration(): added Panel {} to device list.", panelId);

                // add to other maps
                m_DeviceSerialMap[PAS_PanelType][panelId.serialNumber] = panelId;
                m_PanelPositionMap[panelId.position] = panelId;
                m_DeviceNameMap[panelId.name] = panelId;
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
                spdlog::info("Configuration::loadDeviceConfiguration(): added Actuator {} to device list.", actId);

                m_DeviceSerialMap[PAS_ACTType][actId.serialNumber] = actId;
                m_DeviceNameMap[actId.name] = actId;

                // add the actuator and this panel to the parents map
                m_ChildMap[panelId][PAS_ACTType].insert(actId);
                m_ParentMap[actId][PAS_PanelType].insert(panelId);
                spdlog::info("Configuration::loaDeviceConfiguration(): added Panel {} as parent of Actuator {}.",
                             panelId.position, actId.serialNumber);
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
                spdlog::info("Configuration::loadDeviceConfiguration(): added MPES {} to device list.", mpesId);

                m_DeviceSerialMap[PAS_MPESType][mpesId.serialNumber] = mpesId;
                m_DeviceNameMap[mpesId.name] = mpesId;

                // add the sensor and its panels to the parents map
                m_ChildMap[panelId][PAS_MPESType].insert(mpesId);
                m_ParentMap[mpesId][PAS_PanelType].insert(panelId);
                m_MPES_SideMap[mpesId]["w"] = panelId;
                spdlog::info("Configuration::loadDeviceConfiguration(): added Panel {} as w parent of MPES {}.",
                             panelId, mpesId);
            }

            // get OT's PSD
            if (panelId.position==1001 || panelId.position==2001)
            {
                Device::Identity psdId;
                psdId.serialNumber = panelId.position; // need to find it out!!!
                psdId.position = 0;
                psdId.eAddress = "10.0.1.100"; // need to find it out!!!
                psdId.name = std::string("PSD_") + std::to_string(psdId.serialNumber);

                // add to the list of devices
                m_DeviceIdentities[PAS_PSDType].insert(psdId);
                spdlog::info("Configuration::loadDeviceConfiguration(): added PSD {} to device list.", psdId);

                m_DeviceSerialMap[PAS_PSDType][psdId.serialNumber] = psdId;
                m_DeviceNameMap[psdId.name] = psdId;

                // add the PSD and its panels to the parents map
                m_ChildMap[panelId][PAS_PSDType].insert(psdId);
                m_ParentMap[psdId][PAS_PanelType].insert(panelId);
                spdlog::info("Configuration::loadDeviceConfiguration(): added Panel {} as parent of PSD {}.",
                             panelId, psdId);
            }
        }
        try {
            spdlog::debug("Found {} MPES", m_DeviceIdentities.at(PAS_MPESType).size());
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
        catch (const std::out_of_range& oor) {
            spdlog::warn("No MPES found: [Out of Range error: {}]", oor.what() );
        }


        // Get CCDs for GAS
        Device::Identity CCDId;
        for (auto v : CCDs)
        {
            std::cout << v.first << " :: " << std::get<0>(v.second) << std::endl;
            CCDId.serialNumber = v.first;
            CCDId.position = v.first;
            std::string port = std::to_string(v.first);
            CCDId.eAddress = std::get<1>(v.second);
            CCDId.name = std::string("CCD_") + std::get<0>(v.second);

            m_DeviceIdentities[PAS_CCDType].insert(CCDId);
            spdlog::info("Configuration::loadDeviceConfiguration(): added CCD {} to device list.", CCDId);

            m_DeviceSerialMap[PAS_CCDType][CCDId.serialNumber] = CCDId;
            m_DeviceNameMap[CCDId.name] = CCDId;
        }

    }
    catch (sql::SQLException &e) {
        spdlog::error("# ERR: SQLException in {}"
                      "({}) on line {}\n"
                      "# ERR: {}"
                      " (MySQL error code: {}"
                      ", SQLState: {})", __FILE__, __FUNCTION__, __LINE__, e.what(), e.getErrorCode(), e.getSQLState());
        return OpcUa_Bad;
    }

    spdlog::info("Configuration::loadDeviceConfiguration(): Adding missing parents...");
    bool result = addMissingParents();
    if (!result) {
        return OpcUa_Bad;
    }

    return OpcUa_Good;
}

UaStatus Configuration::loadSubclientConfiguration(const std::vector<std::string> &subclientList) {
    for (const auto &subclientName : subclientList) {
        if (SUBCLIENTS.find(subclientName) != SUBCLIENTS.end()) {
            spdlog::info("Added subclient with name {}.", subclientName);
            m_Subclients[subclientName] = SUBCLIENTS.at(subclientName);
        } else {
            spdlog::error("Subclient with name {} not found. Skipping...", subclientName);
        }
    }

    return OpcUa_Good;
}

std::set<Device::Identity> Configuration::getDevices(OpcUa_UInt32 deviceType) {
    if (m_DeviceIdentities.find(deviceType) != m_DeviceIdentities.end()) {
        return m_DeviceIdentities.at(deviceType);
    } else {
        return std::set<Device::Identity>();
    }
}

std::vector<UaString> Configuration::getServerAddresses() {
    std::vector<UaString> serverAddresses;
    if (m_Mode == "subclient") {
        for (const auto &panelId : getDevices(PAS_PanelType)) {
            serverAddresses.emplace_back(UaString(panelId.eAddress.c_str()));
        }
    } else if (m_Mode == "client") {
        for (const auto &subclient : m_Subclients) {
            serverAddresses.emplace_back(UaString(subclient.second.c_str()));
        }
    }

    return serverAddresses;
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
            spdlog::info("Configuration::addMissingParents(): Added Mirror {} to device list.", mirrorId);
            m_DeviceIdentities[PAS_MirrorType].insert(mirrorId);
        }
        m_ChildMap[mirrorId][PAS_PanelType].insert(panelId);
        m_ParentMap[panelId][PAS_MirrorType].insert(mirrorId);
        if (panelId.position==1001 || panelId.position==2001){
            try {
                for (const auto &psdId : m_DeviceIdentities.at(PAS_PSDType)) {
                    if (psdId.serialNumber == panelId.position) {
                        spdlog::trace("Found that {} associated with {}", panelId, psdId);
                        m_ChildMap[panelId][PAS_PSDType].insert(psdId);
                        m_ParentMap[psdId][PAS_PanelType].insert(panelId);
                    } else {
                    }
                }
            }
            catch (const std::out_of_range& oor) {
                spdlog::warn("No PSD found");
            }
        }
    }
    try {
        for (const auto &mpesId : m_DeviceIdentities.at(PAS_MPESType)) {
            if (m_ParentMap.at(mpesId).at(PAS_PanelType).size() != 1) {
                spdlog::error(
                    "Configuration::createMissingParents(): MPES {} has {} parent panels (should only have 1). Aborting...",
                    mpesId, m_ParentMap.at(mpesId).at(PAS_PanelType).size());
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
                    spdlog::info("Configuration::addMissingParents(): Added Edge {} to device list.", edgeId);
                    m_DeviceIdentities[PAS_EdgeType].insert(edgeId);
                }
                m_ChildMap[edgeId][PAS_MPESType].insert(mpesId);
                m_ParentMap[mpesId][PAS_EdgeType].insert(edgeId);
            }

            // Add mirror as parent to all MPES
            Device::Identity mirrorId = getMirrorId(w_panelId.position);
            if (m_DeviceIdentities[PAS_MirrorType].find(mirrorId) == m_DeviceIdentities[PAS_MirrorType].end()) {
                spdlog::info("Configuration::addMissingParents(): Added Mirror {} to device list.", mirrorId);
                m_DeviceIdentities[PAS_MirrorType].insert(mirrorId);
            }
            m_ChildMap[mirrorId][PAS_MPESType].insert(mpesId);
            m_ParentMap[mpesId][PAS_MirrorType].insert(mirrorId);
        }
    }
    catch (const std::out_of_range& oor) {
            spdlog::warn("Out of Range error: ({}), No MPES found on this panel.",oor.what());
        }
    if (m_DeviceIdentities.find(PAS_EdgeType) != m_DeviceIdentities.end()) {
        for (const auto &edgeId : m_DeviceIdentities.at(PAS_EdgeType)) {
            // Add mirror as parent to all edges
            Device::Identity mirrorId = getMirrorId(SCTMath::GetPanelsFromEdge(edgeId.eAddress, 1).at(0));
            if (m_DeviceIdentities[PAS_MirrorType].find(mirrorId) == m_DeviceIdentities[PAS_MirrorType].end()) {
                spdlog::info("Configuration::addMissingParents(): Added Mirror {} to device list.", mirrorId);
                m_DeviceIdentities[PAS_MirrorType].insert(mirrorId);
            }
            m_ChildMap[mirrorId][PAS_EdgeType].insert(edgeId);
            m_ParentMap[edgeId][PAS_MirrorType].insert(mirrorId);
        }
    }
    try {
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
    catch (const std::out_of_range& oor) {
        spdlog::warn("Out of Range error: ({}), No MPES found on this panel.",oor.what());
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
