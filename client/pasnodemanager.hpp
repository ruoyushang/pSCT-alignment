/**
 * @file pasnodemanager.h
 * @brief Header file for PasClientNodeManager class.
 */

#ifndef __PASNODEMANAGER_H__
#define __PASNODEMANAGER_H__

#include <vector>

#include "common/opcua/pasnodemanagercommon.hpp"


class PasClientCommunicationInterface;
class Configuration;
class Client;

/// @brief Node manager class
class PasClientNodeManager : public PasNodeManagerCommon
{
    UA_DISABLE_COPY(PasClientNodeManager);
public:
    /// @brief Constructor for a PasClientNodeManager object.
    explicit PasClientNodeManager(std::shared_ptr<Configuration> pConfiguration, std::string mode = "subclient");
    /// @brief Destructor for a PasClientNodeManager object.
    ~PasClientNodeManager() override;

    /// @brief Initialization method called after startup to add objects and
    /// folders to the OPC UA node graph.
    /// @return An OPC UA status code.
    UaStatus afterStartUp() override;
    /// @brief Cleanup method to disconnect from positioner server and all
    /// panel servers at client shutdown.
    /// @return An OPC UA status code.
    UaStatus beforeShutDown() override;

    void createClients();

    /// @brief Setter method to set the PasClientCommunicationInterface object.
    /// @param pCommIf Pointer to a new PasClientCommunicationInterface object.
    void setCommunicationInterface(std::shared_ptr<PasClientCommunicationInterface> &pCommIf);

    /// @brief Method to get locale ID indicating the region/language.
    /// @return A locale ID string indicating the region/language.
    UaString getDefaultLocaleId() { return m_defaultLocaleId; };

    /// @brief Emergency method to test all actuator devices by toggling their
    /// state variable.
    /// @return Integer status indicator.
    OpcUa_Int32 Panic();

    // SamplingOnRequestExample change begin
    // Added: Overwrite of function variableCacheMonitoringChanged() to get informed by NodeManagerBase
    void variableCacheMonitoringChanged(UaVariableCache* pVariable, TransactionType transactionType);
    // Added: Main function for worker thread used to execute the sampling
    void run();
    // SamplingOnRequestExample change end

private:
    std::string m_Mode;
    /// @brief Adds all custom type nodes for device object types.
    /// @return An OPC UA status code.
    UaStatus amendTypeNodes();
    /// @brief Configuration object used to
    std::shared_ptr<Configuration> m_pConfiguration = nullptr;
    /// @brief Pointers to all OPC UA clients (controller boards) connected to
  	/// the master alignment client.
    std::vector<Client *> m_pClients;
    /// @brief Pointer to telescope positioner OPC UA client.
    Client *m_pPositioner;

};

#endif // __PASNODEMANAGER_H__
