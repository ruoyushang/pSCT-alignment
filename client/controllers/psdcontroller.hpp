#ifndef ALIGNMENT_PSDCONTROLLER_HPP
#define ALIGNMENT_PSDCONTROLLER_HPP

#include "client/controllers/mirrorcontroller.hpp"
#include "client/controllers/pascontroller.hpp"


class PSDController : public PasController {
    UA_DISABLE_COPY(PSDController);

public:
    friend MirrorController;

    // construction / destruction
    PSDController(Device::Identity identity, Client *pClient);

    // Get Controller status and data
    UaStatus getState(Device::DeviceState &state) override;

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    // set Controller status and data
    UaStatus setState(Device::DeviceState state) override;

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args) override;

private:
    struct PSDData {
        OpcUa_Double x1;
        OpcUa_Double y1;
        OpcUa_Double x2;
        OpcUa_Double y2;
        OpcUa_Double dx1;
        OpcUa_Double dy1;
        OpcUa_Double dx2;
        OpcUa_Double dy2;
        OpcUa_Double Temperature;

        PSDData() : x1(0.0), y1(0.0), x2(0.0), y2(0.0), dx1(0.0), dy1(0.0), dx2(0.0), dy2(0.0), Temperature(0.0) {}
    } m_data;

    UaStatus read();
};

#endif //ALIGNMENT_PSDCONTROLLER_HPP
