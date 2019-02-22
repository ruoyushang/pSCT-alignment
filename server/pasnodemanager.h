#ifndef __PASNODEMANAGER_H__
#define __PASNODEMANAGER_H__

#include "pasnodemanagercommon.h"

class PasCommunicationInterface;

class PasNodeManager : public PasNodeManagerCommon
{
    UA_DISABLE_COPY(PasNodeManager);
public:
    PasNodeManager();
    virtual ~PasNodeManager();
    
    UaStatus afterStartUp();
    UaStatus beforeShutDown();

    void setCommunicationInterface(PasCommunicationInterface *pCommIf);

private:
    // add non-common types
    UaStatus amendTypeNodes();
};

#endif // __NMMPESSSERVER_H__
