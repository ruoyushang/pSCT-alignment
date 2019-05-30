#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "uabase.h"
#include "uaclient/uaclientsdk.h"
#include "passervertypeids.hpp"
#include "common/alignment/device.hpp"
#include <vector>
#include <string>
#include <set>
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
    Device::Identity getDeviceBySerial(OpcUa_UInt32 deviceType, int serial) {
        return m_DeviceSerialMap.at(deviceType).at(serial);
    }
    // {serial->Address} map of all devices of the requested type
    const std::set<Device::Identity> &getDevices(OpcUa_UInt32 deviceType) { return m_DeviceIdentities.at(deviceType); };
    // get list of all parents of a given device.
    // this is a vector of pairs {parentType, parentIdentity}
    // need to pass around the whole object!
    // can't return this by reference, since we're creating the objects in place.
    std::map<OpcUa_UInt32, std::set<Device::Identity>> getParents(const Device::Identity &id) {
        return m_ParentMap.at(id);
    }

    std::map<OpcUa_UInt32, std::set<Device::Identity>> getChildren(const Device::Identity &id) {
        return m_ChildMap.at(id);
    }

    Device::Identity getPanelFromAddress(const std::string &eAddress) { return m_PanelAddressMap.at(eAddress); }

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

    bool addMissingParents();

    Device::Identity getMirrorId(int mirrorNum);

    int getThirdPanelPosition(int wPanelPosition, int lPanelPosition);

    std::map<OpcUa_UInt32, std::set<Device::Identity>> m_DeviceIdentities;

    // maps device id -> device type -> identities
    std::map<Device::Identity, std::map<OpcUa_UInt32, std::set<Device::Identity>>> m_ParentMap;

    // maps device id -> device type -> identities
    std::map<Device::Identity, std::map<OpcUa_UInt32, std::set<Device::Identity>>> m_ChildMap;

    // maps mpes id -> "w", "l" -> panelId
    std::map<Device::Identity, std::map<std::string, Device::Identity>> m_MPES_SideMap;

    // maps devicetype -> serial -> identity
    std::map<OpcUa_UInt32, std::map<int, Device::Identity>> m_DeviceSerialMap;

    // maps panel position -> identity
    std::map<int, Device::Identity> m_PanelPositionMap;

    // maps panel eAddress -> identity
    std::map<std::string, Device::Identity> m_PanelAddressMap;
};

#endif // CONFIGURATION_H
