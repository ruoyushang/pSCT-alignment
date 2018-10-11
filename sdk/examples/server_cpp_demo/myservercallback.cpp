#include "myservercallback.h"
#include "uasession.h"
#include "nodemanagerroot.h"
#include "serverconfigdata.h"
#include "sessionusercontext.h"
#include "nodeaccessinfo.h"

#if OPCUA_SUPPORT_PKI
#include "opcua_core.h"
#include "uapkicertificate.h"
#include "uadir.h"
#endif //OPCUA_SUPPORT_PKI

MyServerCallback::MyServerCallback()
{
    m_uRootUserId = 0;
    m_uSecurityAdminGroupId = 0;
    m_uConfigAdminGroupId = 0;

#if OPCUA_SUPPORT_PKI
    m_nRejectedCertificatesCount = 100;
    m_bCertificateTokenConfigured = OpcUa_False;
#endif //OPCUA_SUPPORT_PKI
}

MyServerCallback::~MyServerCallback()
{
}

Session* MyServerCallback::createSession(OpcUa_Int32 sessionID, const UaNodeId &authenticationToken)
{
    // Create the application specific session where we can store our own information during user authentication (logonSessionUser)
    // We will used this information later for user authorisation like in browse, read and write
    UaSession* pSession = new UaSession(sessionID, authenticationToken);
    return pSession;
}

