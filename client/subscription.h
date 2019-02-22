#ifndef __SUBSCRIPTION_H__
#define __SUBSCRIPTION_H__

#include "uabase.h"
#include "uaclient/uaclientsdk.h"

class Configuration;

class Subscription :
    public UaClientSdk::UaSubscriptionCallback
{
    UA_DISABLE_COPY(Subscription);
public:
    Subscription(Configuration* pConfiguration);
    virtual ~Subscription();

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
    UaStatus createSubscription(UaClientSdk::UaSession* pSession);
    UaStatus deleteSubscription();

    // Create monitored items in the subscription
    UaStatus createMonitoredItems();

    // set the configuration where we get the list of NodeIds to monitored from.
    void setConfiguration(Configuration* pConfiguration);

private:
    UaStatus recoverSubscription();

private:
    UaClientSdk::UaSession*          m_pSession;
    UaClientSdk::UaSubscription*     m_pSubscription;
    Configuration*      m_pConfiguration;
};

#endif // SUBSCRIPTION_H
