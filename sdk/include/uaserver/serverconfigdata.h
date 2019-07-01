/******************************************************************************
** serverconfigdata.h
**
** Copyright (c) 2006-2015 Unified Automation GmbH All rights reserved.
**
** Software License Agreement ("SLA") Version 2.5
**
** Unless explicitly acquired and licensed from Licensor under another
** license, the contents of this file are subject to the Software License
** Agreement ("SLA") Version 2.5, or subsequent versions
** as allowed by the SLA, and You may not copy or use this file in either
** source code or executable form, except in compliance with the terms and
** conditions of the SLA.
**
** All software distributed under the SLA is provided strictly on an
** "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
** AND LICENSOR HEREBY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT
** LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
** PURPOSE, QUIET ENJOYMENT, OR NON-INFRINGEMENT. See the SLA for specific
** language governing rights and limitations under the SLA.
**
** The complete license agreement can be found here:
** http://unifiedautomation.com/License/SLA/2.5/
**
** Project: C++ OPC Server SDK core module
**
** Description: Configuration management class for the OPC Server.
**
******************************************************************************/
#ifndef SERVERCONFIGDATA_H
#define SERVERCONFIGDATA_H

#include "serverconfig.h"
#include "version_coremodule.h"

/**
 *  Derived from ServerConfig.
 *  It is not possible to make shallow copies of this class.
 */
class SERVER_CORE_EXPORT ServerConfigData: public ServerConfig
{
    UA_DISABLE_COPY(ServerConfigData);
public:
    /* construction. */
    ServerConfigData(const UaString& sApplicationPath, const UaString& sConfigPath, const UaString& sTracePath);
    /* destruction */
    virtual ~ServerConfigData();

    void clear();

    /* Method that needs to be implemented by the derived class to finish necessary initialization steps. */
    virtual UaStatus afterLoadConfiguration() = 0;

    /* Get Endpoint Configuration. */
    virtual UaStatus getEndpointConfiguration(
        UaString&        sRejectedCertificateDirectory,
        OpcUa_UInt32&    nRejectedCertificatesCount,
        UaEndpointArray& uaEndpointArray);

    /* Set state of successfully opened endpoint */
    virtual void endpointOpened(OpcUa_UInt32 index);

    /* Get the configurations for server certificate stores */
    virtual UaStatus getCertificateStoreConfigurations(
        CertificateStoreConfigurationPArray& certificateStores);

    /* Get the enabled settings for the different user identity tokens supported by the server. */
    virtual UaStatus getUserIdentityTokenConfig(
        OpcUa_Boolean&   bEnableAnonymous,
        OpcUa_Boolean&   bEnableUserPw,
        OpcUa_Boolean&   bEnableCertificate,
        OpcUa_Boolean&   bEnableKerberosTicket);

    /** Get the security policy to use when encrypting or signing the UserIdentityToken when it is passed to the server. */
    virtual UaStatus getUserIdentityTokenSecurityPolicy(
        UaString&   sSecurityPolicyUri);

    /** Get the configurations for the trust list and rejected folder for user certificates
    */
    virtual UaStatus getDefaultUserCertificateStore(
        CertificateStoreConfiguration** ppCertificateStore,         //!< [out] Certificate store configuration for user certificates
        UaString&                   sRejectedCertificateDirectory,  //!< [out] The directory to store rejected user certificates.
        OpcUa_UInt32&               nRejectedCertificatesCount,     //!< [out] The maximum number of rejected user certificates in the directory
        OpcUa_Boolean&              bCertificateTokenConfigured     //!< [out] Returns true if the user identity token certificate is enabled in the configuration
        );

    /** Get Ids for roles defined in the SDK. These roles are needed to control access to certificate configuration and trace configuration.
     *  @return                     Error code.
     */
    virtual UaStatus getIdsForDefaultRoles(
        OpcUa_UInt16&   nRootUserId,
        OpcUa_UInt16&   nSecurityAdminGroupId,
        OpcUa_UInt16&   nConfigAdminGroupId);

    /* Get settings for registration with discovery service. */
    virtual UaStatus getDiscoveryRegistrationConfig(
        OpcUa_UInt32&   nRegistrationInterval,
        UaStringArray&  discoveryUrlArray );

