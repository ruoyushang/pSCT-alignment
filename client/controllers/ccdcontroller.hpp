#ifndef ALIGNMENT_CCDCONTROLLER_HPP
#define ALIGNMENT_CCDCONTROLLER_HPP

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include "client/controllers/pascontroller.h"
#include "client/controllers/mirrorcontroller.hpp"

class GASCCD;


class CCDController : public PasController {
    UA_DISABLE_COPY(CCDController);

public:
    friend MirrorController;

    /* construction / destruction */
    CCDController(Identity identity);

    ~CCDController();

    /* Get Controller status and data */
    UaStatus getState(PASState &state);

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value);

    /* set Controller status and data */
    UaStatus setState(PASState state);

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value);

    UaStatusCode Operate(OpcUa_UInt32 offset = 0, const UaVariantArray &args = UaVariantArray());

private:
    PASState m_state = PASState::Off;
    bool m_updated = false;

    UaStatus read();

    GASCCD *m_pCCD;
};

#endif //ALIGNMENT_CCDCONTROLLER_HPP
