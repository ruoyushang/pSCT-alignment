/**
 * @file pasnodemanager.h
 * @brief Header file for PasNodeManager class.
 */

#ifndef __PASNODEMANAGER_H__
#define __PASNODEMANAGER_H__

#include "pasnodemanagercommon.hpp"
#include <vector>

class PasCommunicationInterface;
class Configuration;
class Client;

/// @brief Node manager class
class PasNodeManager : public PasNodeManagerCommon
{
    UA_DISABLE_COPY(PasNodeManager);
public:
    /// @brief Constructor for a PasNodeManager object.
    PasNodeManager();
    /// @brief Destructor for a PasNodeManager object.
    ~PasNodeManager() override;

    /// @brief Initialization method called after startup to add objects and
    /// folders to the OPC UA node graph.
    /// @return An OPC UA status code.
    UaStatus afterStartUp() override;
    /// @brief Cleanup method to disconnect from positioner server and all
    /// panel servers at client shutdown.
    /// @return An OPC UA status code.
    UaStatus beforeShutDown() override;

    /// @brief Setter method to set the Configuration object.
    /// @param pConfiguration Pointer to a new Configuration object.
    void setConfiguration(Configuration *pConfiguration);
    /// @brief Setter method to set the PasCommunicationInterface object.
    /// @param pCommIf Pointer to a new PasCommunicationInterface object.
    void setCommunicationInterface(PasCommunicationInterface *pCommIf);

    /// @brief Method to get locale ID indicating the region/language.
    /// @return A locale ID string indicating the region/language.
    UaString getDefaultLocaleId() { return m_defaultLocaleId; };

    /// @brief Emergency method to test all actuator devices by toggling their
    /// state variable.
    /// @return Integer status indicator.
    OpcUa_Int32 Panic();

private:
    /// @brief Adds all custom type nodes for device object types.
    /// @return An OPC UA status code.
    UaStatus amendTypeNodes();
    /// @brief Configuration object used to
    Configuration *m_pConfiguration = nullptr;
    /// @brief Pointers to all OPC UA clients (controller boards) connected to
  	/// the master alignment client.
    std::vector<std::shared_ptr<Client>> m_pClients;
    /// @brief Pointer to telescope positioner OPC UA client.
    std::shared_ptr<Client> m_pPositioner;

};

#endif // __PASNODEMANAGER_H__