    /* Get settings for exchanging certificates with local discovery server on a windows computer for registration with local windows discovery server. */
    virtual UaStatus getWindowsDiscoveryRegistrationSecuritySetup(
        OpcUa_Boolean&  bAutomaticCertificateExchange,
        UaString&       sDiscoveryServerTrustListLocation,
        UaString&       sDiscoveryServerStoreName,
        UaString&       sDiscoveryServerCertificateName);

    /* Get Serializer Configuration. */
    virtual UaStatus getSerializerConfiguration(
        OpcUa_Int32& iMaxAlloc,
        OpcUa_Int32& iMaxStringLength,
        OpcUa_Int32& iMaxByteStringLength,
        OpcUa_Int32& iMaxArrayLength,
        OpcUa_Int32& iMaxMessageSize) const;

    /* Returns the settings for the thread pool used in the OPC UA Stack. */
    virtual UaStatus getStackThreadPoolSettings(
        OpcUa_Boolean&   bEnabled,
        OpcUa_Int32&     iMinThreads,
        OpcUa_Int32&     iMaxThreads,
        OpcUa_Int32&     iMaxJobs,
        OpcUa_Boolean&   bBlockOnAdd,
        OpcUa_UInt32&    nTimeout) const;

    /* Get UA Stack trace settings. */
    virtual UaStatus getStackTraceSettings(
        OpcUa_Boolean& bTraceEnabled,
        OpcUa_UInt32&  uTraceLevel) const;

    /*  Get UA Server SDK trace settings. */
    virtual UaStatus getServerTraceSettings(
        OpcUa_Boolean& bTraceEnabled,
        OpcUa_UInt32&  uTraceLevel,
        OpcUa_UInt32&  uMaxTraceEntries,
        OpcUa_UInt32&  uMaxBackupFiles,
        UaString&      sTraceFile,
        OpcUa_Boolean& bDisableFlush) const;

    /*  Get UA Server SDK trace event settings.*/
    virtual UaStatus getTraceEventSettings(OpcUa_UInt32&  uTraceEventLevel) const;

    /* Returns the maximum age of a request the server allows.*/
    virtual OpcUa_Int32 getMaxRequestAge() const;

    /* Get the setting parameters for session creation. */
    virtual UaStatus getSessionSettings(
        OpcUa_Int32& iMaxSessionCount,
        OpcUa_Int32& iMaxSessionsPerClient,
        OpcUa_Int32& iMinSessionTimeout,
        OpcUa_Int32& iMaxSessionTimeout) const;

    // Get the general setting parameters for the server.
    virtual UaStatus getServerSettings(
        OpcUa_Double&       minSupportedSampleRate,
        UaStringArray&      localeIdArray,
        UaStringArray&      serverProfileArray,
        UaByteStringArray&  softwareCertificateArray) const;

    /*  Returns the maximum number of Browse Continuation Points managed by the server.*/
    virtual OpcUa_Int32 getMaxBrowseContinuationPoints() const;

    /* Returns the maximum number of Browse results for one browse operation. */
    virtual OpcUa_Int32 getMaxBrowseResults() const;
    /* Returns the maximum number of nodes to browse the server will accept. */
    virtual OpcUa_Int32 getMaxNodesToBrowse() const;

    /* Get the operation limit setting parameters for historical access. */
    virtual UaStatus getMaxNodesPerHistoryService(
        OpcUa_UInt32& iMaxNodesPerHistoryReadData,
        OpcUa_UInt32& iMaxNodesPerHistoryReadEvents,
        OpcUa_UInt32& iMaxNodesPerHistoryUpdateData,
        OpcUa_UInt32& iMaxNodesPerHistoryUpdateEvents) const;

    /*  Returns the maximum number of History Continuation Points managed by the server.*/
    virtual OpcUa_Int32 getMaxHistoryContinuationPoints() const;

    /* Get the setting parameters for a subscription. */
    virtual UaStatus getSubscriptionSettings(
        OpcUa_UInt32& iMinPublishingInterval,
        OpcUa_UInt32& iMaxPublishingInterval,
        OpcUa_UInt32& iMinKeepAliveInterval,
        OpcUa_UInt32& iMinSubscriptionLifetime,
        OpcUa_UInt32& iMaxSubscriptionLifetime,
        OpcUa_UInt32& iMaxRetransmissionQueueSize,
        OpcUa_UInt32& iMaxNotificationsPerPublish) const;

