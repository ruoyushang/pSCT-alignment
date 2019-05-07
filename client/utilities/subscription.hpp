/**
 * @file subscription.h
 * @brief Header file for a Subscription class wrapping the OPC UA Subscription class.
 *
 * @see http://documentation.unified-automation.com/uasdkcpp/1.5.4/html/classUaClientSdk_1_1UaSubscriptionCallback.html
 */

#ifndef __SUBSCRIPTION_H__
#define __SUBSCRIPTION_H__

#include "uabase/uabase.h"
#include "uaclient/uaclientsdk.h"
#include "client/utilities/configuration.hpp"

/// @brief Class to manage subscriptions to OPC UA nodes. Wraps the standard
/// UaSubscription and UaSubscriptionCallback classes and takes a configuration
// object as input.
class Subscription : public UaClientSdk::UaSubscriptionCallback
{
    UA_DISABLE_COPY(Subscription); // Prevents copying of object constructor
public:
    /// @brief Constructor for a Subscription object.
    /// @param pConfiguration Pointer to a Configuration
    /// object, used to get a list of OPC UA nodes to monitor.
    explicit Subscription(Configuration *pConfiguration);
    /// @brief Destructor for a Subscription object.
    ~Subscription() override;

    // UaSubscriptionCallback implementation ----------------------------------------------------

    /// @brief Callback function called when a subscription has failed/errored.
    /// Recovers the failed subscription. Implements the UaSubscriptionCallback
    /// interface.
    /// @param clientSubscriptionHandle Unique handle which identifies the
    /// subscription which failed.
    /// @param status OPC UA status code which indicates the status of the
    /// subscription.
    void subscriptionStatusChanged(
            OpcUa_UInt32      clientSubscriptionHandle,
            const UaStatus &status) override;
    /// @brief Callback function called when a monitored node has a data change.
    /// @param clientSubscriptionHandle Unique handle which identifies the
    /// subscription which had the data change.
    /// @param dataNotifications List of data change notifications (since the
    /// last publishing period).
    /// @param diagnosticInfos List of additional diagnostic info about the
    /// data change notifications
    void dataChange(
            OpcUa_UInt32               clientSubscriptionHandle,
            const UaDataNotifications& dataNotifications,
            const UaDiagnosticInfos &diagnosticInfos) override;
    /// @brief Callback function called when an OPC UA event occurs at a
    /// monitored node.
    /// @param clientSubscriptionHandle Unique handle which identifies the
    /// subscription which received the event.
    /// @param eventFieldList List of event notifications (since the
    /// last publishing period).
    void newEvents(
            OpcUa_UInt32                clientSubscriptionHandle,
            UaEventFieldLists &eventFieldList) override;

    // UaSubscriptionCallback implementation ----------------------------------

    /// @brief Create an internal OPC UA subscription object.
    /// @param pSession OPC UA session object in which to create the
    /// subscription.
    /// @return An OPC UA status code.
    UaStatus createSubscription(UaClientSdk::UaSession* pSession);
    /// @brief Delete the internal OPC UA subscription object.
    /// @return An OPC UA status code.
    UaStatus deleteSubscription();
    /// @brief Get nodes to monitor from the Configuration object and add them
    /// to the Subscription.
    /// @return An OPC UA status code.
    UaStatus createMonitoredItems();

    // set the configuration where we get the list of NodeIds to monitored from.
    /// @brief Set a new instance as the internal Configuration object.
    /// @param pConfiguration A Configuration object to attach to the
    /// Subscription.
    void setConfiguration(Configuration* pConfiguration);

private:
    /// @brief Method to delete the existing UaSubscription, re-create it,
    /// and re-create monitored items if it becomes invalid/fails.
    /// @return An OPC UA status code.
    UaStatus recoverSubscription();

    /// @brief Pointer to an OPC UA session object, used to
    /// create an OPC UA subscription object.
    UaClientSdk::UaSession* m_pSession;
    /// @brief Pointer to an OPC UA subscription object, used to create
    /// monitored items (nodes).
    UaClientSdk::UaSubscription* m_pSubscription;
    /// @brief Pointer to a Configuration object, used to retrieve a list
    /// of OPC UA nodes to monitor via subscription.
    Configuration* m_pConfiguration;
};

#endif // SUBSCRIPTION_H
