#include "clientconfig.h"
#include "uadir.h"

ClientSampleConfig::ClientSampleConfig()
{
#if OPCUA_SUPPORT_PKI
    m_useWindowsStore = OpcUa_False;
#if OPCUA_SUPPORT_PKI_WIN32
    m_windowsStoreLocation = Location_LocalMachine;
#endif // OPCUA_SUPPORT_PKI_WIN32
#endif // OPCUA_SUPPORT_PKI

    char szHostName[256];
    if ( 0 == UA_GetHostname(szHostName, 256) )
    {
        m_sHostName = szHostName;
    }

    m_sApplicationName = UaString("Client_Cpp_SDK@%1").arg(m_sHostName);
    m_sApplicationUri  = UaString("urn:%1:UnifiedAutomation::Client_Cpp_SDK").arg(m_sHostName);
    m_sProductUri      = "urn:UnifiedAutomation::Client_Cpp_SDK";
}

ClientSampleConfig::~ClientSampleConfig()
{
}

void ClientSampleConfig::loadConfiguration(
    const UaString& sIniFileName,
    const UaString& sAppPath)
{
    m_sConfigFile = sIniFileName;

    UaVariant     value;
    UaString      sTempKey;
    UaString      sTempValue;
    int           i         = 0;
    UaSettings*   pSettings = 0;
    pSettings = new UaSettings(sIniFileName.toUtf16());

    pSettings->beginGroup("UaClientConfig");

    // Server URLs
    value = pSettings->value("DiscoveryURL", UaString("opc.tcp://localhost:4840"));
    m_sDiscoveryUrl = value.toString();
    value = pSettings->value("ServerUrl", UaString("opc.tcp://localhost:48010"));
    m_sDefaultServerUrl = value.toString();

    // GDS
    value = pSettings->value("GdsServerUrl", UaString(""));
    m_sGdsServerUrl = value.toString();
    value = pSettings->value("GdsUser", UaString());
    m_sGdsUser = value.toString();
    value = pSettings->value("GdsPassword", UaString());
    m_sGdsPassword = value.toString();
    value = pSettings->value("GdsApplicationObjectId", UaString(""));
    m_sGdsApplicationObjectId = value.toString();

    // OpenSSL PKI store
    value = pSettings->value("CertificateTrustListLocation", UaString("pki/trusted/certs/"));
    m_sCertificateTrustListLocation = value.toString();
    replaceApplicationPath(m_sCertificateTrustListLocation, sAppPath);
    replaceConfigPath(m_sCertificateTrustListLocation, sAppPath);
    value = pSettings->value("CertificateRevocationListLocation", UaString("pki/trusted/crl/"));
    m_sCertificateRevocationListLocation = value.toString();
    replaceApplicationPath(m_sCertificateRevocationListLocation, sAppPath);
    replaceConfigPath(m_sCertificateRevocationListLocation, sAppPath);
    value = pSettings->value("IssuersCertificatesLocation", UaString("pki/issuers/certs/"));
    m_sIssuersCertificatesLocation = value.toString();
    replaceApplicationPath(m_sIssuersCertificatesLocation, sAppPath);
    replaceConfigPath(m_sIssuersCertificatesLocation, sAppPath);
    value = pSettings->value("IssuersRevocationListLocation", UaString("pki/issuers/crl/"));
    m_sIssuersRevocationListLocation = value.toString();
    replaceApplicationPath(m_sIssuersRevocationListLocation, sAppPath);
    replaceConfigPath(m_sIssuersRevocationListLocation, sAppPath);
    value = pSettings->value("ClientCertificate", UaString("pki/own/certs/uaclientcpp.der"));
    m_sClientCertificate = value.toString();
    replaceApplicationPath(m_sClientCertificate, sAppPath);
    replaceConfigPath(m_sClientCertificate, sAppPath);
    value = pSettings->value("ClientPrivateKey", UaString("pki/own/private/uaclientcpp.pem"));
    m_sClientPrivateKey = value.toString();
    replaceApplicationPath(m_sClientPrivateKey, sAppPath);
    replaceConfigPath(m_sClientPrivateKey, sAppPath);

    // Username and password to use for the session
    value = pSettings->value("Username", UaString(""));
    m_sUsername = value.toString();
    value = pSettings->value("Password", UaString(""));
    m_sPassword = value.toString();

#if OPCUA_SUPPORT_PKI
    // Windows certificate store
#if OPCUA_SUPPORT_PKI_WIN32
    value = pSettings->value("UseWindowsStore", (OpcUa_Boolean)OpcUa_False);
    value.toBool(m_useWindowsStore);

    value = pSettings->value("WindowsStoreLocation", UaString("LOCAL_MACHINE"));
    if ( value.toString() == "LOCAL_MACHINE" )
    {
        m_windowsStoreLocation = Location_LocalMachine;
    }
    else if ( value.toString() == "CURRENT_USER" )
    {
        m_windowsStoreLocation = Location_CurrentUser;
    }
    value = pSettings->value("WindowsStoreName", UaString("UnifiedAutomationUaSamples"));
    m_sWindowsStoreName = value.toString();
    value = pSettings->value("WindowsIssuersStoreName", UaString("UnifiedAutomationUaSamples"));
    m_sWindowsIssuersStoreName = value.toString();
    value = pSettings->value("WindowsClientCertificateThumbprint", UaString("ClientCppSample"));
    m_sWindowsClientCertificateThumbprint = value.toString();

    if ( m_useWindowsStore != OpcUa_False )
    {
        // Check if the certificate exists
        UaByteArray thumbprint = UaByteArray::fromHex(m_sWindowsClientCertificateThumbprint);

        UaPkiCertificate tempServerCert = UaPkiCertificate::fromWindowsStore(
            m_windowsStoreLocation,
            m_sWindowsStoreName,
            thumbprint);
        if ( tempServerCert.isValid() == false )
        {
            // The certificates do not exist, create it
            createCertificateWindowsStore(
                m_windowsStoreLocation,
                m_sWindowsStoreName,
                m_sWindowsClientCertificateThumbprint);

            // Store the thumbpringt in the configuration file
            pSettings->setValue("WindowsClientCertificateThumbprint", m_sWindowsClientCertificateThumbprint);
        }
    }
#else // OPCUA_SUPPORT_PKI_WIN32
    m_useWindowsStore = OpcUa_False;
#endif // OPCUA_SUPPORT_PKI_WIN32
#endif // OPCUA_SUPPORT_PKI

    pSettings->beginGroup("NS1");

    // Read namespace option 1
    value = pSettings->value("NameSpaceUri", UaString(""));
    m_sNamespaceUri = value.toString();
    // Read variable string identifiers
    sTempValue = "xx";
    i = 1;
    while ( sTempValue.length() > 0 )
    {
        sTempKey = UaString("Variable0%1").arg(i);
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        sTempValue = value.toString();
        if ( sTempValue.length() > 0 )
        {
            i++;
        }
    }
    sTempValue = "xx";
    m_VariableNodeIds.create(i-1);
    i = 1;
    while ( sTempValue.length() > 0 )
    {
        sTempKey = UaString("Variable0%1").arg(i);
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        sTempValue = value.toString();
        if ( sTempValue.length() > 0 )
        {
            m_VariableNodeIds[i-1] = sTempValue;
            i++;
        }
    }
    // Read other string identifiers
    value = pSettings->value("WriteVar01", UaString(""));
    m_sWriteVariableNodeId = value.toString();
    value = pSettings->value("Object01", UaString(""));
    m_sObjectNodeId = value.toString();
    value = pSettings->value("Method01", UaString(""));
    m_sMethodNodeId = value.toString();
    value = pSettings->value("HistoryData01", UaString(""));
    m_sHistoryDataId = value.toString();
    value = pSettings->value("HistoryEvent01", UaString(""));
    m_sHistoryEventNotifierId = value.toString();
    value = pSettings->value("EventTriggerObject01", UaString(""));
    m_sEventTriggerObject1= value.toString();
    value = pSettings->value("EventTriggerMethod01", UaString(""));
    m_sEventTriggerMethod1= value.toString();
    value = pSettings->value("EventTriggerObject02", UaString(""));
    m_sEventTriggerObject2= value.toString();
    value = pSettings->value("EventTriggerMethod02", UaString(""));
    m_sEventTriggerMethod2= value.toString();

    pSettings->endGroup(); // NS1

    pSettings->beginGroup("NS2");

    // Read namespace option 1
    value = pSettings->value("NameSpaceUri", UaString(""));
    m_sNamespaceUri2 = value.toString();
    // Read variable string identifiers
    sTempValue = "xx";
    i = 1;
    while ( sTempValue.length() > 0 )
    {
        sTempKey = UaString("Variable0%1").arg(i);
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        sTempValue = value.toString();
        if ( sTempValue.length() > 0 )
        {
            i++;
        }
    }
    sTempValue = "xx";
    m_VariableNodeIds2.create(i-1);
    i = 1;
    while ( sTempValue.length() > 0 )
    {
        sTempKey = UaString("Variable0%1").arg(i);
        value = pSettings->value(sTempKey.toUtf16(), UaString(""));
        sTempValue = value.toString();
        if ( sTempValue.length() > 0 )
        {
            m_VariableNodeIds2[i-1] = sTempValue;
            i++;
        }
    }
    value = pSettings->value("WriteVar01", UaString(""));
    m_sWriteVariableNodeId2 = value.toString();

    pSettings->endGroup(); // NS2

    pSettings->endGroup(); // UaClientConfig

    delete pSettings;
    pSettings = NULL;
}

