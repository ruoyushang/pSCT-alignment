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
#include <common/alignment/actuator.hpp>
#include <common/opcua/pasobject.hpp>

#include "common/alignment/device.hpp"
#include "common/simulatestewart/mathtools.hpp"
#include "common/simulatestewart/mirrordefinitions.hpp" // definitions of the mirror surfaces

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

#include "client/controllers/panelcontroller.hpp"
#include "client/controllers/actcontroller.hpp"

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
    try {
        for (auto devType : deviceTypesToCheck) {
            for (const auto &child : getChildren(devType)) {
                child->getState(s);
                if (s == Device::DeviceState::Busy) {
                    m_State = s;
                }
            }
        }
    }
    catch (const std::out_of_range& oor) {
        spdlog::warn("Out of Range error: ({}), No child device found on this selection.",oor.what());
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
//    UaStatus status;
//    switch (offset) {
//        case : {
//            bool tracking;
//            tracking = m_tracking;
//            value.setBool(tracking);
//            spdlog::trace("{} : Read Tracking value => ({})", m_Identity, tracking);
//            break;
//        }
//        default:
//            return OpcUa_BadInvalidArgument;
//    }
    return OpcUa_Good;
}

UaStatus OptTableController::setData(OpcUa_UInt32 offset, UaVariant value)
{
    //UaMutexLocker lock(&m_mutex);
//    UaStatus status;
//Todo: need to add data variables to set
//    switch (offset) {
//        case 1: {
//            OpcUa_Boolean val;
//            value.toBool(val);
//            spdlog::trace("{} : Setting Verbosity value... value => ({})", m_Identity, val);
//            break;
//        }
//        case 2: {
//            std::string selectionString = value.toString().toUtf8();
//            spdlog::debug("{}: Adding {}", m_Identity, selectionString);
////            parseAndSetSelection(selectionString, PAS_PanelType, 0);
//            break;
//        }
//        default:
//            return OpcUa_BadInvalidArgument;
//    }

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
            double temp;
            UaVariant(args[0]).toDouble(temp);
            spdlog::info("{} : OptTableController calling findMatrix() with step size {}.", m_Identity,
                         temp);
            status = findMatrix(args);
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

// NOTE: This method performs no safety checks for collision (for speed). It is assumed that a reasonably small step size
// will be used such that there is no risk of collision.
UaStatus OptTableController::findMatrix(UaVariantArray args) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    double stepSize;
    UaVariant(args[0]).toDouble(stepSize);

    unsigned numPanels = m_pChildren.at(PAS_PanelType).size();

    setState(Device::DeviceState::Busy);
    for (unsigned i = 0; i < numPanels; i++) {
        if (m_State != Device::DeviceState::Off) {
            status = findSingleMatrix(i, stepSize);
            if (!status.isGood()) {
                spdlog::error(
                        "{} : OptTableController::findMatrix() : Encountered error after first call to findSingleMatrix(). Motion aborted.",
                        m_Identity);
                return status;
            }
        }
        else {
            spdlog::warn("{} : OptTableController::findMatrix() : Table motion stop detected. Motion aborted.", m_Identity);
            break;
        }
    }
    setState(Device::DeviceState::On);

    return status;
}

