/**
 * @file subscription.cpp
 * @brief Source file for a Subscription class wrapping the OPC UA Subscription class.
 *
 * @see http://documentation.unified-automation.com/uasdkcpp/1.5.4/html/classUaClientSdk_1_1UaSubscriptionCallback.html
 */

#include "subscription.hpp"
#include "uaclient/uasubscription.h"
#include "uaclient/uasession.h"
#include "uaclient/uaclientsdk.h"

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
UaStatus Subscription::createSubscription(UaSession* pSession)
{
    if ( m_pSubscription )
    {
        printf("\nError: Subscription already created\n");
        return OpcUa_BadInvalidState;
    }

    m_pSession = pSession;

    UaStatus result;

    ServiceSettings serviceSettings;
    SubscriptionSettings subscriptionSettings;
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
    ServiceSettings serviceSettings;

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
    OpcUa_UInt32 size, i;
    ServiceSettings serviceSettings;
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
    m_pConfiguration = std::move(pConfiguration);
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

/*============================================================================
 * historyReadDataRaw - read raw data history
 *===========================================================================*/
void historyReadDataRaw() {
    printf("\n\n****************************************************************\n");
    printf("** Try to read raw data history\n");
    if (g_pUaSession == NULL) {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }
    if (g_HistoryDataNodeIds.length() < 1) {
        printf("** Error: No history node configured\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus status;
    ServiceSettings serviceSettings;
    HistoryReadRawModifiedContext historyReadRawModifiedContext;
    UaHistoryReadValueIds nodesToRead;
    HistoryReadDataResults results;
    UaDiagnosticInfos diagnosticInfos;

    UaNodeId nodeToRead(g_HistoryDataNodeIds[0]);

    // Read the last 30 minutes
    UaDateTime timeSetting = UaDateTime::now();
    historyReadRawModifiedContext.startTime = timeSetting;
    timeSetting.addMilliSecs(-1800000);
    historyReadRawModifiedContext.endTime = timeSetting;
    historyReadRawModifiedContext.returnBounds = OpcUa_True;
    historyReadRawModifiedContext.numValuesPerNode = 100;

    // Read four aggregates from one node
    nodesToRead.create(1);
    nodeToRead.copyTo(&nodesToRead[0].NodeId);

    /*********************************************************************
     Update the history of events at an event notifier object
    **********************************************************************/
    status = g_pUaSession->historyReadRawModified(
        serviceSettings,
        historyReadRawModifiedContext,
        nodesToRead,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if (status.isBad()) {
        printf("** Error: UaSession::historyReadProcessed failed [ret=%s]\n", status.toString().toUtf8());
        return;
    } else {
        OpcUa_UInt32 i, j;
        for (i = 0; i < results.length(); i++) {
            UaStatus nodeResult(results[i].m_status);
            printf("** Results %d Node=%s status=%s\n", i, nodeToRead.toXmlString().toUtf8(),
                   nodeResult.toString().toUtf8());

            for (j = 0; j < results[i].m_dataValues.length(); j++) {
                UaStatus statusOPLevel(results[i].m_dataValues[j].StatusCode);
                UaDateTime sourceTS(results[i].m_dataValues[j].SourceTimestamp);
                if (OpcUa_IsGood(results[i].m_dataValues[j].StatusCode)) {
                    UaVariant tempValue = results[i].m_dataValues[j].Value;
                    printf("**    [%d] value %s ts %s status %s\n",
                           j,
                           tempValue.toString().toUtf8(),
                           sourceTS.toTimeString().toUtf8(),
                           statusOPLevel.toString().toUtf8());
                } else {
                    printf("**    [%d] status %s ts %s\n",
                           j,
                           statusOPLevel.toString().toUtf8(),
                           sourceTS.toTimeString().toUtf8());
                }
            }
        }
        printf("****************************************************************\n\n");

        while (results[0].m_continuationPoint.length() > 0) {
            printf("\n*******************************************************\n");
            printf("** More data available                          *******\n");
            printf("**        Press x to stop read                  *******\n");
            printf("**        Press c to continue                   *******\n");
            printf("*******************************************************\n");
            int action;
            /******************************************************************************/
            /* Wait for user command. */
            bool waitForInput = true;
            while (waitForInput) {
                if (WaitForKeypress(action)) {
                    // x -> Release continuation point
                    historyReadRawModifiedContext.bReleaseContinuationPoints = OpcUa_True;
                    waitForInput = false;
                }
                    // Continue
                else if (action == 12) waitForInput = false;
                    // Wait
                else usleep(100);
            }
            /******************************************************************************/

            OpcUa_ByteString_Clear(&nodesToRead[0].ContinuationPoint);
            results[0].m_continuationPoint.copyTo(&nodesToRead[0].ContinuationPoint);
            status = g_pUaSession->historyReadRawModified(
                serviceSettings,
                historyReadRawModifiedContext,
                nodesToRead,
                results,
                diagnosticInfos);
            if (status.isBad()) {
                printf("** Error: UaSession::historyReadProcessed with CP failed [ret=%s]\n",
                       status.toString().toUtf8());
                return;
            } else {
                for (i = 0; i < results.length(); i++) {
                    UaStatus nodeResult(results[i].m_status);
                    printf("** Results %d Node=%s status=%s\n", i, nodeToRead.toXmlString().toUtf8(),
                           nodeResult.toString().toUtf8());

                    for (j = 0; j < results[i].m_dataValues.length(); j++) {
                        UaStatus statusOPLevel(results[i].m_dataValues[j].StatusCode);
                        UaDateTime sourceTS(results[i].m_dataValues[j].SourceTimestamp);
                        if (OpcUa_IsGood(results[i].m_dataValues[j].StatusCode)) {
                            UaVariant tempValue = results[i].m_dataValues[j].Value;
                            printf("**    [%d] value %s ts %s status %s\n",
                                   j,
                                   tempValue.toString().toUtf8(),
                                   sourceTS.toTimeString().toUtf8(),
                                   statusOPLevel.toString().toUtf8());
                        } else {
                            printf("**    [%d] status %s ts %s\n",
                                   j,
                                   statusOPLevel.toString().toUtf8(),
                                   sourceTS.toTimeString().toUtf8());
                        }
                    }
                }
            }
        }
    }
}

/*============================================================================
 * historyReadDataProcessed - read processed data history
 *===========================================================================*/
void historyReadDataProcessed() {
    printf("\n\n****************************************************************\n");
    printf("** Try to read processed data history\n");
    if (g_pUaSession == NULL) {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }
    if (g_HistoryDataNodeIds.length() < 1) {
        printf("** Error: No history node configured\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus status;
    ServiceSettings serviceSettings;
    HistoryReadProcessedContext historyReadProcessedContext;
    UaHistoryReadValueIds nodesToRead;
    HistoryReadDataResults results;
    UaDiagnosticInfos diagnosticInfos;

    UaNodeId nodeToRead(g_HistoryDataNodeIds[0]);

    // Read the last 30 minutes
    UaDateTime timeSetting = UaDateTime::now();
    historyReadProcessedContext.startTime = timeSetting;
    timeSetting.addMilliSecs(-1800000);
    historyReadProcessedContext.endTime = timeSetting;
    // Calculate per minute
    historyReadProcessedContext.processingInterval = 60000;

    // Request Count, Minimum, Maximum and Average
    historyReadProcessedContext.aggregateTypes.create(4);
    historyReadProcessedContext.aggregateTypes[0].Identifier.Numeric = OpcUaId_AggregateFunction_Count;
    historyReadProcessedContext.aggregateTypes[1].Identifier.Numeric = OpcUaId_AggregateFunction_Minimum;
    historyReadProcessedContext.aggregateTypes[2].Identifier.Numeric = OpcUaId_AggregateFunction_Maximum;
    historyReadProcessedContext.aggregateTypes[3].Identifier.Numeric = OpcUaId_AggregateFunction_Average;

    // Read four aggregates from one node
    nodesToRead.create(4);
    nodeToRead.copyTo(&nodesToRead[0].NodeId);
    nodeToRead.copyTo(&nodesToRead[1].NodeId);
    nodeToRead.copyTo(&nodesToRead[2].NodeId);
    nodeToRead.copyTo(&nodesToRead[3].NodeId);

    /*********************************************************************
     Update the history of events at an event notifier object
    **********************************************************************/
    status = g_pUaSession->historyReadProcessed(
        serviceSettings,
        historyReadProcessedContext,
        nodesToRead,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if (status.isBad()) {
        printf("** Error: UaSession::historyReadProcessed failed [ret=%s]\n", status.toString().toUtf8());
        return;
    } else {
        OpcUa_UInt32 i, j;
        for (i = 0; i < results.length(); i++) {
            UaStatus nodeResult(results[i].m_status);
            if (i == 0)
                printf("** Results %d Node=%s Aggregate=Count status=%s\n", i, nodeToRead.toXmlString().toUtf8(),
                       nodeResult.toString().toUtf8());
            else if (i == 1)
                printf("** Results %d Node=%s Aggregate=Minimum status=%s\n", i, nodeToRead.toXmlString().toUtf8(),
                       nodeResult.toString().toUtf8());
            else if (i == 2)
                printf("** Results %d Node=%s Aggregate=Maximum status=%s\n", i, nodeToRead.toXmlString().toUtf8(),
                       nodeResult.toString().toUtf8());
            else if (i == 3)
                printf("** Results %d Node=%s Aggregate=Average status=%s\n", i, nodeToRead.toXmlString().toUtf8(),
                       nodeResult.toString().toUtf8());
            else printf("** Unexpected Results %d\n", i);

            for (j = 0; j < results[i].m_dataValues.length(); j++) {
                UaStatus statusOPLevel(results[i].m_dataValues[j].StatusCode);
                UaDateTime sourceTS(results[i].m_dataValues[j].SourceTimestamp);
                if (OpcUa_IsGood(results[i].m_dataValues[j].StatusCode)) {
                    UaVariant tempValue = results[i].m_dataValues[j].Value;
                    printf("**    [%d] value %s ts %s status %s\n",
                           j,
                           tempValue.toString().toUtf8(),
                           sourceTS.toTimeString().toUtf8(),
                           statusOPLevel.toString().toUtf8());
                } else {
                    printf("**    [%d] status %s ts %s\n",
                           j,
                           statusOPLevel.toString().toUtf8(),
                           sourceTS.toTimeString().toUtf8());
                }
            }
        }
    }
}

/*============================================================================
 * historyReadDataAtTime - read data history at timestamps
 *===========================================================================*/
void historyReadDataAtTime() {
    printf("\n\n****************************************************************\n");
    printf("** Try to read data history at timestamps \n");
    if (g_pUaSession == NULL) {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }
    if (g_HistoryDataNodeIds.length() < 1) {
        printf("** Error: No history node configured\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus status;
    ServiceSettings serviceSettings;
    HistoryReadAtTimeContext historyReadAtTimeContext;
    UaHistoryReadValueIds nodesToRead;
    HistoryReadDataResults results;
    UaDiagnosticInfos diagnosticInfos;
    OpcUa_UInt32 i, j, count;

    UaNodeId nodeToRead(g_HistoryDataNodeIds[0]);

    count = 10;

    // Read the values from the last 30 seconds
    historyReadAtTimeContext.useSimpleBounds = OpcUa_True;
    historyReadAtTimeContext.requestedTimes.create(count);
    UaDateTime timeSetting = UaDateTime::now();
    for (i = 0; i < count; i++) {
        timeSetting.addMilliSecs(-1000);
        timeSetting.copyTo(&historyReadAtTimeContext.requestedTimes[i]);
    }

    // Read at time from one node
    nodesToRead.create(1);
    nodeToRead.copyTo(&nodesToRead[0].NodeId);

    /*********************************************************************
     Update the history of events at an event notifier object
    **********************************************************************/
    status = g_pUaSession->historyReadAtTime(
        serviceSettings,
        historyReadAtTimeContext,
        nodesToRead,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if (status.isBad()) {
        printf("** Error: UaSession::historyReadAtTime failed [ret=%s]\n", status.toString().toUtf8());
        return;
    } else {
        for (i = 0; i < results.length(); i++) {
            UaStatus nodeResult(results[i].m_status);
            printf("** Results %d Node=%s status=%s\n", i, nodeToRead.toXmlString().toUtf8(),
                   nodeResult.toString().toUtf8());

            for (j = 0; j < results[i].m_dataValues.length(); j++) {
                if (j >= count) {
                    printf("**    More results returned than the %d requested\n", count);
                    break;
                }
                UaDateTime requTS(historyReadAtTimeContext.requestedTimes[j]);
                printf("**    [%d] Requested TS %s\n", j, requTS.toTimeString().toUtf8());
                UaStatus statusOPLevel(results[i].m_dataValues[j].StatusCode);
                UaDateTime sourceTS(results[i].m_dataValues[j].SourceTimestamp);
                if (OpcUa_IsGood(results[i].m_dataValues[j].StatusCode)) {
                    UaVariant tempValue = results[i].m_dataValues[j].Value;
                    printf("**         value %s ts %s status %s\n",
                           tempValue.toString().toUtf8(),
                           sourceTS.toTimeString().toUtf8(),
                           statusOPLevel.toString().toUtf8());
                } else {
                    printf("**         status %s ts %s\n",
                           statusOPLevel.toString().toUtf8(),
                           sourceTS.toTimeString().toUtf8());
                }
            }
        }
    }
}

/*============================================================================
 * historyUpdateData - update data history
 *===========================================================================*/
void historyUpdateData() {
    printf("\n\n****************************************************************\n");
    printf("** Try to update data history \n");
    if (g_pUaSession == NULL) {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    if (g_HistoryDataNodeIds.length() < 1) {
        printf("** Error: No history node configured\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus status;
    ServiceSettings serviceSettings;
    UpdateDataDetails updateDataDetails;
    UaHistoryUpdateResults results;
    UaDiagnosticInfos diagnosticInfos;
    UaVariant value;
    UaDateTime dtNow;

    // setup data to write into history
    updateDataDetails.create(1);
    updateDataDetails[0].m_nodeId = g_HistoryDataNodeIds[0];
    updateDataDetails[0].m_PerformInsertReplace = OpcUa_PerformUpdateType_Update;
    updateDataDetails[0].m_dataValues.create(10);
    dtNow = UaDateTime::now();
    dtNow.addSecs(-20);

    // fill values to write into history
    for (int i = 0; i < 10; i++) {
        value.setDouble(i * i);
        value.copyTo(&updateDataDetails[0].m_dataValues[i].Value);
        updateDataDetails[0].m_dataValues[i].SourceTimestamp = dtNow;
        updateDataDetails[0].m_dataValues[i].ServerTimestamp = dtNow;
        dtNow.addSecs(1);
    }

    /*********************************************************************
     Update the history of events at an event notifier object
    **********************************************************************/
    status = g_pUaSession->historyUpdateData(
        serviceSettings,
        updateDataDetails,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if (status.isBad()) {
        printf("** Error: UaSession::historyReadProcessed failed [ret=%s]\n", status.toString().toUtf8());
        return;
    } else {
        OpcUa_Int32 i;

        if (OpcUa_IsGood(results[0].StatusCode)) {
            printf("historyUpdateData returned:\n");
        } else {
            printf("historyUpdateData operation returned status = %s:\n",
                   UaStatus(results[0].StatusCode).toString().toUtf8());
        }

        for (i = 0; i < results[0].NoOfOperationResults; i++) {
            if (OpcUa_IsNotGood(results[0].OperationResults[i])) {
                printf("historyUpdateData operation[%d] failed with status = %s:\n", i,
                       UaStatus(results[0].OperationResults[i]).toString().toUtf8());
            }
        }
    }
}

/*============================================================================
 * historyDeleteData - delete data history
 *===========================================================================*/
void historyDeleteData() {
    printf("\n\n****************************************************************\n");
    printf("** Try to delete data history \n");
    if (g_pUaSession == NULL) {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    // delete the last 30 seoncdes of the history
    UaStatus status;
    ServiceSettings serviceSettings;
    DeleteRawModifiedDetails deleteDetails;
    UaHistoryUpdateResults results;
    UaDiagnosticInfos diagnosticInfos;

    deleteDetails.create(1);
    deleteDetails[0].m_startTime = UaDateTime::now();
    deleteDetails[0].m_startTime.addSecs(-30);
    deleteDetails[0].m_endTime = UaDateTime::now();
    deleteDetails[0].m_nodeId = g_HistoryDataNodeIds[0];
    deleteDetails[0].m_IsDeleteModified = OpcUa_False;

    /*********************************************************************
     Delete part of the history
    **********************************************************************/
    status = g_pUaSession->historyDeleteRawModified(
        serviceSettings, // Use default settings
        deleteDetails,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if (status.isBad()) {
        printf("** Error: UaSession::historyDeleteRawModified failed [ret=%s]\n", status.toString().toUtf8());
        return;
    } else {
        if (OpcUa_IsGood(results[0].StatusCode)) {
            printf("historyDeleteRawModified succeeeded.\n");
        } else {
            printf("historyDeleteRawModified operation returned status = %s:\n",
                   UaStatus(results[0].StatusCode).toString().toUtf8());
        }

        OpcUa_Int32 i;
        for (i = 0; i < results[0].NoOfOperationResults; i++) {
            if (OpcUa_IsNotGood(results[0].OperationResults[i])) {
                printf("historyDeleteRawModified operation[%d] failed with status = %s:\n", i,
                       UaStatus(results[0].OperationResults[i]).toString().toUtf8());
            }
        }
    }
}

/*============================================================================
 * historyReadEvents - read event history
 *===========================================================================*/
void historyReadEvents() {
    printf("\n\n****************************************************************\n");
    printf("** Try to read event history \n");
    if (g_pUaSession == NULL) {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    // call methods to generate events
    UaStatus status;
    ServiceSettings serviceSettings;
    CallIn callRequest;
    CallOut callResult;

    if (g_EventIds.length() < 3) {
        // Call Methods to generate events
        callRequest.objectId = g_EventTriggerObjects[0];
        callRequest.methodId = g_EventTriggerMethods[0];
        status = g_pUaSession->call(
            serviceSettings,    // Use default settings
            callRequest,        // In parameters and settings for the method call
            callResult);        // Out parameters and results returned from the method call
        if (status.isBad()) {
            printf("** Error: UaSession::call failed [ret=%s]\n", status.toString().toUtf8());
        }
        // Call Methods to generate events
        callRequest.objectId = g_EventTriggerObjects[1];
        callRequest.methodId = g_EventTriggerMethods[1];
        status = g_pUaSession->call(
            serviceSettings,    // Use default settings
            callRequest,        // In parameters and settings for the method call
            callResult);        // Out parameters and results returned from the method call
        if (status.isBad()) {
            printf("** Error: UaSession::call failed [ret=%s]\n", status.toString().toUtf8());
        }
    }

    HistoryReadEventContext historyReadEventContext;
    UaHistoryReadValueIds nodesToRead;
    HistoryReadEventResults results;
    UaDiagnosticInfos diagnosticInfos;
    UaSimpleAttributeOperand selectElement;

    nodesToRead.create(1);
    g_HistoryEventNodeId.copyTo(&nodesToRead[0].NodeId);

    // show events of last hour
    historyReadEventContext.startTime = UaDateTime::now();
    historyReadEventContext.endTime = UaDateTime::now();
    historyReadEventContext.startTime.addMilliSecs(-3600000);
    historyReadEventContext.numValuesPerNode = 10;

    // Define select clause with 4 event fields to be returned with every event
    selectElement.setBrowsePathElement(0, UaQualifiedName("Time", 0), 1);
    historyReadEventContext.eventFilter.setSelectClauseElement(0, selectElement, 4);
    selectElement.setBrowsePathElement(0, UaQualifiedName("Message", 0), 1);
    historyReadEventContext.eventFilter.setSelectClauseElement(1, selectElement, 4);
    selectElement.setBrowsePathElement(0, UaQualifiedName("SourceName", 0), 1);
    historyReadEventContext.eventFilter.setSelectClauseElement(2, selectElement, 4);
    selectElement.setBrowsePathElement(0, UaQualifiedName("EventId", 0), 1);
    historyReadEventContext.eventFilter.setSelectClauseElement(3, selectElement, 4);

    /*********************************************************************
     Read the history of events from an event notifier object
    **********************************************************************/
    status = g_pUaSession->historyReadEvent(
        serviceSettings,
        historyReadEventContext,
        nodesToRead,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if (status.isBad()) {
        printf("** Error: UaSession::historyReadEvent failed [ret=%s]\n", status.toString().toUtf8());
        return;
    } else {
        // save the eventIds for later use in historyDeleteEvents
        g_EventIds.clear();
        g_EventIds.create(results[0].m_events.length());
        OpcUa_UInt32 i;
        OpcUa_UInt32 j;

        if (results[0].m_status.isGood()) {
            printf("HistoryReadEvent returned:\n");
        } else {
            printf("HistoryReadEvent operation returned status = %s:\n", results[0].m_status.toString().toUtf8());
        }
        j = 0;
        for (i = 0; i < results[0].m_events.length(); i++) {
            if (results[0].m_events[i].NoOfEventFields == 4) {
                UaVariant uvTime(results[0].m_events[i].EventFields[0]);
                UaVariant uvMessage(results[0].m_events[i].EventFields[1]);
                UaVariant uvSource(results[0].m_events[i].EventFields[2]);
                printf("%s %s %s \n", uvTime.toString().toUtf8(), uvSource.toString().toUtf8(),
                       uvMessage.toString().toUtf8());

                UaVariant uvEventId(results[0].m_events[i].EventFields[3]);
                UaByteString bsEventId;
                if (OpcUa_IsGood(uvEventId.toByteString(bsEventId))) {
                    bsEventId.copyTo(&g_EventIds[j]);
                    j++;
                }
            } else {
                printf("Invalid event\n");
            }
        }
        g_EventIds.resize(j);

        while (results[0].m_continuationPoint.length() > 0) {
            printf("\n*******************************************************\n");
            printf("** More data available                          *******\n");
            printf("**        Press x to stop read                  *******\n");
            printf("**        Press c to continue                   *******\n");
            printf("*******************************************************\n");
            int action;
            /******************************************************************************/
            /* Wait for user command. */
            bool waitForInput = true;
            while (waitForInput) {
                if (WaitForKeypress(action)) {
                    // x -> Release continuation point
                    historyReadEventContext.bReleaseContinuationPoints = OpcUa_True;
                    waitForInput = false;
                }
                    // Continue
                else if (action == 12) waitForInput = false;
                    // Wait
                else usleep(100);
            }
            /******************************************************************************/

            OpcUa_ByteString_Clear(&nodesToRead[0].ContinuationPoint);
            results[0].m_continuationPoint.copyTo(&nodesToRead[0].ContinuationPoint);
            status = g_pUaSession->historyReadEvent(
                serviceSettings,
                historyReadEventContext,
                nodesToRead,
                results,
                diagnosticInfos);
            if (status.isBad()) {
                printf("** Error: UaSession::historyReadEvent with CP failed [ret=%s]\n", status.toString().toUtf8());
                return;
            } else {
                g_EventIds.resize(results[0].m_events.length() + g_EventIds.length());
                if (results[0].m_status.isGood()) {
                    printf("HistoryReadEvent returned:\n");
                } else {
                    printf("HistoryReadEvent operation returned status = %s:\n",
                           results[0].m_status.toString().toUtf8());
                }
                for (i = 0; i < results[0].m_events.length(); i++) {
                    if (results[0].m_events[i].NoOfEventFields == 4) {
                        UaVariant uvTime(results[0].m_events[i].EventFields[0]);
                        UaVariant uvMessage(results[0].m_events[i].EventFields[1]);
                        UaVariant uvSource(results[0].m_events[i].EventFields[2]);
                        printf("%s %s %s \n", uvTime.toString().toUtf8(), uvSource.toString().toUtf8(),
                               uvMessage.toString().toUtf8());

                        UaVariant uvEventId(results[0].m_events[i].EventFields[3]);
                        UaByteString bsEventId;
                        if (OpcUa_IsGood(uvEventId.toByteString(bsEventId))) {
                            bsEventId.copyTo(&g_EventIds[j]);
                            j++;
                        }
                    } else {
                        printf("Invalid event\n");
                    }
                }
                g_EventIds.resize(j);
            }
        }
    }
}

/*============================================================================
 * historyUpdateEvents - update event history
 *===========================================================================*/
void historyUpdateEvents() {
    printf("\n\n****************************************************************\n");
    printf("** Try to update event history \n");
    if (g_pUaSession == NULL) {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    UaStatus status;
    ServiceSettings serviceSettings;
    UpdateEventDetails updateEventDetails;
    UaHistoryUpdateResults results;
    UaDiagnosticInfos diagnosticInfos;
    UaSimpleAttributeOperand selectElement;
    OpcUa_Int32 i;

    updateEventDetails.create(1);
    updateEventDetails[0].m_PerformInsertReplace = OpcUa_PerformUpdateType_Update;
    updateEventDetails[0].m_nodeId = g_HistoryEventNodeId;

    // Define select clause with 4 event fields to update
    selectElement.setBrowsePathElement(0, UaQualifiedName("Time", 0), 1);
    updateEventDetails[0].m_eventFilter.setSelectClauseElement(0, selectElement, 4);
    selectElement.setBrowsePathElement(0, UaQualifiedName("Message", 0), 1);
    updateEventDetails[0].m_eventFilter.setSelectClauseElement(1, selectElement, 4);
    selectElement.setBrowsePathElement(0, UaQualifiedName("SourceName", 0), 1);
    updateEventDetails[0].m_eventFilter.setSelectClauseElement(2, selectElement, 4);
    selectElement.setBrowsePathElement(0, UaQualifiedName("EventType", 0), 1);
    updateEventDetails[0].m_eventFilter.setSelectClauseElement(3, selectElement, 4);

    // add 3 events with 4 fields each
    updateEventDetails[0].m_eventData.create(3);
    UaVariantArray eventFields;
    eventFields.create(4);
    UaVariant value;
    UaDateTime dateTime = UaDateTime::now();
    UaHistoryEventFieldList eventFieldList;
    dateTime.addSecs(-10);

    for (i = 0; i < 3; i++) {
        // set time
        dateTime.addSecs(1);
        value.setDateTime(dateTime);
        value.copyTo(&eventFields[0]);
        // set messsage
        value.setLocalizedText(UaLocalizedText("en", UaString("UpdateMessage_%1").arg(i + 1)));
        value.copyTo(&eventFields[1]);
        // set source name
        value.setString(UaString("AirConditioner_%1").arg(i + 1));
        value.copyTo(&eventFields[2]);
        // set event type - we use base event type here
        value.setNodeId(UaNodeId(OpcUaId_BaseEventType, 0));
        value.copyTo(&eventFields[3]);
        eventFieldList.setEventFields(eventFields);
        OpcUa_HistoryEventFieldList_Initialize(&updateEventDetails[0].m_eventData[i]);
        eventFieldList.copyTo(&updateEventDetails[0].m_eventData[i]);
    }

    /*********************************************************************
     Update the history of events at an event notifier object
    **********************************************************************/
    status = g_pUaSession->historyUpdateEvents(
        serviceSettings, // Use default settings
        updateEventDetails,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if (status.isBad()) {
        printf("** Error: UaSession::historyUpdateEvents failed [ret=%s]\n", status.toString().toUtf8());
        return;
    } else {
        if (OpcUa_IsGood(results[0].StatusCode)) {
            printf("historyUpdateEvents succeeded\n");

            for (i = 0; i < results[0].NoOfOperationResults; i++) {
                if (OpcUa_IsNotGood(results[0].OperationResults[i])) {
                    printf("OperationResults[%d] = %s:\n", i,
                           UaStatus(results[0].OperationResults[i]).toString().toUtf8());
                }
            }
        } else {
            printf("historyUpdateEvents operation returned status = %s:\n",
                   UaStatus(results[0].StatusCode).toString().toUtf8());
        }
    }
}

/*============================================================================
 * historyDeleteEvents - delete event history
 *===========================================================================*/
void historyDeleteEvents() {
    printf("\n\n****************************************************************\n");
    printf("** Try to delete event history \n");
    if (g_pUaSession == NULL) {
        printf("** Error: Server not connected\n");
        printf("****************************************************************\n");
        return;
    }

    // we need some data to delete - the eventIds are filled in void historyReadEvents
    if (g_EventIds.length() < 3) {
        historyReadEvents();
    }

    if (g_EventIds.length() < 3) {
        printf("\n\n****************************************************************\n");
        printf("** Error: Not enough event history available - need at least 3 events\n");
        printf("****************************************************************\n");
        return;
    }

    // delete first 2 entries in the event history
    UaStatus status;
    ServiceSettings serviceSettings;
    DeleteEventDetails deleteDetails;
    UaHistoryUpdateResults results;
    UaDiagnosticInfos diagnosticInfos;

    deleteDetails.create(1);
    deleteDetails[0].m_eventIds.create(2);
    deleteDetails[0].m_nodeId = g_HistoryEventNodeId;
    UaByteString::cloneTo(g_EventIds[0], deleteDetails[0].m_eventIds[0]);
    UaByteString::cloneTo(g_EventIds[1], deleteDetails[0].m_eventIds[1]);
    UaByteStringArray bsTmpArray = g_EventIds;
    g_EventIds.clear();
    g_EventIds.resize(bsTmpArray.length() - 2);
    for (OpcUa_UInt32 i = 2; i < bsTmpArray.length(); i++) {
        g_EventIds[i - 2] = bsTmpArray[i];
    }
    bsTmpArray.detach();

    /*********************************************************************
     Delete part of the event history
    **********************************************************************/
    status = g_pUaSession->historyDeleteEvents(
        serviceSettings, // Use default settings
        deleteDetails,
        results,
        diagnosticInfos);
    /*********************************************************************/
    if (status.isBad()) {
        printf("** Error: UaSession::historyDeleteEvents failed [ret=%s]\n", status.toString().toUtf8());
        return;
    } else {
        OpcUa_Int32 i;

        if (OpcUa_IsGood(results[0].StatusCode)) {
            printf("historyDeleteEvents returned:\n");
        } else {
            printf("historyDeleteEvents operation returned status = %s:\n",
                   UaStatus(results[0].StatusCode).toString().toUtf8());
        }

        for (i = 0; i < results[0].NoOfOperationResults; i++) {
            if (OpcUa_IsNotGood(results[0].OperationResults[i])) {
                printf("historyDeleteEvents operation[%d] failed with status = %s:\n", i,
                       UaStatus(results[0].OperationResults[i]).toString().toUtf8());
            }
        }
    }
}