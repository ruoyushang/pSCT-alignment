#include "pasmirror.h"
#include "mathtools.h"
#include "mirrordefinitions.h" // definitions of the mirror surfaces
#include "AGeoAsphericDisk.h" // ROBAST dependency
#include <algorithm> // std::count
#include <string>
#include <cmath>
#include <deque>
#include "TMinuit.h" // ROOT's implementation of MINUIT for chiSq minimization

// implementation of the mirror class that controls the whole mirror.
// math, algorithms and all that coordinate stuff go here

using namespace std;
using namespace Eigen;
using namespace SCT; // mirrordefinitions.h

PasMirror *PasMirrorCompute::Mirror = nullptr;

PasMirror::PasMirror(Identity identity) : PasCompositeController(identity, nullptr),
    m_pSurface(nullptr), m_AlignFrac(0.25)
{
    string mirrorprefix;
    if (m_ID.position == 1)
        mirrorprefix = "Primary";
    else if (m_ID.position == 2)
        mirrorprefix = "Secondary";
    else
        mirrorprefix = "Unknown";
    m_ID.name = mirrorprefix + "Mirror";

    m_state = PASState::PAS_On;

    // define possible children and initialize the selected children string
    m_ChildrenTypes = {PAS_PanelType, PAS_EdgeType, PAS_MPESType};

    // define coordinate vectors -- these are of size 6
    m_inCoords = VectorXd(6);
    m_curCoords = VectorXd(6);
    m_curCoordsErr = VectorXd(6);
    m_sysOffsetsMPES = VectorXd(6);
    VectorXd tmp = VectorXd(2); tmp.setZero();
    // initialize the systematic offsets map to zeros
    SystematicOffsetsMPESMap = {
        {1, {
            {1, tmp}, {2, tmp}, {3, tmp}, {4, tmp}, {5, tmp}
            }},
        {2, {
            {1, tmp}, {2, tmp}, {3, tmp}, {4, tmp}
            }}
    };
}

void PasMirror::addChild(OpcUa_UInt32 deviceType, PasController *const pController)
{
    // call the base type's method
    PasCompositeController::addChild(deviceType, pController);

    // add this to selected children
    if (deviceType == PAS_PanelType)
        m_SelectedChildrenString[deviceType] += to_string(pController->getId().position) + " ";
    else if (deviceType == PAS_EdgeType)
        m_SelectedChildrenString[deviceType] += pController->getId().eAddress + " ";

    // update the selected children if they are not sensors
    if (deviceType != PAS_MPESType)
        __updateSelectedChildren(deviceType);
}


bool PasMirror::Initialize()
{
    cout << "\n\tInitializing " << m_ID.name << "..." << endl;

    // precompute everything we need
    if (m_ID.position == 1) {
        // bottom (w.r.t. z) surface first, then top surface --
        // back surface first, then optical surface
        m_pSurface = new AGeoAsphericDisk(m_ID.name.c_str(),
           Primary::kZ + Primary::kz[0] - Primary::kMirrorThickness, 0,
           Primary::kZ + Primary::kz[0], 0,
           Primary::kD/2., 0);
        m_pSurface->SetPolynomials(kNPar - 1, &Primary::kz[1],
                kNPar - 1, &Primary::kz[1]);
        m_SurfaceNorm = 1.; // along z

        // update ideal panel properties
        for (int ring : {0, 1}) {
            // ideal panel width and offset of x1x1 from it
            m_PanelWidth[ring + 1] = 2.*M_PI / Primary::kPanels[ring];

            for (int coord = 0; coord < 3; coord++)
                for (int pad = 0; pad < 3; pad++)
                    m_PadCoordsTelFrame[ring + 1].col(pad)(coord) = Primary::PadsCoords[ring][pad][coord];

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
           Secondary::kZ + Secondary::kz[0] - Secondary::kMirrorThickness, 0,
           Secondary::kZ + Secondary::kz[0], 0,
           Secondary::kD/2., 0);
        m_pSurface->SetPolynomials(kNPar - 1, &Secondary::kz[1],
                kNPar - 1, &Secondary::kz[1]);
        m_SurfaceNorm = 1.; // parallel to z in its frame

        // update ideal panel properties
        for (int ring : {0, 1}) {
            // ideal panel width and offset of x1x1 from it
            m_PanelWidth[ring + 1] = 2.*M_PI / Secondary::kPanels[ring];

            // update pad coords and panel origin
            for (int coord = 0; coord < 3; coord++)
                for (int pad = 0; pad < 3; pad++)
                    m_PadCoordsTelFrame[ring + 1].col(pad)(coord) = Secondary::PadsCoords[ring][pad][coord];

            m_PanelOriginTelFrame[ring + 1].setZero();
            // this is the center of the mirror panel in the TRF
            for (int pad = 0; pad < 3; pad++)
                m_PanelOriginTelFrame.at(ring + 1) += m_PadCoordsTelFrame.at(ring + 1).col(pad) / 3.;
        }
    }
    else {
        cout << "\tNo mirror at position " << m_ID.position << " -- nothing to do!" << endl;
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
            m_PanelFrame[ring + 1].col(2) = Vector3d(norm);
            // make sure this is pointing where we need it to
            if ( m_PanelFrame.at(ring + 1).col(2).dot(Vector3d(dir)) <   0. )
                m_PanelFrame.at(ring + 1).col(2) *= -1;

            // get the x and y axes:
            for (int i = 0; i < 2; i++) {
                Vector3d axis {0., 0., 0};
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
            for (int i = 0; i < 6; i++)
                actL[i] = kActuatorLength;
            m_SP.ComputeStewart(actL);
            Vector3d PanelCenterPanelFrame(m_SP.GetPanelCoords());
            // this is the center of the mirror panel in the panel frame;
            // what we obtained before was the center of the mirror panel in the TRF --
            // need to shift those coordinates by this much opposite to the mirror norm
            // to get the origin of the panel frame in the TRF:
            m_PanelOriginTelFrame.at(ring + 1) -= m_PanelFrame.at(ring+1)*PanelCenterPanelFrame;

            cout << "Ideal Panel Frame origin for Ring " << ring + 1 << " in the Telescope frame :"
                << endl << m_PanelOriginTelFrame.at(ring + 1) << endl;
            cout << "Ideal Panel Frame for Ring " << ring + 1 << " in the Telescope frame :"
                << endl << m_PanelFrame.at(ring + 1) << endl << endl;
        }
    }

//    cout << "SYSTEMATICS MATRIX FOR 2112 from 2111:\n" << __computeSystematicsMatrix(2111, 2112) << endl;
//    cout << "SYSTEMATICS MATRIX FOR 2111 from 2112:\n" << __computeSystematicsMatrix(2112, 2111) << endl;

    cout << "Done Initializing " << m_ID.name << endl << endl;

    m_SelectedChildrenString[PAS_MPESType] = "";

    return true;
}


