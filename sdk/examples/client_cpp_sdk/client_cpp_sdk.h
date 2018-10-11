/* system */
#include <stdio.h>
#ifndef _WIN32_WCE
#ifdef WIN32
    #include <conio.h>
    #include <crtdbg.h>
#else
    #include <signal.h>
    volatile sig_atomic_t signal_received = 0;

    /* Signal handler for CTRL-C. */
    void SigInt(int)
    {
        signal_received = 1;
        return;
    }
#endif
#endif


/* C++ UA Client API */
#include "uaclientsdk.h"
using namespace UaClientSdk;
#include "uaplatformlayer.h"
#include "uadatavalue.h"
#include "uadiscovery.h"

/* low level and stack */
#include "opcua_core.h"

#include "callback.h"
class ClientSampleConfig;

OpcUa_PKIProvider                       g_PkiProvider;
OpcUa_CertificateStoreConfiguration     g_pkiConfigClient;
Callback*                               g_pCallback;
UaObjectArray<UaNodeId>                 g_VariableNodeIds;
UaObjectArray<UaNodeId>                 g_WriteVariableNodeIds;
UaObjectArray<UaNodeId>                 g_ObjectNodeIds;
UaObjectArray<UaNodeId>                 g_MethodNodeIds;
UaObjectArray<UaNodeId>                 g_HistoryDataNodeIds;
UaNodeId                                g_HistoryEventNodeId;
UaObjectArray<UaNodeId>                 g_EventTriggerObjects;
UaObjectArray<UaNodeId>                 g_EventTriggerMethods;
UaByteStringArray                       g_EventIds;
OpcUa_UInt16                            g_nsIndex;
OpcUa_UInt16                            g_nsIndex2;
ClientSampleConfig*                     g_pClientSampleConfig = NULL;

void connect(UaString& sUrl, SessionSecurityInfo& sessionSecurityInfo);
void disconnect();

void browse();
void read();
void write();
void readAsync();
void writeAsync();
void registerNodes();
void unregisterNodes();
void subscribe();
void subscribeAlarms();
void callMethod();
void translate();
void transferSubscription(UaString& sUrl, SessionSecurityInfo& sessionSecurityInfo);
void historyReadDataRaw();
void historyReadDataProcessed();
void historyReadDataAtTime();
void historyUpdateData();
void historyDeleteData();
void historyReadEvents();
void historyUpdateEvents();
void historyDeleteEvents();

void startDiscovery(const UaString& sDiscoveryUrl, SessionSecurityInfo& sessionSecurityInfo, UaString &sUrl);
void gdsInteraction(SessionSecurityInfo &sessionSecurityInfo);
void buildNodeIds();

void mainMenu();
void printMenu(int level);
