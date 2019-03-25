#include "clienthelper.h"
#include "subscription.h"
#include "database.h"
#include "configuration.h"
#include "pasobject.h"
#include "passervertypeids.h"
#include "pasnodemanager.h"
#include "pascommunicationinterface.h"
#include "components.h"
#include "uaclient/uasession.h"

#include <map>

#define _DEBUG_ 0

using namespace UaClientSdk;

Client::Client(PasNodeManager *pNodeManager) : m_pNodeManager(pNodeManager), m_TransactionId(0)
{
    m_pSession = new UaSession();
    m_pSubscription = new Subscription(m_pConfiguration);
    m_pDatabase = new Database();
}

Client::~Client()
{
    m_pNodeManager = NULL;
    m_pConfiguration = NULL;

    if (m_pSubscription)
    {
        // delete local subscription object
        delete m_pSubscription;
        m_pSubscription = NULL;
    }

    if (m_pDatabase)
    {
        // delete local database object
        delete m_pDatabase;
        m_pDatabase = NULL;
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

void Client::connectionStatusChanged(
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
          if (m_serverStatus != UaClient::NewSessionCreated)
          {
            m_pConfiguration->updateNamespaceIndexes(m_pSession->getNamespaceTable());
          }
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
          m_pConfiguration->updateNamespaceIndexes(m_pSession->getNamespaceTable());
          break; 
    }
    printf("-------------------------------------------------------------\n");

    m_serverStatus = serverStatus; 
}

void Client::setConfiguration(Configuration* pConfiguration) 
{ 
    if (m_pSubscription) 
    {
        m_pSubscription->setConfiguration(pConfiguration); 
    }
    if (m_pDatabase)
    {
        m_pDatabase->setConfiguration(pConfiguration);
    }
    m_pConfiguration = pConfiguration;
}

UaStatus Client::connect()
{
    // Security settings are not initialized - we connect without security for now
    SessionSecurityInfo sessionSecurityInfo;

    return _connect(m_Address, sessionSecurityInfo);
}

UaStatus Client::_connect(const UaString& serverUrl, SessionSecurityInfo& sessionSecurityInfo)
{
    UaStatus result;

    // Provide information about the client
    SessionConnectInfo sessionConnectInfo;
    UaString sNodeName("unknown_host");
    char szHostName[256];
    if (0 == UA_GetHostname(szHostName, 256))
    {
        sNodeName = szHostName;
    }

    // Fill session connect info with data from configuration
    sessionConnectInfo.sApplicationName = m_pConfiguration->getApplicationName();
    // Use the host name to generate a unique application URI
    sessionConnectInfo.sApplicationUri = UaString("urn:%1:%2:%3").arg(sNodeName).arg(COMPANY_NAME).arg(PRODUCT_NAME);
    sessionConnectInfo.sProductUri = UaString("urn:%1:%2").arg(COMPANY_NAME).arg(PRODUCT_NAME);
    sessionConnectInfo.sSessionName = sessionConnectInfo.sApplicationUri;
    sessionConnectInfo.bAutomaticReconnect = m_pConfiguration->getAutomaticReconnect();
    sessionConnectInfo.bRetryInitialConnect = m_pConfiguration->getRetryInitialConnect();

    // Connect to server
    printf("\nConnecting to %s\n", serverUrl.toUtf8());
    result = m_pSession->connect(serverUrl, // Url of the endpoint to connect to
            sessionConnectInfo, // General settings for the connection
            sessionSecurityInfo,// Security Settings
            this);              // the callback -- the client ingerits from UaSessionCallback!

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

UaStatus Client::disconnect()
{
    if (!m_pSession->isConnected())
        return OpcUa_Good;

    UaStatus result;

    // Default settings like timeout
    ServiceSettings serviceSettings;

    printf("\nDisconnecting ...\n");
    result = m_pSession->disconnect(serviceSettings, OpcUa_True);

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

UaStatus Client::read(std::vector<std::string> sNodeNames, UaVariant *data)
{
    UaStatus          result;

    UaDataValues      values;
    ServiceSettings   serviceSettings;
    UaReadValueIds    nodesToRead;
    UaDiagnosticInfos diagnosticInfos;

    OpcUa_UInt32 size = sNodeNames.size();

    UaNodeIdArray nodes;
    nodes.clear();
    nodes.resize(size);

    nodesToRead.create(nodes.length());

    if (_DEBUG_)
        printf("will attempt to read:\n");
    for (OpcUa_UInt32 i = 0; i < size; i++) {
        if (_DEBUG_)
            printf("%s\n", sNodeNames.at(i).c_str());

        UaNodeId::fromXmlString(UaString(sNodeNames.at(i).c_str())).copyTo(&nodes[i]);

        nodesToRead[i].AttributeId = OpcUa_Attributes_Value;
        OpcUa_NodeId_CopyTo(&nodes[i], &nodesToRead[i].NodeId);
    }

    serviceSettings.callTimeout = 2000; // 2000 ms
    result = m_pSession->read(serviceSettings, 0, OpcUa_TimestampsToReturn_Both,
                                                   nodesToRead, values, diagnosticInfos);

    if (result.isGood()) {
        // Read service succeeded - check individual status codes
        for (OpcUa_UInt32 i = 0; i < values.length(); i++) {
            if (OpcUa_IsGood(values[i].StatusCode)) {
                data[i] = UaVariant(values[i].Value);
            }
            else {
                printf("Read failed for item[%d] with status %s\n", i, UaStatus(values[i].StatusCode).toString().toUtf8());
            }
        }
    }
    else {
        // Service call failed
        printf("Read failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus Client::write(std::vector<std::string> sNodeNames, const UaVariant *values)
{
    UaStatus          result;
    UaWriteValues     nodesToWrite;
    UaStatusCodeArray results;
    UaDiagnosticInfos diagnosticInfos;
    ServiceSettings   serviceSettings;
    UaNodeIdArray nodes;
    
    OpcUa_UInt32 size = sNodeNames.size();
    nodes.clear();
    nodes.resize(size);

    nodesToWrite.create(nodes.length());

    if (_DEBUG_)
        printf("will attempt to write:\n");
    for (OpcUa_UInt32 i = 0; i < nodes.length(); i++) {
        if (_DEBUG_)
            printf("%s\n", sNodeNames.at(i).c_str());

        UaNodeId::fromXmlString(UaString(sNodeNames.at(i).c_str())).copyTo(&nodes[i]);

        nodesToWrite[i].AttributeId = OpcUa_Attributes_Value;
        OpcUa_NodeId_CopyTo(&nodes[i], &nodesToWrite[i].NodeId);
        values[i].copyTo(&nodesToWrite[i].Value.Value);
    }

    result = m_pSession->write(serviceSettings, nodesToWrite, results, diagnosticInfos);

    if ( result.isBad() )
    {
        printf("** Error: UaSession::write failed [ret=%s] **\n", result.toString().toUtf8());
    }
    else
    {
        for (OpcUa_UInt32 i=0; i<size; i++ )
        {
            UaNodeId node(nodesToWrite[i].NodeId);
            if ( OpcUa_IsGood(results[i]) )
            {
                if (_DEBUG_)
                    printf("** Variable %s succeeded!\n", node.toString().toUtf8());
            }
            else
            {
                printf("** Variable %s failed with error %s\n", node.toString().toUtf8(), UaStatus(results[i]).toString().toUtf8());
            }
        }
    }
    return result;
}


UaStatus Client::callMethod(std::string sNodeName, UaString sMethod)
{
    UaStatus          status;
    CallIn            callRequest;
    CallOut           callResult;
    ServiceSettings   serviceSettings;
    UaNodeIdArray nodes;

    OpcUa_UInt32 size = 1;
    nodes.clear();
    nodes.resize(2*size);

    UaStringArray sName;
    sName.clear();
    sName.resize(2*size);

    char name[100];
    sprintf(name, "%s", sNodeName.c_str());
    UaString(name).copyTo(&sName[0]);
    sprintf(name, "%s.%s", sNodeName.c_str(), sMethod.toUtf8());
    UaString(name).copyTo(&sName[1]);

    for (OpcUa_UInt32 i = 0; i < nodes.length(); i++)
        UaNodeId::fromXmlString(sName[i]).copyTo(&nodes[i]);

    callRequest.objectId = nodes[0];
    callRequest.methodId = nodes[1];

    serviceSettings.callTimeout = 2000*60; // call timeout in ms; set to 2 minutes
    status = m_pSession->call(serviceSettings, callRequest, callResult);

    if ( status.isBad() )
        printf("** Error: UaSession::call failed [ret=%s] **\n", status.toString().toUtf8());
    else {
        if(_DEBUG_)
            printf("** UaSession::call suceeded!\n");
    }

    return status;
}

UaStatus Client::callMethodAsync(std::string sNodeName, UaString sMethod)
{
    UaStatus          status;
    CallIn            callRequest;
    ServiceSettings   serviceSettings;
    UaNodeIdArray nodes;

    OpcUa_UInt32 size = 1;
    nodes.clear();
    nodes.resize(2*size);

    UaStringArray sName;
    sName.clear();
    sName.resize(2*size);

    char name[100];
    sprintf(name, "%s", sNodeName.c_str());
    UaString(name).copyTo(&sName[0]);
    sprintf(name, "%s.%s", sNodeName.c_str(), sMethod.toUtf8());
    UaString(name).copyTo(&sName[1]);

    for (OpcUa_UInt32 i = 0; i < nodes.length(); i++)
        UaNodeId::fromXmlString(sName[i]).copyTo(&nodes[i]);

    callRequest.objectId = nodes[0];
    callRequest.methodId = nodes[1];

    serviceSettings.callTimeout = 0.; // call timeout in ms; set to 0 -- never times out
    status = m_pSession->beginCall(serviceSettings, callRequest, m_TransactionId);

    if ( status.isBad() )
        printf("** Error: Client at %s: UaSession::beginCall with transactionId=%d failed [ret=%s] **\n", m_Address.toUtf8(), m_TransactionId, status.toString().toUtf8());
    else {
        if(_DEBUG_)
            printf("** Client at %s: UaSession::beginCall with transactionId=%d suceeded!\n", m_Address.toUtf8(), m_TransactionId);

        ++m_TransactionId;
    }

    return status;
}

void Client::callComplete(OpcUa_UInt32 transactionId, const UaStatus &result, const CallOut &callResponse)
{
        printf("-- Event callComplete --------------------------------\n");
        printf("\ttransactionId %d \n", transactionId);
        printf("\tresultStatus %s \n", result.toString().toUtf8());
        // this should duplicate the behavior of the synchronous callMethod() after the method
        // call succeeds. But we're not doing anything there for now, so this too is empty
}

UaStatus Client::browseAndAddDevices()
{
    UaStatus ret;
    UaNodeId nodeToBrowse;

    // browse from root folder with no limitation of references to return
    nodeToBrowse = UaNodeId(OpcUaId_RootFolder);
    ret = recurseAddressSpace(nodeToBrowse, 0);

    return ret;
}

UaStatus Client::recurseAddressSpace(const UaNodeId& nodeToBrowse, OpcUa_UInt32 maxReferencesToReturn)
{
    UaStatus result;

    ServiceSettings serviceSettings;
    UaClientSdk::BrowseContext browseContext;
    UaByteString continuationPoint;
    UaReferenceDescriptions referenceDescriptions;

    // configure browseContext
    browseContext.browseDirection = OpcUa_BrowseDirection_Forward;
    browseContext.referenceTypeId = OpcUaId_HierarchicalReferences;
    browseContext.includeSubtype = OpcUa_True;
    browseContext.maxReferencesToReturn = maxReferencesToReturn;

    //printf("\nBrowsing from Node %s...\n", nodeToBrowse.toXmlString().toUtf8());
    result = m_pSession->browse(serviceSettings, nodeToBrowse, browseContext,
            continuationPoint, referenceDescriptions);

    if (result.isGood())
    {
        OpcUa_UInt32 i;
        for (i = 0; i<referenceDescriptions.length(); i++)
        {

            // Print and add nodes if necessary
            addDevices(referenceDescriptions[i]);
            recurseAddressSpace(referenceDescriptions[i].NodeId.NodeId, maxReferencesToReturn);
        }
        
        // continue browsing
        while (continuationPoint.length() > 0)
        {
            printf("\nContinuationPoint is set. BrowseNext...\n");
            // browse next
            result = m_pSession->browseNext(serviceSettings, OpcUa_False, continuationPoint, referenceDescriptions);

            if (result.isGood())
            {
                // Print and add nodes if necessary
                addDevices(referenceDescriptions[i]);
                recurseAddressSpace(referenceDescriptions[i].NodeId.NodeId, maxReferencesToReturn);
            }
            else
            {
                // Service call failed
                printf("BrowseNext failed with status %s\n", result.toString().toUtf8());
            }
        }
    }
    else
    {
        // Service call failed
        printf("Browse failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

void Client::addDevices(const OpcUa_ReferenceDescription& referenceDescription)
{
    UaStatus ret;
    Identity identity;
    char sTemp[100];

    std::map<OpcUa_UInt32, std::string> typeNamesMap = {{PAS_MPESType, "MPES"}, {PAS_ACTType, "ACT"}, {PAS_PSDType, "PSD"}};

    OpcUa_UInt32 type;
    std::string name;
    for (const auto& it_typeNameMap : typeNamesMap) {
        type = it_typeNameMap.first;
        name = it_typeNameMap.second;
        // if (TYPE) and (in a folder) (as opposed to, say notification area)
        //if (referenceDescription.TypeDefinition.NodeId.Identifier.Numeric == type
        //        && referenceDescription.ReferenceTypeId.Identifier.Numeric == OpcUaId_Organizes) 
        if (referenceDescription.TypeDefinition.NodeId.Identifier.Numeric == type)
        {
            // get the node name of the object we're connecting to!
            sprintf(sTemp, "ns=%d;s=%s",
                    referenceDescription.BrowseName.NamespaceIndex,
                    UaString(referenceDescription.BrowseName.Name).toUtf8());
            // get the USB number -- this is the last character of the node name
            std::string sUSB = std::string(UaString(sTemp).toUtf8()).substr(strlen(sTemp)-1, 1);
            // set the identity of the device we're about to add
            identity.eAddress = std::string(sTemp);
            identity.serialNumber = m_pConfiguration->getDeviceSerial(m_Address, type, UaString(sUSB.c_str()));
            sprintf(sTemp, "%s_%03d", name.c_str(), identity.serialNumber);
            identity.name = std::string(sTemp);
            identity.position = m_pConfiguration->getDevicePosition(type, identity.serialNumber);

            printf("=====================================\n");
            printBrowseResults(referenceDescription);
            printf("will add %s %d as %s\n", name.c_str(), identity.serialNumber, identity.eAddress.c_str());
            ((PasCommunicationInterface*)m_pNodeManager->getComInterface().get())->addDevice(this, type, identity);
        }
    }
}

void Client::printBrowseResults(const OpcUa_ReferenceDescription& referenceDescription)
{
    printf("node: ");
    UaNodeId referenceTypeId(referenceDescription.ReferenceTypeId);
    printf("[Ref=%s] ", referenceTypeId.toString().toUtf8() );
    UaQualifiedName browseName(referenceDescription.BrowseName);
    printf("%s ( ", browseName.toString().toUtf8() );
    if (referenceDescription.NodeClass & OpcUa_NodeClass_Object) 
        printf("Object ");
    if (referenceDescription.NodeClass & OpcUa_NodeClass_Variable) 
        printf("Variable ");
    if (referenceDescription.NodeClass & OpcUa_NodeClass_Method) 
        printf("Method ");
    if (referenceDescription.NodeClass & OpcUa_NodeClass_ObjectType) 
        printf("ObjectType ");
    if (referenceDescription.NodeClass & OpcUa_NodeClass_VariableType) 
        printf("VariableType ");
    if (referenceDescription.NodeClass & OpcUa_NodeClass_ReferenceType) 
        printf("ReferenceType ");
    if (referenceDescription.NodeClass & OpcUa_NodeClass_DataType)
        printf("DataType ");
    if (referenceDescription.NodeClass & OpcUa_NodeClass_View) 
        printf("View ");
    
    UaNodeId nodeId(referenceDescription.NodeId.NodeId);
    printf("[NodeId=%s] ", nodeId.toFullString().toUtf8() );
    printf(")\n");
}


UaStatus Client::subscribe()
{
    UaStatus result;

    result = m_pSubscription->createSubscription(m_pSession);
    if ( result.isGood() )
    {
        result = m_pSubscription->createMonitoredItems();
    }
    return result;
}

UaStatus Client::unsubscribe()
{
    return m_pSubscription->deleteSubscription();
}

void Client::connectDatabase()
{
    m_pDatabase->connectAndPrepare();
}
