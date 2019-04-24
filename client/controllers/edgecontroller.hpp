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
    EdgeController(Identity identity);

    ~EdgeController() override;

    // Get Controller status and data
    UaStatus getState(PASState &state) override;

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    // set Controller status and data
    UaStatus setState(PASState state) override;

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args = UaVariantArray()) override;

    const Eigen::MatrixXd &getResponseMatrix(unsigned panelpos);

    const Eigen::VectorXd &getAlignedReadings();

    const Eigen::VectorXd &getSystematicOffsets();

    const Eigen::VectorXd &getCurrentReadings();

    bool isAligned() { return m_isAligned; }

private:

    // methods
    UaStatus align(unsigned panel_pos, double alignFrac = 0.25, bool moveit = true);

    UaStatus findMatrix(UaVariantArray args);

    // helpers for the above
    UaStatus findSingleMatrix(unsigned panelIdx, double stepSize = 0.5);

    UaStatus alignSinglePanel(unsigned panelpos, double alignFrac, bool moveit = true);

    // keep track of the panels we want to move
    std::vector<unsigned> m_PanelsToMove;
    // maps panel position to its response matrix
    std::map<unsigned, Eigen::MatrixXd> m_ResponseMatMap;
    Eigen::VectorXd m_AlignedReadings;
    Eigen::VectorXd m_systematicOffsets;
    Eigen::VectorXd m_CurrentReadings;
    Eigen::VectorXd m_CurrentReadingsSD;

    bool m_isAligned;
};

#endif //ALIGNMENT_EDGECONTROLLER_HPP
