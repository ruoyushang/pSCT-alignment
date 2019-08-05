#ifndef PASCLIENT_H
#define PASCLIENT_H

#include <memory>
#include <string>

#include "uabase/uabase.h"

#include "uaclient/uaclientsdk.h"
#include "uaclient/uasession.h"

#include "client/utilities/database.hpp"
#include "client/utilities/subscription.hpp"
#include "client/utilities/callback.hpp"

class Configuration;
class PasNodeManager;

class Client : public callback
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
    void addDevices(const OpcUa_ReferenceDescription& referenceDescription);

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
/*
//private

class CallMethodAsyncData {
public:
    ServiceContext Context;
public:
    std::shared_ptr<UaVariantArray> InputArguments;
public:
    MethodHandle OperationHandle;
public:
    CallCompleteEventHandler Callback;
public:
    object CallbackData;
};

//private

void OnAsyncMethodCompletes(object state) {
    CallMethodAsyncData data = (CallMethodAsyncData) state;
    ((CallCompleteEventHandler) data.Callback)(data.OperationHandle, data.CallbackData, CallMethodAsync(data), false);
}

//private

CallMethodResult CallMethodAsync(CallMethodAsyncData data) {
    CallMethodResult result = new CallMethodResult();
    Argument[]
    arguments = Server.InternalClient.ReadValue<Argument[]>(
            data.Context,
            data.OperationHandle.MethodHandle.MethodId,
            UnifiedAutomation.UaBase.BrowseNames.OutputArguments,
            null);
    VariantCollection outputArguments = null;
    if (arguments.Length > 0) {
        outputArguments = new VariantCollection(arguments.Length);
        for (int ii = 0; ii < arguments.Length; ii++) {
            outputArguments.Add(new Variant());
        }
    }
    StatusCode status = data.OperationHandle.MethodHandle.Dispatcher(
            data.Context,
            data.OperationHandle.MethodHandle,
            data.InputArguments,
            result.InputArgumentResults,
            outputArguments);
    if (status.IsGood()) {
        result.OutputArguments = outputArguments;
    }
    result.StatusCode = status;
    return result;
}

//public

override void FinishCallTransaction(TransactionHandle transaction) {
    CallTransactionHandle
    transaction2 = transaction
    as CallTransactionHandle;
    for (int ii = 0; ii < transaction2.MethodHandles.Count; ii++) {
        MethodOperationHandle methodHandle = transaction2.MethodHandles[ii];
        // check for long running methods which are processed
        // asychnonously.

        if (methodHandle.MethodHandle.MethodDeclarationId.Equals(
                MethodIds.MyType_LongRunning.ToNodeId(Server.NamespaceUris))) {
            ThreadPool.QueueUserWorkItem(OnAsyncMethodCompletes, new CallMethodAsyncData()
            {
                Context = transaction.Context,
                InputArguments = transaction2.InputArguments[ii],
                Callback = (CallCompleteEventHandler) transaction.Callback,
                CallbackData = transaction.CallbackData,
                OperationHandle = transaction2.MethodHandles[ii],
            });
            continue;
        }
        // call normal methods synchronously.
        CallMethodResult result = CallMethod(
                transaction.Context,
                transaction2.MethodHandles[ii].MethodHandle,
                transaction2.InputArguments[ii]);
        ((CallCompleteEventHandler) transaction.Callback)(
                transaction2.MethodHandles[ii],
                transaction.CallbackData, result, false);
    }
}

//public

override void FinishCallTransaction(TransactionHandle transaction) {
    CallTransactionHandle
    transaction2 = transaction
    as CallTransactionHandle;
    for (int ii = 0; ii < transaction2.MethodHandles.Count; ii++) {
        MethodOperationHandle methodHandle = transaction2.MethodHandles[ii];
        ThreadPool.QueueUserWorkItem(
                OnAsyncMethodCompletes, new CallMethodAsyncData()
        {
            Context = transaction.Context,
            InputArguments = transaction2.InputArguments[ii],
            Callback = (CallCompleteEventHandler) transaction.Callback,
            CallbackData = transaction.CallbackData,
            OperationHandle = transaction2.MethodHandles[ii],
        });
    }
}

//The following source code shows an example how to add all methods to the ThreadPool:
//private

//class CallMethodAsyncData {
//public
//    ServiceContext Context;
//public
//    IList <Variant> InputArguments;
//public
//    MethodOperationHandle OperationHandle;
//public
//    CallCompleteEventHandler Callback;
//public
//    object CallbackData;
//}

//private

void OnAsyncMethodCompletes(object state) {
    CallMethodAsyncData data = (CallMethodAsyncData) state;
    CallMethodResult result = CallMethod(data.Context, data.OperationHandle.MethodHandle, data.InputArguments);
    ((CallCompleteEventHandler) data.Callback)(data.OperationHandle, data.CallbackData, result, false);
};*/
#endif