PasMirror::~PasMirror()
{
    m_pClient = nullptr; // this shouldn't have been changed anyway, but just in case

    if (m_pSurface)
        delete m_pSurface;
    m_pSurface = nullptr;

    for (auto &devVector : m_pChildren)
        for (auto &dev : devVector.second)
            dev = nullptr;

    m_state = PASState::PAS_Off;
}

UaStatusCode PasMirror::getState(PASState& state)
{
    UaMutexLocker lock(&m_mutex);
    state = m_state;
    return OpcUa_Good;
}

UaStatusCode PasMirror::setState(PASState state)
{
    UaMutexLocker lock(&m_mutex);
    m_state = state;
    return OpcUa_Good;
}

UaStatusCode PasMirror::getData(OpcUa_UInt32 offset, UaVariant& value)
{
    UaMutexLocker lock(&m_mutex);

    if (offset >= PAS_MirrorType_curCoords_x && offset <= PAS_MirrorType_curCoords_zRot) {
        // update current coordinates
        __updateCoords();
        int dataoffset = offset - PAS_MirrorType_curCoords_x;
        value.setDouble(m_curCoords(dataoffset));
    }
    else if (offset >= PAS_MirrorType_inCoords_x && offset <= PAS_MirrorType_inCoords_zRot) {
        int dataoffset = offset - PAS_MirrorType_inCoords_x;
        value.setDouble(m_inCoords(dataoffset));
    }
    else if (offset >= PAS_MirrorType_sysOffsetsMPES_x1 && offset <= PAS_MirrorType_sysOffsetsMPES_y3) {
        int dataoffset = offset - PAS_MirrorType_sysOffsetsMPES_x1;
        value.setDouble(m_sysOffsetsMPES(dataoffset));
    }
    else if (offset == PAS_EdgeType_AlignFrac)
        value.setFloat(m_AlignFrac);
    else if (offset == PAS_MirrorType_selectedPanels)
        value.setString(m_SelectedChildrenString.at(PAS_PanelType).c_str());
    else if (offset == PAS_MirrorType_selectedMPES)
        value.setString(m_SelectedChildrenString.at(PAS_MPESType).c_str());
    else if (offset == PAS_MirrorType_selectedEdges)
        value.setString(m_SelectedChildrenString.at(PAS_EdgeType).c_str());
    else
       return OpcUa_BadInvalidArgument;

    return OpcUa_Good;
}

UaStatusCode PasMirror::setData(OpcUa_UInt32 offset, UaVariant value)
{
    UaMutexLocker lock(&m_mutex);

    if (offset >= PAS_MirrorType_inCoords_x && offset <= PAS_MirrorType_inCoords_zRot) {
        int dataoffset = offset - PAS_MirrorType_inCoords_x;
        value.toDouble(m_inCoords(dataoffset));
    }
    else if (offset >= PAS_MirrorType_sysOffsetsMPES_x1 && offset <= PAS_MirrorType_sysOffsetsMPES_y3) {
        int dataoffset = offset - PAS_MirrorType_sysOffsetsMPES_x1;
        value.toDouble(m_sysOffsetsMPES(dataoffset));
    }
    else if (offset == PAS_EdgeType_AlignFrac)
        value.toFloat(m_AlignFrac);
    else if (offset == PAS_MirrorType_selectedPanels) {
        string tmp = value.toString().toUtf8();
        m_SelectedChildrenString[PAS_PanelType] = tmp;
        __updateSelectedChildren(PAS_PanelType);
    }
    else if (offset == PAS_MirrorType_selectedMPES) {
        string tmp = value.toString().toUtf8();
        m_SelectedChildrenString[PAS_MPESType] = tmp;
        __updateSelectedChildren(PAS_MPESType);
    }
    else if (offset == PAS_MirrorType_selectedEdges) {
        string tmp = value.toString().toUtf8();
        m_SelectedChildrenString[PAS_EdgeType] = tmp;
        __updateSelectedChildren(PAS_EdgeType);
    }
    else
       return OpcUa_BadInvalidArgument;


    return OpcUa_Good;
}

