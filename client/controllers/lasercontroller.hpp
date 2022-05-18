#ifndef _LASERCONTROLLER_HPP
#define _LASERCONTROLLER_HPP

#include "client/controllers/pascontroller.hpp"

class LaserController : public PasController {
    UA_DISABLE_COPY(LaserController);

public:

    // construction / destruction
    explicit LaserController(Device::Identity identity, Client *pClient);

    // Get Controller status and data
    UaStatus getState(Device::DeviceState &state) override;

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    // set Controller status and data
    UaStatus setState(Device::DeviceState state) override;

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args = UaVariantArray()) override;
    
};

#endif //_LASERCONTROLLER_HPP