UaStatus MyServerCallback::logonSessionUser(Session* pSession, UaUserIdentityToken* pUserIdentityToken, ServerConfig* pServerConfig)
{
    if ( pServerConfig == NULL )
    {
        return OpcUa_Bad;
    }
    OpcUa_Boolean  bEnableAnonymous;
    OpcUa_Boolean  bEnableUserPw;
    OpcUa_Boolean  bEnableCertificate;
    OpcUa_Boolean  bEnableKerberosTicket;

    pServerConfig->getUserIdentityTokenConfig(bEnableAnonymous, bEnableUserPw, bEnableCertificate, bEnableKerberosTicket);

    if ( pUserIdentityToken->getTokenType() == OpcUa_UserTokenType_Anonymous )
    {
        if ( bEnableAnonymous == OpcUa_False )
        {
            // Return error if Anonymous is not enabled
            return OpcUa_BadIdentityTokenRejected;
        }
        else
        {
            // reset any existing user context
            pSession->setUserContext(NULL);

            // Return OK but we do not set any user information in MySession
            return OpcUa_Good;
        }
    }
    //! [Username/Password Authentication]
    else if ( pUserIdentityToken->getTokenType() == OpcUa_UserTokenType_UserName )
    {
        if ( bEnableUserPw == OpcUa_False )
        {
            // Return error if User/Password is not enabled
            return OpcUa_BadIdentityTokenRejected;
        }
        else
        {
            // Check user and password and set user related information on MySession
            UaUserIdentityTokenUserPassword* pUserPwToken = (UaUserIdentityTokenUserPassword*)pUserIdentityToken;

            // ++ Simplified sample code +++++++++++++++++++++++++++++++++++++++
            // Implement user authentication here
            // This is just a trivial example with four different users
            if ( pUserPwToken->sUserName == "root" )
            {
                if ( pUserPwToken->sPassword == "secret" )
                {
                    // setup user context for Admin
                    SessionUserContext* pUsercontext = new SessionUserContext();
                    pUsercontext->setUserId(m_uRootUserId);                 // userId for root - special handling when checking permissions
                    pUsercontext->addMembership(m_uSecurityAdminGroupId);   // groupId for SecurityAdminGroup
                    pUsercontext->setDefaultPermissions(NodeAccessInfo::UA_PERMISSION_ALL);
                    pSession->setUserContext(pUsercontext);
                    pSession->setClientUserId("root");
                    pUsercontext->releaseReference();

                    return OpcUa_Good;
                }
                else
                {
                    return OpcUa_BadUserAccessDenied;
                }
            }
            else if ( pUserPwToken->sUserName == "joe" )
            {
                if ( pUserPwToken->sPassword == "god" )
                {
                    // setup user context for joe
                    // joe is member of the groups: users
                    SessionUserContext* pUsercontext = new SessionUserContext();
                    pUsercontext->setUserId(UserIdJoe);
                    pUsercontext->addMembership(GroupIdUsers);
                    pUsercontext->setDefaultPermissions(NodeAccessInfo::UA_PERMISSION_ALL);
                    pSession->setUserContext(pUsercontext);
                    pSession->setClientUserId("joe");
                    pUsercontext->releaseReference();

                    return OpcUa_Good;
                }
                else
                {
                    return OpcUa_BadUserAccessDenied;
                }
            }
            else if ( pUserPwToken->sUserName == "john" )
            {
                if ( pUserPwToken->sPassword == "master" )
                {
                    // setup user context for john
                    // john is member of the groups: users, operators, securityadmin, configadmin
                    SessionUserContext* pUsercontext = new SessionUserContext();
                    pUsercontext->setUserId(UserIdJohn);
                    pUsercontext->addMembership(GroupIdUsers);
                    pUsercontext->addMembership(GroupIdOperators);
                    pUsercontext->addMembership(m_uSecurityAdminGroupId);
                    pUsercontext->addMembership(m_uConfigAdminGroupId);
                    pUsercontext->setDefaultPermissions(NodeAccessInfo::UA_PERMISSION_ALL);
                    pSession->setUserContext(pUsercontext);
                    pSession->setClientUserId("john");
                    pUsercontext->releaseReference();

                    return OpcUa_Good;
                }
                else
                {
                    return OpcUa_BadUserAccessDenied;
                }
            }
            else if ( pUserPwToken->sUserName == "sue" )
            {
                if ( pUserPwToken->sPassword == "curly" )
                {
                    // setup user context for sue
                    // sue is member of the groups: users, operators
                    SessionUserContext* pUsercontext = new SessionUserContext();
                    pUsercontext->setUserId(UserIdSue);
                    pUsercontext->addMembership(GroupIdUsers);
                    pUsercontext->addMembership(GroupIdOperators);
                    pUsercontext->setDefaultPermissions(NodeAccessInfo::UA_PERMISSION_ALL);
                    pSession->setUserContext(pUsercontext);
                    pSession->setClientUserId("sue");
                    pUsercontext->releaseReference();

                    return OpcUa_Good;
                }
                else
                {
                    return OpcUa_BadUserAccessDenied;
                }
            }
            else
            {
                // we treat every known user as operator here
                if ( verifyUserWithOS(pUserPwToken->sUserName, pUserPwToken->sPassword).isGood() )
                {
                    // setup user context for Operator
                    SessionUserContext* pUsercontext = new SessionUserContext();
                    pUsercontext->setUserId(UserIdOS);
                    pUsercontext->addMembership(GroupIdUsers);
                    pUsercontext->addMembership(GroupIdOperators);
                    pUsercontext->setDefaultPermissions(NodeAccessInfo::UA_PERMISSION_ALL);
                    pSession->setUserContext(pUsercontext);
                    pSession->setClientUserId(pUserPwToken->sUserName);
                    pUsercontext->releaseReference();

                    return OpcUa_Good;
                }
                else
                {
                    return OpcUa_BadUserAccessDenied;
                }
            }
            // ++ Simplified sample code +++++++++++++++++++++++++++++++++++++++
        }
    }
    //! [Username/Password Authentication]
    //! [User Certificate Authentication]
    else if ( pUserIdentityToken->getTokenType() == OpcUa_UserTokenType_Certificate )
    {
#if OPCUA_SUPPORT_PKI
        if ( bEnableCertificate == OpcUa_False || m_bCertificateTokenConfigured == OpcUa_False )
        {
            // Return error if CertificateToken is not enabled
            return OpcUa_BadIdentityTokenRejected;
        }
        else
        {
            // Just check if the certificate it trusted - we don't map this to a known user
            UaUserIdentityTokenCertificate* pUserCertToken = (UaUserIdentityTokenCertificate*)pUserIdentityToken;

            // create certificate object to get name and thumbprint
            UaPkiCertificate pkiCertificate = UaPkiCertificate::fromDER(pUserCertToken->userCertificateData);

            // validate the user certificate against the user certificate store
            UaStatus ret = validateCertificate(pUserCertToken->userCertificateData).isGood();

            // user is known so we put him into the Operator group
            if ( ret.isGood() )
            {
                // setup user context for Operator
                SessionUserContext* pUsercontext = new SessionUserContext();
                pUsercontext->setUserId(2);
                pUsercontext->addMembership(1);
                pUsercontext->setDefaultPermissions(NodeAccessInfo::UA_PERMISSION_ALL);
                pSession->setUserContext(pUsercontext);
                pUsercontext->releaseReference();
            }
            // Copy the certificate into the rejected folder
            else
            {
                // ToDo - check number of rejected certifiates to respect the m_nRejectedCertificatesCount
                // get thumbprint and build filename
                UaString fileName = UaString("%1%2.der")
                    .arg(m_sRejectedFolder)
                    .arg(pkiCertificate.thumbPrint().toHex());

                // save certificte
                pkiCertificate.toDERFile(fileName);
            }
            return ret;
        }
#else//OPCUA_SUPPORT_PKI
        // Return error if PKI is not enabled
        return OpcUa_BadIdentityTokenRejected;
#endif //OPCUA_SUPPORT_PKI
    }
    //! [User Certificate Authentication]
    return OpcUa_BadIdentityTokenInvalid;
}

