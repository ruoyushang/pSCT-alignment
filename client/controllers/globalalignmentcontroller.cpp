#include "globalalignmentcontroller.hpp"

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

GlobalAlignmentController::GlobalAlignmentController(Device::Identity identity, Client *pClient)
        : PasCompositeController(
        std::move(identity), nullptr,
        10000){
    // define possible children and initialize the selected children string
    m_ChildrenTypes = {PAS_OpticalAlignmentType, PAS_CCDType, PAS_OptTableType, PAS_PSDType, PAS_PanelType};

    // make sure things update on the first boot up
    // duration takes seconds -- hence the conversion with the 1/1000 ratio
    m_LastUpdateTime = TIME::now() - std::chrono::duration<int, std::ratio<1, 1000>>(m_kUpdateInterval_ms);
}

void GlobalAlignmentController::addChild(OpcUa_UInt32 deviceType, const std::shared_ptr<PasController>& pController)
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


bool GlobalAlignmentController::initialize()
{
    spdlog::info("{} : GlobalAlignmentController::initialize(): Initializing...", m_Identity);

    spdlog::info("{}: GlobalAlignmentController::initialize(): Done.", m_Identity);

    return true;
}

UaStatus GlobalAlignmentController::getState(Device::DeviceState &state)
{
    //UaMutexLocker lock(&m_mutex);
    Device::DeviceState s;
    std::vector<unsigned> deviceTypesToCheck = {PAS_CCDType, PAS_OpticalAlignmentType, PAS_PSDType, PAS_PanelType};
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

UaStatus GlobalAlignmentController::setState(Device::DeviceState state)
{
    //UaMutexLocker lock(&m_mutex);
    m_State = state;
    spdlog::trace("{} : Setting device state => ({})", m_Identity, (int)state);
    return OpcUa_Good;
}

UaStatus GlobalAlignmentController::getData(OpcUa_UInt32 offset, UaVariant &value)
{
    //UaMutexLocker lock(&m_mutex);

    UaStatus status;
    switch (offset) {
        case PAS_GlobalAlignmentType_Tracking: {
            bool tracking;
            tracking = m_tracking;
            value.setBool(tracking);
            spdlog::trace("{} : Read Tracking value => ({})", m_Identity, tracking);
            break;
        }
        default:
            return OpcUa_BadInvalidArgument;
    }
    return OpcUa_Good;
}

UaStatus GlobalAlignmentController::setData(OpcUa_UInt32 offset, UaVariant value)
{
    //UaMutexLocker lock(&m_mutex);

//    if (offset ...) {
//    } else
//        status = OpcUa_BadInvalidArgument;


    return OpcUa_Good;
}

UaStatus GlobalAlignmentController::operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    UaStatus status;
    //UaMutexLocker lock(&m_mutex);
    spdlog::trace("Offset: {}", offset);
    if (m_State == Device::DeviceState::Busy) {
        spdlog::error("{} : GlobalAlignmentController::operate() : Device is busy, method call aborted.", m_Identity);
        return OpcUa_BadInvalidState;
    }

    switch (offset) {
        case PAS_GlobalAlignmentType_StartPSDTrack: {
            spdlog::info("GlobalAlignmentController::operate() :  Calling StartPSDTracking...");

            //pLogic->start();
            m_tracking = OpcUa_True;
            status = OpcUa_Good;
            break;
        }
        case PAS_GlobalAlignmentType_StopPSDTrack: {
            spdlog::info("GlobalAlignmentController::operate() :  Calling StopPSDTracking...");

            //pLogic->stop();
            m_tracking = OpcUa_False;
            status = OpcUa_Good;
            break;
        }
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}
