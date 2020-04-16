#ifndef ALIGNMENT_OPTICALALIGNMENTCONTROLLER_H
#define ALIGNMENT_OPTICALALIGNMENTCONTROLLER_H

#include <Eigen/Dense> // Eigen3 for linear algebra needs
#include <math.h>

#include "client/controllers/mpescontroller.hpp"
#include "client/controllers/actcontroller.hpp"
#include "client/controllers/panelcontroller.hpp"
#include "client/controllers/edgecontroller.hpp"
#include "client/controllers/mirrorcontroller.hpp"

#include "common/alignment/device.hpp"
#include "common/simulatestewart/stewartplatform.hpp"
#include "client/controllers/pascontroller.hpp"
#include "common/alignment/focalplane.hpp"

class MirrorController;
class PanelController;

class OpticalAlignmentController : public PasCompositeController {
    UA_DISABLE_COPY(OpticalAlignmentController);
public:
    explicit OpticalAlignmentController(Device::Identity identity, Client *pClient);

    // initialize and precompute everything
    bool initialize() override;

    // same as before -- get/set status/data and Operate:
    UaStatus getState(Device::DeviceState &state) override;

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    UaStatus setState(Device::DeviceState state) override;

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args) override;

    // own implementation
    void addChild(OpcUa_UInt32 deviceType, const std::shared_ptr<PasController> &pController) override;

protected:
    //

private:
    std::string m_Mode;

    std::set<unsigned> m_selectedPanels;

    focalplane m_focalPlaneImage = focalplane(Device::Identity());

    std::map <int, Eigen::VectorXd> m_corrected_actuator_lengths_per_panel;

    std::string _captureSingleImage();

    std::map<int, std::vector<double>> _analyzeImagePatternAutomatically(std::string image_filepath, bool plot);
    std::vector<double> _analyzeImageSinglePanelAutomatically(std::string image_filepath, bool plot);

    Eigen::VectorXd _calculateActuatorMotion(int panel, double x, double y);

    bool _saveCorrections(std::map<int, Eigen::VectorXd> map);

    void _loadPatternImageParameters();
};


#endif //ALIGNMENT_OPTICALALIGNMENTCONTROLLER_H
