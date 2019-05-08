#include "mirrorcontroller.hpp"
#include "common/simulatestewart/mathtools.h"
#include "common/simulatestewart/mirrordefinitions.h" // definitions of the mirror surfaces
#include "AGeoAsphericDisk.h" // ROBAST dependency
#include <algorithm> // std::count
#include <string>
#include <iostream>
#include <cmath>
#include <deque>
#include <chrono>
#include "TMinuit.h" // ROOT's implementation of MINUIT for chiSq minimization


// implementation of the mirror class that controls the whole mirror.
// math, algorithms and all that coordinate stuff go here

#include "common/alignment/device.hpp"

#include "client/controllers/panelcontroller.hpp"
#include "client/controllers/mpescontroller.hpp"
#include "client/controllers/edgecontroller.hpp"
#include "client/controllers/pascontroller.hpp"

MirrorController *MirrorControllerCompute::m_Mirror = nullptr;

MirrorController::MirrorController(Device::Identity identity) : PasCompositeController(identity, nullptr,
                                                                                       10000),
                                                                m_pSurface(nullptr) {
    std::string mirrorprefix;
    if (m_ID.position == 1)
        mirrorprefix = "Primary";
    else if (m_ID.position == 2)
        mirrorprefix = "Secondary";
    else
        mirrorprefix = "Unknown";
    m_ID.name = mirrorprefix + "Mirror";

    m_state = Device::DeviceState::On;

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
    m_lastUpdateTime = TIME::now() - std::chrono::duration<int, std::ratio<1, 1000>>(m_kUpdateInterval_ms);                                                   
}

void MirrorController::addChild(OpcUa_UInt32 deviceType, const std::shared_ptr<PasController> &pController)
{
    // call the base type's method
    PasCompositeController::addChild(deviceType, pController);

    if (deviceType == PAS_PanelType) {
        m_selectedPanels.insert((unsigned)pController->getId().position);
    }
    else if (deviceType == PAS_EdgeType) {
        m_selectedEdges.insert(pController->getId().eAddress);
    }
}