    // Get the monitored item settings
    virtual UaStatus getMonitoredItemSettings(
        OpcUa_UInt32& iMaxDataQueueSize,
        OpcUa_UInt32& iMaxEventQueueSize) const;

    // Get the max object count parameters for Subscriptions.
    virtual UaStatus getSubscriptionMaxCountSettings(
        OpcUa_UInt32& iMaxSubscriptionCount,
        OpcUa_UInt32& iMaxSubscriptionsPerSession,
        OpcUa_UInt32& iMaxMonitoredItemCount,
        OpcUa_UInt32& iMaxMonitoredItemPerSubscriptionCount,
        OpcUa_UInt32& iMaxMonitoredItemPerSessionCount) const;

    /* Returns the sampling rates provided by the OPC server. */
    virtual UaStatus getAvailableSamplingRates(
        UaUInt32Array& availableSamplingRates) const;

    /* Returns the build info for the OPC server. */
    virtual OpcUa_StatusCode getBuildInfo(
        UaString&       ProductUri,
        UaString&       ManufacturerName,
        UaString&       ProductName,
        UaString&       sSoftwareVersion,
        UaString&       sBuildNumber,
        OpcUa_DateTime& BuildDate) const;

    /* Returns the server instance information for the OPC server.*/
    virtual OpcUa_StatusCode getServerInstanceInfo(
        UaString&             ServerUri,
        UaLocalizedTextArray& ServerName) const;

    // Returns the ServerId used for audit events.
    virtual UaString getServerId() const;

    /* Returns the settings for the thread pools used in the OPC server. */
    virtual UaStatus getThreadPoolSettings(
        OpcUa_UInt32&   minSizeTransactionManager,
        OpcUa_UInt32&   maxSizeTransactionManager,
        OpcUa_UInt32&   minSizeSubscriptionManager,
        OpcUa_UInt32&   maxSizeSubscriptionManager) const;

    // Flag indicating if audit events are activated.
    virtual OpcUa_Boolean isAuditActivated();

    // Provides the redundancy settings for the server.
    virtual UaStatus getRedundancySettings(
        OpcUa_RedundancySupport& redundancySupport,
        UaStringArray&           serverUriArray);

    // Provides capability to return additional servers to be returned in FindServers.
    virtual UaStatus getAdditionalServerEntries(UaApplicationDescriptions& serverDescriptions);

    // Provides configuration option for namespaces other than index 0 and 1 (OPC UA and local server).
    virtual UaStatus getConfiguredNamespaces(UaUInt16Array& namespaceIndexArray, UaStringArray& namespaceUriArray, UaBoolArray& allowRenameUriArray, UaStringArray& uniqueIdArray, UaStringArray& comAliasArray);
    virtual UaStatus addConfiguredNamespace(OpcUa_UInt16 namespaceIndex, const UaString& namespaceUri, OpcUa_Boolean allowRenameUri, const UaString& uniqueId);
    virtual UaStatus removeConfiguredNamespace(OpcUa_UInt16 namespaceIndex);

    // Provides information about tagfile name, path und file ending used in the NodeManagerTagFile.
    virtual UaStatus getTagFileConfiguration(UaString &defaultTagFileName, UaString &tagFileLocation, UaString &tagFileEnding, UaString &defaultFileUpdatePolicy);
    virtual UaStatus setTagFileConfiguration(const UaString &defaultTagFileName, const UaString &tagFileLocation, const UaString &tagFileEnding, const UaString &defaultFileUpdatePolicy);

    // Provides a mapping list of OPC UA property names to COM Data Access property Ids.
    virtual UaStatus getComDaPropertyMapping(UaStringArray& propertyNames, UaStringArray& propertyNameSpaces, UaUInt32Array& propertyIds);
    virtual UaStatus addComDaPropertyMapping(const UaString& propertyName, const UaString& propertyNameSpaceUri, OpcUa_UInt32 propertyId);

    // COM DA Server Settings.
    virtual UaStatus getComDaNamespaceInformation(UaString& defaultNamespace, UaUShort& namespaceDelimiter, OpcUa_Boolean& useComAlias);
    virtual ComDaTimestampSource getComDaTimestampSource();
    virtual OpcUa_Boolean getBrowseNextLevelForVariables();

