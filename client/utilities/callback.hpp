#ifndef __CALLBACK_H__
#define __CALLBACK_H__

#include "uaclient/uaclientsdk.h"
#include "uaclient/uasession.h"
#include <vector>
#include "uagenericunionvalue.h"


class callback:
    public UaClientSdk::UaSessionCallback,
    public UaClientSdk::UaSubscriptionCallback
{
public:
    callback(){};
    virtual ~callback(){};

    /** Status of the server connection. */
    virtual void connectionStatusChanged(
        OpcUa_UInt32              clientConnectionId,
        UaClientSdk::UaClient::ServerStatus serverStatus)
    {
        UaString sStatus;
        switch ( serverStatus )
        {
            case UaClientSdk::UaClient::Disconnected: sStatus="Disconnected"; break;
            case UaClientSdk::UaClient::Connected: sStatus="Connected"; break;
            case UaClientSdk::UaClient::ConnectionWarningWatchdogTimeout: sStatus="ConnectionWarningWatchdogTimeout"; break;
            case UaClientSdk::UaClient::ConnectionErrorApiReconnect: sStatus="ConnectionErrorApiReconnect"; break;
            case UaClientSdk::UaClient::ServerShutdown: sStatus="ServerShutdown"; break;
            case UaClientSdk::UaClient::NewSessionCreated: sStatus="NewSessionCreated"; break;
        }
        spdlog::info("-- Event connectionStatusChanged ----------------------");
        spdlog::info("clientConnectionId {} ", clientConnectionId);
        spdlog::info("serverStatus {} ", sStatus.toUtf8());
        spdlog::info("-------------------------------------------------------");
    }

    /** Connect errors. */
    bool connectError(
        OpcUa_UInt32                    clientConnectionId, //!< [in] Client defined handle of the affected session
        UaClientSdk::UaClient::ConnectServiceType    serviceType, //!< [in] The failing connect step
        const UaStatus&                 error,       //!< [in] Status code for the error situation
        bool                            clientSideError) //!< [in] Flag indicating if the bad status was created in the Client SDK
    {
        UaString sServiceType;
        switch ( serviceType )
        {
            case UaClientSdk::UaClient::CertificateValidation: sServiceType="Certificate validation steps"; break;
            case UaClientSdk::UaClient::OpenSecureChannel: sServiceType="Processing of Service OpenSecureChannel"; break;
            case UaClientSdk::UaClient::CreateSession: sServiceType="Processing of Service CreateSession"; break;
            case UaClientSdk::UaClient::ActivateSession: sServiceType="Processing of Service CreateSession"; break;
        }
        spdlog::error("-- Event connectError ---------------------------------");
        spdlog::error("clientConnectionId {} ", clientConnectionId);
        spdlog::error("Service Type {} ", sServiceType.toUtf8());
        spdlog::error("Status {} ", error.toString().toUtf8());
        spdlog::error("-------------------------------------------------------");

//        if ( clientSideError )
//        {
//            spdlog::info("Press 1 to stop connect\n");
//            spdlog::info("Press 2 to overwrite the error and continue\n");
//            spdlog::info("-------------------------------------------------------\n");
//            int action;
//            while (!WaitForKeypress(action))
//            {
//                if ( action == 1 )
//                {
//                    return false;
//                }
//                if ( action == 2 )
//                {
//                    return true;
//                }
//            }
//        }
        return false;
    }

    /** Send changed data. */
    virtual void dataChange(
        OpcUa_UInt32               clientSubscriptionHandle,
        const UaDataNotifications& dataNotifications,
        const UaDiagnosticInfos&   /*diagnosticInfos*/)
    {
        OpcUa_UInt32 i = 0;
        spdlog::info("-- Event dataChange -----------------------------------");
        spdlog::info("clientSubscriptionHandle {} ", clientSubscriptionHandle);
        for ( i=0; i<dataNotifications.length(); i++ )
        {
            if ( OpcUa_IsGood(dataNotifications[i].Value.StatusCode) )
            {
                UaVariant tempValue = dataNotifications[i].Value.Value;

                spdlog::info("  Variable {}} value = {} ", dataNotifications[i].ClientHandle, tempValue.toString().toUtf8());

            }
            else
            {
                spdlog::error("  Variable {} failed!", i);
            }
        }
        spdlog::info("-------------------------------------------------------");
    }

    /** Send subscription state change. */
    virtual void subscriptionStatusChanged(
        OpcUa_UInt32      clientSubscriptionHandle,
        const UaStatus&   status)
    {
        spdlog::info("-- Event subscriptionStatusChanged ----------------------");
        spdlog::info("clientSubscriptionHandle {} ", clientSubscriptionHandle);
        spdlog::info("subscriptionStatus {} ", status.toString().toUtf8());
        spdlog::info("-------------------------------------------------------");
    }

    /** Send new events. */
    virtual void newEvents(
        OpcUa_UInt32          clientSubscriptionHandle,
        UaEventFieldLists&    eventFieldList)
    {
        OpcUa_UInt32 i = 0;
        OpcUa_Int32 j = 0;
        spdlog::info("-- Event newEvents ------------------------------------");
        spdlog::info("clientSubscriptionHandle {}", clientSubscriptionHandle);
        for ( i=0; i<eventFieldList.length(); i++ )
        {
            // Print event message text first
            UaVariant message    = eventFieldList[i].EventFields[0];
            spdlog::info("Event[{}] Message = {} ",
                i,
                message.toString().toUtf8());

            // Print other requested event fields in a list
            // Event field names must be set in m_eventFields before creating the event monitored item
            for ( j=1; j<eventFieldList[i].NoOfEventFields; j++ )
            {
                if ( (OpcUa_Int32)(m_eventFields.length() + 1) < j )
                {
                    spdlog::warn("  More event fields delivered than requested");
                }
                UaVariant tempValue = eventFieldList[i].EventFields[j];
//                if (tempValue.type() == OpcUaType_ExtensionObject)
//                {
//                    printExtensionObjects(tempValue, UaString(m_eventFields[j-1]));
//                }
                if (tempValue.type() == OpcUaType_Null)
                {
                    // Special handling for NULL values
                    spdlog::info("  {} = NULL ", UaString(m_eventFields[j-1]).toUtf8());
                }
                else
                {
                    spdlog::info("  {} = {} ", UaString(m_eventFields[j-1]).toUtf8(), tempValue.toString().toUtf8());
                }
            }
        }
        spdlog::info("-------------------------------------------------------\n");
    }

    /** Send read results. */
    virtual void readComplete(
        const UaStatus&          result,
        const UaDataValues&      values ,
        const UaDiagnosticInfos& diagnosticInfos)
    {
        spdlog::info("-- Event readComplete ---------------------------------");
        spdlog::info("transactionId {} ", m_TransactionId);
        spdlog::info("resultStatus {} ", result.toString().toUtf8());
        for ( OpcUa_UInt32 i=0; i<values.length(); i++ )
        {
            if ( OpcUa_IsGood(values[i].StatusCode) )
            {
                UaVariant tempValue = values[i].Value;

                    spdlog::info("  Variable {} value = {} ", i, tempValue.toString().toUtf8());
            }
            else
            {
                spdlog::error("  Variable {} failed with error {}", i, UaStatus(values[i].StatusCode).toString().toUtf8());
            }
        }
        spdlog::info("-------------------------------------------------------");
    }

    /** Send write results. */
    virtual void writeComplete(
        const UaStatus&          result,
        const UaStatusCodeArray& results,
        const UaDiagnosticInfos& diagnosticInfos)
    {
        spdlog::info("-- Event writeComplete --------------------------------");
        spdlog::info("transactionId {} ", m_TransactionId);
        spdlog::info("resultStatus {}", result.toString().toUtf8());
        for ( OpcUa_UInt32 i=0; i<results.length(); i++ )
        {
            if ( OpcUa_IsGood(results[i]) )
            {
                spdlog::info("  Variable {} succeeded!", i);
            }
            else
            {
                spdlog::error("  Variable {} failed with error {}", i, UaStatus(results[i]).toString().toUtf8());
            }
        }
        spdlog::info("-------------------------------------------------------");
    }
    virtual void finishCall(OpcUa_UInt32       callbackHandle,
                    UaStatusCodeArray& inputArgumentResults,
                    UaDiagnosticInfos& inputArgumentDiag,
                    UaVariantArray&    outputArguments,
                    UaStatus&          statusCode)
    {
        spdlog::info("-- Event finishCall --------------------------------");
        spdlog::info("\ttransactionId {}", m_TransactionId);
        spdlog::info("\tresultStatus {}", statusCode.toString().toUtf8());
        spdlog::info("\tcallbackHandle {}", callbackHandle);
        // this should duplicate the behavior of the synchronous callMethod() after the method
        // call succeeds. But we're not doing anything there for now, so this too is empty
    }

    UaStringArray m_eventFields;
protected:
    // keep track of asynchronous calls
    OpcUa_UInt32 m_TransactionId;
};

