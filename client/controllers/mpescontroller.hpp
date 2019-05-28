#ifndef CLIENT_MPESCONTROLLER_HPP
#define CLIENT_MPESCONTROLLER_HPP

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include "client/controllers/edgecontroller.hpp"
#include "client/controllers/mirrorcontroller.hpp"
#include "client/controllers/pascontroller.hpp"

#include "common/alignment/mpes.hpp"


class MPESController : public PasController {
    UA_DISABLE_COPY(MPESController);

public:
    friend EdgeController;
    friend MirrorController;

    /* construction / destruction */
    MPESController(Device::Identity identity, std::shared_ptr<Client> pClient);

    /* Get Controller status and data */
    UaStatus getState(Device::DeviceState &state) override;

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    UaStatus getError(OpcUa_UInt32 offset, UaVariant &value);

    /* set Controller status and data */
    UaStatus setState(Device::DeviceState state) override;

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args = UaVariantArray()) override;

    // test if current panel is this sensor's webcam-side panel
    char getPanelSide(unsigned panelpos);

    bool isVisible() const { return m_isVisible; };

    const Eigen::Matrix<double, 2, 6> &getResponseMatrix(char panelside = 'w') const {
        return m_ResponseMatMap.at(panelside);
    }

    const Eigen::Vector2d &getAlignedReadings() const { return m_AlignedReadings; }

    const Eigen::Vector2d &getSystematicOffsets() const { return m_SystematicOffsets; }

    MPES::Position getPosition() { return m_data; }

private:
    bool m_isVisible;
    static float kNominalIntensity;
    static float kNominalSpotWidth;
    static int kMaxAttempts;

    int m_numAttempts;

    MPES::Position m_data;

    // actuator response matrix map -- {panel position -> matrix}
    std::map<char, Eigen::Matrix<double, 2, 6> > m_ResponseMatMap;
    // aligned readings
    Eigen::Vector2d m_AlignedReadings;
    Eigen::Vector2d m_SystematicOffsets;
    // which side the panel is on { panel position -> side ('w' or 'l')
    std::map<unsigned, char> m_PanelSideMap;

    // a read that performs such checks and exposure correction
    UaStatus read(bool print=true);
};

#endif //CLIENT_MPESCONTROLLER_HPP