UaStatusCode PasMirror::Operate(OpcUa_UInt32 offset, const UaVariantArray &args)
{
    UaMutexLocker lock(&m_mutex);

    /**********************************************************
     * Move the whole mirror in the telescope reference frame *
     * ********************************************************/
    if (offset == PAS_MirrorType_MoveTo_Coords)
        __move();

    /**********************************************************
     * Move the selected sector                                *
     * ********************************************************/
    if (offset == PAS_MirrorType_MoveSector) {
        for (unsigned idx : m_SelectedChildren.at(PAS_PanelType))
             m_pChildren.at(PAS_PanelType).at(idx)->Operate(PAS_PanelType_MoveTo_Acts);
    }

    /**********************************************************
     * Read out all current panel positions in their frames   *
     * ********************************************************/
    else if (offset == PAS_MirrorType_ReadPos) {
        // read out all individual positions
        __readPositionAll();
        // and get global mirror coordinates
        __updateCoords();
    }

    /**********************************************************
     * Align the selected sector to selected sensors          *
     * ********************************************************/
    else if (offset == PAS_MirrorType_AlignSector) {
        // make sure there are some selected sensors
        if (m_SelectedChildren.at(PAS_MPESType).size() < 1) {
            cout << "+++ ERROR +++ No sensors selected! Nothing to do." << endl;
            return OpcUa_BadInvalidArgument;
        }

        __alignSector();
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
        catch (out_of_range) {
            cout << "+++ ERROR +++ The selected panel is not connected! Nothing to do." << endl;
            return OpcUa_BadInvalidArgument;
        }

        __alignGlobal(fixPanel);
    }


    /**********************************************************
     * Align all edges of the mirror                          *
     * ********************************************************/
    else if (offset == PAS_MirrorType_Align) {
        // make sure the arguments make sense -- we are supposed
        // to get an edge position and a direction. The type has
        // already been validated by the caller, but we need to
        // check the existence of such edge and direction:
        Identity startId;
        // first argument is the edge address
        startId.eAddress = UaString(args[0].Value.String).toUtf8();
        // second argument is the direction
        unsigned dir = args[1].Value.UInt32;
        // check that these are valid;
        if (dir != 0 && dir != 1)
            return OpcUa_BadInvalidArgument;
        unsigned idx;
        try {
            idx = m_ChildrenIdentityMap.at(PAS_EdgeType).at(startId);
        }
        catch (out_of_range) {
            return OpcUa_BadInvalidArgument;
        }

        __alignAll(idx, m_SelectedChildren.at(PAS_EdgeType), (bool)dir);
    }

    /**********************************************************
     * Read out all current edge sensors                      *
     * ********************************************************/
    else if (offset == PAS_MirrorType_ReadAlign)
        __readAlignmentAll();

    /************************************************
     * stop the motion in progress                  *
     * **********************************************/
    else if (offset == PAS_MirrorType_Stop)
        __stopAll();

    else
        return OpcUa_BadNotImplemented;

    return OpcUa_Good;
}


/* ================= INTERNAL IMPLEMENTATIONS OF ALL METHODS ================== */

void PasMirror::__stopAll()
{
    auto type = PAS_PanelType;
    cout << m_ID.name << "::Operate() : Attempting to gracefully stop all motions." << endl;
    for (const auto& idx: m_SelectedChildren.at(type))
        m_pChildren.at(type).at(idx)->Operate(PAS_PanelType_Stop);
}

void PasMirror::__readAlignmentAll()
{
    auto type = PAS_EdgeType;
    for (const auto& idx : m_SelectedChildren.at(type))
        m_pChildren.at(type).at(idx)->Operate(PAS_EdgeType_Read);
}


void PasMirror::__readPositionAll()
{
    auto type = PAS_PanelType;
    for (const auto& idx : m_SelectedChildren.at(type)) {
        m_pChildren.at(type).at(idx)->Operate(PAS_PanelType_Read);

        auto pos = m_pChildren.at(type).at(idx)->getId().position;

        auto padCoordsActs = static_cast<PasPanel *>(m_pChildren.at(type).at(idx))->getPadCoords();
        cout << "Panel frame pad coordinates:\n" << padCoordsActs << endl;
        // and transform this to the telescope reference frame:
        // these are pad coordinates in TRF as computed from actuator lengths
        for (int i = 0; i < padCoordsActs.cols(); i++)
            padCoordsActs.col(i) = __toTelRF(pos, padCoordsActs.col(i));
        cout << "Telescope frame pad coordinates:\n" << padCoordsActs << endl;
    }
}

