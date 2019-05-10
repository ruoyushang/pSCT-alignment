#ifndef CLIENT_ACTCONTROLLER_HPP
#define CLIENT_ACTCONTROLLER_HPP

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uavariant.h"

#include "client/controllers/pascontroller.hpp"
#include "client/controllers/panelcontroller.hpp"

class Client;

enum class PASState;

class ActController : public PasController {
    UA_DISABLE_COPY(ActController);

public:
    friend PanelController;

    // construction / destruction
    ActController(Identity identity, std::shared_ptr<Client> pClient);

    ~ActController() override;

    // Get Controller status and data
    UaStatus getState(PASState &state) override;

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    UaStatus getError(OpcUa_UInt32 offset, UaVariant &value);

    // set Controller status and data
    UaStatus setState(PASState state) override;

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args) override;
};

#endif //CLIENT_ACTCONTROLLER_HPP