    // COM A&E Server Settings.
    virtual UaStatus getComAeNamespaceInformation(UaString& defaultNamespace);

    // Provides a mapping list of OPC UA event type NodeIds to COM Alarms and Events category IDs.
    virtual UaStatus getComAeEventCategoryMapping(ComAeEventType eventType, UaUInt32Array& categoryIDs, UaStringArray& categoryNames, UaNodeIdArray& eventTypeIds, UaStringArray&, UaBoolArray&);
    virtual UaStatus addComAeEventCategoryMapping(ComAeEventType eventType, const UaUInt32Array& categoryIDs, const UaStringArray& categoryNames, const UaNodeIdArray& eventTypeIds, const UaStringArray&);

    virtual UaStatus getComAeAttributeMapping(ComAeAttributeMappingArray& comAeAttributeMappingArray);
    virtual UaStatus addComAeAttributeMapping(OpcUa_UInt32 attributeID, const UaString& attributeName, OpcUa_BuiltInType dataType,
        OpcUa_Boolean isArray, const UaStringArray& eventFieldPathNames, const UaStringArray& eventFieldPathNamespaceUris);

    // provide groupId and userId for pre-defined roles
    virtual UaStatus getUserAndGroupByRole(const UaString& roleName, OpcUa_UInt16& groupId, OpcUa_UInt16& userId);

    // Synchronization of configuration changes
    void lockConfigFile();
    void unlockConfigFile();

    OpcUa_UInt32 addCertificateStoreConfiguration(CertificateStoreConfiguration* pCertificateStoreConfiguration);

    // generic string replace function
    static int replaceString(UaString& sString, const UaString& sSearch, const UaString& sReplace);
    // Replace placeholder [NodeName] with real node name.
    static void replaceNodeName(UaString& sString, const UaString& sNodeName);
    // Replace placeholder [ServerName] with real server name.
    static void replaceServerName(UaString& sString, const UaString& sServerName);
    // Replace placeholder [ApplicationPath] with real application path.
    static void replaceApplicationPath(UaString& sString, const UaString& sApplicationPath);
    // Replace placeholder [ConfigPath] with real configuration files path.
    static void replaceConfigPath(UaString& sString, const UaString& sConfigPath);
    // Replace placeholder [TracePath] with real trace file path.
    static void replaceTracePath(UaString& sString, const UaString& sTracePath);

    inline UaString sApplicationPath() const {return m_sApplicationPath;}
    inline UaString sConfigPath() const {return m_sConfigPath;}
    inline UaString sTracePath() const {return m_sTracePath;}

    inline void setProductUri(const UaString& sProductUri) {m_sProductUri=sProductUri;}
    inline void setManufacturerName(const UaString& sManufacturerName) {m_sManufacturerName=sManufacturerName;}
    inline void setProductName(const UaString& sProductName) {m_sProductName=sProductName;}
    inline void setSoftwareVersion(const UaString& sSoftwareVersion) {m_sSoftwareVersion=sSoftwareVersion;}
    inline void setBuildNumber(const UaString& sBuildNumber) {m_sBuildNumber=sBuildNumber;}

protected:
    /** Get the build date from the static compiled in string. */
    virtual OpcUa_DateTime getBuildDate() const;

protected:
    // Synchronization object
    mutable UaMutex       m_mutex;

    /* Maximum age of a request the server allows */
    OpcUa_Int32           m_maxRequestAge;

    /* Maximum number of sessions the server allows to create */
    OpcUa_Int32           m_maxSessionCount;
    /* Maximum number of sessions the server allows per Client, 0 is no limitation */
    OpcUa_Int32           m_maxSessionsPerClient;
    /* Minimum timeout in ms for a sessions the server allows to set */
    OpcUa_Int32           m_minSessionTimeout;
    /* Maximum timeout in ms for a sessions the server allows to set */
    OpcUa_Int32           m_maxSessionTimeout;

