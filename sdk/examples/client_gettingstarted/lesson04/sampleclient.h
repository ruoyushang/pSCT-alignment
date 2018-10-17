#ifndef SAMPLECLIENT_H
#define SAMPLECLIENT_H

#include "uabase.h"
#include "uaclientsdk.h"

class SampleSubscription;
class Configuration;

using namespace UaClientSdk;

class SampleClient : public UaSessionCallback
{
    UA_DISABLE_COPY(SampleClient);
public:
    SampleClient();
    virtual ~SampleClient();

    // UaSessionCallback implementation ----------------------------------------------------
    virtual void connectionStatusChanged(OpcUa_UInt32 clientConnectionId, UaClient::ServerStatus serverStatus);
    // UaSessionCallback implementation ------------------------------------------------------

    // set a configuration object we use to get connection parameters and NodeIds
    void setConfiguration(Configuration* pConfiguration);

    // OPC UA service calls
    UaStatus discover();
    UaStatus connect();
    UaStatus connectSecure();
    UaStatus disconnect();
    UaStatus browseSimple();
    UaStatus browseContinuationPoint();
    UaStatus read();
    UaStatus write();
    UaStatus subscribe();
    UaStatus unsubscribe();

private:
    // helper methods
    UaStatus browseInternal(const UaNodeId& nodeToBrowse, OpcUa_UInt32 maxReferencesToReturn);
    UaStatus connectInternal(const UaString& serverUrl, SessionSecurityInfo& sessionSecurityInfo);
    UaStatus findSecureEndpoint(SessionSecurityInfo& sessionSecurityInfo);
    UaStatus checkServerCertificateTrust(SessionSecurityInfo& sessionSecurityInfo);
    void printBrowseResults(const UaReferenceDescriptions& referenceDescriptions);
    void printCertificateData(const UaByteString& serverCertificate);
    int userAcceptCertificate();

private:
    UaSession*              m_pSession;
    SampleSubscription*     m_pSampleSubscription;
    Configuration*          m_pConfiguration;
    UaClient::ServerStatus  m_serverStatus;
};


#endif // SAMPLECLIENT_H

