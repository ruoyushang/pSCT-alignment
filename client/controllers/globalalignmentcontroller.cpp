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

#define kTIME __timeStamp()

GlobalAlignmentController::GlobalAlignmentController(Device::Identity identity, Client *pClient)
        : PasCompositeController(
        std::move(identity), nullptr,
        10000),
        m_terminate{false},
        m_aligned(false),
        m_kPSDDistance{514.61},
        m_kMisalignmentCriterion{0.3},
        m_kAngularScale{StewartPlatform::kMirrorDistance,
                      m_kPSDDistance},
        m_nominalPSDReadings{0., 0., 0., 0.},
        m_secondaryAligned{false},
        m_kInitTime{std::chrono::system_clock::now()},
        m_logstream{"paslogic.log", std::ofstream::app}{
    // define possible children and initialize the selected children string
    m_ChildrenTypes = {PAS_OpticalAlignmentType, PAS_CCDType, PAS_OptTableType, PAS_PSDType, PAS_PanelType, GLOB_PositionerType};

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
    std::vector<unsigned> deviceTypesToCheck = {PAS_CCDType, PAS_OpticalAlignmentType, PAS_PSDType, PAS_PanelType, PAS_OptTableType};
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

    if (m_State == Device::DeviceState::Busy) {
        spdlog::error("{} : GlobalAlignmentController::operate() : Device is busy, method call aborted.", m_Identity);
        return OpcUa_BadInvalidState;
    }

    switch (offset) {
        case PAS_GlobalAlignmentType_StartPSDTrack: {
            spdlog::info("GlobalAlignmentController::operate() :  Calling StartPSDTracking...");

            start();
            m_tracking = OpcUa_True;
            m_terminate = false;
            status = OpcUa_Good;
            break;
        }
        case PAS_GlobalAlignmentType_StopPSDTrack: {
            spdlog::info("GlobalAlignmentController::operate() :  Calling StopPSDTracking...");

            m_tracking = OpcUa_False;
            m_terminate = true;
            spdlog::debug("Set m_terminate to true");
            status = OpcUa_Good;
            break;
        }
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

std::string GlobalAlignmentController::__timeStamp()
{
    using namespace std::chrono;
    char buf[20];

    time_point<system_clock> cur_time = system_clock::now();
    int MSS = duration_cast<milliseconds>(cur_time - m_kInitTime).count();

    int SS = MSS / 1000;
    int MM = SS / 60;
    int HH = MM / 60;
    MSS %= 1000;
    SS %= 60;
    MM %= 60;

    sprintf(buf, "(%02d:%02d:%02d.%03d) ", HH, MM, SS, MSS);
    std::string timestamp(buf);

    return timestamp;
}

bool GlobalAlignmentController::__telescopeMoving()
{
    OpcUa_Boolean isMoving = false;
    UaVariant val;

    spdlog::debug("Getting positioner motion status.");

    try {
        auto pController = m_pChildren[GLOB_PositionerType].at(0);
        UaStatus status = pController->getData(GLOB_PositionerType_isMoving, val);
        UA_ASSERT(status.isGood());
        val.toBool(isMoving);
    }
    catch (std::out_of_range er){
        spdlog::error("Positioner controller not found ({})", er.what());
    }

    return static_cast<bool>(isMoving);
}

float GlobalAlignmentController::__telescopeEl()
{
    float elevation = -1.;
    UaVariant val;

    spdlog::debug("Getting positioner elevation.");

    try {
        auto pController = m_pChildren[GLOB_PositionerType].at(0);
        UaStatus status =  pController->getData(GLOB_PositionerType_curEl,val);
        UA_ASSERT(status.isGood());
        val.toFloat(elevation);
        }
    catch (std::out_of_range er){
        spdlog::error("Positioner controller not found ({})", er.what());
    }

    return elevation;
}

bool GlobalAlignmentController::__PSDmisaligned(unsigned int psdNo)
{
    m_aligned = true;

    double curReadings[4];

    __readPSD(psdNo, curReadings);

    spdlog::debug("PSD readings: ({}, {}) ({}, {})", curReadings[0], curReadings[1], curReadings[2], curReadings[3]);

    double misalignment[2] = {0, 0};
    bool is_misaligned[2];
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            misalignment[i] += (m_nominalPSDReadings[2*i + j] - curReadings[2*i + j])*(m_nominalPSDReadings[2*i + j] - curReadings[2*i + j]);
        }
        misalignment[i] = sqrt(misalignment[i]);
        is_misaligned[i] = (misalignment[i] > m_kMisalignmentCriterion);
        m_aligned &= !is_misaligned[i];
    }

    spdlog::debug("misalignment: ({}, {})", misalignment[0], misalignment[1]);

    return is_misaligned[psdNo];
}