class EventObject
{
public:
    EventObject(){};

    UaByteString    eventId;
    UaNodeId        eventType;
    UaString        sourceName;
    UaDateTime      time;
    UaLocalizedText message;
    OpcUa_UInt16    severity;
    UaNodeId        branchId;
    UaString        conditionName;
    OpcUa_Boolean   retain;
    OpcUa_Boolean   ackedStateId;
    OpcUa_Boolean   confirmedStateId;
    UaLocalizedText activeState;
    OpcUa_Boolean   activeStateId;
    UaLocalizedText activeStateEffectiveDisplayName;
    UaNodeId        conditionNodeId;
};

class CallbackAlarms:
    public UaClientSdk::UaSubscriptionCallback
{
public:
    CallbackAlarms()
    {
        m_bTableChanged = false;
    }
    virtual ~CallbackAlarms(){}

    /** Send changed data. */
    virtual void dataChange(
        OpcUa_UInt32               clientSubscriptionHandle,
        const UaDataNotifications& /*dataNotifications*/,
        const UaDiagnosticInfos&   /*diagnosticInfos*/)
    {
        spdlog::info("-- Event dataChange -----------------------------------");
        spdlog::info("clientSubscriptionHandle {}", clientSubscriptionHandle);
        spdlog::info("-------------------------------------------------------");
    }

    /** Send subscription state change. */
    virtual void subscriptionStatusChanged(
        OpcUa_UInt32      clientSubscriptionHandle,
        const UaStatus&   status)
    {
        spdlog::info("-- Event subscriptionStatusChanged ----------------------");
        spdlog::info("clientSubscriptionHandle {} ", clientSubscriptionHandle);
        spdlog::info("serverStatus {}", status.toString().toUtf8());
        spdlog::info("-------------------------------------------------------");
    };

    /** Send new events. */
    virtual void newEvents(
        OpcUa_UInt32          clientSubscriptionHandle,
        UaEventFieldLists&    eventFieldList)
    {
        spdlog::info("-- Event newEvents ------------------------------------");
        spdlog::info("clientSubscriptionHandle {} ", clientSubscriptionHandle);
        spdlog::info("Number of new events {} ", eventFieldList.length());

        // save list of EventObjects
        OpcUa_UInt32 i = 0;
        for ( i=0; i<eventFieldList.length(); i++ )
        {
            EventObject newEvent;
            UaVariant sourceName = eventFieldList[i].EventFields[2];
            UaVariant message    = eventFieldList[i].EventFields[4];
            UaVariant uvActive   = eventFieldList[i].EventFields[11];
            UaVariant uvAcked    = eventFieldList[i].EventFields[9];

            // check if eventType and conditionNodeId is set
            if ( OpcUa_IsGood(UaVariant(eventFieldList[i].EventFields[1]).toNodeId(newEvent.eventType)) && OpcUa_IsGood(UaVariant(eventFieldList[i].EventFields[14]).toNodeId(newEvent.conditionNodeId)) )
            {
                // get rest of the event fields
                UaVariant(eventFieldList[i].EventFields[0]).toByteString(newEvent.eventId);
                newEvent.sourceName = UaVariant(eventFieldList[i].EventFields[2]).toString();
                UaVariant(eventFieldList[i].EventFields[3]).toDateTime(newEvent.time);
                UaVariant(eventFieldList[i].EventFields[4]).toLocalizedText(newEvent.message);
                UaVariant(eventFieldList[i].EventFields[5]).toUInt16(newEvent.severity);
                UaVariant(eventFieldList[i].EventFields[6]).toNodeId(newEvent.branchId);
                newEvent.conditionName = UaVariant(eventFieldList[i].EventFields[7]).toString();
                UaVariant(eventFieldList[i].EventFields[8]).toBool(newEvent.retain);
                UaVariant(eventFieldList[i].EventFields[9]).toBool(newEvent.ackedStateId);
                UaVariant(eventFieldList[i].EventFields[10]).toBool(newEvent.confirmedStateId);
                UaVariant(eventFieldList[i].EventFields[11]).toLocalizedText(newEvent.activeState);
                UaVariant(eventFieldList[i].EventFields[12]).toBool(newEvent.activeStateId);
                UaVariant(eventFieldList[i].EventFields[13]).toLocalizedText(newEvent.activeStateEffectiveDisplayName);
                UaVariant(eventFieldList[i].EventFields[14]).toNodeId(newEvent.conditionNodeId);

                // add event to internal list
                addEvent(newEvent);
                spdlog::info("Event[{}] SourceName={} | {} | Acked={} | Message={} \n",
                    i,
                    sourceName.toString().toUtf8(),
                    uvActive.toString().toUtf8(),
                    uvAcked.toString().toUtf8(),
                    message.toString().toUtf8());
            }
            else
            {
                if ( newEvent.eventType == OpcUaId_RefreshStartEventType )
                {
                    spdlog::info("Event[{}] Refresh Start Event ", i);
                }
                else if ( newEvent.eventType == OpcUaId_RefreshEndEventType )
                {
                    spdlog::info("Event[{}] Refresh End Event ", i);
                }
                else
                {
                    spdlog::info("Event[{}] Skipped - can not get event type of conditionId - SourceName = {} Message = {} ",
                        i,
                        sourceName.toString().toUtf8(),
                        message.toString().toUtf8());
                }
                // skip this event
            }
        }
        spdlog::info("-------------------------------------------------------");

        // print alarms
        if (m_bTableChanged)
        {
            spdlog::info("-- Alarm list -----------------------------------------");
            printAlarmList();
            spdlog::info("-------------------------------------------------------");
            spdlog::info("|     Press x to stop subscription                    |");
            spdlog::info("|     Press a to acknowledge all                      |");
            spdlog::info("|     Press c to confirm all                          |");
            spdlog::info("|     Press number to acknowledge / confirm single    |");
            spdlog::info("-------------------------------------------------------");
        }
    }

    void addEvent(const EventObject &newEvent)
    {
        // if retain is false delete from list
        if (newEvent.retain == OpcUa_False)
        {
            // find event in list
            std::map<UaNodeId, EventObject>::iterator it = m_mapAlarms.find(newEvent.conditionNodeId);
            if ( it != m_mapAlarms.end() )
            {
                // remove alarms
                m_mapAlarms.erase(it);
                m_bTableChanged = true;
            }
        }
        else
        {
            // for now we don't show branches
            if (newEvent.branchId.isNull())
            {
                // find event in list
                std::map<UaNodeId, EventObject>::iterator it = m_mapAlarms.find(newEvent.conditionNodeId);
                if ( it != m_mapAlarms.end() )
                {
                    // update alarm
                    m_mapAlarms.erase(it);
                    m_mapAlarms.insert(std::pair<UaNodeId, EventObject>(newEvent.conditionNodeId, newEvent));
                    m_bTableChanged = true;
                }
                else
                {
                    // add new alarm
                    m_mapAlarms.insert(std::pair<UaNodeId, EventObject>(newEvent.conditionNodeId, newEvent));
                    m_bTableChanged = true;
                }
            }
        }
    }

    void printAlarmList()
    {
        OpcUa_UInt16 i = 0;
        std::map<UaNodeId, EventObject>::iterator it = m_mapAlarms.begin();

        spdlog::info("# | Ack | Conf | Time | Severity | Source | Message | CondName");

        while (it != m_mapAlarms.end())
        {
            spdlog::info("[{}] | {} | {} | {} | {} | {} | {} | {}",
            i++,
            it->second.ackedStateId,
            it->second.confirmedStateId,
            it->second.time.toString().toUtf8(),
            it->second.severity,
            it->second.sourceName.toUtf8(),
            it->second.message.toString().toUtf8(),
            it->second.conditionName.toUtf8());
            it++;
        }

        spdlog::info("-------------------------------------------------------");
        m_bTableChanged = false;
    }

    std::vector<EventObject> getEventList()
    {
        std::vector<EventObject> result;

        std::map<UaNodeId, EventObject>::iterator it = m_mapAlarms.begin();

        while (it != m_mapAlarms.end())
        {
            result.push_back(it->second);
            it++;
        }

        return result;
    }

private:
    std::map<UaNodeId, EventObject> m_mapAlarms;
    bool m_bTableChanged;
};

#endif // CALLBACK_H
