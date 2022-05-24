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
#include "client/controllers/opttablecontroller.hpp"
#include "client/controllers/opticalalignmentcontroller.hpp"
#include "client/controllers/globalalignmentcontroller.hpp"

#include "common/alignment/mpes.hpp"


class MPESController : public PasController {
    UA_DISABLE_COPY(MPESController);

public:
    friend class EdgeController;
    friend class MirrorController;
    friend class OptTableController;
    friend class OpticalAlignmentController;
    friend class GlobalAlignmentController;

    /* construction / destruction */
    MPESController(Device::Identity identity, Client *pClient, std::string mode = "client");

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

    bool isVisible();

    const Eigen::Matrix<double, 2, 6> &getResponseMatrix(char panelside = 'w') const {
        return m_ResponseMatMap.at(panelside);
    }

    Eigen::Vector2d getAlignedReadings();
    const Eigen::Vector2d &getSystematicOffsets() const { return m_SystematicOffsets; }
    const Eigen::Vector2d &getOpticsOffsets() const { return m_OpticsOffsets; }

    MPESBase::Position getPosition();

    Device::ErrorState getErrorState() override;

private:
    std::string m_Mode;

    UaStatus read();

    UaStatus readAsync();

    // actuator response matrix map -- {panel position -> matrix}
    std::map<char, Eigen::Matrix<double, 2, 6> > m_ResponseMatMap;
    // systematic offsets
    Eigen::Vector2d m_SystematicOffsets;
    Eigen::Vector2d m_OpticsOffsets;
    // which side the panel is on { panel position -> side ('w' or 'l')
    std::map<unsigned, char> m_PanelSideMap;
    std::map<char, double> m_PlateScaleMap;
};

#endif //CLIENT_MPESCONTROLLER_HPP