// helper method for the above -- actually moving the panel and measuring the matrix
// NOTE: This method performs no safety checks for collision (for speed). It is assumed that a reasonably small step size
// will be used such that there is no risk of collision.
UaStatus OptTableController::findSingleMatrix(unsigned int panelIdx, double stepSize) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    bool stop = false;

    Eigen::MatrixXd responseMatrix(6, 6);
    responseMatrix.setZero();
    // convenience variable;
    // no need to check with a try/catch block anymore as this has already been done
    // by the caller
    std::shared_ptr<PanelController> pCurPanel = std::dynamic_pointer_cast<PanelController>(
            m_pChildren.at(PAS_PanelType).at(panelIdx));
    unsigned nACTs = pCurPanel->getActuatorCount();

    Eigen::VectorXd vector0(6); // maximum possible size
    Eigen::VectorXd vector1(6); // maximum possible size

    UaVariant minusdeltaL, deltaL, zeroDelta;
    deltaL.setFloat(stepSize);
    minusdeltaL.setFloat(-1 * stepSize);
    zeroDelta.setFloat(0.);

    UaVariantArray deltas;
    deltas.create(nACTs);

    UaVariant vtmp;
    std::shared_ptr<ActController> actuator;
    float missedDelta;

    for (unsigned j = 0; j < nACTs && !stop; j++) {
        // extra safety -- initialize array to zero every time
        for (unsigned k = 0; k < nACTs; k++)
            zeroDelta.copyTo(&deltas[k]);

        missedDelta = 0.;

        vector0 = __getCurrentReadings().first;
        spdlog::info("{} : PSD readings (initial):\n{}\n", m_Identity, vector0);

        spdlog::info("{} : Moving actuator {} by {} mm...", m_Identity, j + 1, stepSize);
        spdlog::info("{} : Waiting...", m_Identity);
        deltaL.copyTo(&deltas[j]);
        status = pCurPanel->__moveDeltaLengths(deltas);
        if (!status.isGood()) return status;
        // Stepping is asynchronous. but here, we want it to actually complete
        // before the next step. So we wait.
        // Need to change stepping to not be asynchronous?
        Device::DeviceState curState;
        pCurPanel->getState(curState);
        while (curState == Device::DeviceState::Busy) {
            usleep(500 * 1000); // microseconds
            pCurPanel->getState(curState);
        }
        if (m_State == Device::DeviceState::Off) { return status; }

        spdlog::info("{} : Motion finished.", m_Identity);

        // update missed steps
        actuator = std::dynamic_pointer_cast<ActController>(pCurPanel->getChildren(PAS_ACTType)[j]);
        actuator->getData(PAS_ACTType_DeltaLength, vtmp);
        vtmp.toFloat(missedDelta);

        spdlog::info("{} : Actuator {} missed target by {} mm.", m_Identity, j + 1, missedDelta);

        vector1 = __getCurrentReadings().first;
        if (m_State == Device::DeviceState::Off) { return status; }

        spdlog::info("{} : PSD readings (after):\n{}\n", m_Identity, vector1);

        spdlog::info("{} : Change in PSD readings:\n{}\n", m_Identity, vector1 - vector0);

        // move the same actuator back
        spdlog::info("{} : Moving actuator {} back to original position...", m_Identity, j + 1);
        spdlog::info("{} : Waiting...", m_Identity);
        minusdeltaL.copyTo(&deltas[j]);
        status = pCurPanel->__moveDeltaLengths(deltas);
        if (!status.isGood()) return status;
        // Stepping is asynchronous. but here, we want it to actually complete
        // before the next step. So we wait.
        pCurPanel->getState(curState);
        while (curState == Device::DeviceState::Busy) {
            usleep(500 * 1000); // microseconds
            pCurPanel->getState(curState);
        }
        if (m_State == Device::DeviceState::Off) { return status; }
        spdlog::info("{} : Motion finished.", m_Identity);

        responseMatrix.col(j) = (vector1 - vector0) / (stepSize - missedDelta);
        spdlog::info("{} : CURRENT RESPONSE MATRIX:\n{}\n", m_Identity, responseMatrix);
    }
    spdlog::info("{} : Done calculating response matrix for Panel {}!", m_Identity, pCurPanel->getIdentity());

    spdlog::info("{} : Response matrix for Optical Table {} --- Panel {}:\n{}\n", m_Identity, m_Identity,
                 pCurPanel->getIdentity(),
                 responseMatrix);

    std::string outfilename = "/home/ctauser/PanelAlignmentData/OptTableResponseMatrix_" + m_Identity.eAddress + ".txt";
    std::ofstream output(outfilename, std::ofstream::in | std::ofstream::out | std::ofstream::app);
    output << pCurPanel->getIdentity().position << std::endl << responseMatrix << std::endl;

    output.close();

    return status;
}

std::pair<Eigen::VectorXd, Eigen::VectorXd> OptTableController::__getCurrentReadings() {
    auto &pPSD = m_pChildren.at(PAS_PSDType);

    Eigen::VectorXd currentReadings = Eigen::VectorXd(4);
    Eigen::VectorXd currentReadingsDelta = Eigen::VectorXd(4);

    Device::DeviceState state;
    Device::ErrorState errorState;

    UaVariant vtmp;
    int temp;

    std::dynamic_pointer_cast<PSDController>(pPSD.at(0))->getState(state);

    std::dynamic_pointer_cast<PSDController>(pPSD.at(0))->getData(PAS_PSDType_ErrorState, vtmp);
    vtmp.toInt32(temp);
    errorState = static_cast<Device::ErrorState>(temp);
    if (state == Device::DeviceState::Off || errorState == Device::ErrorState::FatalError) {
        spdlog::warn(
                "{} : PSD {} is either off, in a fatal error state, or not in the field of view! Will ignore it.",
                m_Identity, pPSD.at(0)->getIdentity());
    }

    spdlog::debug("Reading PSD {}...", pPSD.at(0)->getIdentity());
    if (m_State == Device::DeviceState::Off) {
        spdlog::warn("PSD is off");
    }

    PSDController::PSDData data = std::dynamic_pointer_cast<PSDController>(pPSD.at(0))->getPSDposition();
    currentReadings(0) = (double) data.x1;
    currentReadings(1) = (double) data.y1;
    currentReadings(2) = (double) data.x2;
    currentReadings(3) = (double) data.y2;
    currentReadingsDelta(0) = (double) data.dx1;
    currentReadingsDelta(1) = (double) data.dy1;
    currentReadingsDelta(2) = (double) data.dx2;
    currentReadingsDelta(3) = (double) data.dy2;

    return std::make_pair(currentReadings, currentReadingsDelta);
}
