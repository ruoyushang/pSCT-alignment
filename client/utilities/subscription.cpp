/**
 * @file subscription.cpp
 * @brief Source file for a Subscription class wrapping the OPC UA Subscription class.
 *
 * @see http://documentation.unified-automation.com/uasdkcpp/1.5.4/html/classUaClientSdk_1_1UaSubscriptionCallback.html
 */

#include "subscription.hpp"
#include "uaclient/uasubscription.h"
#include "uaclient/uasession.h"
#include "clienthelper.hpp"

#include <memory>

class Configuration;

/// @details The constructor nitializes the internal Configuration pointer,
/// but leaves the internal Session and Subscription pointers as NULL.
Subscription::Subscription(std::shared_ptr<Configuration> pConfiguration)
    : m_pSession(nullptr),
      m_pSubscription(nullptr),
      m_pConfiguration(pConfiguration)
{
}

/// @details If this Subscription object has an internal UaSubscription, calls
/// deleteSubscription().
Subscription::~Subscription()
{
    if ( m_pSubscription )
    {
        deleteSubscription();
    }
}

/// @details If the internal Subscription object errors, prints an error
/// message, then calls recoverSubscription() to recover the Subscription.
void Subscription::subscriptionStatusChanged(
    OpcUa_UInt32      clientSubscriptionHandle, //!< [in] Client defined handle of the affected subscription
    const UaStatus&   status)                   //!< [in] Changed status for the subscription
{
    OpcUa_ReferenceParameter(clientSubscriptionHandle); // We use the callback only for this subscription

    if (status.isBad())
    {
        printf("Subscription not longer valid - failed with status %s\n", status.toString().toUtf8());

        // recover subscription on the server
        recoverSubscription();
    }
}

/// @details If a monitored variable(s) changes, prints a notification about the
/// change(s). If any of the changed variables indicate an error, prints an
/// error message instead.
void Subscription::dataChange(
    OpcUa_UInt32               clientSubscriptionHandle, //!< [in] Client defined handle of the affected subscription
    const UaDataNotifications& dataNotifications,        //!< [in] List of data notifications sent by the server
    const UaDiagnosticInfos&   diagnosticInfos)          //!< [in] List of diagnostic info related to the data notifications. This list can be empty.
{
    OpcUa_ReferenceParameter(clientSubscriptionHandle); // We use the callback only for this subscription
    OpcUa_ReferenceParameter(diagnosticInfos);
    OpcUa_UInt32 i = 0;

    printf("-- DataChange Notification ---------------------------------\n");
    for ( i=0; i<dataNotifications.length(); i++ )
    {
        if ( OpcUa_IsGood(dataNotifications[i].Value.StatusCode) )
        {
            UaVariant tempValue = dataNotifications[i].Value.Value;
            printf("  Variable = %d value = %s\n", dataNotifications[i].ClientHandle, tempValue.toString().toUtf8());
        }
        else
        {
            UaStatus itemError(dataNotifications[i].Value.StatusCode);
            printf("  Variable = %d failed with status %s\n", dataNotifications[i].ClientHandle, itemError.toString().toUtf8());
        }
    }
    printf("------------------------------------------------------------\n");
}

/// @details If an event triggers on a monitored OPC UA node, does nothing.
void Subscription::newEvents(
    OpcUa_UInt32                clientSubscriptionHandle, //!< [in] Client defined handle of the affected subscription
    UaEventFieldLists&          eventFieldList)           //!< [in] List of event notifications sent by the server
{
    OpcUa_ReferenceParameter(clientSubscriptionHandle);
    OpcUa_ReferenceParameter(eventFieldList);
}

