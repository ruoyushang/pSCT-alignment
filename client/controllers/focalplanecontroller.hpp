#ifndef ALIGNMENT_FOCALPLANECONTROLLER_HPP
#define ALIGNMENT_FOCALPLANECONTROLLER_HPP

#include <Eigen/Dense>

#include "client/controllers/pascontroller.hpp"
#include "common/alignment/focalplane.hpp"

class FocalPlaneController : public PasCompositeController {
    UA_DISABLE_COPY(FocalPlaneController);

public:

    // construction / destruction
    explicit FocalPlaneController(Device::Identity identity, Client *pClient);

    // Get Controller status and data
    UaStatus getState(Device::DeviceState &state) override;

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    // set Controller status and data
    UaStatus setState(Device::DeviceState state) override;

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args = UaVariantArray()) override;

private:
    std::unique_ptr<focalplane> m_pFP ;
};

#endif //ALIGNMENT_FOCALPLANECONTROLLER_HPP
