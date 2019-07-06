#ifndef ALIGNMENT_EDGECONTROLLER_HPP
#define ALIGNMENT_EDGECONTROLLER_HPP

#include <Eigen/Dense>

#include "client/controllers/mirrorcontroller.hpp"
#include "client/controllers/pascontroller.hpp"

class EdgeController : public PasCompositeController {
    UA_DISABLE_COPY(EdgeController);

public:
    friend MirrorController;

    // construction / destruction
    explicit EdgeController(Device::Identity identity);

    // Get Controller status and data
    UaStatus getState(Device::DeviceState &state) override;

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    // set Controller status and data
    UaStatus setState(Device::DeviceState state) override;

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args = UaVariantArray()) override;

    Eigen::MatrixXd getResponseMatrix(unsigned panelpos);

    std::pair<Eigen::VectorXd, Eigen::VectorXd> getCurrentReadings(bool read = true);

    Eigen::VectorXd getAlignedReadings();

    Eigen::VectorXd getSystematicOffsets();

    bool isAligned() { return m_isAligned; }
private:
    UaStatus align(unsigned panel_pos, double alignFrac = 0.25, bool moveit = true, bool execute = false);

    UaStatus alignSinglePanel(unsigned panelpos, double alignFrac, bool moveit = true, bool execute = false);

    UaStatus findMatrix(UaVariantArray args);
    UaStatus findSingleMatrix(unsigned panelIdx, double stepSize = 0.5);

    std::pair<Eigen::VectorXd, Eigen::VectorXd> __getCurrentReadings(bool read = true);

    // temporarily hold calculated alignment motion
    Eigen::VectorXd m_Xcalculated;

    // indicates whether edge is aligned
    bool m_isAligned;
};

#endif //ALIGNMENT_EDGECONTROLLER_HPP