/// @details If the object does not already have an internal UaSubscription,
/// creates one with a fixed publishing interval of 100 ms in the provided
/// UaSession. Else, returns an error. Prints a success/failure
/// message on exit.
UaStatus Subscription::createSubscription(UaClientSdk::UaSession* pSession)
{
    if ( m_pSubscription )
    {
        printf("\nError: Subscription already created\n");
        return OpcUa_BadInvalidState;
    }

    m_pSession = pSession;

    UaStatus result;

    UaClientSdk::ServiceSettings serviceSettings;
    UaClientSdk::SubscriptionSettings subscriptionSettings;
    subscriptionSettings.publishingInterval = 100;

    printf("\nCreating subscription ...\n");
    result = pSession->createSubscription(
        serviceSettings,
        this,
        1,
        subscriptionSettings,
        OpcUa_True,
        &m_pSubscription);

    if (result.isGood())
    {
        printf("CreateSubscription succeeded\n");
    }
    else
    {
        m_pSubscription = nullptr;
        printf("CreateSubscription failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

/// @details If the object has a UaSubscription, deletes it from the session and
/// cleans up. Else, returns an error. Prints a success/failure
/// message on exit.
UaStatus Subscription::deleteSubscription()
{
    if (m_pSubscription == nullptr)
    {
        printf("\nError: No Subscription created\n");
        return OpcUa_BadInvalidState;
    }

    UaStatus result;
    UaClientSdk::ServiceSettings serviceSettings;

    // let the SDK cleanup the resources for the existing subscription
    printf("\nDeleting subscription ...\n");
    result = m_pSession->deleteSubscription(
        serviceSettings,
        &m_pSubscription);

    if (result.isGood())
    {
        printf("DeleteSubscription succeeded\n");
    }
    else
    {
        printf("DeleteSubscription failed with status %s\n", result.toString().toUtf8());
    }
    m_pSubscription = nullptr;

    return result;
}

/// @details Receives list of OPC UA nodes to monitor from its internal
/// Configuration object's getNodesToMonitor(), then adds them to the
/// Subscription with a sampling interval of 500 ms. If the object does not
/// have an internal UaSubscription, returns an error. Prints a success/failure
/// message on exit.
UaStatus Subscription::createMonitoredItems()
{
    if (m_pSubscription == nullptr)
    {
        printf("\nError: No Subscription created\n");
        return OpcUa_BadInvalidState;
    }

    UaStatus result;
    OpcUa_UInt32 size;
    OpcUa_UInt32 i;
    UaClientSdk::ServiceSettings serviceSettings;
    UaMonitoredItemCreateRequests itemsToCreate;
    UaMonitoredItemCreateResults createResults;

    // Configure items to add to the subscription
    UaNodeIdArray lstNodeIds = m_pConfiguration->getNodesToMonitor();
    size = lstNodeIds.length();
    itemsToCreate.create(size);

    for (i = 0; i < size; i++)
    {
        itemsToCreate[i].ItemToMonitor.AttributeId = OpcUa_Attributes_Value;
        OpcUa_NodeId_CopyTo(&lstNodeIds[i], &itemsToCreate[i].ItemToMonitor.NodeId);
        itemsToCreate[i].RequestedParameters.ClientHandle = i;
        itemsToCreate[i].RequestedParameters.SamplingInterval = 500;
        itemsToCreate[i].RequestedParameters.QueueSize = 1;
        itemsToCreate[i].RequestedParameters.DiscardOldest = OpcUa_True;
        itemsToCreate[i].MonitoringMode = OpcUa_MonitoringMode_Reporting;
    }

    printf("\nAdding monitored items to subscription ...\n");
    result = m_pSubscription->createMonitoredItems(
        serviceSettings,
        OpcUa_TimestampsToReturn_Both,
        itemsToCreate,
        createResults);

    if (result.isGood())
    {
        // check individual results
        for (i = 0; i < createResults.length(); i++)
        {
            if (OpcUa_IsGood(createResults[i].StatusCode))
            {
                printf("CreateMonitoredItems succeeded for item: %s\n",
                    UaNodeId(itemsToCreate[i].ItemToMonitor.NodeId).toXmlString().toUtf8());
            }
            else
            {
                printf("CreateMonitoredItems failed for item: %s - Status %s\n",
                    UaNodeId(itemsToCreate[i].ItemToMonitor.NodeId).toXmlString().toUtf8(),
                    UaStatus(createResults[i].StatusCode).toString().toUtf8());
            }
        }
    }
    // service call failed
    else
    {
        printf("CreateMonitoredItems failed with status %s\n", result.toString().toUtf8());
    }

    return result;
}

/// @details Sets the internal Configuration object pointer to the provided
/// Configuration object pointer.
void Subscription::setConfiguration(std::shared_ptr<Configuration> pConfiguration)
{
    m_pConfiguration = pConfiguration;
}

/// @details If the object has an internal UaSubscription, deletes it using
/// deleteSubscription(), then creates a new one with createSubscription() and
/// re-monitors nodes with createMonitoredItems(). Prints a success/failure
/// message on exit.
UaStatus Subscription::recoverSubscription()
{
    UaStatus result;

    // delete existing subscription
    if (m_pSubscription)
    {
        deleteSubscription();
    }

    // create a new subscription
    result = createSubscription(m_pSession);

    // create monitored items
    if (result.isGood())
    {
        result = createMonitoredItems();
    }

    printf("-------------------------------------------------------------\n");
    if (result.isGood())
    {
        printf("RecoverSubscription succeeded.\n");
    }
    else
    {
        printf("RecoverSubscription failed with status %s\n", result.toString().toUtf8());
    }
    printf("-------------------------------------------------------------\n");

    return result;
}