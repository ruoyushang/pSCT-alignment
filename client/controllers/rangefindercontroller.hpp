#ifndef _RANGEFINDERCONTROLLER_HPP
#define _RANGEFINDERCONTROLLER_HPP

#include "client/controllers/pascontroller.hpp"

class RangefinderController : public PasController {
    UA_DISABLE_COPY(RangefinderController);

public:

    // construction / destruction
    explicit RangefinderController(Device::Identity identity, Client *pClient);

    // Get Controller status and data
    UaStatus getState(Device::DeviceState &state) override;

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    // set Controller status and data
    UaStatus setState(Device::DeviceState state) override;

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args = UaVariantArray()) override;
    
};

#endif //_RANGEFINDERCONTROLLER_HPP
