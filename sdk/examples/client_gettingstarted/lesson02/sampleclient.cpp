#include "sampleclient.h"
#include "uasession.h"
#include "samplesubscription.h"

SampleClient::SampleClient()
{
    m_pSession = new UaSession();
    m_pSampleSubscription = new SampleSubscription();
}

SampleClient::~SampleClient()
{
    if (m_pSampleSubscription)
    {
        // delete local subscription object
        delete m_pSampleSubscription;
        m_pSampleSubscription = NULL;
    }

    if (m_pSession)
    {
        // disconnect if we're still connected
        if (m_pSession->isConnected() != OpcUa_False)
        {
            ServiceSettings serviceSettings;
            m_pSession->disconnect(serviceSettings, OpcUa_True);
        }
        delete m_pSession;
        m_pSession = NULL;
    }
}

void SampleClient::connectionStatusChanged(
    OpcUa_UInt32             clientConnectionId,
    UaClient::ServerStatus   serverStatus)
{
    OpcUa_ReferenceParameter(clientConnectionId);

    printf("-------------------------------------------------------------\n");
    switch (serverStatus)
    {
    case UaClient::Disconnected:
        printf("Connection status changed to Disconnected\n");
        break;
    case UaClient::Connected:
        printf("Connection status changed to Connected\n");
        break;
    case UaClient::ConnectionWarningWatchdogTimeout:
        printf("Connection status changed to ConnectionWarningWatchdogTimeout\n");
        break;
    case UaClient::ConnectionErrorApiReconnect:
        printf("Connection status changed to ConnectionErrorApiReconnect\n");
        break;
    case UaClient::ServerShutdown:
        printf("Connection status changed to ServerShutdown\n");
        break;
    case UaClient::NewSessionCreated:
        printf("Connection status changed to NewSessionCreated\n");
        break;
    }
    printf("-------------------------------------------------------------\n");
}

UaStatus SampleClient::connect()
{
    UaStatus result;

    // For now we use a hardcoded URL to connect to the local DemoServer
    UaString sURL("opc.tcp://localhost:48010");

    // Provide information about the client
    SessionConnectInfo sessionConnectInfo;
    UaString sNodeName("unknown_host");
    char szHostName[256];
    if (0 == UA_GetHostname(szHostName, 256))
    {
        sNodeName = szHostName;
    }
    sessionConnectInfo.sApplicationName = "Unified Automation Getting Started Client";
    // Use the host name to generate a unique application URI
    sessionConnectInfo.sApplicationUri  = UaString("urn:%1:UnifiedAutomation:GettingStartedClient").arg(sNodeName);
    sessionConnectInfo.sProductUri      = "urn:UnifiedAutomation:GettingStartedClient";
    sessionConnectInfo.sSessionName     = sessionConnectInfo.sApplicationUri;

    // Security settings are not initialized - we connect without security for now
    SessionSecurityInfo sessionSecurityInfo;

    printf("\nConnecting to %s\n", sURL.toUtf8());
    result = m_pSession->connect(
        sURL,
        sessionConnectInfo,
        sessionSecurityInfo,
        this);

    if (result.isGood())
    {
        printf("Connect succeeded\n");
    }
    else
    {
        printf("Connect failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus SampleClient::disconnect()
{
    UaStatus result;

    // Default settings like timeout
    ServiceSettings serviceSettings;

    printf("\nDisconnecting ...\n");
    result = m_pSession->disconnect(
        serviceSettings,
        OpcUa_True);

    if (result.isGood())
    {
        printf("Disconnect succeeded\n");
    }
    else
    {
        printf("Disconnect failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus SampleClient::read()
{
    UaStatus          result;
    ServiceSettings   serviceSettings;
    UaReadValueIds    nodeToRead;
    UaDataValues      values;
    UaDiagnosticInfos diagnosticInfos;

    // Configure one node to read
    // We read the value of the ServerStatus -> CurrentTime
    nodeToRead.create(1);
    nodeToRead[0].AttributeId = OpcUa_Attributes_Value;
    nodeToRead[0].NodeId.Identifier.Numeric = OpcUaId_Server_ServerStatus_CurrentTime;

    printf("\nReading ...\n");
    result = m_pSession->read(
        serviceSettings,
        0,
        OpcUa_TimestampsToReturn_Both,
        nodeToRead,
        values,
        diagnosticInfos);

    if (result.isGood())
    {
        // Read service succeded - check status of read value
        if (OpcUa_IsGood(values[0].StatusCode))
        {
            printf("ServerStatusCurrentTime: %s\n", UaVariant(values[0].Value).toString().toUtf8());
        }
        else
        {
            printf("Read failed for item[0] with status %s\n", UaStatus(values[0].StatusCode).toString().toUtf8());
        }
    }
    else
    {
        // Service call failed
        printf("Read failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus SampleClient::subscribe()
{
    UaStatus result;

    result = m_pSampleSubscription->createSubscription(m_pSession);
    if ( result.isGood() )
    {
        result = m_pSampleSubscription->createMonitoredItems();
    }
    return result;
}

UaStatus SampleClient::unsubscribe()
{
    return m_pSampleSubscription->deleteSubscription();
}