bool MirrorController::initialize()
{
    std::cout << "\n\tInitializing " << m_ID.name << "..." << std::endl;

    // precompute everything we need
    if (m_ID.position == 1) {
        // bottom (w.r.t. z) surface first, then top surface --
        // back surface first, then optical surface
        m_pSurface = new AGeoAsphericDisk(m_ID.name.c_str(),
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
    else if (m_ID.position == 2) {
        // bottom (w.r.t. z) surface first, then top surface --
        // optical surface first, then back surface
        m_pSurface = new AGeoAsphericDisk(m_ID.name.c_str(),
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
    }
    else if (m_ID.position == 3) { // this is used for the test setup in the lab (2 P2 panels) - duplicates the primary mirror geometry
        // bottom (w.r.t. z) surface first, then top surface --
        // back surface first, then optical surface
        m_pSurface = new AGeoAsphericDisk(m_ID.name.c_str(),
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
        std::cout << "\tNo mirror at position " << m_ID.position << " -- nothing to do!" << std::endl;
        return false;
    }

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
            m_SP.SetPanelType(StewartPlatform::PanelType::OPT);
            double actL[6];
            for (auto &len : actL)
                len = SCT::kActuatorLength;
            m_SP.ComputeStewart(actL);
            Eigen::Vector3d PanelCenterPanelFrame(m_SP.GetPanelCoords());
            // this is the center of the mirror panel in the panel frame;
            // what we obtained before was the center of the mirror panel in the TRF --
            // need to shift those coordinates by this much opposite to the mirror norm
            // to get the origin of the panel frame in the TRF:
            m_PanelOriginTelFrame.at(ring + 1) -= m_PanelFrame.at(ring+1)*PanelCenterPanelFrame;

            std::cout << "Ideal Panel Frame origin for Ring " << ring + 1 << " in the Telescope frame :"
                      << std::endl << m_PanelOriginTelFrame.at(ring + 1) << std::endl;
            std::cout << "Ideal Panel Frame for Ring " << ring + 1 << " in the Telescope frame :"
                      << std::endl << m_PanelFrame.at(ring + 1) << std::endl << std::endl;
        }
    }

    std::cout << "Done Initializing " << m_ID.name << std::endl << std::endl;

    return true;
}


MirrorController::~MirrorController()
{
    m_pClient = nullptr; // this shouldn't have been changed anyway, but just in case

    delete m_pSurface;
    m_pSurface = nullptr;

    for (auto &devVector : m_pChildren)
        for (auto &dev : devVector.second)
            dev = nullptr;

    m_state = Device::DeviceState::Off;
}

UaStatus MirrorController::getState(Device::DeviceState &state)
{
    //UaMutexLocker lock(&m_mutex);
    state = m_state;
    return OpcUa_Good;
}

UaStatus MirrorController::setState(Device::DeviceState state)
{
    //UaMutexLocker lock(&m_mutex);
    m_state = state;
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
    }
    else
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

    /**********************************************************
     * Move the whole mirror in the telescope reference frame *
     * ********************************************************/
    if (offset == PAS_MirrorType_MoveToCoords) {
        updateCoords(false);    
        Eigen::VectorXd targetMirrorCoords(6);
        for (int i = 0; i < 6; i++) {
            UaVariant(args[i]).toDouble(targetMirrorCoords[i]);
        }

        bool execute = args[6].Value.Boolean;

        status = moveToCoords(targetMirrorCoords, execute);
        updateCoords(false);
    }
    /**********************************************************
     * Read out all current panel positions in their frames   *
     * ********************************************************/
    else if (offset == PAS_MirrorType_ReadPosition) {
        // read out all individual positions
        // and get global mirror coordinates
        updateCoords();
    }

    /**********************************************************
     * Align the selected sector to selected sensors          *
     * ********************************************************/
    else if (offset == PAS_MirrorType_AlignSector) {

        std::set<unsigned> selectedPanels = getSelectedDeviceIndices(PAS_PanelType);
        std::set<unsigned> selectedMPES = getSelectedDeviceIndices(PAS_MPESType);

        // make sure there are some selected sensors
        if (selectedPanels.empty()) {
            std::cout << "+++ ERROR +++ No panels selected! Nothing to do." << std::endl;
            return OpcUa_BadInvalidArgument;
        }
        // make sure there are some selected sensors
        if (selectedMPES.empty()) {
            std::cout << "+++ ERROR +++ No sensors selected! Nothing to do." << std::endl;
            return OpcUa_BadInvalidArgument;
        }

        double alignFrac = args[0].Value.Double;
        bool execute = args[1].Value.Boolean;

        status = alignSector(selectedPanels, selectedMPES, alignFrac, execute);
        updateCoords(false);
    }

    /**********************************************************************************************
     * Perform global alignment and compute the systematics vector keeping the input panel fixed  *
     * ********************************************************************************************/
    else if (offset == PAS_MirrorType_GlobalAlign) {
        // get the position of the panel to keep fixed
        unsigned fixPanel = args[0].Value.UInt32;

        try {
            m_ChildrenPositionMap.at(PAS_PanelType).at(fixPanel);
        }
        catch (std::out_of_range &e) {
            std::cout << "+++ ERROR +++ The selected panel is not connected! Nothing to do." << std::endl;
            return OpcUa_BadInvalidArgument;
        }

        bool execute = args[1].Value.Boolean;
        
        status = alignGlobal(fixPanel, execute);
        updateCoords(false);
    }


    /**********************************************************
     * Align all edges of the mirror                          *
     * ********************************************************/
    else if (offset == PAS_MirrorType_AlignSequential) {
        // make sure the arguments make sense -- we are supposed
        // to get an edge position and a direction. The type has
        // already been validated by the caller, but we need to
        // check the existence of such edge and direction:
        Device::Identity startId;
        // first argument is the starting edge address
        startId.eAddress = UaString(args[0].Value.String).toUtf8();

        std::set<unsigned> selectedEdges = getSelectedDeviceIndices(PAS_EdgeType);

        if (selectedEdges.empty()) {
            std::cout << "No edges selected. Nothing to do..." << std::endl;
            return OpcUa_Good;
        }

        // second argument is the direction
        unsigned dir = args[1].Value.UInt32;
        // check that these are valid;
        if (dir != 0 && dir != 1)
            return OpcUa_BadInvalidArgument;
        unsigned idx;
        try {
            idx = m_ChildrenIdentityMap.at(PAS_EdgeType).at(startId);
        }
        catch (std::out_of_range &e) {
            return OpcUa_BadInvalidArgument;
        }

        status = alignSequential(idx, selectedEdges, (bool) dir);
        updateCoords(false);
    }

    /**********************************************************
     * Read out all current edge sensors                      *
     * ********************************************************/
    else if (offset == PAS_MirrorType_ReadSensors) {
        auto edgeIndices = getSelectedDeviceIndices(PAS_EdgeType);
        if (edgeIndices.empty()) {
            std::cout << "No edges selected in SelectedEdges! Nothing to do..." << std::endl;
        }
        else {
            for (const auto &idx : edgeIndices) {
                status = std::dynamic_pointer_cast<EdgeController>(m_pChildren.at(PAS_EdgeType).at(idx))->operate(
                    PAS_EdgeType_Read);
                if (!status.isGood()) { return status; }
            }
        }
    }

    /************************************************
     * stop the motion in progress                  *
     * **********************************************/
    else if (offset == PAS_MirrorType_Stop) {
        std::cout << m_ID.name << "::Operate() : Attempting to gracefully stop all motions." << std::endl;
        for (const auto &p: m_pChildren.at(PAS_PanelType)) {
            status = std::dynamic_pointer_cast<PanelController>(p)->operate(PAS_PanelType_Stop);
            if (!status.isGood()) { 
                updateCoords(false);
                return status; 
            }
        }
    } else {
        return OpcUa_BadNotImplemented;
    }

    return status;
}


/* ================= INTERNAL IMPLEMENTATIONS OF ALL METHODS ================== */

UaStatus MirrorController::readPositionAll(bool print) {
    UaStatus status;
    
    for (const auto &idx : getSelectedDeviceIndices(PAS_PanelType)) {
        std::dynamic_pointer_cast<PanelController>(m_pChildren.at(PAS_PanelType).at(idx))->updateCoords(print);
        auto pos = m_pChildren.at(PAS_PanelType).at(idx)->getId().position;

        auto padCoordsActs = std::dynamic_pointer_cast<PanelController>(
            m_pChildren.at(PAS_PanelType).at(idx))->getPadCoords();
        if (print) {
            std::cout << "Panel frame pad coordinates:\n" << padCoordsActs << std::endl;
            // and transform this to the telescope reference frame:
            // these are pad coordinates in TRF as computed from actuator lengths
            for (int i = 0; i < padCoordsActs.cols(); i++)
                padCoordsActs.col(i) = __toTelRF(pos, padCoordsActs.col(i));
            std::cout << "Telescope frame pad coordinates:\n" << padCoordsActs << std::endl;
        }
    }

    return status;
}

UaStatus MirrorController::moveToCoords(Eigen::VectorXd targetMirrorCoords, bool execute) {
    UaStatus status;
    
    if (!execute) {
        Eigen::VectorXd deltaMirrorCoords(6);

        deltaMirrorCoords = targetMirrorCoords - m_curCoords;

        std::cout << "Moving Mirror " << m_ID.position << " To Target Coordinates:\n"
                  << targetMirrorCoords << std::endl;

        std::cout << "Delta Coordinates: "
                  << deltaMirrorCoords << std::endl;
        std::cout << std::endl << std::endl;


        std::vector<std::shared_ptr<PanelController>> panelsToMove;
        Eigen::VectorXd X(m_pChildren.at(PAS_PanelType).size()*6);

        unsigned positionNum;
        Eigen::VectorXd prf_coords;
        Eigen::VectorXd deltaActLengths(6);
        Eigen::VectorXd targetActLengths(6);
        Eigen::VectorXd currentActLengths(6);
        
        int j = 0;
        for (const auto &pPanel : m_pChildren.at(PAS_PanelType)) {
            positionNum = pPanel->getId().position;
            std::cout << "Panel " << positionNum << ":" << std::endl;
            std::dynamic_pointer_cast<PanelController>(pPanel)->operate(PAS_PanelType_ReadAll);
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
            m_SP.ComputeActsFromPads(newPadCoords);
            for (int i = 0; i < 6; i++) {
                targetActLengths(i) = (float)m_SP.GetActLengths()[i];
            }
            currentActLengths = std::dynamic_pointer_cast<PanelController>(pPanel)->getActuatorLengths();

            deltaActLengths = targetActLengths - currentActLengths;

            if (std::dynamic_pointer_cast<PanelController>(pPanel)->checkForCollision(deltaActLengths)) {
                std::cout << "Error: Sensors may go out of range! Disallowed motion, please recalculate or relax safety radius." << std::endl;
                return OpcUa_Bad;
            }

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
            std::cout << "No calculated motion found.  Call Mirror.MoveToCoords with execute=false once first to calculate the motion to execute." << std::endl;
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
                status = pCurPanel->moveDeltaLengths(deltas);
                if (!status.isGood()) { return status; }
            }
            m_Xcalculated.setZero(); // reset calculated motion
            m_panelsToMove.clear();
            m_previousCalculatedMethod = 0;
        }
    }
    return status;
}

