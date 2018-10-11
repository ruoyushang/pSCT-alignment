#include "uaclientsdk.h"
#include "uasession.h"
#include <vector>
#include "demo_vector.h"
#include "demo_uniontest.h"
#include "uagenericunionvalue.h"
using namespace UaClientSdk;

UaSession* g_pUaSession;
bool WaitForKeypress(int& action);
void printGenericUnionValue(const UaGenericUnionValue &value, int level);
void printGenericStructureValue(const UaGenericStructureValue &value, int level)
{
    UaStructureDefinition definition = value.definition();
    for (int i=0; i<definition.childrenCount(); i++)
    {
        int lIdx;
        UaStructureField field = definition.child(i);
        UaString sFieldName = field.name().toUtf8();
        if (value.value(i).type() == OpcUaType_ExtensionObject)
        {
            UaStructureDefinition definitionChild = field.structureDefinition();
            for (lIdx=0;lIdx<level;lIdx++) printf("  ");
            // Get structure field ExtensionObject
            if (field.arrayType() == UaStructureField::ArrayType_Scalar)
            {
                if (!definitionChild.isUnion())
                {
                    //structure
                    if (!field.isOptional() || value.isFieldSet(i))
                    {
                        printf("    %s%s = structure value:\n",
                               sFieldName.toUtf8(),
                               field.isOptional() ? " [optional]" : "");
                        printGenericStructureValue(value.genericStructure(i), level+1);
                    }
                }
                else
                {
                    //union
                    if (!field.isOptional() || value.isFieldSet(i))
                    {
                        printf("    %s%s = union value:\n",
                               sFieldName.toUtf8(),
                               field.isOptional() ? " [optional]" : "");
                        printGenericUnionValue(value.genericUnion(i), level+1);
                    }
                }
            }
            else
            {
                if (!definitionChild.isUnion())
                {
                    if (!field.isOptional() || value.isFieldSet(i))
                    {
                        UaGenericStructureArray childValues = value.genericStructureArray(i);
                        printf("    %s%s\n",
                               sFieldName.toUtf8(),
                               field.isOptional() ? " [optional]" : "");
                        for (OpcUa_UInt32 u=0; u<childValues.length(); u++)
                        {
                            for (lIdx=0;lIdx<level+1;lIdx++) printf("  ");
                            printf("    %s\n", field.structureDefinition().name().toUtf8());
                            printGenericStructureValue(childValues[u], level+2);
                        }
                    }
                }
                else if (!field.isOptional() || value.isFieldSet(i))
                {
                    //union
                    UaGenericUnionArray childValues = value.genericUnionArray(i);
                    printf("    %s%s\n",
                            sFieldName.toUtf8(),
                            field.isOptional() ? " [optional]" : "");
                    for (OpcUa_UInt32 u=0; u<childValues.length(); u++)
                    {
                        for (lIdx=0;lIdx<level+1;lIdx++) printf("  ");
                        printf("    %s\n", field.structureDefinition().name().toUtf8());
                        printGenericUnionValue(childValues[u], level+2);
                    }
                }
            }
        }
        else if (!field.isOptional() || value.isFieldSet(i))
        {
            for (lIdx=0;lIdx<level;lIdx++) printf("  ");
            // Get field value
            UaVariant vFieldValue = value.value(i);
            printf("    %s %s= %s\n",
                sFieldName.toUtf8(),
                field.isOptional() ? "[Optional] " : "",
                vFieldValue.toString().toUtf8());
        }
    }
}