void ClientSampleConfig::setGdsApplicationObjectId(const UaString& sGdsApplicationObjectId)
{
    m_sGdsApplicationObjectId = sGdsApplicationObjectId;

    UaSettings settings(m_sConfigFile.toUtf16());
    settings.beginGroup("UaClientConfig");
    settings.setValue("GdsApplicationObjectId", sGdsApplicationObjectId);
}

void ClientSampleConfig::replaceApplicationPath(UaString& sString, const UaString& sApplicationPath)
{
    const char* pszString = sString.toUtf8();
    const char* pszFind;
    int len;
    char szTemp[512];

#ifdef _WIN32
# define UA_PATH_SEPARATOR '\\'
#else
# define UA_PATH_SEPARATOR '/'
#endif

    pszFind = strstr(pszString, "[ApplicationPath]");
    if (pszFind)
    {
        // skip [ApplicationPath]
        pszFind += strlen("[ApplicationPath]");

        // build new path
        strncpy(szTemp, sApplicationPath.toUtf8(), 512);
        len = (int)strlen(szTemp);
        if (szTemp[len-1] == UA_PATH_SEPARATOR)
        {
            szTemp[len-1] = 0; // remove trailing slash
        }
        strlcat(szTemp, pszFind, sizeof(szTemp));

#ifdef _WIN32
        char *pszPos;
        // replace slashes with backslashes
        pszPos = szTemp;
        while (*pszPos)
        {
            if (*pszPos == '/') *pszPos = '\\';
            pszPos++;
        }
#endif
        // update output
        sString = szTemp;
    }
}

