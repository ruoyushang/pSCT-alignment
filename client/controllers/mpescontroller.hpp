#ifndef CLIENT_MPESCONTROLLER_HPP
#define CLIENT_MPESCONTROLLER_HPP

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include "client/controllers/pascontroller.hpp"
#include "client/controllers/edgecontroller.hpp"
#include "client/controllers/mirrorcontroller.hpp"


class MPESController : public PasController {
    UA_DISABLE_COPY(MPESController);

public:
    friend EdgeController;
    friend MirrorController;

    /* construction / destruction */
    MPESController(Identity identity, Client *pClient);

    ~MPESController();

    /* Get Controller status and data */
    UaStatus getState(PASState &state);

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value);

    /* set Controller status and data */
    UaStatus setState(PASState state);

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value);

    UaStatus operate(OpcUa_UInt32 offset = 0, const UaVariantArray &args = UaVariantArray());

    // test if current panel is this sensor's webcam-side panel
    char getPanelSide(unsigned panelpos);

    bool isVisible() const { return m_isVisible; };

    const Eigen::Matrix<double, 2, 6> &getResponseMatrix(char panelside = 'w') const {
        return m_ResponseMatMap.at(panelside);
    }

    const Eigen::Vector2d &getAlignedReadings() const { return m_AlignedReadings; }

    const Eigen::Vector2d &getSystematicOffsets() const { return SystematicOffsets; }

private:
    bool m_updated;
    bool m_isVisible;
    struct MPESData {
        OpcUa_Double m_xCentroidAvg;
        OpcUa_Double m_yCentroidAvg;
        OpcUa_Double m_xCentroidSD;
        OpcUa_Double m_yCentroidSD;
        OpcUa_Double m_CleanedIntensity;
        OpcUa_Double m_xNominal;
        OpcUa_Double m_yNominal;
    } data;
    static float kNominalIntensity;
    static float kNominalCentroidSD;

    Identity m_wPanelId; // id of the webcam-side panel
    // actuator response matrix map -- {panel position -> matrix}
    std::map<char, Eigen::Matrix<double, 2, 6> > m_ResponseMatMap;
    // aligned readings
    Eigen::Vector2d m_AlignedReadings;
    Eigen::Vector2d SystematicOffsets;
    // which side the panel is on { panel position -> side ('w' or 'l')
    std::map<unsigned, char> m_PanelSideMap;

    // a read that performs such checks and exposure correction
    UaStatus read();

    // a helper for the above
    UaStatus __readRequest();
};

#endif //CLIENT_MPESCONTROLLER_HPP