void printGenericUnionValue(const UaGenericUnionValue &value, int level)
{
    UaStructureDefinition definition = value.definition();
    int lIdx;
    int switchValue = value.switchValue();
    if (0 == switchValue)
    {
        for (lIdx=0;lIdx<level;lIdx++) printf("  ");
        printf("    union value not set\n");
    }
    else
    {
        UaStructureField field = definition.child(switchValue-1);
        UaString sFieldName = field.name().toUtf8();
        if (value.value().type() == OpcUaType_ExtensionObject)
        {
            UaStructureDefinition definitionChild = field.structureDefinition();
            for (lIdx=0;lIdx<level;lIdx++) printf("  ");
            // Get structure field ExtensionObject
            if (field.arrayType() == UaStructureField::ArrayType_Scalar)
            {
                if (!definitionChild.isUnion())
                {
                    //structure
                    printf("    %s (union) = structure value:\n",
                            sFieldName.toUtf8());
                    printGenericStructureValue(value.genericStructure(), level+1);
                }
                else
                {
                    printf("    %s (union) = union value:\n",
                            sFieldName.toUtf8());
                    printGenericUnionValue(value.genericUnion(), level+1);
                }
            }
            else
            {
                if (!definitionChild.isUnion())
                {
                    UaGenericStructureArray childValues = value.genericStructureArray(0);
                    printf("    %s (union)\n",
                            sFieldName.toUtf8());
                    for (OpcUa_UInt32 u=0; u<childValues.length(); u++)
                    {
                        for (lIdx=0;lIdx<level+1;lIdx++) printf("  ");
                        printf("    %s\n", field.structureDefinition().name().toUtf8());
                        printGenericStructureValue(childValues[u], level+2);
                    }
                }
                else
                {
                    //union
                    UaGenericUnionArray childValues = value.genericUnionArray(0);
                    printf("    %s (union)\n",
                            sFieldName.toUtf8());
                    for (OpcUa_UInt32 u=0; u<childValues.length(); u++)
                    {
                        for (lIdx=0;lIdx<level+1;lIdx++) printf("  ");
                        printf("    %s\n", field.structureDefinition().name().toUtf8());
                        printGenericUnionValue(childValues[u], level+2);
                    }
                }
            }
        }
        else
        {
            for (lIdx=0;lIdx<level;lIdx++) printf("  ");
            // Get field value
            UaVariant vFieldValue = value.value();
            printf("    %s (union) = %s\n", sFieldName.toUtf8(), vFieldValue.toString().toUtf8());
        }
    }
}

extern OpcUa_UInt16 g_nsIndex2;

/** Returns true if the structured DataType used for extensionObject is registered to the stack
    an known by the client. */
bool isStructuredDataTypeKnown(const UaExtensionObject &extensionObject)
{
    if (extensionObject.encoding() != UaExtensionObject::EncodeableObject)
    {
        return false;
    }
    UaNodeId dataTypeId = extensionObject.dataTypeId();
    return (dataTypeId == UaNodeId(DemoId_Vector, g_nsIndex2)
        || dataTypeId == UaNodeId(DemoId_UnionTest, g_nsIndex2));
}

void printExtensionObjectKnownType(const UaExtensionObject &extensionObject)
{
    if (extensionObject.dataTypeId() == UaNodeId(DemoId_Vector, g_nsIndex2))
    {
        //Try to convert Vector.
        Demo::Vector vector;
        OpcUa_StatusCode status = vector.setVector(extensionObject);
        if (OpcUa_IsGood(status))
        {
            printf("    X: %f\n", vector.getX());
            printf("    Y: %f\n", vector.getY());
            printf("    Z: %f\n", vector.getZ());
            return;
        }
    }

    if (extensionObject.dataTypeId() == UaNodeId(DemoId_UnionTest, g_nsIndex2))
    {
        //Try to convert Union.
        Demo::UnionTest unionTest;
        OpcUa_StatusCode status = unionTest.setUnionTest(extensionObject);
        if (OpcUa_IsGood(status))
        {
            switch (unionTest.type())
            {
                case Demo_UnionTest_Null:
                    break;
                case Demo_UnionTest_Int32:
                    printf("    Int32 (union) %d\n", unionTest.getInt32());
                    break;
                case Demo_UnionTest_String:
                    printf("    String (union) %s\n", unionTest.getString().toUtf8());
                    break;
            }
            return;
        }
    }
}

void printExtensionObjectGeneric(const UaExtensionObject &extensionObject)
{
    // Try to get the structure definition from the dictionary
    UaStructureDefinition definition = g_pUaSession->structureDefinition(extensionObject.encodingTypeId());
    if (!definition.isNull())
    {
        if (!definition.isUnion())
        {
            // Decode the ExtensionObject to a UaGenericValue to provide access to the structure fields
            UaGenericStructureValue genericValue;
            genericValue.setGenericValue(extensionObject, definition);
            printGenericStructureValue(genericValue, 0);
        }
        else
        {
            // union
            // Decode the ExtensionObject to a UaGenericUnionValue to provide access to the structure fields
            UaGenericUnionValue genericValue;
            genericValue.setGenericUnion(extensionObject, definition);
            printGenericUnionValue(genericValue, 0);
        }
    }
    else
    {
        printf("  Cannot get a structure definition - check access to type dictionary\n");
    }
}