void ClientSampleConfig::replaceConfigPath(UaString& sString, const UaString& sConfigPath)
{
    const char* pszString = sString.toUtf8();
    const char* pszFind;
    int len;
    char szTemp[512];

#ifdef _WIN32
# define UA_PATH_SEPARATOR '\\'
#else
# define UA_PATH_SEPARATOR '/'
#endif

    pszFind = strstr(pszString, "[ConfigPath]");
    if (pszFind)
    {
        // skip [ConfigPath]
        pszFind += strlen("[ConfigPath]");

        // build new path
        strncpy(szTemp, sConfigPath.toUtf8(), 512);
        len = (int)strlen(szTemp);
        if (szTemp[len-1] == UA_PATH_SEPARATOR)
        {
            szTemp[len-1] = 0; // remove trailing slash
        }
        strlcat(szTemp, pszFind, sizeof(szTemp));

#ifdef _WIN32
        char *pszPos;
        // replace slashes with backslashes
        pszPos = szTemp;
        while (*pszPos)
        {
            if (*pszPos == '/') *pszPos = '\\';
            pszPos++;
        }
#endif
        // update output
        sString = szTemp;
    }
}

void ClientSampleConfig::replaceTracePath(UaString& sString, const UaString& sTracePath)
{
    const char* pszString = sString.toUtf8();
    const char* pszFind;
    int len;
    char szTemp[512];

#ifdef _WIN32
# define UA_PATH_SEPARATOR '\\'
#else
# define UA_PATH_SEPARATOR '/'
#endif

    pszFind = strstr(pszString, "[TracePath]");
    if (pszFind)
    {
        // skip [TracePath]
        pszFind += strlen("[TracePath]");

        // build new path
        strncpy(szTemp, sTracePath.toUtf8(), 512);
        len = (int)strlen(szTemp);
        if (szTemp[len-1] == UA_PATH_SEPARATOR)
        {
            szTemp[len-1] = 0; // remove trailing slash
        }
        strlcat(szTemp, pszFind, sizeof(szTemp));

#ifdef _WIN32
        char *pszPos;
        // replace slashes with backslashes
        pszPos = szTemp;
        while (*pszPos)
        {
            if (*pszPos == '/') *pszPos = '\\';
            pszPos++;
        }
#endif
        // update output
        sString = szTemp;
    }
}

