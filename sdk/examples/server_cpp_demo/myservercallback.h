#ifndef MYSERVERCALLBACK_H
#define MYSERVERCALLBACK_H

#include "opcserver.h"
#include "serverconfig.h"

// Ids for Users and Groups - these Ids will usually come from a UserManagement component
const OpcUa_UInt16 UserIdJoe = 1;
const OpcUa_UInt16 UserIdJohn = 2;
const OpcUa_UInt16 UserIdSue = 3;
const OpcUa_UInt16 UserIdOS = 4;
const OpcUa_UInt16 GroupIdUsers = 1;
const OpcUa_UInt16 GroupIdOperators = 2;

class MyServerCallback: public UaServerApplicationCallback
{
    UA_DISABLE_COPY(MyServerCallback);
public:
    /** Construction */
    MyServerCallback();

    /** Destruction */
    virtual ~MyServerCallback();

    virtual Session* createSession(OpcUa_Int32 sessionID, const UaNodeId &authenticationToken);
    virtual UaStatus logonSessionUser(Session* pSession, UaUserIdentityToken* pUserIdentityToken, ServerConfig* pServerConfig);
    virtual void afterLoadConfiguration(ServerConfig* pServerConfig);

private:
    UaStatus verifyUserWithOS(const UaString& sUserName, const UaString& sPassword);
#if OPCUA_SUPPORT_PKI
    UaStatus validateCertificate(const UaByteString& certificate);
#endif //OPCUA_SUPPORT_PKI

    // Ids for default roles
    OpcUa_UInt16 m_uRootUserId;
    OpcUa_UInt16 m_uSecurityAdminGroupId;
    OpcUa_UInt16 m_uConfigAdminGroupId;

#if OPCUA_SUPPORT_PKI
    UaString m_sCertificateTrustListLocation;
    UaString m_sCertificateRevocationListLocation;
    UaString m_sIssuersCertificatesLocation;
    UaString m_sIssuersRevocationListLocation;
    UaString m_sRejectedFolder;
    OpcUa_UInt32 m_nRejectedCertificatesCount;
    OpcUa_Boolean m_bCertificateTokenConfigured;
#endif //OPCUA_SUPPORT_PKI
};

#endif // MYSERVERCALLBACK_H
