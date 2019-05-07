/**
 * @file pasnodemanager.hpp
 * @brief Header file for the server-side node manager class.
 */

#ifndef SERVER_PASNODEMANAGER_HPP
#define SERVER_PASNODEMANAGER_HPP

#include <memory>
#include "uabase/statuscode.h"

#include "common/opcua/pasnodemanagercommon.hpp"

class PasCommunicationInterface;

/// @brief Class for initializing and managing all OPC UA nodes in the server.
class PasNodeManager : public PasNodeManagerCommon {
    UA_DISABLE_COPY(PasNodeManager); // Disables copy construction and copy assignment
public:
    /// @brief Instantiate a PasNodeManager object.
    PasNodeManager();

    /// @brief Create OPC UA nodes and folders for all devices in the server.
    /// @return OPC UA status code indicating success or failure.
    UaStatus afterStartUp() override;

    /// @brief Clean up before shutting down server.
    /// @return OPC UA status code indicating success or failure.
    UaStatus beforeShutDown() override;

    /// @brief Set the communication interface to use for operating/reading devices.
    /// @param pCommIf Pointer to the communication interface for the node manager.
    void setCommunicationInterface(std::unique_ptr<PasCommunicationInterface> &pCommIf);

private:
    /// @brief Add a custom type definition node for the Panel type.
    /// @return OPC UA status code indicating success or failure.
    UaStatus amendTypeNodes();
};

#endif //SERVER_PASNODEMANAGER_HPP
