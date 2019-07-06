#include "client/controllers/mirrorcontroller.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <deque>
#include <sstream>
#include <string>
#include <common/alignment/mpes.hpp>

#include "AGeoAsphericDisk.h" // ROBAST dependency
#include "TMinuit.h" // ROOT's implementation of MINUIT for chiSq minimization

#include "common/alignment/device.hpp"
#include "common/simulatestewart/mathtools.hpp"
#include "common/simulatestewart/mirrordefinitions.hpp" // definitions of the mirror surfaces

#include "client/controllers/edgecontroller.hpp"
#include "client/controllers/mpescontroller.hpp"
#include "client/controllers/panelcontroller.hpp"
#include "client/controllers/pascontroller.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

MirrorController *MirrorControllerCompute::m_Mirror = nullptr;

MirrorController::MirrorController(Device::Identity identity, Client *pClient, std::string mode)
    : PasCompositeController(
    std::move(identity), pClient,
    10000),
      m_Mode(mode), m_pSurface(nullptr) {
    // define possible children and initialize the selected children string
    m_ChildrenTypes = {PAS_PanelType, PAS_EdgeType, PAS_MPESType};

    // define coordinate vectors -- these are of size 6
    m_curCoords = Eigen::VectorXd(6);
    m_curCoordsErr = Eigen::VectorXd(6);
    m_sysOffsetsMPES = Eigen::VectorXd(6);
    Eigen::VectorXd tmp = Eigen::VectorXd(2);
    tmp.setZero();
    // initialize the systematic offsets map to zeros
    SystematicOffsetsMPESMap = {
        {1, {
            {1, tmp}, {2, tmp}, {3, tmp}, {4, tmp}, {5, tmp}
            }},
        {2, {
            {1, tmp}, {2, tmp}, {3, tmp}, {4, tmp}
            }}
    };

    m_Xcalculated = Eigen::VectorXd(0);                                                        
    m_previousCalculatedMethod = 0;

    // make sure things update on the first boot up
    // duration takes seconds -- hence the conversion with the 1/1000 ratio
    m_LastUpdateTime = TIME::now() - std::chrono::duration<int, std::ratio<1, 1000>>(m_kUpdateInterval_ms);
}

void MirrorController::addChild(OpcUa_UInt32 deviceType, const std::shared_ptr<PasController>& pController)
{
    // call the base type's method
    PasCompositeController::addChild(deviceType, pController);

    if (deviceType == PAS_PanelType) {
        m_selectedPanels.insert((unsigned) pController->getIdentity().position);
    }
    else if (deviceType == PAS_EdgeType) {
        m_selectedEdges.insert(pController->getIdentity().eAddress);
    }
    else if (deviceType == PAS_MPESType) {
        m_selectedMPES.insert(pController->getIdentity().serialNumber);
    }
}


bool MirrorController::initialize()
{
    spdlog::info("{} : MirrorController::initialize(): Initializing...", m_Identity);

    // precompute everything we need
    if (m_Identity.position == 1) {
        // bottom (w.r.t. z) surface first, then top surface --
        // back surface first, then optical surface
        m_pSurface = std::make_shared<AGeoAsphericDisk>(m_Identity.name.c_str(),
                                          SCT::Primary::kZ + SCT::Primary::kz[0] - SCT::Primary::kMirrorThickness, 0,
                                          SCT::Primary::kZ + SCT::Primary::kz[0], 0,
                                          SCT::Primary::kD / 2., 0);
        m_pSurface->SetPolynomials(SCT::kNPar - 1, &SCT::Primary::kz[1],
                                   SCT::kNPar - 1, &SCT::Primary::kz[1]);
        m_SurfaceNorm = 1.; // along z

        // update ideal panel properties
        for (int ring : {0, 1}) {
            // ideal panel width and offset of x1x1 from it
            m_PanelWidth[ring + 1] = 2. * M_PI / SCT::Primary::kPanels[ring];

            for (int coord = 0; coord < 3; coord++)
                for (int pad = 0; pad < 3; pad++)
                    m_PadCoordsTelFrame[ring + 1].col(pad)(coord) = SCT::Primary::PadsCoords[ring][pad][coord];

            m_PanelOriginTelFrame[ring + 1].setZero();
            // this is the center of the mirror panel in the TRF
            for (int pad = 0; pad < 3; pad++)
                m_PanelOriginTelFrame.at(ring + 1) += m_PadCoordsTelFrame.at(ring + 1).col(pad) / 3.;
        }

    } else if (m_Identity.position == 2) {
        // bottom (w.r.t. z) surface first, then top surface --
        // optical surface first, then back surface
        m_pSurface = std::make_shared<AGeoAsphericDisk>(m_Identity.name.c_str(),
                                          SCT::Secondary::kZ + SCT::Secondary::kz[0] - SCT::Secondary::kMirrorThickness,
                                                        0,
                                          SCT::Secondary::kZ + SCT::Secondary::kz[0], 0,
                                          SCT::Secondary::kD / 2., 0);
        m_pSurface->SetPolynomials(SCT::kNPar - 1, &SCT::Secondary::kz[1],
                                   SCT::kNPar - 1, &SCT::Secondary::kz[1]);
        m_SurfaceNorm = 1.; // parallel to z in its frame

        // update ideal panel properties
        for (int ring : {0, 1}) {
            // ideal panel width and offset of x1x1 from it
            m_PanelWidth[ring + 1] = 2. * M_PI / SCT::Secondary::kPanels[ring];

            // update pad coords and panel origin
            for (int coord = 0; coord < 3; coord++)
                for (int pad = 0; pad < 3; pad++)
                    m_PadCoordsTelFrame[ring + 1].col(pad)(coord) = SCT::Secondary::PadsCoords[ring][pad][coord];

            m_PanelOriginTelFrame[ring + 1].setZero();
            // this is the center of the mirror panel in the TRF
            for (int pad = 0; pad < 3; pad++)
                m_PanelOriginTelFrame.at(ring + 1) += m_PadCoordsTelFrame.at(ring + 1).col(pad) / 3.;
        }
    } else if (m_Identity.position ==
               3) { // this is used for the test setup in the lab (2 P2 panels) - duplicates the primary mirror geometry
        // bottom (w.r.t. z) surface first, then top surface --
        // back surface first, then optical surface
        m_pSurface = std::make_shared<AGeoAsphericDisk>(m_Identity.name.c_str(),
                                          SCT::Primary::kZ + SCT::Primary::kz[0] - SCT::Primary::kMirrorThickness, 0,
                                          SCT::Primary::kZ + SCT::Primary::kz[0], 0,
                                          SCT::Primary::kD / 2., 0);
        m_pSurface->SetPolynomials(SCT::kNPar - 1, &SCT::Primary::kz[1],
                                   SCT::kNPar - 1, &SCT::Primary::kz[1]);
        m_SurfaceNorm = 1.; // along z

        // update ideal panel properties
        for (int ring : {0, 1}) {
            // ideal panel width and offset of x1x1 from it
            m_PanelWidth[ring + 1] = 2. * M_PI / SCT::Primary::kPanels[ring];

            for (int coord = 0; coord < 3; coord++)
                for (int pad = 0; pad < 3; pad++)
                    m_PadCoordsTelFrame[ring + 1].col(pad)(coord) = SCT::Primary::PadsCoords[ring][pad][coord];

            m_PanelOriginTelFrame[ring + 1].setZero();
            // this is the center of the mirror panel in the TRF
            for (int pad = 0; pad < 3; pad++)
                m_PanelOriginTelFrame.at(ring + 1) += m_PadCoordsTelFrame.at(ring + 1).col(pad) / 3.;
        }

    }

    else {
        spdlog::error("{} : Invalid mirror position {}. Could not initialize...", m_Identity, m_Identity.position);
        return false;
    }

    m_pStewartPlatform = std::make_shared<StewartPlatform>();

    // precompute ideal offsets and norms
    if (m_pSurface) {
        double dir[3] = {0., 0., m_SurfaceNorm}; // norm should point along this direction
        double norm[3];
        for (int ring : {0, 1} ) {
            // set panel offset -- note the sign here
            m_PanelOffset[ring + 1] = M_PI - m_PanelWidth.at(ring + 1) / 2.;

            // get the norm
            m_pSurface->ComputeNormal(m_PanelOriginTelFrame.at(ring + 1).data(), dir, norm);
            // save the norm in the third column
            m_PanelFrame[ring + 1].col(2) = Eigen::Vector3d(norm);
            // make sure this is pointing where we need it to
            if (m_PanelFrame.at(ring + 1).col(2).dot(Eigen::Vector3d(dir)) < 0.)
                m_PanelFrame.at(ring + 1).col(2) *= -1;

            // get the x and y axes:
            for (int i = 0; i < 2; i++) {
                Eigen::Vector3d axis{0., 0., 0};
                axis(i) = 1.0;
                // subtract the projection onto the norm
                axis -= axis.dot(m_PanelFrame.at(ring + 1).col(2))*m_PanelFrame.at(ring+1).col(2);
                axis.normalize();
                m_PanelFrame.at(ring + 1).col(i) = axis;
            }

            // the true panel origin is at the base triangle, not at the mirror surface --
            // need to shift the one computed above to the base triangle
            // compute the center of the mirror panel in the panel frame:
            m_pStewartPlatform->SetPanelType(StewartPlatform::PanelType::OPT);
            double actL[6];
            for (auto &len : actL)
                len = SCT::kActuatorLength;
            m_pStewartPlatform->ComputeStewart(actL);
            Eigen::Vector3d PanelCenterPanelFrame(m_pStewartPlatform->GetPanelCoords());
            // this is the center of the mirror panel in the panel frame;
            // what we obtained before was the center of the mirror panel in the TRF --
            // need to shift those coordinates by this much opposite to the mirror norm
            // to get the origin of the panel frame in the TRF:
            m_PanelOriginTelFrame.at(ring + 1) -= m_PanelFrame.at(ring+1)*PanelCenterPanelFrame;
        }
    }

    spdlog::info("{}: MirrorController::initialize(): Done.", m_Identity);

    return true;
}

