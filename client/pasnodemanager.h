#ifndef __PASNODEMANAGER_H__
#define __PASNODEMANAGER_H__

#include "pasnodemanagercommon.h"
#include <vector>

class PasCommunicationInterface;
class Configuration;
class Client;

class PasNodeManager : public PasNodeManagerCommon
{
    UA_DISABLE_COPY(PasNodeManager);
public:
    PasNodeManager();
    virtual ~PasNodeManager();

    UaStatus afterStartUp();
    UaStatus beforeShutDown();

    void setConfiguration(Configuration *pConfiguration);
    void setCommunicationInterface(PasCommunicationInterface *pCommIf);

    UaString getDefaultLocaleId() {return m_defaultLocaleId;};

    // emergency method -- something goes wrong and we want to shut down everything
    OpcUa_Int32 Panic();

private:
    // add non-common types
    UaStatus amendTypeNodes();
    Configuration *m_pConfiguration;
    std::vector<Client *> m_pClient;
    Client *m_pPositioner;

};

#endif // __PASNODEMANAGER_H__