void printExtensionObject(UaExtensionObject &extensionObject)
{
    if (isStructuredDataTypeKnown(extensionObject))
    {
        printExtensionObjectKnownType(extensionObject);
    }
    else
    {
        if (extensionObject.encoding() != UaExtensionObject::Binary)
        {
            // Try to convert back to Binary since this helper is only able to print binary encoded structures
            // This is not very efficient and not recommended for real world client applications
            // If the structure is already decoded, it is a known structure
            // Known structures should be handled different like Vector in this example
            extensionObject.changeEncoding(UaExtensionObject::Binary);
        }
        if (extensionObject.encoding() == UaExtensionObject::Binary)
        {
            printExtensionObjectGeneric(extensionObject);
        }
        else
        {
            printf("  The ExtensionObject does not contain a binary encoded value\n");
        }
    }
}

void printExtensionObjects(const UaVariant &value, const UaString &text)
{
    if (value.arrayType() == OpcUa_VariantArrayType_Scalar)
    {
        printf("  %s with structure value:\n", text.toUtf8());
        UaExtensionObject extensionObject;
        value.toExtensionObject(extensionObject);
        printExtensionObject(extensionObject);
    }
    else if (value.arrayType() == OpcUa_VariantArrayType_Array)
    {
        printf("  %s with array of structured values:\n", text.toUtf8());
        UaExtensionObjectArray extensionObjectArray;
        value.toExtensionObjectArray(extensionObjectArray);
        for (OpcUa_UInt32 u=0; u<extensionObjectArray.length(); u++)
        {
            printf("  [%u]\n", u);
            UaExtensionObject e = extensionObjectArray[u];
            printExtensionObject(e);
        }
    }
}

