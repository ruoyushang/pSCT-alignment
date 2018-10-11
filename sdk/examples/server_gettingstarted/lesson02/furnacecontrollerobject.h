#ifndef __FURNACECONTROLLEROBJECT_H__
#define __FURNACECONTROLLEROBJECT_H__

#include "controllerobject.h"

class FurnaceControllerObject :
    public ControllerObject
{
    UA_DISABLE_COPY(FurnaceControllerObject);
public:
    FurnaceControllerObject(const UaString& name, const UaNodeId& newNodeId, const UaString& defaultLocaleId, NmBuildingAutomation* pNodeManager);
    virtual ~FurnaceControllerObject(void);

    virtual UaNodeId typeDefinitionId() const;
};

#endif