void PasMirror::__move()
{

    cout << "\t\t*** MOVING MIRROR " << m_ID.position << " BY THE FOLLOWING AMOUNT :\n"
        << m_inCoords << endl;

    PasController *pCurObject;

    auto type = PAS_PanelType;
    unsigned curpos;
    VectorXd prf_coords;
    const auto& childrenSet = m_SelectedChildren.at(type);
    for (const auto& idx : childrenSet) {
        pCurObject = m_pChildren.at(type).at(idx);
        curpos = pCurObject->getId().position;
        cout << "Panel " << curpos << ":" << endl;
        static_cast<PasPanel *>(pCurObject)->Operate(PAS_PanelType_Read);
        // for this panel, we get PRF pad coords, transform them to TRF,
        // move them in TRF, transform back to PRF, and then compute new ACT lengths
        // based on the new pad coords. so simple!
        auto padCoords_PanelRF = static_cast<PasPanel *>(pCurObject)->getPadCoords();
        auto padCoords_TelRF = padCoords_PanelRF;
        double newPadCoords[3][3];
        for (unsigned pad = 0; pad < 3; pad++) {
            // transform to TRF
            padCoords_TelRF.col(pad) = __toTelRF(curpos, padCoords_PanelRF.col(pad));
            // move by the desired telescope coordinates
            padCoords_TelRF.col(pad) = __moveInCurrentRF(padCoords_TelRF.col(pad), m_inCoords);
            // transform back to PRF
            padCoords_PanelRF.col(pad) = __toPanelRF(curpos, padCoords_TelRF.col(pad));
            for (unsigned coord = 0; coord < 3; coord++)
                newPadCoords[pad][coord] = padCoords_PanelRF.col(pad)(coord);
        }
        m_SP.ComputeActsFromPads(newPadCoords);
        double newActs[6];
        for (unsigned act = 0; act < 6; act++)
            newActs[act] = m_SP.GetActLengths()[act];
        // phew done!
        // compute new panel coordinates based on these actuator lengths
        m_SP.ComputeStewart(newActs);

        double curcoord;
        UaVariant val;
        cout << "\tNew Coords:";
        for (int i = 0; i < 6; i++) {
            curcoord = m_SP.GetPanelCoords()[i];
            cout << " " << curcoord;
            val.setDouble(curcoord);
            pCurObject->setData(PAS_PanelType_inCoords_x + i, val);
        }
        cout << endl << endl;
    }
    // we have populated all the values, now start moving.
    // this is done as its own loop so that all the = panels move simulataneously.
    // i'm looping with iterators instead of the range-based for-loop to hopefully
    // not let the compiler optimize this away and merge the two loops
    for (auto it = childrenSet.begin(); it != childrenSet.end(); it++)
        m_pChildren.at(type).at(*it)->Operate(PAS_PanelType_MoveTo_Coords);
}

// Align all edges between start_idx and end_idx moving in the direction dir
void PasMirror::__alignAll(unsigned start_idx, const set<unsigned>& need_alignment, bool dir)
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

    deque<unsigned> already_aligned {}; // yes, deque, not vector!

    unsigned cur_idx = start_idx;
    while (need_alignment.find(cur_idx) != need_alignment.end() && (m_state == PASState::PAS_On)) {
        already_aligned.push_front(cur_idx);
        // align all the preceding panels
        for (unsigned edge : already_aligned) {
            if ( m_state != PASState::PAS_On ) break;
            // figure out which panel is "greater" and which one is "smaller" in the sense
            // of dir, assuming a two-panel edge for now.
            // get vector of panel positions:
            auto curPanels = SCTMath::GetPanelsFromEdge(m_pChildren.at(PAS_EdgeType).at(edge)->getId().eAddress, dir);

            // align this edge moving the "smaller" panel
            UaVariantArray alignPanels; alignPanels.create(2);
            alignPanels[0].Value.UInt32 = curPanels.at(0);
            alignPanels[1].Value.UInt32 = curPanels.at(1);
            auto movingPanel_idx = m_ChildrenPositionMap.at(PAS_PanelType).at(curPanels.at(0));
            // do this until the edge is aligned
            int aligniter = 1;
            m_pChildren.at(PAS_EdgeType).at(edge)->Operate(PAS_EdgeType_Align, alignPanels);
            while (!static_cast<PasEdge *>(m_pChildren.at(PAS_EdgeType).at(edge))->isAligned()) {
                cout << "\nAlignment Iteration " << aligniter << endl << endl;
                m_pChildren.at(PAS_EdgeType).at(edge)->Operate(PAS_EdgeType_Move);
                usleep(400*1000); // microseconds

                PASState curstate;
                m_pChildren.at(PAS_PanelType).at(movingPanel_idx)->getState(curstate);
                while (curstate == PASState::PAS_Busy) {
                    cout << "\tPanel " << curPanels.at(0) << " moving..." << endl;
                    usleep(200*1000); // microseconds
                    m_pChildren.at(PAS_PanelType).at(movingPanel_idx)->getState(curstate);
                }
                aligniter++;
                m_pChildren.at(PAS_EdgeType).at(edge)->Operate(PAS_EdgeType_Align, alignPanels);
            }
            cout << "\n" << m_pChildren.at(PAS_EdgeType).at(edge)->getId().name
                << " is aligned!" << endl;
        }

        // get the next edge
        Identity id;
        id.eAddress = SCTMath::GetEdgeNeighbor(m_pChildren.at(PAS_EdgeType).at(cur_idx)->getId().eAddress, dir);
        try {
            cur_idx = m_ChildrenIdentityMap.at(PAS_EdgeType).at(id);
        }
        catch (out_of_range) {
            cur_idx = -1; // max element of unsigned
        }
    }
}

void PasMirror::__updateSelectedChildren(unsigned deviceType)
{
    // clear the current list
    m_SelectedChildren[deviceType].clear();

    // process a separated string and find the panels or edges described by it
    // pad by a space from the right so we don't hit the end of the line without a delimiter
    string inStr = m_SelectedChildrenString.at(deviceType) + " ";
    vector<string> strList;
    // working with comma, space and semicolon
    string delim = " ,;:\"\'{}";
    size_t prev = 0, cur;
    while ( (cur = inStr.find_first_of(delim, prev)) != string::npos ) {
        if (cur > prev)
            strList.push_back(inStr.substr(prev, cur-prev));
        prev = cur + 1;
    }

    // add all the items to the selected children set of indices
    if (deviceType == PAS_PanelType) { // expect a list of panel positions
        unsigned curpos;
        for (const string& item : strList) {
            curpos = stoi(item);
            try {
                m_SelectedChildren[deviceType].insert(m_ChildrenPositionMap.at(deviceType).at(curpos));
            }
            catch (std::out_of_range) {
                continue;
            }
        }
    }
    else if (deviceType == PAS_MPESType) { // expect a list of Sensor serials
        Identity curid;
        for (const string& item : strList) {
            curid.serialNumber = stoi(item);
            try {
                m_SelectedChildren[deviceType].insert(m_ChildrenIdentityMap.at(deviceType).at(curid));
            }
            catch (std::out_of_range) {
                continue;
            }
        }
    }

    else if (deviceType == PAS_EdgeType) { // expect a list of edge addresses
        Identity curid;
        for (const string& item : strList) {
            curid.eAddress = item;
            try {
                m_SelectedChildren[deviceType].insert(m_ChildrenIdentityMap.at(deviceType).at(curid));
            }
            catch (std::out_of_range) {
                continue;
            }
        }
    }

    if (m_SelectedChildren.at(deviceType).size() > 0)
        cout << endl << m_ID.name << " selected the following children:\n\t";
    for (auto selected : m_SelectedChildren.at(deviceType))
        cout << m_pChildren.at(deviceType).at(selected)->getId().name << " ";
    cout << endl << endl;
}