void GlobalAlignmentController::__readPSD(int psdNo, double readings[4])
{
    UaVariant val;
    UaVariantArray args;
    spdlog::debug("__readPSD(): Reading PSD {}",getChildAtPosition(PAS_PSDType, m_psdId[psdNo].position)->getIdentity());
    UaStatus status = getChildAtPosition(PAS_PSDType, m_psdId[psdNo].position)->operate(PAS_PSDType_Read, args);

    if (status.isGood()){
        for (OpcUa_UInt32 i = 0; i < 4; i++) {
            status = getChildAtPosition(PAS_PSDType, m_psdId[psdNo].position)->getData(PAS_PSDType_x1 + i, val);
            UA_ASSERT(status.isGood());
            val.toDouble(readings[i]);
        }
    }
}

void GlobalAlignmentController::__align_PSD(unsigned psdNo)
{
    double curReadings[4];
    __readPSD(psdNo, curReadings);

    /* OT AND PSD AXES ORIENTATIONS */
    // looking from the Primary OT towards the secondary:
    // primary OT x up
    // primary OT y to the right
    // secondary OT x down
    // secondary OT y to the right
    // PSD x to the right
    // PSD y up
    double x_rot = (curReadings[2*psdNo] - m_nominalPSDReadings[2*psdNo])/m_kAngularScale[psdNo];
    double y_rot = (curReadings[2*psdNo + 1] - m_nominalPSDReadings[2*psdNo + 1])/m_kAngularScale[psdNo];
    // adjust the sign for the y rotation of the primary OT:
    y_rot *= (static_cast<double>(psdNo) - 0.5)*2;

    spdlog::debug("rotations to be adjusted by: {} {}", x_rot*m_SP.kRp, y_rot*m_SP.kRp);

    UaStatus status;
    // get current panel coordinates
    UaVariant val;
    double panelCoords[6];
    for (int i = 0; i < 6; i++) {
        spdlog::debug("__align_PSD(): Getting panel coordinates ",m_optId[psdNo]);
        status = getChildAtPosition(PAS_PanelType, m_optId[psdNo].serialNumber)->getData(PAS_PanelType_x + i, val);
        val.toDouble(panelCoords[i]);
    }

    // new ones are just an additional rotation
    panelCoords[PAS_PanelType_xRot - PAS_PanelType_x] += x_rot * m_SP.kRp;
    panelCoords[PAS_PanelType_yRot - PAS_PanelType_x] += y_rot * m_SP.kRp;

    spdlog::debug("New OPT {} coords: ",psdNo + 1);
    std::string buff;
    for (auto coord : panelCoords)
        buff += std::to_string(coord) + " ";
    spdlog::debug(" {} ",  buff);

    status = __move_OT(psdNo, panelCoords);
    __PSDmisaligned(psdNo);
}

