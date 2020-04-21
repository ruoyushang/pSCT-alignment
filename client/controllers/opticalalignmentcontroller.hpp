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

#include "uathread.h"

class MirrorController;
class PanelController;

class OpticalAlignmentController : public PasCompositeController, UaThread {
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

    void run() override ;

protected:
    //

private:
    std::string m_Mode;

    double m_target_coordinates_center_x;
    double m_target_coordinates_center_y;
    bool m_show_plot ;
    double m_offset_limit;
    std::string m_respFile;
    bool m_processing = true;

    void setTargetX(double x) {m_target_coordinates_center_x = x;};
    void setTargetY(double y) {m_target_coordinates_center_y = y;};
    void showPlot(bool show) {m_show_plot = show ;};
    void setOffsetLimit(double offset) {m_offset_limit = offset;};

    std::set<unsigned> m_selectedPanels;

    focalplane m_focalPlaneImage = focalplane(Device::Identity());

    std::map <int, Eigen::VectorXd> m_corrected_coordinate_deltas_per_panel;

    std::string _captureSingleImage();

    std::map<int, std::vector<double>> _analyzeImagePatternAutomatically(const std::string& image_filepath, bool plot);
    std::vector<double> _analyzeImageSinglePanelAutomatically(const std::string& image_filepath, bool plot);

    Eigen::VectorXd _calculatePanelMotion(int panel, double x, double y, std::string respFile);

    bool _saveCorrections(std::map<int, Eigen::VectorXd> map);

    void _loadPatternImageParameters();

    void calibrateFirstOrderCorrection();

    bool _doSafePanelMotion(std::shared_ptr<PanelController> sharedPtr, Eigen::VectorXd matrix);
};


#endif //ALIGNMENT_OPTICALALIGNMENTCONTROLLER_H
