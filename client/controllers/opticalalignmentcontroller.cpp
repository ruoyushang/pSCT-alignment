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

#include "client/controllers/edgecontroller.hpp"
#include "client/controllers/mpescontroller.hpp"
#include "client/controllers/panelcontroller.hpp"
#include "client/controllers/actcontroller.hpp"
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
    m_ChildrenTypes = {PAS_FocalPlaneType};

    // make sure things update on the first boot up
    // duration takes seconds -- hence the conversion with the 1/1000 ratio
    m_LastUpdateTime = TIME::now() - std::chrono::duration<int, std::ratio<1, 1000>>(m_kUpdateInterval_ms);
}

void OpticalAlignmentController::addChild(OpcUa_UInt32 deviceType, const std::shared_ptr<PasController>& pController)
{
    // call the base type's method
    PasCompositeController::addChild(deviceType, pController);

//    if (deviceType == PAS_PanelType) {
//        m_selectedPanels.insert((unsigned) pController->getIdentity().position);
//    }
//    else if (deviceType == PAS_EdgeType) {
//        m_selectedEdges.insert(pController->getIdentity().eAddress);
//    }
//    else if (deviceType == PAS_MPESType) {
//        m_selectedMPES.insert(pController->getIdentity().serialNumber);
//    }
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
    std::vector<unsigned> deviceTypesToCheck = {PAS_MPESType};
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

//    UaStatus status;
//    if (offset...)
//    else
//    {
//        status = OpcUa_BadInvalidArgument;
//    }
    return OpcUa_Good;
}

UaStatus OpticalAlignmentController::setData(OpcUa_UInt32 offset, UaVariant value)
{
    //UaMutexLocker lock(&m_mutex);

//    if (offset ...) {
//    } else
//        status = OpcUa_BadInvalidArgument;


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
             *

             loadPatternImageParameters() //get best (human derived) parameters to analyze image for this ring that label panels properly.  This should come from focalplaneimage object.

             args[0] = target_coordinates_center_x // [x,y]
             args[1] = target_coordinates_center_y // [x,y]
             args[2] = show_plot //bool
             args[3] = offset_limit // float, in pixels. Max distance to target position

             map <int panel, set(float x,float y) > coordinates_per_panel;
             map <int panel, eigen actuator_lengths(6) > m_corrected_actuator_lengths_per_panel;

             image_filepath = captureSingleImage();
             coordinates_per_panel = analyzeImagePatternAutomatically(image_filepath,show_plot);

             for panel_item in coordinates_per_panel {
                image_filepath = captureSingleImage();
                coordinates_per_panel = analyzeImagePatternAutomatically(image_filepath, show_plot);

                int panel = panel_item.first;
                set panel_coordinates = panel_item.second;

                actuator_deltas = eigen.empty(6);
                total_actuator_deltas = eigen.empty(6);

                image_delta_x = panel_coordinates.X - target_coordinates_center_x;
                image_delta_y = panel_coordinates.Y - target_coordinates_center_y;

                distance_panel_to_target = math.sqrt(image_delta_x**2+image_delta_y**2);

                // while statement to repeat motion/analysis until panel reaches target within offset_limit.
                while (distance_panel_to_target > offset_limit) {
                    {
                    actuator_deltas = calculateActuatorMotion(panel, imageDeltaX, imageDeltaY);
                    panel->moveDeltaLength(actuator_deltas);

                    image_filepath = captureSingleImage();
                    panel_coordinates = analyzeImageSinglePanelAutomatically(image_filepath,show_plot);

                    image_delta_x = panel_coordinates.X - target_coordinates_center_x;
                    image_delta_y = panel_coordinates.Y - target_coordinates_center_y;

                    total_actuator_deltas += actuator_deltas;

                    distance_panel_to_target = math.sqrt(image_delta_x**2+image_delta_y**2);
                    }
                m_corrected_actuator_lengths_per_panel.find(panel) = total_actuator_deltas;
                }
            save(m_corrected_actuator_lengths_per_panel) //save these corrected values to file/DB. Also use elsewhere here during this session.

            */
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