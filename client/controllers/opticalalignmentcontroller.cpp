#include <algorithm>
#include "opticalalignmentcontroller.hpp"

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
    std::vector<unsigned> deviceTypesToCheck = {PAS_PanelType, PAS_FocalPlaneType};
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
        case PAS_OpticalAlignmentType_SelectedPanels : {
            std::vector<unsigned> v(m_selectedPanels.begin(), m_selectedPanels.end());
            std::string s = "[";
            for (int i = 0; i < (int)(v.size()); i++) {
                s += std::to_string(v[i]);
                if (i != (int)(v.size() - 1)) {
                    s += ", ";
                }
            }
            s += "]";
            value.setString(s.c_str());
            spdlog::trace("{} : Read SelectedPanels value => ({})", m_Identity, s.c_str());
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
        case PAS_OpticalAlignmentType_SelectedPanels: {
            std::string selectionString = value.toString().toUtf8();
            spdlog::debug("{}: Adding {}", m_Identity, selectionString);
            parseAndSetSelection(selectionString, PAS_PanelType, 0);
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

    if (m_State == Device::DeviceState::Busy) {
        spdlog::error("{} : OpticalAlignmentController::operate() : Device is busy, method call aborted.", m_Identity);
        return OpcUa_BadInvalidState;
    }

    switch (offset) {
        case PAS_OpticalAlignmentType_CalibrateFirstOrderCorr: {
            spdlog::info("OpticalAlignmentController::operate() :  Calling CalibrateFirstOrderCorr...");

            _loadPatternImageParameters(); //get best (human derived) parameters to analyze image for this ring that label panels properly.  This should come from focalplaneimage object.

            m_target_coordinates_center_x = args[0].Value.Double;// [x,y]
            m_target_coordinates_center_y = args[1].Value.Double ;// [x,y]
            m_show_plot = args[2].Value.Boolean; //bool
            m_offset_limit = args[3].Value.Double; // float, in pixels. Max distance to target position
            m_respFile = UaString(args[4].Value.String).toUtf8();

            m_processing = true;
            start();

            status = OpcUa_Good;
            break;
        }
        case PAS_OpticalAlignmentType_StopProcess: {
            spdlog::info("OpticalAlignmentController::operate() :  Calling StopProcess...");
            if (m_processing) {
                m_processing = 0;
            }
            break;
        }
        case PAS_OpticalAlignmentType_MoveFocusToPattern: {
            spdlog::info("OpticalAlignmentController::operate() :  Calling MoveFocusToPattern...");

            // Todo implement this move for all panels selected from focus to pattern. Use calc_motion.py to get delta motion
            status = OpcUa_Good;
            break;
        }
        case PAS_OpticalAlignmentType_MovePatternToFocus: {
            spdlog::info("OpticalAlignmentController::operate() :  Calling MovePatternToFocus...");

            // Todo implement this move for all panels selected from pattern to focus. Use calc_motion.py to get delta motion
            status = OpcUa_Good;
            break;
        }
        case PAS_OpticalAlignmentType_MoveForCalibration: {
            spdlog::info("OpticalAlignmentController::operate() :  Calling MoveForCalibration...");

            // Todo implement this move for all panels. Use calc_motion.py to get delta motion. Motion should be +/- 1 in Rx and Ry, don't save automatically
            status = OpcUa_Good;
            break;
        }
        case PAS_OpticalAlignmentType_GetResponseMatrix: {
            spdlog::info("OpticalAlignmentController::operate() :  Calling GetResponseMatrix...");

            // Todo Use calc_motion.py to get response matrix, with prior motions and 5 images
            status = OpcUa_Good;
            break;
        }
        case  PAS_OpticalAlignmentType_SelectAll: {
            spdlog::info("OpticalAlignmentController::operate() : Calling SelectAll()...");
            m_selectedPanels.clear();
            for (const auto &panel : getChildren(PAS_PanelType)) {
                spdlog::trace("Found panel: {}", panel->getIdentity().position);
                m_selectedPanels.insert((unsigned) panel->getIdentity().position);
            }
            break;
        }
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

void OpticalAlignmentController::calibrateFirstOrderCorrection() {
    double image_delta_x;
    double image_delta_y;
    double image_delta_x_resampled;
    double image_delta_y_resampled;
    int panel;
    double distance_panel_to_target;
    double distance_panel_to_target_resampled;
    int n_correction_tries;

    Eigen::VectorXd deltaCoords(6);
    map <int , vector<double>> starting_coordinates_per_panel;
    map <int , vector<double>> resampled_coordinates_per_panel;

    vector<double> current_panel_coordinates;

    Eigen::VectorXd curr_coords_deltas_for_panel(6);
    Eigen::VectorXd total_coord_deltas(6);

#ifdef SIMMODE
    std::string image_filepath = "/app/focal_plane/data/The Imaging Source Europe GmbH-37514083-2592-1944-Mono8-2019-12-17-03:20:27.raw";
    m_focalPlaneImage.setDataDir("/app/focal_plane/data/");
#else
    std::string image_filepath = _captureSingleImage();
#endif
    starting_coordinates_per_panel = _analyzeImagePatternAutomatically(image_filepath, m_show_plot);
    resampled_coordinates_per_panel = starting_coordinates_per_panel;

    spdlog::debug("Starting loop");

    for (const auto &panel_item : starting_coordinates_per_panel) {
        if (m_processing) {
            panel = panel_item.first;
            current_panel_coordinates = panel_item.second;
            bool pController_exists =
                    m_ChildrenPositionMap.at(PAS_PanelType).find(panel) !=
                    m_ChildrenPositionMap.at(PAS_PanelType).end();
            bool panel_selected = (m_selectedPanels.find(panel) != m_selectedPanels.end());

            if (!pController_exists | !panel_selected) {
                spdlog::warn("{}: This panel not available or selected. Moving on...", panel);
                continue;
            }

            spdlog::info("Capturing starting image for panel {}", panel);

#ifdef SIMMODE
            image_filepath = "/app/focal_plane/data/The Imaging Source Europe GmbH-37514083-2592-1944-Mono8-2019-12-17-03:20:27.raw";
            m_focalPlaneImage.setDataDir("/app/focal_plane/data/");
#else
            image_filepath = _captureSingleImage();
#endif
            resampled_coordinates_per_panel = _analyzeImagePatternAutomatically(image_filepath, m_show_plot);
            spdlog::info("Remeasured pattern in case coordinates changed over time.");

            image_delta_x_resampled =
                    resampled_coordinates_per_panel.at(panel)[0] - m_target_coordinates_center_x;
            image_delta_y_resampled =
                    resampled_coordinates_per_panel.at(panel)[1] - m_target_coordinates_center_y;
            distance_panel_to_target_resampled = sqrt(image_delta_x_resampled * image_delta_x_resampled +
                                                      image_delta_y_resampled * image_delta_y_resampled);

            image_delta_x = current_panel_coordinates[0] - m_target_coordinates_center_x;
            image_delta_y = current_panel_coordinates[1] - m_target_coordinates_center_y;
            distance_panel_to_target = sqrt(image_delta_x * image_delta_x + image_delta_y * image_delta_y);

            spdlog::info("{}: New pattern position changed from ({}, {}) to ({}, {}) since this loop started",
                         panel,
                         current_panel_coordinates[0], current_panel_coordinates[1],
                         resampled_coordinates_per_panel.at(panel)[0],
                         resampled_coordinates_per_panel.at(panel)[1]);

            spdlog::info("{}: Using the new coordinates...", panel);

            current_panel_coordinates = resampled_coordinates_per_panel.at(panel);

            image_delta_x = current_panel_coordinates[0] - m_target_coordinates_center_x;
            image_delta_y = current_panel_coordinates[1] - m_target_coordinates_center_y;
            distance_panel_to_target = sqrt(image_delta_x * image_delta_x + image_delta_y * image_delta_y);

            spdlog::info("{}: Current coordinates ({},{}), {} from target.", panel,
                         current_panel_coordinates[0],
                         current_panel_coordinates[1], distance_panel_to_target);

            n_correction_tries = 0;
            // while statement to repeat motion/analysis until panel reaches target within m_offset_limit.
            while ((distance_panel_to_target > m_offset_limit) & m_processing) {
                if (n_correction_tries > 5 ) { break;}
                spdlog::info("Still far from target {} > {} ", distance_panel_to_target, m_offset_limit);

                curr_coords_deltas_for_panel = _calculatePanelMotion(panel, image_delta_x, image_delta_y,
                                                                     m_respFile);
                spdlog::info("{}: Found actuator deltas for this motion to target", panel);
                try {
                    auto pPanel = dynamic_pointer_cast<PanelController>(
                            m_ChildrenPositionMap.at(PAS_PanelType).at(panel));
                    _doSafePanelMotion(pPanel, curr_coords_deltas_for_panel);
                    pPanel->getActuatorLengths();
                    //                MirrorController::__calculateMoveDeltaCoords();
                    //                pPanel->__moveDeltaLengths(curr_coords_deltas_for_panel);

                }
                catch (std::out_of_range &oor) {
                    spdlog::error("{}: Not found in controller list. Moving on to a different panel... ({})",
                                  panel,
                                  oor.what());
                    break;
                }
                spdlog::info("{}: Moved to new position", panel);

#ifdef SIMMODE
                image_filepath = "/app/focal_plane/data/The Imaging Source Europe GmbH-37514083-2592-1944-Mono8-2019-12-17-03:20:27.raw";
                m_focalPlaneImage.setDataDir("/app/focal_plane/data/");
#else
                image_filepath = _captureSingleImage();
#endif
                current_panel_coordinates = _analyzeImageSinglePanelAutomatically(image_filepath, m_show_plot);
                spdlog::info("{}: Took new image and found new panel coordinate", panel);

                image_delta_x = current_panel_coordinates[0] - m_target_coordinates_center_x;
                image_delta_y = current_panel_coordinates[1] - m_target_coordinates_center_y;

                total_coord_deltas += curr_coords_deltas_for_panel;

                distance_panel_to_target = sqrt(image_delta_x * image_delta_x + image_delta_y * image_delta_y);
                spdlog::info("{} Current coordinates ({},{}), {} from target.", panel,
                             current_panel_coordinates[0],
                             current_panel_coordinates[1], distance_panel_to_target);

                n_correction_tries++;
            }
            m_corrected_coordinate_deltas_per_panel[panel] = total_coord_deltas;
            spdlog::info(
                    "{}: Total coordinate deltas (pattern position to focus point) found for this panel after 1st order correction",
                    panel);

            spdlog::info("{}: Returning this panel to pattern position", panel);

            curr_coords_deltas_for_panel = _calculatePanelMotion(panel, image_delta_x, image_delta_y, m_respFile);
            curr_coords_deltas_for_panel *= -1;
            spdlog::info("{}: Found actuator deltas for this motion to target", panel);

            try {
                auto pPanel = dynamic_pointer_cast<PanelController>(
                        m_ChildrenPositionMap.at(PAS_PanelType).at(panel));
                _doSafePanelMotion(pPanel, curr_coords_deltas_for_panel);
                pPanel->getActuatorLengths();
                //                MirrorController::__calculateMoveDeltaCoords();
                //                pPanel->__moveDeltaLengths(curr_coords_deltas_for_panel);
            }
            catch (std::out_of_range & oor) {
                spdlog::error("{}: Not found in controller list. Moving on to a different panel... ({})", panel,
                              oor.what());
                break;
            }


            spdlog::info("Completed calibration of panel {}. Moving on...", panel);
        }
        else {
            break;
        }
    }
    _saveCorrections(
            m_corrected_coordinate_deltas_per_panel); //save these corrected values to file/DB. Also use elsewhere here during this session.
    spdlog::info("Completed calibration of all panels. ");
}

std::string OpticalAlignmentController::_captureSingleImage() {
    m_focalPlaneImage.saveImage();
    std::string filename = m_focalPlaneImage.get_image_file();
    return filename;
}

std::map<int, std::vector<double>> OpticalAlignmentController::_analyzeImagePatternAutomatically(const std::string& image_filepath, bool plot) {
    string command;
    m_focalPlaneImage.m_ImageFile = image_filepath;
    m_focalPlaneImage.m_show = plot;
    spdlog::info("Image to analyze: {}", m_focalPlaneImage.get_image_file());
    command = m_focalPlaneImage.analyzePatternCommand();
    spdlog::debug(command);

    string ret;
    ret = focalplane::exec(command.c_str());
    spdlog::info(ret);

    spdlog::info("Loading output CSV file");
    spdlog::trace("Getting csv filepath from {}.", m_focalPlaneImage.m_ImageFile);
    string csv_file = m_focalPlaneImage.getCSVFilepathFromImageName(image_filepath);
    spdlog::trace("Reading csv file {} ", csv_file);
    std::vector<std::vector<std::string>> csv_results = focalplane::getCSVData(csv_file);

    spdlog::trace("Converting csv vectors to coordinate map");
    std::map<int, std::vector<double>> coordinate_map = m_focalPlaneImage.makePanelCoordinateMap(csv_results);
    
    return coordinate_map;
}

std::vector<double> OpticalAlignmentController::_analyzeImageSinglePanelAutomatically(const std::string& image_filepath, bool plot) {
    string command;
    m_focalPlaneImage.m_ImageFile = image_filepath;
    m_focalPlaneImage.m_show = plot;
    spdlog::info("Image to analyze: {}", m_focalPlaneImage.get_image_file());
    command = m_focalPlaneImage.analyzeSinglePanelCommand();
    //TODO need to fix focal_plane.py so that the output of this is either _vvv.cvs file, or a printout to regex.
    spdlog::debug(command);

    string ret;
    ret = focalplane::exec(command.c_str());
    spdlog::info(ret);

    spdlog::debug("Loading output CSV file");
    spdlog::trace("Getting csv filepath.");
    std::string csv_file = m_focalPlaneImage.getCSVFilepathFromImageName(image_filepath);
    spdlog::trace("Reading csv file {} ", csv_file);
    std::vector<std::vector<std::string>> csv_results = focalplane::getCSVData(csv_file);

    spdlog::trace("Converting csv vectors to coordinate map");
    std::map<int, std::vector<double>> coordinate_map = m_focalPlaneImage.makePanelCoordinateMap(csv_results);

    std::vector<double> panel_coordinates;

#ifdef SIMMODE
    panel_coordinates = {1700.9,900.9};
#endif

    return panel_coordinates;
}

Eigen::VectorXd OpticalAlignmentController::_calculatePanelMotion(int panel, double x, double y, std::string respFile) {
    Eigen::VectorXd panel_motion_coords(6);
    panel_motion_coords << 0,0,0,0,0,0 ;
    string command;
    command = m_focalPlaneImage.CalcMotionSinglePanel2center(panel, x, y, respFile);

    spdlog::debug(command);

    string ret;
    ret = focalplane::exec(command.c_str());
    spdlog::info(ret);

    const char * raw_literal_expr = R"rxp(rx.{3}(\d{1,4}\.\d{1,4}).{3}ry.{3}(\d{1,4}\.\d{1,4}))rxp";
    spdlog::trace("Regex search pattern: {}", raw_literal_expr);

    boost::regex expr(raw_literal_expr);
    boost::smatch what;

    if (boost::regex_search(ret, what, expr))
    {
        spdlog::trace("Found something during regex");
        spdlog::trace("Response: {}", what[0].str());
        panel_motion_coords[3] = std::stod(what[1]);
        panel_motion_coords[4] = std::stod(what[2]);
    }
    else{
        spdlog::warn("Nothing came out of regex search for this motion.");
    }

#ifdef SIMMODE
    panel_motion_coords << .1, .1 , .1, .1, .1, .1 ;
    sleep(2);
#endif
    return panel_motion_coords;
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
    value.toInt32(m_focalPlaneImage.m_PatternRadius);
    pController->getData(PAS_FocalPlaneType_PhaseOffsetRad, value);
    value.toDouble(m_focalPlaneImage.m_PhaseOffsetRad);
    pController->getData(PAS_FocalPlaneType_RingFrac, value);
    value.toDouble(m_focalPlaneImage.m_RingFrac);
    pController->getData(PAS_FocalPlaneType_MinDist, value);
    value.toInt32(m_focalPlaneImage.m_MinDist);
    pController->getData(PAS_FocalPlaneType_PatternCenter, value);
    m_focalPlaneImage.m_PatternCenter = value.toString().toUtf8();
    pController->getData(PAS_FocalPlaneType_RingTol, value);
    value.toDouble(m_focalPlaneImage.m_RingTol);

}

bool
OpticalAlignmentController::_doSafePanelMotion(std::shared_ptr<PanelController> pPanel, Eigen::VectorXd deltaCoords) {
    UaStatus status;
    float deltaLength;
    float targetLength;
    Eigen::VectorXd deltaLengths(6);
    Eigen::VectorXd targetLengths(6);

    status = pPanel->updateCoords();

    spdlog::debug(
            "{} : PanelController::operate() : Current panel coordinates (x, y ,z xRot, yRot, zRot):\n{} {} {} {} {} {}\n",
            m_Identity, pPanel->m_curCoords[0], pPanel->m_curCoords[1], pPanel->m_curCoords[2], pPanel->m_curCoords[3], pPanel->m_curCoords[4], pPanel->m_curCoords[5]);

    double inputDeltaCoordinates[6];
    for (int i = 0; i < 6; i++) {
        inputDeltaCoordinates[i] = deltaCoords[i];
    }
    spdlog::debug(
            "{} : PanelController::operate() : Delta panel coordinates (x, y ,z xRot, yRot, zRot):\n{} {} {} {} {} {}\n",
            m_Identity, inputDeltaCoordinates[0], inputDeltaCoordinates[1], inputDeltaCoordinates[2],
            inputDeltaCoordinates[3],
            inputDeltaCoordinates[4], inputDeltaCoordinates[5]);

    double targetCoordinates[6];
    for (int i = 0; i < 6; i++) {
        targetCoordinates[i] = pPanel->m_curCoords[i] + inputDeltaCoordinates[i];
    }

    spdlog::debug(
            "{} : PanelController::operate() : Target panel coordinates (x, y ,z xRot, yRot, zRot):\n{} {} {} {} {} {}\n",
            m_Identity, targetCoordinates[0], targetCoordinates[1], targetCoordinates[2], targetCoordinates[3],
            targetCoordinates[4], targetCoordinates[5]);

    // find actuator lengths needed
    pPanel->m_SP.ComputeActsFromPanel(targetCoordinates);

    UaVariantArray lengthArgs;
    lengthArgs.create(6);
    UaVariant val;

    // Get actuator lengths for motion
    for (int i = 0; i < 6; i++) {
        targetLengths(i) = (float) pPanel->m_SP.GetActLengths()[i];
        val.setFloat(targetLengths(i));
        val.copyTo(&lengthArgs[i]);
    }
    spdlog::debug("{} : PanelController::operate() : Moving actuators to lengths:\n{}\n\n", m_Identity,
                  targetLengths);

    Eigen::VectorXd currentLengths;
    status = pPanel->__getActuatorLengths(currentLengths);

    if (status.isBad()) {
        spdlog::error("{}: Unable to moveDeltaCoords, failed to get actuator lengths.", m_Identity);
        return OpcUa_Bad;
    }

    deltaLengths = targetLengths - currentLengths;
    if (pPanel->checkForCollision(deltaLengths)) {
        return OpcUa_Bad;
    } else {
        status = pPanel->operate(PAS_PanelType_MoveToLengths, lengthArgs);
    }

    // Wait for completion
    spdlog::info("{}: Waiting for all motions to complete...", m_Identity);
    UaThread::sleep(5);
    bool stillMoving = true;
    while (stillMoving) {
        stillMoving = false;
        Device::DeviceState state;
        getState(state);
        if (state == Device::DeviceState::Busy) {
            spdlog::trace("{}: Panel {} is still busy...", m_Identity, getIdentity());
            stillMoving = true;
        } else {
            spdlog::trace("{}: Panel {} is idle.", m_Identity, getIdentity());
        }
        UaThread::sleep(1);
    }
    spdlog::info("{}: Done! All motions completed for MoveDeltaCoords method.", m_Identity);


}

void OpticalAlignmentController::run() {
    /*
     * Pseudocode for first order calibration strategy. This strategy corrects motion from pattern to focal point per panel
     */

    spdlog::debug("Target Coordinates: ({}, {})", m_target_coordinates_center_x, m_target_coordinates_center_y);
    spdlog::debug("Minimum distance to target: {}", m_offset_limit);
    spdlog::debug("Show plots in between steps? {}", m_show_plot);

    calibrateFirstOrderCorrection();
}

void OpticalAlignmentController::parseAndSetSelection(const string &selectionString, unsigned int deviceType,
                                                      int usage_type) {

    // process a separated string and find the panels or edges described by it
    // pad by a space from the right so we don't hit the end of the line without a delimiter

    // Strip leading and trailing brackets if present
    int front = 0;
    int back = selectionString.size();
    if (selectionString.front() == '[') {
        front++;
    }
    if (selectionString.back() == ']') {
        back--;
    }

    std::string inStr = selectionString.substr(front,back) + std::string(" ");
    std::vector<std::string> strList;
    // working with comma, space and semicolon
    std::string delim = " ,;:\"\'{}";
    size_t prev = 0, cur;
    while ((cur = inStr.find_first_of(delim, prev)) != std::string::npos) {
        if (cur > prev)
            strList.push_back(inStr.substr(prev, cur-prev));
        prev = cur + 1;
    }

    // add all the items to the selected children set of indices
    if (deviceType == PAS_PanelType && usage_type==0) { // expect a list of panel positions
        m_selectedPanels.clear();
        unsigned curpos;
        std::ostringstream os;
        for (const std::string &item : strList) {
            curpos = stoi(item);
            if (m_ChildrenPositionMap.at(deviceType).count(curpos) > 0) {
                m_selectedPanels.insert(curpos);
                os << "Added Panel with position " << curpos << "." << std::endl;
            }
            else {
                os << "Unable to find Panel with position " << curpos << ". Skipping..." << std::endl;
            }
        }
        os << std::endl;
        spdlog::info("{}: Selecting panels (positions):\n{}\n", m_Identity, os.str());
    }
}
