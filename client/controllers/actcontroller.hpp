#ifndef ALIGNMENT_ACTCONTROLLER_HPP
#define ALIGNMENT_ACTCONTROLLER_HPP

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include "common/opcua/pascominterfacecommon.h"
#include "client/controllers/pascontroller.h"
#include "client/controllers/panelcontroller.hpp"

class Client;


class ActController : public PasController {
    UA_DISABLE_COPY(ActController);

public:
    friend PanelController;

    // construction / destruction
    ActController(Identity identity, Client *pClient);

    virtual ~ActController();

    // Get Controller status and data
    UaStatus getState(PASState &state);

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value);

    UaStatusCode getError(OpcUa_UInt32 offset, UaVariant &value);

    // set Controller status and data
    UaStatus setState(PASState state);

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value);

    UaStatusCode operate(OpcUa_UInt32 offset = 0, const UaVariantArray &args = UaVariantArray());

private:
    UaStatus moveDelta(const UaVariantArray &args);
};

#endif //ALIGNMENT_ACTCONTROLLER_HPP
