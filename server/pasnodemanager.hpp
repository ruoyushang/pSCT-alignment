/**
 * @file pasnodemanager.hpp
 * @brief Header file for the server-side node manager class.
 */

#ifndef SERVER_PASNODEMANAGER_HPP
#define SERVER_PASNODEMANAGER_HPP

#include <memory>
#include "uabase/statuscode.h"

#include "common/opcua/pasnodemanagercommon.hpp"
#include "common/opcua/pasobject.hpp"
#include "common/opcua/historymanagercache.h"

class PasServerCommunicationInterface;
#if SUPPORT_Historical_Access
class HistoryManagerCache;
#endif // SUPPORT_Historical_Access

/// @brief Class for initializing and managing all OPC UA nodes in the server.
class PasServerNodeManager : public PasNodeManagerCommon {
    UA_DISABLE_COPY(PasServerNodeManager); // Disables copy construction and copy assignment
public:
    /// @brief Instantiate a PasServerNodeManager object.
    PasServerNodeManager();

    /// @brief Destroy a PasServerNodeManager object.
    ~PasServerNodeManager() override;

    /// @brief Create OPC UA nodes and folders for all devices in the server.
    /// @return OPC UA status code indicating success or failure.
    UaStatus afterStartUp() override;

    /// @brief Clean up before shutting down server.
    /// @return OPC UA status code indicating success or failure.
    UaStatus beforeShutDown() override;

    /// @brief Set the panel number used when retreiving information from the database.
    /// @param panelNumber Position number of the panel for this server.
    void setPanelNumber(int panelNumber);

    // SamplingOnRequestExample change begin
    // Added: Overwrite of function variableCacheMonitoringChanged() to get informed by NodeManagerBase
    void variableCacheMonitoringChanged(UaVariableCache* pVariable, TransactionType transactionType);
    // Added: Main function for worker thread used to execute the sampling
    void run();
    // SamplingOnRequestExample change end

private:
    /// @brief Add a custom type definition node for the Panel type.
    /// @return OPC UA status code indicating success or failure.
    UaStatus amendTypeNodes();

    /// @brief Position number of the panel. Used for device database lookup.
    int m_panelNum;

    std::shared_ptr<PasServerCommunicationInterface> m_pCommIf;
};

#endif //SERVER_PASNODEMANAGER_HPP
