#ifndef __NMBUILDINGAUTOMATION_H__
#define __NMBUILDINGAUTOMATION_H__

#include "nodemanagerbase.h"

class NmBuildingAutomation : public NodeManagerBase
{
    UA_DISABLE_COPY(NmBuildingAutomation);
public:
    NmBuildingAutomation();
    virtual ~NmBuildingAutomation();

    virtual UaStatus   afterStartUp();
    virtual UaStatus   beforeShutDown();

    UaVariable* getInstanceDeclarationVariable(OpcUa_UInt32 numericIdentifier);

private:
    UaStatus createTypeNodes();
};

#endif // __NMBUILDINGAUTOMATION_H__
