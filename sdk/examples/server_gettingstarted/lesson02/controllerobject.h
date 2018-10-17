#ifndef __CONTROLLEROBJECT_H__
#define __CONTROLLEROBJECT_H__

#include "uaobjecttypes.h"

class NmBuildingAutomation;

class ControllerObject :
    public UaObjectBase
{
    UA_DISABLE_COPY(ControllerObject);
public:
    ControllerObject(const UaString& name, const UaNodeId& newNodeId, const UaString& defaultLocaleId, NmBuildingAutomation* pNodeManager);
    virtual ~ControllerObject(void);

    OpcUa_Byte eventNotifier() const;

protected:
    UaMutexRefCounted*        m_pSharedMutex;
};

#endif
