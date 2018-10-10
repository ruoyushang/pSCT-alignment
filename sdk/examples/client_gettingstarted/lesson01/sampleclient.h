#ifndef SAMPLECLIENT_H
#define SAMPLECLIENT_H

#include "uabase.h"
#include "uaclientsdk.h"

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

private:
    UaSession* m_pSession;
};

#endif // SAMPLECLIENT_H