void MyServerCallback::afterLoadConfiguration(ServerConfig* pServerConfig)
{
    ServerConfigData* pServerConfigData = (ServerConfigData*)pServerConfig;

    // Make sure the demo server product information can not be overwritten by the config file
    pServerConfigData->setProductUri("urn:UnifiedAutomation:UaServerCpp");
    pServerConfigData->setManufacturerName("Unified Automation GmbH");
    pServerConfigData->setProductName("C++ SDK OPC UA Demo Server");
    pServerConfigData->setSoftwareVersion(SERVERCONFIG_SOFTWAREVERSION);
    pServerConfigData->setBuildNumber(SERVERCONFIG_BUILDNUMBER);

    // read Ids for Root, SecurityAdminGroup and ConfigAdminGroup
    pServerConfigData->getIdsForDefaultRoles(
        m_uRootUserId,
        m_uSecurityAdminGroupId,
        m_uConfigAdminGroupId);

#if OPCUA_SUPPORT_PKI
    CertificateStoreConfiguration* pCertificateStore = NULL;
    UaString sRejectedCertificateDirectory;
    OpcUa_UInt32 nRejectedCertificatesCount;
    OpcUa_Boolean bCertificateTokenConfigured = OpcUa_False;

    pServerConfigData->getDefaultUserCertificateStore(
        &pCertificateStore,
        sRejectedCertificateDirectory,
        nRejectedCertificatesCount,
        bCertificateTokenConfigured);

    m_sCertificateTrustListLocation = pCertificateStore->m_sCertificateTrustListLocation;
    m_sCertificateRevocationListLocation = pCertificateStore->m_sCertificateRevocationListLocation;
    m_sIssuersCertificatesLocation = pCertificateStore->m_sIssuersCertificatesLocation;
    m_sIssuersRevocationListLocation = pCertificateStore->m_sIssuersRevocationListLocation;
    m_sRejectedFolder = sRejectedCertificateDirectory;
    m_nRejectedCertificatesCount = nRejectedCertificatesCount;
    m_bCertificateTokenConfigured = bCertificateTokenConfigured;

    // make sure the paths all exist
    if (bCertificateTokenConfigured)
    {
        UaDir dirHelper("");
        UaUniString usPath;
        usPath = UaUniString(dirHelper.filePath(UaDir::fromNativeSeparators(m_sCertificateTrustListLocation.toUtf16())));
        dirHelper.mkpath(usPath);
        usPath = UaUniString(dirHelper.filePath(UaDir::fromNativeSeparators(m_sCertificateRevocationListLocation.toUtf16())));
        dirHelper.mkpath(usPath);
        usPath = UaUniString(dirHelper.filePath(UaDir::fromNativeSeparators(m_sIssuersCertificatesLocation.toUtf16())));
        dirHelper.mkpath(usPath);
        usPath = UaUniString(dirHelper.filePath(UaDir::fromNativeSeparators(m_sIssuersRevocationListLocation.toUtf16())));
        dirHelper.mkpath(usPath);
        usPath = UaUniString(dirHelper.filePath(UaDir::fromNativeSeparators(m_sRejectedFolder.toUtf16())));
        dirHelper.mkpath(usPath);
    }
#endif //OPCUA_SUPPORT_PKI
}

