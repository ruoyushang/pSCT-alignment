#ifndef __PASOBJECTFACTORY_H__
#define __PASOBJECTFACTORY_H__

#include "components.hpp"
#include "pasobject.hpp"

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
                Identity identity,
                PasCommunicationInterface *pCommIf);
};

#endif
