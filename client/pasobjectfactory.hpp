#ifndef __PASOBJECTFACTORY_H__
#define __PASOBJECTFACTORY_H__

#include "common/alignment/device.hpp"
#include "common/opcua/pasobject.hpp"

class PasObjectFactory
{
    public:
    PasObjectFactory() = default;

    ~PasObjectFactory() = default;

    static PasObject *Create(
            unsigned deviceType,
            const UaString& name,
            const UaNodeId& newNodeId,
            const UaString& defaultLocaleId,
            PasClientNodeManager *pNodeManager,
            const Device::Identity &identity,
            PasClientCommunicationInterface *pCommIf);
};

#endif
