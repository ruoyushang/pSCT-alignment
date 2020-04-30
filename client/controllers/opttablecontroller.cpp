//
// Created by Deivid Ribeiro on 4/27/20.
//

#include "opttablecontroller.hpp"

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
#include "client/controllers/psdcontroller.hpp"

#include "client/objects/panelobject.hpp"

#include "uathread.h"

// MySQL C++ Connector includes
#include "mysql_driver.h"
#include "cppconn/statement.h"
#include "DBConfig.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

OptTableController::OptTableController(Device::Identity identity, Client *pClient)
        : PasCompositeController(
        std::move(identity), nullptr,
        10000){
    // define possible children and initialize the selected children string
    m_ChildrenTypes = {PAS_PSDType, PAS_PanelType};

    // make sure things update on the first boot up
    // duration takes seconds -- hence the conversion with the 1/1000 ratio
    m_LastUpdateTime = TIME::now() - std::chrono::duration<int, std::ratio<1, 1000>>(m_kUpdateInterval_ms);
}

void OptTableController::addChild(OpcUa_UInt32 deviceType, const std::shared_ptr<PasController>& pController)
{
    // call the base type's method
    PasCompositeController::addChild(deviceType, pController);

    if (deviceType == PAS_PanelType) {
        m_selectedPanels.insert((unsigned) pController->getIdentity().position);
    }
    else if (deviceType == PAS_PSDType) {
        m_selectedPSD.insert(pController->getIdentity().serialNumber);
    }
//    else if (deviceType == PAS_MPESType) {
//        m_selectedMPES.insert(pController->getIdentity().serialNumber);
//    }
}


bool OptTableController::initialize()
{
    spdlog::info("{} : OptTableController::initialize(): Initializing...", m_Identity);

    spdlog::info("{}: OptTableController::initialize(): Done.", m_Identity);

    return true;
}

UaStatus OptTableController::getState(Device::DeviceState &state)
{
    //UaMutexLocker lock(&m_mutex);
    Device::DeviceState s;
    std::vector<unsigned> deviceTypesToCheck = {PAS_PSDType, PAS_PanelType};
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

UaStatus OptTableController::setState(Device::DeviceState state)
{
    //UaMutexLocker lock(&m_mutex);
    m_State = state;
    spdlog::trace("{} : Setting device state => ({})", m_Identity, (int)state);
    return OpcUa_Good;
}

UaStatus OptTableController::getData(OpcUa_UInt32 offset, UaVariant &value)
{
    //UaMutexLocker lock(&m_mutex);
//Todo: need to add data variables to collect
    UaStatus status;
    switch (offset) {
        case 1: {
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

UaStatus OptTableController::setData(OpcUa_UInt32 offset, UaVariant value)
{
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;
//Todo: need to add data variables to set
    switch (offset) {
        case 1: {
            OpcUa_Boolean val;
            value.toBool(val);
            spdlog::trace("{} : Setting Verbosity value... value => ({})", m_Identity, val);
            break;
        }
        case 2: {
            std::string selectionString = value.toString().toUtf8();
            spdlog::debug("{}: Adding {}", m_Identity, selectionString);
//            parseAndSetSelection(selectionString, PAS_PanelType, 0);
            break;
        }
        default:
            return OpcUa_BadInvalidArgument;
    }

    return OpcUa_Good;
}

UaStatus OptTableController::operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    UaStatus status;
    //UaMutexLocker lock(&m_mutex);

    if (m_State == Device::DeviceState::Busy) {
        spdlog::error("{} : OptTableController::operate() : Device is busy, method call aborted.", m_Identity);
        return OpcUa_BadInvalidState;
    }

    switch (offset) {
        case PAS_OptTableType_Align: {
            spdlog::info("OptTableController::operate() :  Calling Align...");

            //Todo need to create align method
            status = OpcUa_Good;
            break;
        }
        case PAS_OptTableType_FindMatrix: {
            spdlog::info("OptTableController::operate() :  Calling FindMatrix...");

            //todo need to create FindMatrix method.
            status = OpcUa_Good;
            break;
        }
        case PAS_OptTableType_StepAll: {
            spdlog::info("OptTableController::operate() :  Calling StepAll...");

            status = OpcUa_Good;
            break;
        }
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}
