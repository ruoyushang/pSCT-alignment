#include "opticalalignmentcontroller.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <common/alignment/mpes.hpp>
#include <common/alignment/actuator.hpp>
#include <common/opcua/pasobject.hpp>

#include "common/alignment/device.hpp"
#include "common/simulatestewart/mathtools.hpp"
#include "common/simulatestewart/mirrordefinitions.hpp" // definitions of the mirror surfaces
#include "common/alignment/focalplane.hpp"

#include "client/controllers/edgecontroller.hpp"
#include "client/controllers/mpescontroller.hpp"
#include "client/controllers/panelcontroller.hpp"
#include "client/controllers/actcontroller.hpp"
#include "client/controllers/focalplanecontroller.hpp"
#include "client/controllers/pascontroller.hpp"

#include "client/objects/panelobject.hpp"

#include "uathread.h"

// MySQL C++ Connector includes
#include "mysql_driver.h"
#include "cppconn/statement.h"
#include "DBConfig.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

OpticalAlignmentController::OpticalAlignmentController(Device::Identity identity, Client *pClient)
        : PasCompositeController(
        std::move(identity), nullptr,
        10000){
    // define possible children and initialize the selected children string
    m_ChildrenTypes = {PAS_FocalPlaneType, PAS_PanelType};

    // make sure things update on the first boot up
    // duration takes seconds -- hence the conversion with the 1/1000 ratio
    m_LastUpdateTime = TIME::now() - std::chrono::duration<int, std::ratio<1, 1000>>(m_kUpdateInterval_ms);
}

void OpticalAlignmentController::addChild(OpcUa_UInt32 deviceType, const std::shared_ptr<PasController>& pController)
{
    // call the base type's method
    PasCompositeController::addChild(deviceType, pController);

    if (deviceType == PAS_PanelType) {
        m_selectedPanels.insert((unsigned) pController->getIdentity().position);
    }
    else if (deviceType == PAS_FocalPlaneType) {
        m_focalPlaneImage = focalplane(pController->getIdentity());
    }
}


bool OpticalAlignmentController::initialize()
{
    spdlog::info("{} : OpticalAlignmentController::initialize(): Initializing...", m_Identity);

    spdlog::info("{}: OpticalAlignmentController::initialize(): Done.", m_Identity);

    return true;
}

UaStatus OpticalAlignmentController::getState(Device::DeviceState &state)
{
    //UaMutexLocker lock(&m_mutex);
    Device::DeviceState s;
    std::vector<unsigned> deviceTypesToCheck = {PAS_PanelType};
    for (auto devType : deviceTypesToCheck) {
        for (const auto &child : getChildren(devType)) {
            child->getState(s);
            if (s == Device::DeviceState::Busy) {
                m_State = s;
            }
        }
    }
    state = m_State;
    spdlog::trace("{} : Read device state => ({})", m_Identity, (int)state);
    return OpcUa_Good;
}

UaStatus OpticalAlignmentController::setState(Device::DeviceState state)
{
    //UaMutexLocker lock(&m_mutex);
    m_State = state;
    spdlog::trace("{} : Setting device state => ({})", m_Identity, (int)state);
    return OpcUa_Good;
}

UaStatus OpticalAlignmentController::getData(OpcUa_UInt32 offset, UaVariant &value)
{
    //UaMutexLocker lock(&m_mutex);

    UaStatus status;
    switch (offset) {
        case PAS_OpticalAlignmentType_ImageParamSet: {
            bool verbosity;
            verbosity = true;
            value.setBool(verbosity);
            spdlog::trace("{} : Read Verbosity value => ({})", m_Identity, verbosity);
            break;
        }
        default:
            return OpcUa_BadInvalidArgument;
    }
    return OpcUa_Good;
}

UaStatus OpticalAlignmentController::setData(OpcUa_UInt32 offset, UaVariant value)
{
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    switch (offset) {
        case PAS_OpticalAlignmentType_ImageParamSet: {
            OpcUa_Boolean val;
            value.toBool(val);
            spdlog::trace("{} : Setting Verbosity value... value => ({})", m_Identity, val);
            break;
        }
        default:
            return OpcUa_BadInvalidArgument;
    }

    return OpcUa_Good;
}

