#ifndef PASCLIENT_H
#define PASCLIENT_H

#include <memory>
#include <string>

#include "uabase/uabase.h"

#include "uaclient/uaclientsdk.h"
#include "uaclient/uasession.h"

#include "client/utilities/database.hpp"
#include "client/utilities/subscription.hpp"

class Configuration;
class PasNodeManager;

class Client : public UaClientSdk::UaSessionCallback
{
    UA_DISABLE_COPY(Client);
public:
    explicit Client(PasNodeManager *pNodeManager, std::string mode = "subclient");

    ~Client() override = default;

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
    UaStatus readAsync(std::vector<std::string> sNodeNames, UaVariant *data);
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

    std::string getDeviceNodeId(const Device::Identity &identity) { return m_DeviceNodeIdMap.at(identity); }

private:
    std::string m_mode;

    UaString m_Address;
    // helper methods
    UaStatus _connect(const UaString& serverUrl, UaClientSdk::SessionSecurityInfo& sessionSecurityInfo);
    UaStatus recurseAddressSpace(const UaNodeId& nodeToBrowse, OpcUa_UInt32 maxReferencesToReturn);

    static void printBrowseResults(const OpcUa_ReferenceDescription &referenceDescription);
    void addDevices(const OpcUa_ReferenceDescription& eferenceDescription);

    // variables
    std::shared_ptr<PasNodeManager> m_pNodeManager;
    std::unique_ptr<UaClientSdk::UaSession> m_pSession;
    std::unique_ptr<Subscription> m_pSubscription;
    std::shared_ptr<Configuration> m_pConfiguration;
    UaClientSdk::UaClient::ServerStatus m_serverStatus;
    std::unique_ptr<Database> m_pDatabase;

    // keep track of asynchronous calls
    OpcUa_UInt32 m_TransactionId;

    std::map<Device::Identity, std::string> m_DeviceNodeIdMap;
};

#endif
