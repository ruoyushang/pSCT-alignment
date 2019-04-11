/******************************************************************************
** Project: pSCT Alignment system
**
** Description: implementation of Positioner.
******************************************************************************/
#ifndef __CONTROLPOSITIONER_H__
#define __CONTROLPOSITIONER_H__

#include "uabase.h"
#include "statuscode.h"
#include "pascontroller.h"
#include "pascommunicationinterface.h"
#include "uamutex.h"
#include "components.h"

class Client;

/*===========================================================================*/
/* Positioner class, part of the PasController family of devices.            */
/*===========================================================================*/
class ControlPositioner : public PasController
{
    UA_DISABLE_COPY(ControlPositioner);
public:
    /* construction / destruction */
    ControlPositioner(Identity identity, Client *pClient);
    ~ControlPositioner();

    /* Get Controller status and data */
    UaStatusCode getState(PASState& state);
    UaStatusCode getData(OpcUa_UInt32 offset, UaVariant& value);

    /* set Controller status and data */
    UaStatusCode setState(PASState state);
    UaStatusCode setData(OpcUa_UInt32 offset, UaVariant value);

    UaStatusCode operate(OpcUa_UInt32 offset = 0, const UaVariantArray &args = UaVariantArray());

private:
    struct Position {
        OpcUa_Float curAz;
        OpcUa_Float curEl;
        OpcUa_Float inAz;
        OpcUa_Float inEl;
        OpcUa_Boolean isMoving;
    } data;
    
    UaStatus move();
};

#endif