// Align all edges between start_idx and end_idx moving in the direction dir
UaStatus MirrorController::alignSequential(unsigned startEdge, const std::set<unsigned> &selectedEdges, bool dir)
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

    std::deque<unsigned> already_aligned{}; // yes, deque, not vector!

    unsigned cur_idx = startEdge;
    while (selectedEdges.find(cur_idx) != selectedEdges.end() && (m_state == Device::DeviceState::On)) {
        already_aligned.push_front(cur_idx);
        // align all the preceding panels
        for (unsigned edge : already_aligned) {
            if (m_state != Device::DeviceState::On) break;
            // figure out which panel is "greater" and which one is "smaller" in the sense
            // of dir, assuming a two-panel edge for now.
            // get vector of panel positions:
            auto curPanels = SCTMath::GetPanelsFromEdge(m_pChildren.at(PAS_EdgeType).at(edge)->getId().eAddress, dir);

            // align this edge moving the "smaller" panel
            UaVariantArray args; 
            args.create(4);
            args[0].Value.UInt32 = curPanels.at(0);
            args[1].Value.UInt32 = curPanels.at(1);
            args[2].Value.Double = 1.0;
            args[3].Value.Boolean = false;
            auto movingPanel_idx = m_ChildrenPositionMap.at(PAS_PanelType).at(curPanels.at(0));
            // do this until the edge is aligned
            int aligniter = 1;
            status = m_pChildren.at(PAS_EdgeType).at(edge)->operate(PAS_EdgeType_Align, args);
            if (!status.isGood()) { 
                std::cout << "Error: Failed when calculating motion." << std::endl;
                return status; 
            }
            args[3].Value.Boolean = true;
            status = m_pChildren.at(PAS_EdgeType).at(edge)->operate(PAS_EdgeType_Align, args);
            if (!status.isGood()) { 
                std::cout << "Error: Failed when executing motion." << std::endl;
                return status; 
            }
            while (!std::dynamic_pointer_cast<EdgeController>(m_pChildren.at(PAS_EdgeType).at(edge))->isAligned()) {
                std::cout << "\nAlignment Iteration " << aligniter << std::endl << std::endl;
                usleep(400*1000); // microseconds

                Device::DeviceState curstate;
                m_pChildren.at(PAS_PanelType).at(movingPanel_idx)->getState(curstate);
                while (curstate == Device::DeviceState::Busy) {
                    std::cout << "\tPanel " << curPanels.at(0) << " moving..." << std::endl;
                    usleep(200*1000); // microseconds
                    m_pChildren.at(PAS_PanelType).at(movingPanel_idx)->getState(curstate);
                }
                aligniter++;
                args[3].Value.Boolean = false;
                status = m_pChildren.at(PAS_EdgeType).at(edge)->operate(PAS_EdgeType_Align, args);
                if (!status.isGood()) { 
                    std::cout << "Error: Failed when calculating motion." << std::endl;
                    return status; 
                }
                args[3].Value.Boolean = true;
                status = m_pChildren.at(PAS_EdgeType).at(edge)->operate(PAS_EdgeType_Align, args);
                if (!status.isGood()) { 
                    std::cout << "Error: Failed when executing motion." << std::endl;
                    return status; 
                }
            }
            std::cout << "\n" << m_pChildren.at(PAS_EdgeType).at(edge)->getId().name
                      << " is aligned!" << std::endl;
        }

        // get the next edge
        Device::Identity id;
        id.eAddress = SCTMath::GetEdgeNeighbor(m_pChildren.at(PAS_EdgeType).at(cur_idx)->getId().eAddress, dir);
        try {
            cur_idx = m_ChildrenIdentityMap.at(PAS_EdgeType).at(id);
        }
        catch (std::out_of_range &e) {
            cur_idx = -1; // max element of unsigned
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
        std::cout << std::endl << m_ID.name << " selected the following panels (positions):\n\t";
        for (const std::string &item : strList) {
            curpos = stoi(item);
            if (m_ChildrenPositionMap.at(deviceType).count(curpos) > 0) {
                m_selectedPanels.insert(curpos);
                std::cout << "Added Panel with serial: " << curpos << std::endl;
            }
            else {
                std::cout << "Unable to find Panel with serial: " << curpos << std::endl;
            }
        }
        std::cout << std::endl;
    }
    else if (deviceType == PAS_MPESType) { // expect a list of Sensor serials
        m_selectedMPES.clear();
        Device::Identity curid;
        std::cout << std::endl << m_ID.name << " selected the following MPES (serials):\n\t";
        for (const std::string &item : strList) {
            curid.serialNumber = stoi(item);
            if (m_ChildrenIdentityMap.at(deviceType).count(curid) > 0) {
                m_selectedMPES.insert(curid.serialNumber);
                std::cout << "Added MPES with serial: " << curid.serialNumber << std::endl;
            }
            else {
                std::cout << "Unable to find MPES with serial: " << curid.serialNumber << std::endl;
            }
        }
        std::cout << std::endl;
    }

    else if (deviceType == PAS_EdgeType) { // expect a list of edge addresses
        m_selectedEdges.clear();
        Device::Identity curid;
        std::cout << std::endl << m_ID.name << " selected the following edges (eAddresses):\n\t";
        for (const std::string &item : strList) {
            curid.eAddress = item;
            if (m_ChildrenIdentityMap.at(deviceType).count(curid) > 0) {
                m_selectedEdges.insert(curid.eAddress);
                std::cout << "Added Edge with eAddress: " << curid.eAddress << std::endl;
            }
            else {
                std::cout << "Unable to find Edge with eAddress: " << curid.eAddress << std::endl;
            }
            std::cout << std::endl;
        }
    }
}

