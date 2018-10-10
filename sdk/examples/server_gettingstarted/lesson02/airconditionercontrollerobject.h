#ifndef __AIRCONDITIONERCONTROLLEROBJECT_H__
#define __AIRCONDITIONERCONTROLLEROBJECT_H__

#include "controllerobject.h"

class AirConditionerControllerObject :
    public ControllerObject
{
    UA_DISABLE_COPY(AirConditionerControllerObject);
public:
    AirConditionerControllerObject(const UaString& name, const UaNodeId& newNodeId, const UaString& defaultLocaleId, NmBuildingAutomation* pNodeManager);
    virtual ~AirConditionerControllerObject(void);

    virtual UaNodeId typeDefinitionId() const;
};

#endif
