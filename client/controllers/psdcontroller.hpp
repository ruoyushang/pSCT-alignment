#ifndef ALIGNMENT_PSDCONTROLLER_HPP
#define ALIGNMENT_PSDCONTROLLER_HPP

#include "client/controllers/pascontroller.hpp"
#include "client/controllers/mirrorcontroller.hpp"

class PSDController : public PasController {
    UA_DISABLE_COPY(PSDController);

public:
    friend MirrorController;

    // construction / destruction
    PSDController(Identity identity, std::shared_ptr<Client> pClient);

    virtual ~PSDController();

    // Get Controller status and data
    UaStatus getState(PASState &state);

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value);

    // set Controller status and data
    UaStatus setState(PASState state);

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value);

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args);

    // update interval in ms
    static int kUpdateInterval;

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
    } m_Data;

    UaStatus read();
};

#endif //ALIGNMENT_PSDCONTROLLER_HPP
