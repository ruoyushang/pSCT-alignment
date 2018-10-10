#ifndef SAMPLECLIENT_H
#define SAMPLECLIENT_H

#include "uabase.h"
#include "uaclientsdk.h"

class SampleSubscription;

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

    // OPC UA service calls
    UaStatus connect();
    UaStatus disconnect();
    UaStatus read();
    UaStatus subscribe();
    UaStatus unsubscribe();

private:
    UaSession*          m_pSession;
    SampleSubscription* m_pSampleSubscription;
};


#endif // SAMPLECLIENT_H

