#ifndef ALIGNMENT_CCDCONTROLLER_HPP
#define ALIGNMENT_CCDCONTROLLER_HPP

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include "client/controllers/pascontroller.hpp"
#include "client/controllers/mirrorcontroller.hpp"

#include "common/globalalignment/ccdclass.h"

class CCDController : public PasController {
    UA_DISABLE_COPY(CCDController);

public:
    friend MirrorController;

    /* construction / destruction */
    explicit CCDController(Device::Identity identity, PasClientNodeManager * pNodeManager);

    /* Get Controller status and data */
    UaStatus getState(Device::DeviceState &state) override;

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    /* set Controller status and data */
    UaStatus setState(Device::DeviceState state) override;

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args) override;

private:
    bool m_updated = false;

    UaStatus read(bool print = true);

    std::unique_ptr<GASCCD> m_pCCD;
};

#endif //ALIGNMENT_CCDCONTROLLER_HPP