std::set<unsigned> MirrorController::getSelectedDeviceIndices(unsigned deviceType) {

    std::set<unsigned> devices;

    // add all the items to the selected children set of indices
    if (deviceType == PAS_PanelType) { // expect a list of panel positions
        for (auto &pos : m_selectedPanels) {
            devices.insert(m_ChildrenPositionMap.at(deviceType).at(pos));
        }
    } else if (deviceType == PAS_MPESType) { // expect a list of Sensor serials
        Device::Identity curid;
        for (auto &serial : m_selectedMPES) {
            curid.serialNumber = serial;
            devices.insert(m_ChildrenIdentityMap.at(deviceType).at(curid));
        }
    } else if (deviceType == PAS_EdgeType) { // expect a list of edge addresses
        Device::Identity curid;
        for (auto &eAddress : m_selectedEdges) {
            curid.eAddress = eAddress;
            devices.insert(m_ChildrenIdentityMap.at(deviceType).at(curid));
        }
    }

    return devices;
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
        std::cout << "\n" << m_ID.name << " coordinates:\n";
        for (int i = 0; i < 6; i++)
            std::cout << m_curCoords(i) << " +/- " << m_curCoordsErr(i) << std::endl;
        // display angle in common coordinates -- angle*baseRad
    //    for (int i = 3; i < 6; i++)
    //       std::cout << m_curCoords(i)*kBaseRadius << " +/- " << m_curCoordsErr(i)*kBaseRadius << std::endl;
    }
    m_lastUpdateTime = TIME::now();

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

    m_SP.ComputeActsFromPads(PadCoords);
    for (unsigned act = 0; act < 6; act++)
        Acts[act] = m_SP.GetActLengths()[act];
    m_SP.ComputeStewart(Acts);
    for (unsigned coord = 0; coord < 6; coord++)
        orig_panelCoords2(coord) = m_SP.GetPanelCoords()[coord];

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
        m_SP.ComputeActsFromPads(PadCoords);
        for (unsigned act = 0; act < 6; act++)
            Acts[act] = m_SP.GetActLengths()[act];
        // phew done!
        // compute new panel coordinates based on these actuator lengths
        m_SP.ComputeStewart(Acts);

        for (unsigned coord = 0; coord < 6; coord++)
            panelCoords2(coord) = m_SP.GetPanelCoords()[coord];

        res.col(TR) = (panelCoords2 - orig_panelCoords2);
    }

    return res;
}