UaStatus MirrorController::getState(Device::DeviceState &state)
{
    //UaMutexLocker lock(&m_mutex);
    Device::DeviceState s;
    std::vector<unsigned> deviceTypesToCheck = {PAS_PanelType, PAS_MPESType};
    for (auto devType : deviceTypesToCheck) {
        for (const auto &child : getChildren(devType)) {
            child->getState(s);
            if (s == Device::DeviceState::Busy) {
                m_State = s;
            }
        }
    }
    state = m_State;
    spdlog::trace("{} : Read device state => ({})", m_Identity, Device::deviceStateNames.at(state));
    return OpcUa_Good;
}

UaStatus MirrorController::setState(Device::DeviceState state)
{
    //UaMutexLocker lock(&m_mutex);
    m_State = state;
    spdlog::trace("{} : Setting device state => ({})", m_Identity, Device::deviceStateNames.at(state));
    return OpcUa_Good;
}

UaStatus MirrorController::getData(OpcUa_UInt32 offset, UaVariant &value)
{
    //UaMutexLocker lock(&m_mutex);

    if (offset >= PAS_MirrorType_x && offset <= PAS_MirrorType_zRot) {
        // update current coordinates
        if (__expired()) {
            updateCoords(false);
        }
        int dataoffset = offset - PAS_MirrorType_x;
        value.setDouble(m_curCoords(dataoffset));
    }
    else if (offset >= PAS_MirrorType_sysOffsetsMPES_x1 && offset <= PAS_MirrorType_sysOffsetsMPES_y3) {
        int dataoffset = offset - PAS_MirrorType_sysOffsetsMPES_x1;
        value.setDouble(m_sysOffsetsMPES(dataoffset));
    } else if (offset == PAS_MirrorType_SafetyRadius)
        value.setDouble(m_safetyRadius);
    else if (offset == PAS_MirrorType_Position)
        value.setInt32(m_Identity.position);
    else if (offset == PAS_MirrorType_SelectedEdges) {
        std::vector<std::string> v(m_selectedEdges.begin(), m_selectedEdges.end());
        std::string s = "[";
        for (int i = 0; i < (int)(v.size()); i++) {
            s += v[i];
            if (i != (int)(v.size() - 1)) {
                s += ", ";
            }
        }
        s += "]";
        value.setString(s.c_str());
    } else if (offset == PAS_MirrorType_SelectedPanels) {
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
    } else if (offset == PAS_MirrorType_SelectedMPES) {
        std::vector<int> v(m_selectedMPES.begin(), m_selectedMPES.end());
        std::string s = "[";
        for (int i = 0; i < (int)(v.size()); i++) {
            s += std::to_string(v[i]);
            if (i != (int)(v.size() - 1)) {
                s += ", ";
            }
        }
        s += "]";
        value.setString(s.c_str());
    } else
        return OpcUa_BadInvalidArgument;

    return OpcUa_Good;
}

UaStatus MirrorController::setData(OpcUa_UInt32 offset, UaVariant value)
{
    //UaMutexLocker lock(&m_mutex);
    if (offset >= PAS_MirrorType_sysOffsetsMPES_x1 && offset <= PAS_MirrorType_sysOffsetsMPES_y3) {
        int dataoffset = offset - PAS_MirrorType_sysOffsetsMPES_x1;
        value.toDouble(m_sysOffsetsMPES(dataoffset));
    } else if (offset == PAS_MirrorType_SafetyRadius) {
        value.toDouble(m_safetyRadius);
        spdlog::info("{} : Setting safety radius (for all child panels) to {} px.", m_Identity, m_safetyRadius);
        // Set for all child panels
        std::shared_ptr<PanelController> panel;
        for (auto &p : getChildren(PAS_PanelType)) {
            panel = std::dynamic_pointer_cast<PanelController>(p);
            panel->setData(PAS_PanelType_SafetyRadius, value);
        }
    } else if (offset == PAS_MirrorType_SelectedEdges) {
        std::string selectionString = value.toString().toUtf8();
        parseAndSetSelection(selectionString, PAS_EdgeType);
    } else if (offset == PAS_MirrorType_SelectedPanels) {
        std::string selectionString = value.toString().toUtf8();
        parseAndSetSelection(selectionString, PAS_PanelType);
    } else if (offset == PAS_MirrorType_SelectedMPES) {
        std::string selectionString = value.toString().toUtf8();
        parseAndSetSelection(selectionString, PAS_MPESType);
    } else
        return OpcUa_BadInvalidArgument;


    return OpcUa_Good;
}