    /* Maximum number of Browse Continuation Points managed by the server */
    OpcUa_Int32           m_maxBrowseContinuationPoints;
    /* Maximum number of Browse results for one browse operation */
    OpcUa_Int32           m_maxBrowseResults;
    /* Maximum number of nodes to browse the server will accept */
    OpcUa_Int32           m_maxNodesToBrowse;
    OpcUa_UInt32          m_iMaxNodesPerHistoryReadData;
    OpcUa_UInt32          m_iMaxNodesPerHistoryReadEvents;
    OpcUa_UInt32          m_iMaxNodesPerHistoryUpdateData;
    OpcUa_UInt32          m_iMaxNodesPerHistoryUpdateEvents;
    /* Maximum number of History Continuation Points managed by the server */
    OpcUa_Int32           m_maxHistoryContinuationPoints;

    /* Minimum publishing interval in milliseconds the server allows */
    OpcUa_UInt32          m_minPublishingInterval;
    /* Maximum publishing interval in milliseconds the server allows */
    OpcUa_UInt32          m_maxPublishingInterval;
    /* Minimum KeepAlive interval  in milliseconds the server allows */
    OpcUa_UInt32          m_minKeepAliveInterval;
    /* Minimum Subscription lifetime in milliseconds the server allows */
    OpcUa_UInt32          m_minSubscriptionLifetime;
    /* Maximum Subscription lifetime in milliseconds the server allows */
    OpcUa_UInt32          m_maxSubscriptionLifetime;
    /* Maximum number of messages in the republish queue the server allows per Subscription */
    OpcUa_UInt32          m_maxRetransmissionQueueSize;
    /* Maximum number of notifications per Publish the server allows */
    OpcUa_UInt32          m_maxNotificationsPerPublish;
    OpcUa_UInt32          m_maxSubscriptionCount;
    OpcUa_UInt32          m_maxSubscriptionsPerSession;
    OpcUa_UInt32          m_maxMonitoredItemCount;
    OpcUa_UInt32          m_maxMonitoredItemPerSubscriptionCount;
    OpcUa_UInt32          m_maxMonitoredItemPerSessionCount;

    OpcUa_Double          m_minSupportedSampleRate;
    UaStringArray         m_localeIdArray;
    UaStringArray         m_serverProfileArray;
    UaByteStringArray     m_softwareCertificateArray;

    OpcUa_UInt32          m_iMaxDataQueueSize;
    OpcUa_UInt32          m_iMaxEventQueueSize;

    /* Array of sampling rates provided by the OPC server */
    UaUInt32Array         m_availableSamplingRates;

    /* BuildInfo structure */
    UaString              m_sProductUri;
    UaString              m_sManufacturerName;
    UaString              m_sProductName;
    UaString              m_sSoftwareVersion;
    UaString              m_sBuildNumber;

    /* Server Description */
    UaString              m_sServerUri;
    UaString              m_sServerName;

    /* UA Stack Thread pool settings */
    OpcUa_Boolean         m_bStackThreadPoolEnabled;
    OpcUa_Int32           m_iMinStackThreads;
    OpcUa_Int32           m_iMaxStackThreads;
    OpcUa_Int32           m_iMaxStackThreadJobs;
    OpcUa_Boolean         m_bStackThreadBlockOnAdd;
    OpcUa_UInt32          m_nStackThreadTimeout;

    /* SDK Thread pool settings */
    OpcUa_UInt32          m_minSizeTransactionManager;
    OpcUa_UInt32          m_maxSizeTransactionManager;
    OpcUa_UInt32          m_minSizeSubscriptionManager;
    OpcUa_UInt32          m_maxSizeSubscriptionManager;

    /* directory to store rejected certificates in */
    UaString              m_sRejectedCertificatesDirectory;
    OpcUa_UInt32          m_nRejectedCertificatesCount;
    /* Array of UA endpoint descriptions */
    UaEndpointArray       m_uaEndpointArray;

    /* User Identity Token settings*/
    OpcUa_Boolean         m_bEnableAnonymous;
    OpcUa_Boolean         m_bEnableUserPw;
    OpcUa_Boolean         m_bEnableCertificate;
    OpcUa_Boolean         m_bEnableKerberosTicket;
    UaString              m_sUserIdentityTokenSecurityPolicy;
    /* Certificate store for user certificates*/
    CertificateStoreConfiguration* m_pCertificateStoreUser;
    /* directory to store rejected user certificates*/
    UaString              m_sRejectedUserCertificatesDirectory;
    OpcUa_UInt32          m_nRejectedUserCertificatesCount;

