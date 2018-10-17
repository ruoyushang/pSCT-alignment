#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "uabase.h"
#include "uaclientsdk.h"

#define COMPANY_NAME "UnifiedAutomation"
#define PRODUCT_NAME "GettingStartedClient"

using namespace UaClientSdk;

class Configuration
{
    UA_DISABLE_COPY(Configuration);
public:
    Configuration();
    virtual ~Configuration();

    // get connection and session parameters
    UaString getServerUrl() const;
    UaString getDiscoveryUrl() const;
    UaString getApplicationName() const;
    UaString getProductUri() const;
    OpcUa_Boolean getAutomaticReconnect() const;
    OpcUa_Boolean getRetryInitialConnect() const;

    // get lists of NodeIds and values
    UaNodeIdArray getNodesToRead() const;
    UaNodeIdArray getNodesToWrite() const;
    UaVariantArray getWriteValues() const;

    // load configuration from file to fill connection parameters, NamespaceArray and NodeIds
    UaStatus loadConfiguration(const UaString& sConfigurationFile);

    // create the folder structure to handle certificates and load or create a client certificate.
    UaStatus setupSecurity(SessionSecurityInfo& sessionSecurityInfo);

    // update the namespace indexes for all nodeIds and update the internal namespaceArray
    UaStatus updateNamespaceIndexes(const UaStringArray& namespaceArray);

private:
    // connection and session configuration
    UaString        m_applicationName;
    UaString        m_serverUrl;
    UaString        m_discoveryUrl;
    OpcUa_Boolean   m_bAutomaticReconnect;
    OpcUa_Boolean   m_bRetryInitialConnect;

    // NamespaceArray and NodeIds
    UaStringArray   m_namespaceArray;
    UaNodeIdArray   m_nodesToRead;
    UaNodeIdArray   m_nodesToWrite;
    UaVariantArray  m_writeValues;

    // Certificate handling
    UaString        m_certificateTrustListLocation;
    UaString        m_certificateRevocationListLocation;
    UaString        m_issuersCertificatesLocation;
    UaString        m_issuersRevocationListLocation;
    UaString        m_clientCertificateFile;
    UaString        m_clientPrivateKeyFile;
};

#endif // CONFIGURATION_H