UaStatus GlobalAlignmentController::__move_OT(unsigned otNo, const double coords[6])
{
    UaVariantArray args;
    for (int i = 0; i < 6; i++) {
        args[i] = UaVariant(coords[i])[0];
    }

    spdlog::debug("set new panel coords for OPT {}; moving...", otNo+1);
    UaStatus status = getChildAtPosition(PAS_PanelType,m_optId[otNo].serialNumber)->operate(PAS_PanelType_MoveToCoords, args);
    spdlog::debug("done moving");
    if (!status.isGood())
        spdlog::warn("!!! THERE WAS A PROBLEM MOVING THE PANEL !!!");

    return status;
}

void GlobalAlignmentController::run()
{
    m_psdId[0] = getChildAtPosition(PAS_PSDType, SCTMath::Mirror(1001))->getIdentity();
    m_psdId[1] = getChildAtPosition(PAS_PSDType, SCTMath::Mirror(2001))->getIdentity();
    m_optId[0] = getChildAtPosition(PAS_OptTableType, SCTMath::Mirror(1001))->getIdentity(); // primary OPT
    m_optId[1] = getChildAtPosition(PAS_OptTableType, SCTMath::Mirror(2001))->getIdentity(); // secondary OPT
//    m_optId[0].position = 1001; // primary OPT
//    m_optId[1].position = 2001; // secondary OPT
    spdlog::debug("PSD: \n{} \n{}", m_psdId[0], m_psdId[1]);
    spdlog::debug("OptTables: \n{} \n{}", m_optId[0], m_optId[1]);

    spdlog::info("Trying to run PasLogic");
    while (__PSDmisaligned(0) || __PSDmisaligned(1)) {
        for (int ot = 0; ot < 2; ot++) {
            spdlog::debug(" --- PasLogic: Initial Alignment of OT {} ---",ot+1);
            while (__PSDmisaligned(ot)) {
                __align_PSD(ot);
                __PSDmisaligned(ot);
            }
            spdlog::debug("--- PasLogic: DONE Initial Alignment of OT {}", ot+1);
        }
    }

    UaVariant val;
    double panelCoords[6];
    std::string buff;
    for (int i = 0; i < 2; i++) {
        spdlog::debug("--- PasLogic: Initial OPT {} ", i+1);
        spdlog::debug("run(): Getting panel coordinates {} of OptTable {}",getChildAtPosition(PAS_PanelType, m_optId[i].serialNumber)->getIdentity(), m_optId[i]);
        for (int j = 0; j < 6; j++) {
            getChildAtPosition(PAS_PanelType, m_optId[i].serialNumber)->getData(PAS_PanelType_x + j, val);
            val.toDouble(panelCoords[j]);
            buff += std::to_string(panelCoords[j]) + " ";
        }
        spdlog::debug("{} ", buff);
        buff.clear();
    }

    bool postalign;

    float cur_elevation;
    while (!m_terminate) {
        postalign = false;
        while (__telescopeMoving() && !m_terminate ) {
            postalign = true;
            cur_elevation = __telescopeEl();
            spdlog::debug("elevation: {}", cur_elevation);
            for (int ot = 0; ot < 2; ot++) {
                if (__PSDmisaligned(ot)) {
                    spdlog::debug("PSD {} misaligned! Aligning OT", ot + 1);
                    __align_PSD(ot);
                    spdlog::debug("DONE");
                }
            }
            __PSDmisaligned(0);
        }

        // align after the telescope stopped moving
        if (postalign) {
            spdlog::debug("Postalignment:");
            while (!m_aligned && !m_terminate ) {
                for (int ot = 0; ot < 2; ot++) {
                    if (__PSDmisaligned(ot)) {
                        spdlog::debug("PSD {} misaligned! Aligning OT", ot + 1);
                        __align_PSD(ot);
                        spdlog::debug("DONE");
                    }
                }
            }
            spdlog::debug("DONE Postalignment");
            __PSDmisaligned(0);
        }

        UaThread::sleep(1);
    }


    spdlog::info("PasLogic: Received terminate signal. Shutting down...");
}