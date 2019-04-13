#ifndef __PASOBJECTFACTORY_H__
#define __PASOBJECTFACTORY_H__

#include "common/alignment/device.hpp"
#include "pasobject.h"

class PasObjectFactory
{
    public:
        PasObjectFactory(){};
        ~PasObjectFactory(){};

        PasObject* Create(
                unsigned deviceType,
                const UaString& name,
                const UaNodeId& newNodeId,
                const UaString& defaultLocaleId,
                PasNodeManager *pNodeManager,
                Device::Identity identity,
                PasCommunicationInterface *pCommIf);
};

#endif