UaStatus MirrorController::operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    UaStatus status;
    //UaMutexLocker lock(&m_mutex);

    if (m_State == Device::DeviceState::Busy && offset != PAS_MirrorType_Stop) {
        spdlog::error("{} : MirrorController::operate() : Device is busy, method call aborted.", m_Identity);
        return OpcUa_BadInvalidState;
    }

    /**********************************************************
     * Move the whole mirror in the telescope reference frame *
     * ********************************************************/
    if (offset == PAS_MirrorType_MoveToCoords) {
        spdlog::info("{} : MirrorController::operate() : Calling moveToCoords()...", m_Identity);
        setState(Device::DeviceState::Busy);
        updateCoords(false);    
        Eigen::VectorXd targetMirrorCoords(6);
        for (int i = 0; i < 6; i++) {
            UaVariant(args[i]).toDouble(targetMirrorCoords[i]);
        }

        bool execute = args[6].Value.Boolean;

        status = moveToCoords(targetMirrorCoords, execute);
        updateCoords(false);
        setState(Device::DeviceState::On);
    }
    /**********************************************************
     * Read out all current panel positions in their frames   *
     * ********************************************************/
    else if (offset == PAS_MirrorType_ReadPosition) {
        setState(Device::DeviceState::Busy);
        // read out all individual positions
        // and get global mirror coordinates
        spdlog::info("{} : MirrorController::operate() : Calling readPosition()...", m_Identity);
        updateCoords();
        setState(Device::DeviceState::On);
    }

    /**********************************************************
     * Align the selected sector to selected sensors          *
     * ********************************************************/
    else if (offset == PAS_MirrorType_AlignSector) {
        double alignFrac = args[0].Value.Double;
        bool execute = args[1].Value.Boolean;
        spdlog::info("{} : MirrorController::operate() : Calling alignSector() with align fraction {}, execute {}...",
                     m_Identity, alignFrac, execute);
        setState(Device::DeviceState::Busy);
        status = alignSector(alignFrac, execute);
        updateCoords(false);
        setState(Device::DeviceState::On);
    }

    /**********************************************************************************************
     * Perform global alignment and compute the systematics vector keeping the input panel fixed  *
     * ********************************************************************************************/
    else if (offset == PAS_MirrorType_AlignRing) {
        // get the position of the panel to keep fixed
        unsigned fixPanel = args[0].Value.UInt32;
        double alignFrac = args[1].Value.Double;
        bool execute = args[2].Value.Boolean;

        spdlog::info(
            "{} : MirrorController::operate() : Calling alignRing() with fixed panel {}, align fraction {}, execute {}...",
            m_Identity, fixPanel, alignFrac, execute);
        setState(Device::DeviceState::Busy);

        status = alignRing(fixPanel, alignFrac, execute);
        updateCoords(false);
        setState(Device::DeviceState::On);
    }

    /**********************************************************
     * Align all edges of the mirror                          *
     * ********************************************************/
    else if (offset == PAS_MirrorType_AlignSequential) {
        // make sure the arguments make sense -- we are supposed
        // to get an edge position and a direction. The type has
        // already been validated by the caller, but we need to
        // check the existence of such edge and direction:
        // first argument is the starting edge address
        std::string startEdge = UaString(args[0].Value.String).toUtf8();
        std::string endEdge = UaString(args[1].Value.String).toUtf8();
        OpcUa_UInt32 dir = args[2].Value.UInt32;

        spdlog::info(
            "{} : MirrorController::operate() : Calling alignSequential() with start edge {}, end edge {}, direction {}...",
            m_Identity, startEdge, endEdge, dir);
        setState(Device::DeviceState::Busy);


        status = alignSequential(startEdge, endEdge, dir);
        updateCoords(false);
        setState(Device::DeviceState::On);
    }

    /**********************************************************
     * Read out all current edge sensors                      *
     * ********************************************************/
    else if (offset == PAS_MirrorType_ReadSensors) {
        spdlog::info("{} : MirrorController::operate() : Calling readSensors()...", m_Identity);
        setState(Device::DeviceState::Busy);
        if (m_selectedEdges.empty()) {
            spdlog::error("{} : MirrorController::operate() : No Edges selected. Nothing to do, method call aborted.",
                          m_Identity);
        }
        else {
            for (const auto &eAddress : m_selectedEdges) {
                if (m_State == Device::DeviceState::Busy) {
                    status = std::dynamic_pointer_cast<EdgeController>(m_pChildren.at(PAS_EdgeType).at(getEdgeIndex(eAddress)))->operate(
                        PAS_EdgeType_Read);
                    if (!status.isGood()) { return status; }
                }
                else {
                    spdlog::warn("{} : MirrorController::readSensors() : Mirror stop detected. Readings stopped.",
                                 m_Identity);
                }
            }
        }
        setState(Device::DeviceState::On);
    } else if (offset == PAS_MirrorType_ReadSensorsParallel) {
        spdlog::info("{} : MirrorController::operate() : Calling readSensorsParallel()...", m_Identity);
        if (m_Mode == "client") {
            status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("ReadSensorsParallel"));

            Device::DeviceState state;
            getState(state);
            while (state == Device::DeviceState::Busy) {
                sleep(1);
                getState(state);
            }

            std::map<Device::Identity, MPESBase::Position> readings;
            for (auto it = getChildren(PAS_PanelType).begin(); it < getChildren(PAS_PanelType).end(); it++) {
                for (const auto &mpes : std::dynamic_pointer_cast<PanelController>(*it)->getChildren(
                    PAS_MPESType)) {
                    if (m_selectedMPES.find(mpes->getIdentity().serialNumber) != m_selectedMPES.end()) {
                        std::dynamic_pointer_cast<MPESController>(mpes)->read();
                        readings.insert(
                            std::make_pair(mpes->getIdentity(), std::dynamic_pointer_cast<MPESController>(
                                mpes)->getPosition()));
                    }
                }
            }

            for (const auto &edge : getChildren(PAS_EdgeType)) {
                std::ostringstream os;
                for (const auto &mpes : std::dynamic_pointer_cast<PasCompositeController>(edge)->getChildren(
                    PAS_MPESType)) {
                    os << mpes->getIdentity() << " : " << std::endl;
                    auto it = readings.find(mpes->getIdentity());
                    if (it != readings.end()) {
                        os << "    " << it->second.xCentroid << " +/- " << it->second.xSpotWidth << " ["
                           << it->second.xNominal << "] ("
                           << it->second.xCentroid - it->second.xNominal << ")" << std::endl;
                        os << "    " << it->second.yCentroid << " +/- " << it->second.ySpotWidth << " ["
                           << it->second.yNominal << "] ("
                           << it->second.yCentroid - it->second.yNominal << ")" << std::endl;
                    }
                }
                spdlog::info(
                    "{}: Readings for Edge {}:\nCurrent position +/- Spot width [Aligned position] (Misalignment)\n\n{}\n\n",
                    m_Identity, edge->getIdentity(), os.str());
            }
        } else if (m_Mode == "subclient") {
            setState(Device::DeviceState::Busy);
            bool stopped = false;
            if (m_selectedMPES.empty()) {
                spdlog::error(
                    "{} : MirrorController::operate() : No MPES selected. Nothing to do, method call aborted.",
                    m_Identity);
            } else {
#pragma omp parallel for
                for (auto it = getChildren(PAS_PanelType).begin(); it < getChildren(PAS_PanelType).end(); it++) {
                    for (const auto &mpes : std::dynamic_pointer_cast<PanelController>(*it)->getChildren(
                        PAS_MPESType)) {
                        if (m_selectedMPES.find(mpes->getIdentity().serialNumber) != m_selectedMPES.end()) {
                            if (m_State == Device::DeviceState::Busy) {
                                std::dynamic_pointer_cast<MPESController>(mpes)->read();
                            } else {
                                stopped = true;
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            spdlog::warn("{} : MirrorController::readSensorsParallel() : Mirror stop detected. Readings stopped.",
                         m_Identity);
        }
        setState(Device::DeviceState::On);
    } else if (offset == PAS_MirrorType_SelectAll) {
        spdlog::info("{} : MirrorController::operate() : Calling selectAll()...", m_Identity);
        m_selectedPanels.clear();
        for (const auto &panel : getChildren(PAS_PanelType)) {
            m_selectedPanels.insert((unsigned) panel->getIdentity().position);
        }

        m_selectedEdges.clear();
        for (const auto &edge : getChildren(PAS_EdgeType)) {
            m_selectedEdges.insert(edge->getIdentity().eAddress);
        }

        m_selectedMPES.clear();
        for (const auto &mpes : getChildren(PAS_MPESType)) {
            m_selectedMPES.insert(mpes->getIdentity().serialNumber);
        }
    }

        /************************************************
         * stop the motion in progress                  *
         * **********************************************/
    else if (offset == PAS_MirrorType_Stop) {
        spdlog::warn(
            "{} : MirrorController::operate() : Calling stop(). Stopping motion of all edges and child panels...",
            m_Identity);
        for (const auto &pEdge : m_pChildren.at(PAS_EdgeType)) {
            std::dynamic_pointer_cast<EdgeController>(pEdge)->operate(PAS_EdgeType_Stop);
        }
        setState(Device::DeviceState::Off); // Turn state off to stop all methods
    } else {
        return OpcUa_BadNotImplemented;
    }

    return status;
}


/* ================= INTERNAL IMPLEMENTATIONS OF ALL METHODS ================== */

UaStatus MirrorController::readPositionAll(bool print) {
    UaStatus status;
    
    for (unsigned panelPos : m_selectedPanels) {
        std::dynamic_pointer_cast<PanelController>(m_pChildren.at(PAS_PanelType).at(getPanelIndex(panelPos)))->updateCoords(print);

        auto padCoordsActs = std::dynamic_pointer_cast<PanelController>(
            m_pChildren.at(PAS_PanelType).at(getPanelIndex(panelPos)))->getPadCoords();
        if (print) {
            spdlog::info("{}: MirrorController::readPositionAll(): Panel frame pad coordinates:\n{}\n", m_Identity,
                         padCoordsActs);
            // and transform this to the telescope reference frame:
            // these are pad coordinates in TRF as computed from actuator lengths
            for (int i = 0; i < padCoordsActs.cols(); i++)
                padCoordsActs.col(i) = __toTelRF(panelPos, padCoordsActs.col(i));
            spdlog::info("{}: MirrorController::readPositionAll(): Telescope frame pad coordinates:\n{}\n", m_Identity,
                         padCoordsActs);
        }
    }

    return status;
}

UaStatus MirrorController::moveToCoords(const Eigen::VectorXd &targetMirrorCoords, bool execute) {
    UaStatus status;
    
    if (!execute) {
        spdlog::info(
            "{} : MirrorController::moveToCoords(): Called with execute=False. Pre-calculating alignment motion...",
            m_Identity);
        Eigen::VectorXd deltaMirrorCoords(6);

        deltaMirrorCoords = targetMirrorCoords - m_curCoords;

        spdlog::info("{} : Moving to target coordinates:\n{}\n", m_Identity, targetMirrorCoords);
        spdlog::info("{} : Delta coordinates:\n{}\n", m_Identity, deltaMirrorCoords);

        std::vector<std::shared_ptr<PanelController>> panelsToMove;
        Eigen::VectorXd X(m_pChildren.at(PAS_PanelType).size()*6);

        unsigned positionNum;
        Eigen::VectorXd prf_coords;
        Eigen::VectorXd deltaActLengths(6);
        Eigen::VectorXd targetActLengths(6);
        Eigen::VectorXd currentActLengths(6);
        
        int j = 0;
        for (const auto &pPanel : m_pChildren.at(PAS_PanelType)) {
            positionNum = pPanel->getIdentity().position;
            std::dynamic_pointer_cast<PanelController>(pPanel)->operate(PAS_PanelType_ReadPosition);
            // for this panel, we get PRF pad coords, transform them to TRF,
            // move them in TRF, transform back to PRF, and then compute new ACT lengths
            // based on the new pad coords. so simple!
            auto padCoords_PanelRF = std::dynamic_pointer_cast<PanelController>(pPanel)->getPadCoords();
            auto padCoords_TelRF = padCoords_PanelRF;
            double newPadCoords[3][3];
            for (unsigned pad = 0; pad < 3; pad++) {
                // transform to TRF
                padCoords_TelRF.col(pad) = __toTelRF(positionNum, padCoords_PanelRF.col(pad));
                // move by the desired telescope coordinates
                padCoords_TelRF.col(pad) = __moveInCurrentRF(padCoords_TelRF.col(pad), deltaMirrorCoords);
                // transform back to PRF
                padCoords_PanelRF.col(pad) = __toPanelRF(positionNum, padCoords_TelRF.col(pad));
                for (unsigned coord = 0; coord < 3; coord++)
                    newPadCoords[pad][coord] = padCoords_PanelRF.col(pad)(coord);
            }
            m_pStewartPlatform->ComputeActsFromPads(newPadCoords);
            for (int i = 0; i < 6; i++) {
                targetActLengths(i) = (float) m_pStewartPlatform->GetActLengths()[i];
            }
            currentActLengths = std::dynamic_pointer_cast<PanelController>(pPanel)->getActuatorLengths();

            deltaActLengths = targetActLengths - currentActLengths;
            spdlog::info("{} : Panel {} calculated actuator motion:\n{}\n", m_Identity, pPanel->getIdentity(),
                         deltaActLengths);

            /**
            if (std::dynamic_pointer_cast<PanelController>(pPanel)->checkForCollision(deltaActLengths)) {
                return OpcUa_Bad;
            }
             */

            panelsToMove.push_back(std::dynamic_pointer_cast<PanelController>(pPanel));
            X.segment(j,6) = deltaActLengths;
            j += 6;
        }
        m_Xcalculated = X;
        m_panelsToMove = panelsToMove;
        m_previousCalculatedMethod = PAS_MirrorType_MoveToCoords;
    }
    else {
        if (m_Xcalculated.isZero(0) || m_previousCalculatedMethod != PAS_MirrorType_MoveToCoords) {
            spdlog::error(
                "{} : No calculated motion found. Call Mirror.moveToCoords with execute=false once first to calculate the motion to execute.",
                m_Identity);
            return OpcUa_Bad;
        }
        else {        
            unsigned j = 0;
            for (auto &pCurPanel : m_panelsToMove) {
                auto nACT = pCurPanel->getActuatorCount();
                UaVariantArray deltas;
                deltas.create(nACT);
                UaVariant val;
                for (unsigned i = 0; i < nACT; i++) {
                    val.setFloat(m_Xcalculated(j++));
                    val.copyTo(&deltas[i]);
                }
                status = pCurPanel->__moveDeltaLengths(deltas);
                if (!status.isGood()) { return status; }
                if (m_State == Device::DeviceState::Off) { break; }
            }
            m_Xcalculated.setZero(); // reset calculated motion
            m_panelsToMove.clear();
            m_previousCalculatedMethod = 0;
        }
    }
    return status;
}

// Align all edges between start_idx and end_idx moving in the direction dir
UaStatus MirrorController::alignSequential(const std::string &startEdge, const std::string &endEdge, unsigned dir)
{
    // we need to traverse the mirror in the correct order of edges.
    // dir = 0 decreases panel position (+z rotation);
    // dir = 1 increases panel position (-z rotation);
    // The individual edges and panels don't know anything about the mirror topology,
    // only the mirror has this information. So we need to reconstruct the topology
    // here.
    // We represent the topology as a simple cyclic group of N=(number of panels in the ring),
    // represented in base B=(number of panels in the quadrant)

    // The alignment procedure is as follows:
    // For the current edge, take the panel that's greater dir-wise and align all all the
    // preceding panels to it (so going in the direction opposite to dir);
    // increment the current edge in the direction dir.

    UaStatus status;

    if (m_ChildrenEaddressMap.at(PAS_EdgeType).find(startEdge) == m_ChildrenEaddressMap.at(PAS_EdgeType).end()) {
        spdlog::error("{}: Could not find start edge {} in mirror. Method call aborted.", m_Identity, startEdge);
        return OpcUa_Bad;
    }

    if (m_ChildrenEaddressMap.at(PAS_EdgeType).find(endEdge) == m_ChildrenEaddressMap.at(PAS_EdgeType).end()) {
        spdlog::error("{}: Could not find end edge {} in mirror. Method call aborted.", m_Identity, endEdge);
        return OpcUa_Bad;
    }

    std::string curEdge = startEdge;
    std::vector<std::string> selectedEdges = {curEdge};
    // First collect and calculate all edges (in order) between start edge and end edge in direction
    while (curEdge != endEdge) {
        std::string nextEdge = SCTMath::GetEdgeNeighbor(curEdge, dir);
        if (std::find(selectedEdges.begin(), selectedEdges.end(), nextEdge) == selectedEdges.end()) {
            if (m_ChildrenEaddressMap.at(PAS_EdgeType).find(nextEdge) != m_ChildrenEaddressMap.at(PAS_EdgeType).end()) {
                selectedEdges.push_back(nextEdge);
                curEdge = nextEdge;
            } else {
                spdlog::error(
                    "{} : MirrorController::alignSequential(): Searched for next edge ({}), but did not find as child of mirror. Please double-check that all edges between start and end are present in the client.",
                    m_Identity, nextEdge);
                return OpcUa_Bad;
            }
        }
        else {
            spdlog::error(
                "{} : MirrorController::alignSequential(): Went around full cycle from start edge ({}) in requested direction ({}) without reaching end edge ({}). Double-check that you chose valid edges (in the same ring).",
                m_Identity, startEdge, dir, endEdge);
            return OpcUa_Bad;
        }    
    }

    std::ostringstream os;
    for (const auto &edge : selectedEdges) {
        os << edge << std::endl;
    }
    spdlog::info("{}: Edges to align sequentially:\n{}", m_Identity, os.str());

    std::deque<std::string> toAlign{}; // yes, deque, not vector!
    for (int i = 0; i < (int) selectedEdges.size() && m_State != Device::DeviceState::Off; i++) {
        toAlign.push_front(selectedEdges.at(i));
        spdlog::info("{}: Aligning Edge {} and all previous edges...", m_Identity, toAlign.front());
        // align all the preceding panels
        for (const auto &edgeEaddress : toAlign) {
            // figure out which panel is "greater" and which one is "smaller" in the sense
            // of dir, assuming a two-panel edge for now.
            // get vector of panel positions:
            auto curPanels = SCTMath::GetPanelsFromEdge(edgeEaddress, dir);

            // align this edge moving the "smaller" panel
            UaVariantArray args; 
            args.create(4);
            args[0].Value.UInt32 = curPanels.at(0); // "smaller" panel
            args[1].Value.UInt32 = curPanels.at(1); // larger panel
            args[2].Value.Double = 1.0;
            args[3].Value.Boolean = false; // first, calculate
            auto movingPanel_idx = m_ChildrenPositionMap.at(PAS_PanelType).at(curPanels.at(0));
            // do this until the edge is aligned
            int alignIter = 1;
            spdlog::debug("{} Calculating motion for Edge {}...", m_Identity, edgeEaddress);
            status = m_pChildren.at(PAS_EdgeType).at(getEdgeIndex(edgeEaddress))->operate(PAS_EdgeType_Align, args);
            Device::DeviceState curState;
            m_pChildren.at(PAS_EdgeType).at(getEdgeIndex(edgeEaddress))->getState(curState);
            while (curState == Device::DeviceState::Busy) {
                usleep(500 * 1000); // microseconds
                m_pChildren.at(PAS_EdgeType).at(getEdgeIndex(edgeEaddress))->getState(curState);
            }
            if (m_State == Device::DeviceState::Off) { break; }
            args[3].Value.Boolean = true; // this time, execute
            spdlog::debug("{}: Executing alignment motion for Edge {}...", m_Identity, edgeEaddress);
            status = m_pChildren.at(PAS_EdgeType).at(getEdgeIndex(edgeEaddress))->operate(PAS_EdgeType_Align, args);
            while (curState == Device::DeviceState::Busy) {
                usleep(500 * 1000); // microseconds
                m_pChildren.at(PAS_EdgeType).at(getEdgeIndex(edgeEaddress))->getState(curState);
            }
            if (!status.isGood()) {
                spdlog::error("{}: Failed while executing motion. Method aborted.", m_Identity);
                return status; 
            }
            if (m_State == Device::DeviceState::Off) { break; }
            while (!std::dynamic_pointer_cast<EdgeController>(
                m_pChildren.at(PAS_EdgeType).at(getEdgeIndex(edgeEaddress)))->isAligned()) {
                spdlog::info("{}: Alignment Iteration {}.", m_Identity, alignIter);
                usleep(400*1000); // microseconds

                Device::DeviceState curstate;
                m_pChildren.at(PAS_PanelType).at(movingPanel_idx)->getState(curstate);
                while (curstate == Device::DeviceState::Busy) {
                    spdlog::debug("{}: Waiting for Panel {}", m_Identity, curPanels.at(0));
                    usleep(200*1000); // microseconds
                    m_pChildren.at(PAS_PanelType).at(movingPanel_idx)->getState(curstate);
                }
                alignIter++;
                args[3].Value.Boolean = false;
                spdlog::debug("{} Calculating motion for Edge {}...", m_Identity, edgeEaddress);
                status = m_pChildren.at(PAS_EdgeType).at(getEdgeIndex(edgeEaddress))->operate(PAS_EdgeType_Align, args);
                while (curState == Device::DeviceState::Busy) {
                    usleep(500 * 1000); // microseconds
                    m_pChildren.at(PAS_EdgeType).at(getEdgeIndex(edgeEaddress))->getState(curState);
                }
                if (m_State == Device::DeviceState::Off) { break; }
                args[3].Value.Boolean = true; // execute motion
                spdlog::debug("{}: Executing alignment motion for Edge {}...", m_Identity, edgeEaddress);
                status = m_pChildren.at(PAS_EdgeType).at(getEdgeIndex(edgeEaddress))->operate(PAS_EdgeType_Align, args);
                if (!status.isGood()) {
                    spdlog::error("{}: Failed while executing motion. Method aborted.", m_Identity);
                    return status; 
                }
                while (curState == Device::DeviceState::Busy) {
                    usleep(500 * 1000); // microseconds
                    m_pChildren.at(PAS_EdgeType).at(getEdgeIndex(edgeEaddress))->getState(curState);
                }
                if (m_State == Device::DeviceState::Off) { break; }
            }
            spdlog::info("{}: Edge {} aligned.", m_Identity,
                         m_pChildren.at(PAS_EdgeType).at(getEdgeIndex(edgeEaddress))->getIdentity());
        }
    }

    return status;
}

void MirrorController::parseAndSetSelection(const std::string &selectionString, unsigned deviceType) {
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
    if (deviceType == PAS_PanelType) { // expect a list of panel positions
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
    else if (deviceType == PAS_MPESType) { // expect a list of Sensor serials
        m_selectedMPES.clear();
        std::ostringstream os;
        for (const std::string &item : strList) {
            if (m_ChildrenSerialMap.at(deviceType).find(std::stoi(item)) != m_ChildrenSerialMap.at(deviceType).end()) {
                m_selectedMPES.insert(std::stoi(item));
                os << "Added MPES with serial " << item << "." << std::endl;
            }
            else {
                os << "Unable to find MPES with serial " << item << ". Skipping..." << std::endl;
            }
        }
        os << std::endl;
        spdlog::info("{}: Selecting MPES (serial numbers):\n{}\n", m_Identity, os.str());
    }

    else if (deviceType == PAS_EdgeType) { // expect a list of edge addresses
        m_selectedEdges.clear();
        std::ostringstream os;
        for (const std::string &item : strList) {
            if (m_ChildrenEaddressMap.at(deviceType).find(item) != m_ChildrenEaddressMap.at(deviceType).end()) {
                m_selectedEdges.insert(item);
                os << "Added Edge with eAddress " << item << "." << std::endl;
            }
            else {
                os << "Unable to find Edge with eAddress " << item << ". Skipping..." << std::endl;
            }
            os << std::endl;
        }
        os << std::endl;
        spdlog::info("{}: Selecting Edges (eAddresses):\n{}\n", m_Identity, os.str());
    }
}

unsigned MirrorController::getPanelIndex(unsigned position) {
    return m_ChildrenPositionMap.at(PAS_PanelType).at(position);
}

unsigned MirrorController::getMPESIndex(int serialNumber) {
    return m_ChildrenSerialMap.at(PAS_MPESType).at(serialNumber);
}

unsigned MirrorController::getEdgeIndex(const std::string &eAddress) {
    return m_ChildrenEaddressMap.at(PAS_EdgeType).at(eAddress);
}

UaStatus MirrorController::updateCoords(bool print)
{
    UaStatus status;

    status = readPositionAll(print);

    // minimize chisq and get telescope coordinates
    auto minuit = new TMinuit(6); // 6 parameters for 6 telescope coords
    minuit->SetPrintLevel(-1); // suppress all output
    minuit->SetFCN(MirrorControllerCompute::getInstance(this).chiSqFCN);
    // mnparm implements parameter definition:
    // void mnparm(I index, S "name", D start, D step, D LoLim, D HiLim, I& errflag)
    int ierflg;
    // distances in mm
    minuit->mnparm(0, "x", 0., 0.1, -20., 20., ierflg);
    minuit->mnparm(1, "y", 0., 0.1, -20., 20., ierflg);
    minuit->mnparm(2, "z", 0., 0.1, -10., 40., ierflg);
    // angles in radians
    minuit->mnparm(3, "xRot", 0., 0.0001, -0.05, 0.05, ierflg);
    minuit->mnparm(4, "yRot", 0., 0.0001, -0.05, 0.05, ierflg);
    minuit->mnparm(5, "zRot", 0., 0.0001, -0.05, 0.05, ierflg);

    // minimize
    double arglist[2] = {5000, 0.1}; // migrad args: max iterations; convergence tolerance
    minuit->mnexcm("MIGRAD", arglist, 2, ierflg);

    // get results back from MINUIT and copy them to current coordinates vars
    for (int i = 0; i < 6; i++)
        minuit->GetParameter(i, m_curCoords(i), m_curCoordsErr(i));

    if (print) {
        std::ostringstream os;
        for (int i = 0; i < 6; i++)
            os << m_curCoords(i) << " +/- " << m_curCoordsErr(i) << std::endl;
        os << std::endl << std::endl;
        spdlog::info("{} : Computed mirror coordinates:\n{}\n", m_Identity, os.str());
        // display angle in common coordinates -- angle*baseRad
        //    for (int i = 3; i < 6; i++)
        //       os << m_curCoords(i)*kBaseRadius << " +/- " << m_curCoordsErr(i)*kBaseRadius << std::endl;
    }
    m_LastUpdateTime = TIME::now();

    return OpcUa_Good;
}

Eigen::MatrixXd MirrorController::__computeSystematicsMatrix(unsigned pos1, unsigned pos2)
{
    // find influence of moving panel at pos1 on the panel at pos2
    Eigen::MatrixXd res(6, 6);
    // repeat for 6 coords:
    // take panel at pos1, panel1
    // take the coordinates of its neighbor pos2 in the PRF of panel1
    // find the transformed coordinates of panel2 under a motion of panel1 in the PRF of panel1
    // convert these to TRF
    // convert these to PRF of panel2
    // compute the difference between the old coordinates and the new

    int ring = SCTMath::Ring(pos2);
    auto tr2 = __rotMat(2, __getAzOffset(pos2));
    double Acts[6];
    double PadCoords[3][3];

    // original pad coords in TRF:
    Eigen::Matrix3d orig_padCoords2;
    Eigen::Matrix3d orig_padCoords2_PRF2;
    // original panel coords in PRF2:
    Eigen::VectorXd orig_panelCoords2(6);
    for (auto pad = 0; pad < 3; pad++) {
        orig_padCoords2.col(pad) = tr2 * m_PadCoordsTelFrame.at(ring).col(pad);
        orig_padCoords2_PRF2.col(pad) = __toPanelRF(pos2, orig_padCoords2.col(pad));
        for (unsigned coord = 0; coord < 3; coord++)
            PadCoords[pad][coord] = orig_padCoords2_PRF2.col(pad)(coord);
    }

    m_pStewartPlatform->ComputeActsFromPads(PadCoords);
    for (unsigned act = 0; act < 6; act++)
        Acts[act] = m_pStewartPlatform->GetActLengths()[act];
    m_pStewartPlatform->ComputeStewart(Acts);
    for (unsigned coord = 0; coord < 6; coord++)
        orig_panelCoords2(coord) = m_pStewartPlatform->GetPanelCoords()[coord];

    // handle pads coords of panel 2
    Eigen::Matrix3d padCoords2;
    Eigen::VectorXd panelCoords2(6);
    Eigen::VectorXd TRANSFORM(6);
    for (auto TR = 0; TR < 6; TR++) {
        TRANSFORM.setZero();
        TRANSFORM(TR) = (TR > 2) ? 1. / 320. : 1.;
        for (auto pad = 0; pad < 3; pad++) {
            // To PRF::
            padCoords2.col(pad) = __toPanelRF(pos1, orig_padCoords2.col(pad));
            // Moved in PRF of panel1:
            padCoords2.col(pad) = __moveInCurrentRF(padCoords2.col(pad), TRANSFORM);
            // back to TRF:
            padCoords2.col(pad) = __toTelRF(pos1, padCoords2.col(pad));
            // to PRF of panel2:
            padCoords2.col(pad) = __toPanelRF(pos2, padCoords2.col(pad));

            for (unsigned coord = 0; coord < 3; coord++)
                PadCoords[pad][coord] = padCoords2.col(pad)(coord);
        }
        // compute the new position of panel2 in its own frame:
        m_pStewartPlatform->ComputeActsFromPads(PadCoords);
        for (unsigned act = 0; act < 6; act++)
            Acts[act] = m_pStewartPlatform->GetActLengths()[act];
        // phew done!
        // compute new panel coordinates based on these actuator lengths
        m_pStewartPlatform->ComputeStewart(Acts);

        for (unsigned coord = 0; coord < 6; coord++)
            panelCoords2(coord) = m_pStewartPlatform->GetPanelCoords()[coord];

        res.col(TR) = (panelCoords2 - orig_panelCoords2);
    }

    return res;
}

UaStatus MirrorController::alignSector(double alignFrac, bool execute) {

    UaStatus status;
 
    // make sure there are some selected sensors
    if (m_selectedPanels.empty()) {
        spdlog::error("{} : MirrorController::alignSector(): No panels selected. Nothing to do, method aborted.",
                      m_Identity);
        return OpcUa_BadInvalidArgument;
    }
    // make sure there are some selected sensors
    if (m_selectedMPES.empty()) {
        spdlog::error("{} : MirrorController::alignSector(): No sensors selected. Nothing to do, method aborted.",
                      m_Identity);
        return OpcUa_BadInvalidArgument;
    }

    if (alignFrac > 1.0 || alignFrac <= 0.0) {
        spdlog::error(
            "{} : MirrorController::alignSector(): Invalid choice of alignFrac ({}), should be between 0.0 and 1.0.",
            m_Identity, alignFrac);
        return OpcUa_BadInvalidArgument;
    }

    if (!execute) {
        spdlog::info(
            "{} : MirrorController::alignSector(): Called with execute=False. Pre-calculating alignment motion...",
            m_Identity);
        // following the align method for an edge:
        Eigen::MatrixXd C; // constraint
        Eigen::MatrixXd B; // complete matrix

        Eigen::VectorXd X; // solutions vector -- this moves actuators
        Eigen::VectorXd Y; // sensor misalignment vector, we want to fit this

        // grab all user specified panels to move and sensors to fit
        std::vector<std::shared_ptr<PanelController>> panelsToMove;
        std::vector<std::shared_ptr<MPESController>> alignMPES;
        for (unsigned panelPos : m_selectedPanels)
            panelsToMove.push_back(std::dynamic_pointer_cast<PanelController>(m_pChildren.at(PAS_PanelType).at(getPanelIndex(panelPos))));
        for (int mpesSerial : m_selectedMPES) {
            std::shared_ptr<MPESController> mpes = std::dynamic_pointer_cast<MPESController>(
                m_pChildren.at(PAS_MPESType).at(getMPESIndex(mpesSerial)));
            mpes->read();
            if (mpes->isVisible())
                alignMPES.push_back(mpes);
        }

        // get the overlapping sensors -- these are the constraining internal ones
        std::set<unsigned> overlapIndices;
        unsigned idx;
        bool userOverlap = false;
        for (const auto &panel : panelsToMove) {
            for (const auto &mpes : panel->getChildren(PAS_MPESType)) {
                unsigned overlap = 0;
                for (const auto &overlapPanel : panelsToMove)
                    overlap += (
                        std::dynamic_pointer_cast<MPESController>(mpes)->getPanelSide(
                            overlapPanel->getIdentity().position) !=
                        0);

                if (overlap == 2) {
                    idx = m_ChildrenSerialMap.at(PAS_MPESType).at(mpes->getIdentity().serialNumber);
                    if (!overlapIndices.count(idx)) {
                        overlapIndices.insert(idx);
                        if (count(alignMPES.begin(), alignMPES.end(), mpes)) {
                            spdlog::info("{} : MirrorController::alignSector(): Internal MPES selected: {}", m_Identity,
                                         mpes->getIdentity());
                            userOverlap = true;
                        }
                    }
                }
            }
        }

        // if no user specified overlapping sensors, get their readings
        if (!userOverlap && !overlapIndices.empty()) {
            spdlog::warn(
                "{} : MirrorController::alignSector(): No user-selected internal MPES. Reading all internal MPES...",
                m_Identity);
            // only read the internal MPES if no user-specified ones have been found
            for (const auto &i: overlapIndices) {
                std::shared_ptr<MPESController> mpes = std::dynamic_pointer_cast<MPESController>(
                    m_pChildren.at(PAS_MPESType).at(i));
                mpes->read();
                if (mpes->isVisible())
                    alignMPES.push_back(mpes);
            }
        } else if (overlapIndices.empty()) {
            spdlog::warn(
                "{} : MirrorController::alignSector(): Found no internal MPES. This should only occur when the sector is a single panel.",
                m_Identity);
        }
        else
            spdlog::warn("{} : MirrorController::alignSector(): Found user-selected internal MPES.", m_Identity);

        std::ostringstream os;
        for (auto &panel : panelsToMove)
            os << panel->getIdentity().position << " ";
        os << std::endl << std::endl;
        spdlog::info("{}: Will align the following panels:\n{}\n", m_Identity, os.str());
        os.str(""); // Clear ostringstream

        for (auto &mpes: alignMPES)
            os << mpes->getIdentity().serialNumber << " ";
        os << std::endl << std::endl;
        spdlog::info("{}: Will use the following sensors for alignment:\n{}\n", m_Identity, os.str());

        // construct the overall target vector and the response matrix
        UaVariant vtmp;
        // store the current readings and the target readings
        Eigen::VectorXd curRead(2 * alignMPES.size());
        Eigen::VectorXd targetRead(2 * alignMPES.size());
        // store individual response matrix;
        Eigen::MatrixXd responseMat(2, 6);

        unsigned nCols = 6 * panelsToMove.size();
        unsigned nRows = 2 * alignMPES.size();
        B = Eigen::MatrixXd(nRows, nCols);
        Y = Eigen::VectorXd(nRows);
        for (int m = 0; m < (int) alignMPES.size(); m++) {
            alignMPES.at(m)->getData(PAS_MPESType_xCentroidAvg, vtmp);
            vtmp.toDouble(curRead(m * 2));
            alignMPES.at(m)->getData(PAS_MPESType_yCentroidAvg, vtmp);
            vtmp.toDouble(curRead(m * 2 + 1));
            targetRead.segment(2 * m, 2) = alignMPES.at(m)->getAlignedReadings()
                                           - alignMPES.at(m)->getSystematicOffsets();

            for (int p = 0; p < (int) panelsToMove.size(); p++) {
                auto panelSide = alignMPES.at(m)->getPanelSide(panelsToMove.at(p)->getIdentity().position);
                if (panelSide)
                    responseMat = alignMPES.at(m)->getResponseMatrix(panelSide);
                else
                    responseMat.setZero();
                B.block(2 * m, 6 * p, 2, 6) = responseMat;
            }
        }
        Y = targetRead - curRead;

        // make sure we have enough constraints to solve this
        if (Y.size() < B.cols()) {
            spdlog::error(
                "{}: Found () sensors and {} actuators. Not enough sensors to constrain the motion. Method aborted.",
                m_Identity, B.rows() / 2, B.cols());
            return OpcUa_Bad;
        }

        try {
            X = B.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(Y);
        }
        catch (...) {
            spdlog::error(
                "{} : Singular Value Decomposition failed. Result discarded and method aborted. Check your sensor readings!",
                m_Identity);
            return OpcUa_Bad;
        }

        spdlog::debug("{}: Vector to solve for:\n{}\n", m_Identity, Y);
        spdlog::debug("{}: Matrix to solve with:\n{}\n", m_Identity, B);
        spdlog::debug("{}: Least squares solution:\n{}\n", m_Identity, X);

        if (alignFrac < 1.) {
            X *= alignFrac;
            spdlog::info("{} : Fractional motion of {} requested. Final (fractional) motion is:\n{}\n", m_Identity,
                         alignFrac,
                         X);
        }

        unsigned j = 0;
        Eigen::VectorXd deltaLengths(6);
        for (auto &pCurPanel : panelsToMove) {
            // print out to make sure
            spdlog::info("{} : Will move Panel {} actuators by:\n{}\n", m_Identity, pCurPanel->getIdentity(),
                         X.segment(j, 6));

            for (unsigned i = 0; i < 6; i++) {
                deltaLengths(i) = X(j++);
            }

            /**
            if(pCurPanel->checkForCollision(deltaLengths)) {
                return OpcUa_Bad;
            }
             */
        }
        m_Xcalculated = X; // set calculated motion
        m_panelsToMove = panelsToMove;
        m_previousCalculatedMethod = PAS_MirrorType_AlignSector;

        spdlog::info(
            "{} : Calculation done! You should call the method again with execute=True to actually execute the motion.",
            m_Identity);
    }
    else {
        if (m_Xcalculated.isZero(0) || m_previousCalculatedMethod != PAS_MirrorType_AlignSector) {
            spdlog::error(
                "{} : No calculated motion found. Call Mirror.alignSector with execute=false once first to calculate the motion to execute.",
                m_Identity);
            return OpcUa_Bad;
        }
        else {        
            unsigned j = 0;
            for (auto &pCurPanel : m_panelsToMove) {
                auto nACT = pCurPanel->getActuatorCount();
                UaVariantArray deltas;
                deltas.create(nACT);
                UaVariant val;
                for (unsigned i = 0; i < nACT; i++) {
                    val.setFloat(m_Xcalculated(j++));
                    val.copyTo(&deltas[i]);
                }
                status = pCurPanel->__moveDeltaLengths(deltas);
                if (!status.isGood()) { return status; }
                if (m_State == Device::DeviceState::Off) { break; }
            }
            m_Xcalculated.setZero(); // reset calculated motion
            m_panelsToMove.clear();
            m_previousCalculatedMethod = 0;
        }
    }
    return status;
}


UaStatus MirrorController::alignRing(int fixPanel, double alignFrac, bool execute)
{
    UaStatus status;

    if (m_ChildrenPositionMap.at(PAS_PanelType).find(fixPanel) == m_ChildrenPositionMap.at(PAS_PanelType).end()) {
        spdlog::error(
            "{} : MirrorController::alignRing(): The selected fixed panel ({}) is not found in the mirror. Please double check and try again.",
            m_Identity, fixPanel);
        return OpcUa_BadInvalidArgument;
    }

    if (alignFrac > 1.0 || alignFrac <= 0.0) {
        spdlog::error(
            "{} : MirrorController::alignRing(): Invalid choice of alignFrac ({}), should be between 0.0 and 1.0.",
            m_Identity, alignFrac);
        return OpcUa_BadInvalidArgument;
    }

    if (!execute) {
        spdlog::info(
            "{} : MirrorController::alignRing(): Called with execute=False. Pre-calculating alignment motion...",
            m_Identity);
        // First check that all panels in ring are present
        spdlog::info("{} : MirrorController::alignRing(): Locating all panels in ring to align...");
        int curPanel = fixPanel;
        do {
            int nextPanel = SCTMath::GetPanelNeighbor(curPanel, 0);
            if (m_ChildrenPositionMap.at(PAS_PanelType).find(nextPanel) !=
                    m_ChildrenPositionMap.at(PAS_PanelType).end()) {
                spdlog::debug("{} : MirrorController::alignRing(): Found next panel {}...", m_Identity, nextPanel);
                continue;
            }
            else {
                spdlog::error(
                    "{} : MirrorController::alignRing(): Searched for next panel ({}), but did not find as child of mirror. Please double-check that all panels in ring are present in the client.",
                    m_Identity, nextPanel);
                    return OpcUa_Bad;
            }
            curPanel = nextPanel;
        } while (curPanel != fixPanel);

        
        // we want to fit all sensors simultaneously while constraining the motion of 'fixPanel'
        // first, find which ring we are on:
        unsigned ring = SCTMath::Ring(fixPanel);
        unsigned mirror = SCTMath::Mirror(fixPanel);
        
        unsigned numPanels = 0;

        if (mirror == 1)
            numPanels = SCT::Primary::kPanels[ring - 1];
        else if (mirror == 2)
            numPanels = SCT::Secondary::kPanels[ring - 1];
        else if (mirror == 3)
            numPanels = SCT::Primary::kPanels[ring - 1];

        // initialize the response matrices
        Eigen::MatrixXd localResponse;
        Eigen::MatrixXd globResponse = Eigen::MatrixXd(numPanels * 6, numPanels * 6);
        globResponse.setZero();
        // initialize the misalignment vector
        Eigen::VectorXd localAlignRead;
        Eigen::VectorXd localCurRead;
        Eigen::VectorXd misalignVec;
        Eigen::VectorXd globMisalignVec;
        // initialize the displacement vector
        Eigen::VectorXd globDisplaceVec = Eigen::VectorXd(numPanels * 6);

        // get all the edges we need to fit:
        std::vector<std::shared_ptr<EdgeController>> edgesToFit; // we actually don't need to keep these in a vector,
                                      // but doing this for possible future needs
        std::vector<std::shared_ptr<PanelController>> panelsToMove;
        curPanel = fixPanel;
        int nextPanel, cursize = 0;
        Device::Identity id;
        // keep track of the position in the global response matrix
        unsigned blockRow = 0;
        spdlog::info(
            "{}: MirrorController::alignRing() : Traversing Ring {} of Mirror {} clockwise from fix panel ({}) to locate all edges to align.",
            m_Identity, ring, mirror, fixPanel);

        Eigen::MatrixXd T; // Vladimir's T operator
        Eigen::MatrixXd Eye = Eigen::MatrixXd::Identity(6, 6);
        Eigen::MatrixXd MCurPrev, MCurNext, MNextCur;
        Eigen::MatrixXd E;
        T = Eye;

        panelsToMove.push_back(std::dynamic_pointer_cast<PanelController>(
            m_pChildren.at(PAS_PanelType).at(m_ChildrenPositionMap.at(PAS_PanelType).at(
                curPanel)))); // add the fixed panel to panelsToMove (only temporary, will remove later)

        do {
            nextPanel = SCTMath::GetPanelNeighbor(curPanel, 0);
            try {
                if (m_ChildrenPositionMap.at(PAS_PanelType).find(nextPanel) !=
                    m_ChildrenPositionMap.at(PAS_PanelType).end()) {
                    panelsToMove.push_back(std::dynamic_pointer_cast<PanelController>(
                        m_pChildren.at(PAS_PanelType).at(m_ChildrenPositionMap.at(PAS_PanelType).at(curPanel)) ) );
                    edgesToFit.push_back(std::dynamic_pointer_cast<EdgeController>(
                        m_pChildren.at(PAS_EdgeType).at(
                            getEdgeIndex(SCTMath::GetEdgeFromPanels({curPanel, nextPanel})))));
                } else {
                    curPanel = nextPanel;
                    continue;
                }

                spdlog::info("{}: Found next edge to align -- {} ", m_Identity, edgesToFit.back()->getIdentity());

                spdlog::debug("{}: Reading sensors...", m_Identity);
                localCurRead = edgesToFit.back()->getCurrentReadings().first;
                localAlignRead = edgesToFit.back()->getAlignedReadings();

                misalignVec = localAlignRead - localCurRead;
                spdlog::info("{}: Edge {} current misalignment:\n{}\n", m_Identity, edgesToFit.back()->getIdentity(),
                             misalignVec);
                spdlog::info("{}: Adding to global misalignment vector...", m_Identity);
                globMisalignVec.conservativeResize(cursize + misalignVec.size());
                globMisalignVec.tail(misalignVec.size()) = misalignVec;
                cursize = globMisalignVec.size();

                // get the response of this edge
                localResponse = edgesToFit.back()->getResponseMatrix(curPanel);
                spdlog::debug("{}: Response matrix [{}][{}]:\n{}\n", m_Identity, curPanel, nextPanel, localResponse);
                spdlog::trace("{}: Placing it at location [{}][{}] of the global response matrix", m_Identity,
                              blockRow * 6,
                              blockRow * 6);
                globResponse.block(blockRow*6, blockRow*6,
                        localResponse.rows(), localResponse.cols()) = localResponse;
                MCurNext = localResponse;

                localResponse = edgesToFit.back()->getResponseMatrix(nextPanel);
                spdlog::debug("{}: Response matrix [{}][{}]:\n{}\n", m_Identity, nextPanel, curPanel, localResponse);
                spdlog::trace("{}: Placing it at location [{}][{}] of the global response matrix", m_Identity,
                              blockRow * 6,
                              ((blockRow + 1) % numPanels) * 6);
                globResponse.block(blockRow*6, ((blockRow + 1) % numPanels)*6,
                        localResponse.rows(), localResponse.cols()) = localResponse;

                MNextCur = localResponse;

                // replace the first 6 columns of the response matrix with 6x6 identity matrices:
                // this gets rid of the response matrices corresponding to the fixed panel
                // and replaces them with the Systematic Offset Response matrix (just identity)
                globResponse.block(blockRow * 6, 0, 6, 6) = Eigen::MatrixXd::Identity(6, 6);
            }
            catch (...) {
            }

            curPanel = nextPanel;
            ++blockRow;

            // nothing to do if on the first panel
            if (blockRow != 1) {
                spdlog::debug("{}: Taking into account/calculating the systematic displacement for edge {}", m_Identity,
                              edgesToFit.back()->getIdentity());
                E = MCurNext*MCurPrev.inverse();
                spdlog::debug("{}: Matrix ML:\n{}\n", m_Identity, MCurPrev);
                spdlog::debug("{}: ML*ML.inverse():\n{}\n", m_Identity, MCurPrev * MCurPrev.inverse());
                spdlog::debug("{}: Matrix MR:\n{}\n", MCurNext);
                spdlog::debug("{}: Operator E = MR*ML.inverse():\n{}\n", E);
                T = Eye - E*T;
            }

            MCurPrev = MNextCur;

        } while ( curPanel != fixPanel );

        spdlog::info("{}: Size of global misalignment vector: {}", m_Identity, globMisalignVec.size());
        spdlog::info("{}: Shape of global response matrix: ({}, {})", m_Identity, globResponse.rows(),
                     globResponse.cols());

        spdlog::debug("{} : Operator T:\n{}\n", m_Identity, T);
        spdlog::debug("{} : Operator -1*T.inverse():\n{}\n", m_Identity, -1 * T.inverse());
        spdlog::debug("{} : Operator T*T.inverse():\n{}\n", m_Identity, T * T.inverse());

        // check that we have enough sensor readings
        if (globMisalignVec.size() < globResponse.rows()) {
            spdlog::error("{} : Not enough sensors () to constrain the motion ({} required). Method call aborted.",
                          m_Identity, globMisalignVec.size(), globResponse.rows());
            return OpcUa_Bad;
        }

        spdlog::debug("{} : Misalignment vector size:{}.", m_Identity, globMisalignVec.size());
        spdlog::debug("{} : First 12 entries:\n{}\n.", m_Identity, globMisalignVec.head(12));
        spdlog::debug("{} : Last 12 entries:\n{}\n.", m_Identity, globMisalignVec.tail(12));
        spdlog::debug("{} : First 12x18 block of the global response matrix:\n{}\n.", m_Identity,
                      globResponse.block(0, 0, 12, 18));
        spdlog::debug("{} : Last 12x18 block of the global response matrix:\n{}\n.", m_Identity,
                      globResponse.block(globResponse.rows() - 12, globResponse.cols() - 18, 12, 18));

        panelsToMove.erase(panelsToMove.begin()); // Erase fixed panel (first panel) from panelsToMove

        // we are all set now. now solve the system
        // globResponse * globDisplaceVec = globMisalignVec;
        // the first 6 elements of the solution are the systematic vectors, the others are the
        // panel displacements
        globDisplaceVec = globResponse.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(globMisalignVec);

        Eigen::VectorXd check = globResponse * globDisplaceVec - globMisalignVec;
        spdlog::debug("{} : Checking the calculated solution... norm(R*Solution - MisAlign) = {}.", m_Identity,
                      check.norm());
        //spdlog::debug("{} : First 12 entries:\n{}\n.", m_ID, check.head(12));
        //spdlog::debug("{} : Last 12 entries:\n{}\n.", m_ID, check.tail(12));

        m_sysOffsetsMPES = globDisplaceVec.head(6);
        // get the order of read sensors:
        unsigned pos = 0;
        for (const auto& mpes : edgesToFit.back()->m_ChildrenPositionMap.at(PAS_MPESType)) {
            SystematicOffsetsMPESMap[ring][mpes.first] = m_sysOffsetsMPES.segment(pos*2, 2);
            ++pos;
        }
        // make sure we read out the systematic vector in the same order as the sensors
        // and assign it correctly to outer/middle/inner
        spdlog::info("{} : Calculated systematic offsets:\n{}\n.", m_Identity, m_sysOffsetsMPES);

        // set the offset for each sensor edge
        for (const auto& edge : edgesToFit) {
            // and set them for each sensor along each edge
            for (const auto& mpes : edge->m_ChildrenPositionMap.at(PAS_MPESType) ) {
                (std::dynamic_pointer_cast<MPESController>(
                    edge->m_pChildren.at(PAS_MPESType).at(mpes.second)))->m_SystematicOffsets =
                    SystematicOffsetsMPESMap.at(ring).at(mpes.first);
            }
        }

        if (alignFrac < 1.) {
            spdlog::info("{} : Fractional motion of {} requested.", m_Identity, alignFrac);
        }

        /* MOVE ACTUATORS */
        unsigned j = 6;
        Eigen::VectorXd deltaLengths(6);
        for (auto &pCurPanel : panelsToMove) {
            // print out to make sure
            spdlog::info("{} : Panel {} -- Calculated change in actuator lengths is:\n{}\n", m_Identity,
                         pCurPanel->getIdentity(),
                         alignFrac * globDisplaceVec.segment(j, 6));

            for (unsigned i = 0; i < 6; i++) {
                deltaLengths(i) = alignFrac * globDisplaceVec(j++);
            }
            /**
            if(pCurPanel->checkForCollision(deltaLengths)) {
                return OpcUa_Bad;
            }
             */
        }
        m_Xcalculated = globDisplaceVec.tail(globDisplaceVec.size()-6);
        m_panelsToMove = panelsToMove;
        m_previousCalculatedMethod = PAS_MirrorType_AlignRing;
    }
    else {
        if (m_Xcalculated.isZero(0) || m_previousCalculatedMethod != PAS_MirrorType_AlignRing) {
            spdlog::error(
                "{} : No calculated motion found. Call Mirror.AlignRing with execute=false once first to calculate the motion to execute.",
                m_Identity);
            return OpcUa_Bad;
        }
        else {        
            unsigned j = 0;
            for (auto &pCurPanel : m_panelsToMove) {
                auto nACT = pCurPanel->getActuatorCount();
                UaVariantArray deltas;
                deltas.create(nACT);
                UaVariant val;
                for (unsigned i = 0; i < nACT; i++) {
                    val.setFloat(m_Xcalculated(j++));
                    val.copyTo(&deltas[i]);
                }
                status = pCurPanel->__moveDeltaLengths(deltas);
                if (!status.isGood()) { return status; }
                if (m_State == Device::DeviceState::Off) { break; }
            }
            m_Xcalculated.setZero(); // reset calculated motion
            m_panelsToMove.clear();
            m_previousCalculatedMethod = 0;
        }
    }

    return status;
}

/* =========== Coordinate Transformations =========== */

// get the angular offset of this panel from our ideal one for which everything is precomputed
double MirrorController::__getAzOffset(unsigned pos)
{
    // we have the ideal origin and norm of the "nominal" panel.
    // Need to identify the current panel placement and transform accordingly
    //
    // pos = <MIRROR><QUADRANT><RING><PANEL>
    // we need the quadrant and the panel within it to calculate the azimuthal rotation
    int quadrant = SCTMath::Quadrant(pos); // 4 quadrants in total
    int ring = SCTMath::Ring(pos);
    int panel = SCTMath::Panel(pos);

    // number of panels in a quadrant
    double quadPanels = M_PI/2./m_PanelWidth.at(ring);
    // azimuthal offset of this panel from the ideal one
    // (numbering of the panels increases in the direction opposite from z rotation, hence the
    // minus sign below)
    double phiOffset = m_PanelOffset.at(ring) -
        (quadPanels*(quadrant - 1) + panel - 1)*m_PanelWidth.at(ring);

    return phiOffset;
}


Eigen::Vector3d MirrorController::__toPanelRF(unsigned pos, const Eigen::Vector3d &in_coords)
{
    // all angles in radians

    int ring = SCTMath::Ring(pos);
    double phi = __getAzOffset(pos);

    // this panel's frame is the rotated frame of the ideal panel
    Eigen::Matrix3d zRot = __rotMat(2, phi);
    Eigen::Matrix3d panelFrame = zRot * m_PanelFrame.at(ring);
    Eigen::Vector3d panelOrigin = zRot * m_PanelOriginTelFrame.at(ring);

    return panelFrame.transpose() * (in_coords - panelOrigin);
}

Eigen::Vector3d MirrorController::__toTelRF(unsigned pos, const Eigen::Vector3d &in_coords)
{
    // all angles in radians
    // The inverse of the above

    int ring = SCTMath::Ring(pos);
    double phi = __getAzOffset(pos);

    // this panel's frame is the rotated frame of the ideal panel
    Eigen::Matrix3d zRot = __rotMat(2, phi);
    Eigen::Matrix3d panelFrame = zRot * m_PanelFrame.at(ring);
    Eigen::Vector3d panelOrigin = zRot * m_PanelOriginTelFrame.at(ring);

    // remember panel frame is an orthogonal matrix -- very simple inversion
    return panelFrame * in_coords + panelOrigin;
}

Eigen::Matrix3d MirrorController::__rotMat(int axis, double a)
{
    double c = cos(a);
    double s = sin(a);

    Eigen::Matrix3d rot;
    if (axis == 0) // x rotation
        rot << 1., 0., 0.,
               0., c, -s,
               0., s,  c;
    else if (axis == 1) // y rotation
        rot << c,  0., s,
               0., 1., 0.,
              -s,  0., c;
    else if (axis == 2) // z rotation
        rot << c, -s,  0.,
               s,  c,  0.,
               0., 0., 1.;

    return rot;
}

Eigen::Vector3d MirrorController::__moveInCurrentRF(const Eigen::Vector3d &in_vec, const Eigen::VectorXd &tr_coords)
{
    // check that we have 6 coords. if not, resize if necessary
    Eigen::VectorXd tr(6);
    tr.head(tr_coords.size()) = tr_coords;
    if (tr_coords.size() != 6)
        tr.tail(6 - tr_coords.size()).setZero();

    Eigen::Vector3d out_vec = in_vec;
    // compute the transform due to the change in TRF: Rot(z -> x -> y)*v + T
    out_vec = __rotMat(2, tr(5)) * out_vec;
    out_vec = __rotMat(0, tr(3)) * out_vec;
    out_vec = __rotMat(1, tr(4)) * out_vec;
    out_vec += tr.head(3);

    return out_vec;
}



/* ============== MINUIT INTERFACE ============== */
double MirrorController::chiSq(const Eigen::VectorXd &telDelta)
{
    // tel delta is a perturbation to the coordinates of the mirror
    double chiSq = 0.;
    const auto& panels = m_pChildren.at(PAS_PanelType);

    // pad coordinates in TRF as computed from actuator lengths
    Eigen::Vector3d padCoordsActs;
    // pad coordinates in TRF as computed from telescope perturbation
    Eigen::Vector3d padCoordsTel;

    UaVariant val;
    // go over all panels
    for (unsigned panelPos : m_selectedPanels) {
        int pos = (int)panelPos;
        int ring = SCTMath::Ring(pos);

        // go over all pads -- ChiSq is the squared difference between pad coordinates
        // as computed from actuator lengths and pad coordinates as computed from telescope
        // coordinates
        for (int pad = 0; pad < 3; pad++) {
            padCoordsActs = std::dynamic_pointer_cast<PanelController>(panels.at(getPanelIndex(panelPos)))->getPadCoords().col(
                pad);
            // and transform this to the telescope reference frame:
            // these are pad coordinates in TRF as computed from actuator lengths
            padCoordsActs = __toTelRF(pos, padCoordsActs);

            // compute telescope frame pad coordinates by transforming the ideal ones
            padCoordsTel = __rotMat(2, __getAzOffset(pos)) * m_PadCoordsTelFrame.at(ring).col(pad);
            // compute transform by the telescope perturbation: Rot(z -> x -> y)*v + T
            padCoordsTel = __moveInCurrentRF(padCoordsTel, telDelta);
            // padCoordsTel are the pad coords in TRF as computed from Tel coords

            // change the value of ChiSq:
            chiSq += (padCoordsTel - padCoordsActs).squaredNorm();
        }
    }

    return chiSq;
}

void MirrorControllerCompute::chiSqFCN(int &npar, double *gin, double &f, double *par, int iflag) // MINUIT interface
{
    Eigen::Matrix<double, 6, 1> vecPars(par);
    f = m_Mirror->chiSq(vecPars);
}
