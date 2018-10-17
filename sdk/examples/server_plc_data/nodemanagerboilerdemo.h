#ifndef __NODEMANAGERBOILDERDEMO_H__
#define __NODEMANAGERBOILDERDEMO_H__

#include "nodemanagerbase.h"
#include "uathread.h"

class NodeManagerBoilerDemo : 
    public NodeManagerBase,
    public UaThread
{
    UA_DISABLE_COPY(NodeManagerBoilerDemo);
public:
    NodeManagerBoilerDemo();
    virtual ~NodeManagerBoilerDemo();

    // NodeManagerUaNode implementation
    virtual UaStatus   afterStartUp();
    virtual UaStatus   beforeShutDown();

private:
    void run();

    bool m_bStopThread;

    UaDataVariableCache* m_pB1_FillLevel;
    UaDataVariableCache* m_pB1_FillLevelSP;
    UaDataVariableCache* m_pB2_FillLevel;
    UaDataVariableCache* m_pB2_FillLevelSP;
};

#endif // __NODEMANAGERBOILDERDEMO_H__
