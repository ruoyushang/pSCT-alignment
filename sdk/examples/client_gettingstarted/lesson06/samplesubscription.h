#ifndef SAMPLESUBSCRIPTION_H
#define SAMPLESUBSCRIPTION_H

#include "uabase.h"
#include "uaclientsdk.h"

using namespace UaClientSdk;

class Configuration;

class SampleSubscription :
    public UaSubscriptionCallback
{
    UA_DISABLE_COPY(SampleSubscription);
public:
    SampleSubscription(Configuration* pConfiguration);
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

    // set the configuration where we get the list of NodeIds to monitored from.
    void setConfiguration(Configuration* pConfiguration);

private:
    UaStatus recoverSubscription();

private:
    UaSession*          m_pSession;
    UaSubscription*     m_pSubscription;
    Configuration*      m_pConfiguration;
};

#endif // SAMPLESUBSCRIPTION_H