class Callback:
    public UaSessionCallback,
    public UaSubscriptionCallback
{
public:
    Callback(){};
    virtual ~Callback(){};

    /** Status of the server connection. */
    virtual void connectionStatusChanged(
        OpcUa_UInt32              clientConnectionId,
        UaClient::ServerStatus serverStatus)
    {
        UaString sStatus;
        switch ( serverStatus )
        {
            case UaClient::Disconnected: sStatus="Disconnected"; break;
            case UaClient::Connected: sStatus="Connected"; break;
            case UaClient::ConnectionWarningWatchdogTimeout: sStatus="ConnectionWarningWatchdogTimeout"; break;
            case UaClient::ConnectionErrorApiReconnect: sStatus="ConnectionErrorApiReconnect"; break;
            case UaClient::ServerShutdown: sStatus="ServerShutdown"; break;
            case UaClient::NewSessionCreated: sStatus="NewSessionCreated"; break;
        }
        printf("-- Event connectionStatusChanged ----------------------\n");
        printf("clientConnectionId %d \n", clientConnectionId);
        printf("serverStatus %s \n", sStatus.toUtf8());
        printf("-------------------------------------------------------\n");
    }

    /** Connect errors. */
    bool connectError(
        OpcUa_UInt32                    clientConnectionId, //!< [in] Client defined handle of the affected session
        UaClient::ConnectServiceType    serviceType, //!< [in] The failing connect step
        const UaStatus&                 error,       //!< [in] Status code for the error situation
        bool                            clientSideError) //!< [in] Flag indicating if the bad status was created in the Client SDK
    {
        UaString sServiceType;
        switch ( serviceType )
        {
            case UaClient::CertificateValidation: sServiceType="Certificate validation steps"; break;
            case UaClient::OpenSecureChannel: sServiceType="Processing of Service OpenSecureChannel"; break;
            case UaClient::CreateSession: sServiceType="Processing of Service CreateSession"; break;
            case UaClient::ActivateSession: sServiceType="Processing of Service CreateSession"; break;
        }
        printf("-- Event connectError ---------------------------------\n");
        printf("clientConnectionId %d \n", clientConnectionId);
        printf("Service Type %s \n", sServiceType.toUtf8());
        printf("Status %s \n", error.toString().toUtf8());
        printf("-------------------------------------------------------\n");

        if ( clientSideError )
        {
            printf("Press 1 to stop connect\n");
            printf("Press 2 to overwrite the error and continue\n");
            printf("-------------------------------------------------------\n");
            int action;
            while (!WaitForKeypress(action))
            {
                if ( action == 1 )
                {
                    return false;
                }
                if ( action == 2 )
                {
                    return true;
                }
            }
        }
        return false;
    }

    /** Send changed data. */
    virtual void dataChange(
        OpcUa_UInt32               clientSubscriptionHandle,
        const UaDataNotifications& dataNotifications,
        const UaDiagnosticInfos&   /*diagnosticInfos*/)
    {
        OpcUa_UInt32 i = 0;
        printf("-- Event dataChange -----------------------------------\n");
        printf("clientSubscriptionHandle %d \n", clientSubscriptionHandle);
        for ( i=0; i<dataNotifications.length(); i++ )
        {
            if ( OpcUa_IsGood(dataNotifications[i].Value.StatusCode) )
            {
                UaVariant tempValue = dataNotifications[i].Value.Value;
                if (tempValue.type() == OpcUaType_ExtensionObject)
                {
                    printExtensionObjects(tempValue, UaString("Variable %1").arg((unsigned int) dataNotifications[i].ClientHandle));
                }
                else
                {
                    printf("  Variable %d value = %s\n", dataNotifications[i].ClientHandle, tempValue.toString().toUtf8());
                }
            }
            else
            {
                printf("  Variable %d failed!\n", i);
            }
        }
        printf("-------------------------------------------------------\n");
    }

    /** Send subscription state change. */
    virtual void subscriptionStatusChanged(
        OpcUa_UInt32      clientSubscriptionHandle,
        const UaStatus&   status)
    {
        printf("-- Event subscriptionStatusChanged ----------------------\n");
        printf("clientSubscriptionHandle %d \n", clientSubscriptionHandle);
        printf("subscriptionStatus %s \n", status.toString().toUtf8());
        printf("-------------------------------------------------------\n");
    }

    /** Send new events. */
    virtual void newEvents(
        OpcUa_UInt32          clientSubscriptionHandle,
        UaEventFieldLists&    eventFieldList)
    {
        OpcUa_UInt32 i = 0;
        OpcUa_Int32 j = 0;
        printf("-- Event newEvents ------------------------------------\n");
        printf("clientSubscriptionHandle %d \n", clientSubscriptionHandle);
        for ( i=0; i<eventFieldList.length(); i++ )
        {
            // Print event message text first
            UaVariant message    = eventFieldList[i].EventFields[0];
            printf("Event[%d] Message = %s\n",
                i,
                message.toString().toUtf8());

            // Print other requested event fields in a list
            // Event field names must be set in m_eventFields before creating the event monitored item
            for ( j=1; j<eventFieldList[i].NoOfEventFields; j++ )
            {
                if ( (OpcUa_Int32)(m_eventFields.length() + 1) < j )
                {
                    printf("  More event fields delivered than requested\n");
                }
                UaVariant tempValue = eventFieldList[i].EventFields[j];
                if (tempValue.type() == OpcUaType_ExtensionObject)
                {
                    printExtensionObjects(tempValue, UaString(m_eventFields[j-1]));
                }
                else if (tempValue.type() == OpcUaType_Null)
                {
                    // Special handling for NULL values
                    printf("  %s = NULL\n", UaString(m_eventFields[j-1]).toUtf8());
                }
                else
                {
                    printf("  %s = %s\n", UaString(m_eventFields[j-1]).toUtf8(), tempValue.toString().toUtf8());
                }
            }
        }
        printf("-------------------------------------------------------\n");
    }

    /** Send read results. */
    virtual void readComplete(
        OpcUa_UInt32             transactionId,
        const UaStatus&          result,
        const UaDataValues&      values,
        const UaDiagnosticInfos& /*diagnosticInfos*/)
    {
        printf("-- Event readComplete ---------------------------------\n");
        printf("transactionId %d \n", transactionId);
        printf("resultStatus %s \n", result.toString().toUtf8());
        for ( OpcUa_UInt32 i=0; i<values.length(); i++ )
        {
            if ( OpcUa_IsGood(values[i].StatusCode) )
            {
                UaVariant tempValue = values[i].Value;
                if (tempValue.type() == OpcUaType_ExtensionObject)
                {
                    printExtensionObjects(tempValue, UaString("Variable %1").arg((unsigned int) i));
                }
                else
                {
                    printf("  Variable %d value = %s\n", i, tempValue.toString().toUtf8());
                }
            }
            else
            {
                printf("  Variable %d failed with error %s\n", i, UaStatus(values[i].StatusCode).toString().toUtf8());
            }
        }
        printf("-------------------------------------------------------\n");
    }

    /** Send write results. */
    virtual void writeComplete(
        OpcUa_UInt32             transactionId,
        const UaStatus&          result,
        const UaStatusCodeArray& results,
        const UaDiagnosticInfos& /*diagnosticInfos*/)
    {
        printf("-- Event writeComplete --------------------------------\n");
        printf("transactionId %d \n", transactionId);
        printf("resultStatus %s \n", result.toString().toUtf8());
        for ( OpcUa_UInt32 i=0; i<results.length(); i++ )
        {
            if ( OpcUa_IsGood(results[i]) )
            {
                printf("  Variable %d succeeded!\n", i);
            }
            else
            {
                printf("  Variable %d failed with error %s\n", i, UaStatus(results[i]).toString().toUtf8());
            }
        }
        printf("-------------------------------------------------------\n");
    }

    UaStringArray m_eventFields;
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
    public UaSubscriptionCallback
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
        printf("-- Event dataChange -----------------------------------\n");
        printf("clientSubscriptionHandle %d \n", clientSubscriptionHandle);
        printf("-------------------------------------------------------\n");
    }

    /** Send subscription state change. */
    virtual void subscriptionStatusChanged(
        OpcUa_UInt32      clientSubscriptionHandle,
        const UaStatus&   status)
    {
        printf("-- Event subscriptionStatusChanged ----------------------\n");
        printf("clientSubscriptionHandle %d \n", clientSubscriptionHandle);
        printf("serverStatus %s \n", status.toString().toUtf8());
        printf("-------------------------------------------------------\n");
    };

    /** Send new events. */
    virtual void newEvents(
        OpcUa_UInt32          clientSubscriptionHandle,
        UaEventFieldLists&    eventFieldList)
    {
        printf("\n-- Event newEvents ------------------------------------\n");
        printf("clientSubscriptionHandle %d \n", clientSubscriptionHandle);
        printf("Number of new events %d \n", eventFieldList.length());

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
                printf("Event[%d] SourceName=%s | %s | Acked=%s | Message=%s \n",
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
                    printf("Event[%d] Refresh Start Event \n", i);
                }
                else if ( newEvent.eventType == OpcUaId_RefreshEndEventType )
                {
                    printf("Event[%d] Refresh End Event \n", i);
                }
                else
                {
                    printf("Event[%d] Skipped - can not get event type of conditionId - SourceName = %s Message = %s \n",
                        i,
                        sourceName.toString().toUtf8(),
                        message.toString().toUtf8());
                }
                // skip this event
            }
        }
        printf("-------------------------------------------------------\n");

        // print alarms
        if (m_bTableChanged)
        {
            printf("\n-- Alarm list -----------------------------------------\n");
            printAlarmList();
            printf("\n-------------------------------------------------------\n");
            printf("|     Press x to stop subscription                    |\n");
            printf("|     Press a to acknowledge all                      |\n");
            printf("|     Press c to confirm all                          |\n");
            printf("|     Press number to acknowledge / confirm single    |\n");
            printf("-------------------------------------------------------\n\n");
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

        printf("# | Ack | Conf | Time | Severity | Source | Message | CondName\n");

        while (it != m_mapAlarms.end())
        {
            printf("[%d] | %d | %d | %s | %d | %s | %s | %s\n",
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

        printf("-------------------------------------------------------\n");
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


