#ifndef __PANELCONTROLLER_H__
#define __PANELCONTROLLER_H__

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include "common/opcua/pascominterfacecommon.h"

#include "server/controllers/actcontroller.hpp"
#include "server/controllers/pascontroller.hpp"

#include <memory>
#include <vector>

class PanelController : public PasController
{
    UA_DISABLE_COPY(PanelController);
public:
    // construction / destruction
    PanelController(int ID, std::shared_ptr<Platform> pPlatform);
    ~PanelController();

    // Get Controller status and data
    UaStatus getState(PASState& state);
    UaStatus getData(OpcUa_UInt32 offset, UaVariant& value);

    // set Controller status and data
    UaStatus updateState();
    UaStatus setState(PASState state);
    UaStatus setData(OpcUa_UInt32 offset, UaVariant value);
    UaStatus Operate(OpcUa_UInt32 offset, const UaVariantArray& args);

    void addActuator(std::shared_ptr<ActController> pActuator) { m_pActuators.push_back(pActuator); };

private:
    PASState m_state = PASState::Off;

    std::vector<std::shared_ptr<ActController>> m_pActuators; // vector of Actuators on this panel
};

#endif