UaStatus MyServerCallback::verifyUserWithOS(const UaString& sUserName, const UaString& sPassword)
{
    OpcUa_ReferenceParameter(sUserName);
    OpcUa_ReferenceParameter(sPassword);
    UaStatus ret = OpcUa_BadUserAccessDenied;

    // Only Windows sample code available at the moment
#if defined (_WIN32) && !defined(_WIN32_WCE) && !defined(WIN_IOT)

    HANDLE userContextHandle;
    UaString tempUserName;
    UaString uaDomain;
    int i;
    int iBSPos = sUserName.find("\\");
    int iAtPos = sUserName.find("@");
    if (iBSPos > 0 && iAtPos == -1)
    {
        for (i = 0; i < iBSPos; i++) {uaDomain += sUserName.at(i).data();}
        for (i = iBSPos + 1; i < sUserName.length(); i++) {tempUserName += sUserName.at(i).data();}
    }
    else if (iAtPos > 0 && iBSPos == -1)
    {
        for (i = 0; i < iAtPos; i++) {tempUserName += sUserName.at(i).data();}
        for (i = iAtPos + 1; i < sUserName.length(); i++) {uaDomain += sUserName.at(i).data();}
    }
    else
    {
        tempUserName = sUserName;
    }

#ifdef  UNICODE
    UaByteArray wsUserName = tempUserName.toUtf16();
    LPTSTR lpszUsername = (wchar_t*)(const UaUShort*)wsUserName;
    UaByteArray wsDomain = uaDomain.toUtf16();
    LPTSTR lpszDomain = (wsDomain.size() > 0) ? ((wchar_t*)(const UaUShort*)wsDomain) : NULL;
    UaByteArray wsPassword = sPassword.toUtf16();
    LPTSTR lpszPassword = (wchar_t*)(const UaUShort*)wsPassword;

#else   /* UNICODE */
    char *lpszUsername = (char*)uaUserName.toUtf8();
    char *lpszDomain = (uaDomain.length() > 0) ? ((char*)uaDomain.toUtf8()) : NULL;
    char *lpszPassword = (char*)pUserPwToken->sPassword.toUtf8();
#endif /* UNICODE */

    // http://msdn2.microsoft.com/en-us/library/Aa378184.aspx
    // requires include Windows.h
    // requires lib Advapi32.lib
    BOOL logOnResult = LogonUser(
        lpszUsername,
        lpszDomain,
        lpszPassword,
        LOGON32_LOGON_INTERACTIVE,     // DWORD dwLogonType,
        LOGON32_PROVIDER_DEFAULT,  // DWORD dwLogonProvider,
        &userContextHandle);

    if ( logOnResult == FALSE )
    {
        ret = OpcUa_BadUserAccessDenied;
    }
    else
    {
        CloseHandle(userContextHandle);
        ret = OpcUa_Good;
    }
#endif /* defined (_WIN32) && !defined(_WIN32_WCE) && !defined(WIN_IOT) */

    return ret;
}

#if OPCUA_SUPPORT_PKI
UaStatus MyServerCallback::validateCertificate(const UaByteString& certificate)
{
    UaStatus ret;

    // create configuration for PkiProvider
    OpcUa_CertificateStoreConfiguration pkiConfig;
    OpcUa_CertificateStoreConfiguration_Initialize(&pkiConfig);

    pkiConfig.strPkiType                            = (OpcUa_StringA)OPCUA_P_PKI_TYPE_OPENSSL;
    pkiConfig.strRevokedCertificateListLocation     = (OpcUa_StringA)m_sCertificateRevocationListLocation.toUtf8();
    pkiConfig.strTrustedCertificateListLocation     = (OpcUa_StringA)m_sCertificateTrustListLocation.toUtf8();

    if ( m_sIssuersRevocationListLocation.length() > 0 && m_sIssuersCertificatesLocation.length() > 0 )
    {
        pkiConfig.strRevokedIssuerCertificateListLocation   = (OpcUa_StringA)m_sIssuersRevocationListLocation.toUtf8();
        pkiConfig.strIssuerCertificateStoreLocation         = (OpcUa_StringA)m_sIssuersCertificatesLocation.toUtf8();
    }
    else
    {
        pkiConfig.strRevokedIssuerCertificateListLocation   = OpcUa_Null;
        pkiConfig.strIssuerCertificateStoreLocation         = OpcUa_Null;
    }

    pkiConfig.uFlags                        = OpcUa_P_PKI_OPENSSL_CHECK_REVOCATION_STATUS;
    pkiConfig.pvOverride                    = NULL;
    pkiConfig.pfVerifyCallback              = NULL;
    pkiConfig.pvVerifyCallbackUserData      = NULL;

    // create Pki Provider with the configuration setup above
    OpcUa_PKIProvider pkiProvider;
    OpcUa_PKIProvider_Create(&pkiConfig, &pkiProvider);

    // open the certificate store
    OpcUa_Handle hCertificateStore;
    ret = pkiProvider.OpenCertificateStore(
        &pkiProvider,
        &hCertificateStore);

    // validate the certificate
    if ( ret.isGood() )
    {
        OpcUa_Int validationCode;
        ret = pkiProvider.ValidateCertificate(
            &pkiProvider,
            (const OpcUa_ByteString*)certificate,
            hCertificateStore,
            &validationCode);

        // close the certificate store
        pkiProvider.CloseCertificateStore(
            &pkiProvider,
            &hCertificateStore);
    }

    return ret;
}
#endif //OPCUA_SUPPORT_PKI