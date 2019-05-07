#ifndef CLIENT_POSITIONERCONTROLLER_HPP
#define CLIENT_POSITIONERCONTROLLER_HPP

#include "uabase.h"
#include "statuscode.h"
#include "client/controllers/pascontroller.hpp"
#include "pascommunicationinterface.hpp"
#include "uamutex.h"
#include "common/alignment/device.hpp"

class Client;


class PositionerController : public PasController {
    UA_DISABLE_COPY(PositionerController);
public:
    /* construction / destruction */
    PositionerController(Identity identity, Client *pClient);

    ~PositionerController() override;

    /* Get Controller status and data */
    UaStatus getState(Device::DeviceState &state) override;

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    /* set Controller status and data */
    UaStatus setState(Device::DeviceState state) override;

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args) override;

private:
    struct Position {
        OpcUa_Float curAz;
        OpcUa_Float curEl;
        OpcUa_Float inAz;
        OpcUa_Float inEl;
        OpcUa_Boolean isMoving;
    } m_Data;
};

#endif//CLIENT_POSITIONERCONTROLLER_HPP