UaStatus MirrorController::alignSector(const std::set<unsigned> &selectedPanels, const std::set<unsigned> &selectedMPES,
                              double alignFrac, bool execute) {

    UaStatus status;
 
    if (!execute) {   
        // following the align method for an edge:
        Eigen::MatrixXd C; // constraint
        Eigen::MatrixXd B; // complete matrix

        Eigen::VectorXd X; // solutions vector -- this moves actuators
        Eigen::VectorXd Y; // sensor misalignment vector, we want to fit this

        // grab all user specified panels to move and sensors to fit
        std::vector<std::shared_ptr<PanelController>> panelsToMove;
        std::vector<std::shared_ptr<MPESController>> alignMPES;
        for (unsigned idx : getSelectedDeviceIndices(PAS_PanelType))
            panelsToMove.push_back(std::dynamic_pointer_cast<PanelController>(m_pChildren.at(PAS_PanelType).at(idx)));
        for (unsigned idx : getSelectedDeviceIndices(PAS_MPESType)) {
            std::shared_ptr<MPESController> mpes = std::dynamic_pointer_cast<MPESController>(
                m_pChildren.at(PAS_MPESType).at(idx));
            mpes->read(false);
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
                        std::dynamic_pointer_cast<MPESController>(mpes)->getPanelSide(overlapPanel->getId().position) !=
                        0);

                if (overlap == 2) {
                    idx = m_ChildrenIdentityMap.at(PAS_MPESType).at(mpes->getId());
                    if (!overlapIndices.count(idx)) {
                        overlapIndices.insert(idx);
                        if (count(alignMPES.begin(), alignMPES.end(), mpes)) {
                            std::cout << "You specified the following internal MPES: " << mpes->getId().serialNumber
                                      << std::endl;
                            userOverlap = true;
                        }
                    }
                }
            }
        }

        // if no user specified overlapping sensors, get their readings
        if (!userOverlap && !overlapIndices.empty()) {
            std::cout << "\nNo user-speficied internal MPES." << std::endl;
            std::cout << "Reading the automatically identfied internal MPES:" << std::endl;
            // only read the internal MPES if no user-specified ones have been found
            for (const auto &idx: overlapIndices) {
                std::shared_ptr<MPESController> mpes = std::dynamic_pointer_cast<MPESController>(
                    m_pChildren.at(PAS_MPESType).at(idx));
                mpes->read(false);
                if (mpes->isVisible())
                    alignMPES.push_back(mpes);
            }
        } else if (overlapIndices.empty())
            std::cout << "\nIdentified NO internal MPES. Are you alignining a single panel?";
        else
            std::cout << "\nWill be using user-specified internal MPES.";
        std::cout << std::endl;


        std::cout << "Will align the following panels:" << std::endl;
        for (auto &panel : panelsToMove)
            std::cout << panel->getId().position << " ";
        std::cout << std::endl;
        std::cout << "Will use the following sensors for alignment:" << std::endl;
        for (auto &mpes: alignMPES)
            std::cout << mpes->getId().serialNumber << " ";
        std::cout << std::endl;

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
                auto panelSide = alignMPES.at(m)->getPanelSide(panelsToMove.at(p)->getId().position);
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
            std::cout << "+++ ERROR! +++ There are " << B.rows() / 2 << " sensors and " << B.cols()
                      << " actuators -- not enough sensors to constrain the motion. Won't do anything!" << std::endl;
            return OpcUa_Bad;
        }

        try {
            X = B.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(Y);
        }
        catch (...) {
            std::cout << "+++ WARNING! +++ Failed to perform Singular Value Decomposition. "
                      << "Check your sensor readings! Discarding this result!" << std::endl;
            return OpcUa_Bad;
        }

        std::cout << "\nThe vector to solve for is\n" << Y << std::endl;
        std::cout << "\nThe matrix to solve with is\n" << B << std::endl;
        std::cout << "\nThe least squares solution is\n" << X << std::endl;

        if (alignFrac < 1.) {
            std::cout << "+++ WARNING +++ You requested fractional motion: will move fractionally by " << alignFrac
                      << " of the above:" << std::endl;
            X *= alignFrac;
            std::cout << X << std::endl;
        }

        unsigned j = 0;
        Eigen::VectorXd deltaLengths(6);
        for (auto &pCurPanel : panelsToMove) {
            // print out to make sure
            std::cout << "Will move actuators of Panel "
                      << pCurPanel->getId().position << " by\n" << X.segment(j, 6) << std::endl;

            for (unsigned i = 0; i < 6; i++) {
                deltaLengths(i) = X(j++);
            }

            if(pCurPanel->checkForCollision(deltaLengths)) {
                std::cout << "Error: Sensors may go out of range! Disallowed motion, please recalculate or relax safety radius." << std::endl;
                return OpcUa_Bad;
            }
        }
        m_Xcalculated = X; // set calculated motion
        m_panelsToMove = panelsToMove;
        m_previousCalculatedMethod = PAS_MirrorType_AlignSector;
    }
    else {
        if (m_Xcalculated.isZero(0) || m_previousCalculatedMethod != PAS_MirrorType_AlignSector) {
            std::cout << "No calculated motion found.  Call Mirror.alignSector with execute=false once first to calculate the motion to execute." << std::endl;
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
                status = pCurPanel->moveDeltaLengths(deltas);
                if (!status.isGood()) { return status; }
            }
            m_Xcalculated.setZero(); // reset calculated motion
            m_panelsToMove.clear();
            m_previousCalculatedMethod = 0;
        }
    }
    return status;
}


