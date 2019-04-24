#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "uabase.h"
#include "uaclient/uaclientsdk.h"
#include "passervertypeids.hpp"
#include "components.hpp"
#include <vector>
#include <string>
#include <map>
#include <utility> // pair

#define COMPANY_NAME "CTA-US"
#define PRODUCT_NAME "P2PASClient"

class Configuration
{
    UA_DISABLE_COPY(Configuration);
public:
    Configuration();

    virtual ~Configuration() = default;

    // get connection and session parameters
    OpcUa_UInt32 getServers() const;

    UaString getDiscoveryUrl() const { return m_discoveryUrl; }

    UaString getPositionerUrl() const { return m_positionerUrl; }

    UaString getApplicationName() const { return m_applicationName; }

    OpcUa_Boolean getAutomaticReconnect() const { return m_bAutomaticReconnect; }

    OpcUa_Boolean getRetryInitialConnect() const { return m_bRetryInitialConnect; }

    // get lists of NodeIds and values
    UaNodeIdArray getNodesToRead() const { return m_nodesToRead; }

    UaNodeIdArray getNodesToWrite() const { return m_nodesToWrite; }

    UaNodeIdArray getNodesToMonitor() const { return m_nodesToMonitor; }

    UaVariantArray getWriteValues() const { return m_writeValues; }

    // get database params
    UaStringArray getDatabaseHost() const { return m_databaseHost; }

    UaStringArray getDatabaseUser() const { return m_databaseUser; }

    UaStringArray getDatabasePassword() const { return m_databasePassword; }

    UaStringArray getDatabaseName() const { return m_databaseName; }

    std::vector<UaStringArray> getDatabaseEntries() const { return m_databaseEntries; }

    // get device mappings
    OpcUa_Int32 getDeviceSerial(const UaString& address, OpcUa_UInt32 deviceType, const UaString& sPort);
    OpcUa_Int32 getDevicePosition(OpcUa_UInt32 deviceType, OpcUa_UInt32 serial);
    // {serial->Address} map of all devices of the requested type
    const std::vector<Identity>& getDeviceList(OpcUa_UInt32 deviceType) {return m_DeviceList.at(deviceType);};
    // get list of all parents of a given device.
    // this is a vector of pairs {parentType, parentIdentity}
    // need to pass around the whole object!
    // can't return this by reference, since we're creating the objects in place.
    std::vector< std::pair<OpcUa_UInt32, Identity> > getParents(OpcUa_UInt32 deviceType, const Identity& id);

    // load configuration from file to fill connection parameters, NamespaceArray and NodeIds
    UaStatus loadConnectionConfiguration(const UaString& sConfigurationFile);
    // load configuration from DB to fill in device mappings
    UaStatus loadDeviceConfiguration(const std::vector<std::string> &positionList);

    // create the folder structure to handle certificates and load or create a client certificate.
    UaStatus setupSecurity(UaClientSdk::SessionSecurityInfo& sessionSecurityInfo);

    // update the namespace indexes for all nodeIds and update the internal namespaceArray
    UaStatus updateNamespaceIndexes(const UaStringArray& namespaceArray);


private:
    // connection and session configuration
    UaString        m_applicationName;
    UaString        m_discoveryUrl;
    UaString        m_positionerUrl;
    OpcUa_Boolean   m_bAutomaticReconnect;
    OpcUa_Boolean   m_bRetryInitialConnect;

    // NamespaceArray and NodeIds
    UaStringArray   m_namespaceArray;
    UaNodeIdArray   m_nodesToRead;
    UaNodeIdArray   m_nodesToWrite;
    UaNodeIdArray   m_nodesToMonitor;
    UaVariantArray  m_writeValues;

    // Certificate handling
    UaString        m_certificateTrustListLocation;
    UaString        m_certificateRevocationListLocation;
    UaString        m_issuersCertificatesLocation;
    UaString        m_issuersRevocationListLocation;
    UaString        m_clientCertificateFile;
    UaString        m_clientPrivateKeyFile;

    // Database handling
    UaStringArray          m_databaseHost;
    UaStringArray          m_databaseUser;
    UaStringArray          m_databasePassword;
    UaStringArray          m_databaseName;
    std::vector<UaStringArray>  m_databaseEntries;

    // maps device type -> { serial -> immediate parent position } to be able to get parents
    std::map< OpcUa_UInt32, std::map<OpcUa_UInt32, std::string> > m_ParentMap;

    // maps device type -> { serial -> address } to be able to retrieve list of all devices
    std::map< OpcUa_UInt32, std::vector<Identity> > m_DeviceList;

    // maps to be able to set device serial and position:
    //
    // completely insane map for device type -> {its panel address -> { Port->Serial } mappings
    // this one has to map to a map because different panels have different port mappings
    std::map< OpcUa_UInt32, std::map<UaString, std::map<UaString, OpcUa_Int32> > > m_DeviceSerialMap;
    // ridiculous map for device type -> { serial->Position } mappings
    std::map< OpcUa_UInt32, std::map<OpcUa_UInt32, OpcUa_UInt32> > m_DevicePositionMap;
    // simple map from panel position to its address.
    // so with the above, this also gives us serial to address.
    std::map<OpcUa_UInt32, std::string> m_PanelAddressMap;
};

#endif // CONFIGURATION_H
