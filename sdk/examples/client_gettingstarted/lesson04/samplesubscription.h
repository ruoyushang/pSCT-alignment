#ifndef SAMPLESUBSCRIPTION_H
#define SAMPLESUBSCRIPTION_H

#include "uabase.h"
#include "uaclientsdk.h"

using namespace UaClientSdk;

class SampleSubscription :
    public UaSubscriptionCallback
{
    UA_DISABLE_COPY(SampleSubscription);
public:
    SampleSubscription();
    virtual ~SampleSubscription();

    // UaSubscriptionCallback implementation ----------------------------------------------------
    virtual void subscriptionStatusChanged(
        OpcUa_UInt32      clientSubscriptionHandle,
        const UaStatus&   status);
    virtual void dataChange(
        OpcUa_UInt32               clientSubscriptionHandle,
        const UaDataNotifications& dataNotifications,
        const UaDiagnosticInfos&   diagnosticInfos);
    virtual void newEvents(
        OpcUa_UInt32                clientSubscriptionHandle,
        UaEventFieldLists&          eventFieldList);
    // UaSubscriptionCallback implementation ------------------------------------------------------

    // Create / delete a subscription on the server
    UaStatus createSubscription(UaSession* pSession);
    UaStatus deleteSubscription();

    // Create monitored items in the subscription
    UaStatus createMonitoredItems();

private:
    UaSession*                  m_pSession;
    UaSubscription*             m_pSubscription;
};

#endif // SAMPLESUBSCRIPTION_H