UaStatus MirrorController::alignGlobal(unsigned fixPanel, double alignFrac, bool execute)
{
    UaStatus status;
    if (!execute) {
        // we want to fit all sensors simultaneously while constraining the motion of 'fixPanel'
        // first, find which ring we are on:
        unsigned ring = SCTMath::Ring(fixPanel);
        unsigned mirror = SCTMath::Mirror(fixPanel);
        
        unsigned numPanels = 0;
        if (mirror != (unsigned) m_ID.position) {
            std::cout << "+++ ERROR +++ The entered fixPanel position is wrong! Check it and try again."
                      << std::endl;
            return OpcUa_Bad;
        }

        if (mirror == 1)
            numPanels = SCT::Primary::kPanels[ring - 1];
        else if (mirror == 2)
            numPanels = SCT::Secondary::kPanels[ring - 1];

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
        unsigned curPanel = fixPanel, nextPanel, cursize = 0;
        Device::Identity id;
        // keep track of the position in the global response matrix
        unsigned blockRow = 0;
        std::cout << "+++ DEBUG +++ Traversing Ring " << ring << " of Mirror " << mirror << " clockwise\n"
                  << std::endl;

        Eigen::MatrixXd T; // Vladimir's T operator
        Eigen::MatrixXd Eye = Eigen::MatrixXd::Identity(6, 6);
        Eigen::MatrixXd MCurPrev, MCurNext, MNextCur;
        Eigen::MatrixXd E;
        T = Eye;

        do {
            nextPanel = SCTMath::GetPanelNeighbor(curPanel, 0);
            std::cout << "+++ DEBUG +++ Currently on panels (" << curPanel << ", " << nextPanel << ")"
                      << std::endl;

            try {
                panelsToMove.push_back(std::dynamic_pointer_cast<PanelController>(
                        m_pChildren.at(PAS_PanelType).at(m_ChildrenPositionMap.at(PAS_PanelType).at(curPanel)) ) );

                id.eAddress = SCTMath::GetEdgeFromPanels({curPanel, nextPanel});
                edgesToFit.push_back(std::dynamic_pointer_cast<EdgeController>(
                            m_pChildren.at(PAS_EdgeType).at(m_ChildrenIdentityMap.at(PAS_EdgeType).at(id)) ) );
                std::cout << "\t\tThese correspond to the edge " << edgesToFit.back()->getId() << std::endl;

                std::cout << "\t\tReading the sensors of this edge." << std::endl;
                localCurRead = edgesToFit.back()->getCurrentReadings();
                localAlignRead = edgesToFit.back()->getAlignedReadings();

                misalignVec = localAlignRead - localCurRead;
                std::cout << "+++ DEBUG +++ The current misalignment is\n" << misalignVec << std::endl;
                std::cout << "+++ DEBUG +++ Adding this to the the end of the global misalignment vector" << std::endl;
                globMisalignVec.conservativeResize(cursize + misalignVec.size());
                globMisalignVec.tail(misalignVec.size()) = misalignVec;
                cursize = globMisalignVec.size();


                // get the response of this edge
                localResponse = edgesToFit.back()->getResponseMatrix(curPanel);
                std::cout << "\t\tResponse matrix [" << curPanel << "][" << nextPanel << "]:\n"
                          << localResponse << std::endl;
                std::cout << "+++ DEBUG +++ Placing this at location [" << blockRow << "x6]["
                          << blockRow << "x6] of the global response matrix" << std::endl;
                globResponse.block(blockRow*6, blockRow*6,
                        localResponse.rows(), localResponse.cols()) = localResponse;
                MCurNext = localResponse;

                localResponse = edgesToFit.back()->getResponseMatrix(nextPanel);
                std::cout << "\t\tResponse matrix [" << nextPanel << "][" << curPanel << "]:\n"
                          << localResponse << std::endl;
                std::cout << "+++ DEBUG +++ Placing this at location [" << blockRow << "x6]["
                          << (blockRow + 1) % numPanels << "x6] of the global response matrix" << std::endl;
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
                E = MCurNext*MCurPrev.inverse();
                std::cout << "+++ SPECIAL DEBUG +++ ML = \n" << MCurPrev << std::endl;
                std::cout << "+++ SPECIAL DEBUG +++ ML*ML.invserse() = \n" << MCurPrev * MCurPrev.inverse() << std::endl;
                std::cout << "+++ SPECIAL DEBUG +++ MR = \n" << MCurNext << std::endl;
                std::cout << "+++ SPECIAL DEBUG +++ Operator E = MR*ML.inverse() \n" << E << std::endl;
                T = Eye - E*T;
            }

            MCurPrev = MNextCur;

        } while ( curPanel != fixPanel );

        std::cout << "+++ SPECIAL DEBUG +++ Operator T = \n" << T << std::endl;
        std::cout << "+++ SPECIAL DEBUG +++ Operator -1*T.inverse() = \n" << -1 * T.inverse() << std::endl;
        std::cout << "+++ SPECIAL DEBUG +++ Operator T*T.inverse() = \n" << T * T.inverse() << std::endl;
        std::cout << "+++ SPECIAL DEBUG +++ You want to compute -1*T.inverse*misalignmentOfLastEdge..." << std::endl;

        // check that we have enough sensor readings
        if (globMisalignVec.size() < globResponse.rows()) {
            std::cout
                    << "+++ ERROR +++ Not enough sensor readings to perform the fit! Go through the output above and find which sensor is not in the field of view, fix it, and come back."
                    << std::endl;
            return OpcUa_Bad;
        }

        std::cout << "+++ DEBUG +++ misalignment vector size is " << globMisalignVec.size() << std::endl;
        std::cout << "+++ DEBUG +++ The first 12 entries:\n" << globMisalignVec.head(12) << std::endl;
        std::cout << "+++ DEBUG +++ The last 12 entries:\n" << globMisalignVec.tail(12) << std::endl;
        std::cout << "+++ DEBUG +++ The first 12x18 block of the global response matrix:\n"
                  << globResponse.block(0, 0, 12, 18) << std::endl;
        std::cout << "+++ DEBUG +++ The last 12x18 block of the global response matrix:\n"
                  << globResponse.block(globResponse.rows() - 12, globResponse.cols() - 18, 12, 18) << std::endl;

        // erase the first element of panelsToMove -- this is the panel we decided to keep fixed
        panelsToMove.erase(panelsToMove.begin());

        // we are all set now. now solve the system
        // globResponse * globDisplaceVec = globMisalignVec;
        // the first 6 elements of the solution are the systematic vectors, the others are the
        // panel displacements
        globDisplaceVec = globResponse.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(globMisalignVec);

        Eigen::VectorXd check = globResponse * globDisplaceVec - globMisalignVec;
        std::cout
                << "+++ DEBUG +++ Checking that the found solution is correct: looking at (R*Solution - MisAlign): norm = "
                << check.norm() << std::endl;
        std::cout << "\t\t First 12 entries\n" << check.head(12) << std::endl;
        std::cout << "\t\t Last 12 entries\n" << check.tail(12) << std::endl;

        m_sysOffsetsMPES = globDisplaceVec.head(6);
        // get the order of read sensors:
        unsigned pos = 0;
        for (const auto& mpes : edgesToFit.back()->m_ChildrenPositionMap.at(PAS_MPESType)) {
            SystematicOffsetsMPESMap[ring][mpes.first] = m_sysOffsetsMPES.segment(pos*2, 2);
            ++pos;
        }
        // make sure we read out the systematic vector in the same order as the sensors
        // and assign it correctly to outer/middle/inner

        std::cout << "The fitted systematic offset is, in the order of read sensors:\n"
                  << m_sysOffsetsMPES << std::endl;

        // set the offset for each sensor edge
        for (const auto& edge : edgesToFit) {
            // and set them for each sensor along each edge
            for (const auto& mpes : edge->m_ChildrenPositionMap.at(PAS_MPESType) ) {
                (std::dynamic_pointer_cast<MPESController>(
                    edge->m_pChildren.at(PAS_MPESType).at(mpes.second)))->SystematicOffsets =
                    SystematicOffsetsMPESMap.at(ring).at(mpes.first);
            }
        }

        if (alignFrac < 1.)
            std::cout << "\n+++ WARNING +++ You requested fractional motion: will move fractionally by "
                      << alignFrac << " of the computed displacement" << std::endl;

        /* MOVE ACTUATORS */
        unsigned j = 6;
        Eigen::VectorXd deltaLengths(6);
        for (auto &pCurPanel : panelsToMove) {
            // print out to make sure
            std::cout << "Will move actuators of "
                      << pCurPanel->getId().name << " by (accounting for fractional motion)\n"
                      << alignFrac * globDisplaceVec.segment(j, 6) << std::endl;

            for (unsigned i = 0; i < 6; i++) {
                deltaLengths(i) = alignFrac * globDisplaceVec(j++);
            }

            if(pCurPanel->checkForCollision(deltaLengths)) {
                std::cout << "Error: Sensors may go out of range! Disallowed motion, please recalculate or relax safety radius." << std::endl;
                return OpcUa_Bad;
            }
        }
        m_Xcalculated = globDisplaceVec.tail(globDisplaceVec.size()-6);
        m_panelsToMove = panelsToMove;
        m_previousCalculatedMethod = PAS_MirrorType_GlobalAlign;
    }
    else {
        if (m_Xcalculated.isZero(0) || m_previousCalculatedMethod != PAS_MirrorType_GlobalAlign) {
            std::cout << "No calculated motion found.  Call Mirror.GlobalAlign with execute=false once first to calculate the motion to execute." << std::endl;
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
                status = pCurPanel->moveDeltaLengths(deltas);
                if (!status.isGood()) { return status; }
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

    Eigen::Vector3d out_vec = std::move(in_vec);
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
    for (auto idx : getSelectedDeviceIndices(PAS_PanelType)) {
        int pos = panels.at(idx)->getId().position;
        int ring = SCTMath::Ring(pos);

        // go over all pads -- ChiSq is the squared difference between pad coordinates
        // as computed from actuator lengths and pad coordinates as computed from telescope
        // coordinates
        for (int pad = 0; pad < 3; pad++) {
            padCoordsActs = std::dynamic_pointer_cast<PanelController>(panels.at(idx))->getPadCoords().col(
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
