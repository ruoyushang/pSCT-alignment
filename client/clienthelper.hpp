#ifndef PASCLIENT_H
#define PASCLIENT_H

#include "uabase.h"
#include "uaclient/uaclientsdk.h"
#include <memory>
#include <string>


class Subscription;
class Configuration;
class Database;
class PasNodeManager;

class Client : public std::enable_shared_from_this<Client>, public UaClientSdk::UaSessionCallback
{
    UA_DISABLE_COPY(Client);
public:
    explicit Client(PasNodeManager *pNodeManager);

    ~Client() override;

    // UaSessionCallback implementation ----------------------------------------------------
    void
    connectionStatusChanged(OpcUa_UInt32 clientConnectionId, UaClientSdk::UaClient::ServerStatus serverStatus) override;
    // UaSessionCallback implementation ------------------------------------------------------

    // set a configuration object we use to get connection parameters and NodeIds
    void setConfiguration(std::shared_ptr<Configuration> pConfiguration);
    void setAddress(const UaString& address) { m_Address = address; };

    // OPC UA service calls
    UaStatus connect();
    UaStatus disconnect();
    UaStatus browseAndAddDevices();

    UaStatus read(std::vector<std::string> sNodeNames, UaVariant *data);
    UaStatus write(std::vector<std::string> sNodeName, const UaVariant *values);

    // synchronous call
    UaStatus
    callMethod(const std::string &sNodeName, const UaString &sMethod, const UaVariantArray &args = UaVariantArray());
    // asynchronous call -- needs a callComplete for the callback as welcl
    UaStatus callMethodAsync(const std::string &sNodeName, const UaString &sMethod,
                             const UaVariantArray &args = UaVariantArray());
    // UaSessionCallback implementation
    void
    callComplete(OpcUa_UInt32 transactionId, const UaStatus &result, const UaClientSdk::CallOut &callResponse) override;

    UaStatus subscribe();
    UaStatus unsubscribe();
    void connectDatabase();

private:
    std::shared_ptr<Client> get_this_shared() {
        return shared_from_this();
    }

    UaString m_Address;
    // helper methods
    UaStatus _connect(const UaString& serverUrl, UaClientSdk::SessionSecurityInfo& sessionSecurityInfo);
    UaStatus recurseAddressSpace(const UaNodeId& nodeToBrowse, OpcUa_UInt32 maxReferencesToReturn);

    static void printBrowseResults(const OpcUa_ReferenceDescription &referenceDescription);
    void addDevices(const OpcUa_ReferenceDescription& eferenceDescription);

    // variables
    PasNodeManager*                     m_pNodeManager;
    UaClientSdk::UaSession*             m_pSession;
    Subscription*                       m_pSubscription;
    std::shared_ptr<Configuration> m_pConfiguration;
    UaClientSdk::UaClient::ServerStatus m_serverStatus;
    Database*                           m_pDatabase;

    // keep track of asynchronous calls
    OpcUa_UInt32 m_TransactionId;
};

#endif
