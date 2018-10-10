#include "sampleclient.h"
#include "uasession.h"
#include "samplesubscription.h"
#include "configuration.h"
#include "uadiscovery.h"
#include "uapkicertificate.h"
#include "uaplatformdefs.h"

SampleClient::SampleClient()
{
    m_pSession = new UaSession();
    m_pConfiguration = new Configuration();
    m_pSampleSubscription = new SampleSubscription(m_pConfiguration);
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

    if (m_pConfiguration)
    {
        delete m_pConfiguration;
        m_pConfiguration = NULL;
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
        // the registered nodes are no longer valid in the new session
        registerNodes();
        break;
    }
    printf("-------------------------------------------------------------\n");

    m_serverStatus = serverStatus;
}

void SampleClient::setConfiguration(Configuration* pConfiguration)
{
    if (m_pSampleSubscription)
    {
        m_pSampleSubscription->setConfiguration(pConfiguration);
    }
    if (m_pConfiguration)
    {
        delete m_pConfiguration;
    }
    m_pConfiguration = pConfiguration;
}

UaStatus SampleClient::discover()
{
    UaStatus result;
    UaDiscovery discovery;
    ServiceSettings serviceSettings;
    ClientSecurityInfo clientSecurityInfo;
    UaApplicationDescriptions applicationDescriptions;
    UaEndpointDescriptions endpointDescriptions;
    OpcUa_UInt32 i, j;
    OpcUa_Int32 k;
    UaString sTemp;

    // get a list of available servers
    printf("\nCall FindServers on Url %s\n", m_pConfiguration->getDiscoveryUrl().toUtf8());
    result = discovery.findServers(
        serviceSettings,
        m_pConfiguration->getDiscoveryUrl(),
        clientSecurityInfo,
        applicationDescriptions);

    if (result.isGood())
    {
        // print list of servers
        printf("\nFindServers succeeded\n");
        for (i = 0; i < applicationDescriptions.length(); i++)
        {
            printf("** Application [%d] **********************************************************\n", i);
            sTemp = &applicationDescriptions[i].ApplicationUri;
            printf(" ApplicationUri       %s\n", sTemp.toUtf8());
            sTemp = &applicationDescriptions[i].ApplicationName.Text;
            printf(" ApplicationName      %s\n", sTemp.toUtf8());
            for (k = 0; k < applicationDescriptions[i].NoOfDiscoveryUrls; k++)
            {
                UaString sDiscoveryUrl(applicationDescriptions[i].DiscoveryUrls[k]);
                printf("** DiscoveryUrl [%s] ***********************\n", sDiscoveryUrl.toUtf8());
                // for each server get a list of available endpoints
                result = discovery.getEndpoints(
                    serviceSettings,
                    sDiscoveryUrl,
                    clientSecurityInfo,
                    endpointDescriptions);

                if (result.isGood())
                {
                    // print list of Endpoints
                    for (j = 0; j < endpointDescriptions.length(); j++)
                    {
                        printf("** Endpoint[%d] ***********************************************\n", j);
                        sTemp = &endpointDescriptions[j].EndpointUrl;
                        printf(" Endpoint URL     %s\n", sTemp.toUtf8());
                        sTemp = &endpointDescriptions[j].SecurityPolicyUri;
                        printf(" Security Policy  %s\n", sTemp.toUtf8());
                        sTemp = "Invalid";
                        if ( endpointDescriptions[j].SecurityMode == OpcUa_MessageSecurityMode_None )
                        {
                            sTemp = "None";
                        }
                        if ( endpointDescriptions[j].SecurityMode == OpcUa_MessageSecurityMode_Sign )
                        {
                            sTemp = "Sign";
                        }
                        if ( endpointDescriptions[j].SecurityMode == OpcUa_MessageSecurityMode_SignAndEncrypt )
                        {
                            sTemp = "SignAndEncrypt";
                        }
                        printf(" Security Mode    %s\n", sTemp.toUtf8());
                        printf("**************************************************************\n");
                    }
                }
                else
                {
                    printf("GetEndpoints failed with status %s\n", result.toString().toUtf8());
                }
                printf("************************************************************************\n");
            }
            printf("******************************************************************************\n");
        }
    }
    else
    {
        printf("FindServers failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus SampleClient::connect()
{
    // Security settings are not initialized - we connect without security for now
    SessionSecurityInfo sessionSecurityInfo;

    return connectInternal(m_pConfiguration->getServerUrl(), sessionSecurityInfo);
}

UaStatus SampleClient::connectSecure()
{
    UaStatus result;
    SessionSecurityInfo sessionSecurityInfo;

    //************************************************
    // Step (1) - Load or create a client certificate
    //************************************************
    // Secure connections require a certificate for both the client and the server
    // setupSecurity loads the client certificate and initializes the certificate store
    result = m_pConfiguration->setupSecurity(sessionSecurityInfo);

    //************************************************
    // Step (2) - Find a secure Endpoint on the server
    //************************************************
    // This is normally done the first time the server is used
    // The settings ServerCertificate, SecurityPolicyUri and SecurityMode should be stored
    // with other connection information like the URL to be reused in later connects
    if (result.isGood())
    {
        result = findSecureEndpoint(sessionSecurityInfo);
    }

    //************************************************
    // Step (3) - Validate the server certificate
    //************************************************
    // This step is normally neccessary at first connect (like Step 2)
    // If the server certificate is in the trust list the check succeeds at connect
    if (result.isGood())
    {
        result = checkServerCertificateTrust(sessionSecurityInfo);
    }

    if (result.isGood())
    {
        result = connectInternal(m_pConfiguration->getServerUrl(), sessionSecurityInfo);

        if (result.isBad())
        {
            printf("********************************************************************************************\n");
            printf("Connect with security failed. Make sure the client certificate is in the servers trust list.\n");
            printf("********************************************************************************************\n");
        }
    }

    return result;
}

UaStatus SampleClient::connectInternal(const UaString& serverUrl, SessionSecurityInfo& sessionSecurityInfo)
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
    result = m_pSession->connect(
        serverUrl,
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

UaStatus SampleClient::writeInternal(const UaNodeIdArray& nodesToWrite, const UaVariantArray& valuesToWrite)
{
    UaStatus            result;
    ServiceSettings     serviceSettings;
    UaWriteValues       writeValues;
    UaStatusCodeArray   results;
    UaDiagnosticInfos   diagnosticInfos;

    // check in parameters
    if (nodesToWrite.length() != valuesToWrite.length())
    {
        return OpcUa_BadInvalidArgument;
    }

    // write all nodes from the configuration
    writeValues.create(nodesToWrite.length());

    for (OpcUa_UInt32 i = 0; i < writeValues.length(); i++)
    {
        writeValues[i].AttributeId = OpcUa_Attributes_Value;
        OpcUa_NodeId_CopyTo(&nodesToWrite[i], &writeValues[i].NodeId);
        // set value to write
        OpcUa_Variant_CopyTo(&valuesToWrite[i], &writeValues[i].Value.Value);
    }

    printf("\nWriting...\n");
    result = m_pSession->write(
        serviceSettings,
        writeValues,
        results,
        diagnosticInfos);

    if (result.isGood())
    {
        // Write service succeded - check individual status codes
        for (OpcUa_UInt32 i = 0; i < results.length(); i++)
        {
            if (OpcUa_IsGood(results[i]))
            {
                printf("Write succeeded for item[%d]\n", i);
            }
            else
            {
                printf("Write failed for item[%d] with status %s\n", i, UaStatus(results[i]).toString().toUtf8());
            }
        }
    }
    else
    {
        // Service call failed
        printf("Write failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus SampleClient::findSecureEndpoint(SessionSecurityInfo& sessionSecurityInfo)
{
    UaStatus result;
    ServiceSettings serviceSettings;
    ClientSecurityInfo clientSecurityInfo;
    UaEndpointDescriptions endpointDescriptions;
    UaDiscovery discovery;
    OpcUa_UInt32 bestSecurityIndex = 0;
    UaString sTemp;

    printf("\nTry to find secure Endpoint on: %s\n", m_pConfiguration->getServerUrl().toUtf8());
    result = discovery.getEndpoints(
        serviceSettings,
        m_pConfiguration->getServerUrl(),
        clientSecurityInfo,
        endpointDescriptions);

    if (result.isGood())
    {
        OpcUa_Byte securityLevel = 0;
        OpcUa_UInt32 i;

        // select the endpoint with the best security
        for (i = 0; i < endpointDescriptions.length(); i++)
        {
            if (endpointDescriptions[i].SecurityLevel > securityLevel)
            {
                bestSecurityIndex = i;
                securityLevel = endpointDescriptions[i].SecurityLevel;
            }
        }

        // check if the endpoint we found is a secure endpoint
        if (endpointDescriptions[bestSecurityIndex].SecurityMode < OpcUa_MessageSecurityMode_Sign)
        {
            printf("No secure endpoint available on server\n");
            result = OpcUa_BadSecurityConfig;
        }
        // print the secure endpoint we found
        else
        {
            printf("Endpoint with best security found:\n");
            sTemp = &endpointDescriptions[bestSecurityIndex].EndpointUrl;
            printf(" Endpoint URL     %s\n", sTemp.toUtf8());
            sTemp = &endpointDescriptions[bestSecurityIndex].SecurityPolicyUri;
            printf(" Security Policy  %s\n", sTemp.toUtf8());
            sTemp = "Invalid";
            if ( endpointDescriptions[bestSecurityIndex].SecurityMode == OpcUa_MessageSecurityMode_None )
            {
                sTemp = "None";
            }
            if ( endpointDescriptions[bestSecurityIndex].SecurityMode == OpcUa_MessageSecurityMode_Sign )
            {
                sTemp = "Sign";
            }
            if ( endpointDescriptions[bestSecurityIndex].SecurityMode == OpcUa_MessageSecurityMode_SignAndEncrypt )
            {
                sTemp = "SignAndEncrypt";
            }
            printf(" Security Mode    %s\n", sTemp.toUtf8());

            // The following setting should be stored together with the server configuration
            // set server certificate
            sessionSecurityInfo.serverCertificate = endpointDescriptions[bestSecurityIndex].ServerCertificate;
            // set security mode and security policy
            sessionSecurityInfo.sSecurityPolicy = endpointDescriptions[bestSecurityIndex].SecurityPolicyUri;
            sessionSecurityInfo.messageSecurityMode = endpointDescriptions[bestSecurityIndex].SecurityMode;
        }
    }
    else
    {
        printf("GetEndpoints failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus SampleClient::checkServerCertificateTrust(SessionSecurityInfo& sessionSecurityInfo)
{
    UaStatus result;

    // Handling validation errors requires the following steps:
    // 1. show certificate and ask the user to accept the certificate
    // 2. user can either reject, accept temporary or accept permanently
    // 3. when accepting permanently copy the server certificate to the clients trust list
    if (sessionSecurityInfo.verifyServerCertificate().isBad())
    {
        printf("\n");
        printf("\n");
        printf("-------------------------------------------------------\n");
        printf("- The following certificate is not trusted yet        -\n");
        printf("-------------------------------------------------------\n");
        printCertificateData(sessionSecurityInfo.serverCertificate);
        printf("\n");

        printf("'y' + Enter if you want to trust the certificate temporarily.\n");
        printf("'p' + Enter if you want to trust the certificate permanently an copy the server certificate into the client trust list.\n");
        printf("Enter if you don't want to trust the certificate.\n");

        int accept = userAcceptCertificate();

        if (accept == 1)
        {
            printf("Certificate was acceppted temporarily.\n");
            // we already validated the certificate above
            // skip validation during connect since we only accept temporarily
            sessionSecurityInfo.doServerCertificateVerify = OpcUa_False;
        }
        else if (accept == 2)
        {
            // copy the server certificate into the client trust list
            UaPkiCertificate cert = UaPkiCertificate::fromDER(sessionSecurityInfo.serverCertificate);
            UaString sThumbprint = cert.thumbPrint().toHex();
            result = sessionSecurityInfo.saveServerCertificate(sThumbprint);

            if (result.isGood())
            {
                printf("Certificate was accepted permanently.\n");
            }
            else
            {
                printf("Failed to accept certifcate permanently :%s\n", result.toString().toUtf8());
            }
            // We stored the certificate in the trust list - certificate can be validated during connect
            sessionSecurityInfo.doServerCertificateVerify = OpcUa_True;
        }
        else
        {
            printf("Certificate was rejected by user.\n");
            result = OpcUa_BadCertificateUntrusted;
        }
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

UaStatus SampleClient::browseSimple()
{
    UaStatus result;
    UaNodeId nodeToBrowse;

    // browse from root folder with no limitation of references to return
    nodeToBrowse = UaNodeId(OpcUaId_RootFolder);
    result = browseInternal(nodeToBrowse, 0);

    return result;
}

UaStatus SampleClient::browseContinuationPoint()
{
    UaStatus result;
    UaNodeId nodeToBrowse;

    // browse from Massfolder with max references to return set to 5
    nodeToBrowse = UaNodeId("Demo", 2);
    result = browseInternal(nodeToBrowse, 5);

    return result;
}

UaStatus SampleClient::read()
{
    UaStatus          result;
    ServiceSettings   serviceSettings;
    UaReadValueIds    nodesToRead;
    UaDataValues      values;
    UaDiagnosticInfos diagnosticInfos;

    // read all nodes from the configuration
    UaNodeIdArray nodes = m_pConfiguration->getNodesToRead();
    nodesToRead.create(nodes.length());

    for (OpcUa_UInt32 i = 0; i < nodes.length(); i++)
    {
        nodesToRead[i].AttributeId = OpcUa_Attributes_Value;
        OpcUa_NodeId_CopyTo(&nodes[i], &nodesToRead[i].NodeId);
    }

    printf("\nReading ...\n");
    result = m_pSession->read(
        serviceSettings,
        0,
        OpcUa_TimestampsToReturn_Both,
        nodesToRead,
        values,
        diagnosticInfos);

    if (result.isGood())
    {
        // Read service succeded - check individual status codes
        for (OpcUa_UInt32 i = 0; i < nodes.length(); i++)
        {
            if (OpcUa_IsGood(values[i].StatusCode))
            {
                printf("Value[%d]: %s\n", i, UaVariant(values[i].Value).toString().toUtf8());
            }
            else
            {
                printf("Read failed for item[%d] with status %s\n", i, UaStatus(values[i].StatusCode).toString().toUtf8());
            }
        }
    }
    else
    {
        // Service call failed
        printf("Read failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus SampleClient::write()
{
    UaStatus result;

    // write all nodes from the configuration
    result = writeInternal(
        m_pConfiguration->getNodesToWrite(),
        m_pConfiguration->getWriteValues());

    return result;
}

UaStatus SampleClient::writeRegistered()
{
    UaStatus result;

    // write all registered nodes
    result = writeInternal(
        m_registeredNodes,
        m_pConfiguration->getWriteValues());

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

UaStatus SampleClient::registerNodes()
{
    UaStatus            result;
    ServiceSettings     serviceSettings;
    UaNodeIdArray       nodesToRegister;

    // register all nodes to write from the configuration
    nodesToRegister = m_pConfiguration->getNodesToWrite();
    printf("\nRegisterNodes...\n");
    result = m_pSession->registerNodes(
        serviceSettings,
        nodesToRegister,
        m_registeredNodes);

    if (result.isGood())
    {
        // RegisterNodes service succeded
        printf("RegisterNodes succeeded\n");

        for (OpcUa_UInt32 i = 0; i < nodesToRegister.length(); i++)
        {
            printf("Original NodeId[%d]: %s\tRegistered NodeId[%d]: %s\n", i, UaNodeId(nodesToRegister[i]).toXmlString().toUtf8(), i, UaNodeId(m_registeredNodes[i]).toXmlString().toUtf8());
        }
    }
    else
    {
        // Service call failed
        printf("RegisterNodes failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus SampleClient::unregisterNodes()
{
    UaStatus            result;
    ServiceSettings     serviceSettings;

    // unregister all nodes we registered before
    printf("\nUnregisterNodes...\n");
    result = m_pSession->unregisterNodes(
        serviceSettings,
        m_registeredNodes);

    if (result.isGood())
    {
        // RegisterNodes service succeded
        printf("UnregisterNodes succeeded\n");
    }
    else
    {
        // Service call failed
        printf("UnregisterNodes failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

UaStatus SampleClient::browseInternal(const UaNodeId& nodeToBrowse, OpcUa_UInt32 maxReferencesToReturn)
{
    UaStatus result;

    ServiceSettings serviceSettings;
    BrowseContext browseContext;
    UaByteString continuationPoint;
    UaReferenceDescriptions referenceDescriptions;

    // configure browseContext
    browseContext.browseDirection = OpcUa_BrowseDirection_Forward;
    browseContext.referenceTypeId = OpcUaId_HierarchicalReferences;
    browseContext.includeSubtype = OpcUa_True;
    browseContext.maxReferencesToReturn = maxReferencesToReturn;

    printf("\nBrowsing from Node %s...\n", nodeToBrowse.toXmlString().toUtf8());
    result = m_pSession->browse(
        serviceSettings,
        nodeToBrowse,
        browseContext,
        continuationPoint,
        referenceDescriptions);

    if (result.isGood())
    {
        // print results
        printBrowseResults(referenceDescriptions);

        // continue browsing
        while (continuationPoint.length() > 0)
        {
            printf("\nContinuationPoint is set. BrowseNext...\n");
            // browse next
            result = m_pSession->browseNext(
                serviceSettings,
                OpcUa_False,
                continuationPoint,
                referenceDescriptions);

            if (result.isGood())
            {
                // print results
                printBrowseResults(referenceDescriptions);
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

void SampleClient::printBrowseResults(const UaReferenceDescriptions& referenceDescriptions)
{
    OpcUa_UInt32 i;
    for (i=0; i<referenceDescriptions.length(); i++)
    {
        printf("node: ");
        UaNodeId referenceTypeId(referenceDescriptions[i].ReferenceTypeId);
        printf("[Ref=%s] ", referenceTypeId.toString().toUtf8() );
        UaQualifiedName browseName(referenceDescriptions[i].BrowseName);
        printf("%s ( ", browseName.toString().toUtf8() );
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Object) printf("Object ");
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Variable) printf("Variable ");
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_Method) printf("Method ");
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_ObjectType) printf("ObjectType ");
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_VariableType) printf("VariableType ");
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_ReferenceType) printf("ReferenceType ");
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_DataType) printf("DataType ");
        if (referenceDescriptions[i].NodeClass & OpcUa_NodeClass_View) printf("View ");
        UaNodeId nodeId(referenceDescriptions[i].NodeId.NodeId);
        printf("[NodeId=%s] ", nodeId.toFullString().toUtf8() );
        printf(")\n");
    }
}

void SampleClient::printCertificateData(const UaByteString& serverCertificate)
{
    // Assign certificate byte string to UaPkiCertificate class
    UaPkiCertificate cert = UaPkiCertificate::fromDER(serverCertificate);

    printf("- CommonName              %s\n", cert.commonName().toUtf8() );
    printf("- Issuer.commonName       %s\n", cert.issuer().commonName.toUtf8() );
    printf("- Issuer.organization     %s\n", cert.issuer().organization.toUtf8() );
    printf("- Issuer.organizationUnit %s\n", cert.issuer().organizationUnit.toUtf8() );
    printf("- Issuer.state            %s\n", cert.issuer().state.toUtf8() );
    printf("- Issuer.country          %s\n", cert.issuer().country.toUtf8() );
    printf("- ValidFrom               %s\n", cert.validFrom().toString().toUtf8() );
    printf("- ValidTo                 %s\n", cert.validTo().toString().toUtf8() );
}

int SampleClient::userAcceptCertificate()
{
    int result = 0;

    int ch = getchar();

    if (ch == 'y' || ch == 'Y')
    {
        result = 1;
    }
    else if (ch == 'p' || ch == 'P')
    {
        result = 2;
    }
    else
    {
        result = 0;
    }

    // empty read buffer from console
    while (ch != '\n')
    {
        ch = getchar();
    }

    return result;
}