UaStatus OpticalAlignmentController::operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    UaStatus status;
    //UaMutexLocker lock(&m_mutex);
    spdlog::trace("Offset: {}", offset);
    if (m_State == Device::DeviceState::Busy) {
        spdlog::error("{} : OpticalAlignmentController::operate() : Device is busy, method call aborted.", m_Identity);
        return OpcUa_BadInvalidState;
    }

    switch (offset) {
        case PAS_OpticalAlignmentType_CalibrateFirstOrderCorr: {
            spdlog::info("OpticalAlignmentController::operate() :  Calling CalibrateFirstOrderCorr...");

            /*
             * Pseudocode for first order calibration strategy. This strategy corrects motion from pattern to focal point per panel
             */

            _loadPatternImageParameters(); //get best (human derived) parameters to analyze image for this ring that label panels properly.  This should come from focalplaneimage object.

            double target_coordinates_center_x = args[0].Value.Double;// [x,y]
            double target_coordinates_center_y = args[1].Value.Double ;// [x,y]
            bool show_plot = args[2].Value.Boolean; //bool
            double offset_limit = args[3].Value.Double; // float, in pixels. Max distance to target position

            spdlog::debug("Target Coordinates: ({}, {})", target_coordinates_center_x, target_coordinates_center_y);
            spdlog::debug("Minimum distance to target: {}", offset_limit);
            spdlog::debug("Show plots in between steps? {}", show_plot);

            double image_delta_x;
            double image_delta_y;
            int panel;
            double distance_panel_to_target;

            Eigen::VectorXd deltaActLengths(6);
            std::map <int , std::vector<double>> coordinates_per_panel;

            std::vector<double> panel_coordinates;

            Eigen::VectorXd actuator_deltas(6);
            Eigen::VectorXd total_actuator_deltas(6);

            std::string image_filepath = _captureSingleImage();
            coordinates_per_panel = _analyzeImagePatternAutomatically(image_filepath, show_plot);

            for (const auto& panel_item : coordinates_per_panel) {
                image_filepath = _captureSingleImage();
                coordinates_per_panel = _analyzeImagePatternAutomatically(image_filepath, show_plot);

                panel = panel_item.first;
                panel_coordinates = panel_item.second;

                image_delta_x = panel_coordinates[0] - target_coordinates_center_x;
                image_delta_y = panel_coordinates[1] - target_coordinates_center_y;

                distance_panel_to_target = sqrt(image_delta_x*image_delta_x + image_delta_y*image_delta_y);

                // while statement to repeat motion/analysis until panel reaches target within offset_limit.
                while (distance_panel_to_target > offset_limit) {
                    {
                    actuator_deltas = _calculateActuatorMotion(panel, image_delta_x, image_delta_y);
                    auto pPanel = std::dynamic_pointer_cast<PanelController>(m_ChildrenPositionMap.at(PAS_PanelType).at(panel));
//                    pPanel->__moveDeltaLengths(actuator_deltas);

                    image_filepath = _captureSingleImage();
                    panel_coordinates = _analyzeImageSinglePanelAutomatically(image_filepath, show_plot);

                    image_delta_x = panel_coordinates[0] - target_coordinates_center_x;
                    image_delta_y = panel_coordinates[1] - target_coordinates_center_y;

                    total_actuator_deltas += actuator_deltas;

                    distance_panel_to_target = sqrt(image_delta_x*image_delta_x + image_delta_y*image_delta_y);
                    }
                m_corrected_actuator_lengths_per_panel.at(panel) = total_actuator_deltas;
                }
             }
            _saveCorrections(m_corrected_actuator_lengths_per_panel); //save these corrected values to file/DB. Also use elsewhere here during this session.
            status = OpcUa_Good;
            break;
        }
        case PAS_OpticalAlignmentType_MoveFocusToPattern: {
            spdlog::info("OpticalAlignmentController::operate() :  Calling MoveFocusToPattern...");

            status = OpcUa_Good;
            break;
        }
        case PAS_OpticalAlignmentType_MovePatternToFocus: {
            spdlog::info("OpticalAlignmentController::operate() :  Calling MovePatternToFocus...");

            status = OpcUa_Good;
            break;
        }
        case PAS_OpticalAlignmentType_MoveForCalibration: {
            spdlog::info("OpticalAlignmentController::operate() :  Calling MoveForCalibration...");

            status = OpcUa_Good;
            break;
        }
        case PAS_OpticalAlignmentType_GetResponseMatrix: {
            spdlog::info("OpticalAlignmentController::operate() :  Calling GetResponseMatrix...");

            status = OpcUa_Good;
            break;
        }
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

std::string OpticalAlignmentController::_captureSingleImage() {
    std::string filename = "";
    return filename;
}

std::map<int, std::vector<double>> OpticalAlignmentController::_analyzeImagePatternAutomatically(std::string image_filepath, bool plot) {
    string command;
    command = m_focalPlaneImage.analyzePatternCommand();
    spdlog::debug(command);

    string ret;
    ret = m_focalPlaneImage.exec(command.c_str());
    spdlog::info(ret);
    return std::map<int, std::vector<double>>();
}

std::vector<double> OpticalAlignmentController::_analyzeImageSinglePanelAutomatically(std::string image_filepath, bool plot) {
    string command;
    command = m_focalPlaneImage.analyzeSinglePanelCommand();
    spdlog::debug(command);

    string ret;
    ret = m_focalPlaneImage.exec(command.c_str());
    spdlog::info(ret);

    return std::vector<double>();
}

Eigen::VectorXd OpticalAlignmentController::_calculateActuatorMotion(int panel, double x, double y) {
    return Eigen::VectorXd();
}

bool OpticalAlignmentController::_saveCorrections(std::map<int, Eigen::VectorXd> map) {
    return true;
}

void OpticalAlignmentController::_loadPatternImageParameters() {
    std::shared_ptr<PasController> pController = m_pChildren[PAS_FocalPlaneType][0];
    UaVariant value;

    pController->getData(PAS_FocalPlaneType_DETECT_MINAREA, value);
    value.toInt32(m_focalPlaneImage.m_imgAnalysisParams.m_DetectMinArea);
    spdlog::trace("m_DetectMinArea: {}",m_focalPlaneImage.m_imgAnalysisParams.m_DetectMinArea);
    pController->getData(PAS_FocalPlaneType_DEBLEND_MINCONT, value);
    value.toDouble(m_focalPlaneImage.m_imgAnalysisParams.m_DeblendMinCont);
    spdlog::trace("m_DeblendMinCont: {}",m_focalPlaneImage.m_imgAnalysisParams.m_DeblendMinCont);
    pController->getData(PAS_FocalPlaneType_SEARCH_XS, value);
    m_focalPlaneImage.m_imgAnalysisParams.m_SearchXs = value.toString().toUtf8();
    spdlog::trace("m_SearchXs: {}",m_focalPlaneImage.m_imgAnalysisParams.m_SearchXs);
    pController->getData(PAS_FocalPlaneType_SEARCH_YS, value);
    m_focalPlaneImage.m_imgAnalysisParams.m_SearchYs = value.toString().toUtf8();
    spdlog::trace("m_SearchYs: {}",m_focalPlaneImage.m_imgAnalysisParams.m_SearchYs);
    pController->getData(PAS_FocalPlaneType_THRESH, value);
    value.toInt32(m_focalPlaneImage.m_imgAnalysisParams.m_Thresh);
    spdlog::trace("m_Thresh: {}",m_focalPlaneImage.m_imgAnalysisParams.m_Thresh);

    pController->getData(PAS_FocalPlaneType_PatternRadius, value);
    value.toDouble(m_focalPlaneImage.m_PatternRadius);
    pController->getData(PAS_FocalPlaneType_PhaseOffsetRad, value);
    value.toDouble(m_focalPlaneImage.m_PhaseOffsetRad);
    pController->getData(PAS_FocalPlaneType_RingFrac, value);
    value.toDouble(m_focalPlaneImage.m_RingFrac);
    pController->getData(PAS_FocalPlaneType_MinDist, value);
    value.toDouble(m_focalPlaneImage.m_MinDist);
    pController->getData(PAS_FocalPlaneType_PatternCenter, value);
    m_focalPlaneImage.m_PatternCenter = value.toString().toUtf8();
    pController->getData(PAS_FocalPlaneType_RingTol, value);
    value.toDouble(m_focalPlaneImage.m_RingTol);

}
