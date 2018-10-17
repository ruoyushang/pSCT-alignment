#include "uasettings.h"
#include "uaclientsdk.h"
using namespace UaClientSdk;

#if OPCUA_SUPPORT_PKI
#include "uapkicertificate.h"
#endif // OPCUA_SUPPORT_PKI

// Configuration for the sample client
class ClientSampleConfig
{
public:
    ClientSampleConfig();
    ~ClientSampleConfig();

    void loadConfiguration(const UaString& sIniFileName, const UaString& sAppPath);

    inline UaString sHostName() const { return m_sHostName;}
    inline UaString sApplicationName() const { return m_sApplicationName;}
    inline UaString sApplicationUri() const { return m_sApplicationUri;}
    inline UaString sProductUri() const { return m_sProductUri;}

    inline UaString sDefaultServerUrl() const { return m_sDefaultServerUrl;}
    inline UaString sDiscoveryUrl() const { return m_sDiscoveryUrl;}

    inline UaString sUsername() const { return m_sUsername;}
    inline UaString sPassword() const { return m_sPassword;}

    inline UaString sGdsServerUrl() const { return m_sGdsServerUrl;}
    inline UaString sGdsApplicationObjectId() const { return m_sGdsApplicationObjectId;}
    void setGdsApplicationObjectId(const UaString& sGdsApplicationObjectId);
    inline UaString sGdsUser() const { return m_sGdsUser; }
    inline UaString sGdsPassword() const { return m_sGdsPassword; }

#if OPCUA_SUPPORT_PKI
    inline OpcUa_Boolean useWindowsStore() const { return m_useWindowsStore;}
#if OPCUA_SUPPORT_PKI_WIN32
    inline WindowsStoreLocation windowsStoreLocation() { return m_windowsStoreLocation;}
    inline UaString sWindowsStoreName() const { return m_sWindowsStoreName;}
    inline void setWindowsClientCertificateThumbprint(const UaString &thumbprint) {m_sWindowsClientCertificateThumbprint = thumbprint;}
#endif // OPCUA_SUPPORT_PKI_WIN32
#endif // OPCUA_SUPPORT_PKI
    inline UaString sCertificateTrustListLocation() const { return m_sCertificateTrustListLocation;}
    inline UaString sCertificateRevocationListLocation() const { return m_sCertificateRevocationListLocation;}
    inline UaString sIssuersCertificatesLocation() const { return m_sIssuersCertificatesLocation;}
    inline UaString sIssuersRevocationListLocation() const { return m_sIssuersRevocationListLocation;}
    inline UaString sClientCertificate() const { return m_sClientCertificate;}
    inline UaString sClientPrivateKey() const { return m_sClientPrivateKey;}

#if OPCUA_SUPPORT_PKI
    UaStatus setupSecurity(SessionSecurityInfo& sessionSecurityInfo);
    UaStatus setupSecurityOpenSSL(SessionSecurityInfo& sessionSecurityInfo);
    void createCertificates(const UaString& sClientCertificateFile, const UaString& sClientPrivateKeyFile, const UaString& sCertificateTrustListLocation, const UaString& sCertificateRevocationListLocation);
    static void printCertificateData(const UaByteString& serverCertificate);
#if OPCUA_SUPPORT_PKI_WIN32
    UaStatus setupSecurityWindowsStore(SessionSecurityInfo& sessionSecurityInfo);
    void createCertificateWindowsStore(WindowsStoreLocation windowsStoreLocation, const UaString& sWindowsStoreName, UaString& sThumbprint);
#endif // OPCUA_SUPPORT_PKI_WIN32
#endif // OPCUA_SUPPORT_PKI

    static void replaceApplicationPath(UaString& sString, const UaString& sApplicationPath);
    static void replaceConfigPath(UaString& sString, const UaString& sConfigPath);
    static void replaceTracePath(UaString& sString, const UaString& sTracePath);

private:
    UaString m_sConfigFile;
    UaString m_sHostName;
    UaString m_sApplicationName;
    UaString m_sApplicationUri;
    UaString m_sProductUri;

    // URLs
    UaString  m_sDefaultServerUrl;
    UaString  m_sDiscoveryUrl;

    // GDS
    UaString  m_sGdsServerUrl;
    UaString  m_sGdsUser;
    UaString  m_sGdsPassword;
    UaString  m_sGdsApplicationObjectId;

    // OpenSSL PKI store
    UaString  m_sCertificateTrustListLocation;
    UaString  m_sCertificateRevocationListLocation;
    UaString  m_sIssuersCertificatesLocation;
    UaString  m_sIssuersRevocationListLocation;
    UaString  m_sClientCertificate;
    UaString  m_sClientPrivateKey;

    // Username and password
    UaString  m_sUsername;
    UaString  m_sPassword;

#if OPCUA_SUPPORT_PKI
    // Windows certificate store
    OpcUa_Boolean           m_useWindowsStore;
#if OPCUA_SUPPORT_PKI_WIN32
    WindowsStoreLocation    m_windowsStoreLocation;
    UaString                m_sWindowsStoreName;
    UaString                m_sWindowsIssuersStoreName;
    UaString                m_sWindowsClientCertificateThumbprint;
#endif // OPCUA_SUPPORT_PKI_WIN32
#endif // OPCUA_SUPPORT_PKI


public:
    // Sample nodes
    UaString                m_sNamespaceUri;
    UaObjectArray<UaString> m_VariableNodeIds;
    UaString                m_sNamespaceUri2;
    UaObjectArray<UaString> m_VariableNodeIds2;
    UaString                m_sWriteVariableNodeId;
    UaString                m_sWriteVariableNodeId2;
    UaString                m_sObjectNodeId;
    UaString                m_sMethodNodeId;
    UaString                m_sHistoryDataId;
    UaString                m_sHistoryEventNotifierId;
    UaString                m_sEventTriggerObject1;
    UaString                m_sEventTriggerObject2;
    UaString                m_sEventTriggerMethod1;
    UaString                m_sEventTriggerMethod2;
};
