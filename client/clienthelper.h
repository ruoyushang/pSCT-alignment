#ifndef PASCLIENT_H
#define PASCLIENT_H

#include "uabase.h"
#include "uaclient/uaclientsdk.h"
#include <string>

class Subscription;
class Configuration;
class Database;
class PasNodeManager;

class Client : public UaClientSdk::UaSessionCallback
{
    UA_DISABLE_COPY(Client);
public:
    Client(PasNodeManager *pNodeManager);
    virtual ~Client();

    // UaSessionCallback implementation ----------------------------------------------------
    virtual void connectionStatusChanged(OpcUa_UInt32 clientConnectionId, UaClientSdk::UaClient::ServerStatus serverStatus);
    // UaSessionCallback implementation ------------------------------------------------------

    // set a configuration object we use to get connection parameters and NodeIds
    void setConfiguration(Configuration* pConfiguration);
    void setAddress(const UaString& address) { m_Address = address; };

    // OPC UA service calls
    UaStatus connect();
    UaStatus disconnect();
    UaStatus browseAndAddDevices();

    UaStatus read(std::vector<std::string> sNodeNames, UaVariant *data);
    UaStatus write(std::vector<std::string> sNodeName, const UaVariant *values);

    // synchronous call
    UaStatus callMethod(std::string sNodeName, UaString sMethod);
    // asynchronous call -- needs a callComplete for the callback as welcl
    UaStatus callMethodAsync(std::string sNodeName, UaString sMethod);
    // UaSessionCallback implementation
    void callComplete(OpcUa_UInt32 transactionId, const UaStatus &result, const UaClientSdk::CallOut &callResponse);

    UaStatus subscribe();
    UaStatus unsubscribe();
    void connectDatabase();

private:
    UaString m_Address;
    // helper methods
    UaStatus _connect(const UaString& serverUrl, UaClientSdk::SessionSecurityInfo& sessionSecurityInfo);
    UaStatus recurseAddressSpace(const UaNodeId& nodeToBrowse, OpcUa_UInt32 maxReferencesToReturn);
    void printBrowseResults(const OpcUa_ReferenceDescription& referenceDescription);
    void addDevices(const OpcUa_ReferenceDescription& eferenceDescription);

    // variables
    PasNodeManager*                     m_pNodeManager;
    UaClientSdk::UaSession*             m_pSession;
    Subscription*                       m_pSubscription;
    Configuration*                      m_pConfiguration;
    UaClientSdk::UaClient::ServerStatus m_serverStatus;
    Database*                           m_pDatabase;

    // keep track of asynchronous calls
    OpcUa_UInt32 m_TransactionId;
};

#endif
