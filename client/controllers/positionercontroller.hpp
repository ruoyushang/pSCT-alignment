#ifndef CLIENT_POSITIONERCONTROLLER_HPP
#define CLIENT_POSITIONERCONTROLLER_HPP

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"

#include "common/alignment/device.hpp"

#include "client/controllers/pascontroller.hpp"
#include "client/pascommunicationinterface.hpp"


class Client;

class PositionerController : public PasController {
    UA_DISABLE_COPY(PositionerController);
public:
    /* construction / destruction */
    PositionerController(Device::Identity identity, Client *pClient) : PasController(
        std::move(identity),
        pClient), m_Data() {
    }
    // TODO does this need initialize()?

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
        OpcUa_Int16 energyLevel;
        OpcUa_Boolean isMoving;

        Position() : curAz(0.0), curEl(0.0), inAz(0.0), inEl(0.0), energyLevel(0), isMoving(OpcUa_False) {}
    } m_Data;
};

class DummyPositionerController : public PositionerController {
    UA_DISABLE_COPY(DummyPositionerController);

public:
    /* construction / destruction */
    explicit DummyPositionerController(Device::Identity identity) : PositionerController(
        std::move(identity), nullptr), m_Data() {
    }

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
        OpcUa_Int16 energyLevel;
        OpcUa_Boolean isMoving;

        Position() : curAz(0.0), curEl(0.0), inAz(0.0), inEl(0.0), energyLevel(0), isMoving(OpcUa_False) {}
    } m_Data;
};

#endif//CLIENT_POSITIONERCONTROLLER_HPP