#if OPCUA_SUPPORT_PKI
/*============================================================================
 * Security Helper
 *===========================================================================*/
UaStatus ClientSampleConfig::setupSecurity(SessionSecurityInfo &sessionSecurityInfo)
{
    UaStatus status;
    if ( useWindowsStore() == OpcUa_False )
    {
        status = setupSecurityOpenSSL(sessionSecurityInfo);
    }
    else
    {
#if OPCUA_SUPPORT_PKI_WIN32
        status = setupSecurityWindowsStore(sessionSecurityInfo);
#endif // OPCUA_SUPPORT_PKI_WIN32
    }
    return status;
}


UaStatus ClientSampleConfig::setupSecurityOpenSSL(SessionSecurityInfo& sessionSecurityInfo)
{
    UaStatus uStatus;

    // Check if the PKI infrastructure exists
    FILE* pFile = fopen( m_sClientCertificate.toUtf8(), "r");
    if ( pFile != NULL )
    {
        fclose(pFile);
    }
    else
    {
        // The certificates and the revocation list do not exist, create them
        createCertificates(
            m_sClientCertificate,
            m_sClientPrivateKey,
            m_sCertificateTrustListLocation,
            m_sCertificateRevocationListLocation);
    }

    /*********************************************************************
     Initialize the PKI provider for OpenSSL
    **********************************************************************/
    uStatus = sessionSecurityInfo.initializePkiProviderOpenSSL(
        m_sCertificateRevocationListLocation,
        m_sCertificateTrustListLocation,
        m_sIssuersRevocationListLocation,
        m_sIssuersCertificatesLocation);
    /*********************************************************************/
    if (uStatus.isBad())
    {
        printf("*******************************************************\n");
        printf("** setupSecurity failed!\n");
        printf("** Could not initialize PKI\n");
        printf("*******************************************************\n");
        return uStatus;
    }

    /*********************************************************************
     Load certificate and private key for client from OpenSSL store
    **********************************************************************/
    uStatus = sessionSecurityInfo.loadClientCertificateOpenSSL(
        m_sClientCertificate,
        m_sClientPrivateKey);
    /*********************************************************************/
    if (uStatus.isBad())
    {
        printf("*******************************************************\n");
        printf("** setupSecurity failed!\n");
        printf("** Could not load Client certificate\n");
        printf("** Connect will work only without security\n");
        printf("*******************************************************\n");
        return uStatus;
    }

    return uStatus;
}

/*============================================================================
 * Certificate Helper
 *===========================================================================*/
void ClientSampleConfig::createCertificates(const UaString& sClientCertificateFile, const UaString& sClientPrivateKeyFile, const UaString& sCertificateTrustListLocation, const UaString& sCertificateRevocationListLocation)
{
    // First make sure the directories exist
    UaDir dirHelper("");
    UaUniString usClientCertificatePath(dirHelper.filePath(UaDir::fromNativeSeparators(sClientCertificateFile.toUtf16())));
    dirHelper.mkpath(usClientCertificatePath);
    UaUniString usPrivateKeyPath(dirHelper.filePath(UaDir::fromNativeSeparators(sClientPrivateKeyFile.toUtf16())));
    dirHelper.mkpath(usPrivateKeyPath);
    UaUniString usTrustListLocationPath(dirHelper.filePath(UaDir::fromNativeSeparators(sCertificateTrustListLocation.toUtf16())));
    dirHelper.mkpath(usTrustListLocationPath);
    UaUniString usRevocationListPath(dirHelper.filePath(UaDir::fromNativeSeparators(sCertificateRevocationListLocation.toUtf16())));
    dirHelper.mkpath(usRevocationListPath);

    // Create the certificate
    UaPkiRsaKeyPair keyPair ( 2048 );
    UaPkiIdentity   identity;

    identity.commonName       = m_sApplicationName;
    identity.organization     = "Organization";
    identity.organizationUnit = "Unit";
    identity.locality         = "LocationName";
    identity.state            = "State";
    identity.country          = "DE";
    identity.domainComponent  = "MyComputer";

    UaPkiCertificateInfo info;
    info.URI       = m_sApplicationUri;
    info.DNSNames.create(1);
    m_sHostName.copyTo(&info.DNSNames[0]);
    info.validTime = 3600*24*365*5; // seconds

    // create a self signed certificate
    UaPkiCertificate cert ( info, identity, keyPair, false, UaPkiCertificate::SignatureAlgorithm_Sha256 );

    // encoded to DER format
    cert.toDERFile ( sClientCertificateFile.toUtf8() );

    keyPair.toPEMFile ( sClientPrivateKeyFile.toUtf8(), 0 );
    UaByteArray DERData = keyPair.toDER();
}