    /* User and group Ids for defined default roles */
    OpcUa_UInt16          m_nRootUserId;
    OpcUa_UInt16          m_nSecurityAdminGroupId;
    OpcUa_UInt16          m_nConfigAdminGroupId;

    /* Discovery server registration settings. */
    OpcUa_UInt32          m_nRegistrationInterval;
    UaStringArray         m_discoveryUrlArray;
    OpcUa_Boolean         m_bAutomaticCertificateExchange;
    UaString              m_sDiscoveryServerTrustListLocation;
    UaString              m_sDiscoveryServerStoreName;
    UaString              m_sDiscoveryServerCertificateName;

    /* These are the Security constraints for the serializer. Set this values carefully.*/
    OpcUa_Int32           m_iMaxAlloc;
    OpcUa_Int32           m_iMaxStringLength;
    OpcUa_Int32           m_iMaxByteStringLength;
    OpcUa_Int32           m_iMaxArrayLength;
    OpcUa_Int32           m_iMaxMessageSize;

    /* UA Stack trace settings. */
    OpcUa_Boolean         m_bTraceEnabled;
    OpcUa_UInt32          m_uTraceLevel;

    /* UA Server SDK trace settings. */
    OpcUa_Boolean         m_bSdkTraceEnabled;
    OpcUa_UInt32          m_uSdkTraceLevel;
    OpcUa_UInt32          m_uMaxTraceEntries;
    OpcUa_UInt32          m_uMaxBackupFiles;
    UaString              m_sTraceFile;
    OpcUa_Boolean         m_bDisableFlush;

    /* UA Server SDK trace event settings */
    OpcUa_UInt32          m_uTraceEventLevel;

    OpcUa_Boolean         m_isAuditActivated;

    OpcUa_RedundancySupport m_redundancySupport;
    UaStringArray           m_serverUriArray;

    UaApplicationDescriptions m_serverDescriptions;

    // Configured Namespaces
    UaUInt16Array         m_namespaceIndexArray;
    UaStringArray         m_namespaceUriArray;
    UaBoolArray           m_allowRenameUriArray;
    UaStringArray         m_uniqueIdArray;
    UaStringArray         m_comAliasArray;

    // Tagfile
    UaString    m_sDefaultTagFileName;
    UaString    m_sTagFileLocation;
    UaString    m_sTagFileEnding;
    UaString    m_sDefaultFileUpdatePolicy;

    // Provides a mapping list of OPC UA property names to COM Data Access property Ids.
    UaStringArray         m_propertyNames;
    UaStringArray         m_propertyNameSpaces;
    UaUInt32Array         m_propertyIds;

    // Settings for COM DA Server.
    UaString              m_defaultNamespace;
    UaUShort              m_ComDaNamespaceDelimiter;
    OpcUa_Boolean         m_useComAlias;
    ServerConfig::ComDaTimestampSource m_comDaTimestampType;
    OpcUa_Boolean         m_browseNextLevelForVariables;

    // Settings for COM A&E Server.
    UaString              m_defaultAeNamespace;

    // Provides a mapping list of OPC UA event type NodeIds to COM Alarms and Events category IDs.
    UaUInt32Array         m_simpleCategoryIDs;
    UaStringArray         m_simpleCategoryNames;
    UaNodeIdArray         m_simpleEventTypeIds;
    UaStringArray         m_simpleNamespaceUris;
    UaBoolArray           m_simpleVisbilityFlags;
    UaUInt32Array         m_trackingCategoryIDs;
    UaStringArray         m_trackingCategoryNames;
    UaNodeIdArray         m_trackingEventTypeIds;
    UaStringArray         m_trackingNamespaceUris;
    UaBoolArray           m_trackingVisbilityFlags;
    UaUInt32Array         m_conditionCategoryIDs;
    UaStringArray         m_conditionCategoryNames;
    UaNodeIdArray         m_conditionEventTypeIds;
    UaStringArray         m_conditionNamespaceUris;
    UaBoolArray           m_conditionVisbilityFlags;

    ComAeAttributeMappingArray m_comAeAttributeMappingArray;

    UaString              m_sApplicationPath;
    UaString              m_sConfigPath;
    UaString              m_sTracePath;

    OpcUa_Boolean         m_isConfigLoaded;

    CertificateStoreConfigurationPArray m_certificateStores;
};

#endif // SERVERCONFIGDATA_H