void PasMirror::__updateCoords()
{
    // minimize chisq and get telescope coordinates
    TMinuit *minuit = new TMinuit(6); // 6 parameters for 6 telescope coords
    minuit->SetPrintLevel(-1); // suppress all output
    minuit->SetFCN(PasMirrorCompute::getInstance(this).chiSqFCN);
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

    cout << "\n" << m_ID.name << " coordinates:\n";
    for (int i = 0; i < 6; i++)
       cout << m_curCoords(i) << " +/- " << m_curCoordsErr(i) << endl;
    // display angle in common coordinates -- angle*baseRad
//    for (int i = 3; i < 6; i++)
//       cout << m_curCoords(i)*kBaseRadius << " +/- " << m_curCoordsErr(i)*kBaseRadius << endl;

    return;
}

MatrixXd PasMirror::__computeSystematicsMatrix(unsigned pos1, unsigned pos2)
{
    // find influence of moving panel at pos1 on the panel at pos2
    MatrixXd res(6,6);
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
    Matrix3d orig_padCoords2;
    Matrix3d orig_padCoords2_PRF2;
    // original panel coords in PRF2:
    VectorXd orig_panelCoords2(6);
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
    Matrix3d padCoords2;
    VectorXd panelCoords2(6);
    VectorXd TRANSFORM(6);
    for (auto TR = 0; TR < 6; TR++) {
        TRANSFORM.setZero();
        TRANSFORM(TR) = 1. * (TR > 2) ? 1./320. : 1.;
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


void PasMirror::__alignSector()
{
    // following the align method for an edge:
    MatrixXd C; // constraint
    MatrixXd B; // complete matrix

    VectorXd X; // solutions vector -- this moves actuators
    VectorXd Y; // sensor misalignment vector, we want to fit this

    // grab all user specified panels to move and sensors to fit
    vector<PasPanel*> panelsToMove;
    vector<PasMPES*> alignMPES;
    for (unsigned idx : m_SelectedChildren.at(PAS_PanelType))
        panelsToMove.push_back(static_cast<PasPanel *>(m_pChildren.at(PAS_PanelType).at(idx)));
    for (unsigned idx : m_SelectedChildren.at(PAS_MPESType)) {
        PasMPES  *mpes = static_cast<PasMPES *>(m_pChildren.at(PAS_MPESType).at(idx));
        mpes->Operate();
        if (mpes->isVisible())
            alignMPES.push_back(mpes);
    }

    // get the overlapping sensors -- these are the constraining internal ones
    set<unsigned> overlapIndices;
    unsigned idx;
    bool userOverlap = false;
    for (const auto& panel : panelsToMove) {
        for (const auto& mpes : panel->getChildren(PAS_MPESType)) {
            unsigned overlap = 0;
            for (const auto& overlapPanel : panelsToMove)
                overlap += (static_cast<PasMPES *>(mpes)->getPanelSide(overlapPanel->getId().position) != 0);
        
            if (overlap == 2) {
                idx = m_ChildrenIdentityMap.at(PAS_MPESType).at(mpes->getId());
                if (!overlapIndices.count(idx)) {
                    overlapIndices.insert(idx);
                    if (count(alignMPES.begin(), alignMPES.end(), mpes)) {
                        cout << "You specified the following internal MPES: " << mpes->getId().serialNumber << endl;
                        userOverlap = true;
                    }
                }
            }
        }
    }

    // if no user specified overlapping sensors, get their readings
    if (!userOverlap && overlapIndices.size() > 0) {
        cout << "\nNo user-speficied internal MPES." << endl;
        cout << "Reading the automatically identfied internal MPES:" << endl;
        // only read the internal MPES if no user-specified ones have been found
        for (const auto& idx: overlapIndices) {
            PasMPES *mpes = static_cast<PasMPES *>(m_pChildren.at(PAS_MPESType).at(idx));
            mpes->Operate();
            if ( mpes->isVisible() )
                alignMPES.push_back(mpes);
        }
    }
    else if (overlapIndices.size() == 0)
        cout << "\nIdentified NO internal MPES. Are you alignining a single panel?";
    else
        cout << "\nWill be using user-specified internal MPES.";
    cout << endl;


    cout << "Will align the following panels:" << endl;
    for (auto& panel : panelsToMove)
        cout << panel->getId().position << " ";
    cout << endl;
    cout << "Will use the following sensors for alignment:" << endl;
    for (auto& mpes: alignMPES)
        cout << mpes->getId().serialNumber << " ";
    cout << endl;

    // construct the overall target vector and the response matrix
    UaVariant vtmp;
    // store the current readings and the target readings
    VectorXd curRead(2*alignMPES.size());
    VectorXd targetRead(2*alignMPES.size());
    // store individual response matrix;
    MatrixXd responseMat(2, 6);

    unsigned nCols = 6*panelsToMove.size();
    unsigned nRows = 2*alignMPES.size();
    B = MatrixXd(nRows, nCols);
    Y = VectorXd(nRows);
    for (int m = 0; m < alignMPES.size(); m++) {
        alignMPES.at(m)->getData(PAS_MPESType_xCentroidAvg, vtmp);
        vtmp.toDouble(curRead(m*2));
        alignMPES.at(m)->getData(PAS_MPESType_yCentroidAvg, vtmp);
        vtmp.toDouble(curRead(m*2 + 1));
        targetRead.segment(2*m, 2) = alignMPES.at(m)->getAlignedReadings()
                                           - alignMPES.at(m)->getSystematicOffsets();

        for (int p = 0; p < panelsToMove.size(); p++) {
            auto panelSide = alignMPES.at(m)->getPanelSide(panelsToMove.at(p)->getId().position);
            if (panelSide)
                responseMat = alignMPES.at(m)->getResponseMatrix(panelSide);
            else
                responseMat.setZero(); 
            B.block(2*m, 6*p, 2, 6) = responseMat;
        }
    }
    Y = targetRead - curRead;

    // make sure we have enough constraints to solve this
    if (Y.size() < B.cols()) {
        cout << "+++ ERROR! +++ There are " << B.rows()/2 << " sensors and " << B.cols()
            << " actuators -- not enough sensors to constrain the motion. Won't do anything!" << endl;
        return;
    }

    try {
        X = B.jacobiSvd(ComputeThinU | ComputeThinV).solve(Y);
    }
    catch (...) {
        cout << "+++ WARNING! +++ Failed to perform Singular Value Decomposition. "
            << "Check your sensor readings! Discarding this result!" << endl;
        return;
    }

    cout << "\nThe vector to solve for is\n" << Y << endl;
    cout << "\nThe matrix to solve with is\n" << B << endl;
    cout << "\nThe least squares solution is\n" << X << endl;

    if (m_AlignFrac < 1.) {
        cout << "+++ WARNING +++ You requested fractional motion: will move fractionally by " << m_AlignFrac << " of the above:" << endl;
        X *= m_AlignFrac;
        cout << X << endl;
    }

    /* MOVE ACTUATORS */
    unsigned j = 0;
    for (auto& pCurPanel : panelsToMove) {
        auto nACT = pCurPanel->getActuatorCount();
        // print out to make sure
        cout << "Will move actuators of "
            << pCurPanel->getId().name << " by\n" << X.segment(j, 6) << endl;

        UaVariantArray deltas;
        deltas.create(nACT);
        for (unsigned i = 0; i < nACT; i++)
            deltas[i].Value.Float = X(j++); // X has dimension of 6*nPanelsToMove !

        pCurPanel->Operate(PAS_PanelType_StepAll, deltas);
    }
}



void PasMirror::__alignGlobal(unsigned fixPanel)
{
    // we want to fit all sensors simultaneously while constraining the motion of 'fixPanel'

    // first, find which ring we are on:
    unsigned ring = SCTMath::Ring(fixPanel);
    unsigned mirror = SCTMath::Mirror(fixPanel);
    
    unsigned numPanels = 0;
    if (mirror != m_ID.position) {
        cout << "+++ ERROR +++ The entered fixPanel position is wrong! Check it and try again."
            << endl;
        return;
    }

    if (mirror == 1)
        numPanels = Primary::kPanels[ring - 1];
    else if (mirror == 2)
        numPanels = Secondary::kPanels[ring - 1];

    // initialize the response matrices
    MatrixXd localResponse;
    MatrixXd globResponse = MatrixXd(numPanels*6, numPanels*6);
    globResponse.setZero();
    // initialize the misalignment vector
    VectorXd localAlignRead;
    VectorXd localCurRead;
    VectorXd misalignVec;
    VectorXd globMisalignVec;
    // initialize the displacement vector
    VectorXd globDisplaceVec = VectorXd(numPanels*6);

    // get all the edges we need to fit:
    vector<PasEdge *> edgesToFit; // we actually don't need to keep these in a vector,
                                  // but doing this for possible future needs
    vector<PasPanel *> panelsToMove;
    unsigned curPanel = fixPanel, nextPanel, cursize = 0;
    Identity id;
    // keep track of the position in the global response matrix
    unsigned blockRow = 0;
    cout << "+++ DEBUG +++ Traversing Ring " << ring << " of Mirror " << mirror << " clockwise\n"
        << endl;

    MatrixXd T; // Vladimir's T operator
    MatrixXd Eye = MatrixXd::Identity(6,6);
    MatrixXd MCurPrev, MCurNext, MNextCur;
    MatrixXd E;
    T = Eye;

    do {
        nextPanel = SCTMath::GetPanelNeighbor(curPanel, 0);
        cout << "+++ DEBUG +++ Currently on panels (" << curPanel << ", " << nextPanel << ")"
            << endl;

        panelsToMove.push_back( static_cast<PasPanel *> (
                    m_pChildren.at(PAS_PanelType).at(m_ChildrenPositionMap.at(PAS_PanelType).at(curPanel)) ) );

        id.eAddress = SCTMath::GetEdgeFromPanels({curPanel, nextPanel});
        edgesToFit.push_back( static_cast<PasEdge *>(
                    m_pChildren.at(PAS_EdgeType).at(m_ChildrenIdentityMap.at(PAS_EdgeType).at(id)) ) );
        cout << "\t\tThese correspond to the edge " << edgesToFit.back()->getId() << endl;

        cout << "\t\tReading the sensors of this edge." << endl;
        localCurRead = edgesToFit.back()->getCurrentReadings();
        localAlignRead = edgesToFit.back()->getAlignedReadings();

        misalignVec = localAlignRead - localCurRead;
        cout << "+++ DEBUG +++ The current misalignment is\n" << misalignVec << endl;
        cout << "+++ DEBUG +++ Adding this to the the end of the global misalignment vector" << endl;
        globMisalignVec.conservativeResize(cursize + misalignVec.size());
        globMisalignVec.tail(misalignVec.size()) = misalignVec;
        cursize = globMisalignVec.size();


        // get the response of this edge
        localResponse = edgesToFit.back()->getResponseMatrix(curPanel);
        cout << "\t\tResponse matrix [" << curPanel << "][" << nextPanel << "]:\n"
            << localResponse << endl;
        cout << "+++ DEBUG +++ Placing this at location [" << blockRow << "x6]["
            << blockRow << "x6] of the global response matrix" << endl;
        globResponse.block(blockRow*6, blockRow*6,
                localResponse.rows(), localResponse.cols()) = localResponse;
        MCurNext = localResponse;

        localResponse = edgesToFit.back()->getResponseMatrix(nextPanel);
        cout << "\t\tResponse matrix [" << nextPanel << "][" << curPanel << "]:\n"
            << localResponse << endl;
        cout << "+++ DEBUG +++ Placing this at location [" << blockRow << "x6]["
            << (blockRow + 1) % numPanels << "x6] of the global response matrix" << endl;
        globResponse.block(blockRow*6, ((blockRow + 1) % numPanels)*6,
                localResponse.rows(), localResponse.cols()) = localResponse;

        MNextCur = localResponse;


        // replace the first 6 columns of the response matrix with 6x6 identity matrices:
        // this gets rid of the response matrices corresponding to the fixed panel
        // and replaces them with the Systematic Offset Response matrix (just identity)
        globResponse.block(blockRow*6, 0, 6, 6) = MatrixXd::Identity(6, 6);

        curPanel = nextPanel;
        ++blockRow;

        // nothing to do if on the first panel
        if (blockRow != 1) {
            E = MCurNext*MCurPrev.inverse();
            cout << "+++ SPECIAL DEBUG +++ ML = \n" << MCurPrev << endl;
            cout << "+++ SPECIAL DEBUG +++ ML*ML.invserse() = \n" << MCurPrev*MCurPrev.inverse() << endl;
            cout << "+++ SPECIAL DEBUG +++ MR = \n" << MCurNext << endl;
            cout << "+++ SPECIAL DEBUG +++ Operator E = MR*ML.inverse() \n" << E << endl;
            T = Eye - E*T;
        }

        MCurPrev = MNextCur;

    } while ( curPanel != fixPanel );

    cout << "+++ SPECIAL DEBUG +++ Operator T = \n" << T << endl;
    cout << "+++ SPECIAL DEBUG +++ Operator -1*T.inverse() = \n" << -1*T.inverse() << endl;
    cout << "+++ SPECIAL DEBUG +++ Operator T*T.inverse() = \n" << T*T.inverse() << endl;
    cout << "+++ SPECIAL DEBUG +++ You want to compute -1*T.inverse*misalignmentOfLastEdge..." << endl;

    // check that we have enough sensor readings
    if (globMisalignVec.size() < globResponse.rows()) {
        cout << "+++ ERROR +++ Not enough sensor readings to perform the fit! Go through the output above and find which sensor is not in the field of view, fix it, and come back." << endl;
        return;
    }

    cout << "+++ DEBUG +++ misalignment vector size is " << globMisalignVec.size() << endl;
    cout << "+++ DEBUG +++ The first 12 entries:\n" << globMisalignVec.head(12) << endl;
    cout << "+++ DEBUG +++ The last 12 entries:\n" << globMisalignVec.tail(12) << endl;
    cout << "+++ DEBUG +++ The first 12x18 block of the global response matrix:\n"
        << globResponse.block(0,0,12,18) << endl;
    cout << "+++ DEBUG +++ The last 12x18 block of the global response matrix:\n"
        << globResponse.block(globResponse.rows() - 12, globResponse.cols() - 18, 12, 18) << endl;

    // erase the first element of panelsToMove -- this is the panel we decided to keep fixed
    panelsToMove.erase(panelsToMove.begin());

    // we are all set now. now solve the system
    // globResponse * globDisplaceVec = globMisalignVec;
    // the first 6 elements of the solution are the systematic vectors, the others are the
    // panel displacements
    globDisplaceVec = globResponse.jacobiSvd(ComputeThinU | ComputeThinV).solve(globMisalignVec);

    VectorXd check = globResponse*globDisplaceVec - globMisalignVec;
    cout << "+++ DEBUG +++ Checking that the found solution is correct: looking at (R*Solution - MisAlign): norm = " << check.norm() << endl;
    cout << "\t\t First 12 entries\n" << check.head(12) << endl;
    cout << "\t\t Last 12 entries\n" << check.tail(12) << endl;

    m_sysOffsetsMPES = globDisplaceVec.head(6);
    // get the order of read sensors:
    unsigned pos = 0;
    for (const auto& mpes : edgesToFit.back()->m_ChildrenPositionMap.at(PAS_MPESType)) {
        SystematicOffsetsMPESMap[ring][mpes.first] = m_sysOffsetsMPES.segment(pos*2, 2);
        ++pos;
    }
    // make sure we read out the systematic vector in the same order as the sensors
    // and assign it correctly to outer/middle/inner

    cout << "The fitted systematic offset is, in the order of read sensors:\n"
        << m_sysOffsetsMPES << endl;

    // set the offset for each sensor edge
    for (const auto& edge : edgesToFit) {
        // and set them for each sensor along each edge
        for (const auto& mpes : edge->m_ChildrenPositionMap.at(PAS_MPESType) ) {
            (static_cast<PasMPES *>(edge->m_pChildren.at(PAS_MPESType).at(mpes.second)))->SystematicOffsets =
                SystematicOffsetsMPESMap.at(ring).at(mpes.first);
        }
    }

    if (m_AlignFrac < 1.)
        cout << "\n+++ WARNING +++ You requested fractional motion: will move fractionally by "
            << m_AlignFrac << " of the computed displacement" << endl;

    // loop through panels and set the dispalements
    /* MOVE ACTUATORS */
    // remember to update selected panels too
    m_SelectedChildrenString.at(PAS_PanelType) = "";
    unsigned j = 6;
    for (auto& pCurPanel : panelsToMove) {
        m_SelectedChildrenString.at(PAS_PanelType) += to_string(pCurPanel->getId().position) + " ";
        auto nACT = pCurPanel->getActuatorCount();
        // print out to make sure
        cout << "Will move actuators of "
            << pCurPanel->getId().name << " by (accounting for fractional motion)\n" << m_AlignFrac*globDisplaceVec.segment(j, 6) << endl;

        UaVariantArray deltas;
        deltas.create(nACT);
        for (unsigned i = 0; i < nACT; i++)
            deltas[i].Value.Float = m_AlignFrac*globDisplaceVec(j++); // X has dimension of 6*nPanelsToMove !

        pCurPanel->Operate(PAS_PanelType_StepAll, deltas);
    }
    // remember to update selected panels
    __updateSelectedChildren(PAS_PanelType);

    cout << "Target actuator lengths set, you can move the panels now by calling .moveSector()" << endl;

    return;
}

/* =========== Coordinate Transformations =========== */

// get the angular offset of this panel from our ideal one for which everything is precomputed
double PasMirror::__getAzOffset(unsigned pos)
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


Vector3d PasMirror::__toPanelRF(unsigned pos, Vector3d in_coords)
{
    // all angles in radians

    int ring = SCTMath::Ring(pos);
    double phi = __getAzOffset(pos);

    // this panel's frame is the rotated frame of the ideal panel
    Matrix3d zRot = __rotMat(2, phi);
    Matrix3d panelFrame =  zRot * m_PanelFrame.at(ring);
    Vector3d panelOrigin = zRot * m_PanelOriginTelFrame.at(ring);

    return panelFrame.transpose() * (in_coords - panelOrigin);
}

Vector3d PasMirror::__toTelRF(unsigned pos, Vector3d in_coords)
{
    // all angles in radians
    // The inverse of the above

    int ring = SCTMath::Ring(pos);
    double phi = __getAzOffset(pos);

    // this panel's frame is the rotated frame of the ideal panel
    Matrix3d zRot = __rotMat(2, phi);
    Matrix3d panelFrame =  zRot * m_PanelFrame.at(ring);
    Vector3d panelOrigin = zRot * m_PanelOriginTelFrame.at(ring);

    // remember panel frame is an orthogonal matrix -- very simple inversion
    return panelFrame * in_coords + panelOrigin;
}

Matrix3d PasMirror::__rotMat(int axis, double a)
{
    double c = cos(a);
    double s = sin(a);

    Matrix3d rot;
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

Vector3d PasMirror::__moveInCurrentRF(Vector3d in_vec, VectorXd tr_coords)
{
    // check that we have 6 coords. if not, resize if necessary
    VectorXd tr(6);
    tr.head(tr_coords.size()) = tr_coords;
    if (tr_coords.size() != 6)
        tr.tail(6 - tr_coords.size()).setZero();

    Vector3d out_vec = in_vec;
    // compute the transform due to the change in TRF: Rot(z -> x -> y)*v + T
    out_vec = __rotMat(2, tr(5)) * out_vec;
    out_vec = __rotMat(0, tr(3)) * out_vec;
    out_vec = __rotMat(1, tr(4)) * out_vec;
    out_vec += tr.head(3);

    return out_vec;
}



/* ============== MINUIT INTERFACE ============== */
double PasMirror::chiSq(VectorXd telDelta)
{
    // tel delta is a perturbation to the coordinates of the mirror
    double chiSq = 0.;
    const auto& panels = m_pChildren.at(PAS_PanelType);

    // pad coordinates in TRF as computed from actuator lengths
    Vector3d padCoordsActs;
    // pad coordinates in TRF as computed from telescope perturbation
    Vector3d padCoordsTel;

    UaVariant val;
    // go over all panels
    for (auto idx : m_SelectedChildren.at(PAS_PanelType)) {
        int pos = panels.at(idx)->getId().position;
        int ring = SCTMath::Ring(pos);

        // go over all pads -- ChiSq is the squared difference between pad coordinates
        // as computed from actuator lengths and pad coordinates as computed from telescope
        // coordinates
        for (int pad = 0; pad < 3; pad++) {
            padCoordsActs = static_cast<PasPanel *>(panels.at(idx))->getPadCoords().col(pad);
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

void PasMirrorCompute::chiSqFCN(int &npar, double *gin, double &f, double *par, int iflag) // MINUIT interface
{
    Matrix<double, 6, 1> vecPars(par);
    f = Mirror->chiSq(vecPars);
}