/*============================================================================
 * printCertificateData
 *===========================================================================*/
void ClientSampleConfig::printCertificateData(const UaByteString& serverCertificate)
{
    // Assign certificate byte string to UaPkiCertificate class
    UaPkiCertificate cert = UaPkiCertificate::fromDER(serverCertificate);

    printf("- CommonName              %s\n", cert.commonName().toUtf8() );
    printf("- Issuer.commonName       %s\n", cert.issuer().commonName.toUtf8() );
    printf("- Issuer.organization     %s\n", cert.issuer().organization.toUtf8() );
    printf("- Issuer.organizationUnit %s\n", cert.issuer().organizationUnit.toUtf8() );
    printf("- Issuer.state            %s\n", cert.issuer().state.toUtf8() );
    printf("- Issuer.country          %s\n", cert.issuer().country.toUtf8() );
    printf("- ValidFrom               %s\n", cert.validFrom().toString().toUtf8() );
    printf("- ValidTo                 %s\n", cert.validTo().toString().toUtf8() );
}

#if OPCUA_SUPPORT_PKI_WIN32
/*============================================================================
 * setupSecurityWindowsStore
 *===========================================================================*/
UaStatus ClientSampleConfig::setupSecurityWindowsStore(SessionSecurityInfo& sessionSecurityInfo)
{
    UaStatus uStatus;

    /*********************************************************************
     Initialize the PKI provider for Windows certificate store
    **********************************************************************/
    uStatus = sessionSecurityInfo.initializePkiProviderWindows(
        m_windowsStoreLocation,
        m_sWindowsStoreName,
        m_sWindowsIssuersStoreName);
    /*********************************************************************/
    if (uStatus.isBad())
    {
        printf("*******************************************************\n");
        printf("** setupSecurityWindowsStore failed!\n");
        printf("** Could not initialize PKI\n");
        printf("*******************************************************\n");
        return uStatus;
    }

    /*********************************************************************
     Load certificate and private key for client from Windows certificate store
    **********************************************************************/
    uStatus = sessionSecurityInfo.loadClientCertificateWindows(
        m_sWindowsClientCertificateThumbprint);
    /*********************************************************************/
    if (uStatus.isBad())
    {
        printf("*******************************************************\n");
        printf("** setupSecurityWindowsStore failed!\n");
        printf("** Could not load Client certificate\n");
        printf("** Connect will work only without security\n");
        printf("*******************************************************\n");
        return uStatus;
    }

    return uStatus;
}

/*============================================================================
 * createCertificateWindowsStore
 *===========================================================================*/
void ClientSampleConfig::createCertificateWindowsStore(WindowsStoreLocation windowsStoreLocation, const UaString& sWindowsStoreName, UaString& sThumbprint)
{
    // Create the certificate
    UaPkiRsaKeyPair keyPair ( 2048 );
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
    identity.domainComponent  = "MyComputer";

    UaPkiCertificateInfo info;
    info.URI       = UaString("urn:%1:UnifiedAutomation::Client_Cpp_SDK").arg(sNodeName);
    info.DNSNames.create(1);
    sNodeName.copyTo(&info.DNSNames[0]);
    info.validTime = 3600*24*365*5; // seconds

    // create a self signed certificate
    UaPkiCertificate cert ( info, identity, keyPair, false, UaPkiCertificate::SignatureAlgorithm_Sha256 );

    // Store in Windows certificate store
    cert.toWindowsStoreWithPrivateKey(windowsStoreLocation, sWindowsStoreName, keyPair);

    // Create the thumbprint string to be stored as name
    sThumbprint = cert.thumbPrint().toHex();
}
#endif // OPCUA_SUPPORT_PKI_WIN32
#endif // OPCUA_SUPPORT_PKI
