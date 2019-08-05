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
      m_pConfiguration(std::move(pConfiguration))
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
        spdlog::error("Subscription not longer valid - failed with status {}", status.toString().toUtf8());

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

    spdlog::info("-- DataChange Notification ---------------------------------");
    for ( i=0; i<dataNotifications.length(); i++ )
    {
        if ( OpcUa_IsGood(dataNotifications[i].Value.StatusCode) )
        {
            UaVariant tempValue = dataNotifications[i].Value.Value;
            spdlog::info("  Variable = {} value = {}", dataNotifications[i].ClientHandle, tempValue.toString().toUtf8());
        }
        else
        {
            UaStatus itemError(dataNotifications[i].Value.StatusCode);
            spdlog::error("  Variable = {} failed with status {}", dataNotifications[i].ClientHandle, itemError.toString().toUtf8());
        }
    }
    spdlog::info("------------------------------------------------------------");
}

/// @details If an event triggers on a monitored OPC UA node, does nothing.
void Subscription::newEvents(
    OpcUa_UInt32                clientSubscriptionHandle, //!< [in] Client defined handle of the affected subscription
    UaEventFieldLists&          eventFieldList)           //!< [in] List of event notifications sent by the server
{
    OpcUa_UInt32 i = 0;
    spdlog::info("-- Event newEvents -----------------------------------------");
    spdlog::info("clientSubscriptionHandle {}", clientSubscriptionHandle);
    for ( i=0; i<eventFieldList.length(); i++ )
    {
        UaVariant message    = eventFieldList[i].EventFields[0];
        UaVariant sourceName = eventFieldList[i].EventFields[1];
        UaVariant severity   = eventFieldList[i].EventFields[2];
        spdlog::info("Event[{}] Message = {} SourceName = {} Severity = {}",
               i,
               message.toString().toUtf8(),
               sourceName.toString().toUtf8(),
               severity.toString().toUtf8());
    }
    spdlog::info("------------------------------------------------------------");
}

/// @details If the object does not already have an internal UaSubscription,
/// creates one with a fixed publishing interval of 100 ms in the provided
/// UaSession. Else, returns an error. Prints a success/failure
/// message on exit.
UaStatus Subscription::createSubscription(UaClientSdk::UaSession* pSession)
{
    if ( m_pSubscription )
    {
        spdlog::error("Error: Subscription already created");
        return OpcUa_BadInvalidState;
    }

    m_pSession = pSession;

    UaStatus result;

    UaClientSdk::ServiceSettings serviceSettings;
    UaClientSdk::SubscriptionSettings subscriptionSettings;
    subscriptionSettings.publishingInterval = 100;

    spdlog::info("Creating subscription ...");
    result = pSession->createSubscription(
        serviceSettings,
        this,
        1,
        subscriptionSettings,
        OpcUa_True,
        &m_pSubscription);

    if (result.isGood())
    {
        spdlog::info("CreateSubscription succeeded");
    }
    else
    {
        m_pSubscription = nullptr;
        spdlog::error("CreateSubscription failed with status {}", result.toString().toUtf8());
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
        spdlog::error("Error: No Subscription created");
        return OpcUa_BadInvalidState;
    }

    UaStatus result;
    UaClientSdk::ServiceSettings serviceSettings;

    // let the SDK cleanup the resources for the existing subscription
    spdlog::info("Deleting subscription ...");
    result = m_pSession->deleteSubscription(
        serviceSettings,
        &m_pSubscription);

    if (result.isGood())
    {
        spdlog::info("DeleteSubscription succeeded");
    }
    else
    {
        spdlog::error("DeleteSubscription failed with status {}", result.toString().toUtf8());
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
        spdlog::error("Error: No Subscription created");
        return OpcUa_BadInvalidState;
    }

    UaStatus result;
    OpcUa_UInt32 i;
    UaClientSdk::ServiceSettings serviceSettings;
    UaMonitoredItemCreateRequests itemsToCreate;
    UaMonitoredItemCreateResults createResults;
// Configure one event monitored item - we use the server object here
    itemsToCreate.create(1);

    UaEventFilter            eventFilter;
    UaSimpleAttributeOperand selectElement;
    UaContentFilter*         pContentFilter        = NULL;
    UaContentFilterElement*  pContentFilterElement = NULL;
    UaFilterOperand*         pOperand              = NULL;

    itemsToCreate[0].ItemToMonitor.AttributeId = OpcUa_Attributes_EventNotifier;
    itemsToCreate[0].ItemToMonitor.NodeId.Identifier.Numeric = OpcUaId_Server;
    itemsToCreate[0].RequestedParameters.ClientHandle = 0;
    itemsToCreate[0].RequestedParameters.SamplingInterval = 0;
    itemsToCreate[0].RequestedParameters.QueueSize = 0;
    itemsToCreate[0].RequestedParameters.DiscardOldest = OpcUa_True;
    itemsToCreate[0].MonitoringMode = OpcUa_MonitoringMode_Reporting;

    // Define which eventfields to send with each event
    selectElement.setBrowsePathElement(0, UaQualifiedName("Message", 0), 1);
    eventFilter.setSelectClauseElement(0, selectElement, 3);
    selectElement.setBrowsePathElement(0, UaQualifiedName("SourceName", 0), 1);
    eventFilter.setSelectClauseElement(1, selectElement, 3);
    selectElement.setBrowsePathElement(0, UaQualifiedName("Severity", 0), 1);
    eventFilter.setSelectClauseElement(2, selectElement, 3);
// We only want to receive events of type ControllerEventType
    pContentFilter = new UaContentFilter;
    pContentFilterElement = new UaContentFilterElement;
// Operator OfType
    pContentFilterElement->setFilterOperator(OpcUa_FilterOperator_OfType);
// Operand 1 (Literal)
    pOperand = new UaLiteralOperand;
    ((UaLiteralOperand*)pOperand)->setLiteralValue(m_pConfiguration->getEventTypeToFilter());
    pContentFilterElement->setFilterOperand(0, pOperand, 1);
    pContentFilter->setContentFilterElement(0, pContentFilterElement, 1);
    eventFilter.setWhereClause(pContentFilter);
// set filter for monitored items
    eventFilter.detachFilter(itemsToCreate[0].RequestedParameters.Filter);

    spdlog::info("Adding monitored items to subscription ...");
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
                spdlog::info("CreateMonitoredItems succeeded for item: {}",
                    UaNodeId(itemsToCreate[i].ItemToMonitor.NodeId).toXmlString().toUtf8());
            }
            else
            {
                spdlog::error("CreateMonitoredItems failed for item: {} - Status {}",
                    UaNodeId(itemsToCreate[i].ItemToMonitor.NodeId).toXmlString().toUtf8(),
                    UaStatus(createResults[i].StatusCode).toString().toUtf8());
            }
        }
    }
    // service call failed
    else
    {
        spdlog::error("CreateMonitoredItems failed with status {}", result.toString().toUtf8());
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

    spdlog::info("-------------------------------------------------------------");
    if (result.isGood())
    {
        spdlog::info("RecoverSubscription succeeded.");
    }
    else
    {
        spdlog::error("RecoverSubscription failed with status {}", result.toString().toUtf8());
    }
    spdlog::info("-------------------------------------------------------------");

    return result;
}