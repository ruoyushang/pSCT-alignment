#include "client/controllers/edgecontroller.hpp"

EdgeController::EdgeController(Identity identity) : PasCompositeController(identity, nullptr), m_isAligned(false) {
    m_ID.name = string("Edge_") + m_ID.eAddress;
    m_state = PASState::On;
    // defin possible children types
    m_ChildrenTypes = {PAS_MPESType, PAS_PanelType};
}

EdgeController::~EdgeController() {
    m_pClient = nullptr;

    for (auto &devVector : m_pChildren)
        for (auto &dev : devVector.second)
            dev = nullptr;

    m_state = PASState::Off;
}

/* ----------------------------------------------------------------------------
    Class        EdgeController
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatus EdgeController::getState(PASState &state) {
    UaMutexLocker lock(&m_mutex);
    state = m_state;
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        EdgeController
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatus EdgeController::setState(PASState state) {
    if (state == PASState::OperableError || state == PASState::FatalError) {
        return OpcUa_BadInvalidArgument;
    }
    if (state == m_state) {
        return OpcUa_BadInvalidState;
    }
    UaMutexLocker lock(&m_mutex);
    m_state = state;
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        EdgeController
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatus EdgeController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    UaMutexLocker lock(&m_mutex);

    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        EdgeController
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatus EdgeController::setData(OpcUa_UInt32 offset, UaVariant value) {
    UaMutexLocker lock(&m_mutex);

    return OpcUa_BadNotWritable;
}

/* ----------------------------------------------------------------------------
    Class        EdgeController
    Method       Operate
    Description  run a method on the actuator
-----------------------------------------------------------------------------*/
UaStatusCode EdgeController::Operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    UaMutexLocker lock(&m_mutex);
    UaStatusCode status;

    unsigned numPanels;
    try {
        numPanels = m_pChildren.at(PAS_PanelType).size();
        m_pChildren.at(PAS_MPESType).size();
    }
    catch (out_of_range) {
        // this should never happen -- and edge should never exist without panels
        cout << m_ID << "::findMatrix() : no panels or sensors, nothing to do. "
             << "This should never happen, by the way. Just saying." << endl;
        return OpcUa_BadInvalidState;
    }

    switch (offset) {
        case PAS_EdgeType_FindMatrix:
            status = findMatrix(args);
            break;
        case PAS_EdgeType_Align: {
            // input args should have been validated by this point.

            // this is the utilities:
            //      if edge has 2 panels, things are unambiguous -- move the
            //      panel specified by the panel_move value (arg[0]) keeping
            //      the panel_fix (arg[1]) panel fixed.
            //
            //      if the edge has 3 panels, things become a bit more ambiguous,
            //      but there are still only two possibilities:
            //      keep the P1 panel fixed while moving the P2 panels;
            //      or move the P1 panel while keeping the two P2 panels fixed.
            //      so in this case, we're looking for which of the two args is
            //      the P1 panel. If neither is a P1 panel, the input is bad.

            OpcUa_UInt32 panel;
            bool moveit;
            double alignFrac;

            if (numPanels == 2) {
                panel = args[0].Value.UInt32;
                moveit = true;
            } else {
                // panel ring is the decimal value -- 1 for inner; 2 for outer:
                //              panelRing = (panel_pos / 10) % 10;
                if (((panel = args[0].Value.UInt32) / 10) % 10 == 1)
                    // the panel_move panel is P1
                    moveit = true;
                else if (((panel = args[1].Value.UInt32) / 10) % 10 == 1)
                    // the panel_fix panel is P1
                    moveit = false;
                else {
                    status = OpcUa_BadInvalidArgument;
                    break;
                }
            }

            UaVariant(args[2]).toDouble(alignFrac);

            status = align(panel, moveit, alignFrac);
            break;
        }
        case PAS_EdgeType_Read:
            // update current and target readings and print them out
            cout << "\n" << m_ID.name << " :" << endl;;
            getCurrentReadings();
            getAlignedReadings();

            if (!m_CurrentReadings.size() || !m_AlignedReadings.size()) {
                cout
                        << "+++ ERROR +++ No physical sensor readings! Make sure the sensors are in the field of view. Nothing to do for now."
                        << endl;
                status = OpcUa_Bad;
                break;
            }

            cout << "\nCurrent MPES readings:\n";
            for (unsigned i = 0; i < m_CurrentReadings.size(); i++)
                cout << m_CurrentReadings(i) << " +/- " << m_CurrentReadingsSD(i) << endl;

            cout << "\nTarget MPES readings:\n" << m_AlignedReadings << endl << endl;
            cout << "\nMisalignment:\n" << m_AlignedReadings - m_CurrentReadings << endl << endl;
            status = OpcUa_Good;
            break;
        case PAS_EdgeType_Stop: {
            // stop motion of all panels
            for (const auto &panel : m_pChildren.at(PAS_PanelType))
                panel->operate(PAS_PanelType_Stop);
            break;
        }
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/* ----------------------------------------------------------------------------
    Class        EdgeController
    Method       findMatrix
    Description  Find the alignment matrix for this edge
-----------------------------------------------------------------------------*/
UaStatus EdgeController::findMatrix(UaVariantArray args) {
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    double stepSize;
    UaVariant(args[0]).toDouble(stepSize);

    // safety of this has already been checked by the caller
    unsigned numPanels = m_pChildren.at(PAS_PanelType).size();

    for (unsigned i = 0; i < numPanels; i++) {
        status = __findSingleMatrix(i, stepSize);
        if (!status.isGood())
            return status;
    }

    return status;
}

// helper method for the above -- actually moving the panel and measuring the matrix
UaStatus EdgeController::__findSingleMatrix(unsigned panelIdx, double stepSize) {
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    MatrixXd responseMatrix(6, 6);
    responseMatrix.setZero();
    // convenience variable;
    // no need to check with a try/catch block anymore as this has already been done
    // by the caller
    PasPanel *pCurPanel = static_cast<PasPanel *>(m_pChildren.at(PAS_PanelType).at(panelIdx));
    unsigned nACTs = pCurPanel->getActuatorCount();

    VectorXd vector0(6); // maximum possible size
    VectorXd vector1(6); // maximum possible size

    UaVariant minusdeltaL, deltaL, zeroDelta;
    deltaL.setFloat(stepSize);
    minusdeltaL.setFloat(-1 * stepSize);
    zeroDelta.setFloat(0.);

    UaVariantArray deltas;
    deltas.create(nACTs);

    UaVariant vtmp;
    float missedDelta;

    for (unsigned j = 0; j < nACTs; j++) {
        // extra safety -- initialize array to zero every time
        for (unsigned k = 0; k < nACTs; k++)
            zeroDelta.copyTo(&deltas[k]);

        missedDelta = 0.;

        vector0 = getCurrentReadings();
        cout << "+++ CURRENT READINGS:\n" << vector0 << endl << "Sleeping for 1s" << endl;
        sleep(1); // seconds

        printf("attempting to move actuator %d by %5.3f mm\n", j, stepSize);
        deltaL.copyTo(&deltas[j]);
        status = pCurPanel->Operate(PAS_PanelType_MoveDeltaLengths, deltas);
        if (!status.isGood()) return status;
        // Stepping is asynchronous. but here, we want it to actually complete
        // before the next step. So we wait.
        // Need to change stepping to not be asynchronous?
        PASState curState = PASState::Busy;
        while (curState == PASState::Busy) {
            usleep(300 * 1000); // microseconds
            pCurPanel->getState(curState);
        }
        cout << "+++ MOTION SHOULD HAVE FINISHED. Sleeping for 2s" << endl;
        sleep(2);

        // update missed steps
        pCurPanel->getActuatorSteps(deltas);
        missedDelta = deltas[j].Value.Float;

        printf("actuator %d missed target by %5.3f mm\n", j, missedDelta);

        vector1 = getCurrentReadings();
        cout << "+++ CURRENT READINGS:\n" << vector1 << endl << "Sleeping for 1s" << endl;
        sleep(1); // seconds
        cout << "+++ Difference in sensor readings:\n" << vector1 - vector0 << endl;
        // move the same actuator back
        printf("moving actuator %d back", j);
        minusdeltaL.copyTo(&deltas[j]);
        status = pCurPanel->Operate(PAS_PanelType_MoveDeltaLengths, deltas);
        if (!status.isGood()) return status;
        // Stepping is asynchronous. but here, we want it to actually complete
        // before the next step. So we wait.
        curState = PASState::Busy;
        while (curState == PASState::Busy) {
            usleep(300 * 1000); // microseconds
            pCurPanel->getState(curState);
        }
        cout << "+++ MOTION SHOULD HAVE FINISHED. Sleeping for 2s" << endl;
        sleep(2);

        responseMatrix.col(j) = (vector1 - vector0) / (stepSize - missedDelta);
        cout << "+++ CURRENT RESPONSE MATRIX:\n" << responseMatrix << endl;
    }
    cout << "\n+++ ALL DONE FOR THIS PANEL!" << endl;
    cout << "+++ Response matrix for " << m_ID.eAddress << ": panel " << pCurPanel->getId().position << endl;
    cout << responseMatrix << endl;

    string outfilename = "/home/ctauser/PanelAlignmentData/ResponseMatrix_" + m_ID.eAddress + ".txt";
    ofstream output(outfilename, ios_base::in | ios_base::out | ios_base::app);
    stringstream outputstr;
    output << pCurPanel->getId().position << endl << responseMatrix << endl;

    /*
    auto& pMPES = m_pChildren.at(PAS_MPESType);
    for (const auto& mpes : m_ChildrenPositionMap.at(PAS_MPESType)) {
        for (auto const& coord : {'x', 'y'}) {
            // explicit cast here
            char panelside = static_cast<PasMPES *>(pMPES.at(mpes.second))->getPanelSide(pCurPanel->getId().position);

            // prepare command for DB
            outputstr.str(string());
            outputstr << "addResponseMatrix2db "
                << pMPES.at(mpes.second)->getId().serialNumber << " " << coord << " " << panelside;
            outputstr << " \""; // begin quoted list of values
            // -------------------- //

            output << pMPES.at(mpes.second)->getId().serialNumber << " " << coord << " " << panelside;
            for (unsigned j = 0; j < nACTs; j++) {
                output << " " << responseMatrix[2*i + int(coord - 'x')][j];
                outputstr << " " << responseMatrix[2*i + int(coord - 'x')][j];
            }
            output << endl;

            outputstr << "\""; //end quoted list of values
        //    system(outputstr.str().c_str());
        }
    } */
    output.close();

    return status;
}

/* ----------------------------------------------------------------------------
    Class        EdgeController
    Method       align
    Description  Align this edge to the nominal position using the found matrix
-----------------------------------------------------------------------------*/
UaStatus EdgeController::align(unsigned panel_pos, bool moveit, double alignFrac) {
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    cout << "\nAligning " << m_ID.name << ": ";
    if (moveit)
        cout << "Will align panel " << panel_pos << endl;
    else
        cout << "Will keep fixed panel " << panel_pos << endl;

    status = __alignSinglePanel(panel_pos, moveit, alignFrac);


/*
    unsigned numPanels;
    try {
        numPanels = m_pChildren.at(PAS_PanelType).size();
        m_pChildren.at(PAS_MPESType).size();
    }
    catch (out_of_range) {
        // this should never happen -- and edge should never exist without panels
        cout << m_ID << "::align() : no panels or sensors, nothing to do. "
            << "This should never happen, by the way. Just saying." << endl;
        return OpcUa_BadInvalidState;
    }

    // FOR NOW: move only the panel with the lower position value
    // (this will move P1/S1 in the case of the P1-P2/S1-S2 edge)
    for (const auto& panelPair : m_ChildrenPositionMap.at(PAS_PanelType)) {
        cout << "Will align panel " << panelPair.first << endl;
        status = __alignSinglePanel(panelPair.second);
        // return after the first panel
        //if (!status.isGood())
            return status;
    }
*/
    return status;
}

UaStatus EdgeController::__alignSinglePanel(unsigned panelpos, bool moveit, double alignFrac) {
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    MatrixXd A; // response matrix
    MatrixXd C; // constraint matrix
    MatrixXd B; // complete matrix we want to solve for
    VectorXd current_read = getCurrentReadings();
    VectorXd aligned_read = getAlignedReadings() - getSystematicOffsets();
    if (!current_read.size() || !aligned_read.size()) {
        cout
                << "+++ ERROR +++ No physical sensor readings! Make sure the sensors are in the field of view. Nothing to do for now."
                << endl;
        return OpcUa_Bad;
    }

    VectorXd X; // solutions vector
    VectorXd Y; // sensor misalignment vector, we want to fit to this

    if (moveit) {
        A = getResponseMatrix(panelpos);
        C.setZero();

        B = A;
        Y = aligned_read - current_read;
    } else { // this panel kept fixed -- we have three panels
        // get the response matrix, which is [A1 | A2]
        vector <MatrixXd> responseMats;
        for (const auto &panelPair : m_ChildrenPositionMap.at(PAS_PanelType))
            if (panelPair.first != panelpos)
                responseMats.push_back(getResponseMatrix(panelPair.first)); // get response from the two other panels
        auto totalRows = max_element(responseMats.begin(), responseMats.end(),
                                     [](MatrixXd a, MatrixXd b) { return a.rows() < b.rows(); })->rows();
        auto totalCols = accumulate(responseMats.begin(), responseMats.end(), 0,
                                    [](int curval, MatrixXd a) { return curval + a.cols(); });

        A = MatrixXd(totalRows, totalCols);
        A.setZero();

        int curcol = 0;
        for (unsigned i = 0; i < responseMats.size(); i++) {
            auto curMat = responseMats.at(i);
            A.block(0, curcol, curMat.rows(), curMat.cols()) = curMat;
            curcol += curMat.cols();
        }

        // get the constraint matrix, which is [U1 | U2]: we need U1*L1 + U2*L2 = <OVERLAP MISALIGNMENT>;
        // so we construct the constraint matrix as [U1 | U2] and the vector to solve for as [L1; L2]
        //
        // get the overlapping sensors between the other two panels
        int twopanels[2]; // hold positions of the other two panels
        int i = 0;
        for (const auto &panelPair : m_ChildrenPositionMap.at(PAS_PanelType))
            if (panelPair.first != panelpos)
                twopanels[i++] = panelPair.first;

        vector < PasMPES * > overlapMPES;
        for (const auto &panelPair : m_ChildrenPositionMap.at(PAS_PanelType)) {
            if (panelPair.first == panelpos)
                continue;

            auto pMPES = static_cast<PasPanel *>(m_pChildren.at(PAS_PanelType).at(panelPair.second))->getChildren(
                    PAS_MPESType);
            for (const auto &mpes : pMPES)
                if (static_cast<PasMPES *>(mpes)->getPanelSide(twopanels[0])
                    && static_cast<PasMPES *>(mpes)->getPanelSide(twopanels[1]))
                    overlapMPES.push_back(static_cast<PasMPES *>(mpes));
        }
        auto blockRows = overlapMPES.front()->getResponseMatrix().rows();
        auto blockCols = overlapMPES.front()->getResponseMatrix().cols();

        // get misalignment of overlap sensors
        VectorXd overlap_current(overlapMPES.size() * 2);
        VectorXd overlap_aligned(overlapMPES.size() * 2);
        UaVariant vtmp;
        unsigned visible = 0;
        for (auto &mpes : overlapMPES) {
            mpes->Operate();
            if (!mpes->isVisible()) continue;

            mpes->getData(PAS_MPESType_xCentroidAvg, vtmp);
            vtmp.toDouble(overlap_current(visible * 2));
            mpes->getData(PAS_MPESType_yCentroidAvg, vtmp);
            vtmp.toDouble(overlap_current(visible * 2 + 1));

            overlap_aligned.segment(visible * 2, 2) = mpes->getAlignedReadings()
                                                      - mpes->getSystematicOffsets();

            ++visible;
        }
        overlap_current.conservativeResize(2 * visible);
        overlap_aligned.conservativeResize(2 * visible);

        C = MatrixXd(visible * blockRows, 2 * blockCols);
        C.setZero();

        for (int j = 0; j < 2; j++) {
            int i = 0;
            for (const auto &mpes : overlapMPES) {
                if (!mpes->isVisible()) continue;
                auto panelside = mpes->getPanelSide(twopanels[j]);
                C.block(blockRows * i, blockCols * j, blockRows, blockCols) = mpes->getResponseMatrix(panelside);
                i++;
            }
        }

        // B = |A|
        //     |C|
        // Y = |y1|, where y1,2 = aligned - current
        //     |y2|
        B = MatrixXd(A.rows() + C.rows(), A.cols());
        B.block(0, 0, A.rows(), A.cols()) = A;
        B.block(A.rows(), 0, C.rows(), A.cols()) =
                sqrt(2.) * C; // increase the weight of the constraint due to the degenaracy along the P1-P2 edge
        Y = VectorXd(aligned_read.size() + overlap_current.size());
        Y.head(aligned_read.size()) = aligned_read - current_read;
        Y.tail(overlap_current.size()) = sqrt(2.) * (overlap_aligned - overlap_current);

        aligned_read.conservativeResize(aligned_read.size() + overlap_aligned.size());
        aligned_read.tail(overlap_aligned.size()) = overlap_aligned;
        current_read.conservativeResize(current_read.size() + overlap_current.size());
        current_read.tail(overlap_current.size()) = overlap_current;
    }

    // make sure we have enough constraints to solve this
    if (Y.size() < B.cols()) {
        cout << "+++ ERROR! +++ Not enough sensors to constrain the motion. Won't do anything!" << endl;
        return OpcUa_Bad;
    }

    try {
        X = B.jacobiSvd(ComputeThinU | ComputeThinV).solve(Y);
    }
    catch (...) {
        cout << "+++ WARNING! +++ Failed to perform Singular Value Decomposition. "
             << "Check your sensor readings! Discarding this result!" << endl;
        return OpcUa_Bad;
    }

    cout << "\nCurrent MPES readings:\n" << current_read << endl;
    cout << "\nTarget MPES readings (accounting for systematics):\n" << aligned_read << endl;
    cout << "\nActuator response matrix for this edge:\n" << A << endl;
    if (!C.isZero(0))
        cout << "\nConstraint matrix for this edge:\n" << C << endl;

    // in the case of moving this panel (3 sensors and 6 actuators),
    // we want to solve delS = A * delL for delL.
    // we get a least squares solution: A^T * delS = (A^T * A) delL
    // delL = (A^T * A) \ A^T * delS
    // This is done through singular value decomposition for numerical stability.
    //
    // When this panel is fixed and we move the other two (4 sensors and 12 actuators),
    // we want to solve the constrained least squares problem
    // min(|A * delL - delS|) given that C*delL = 0 for delL.
    // we get the following least squares solution:
    // delL = (A^T * A) \ A^T * delS - (A^T * A) \ C^T * [C(A^T * A) \ C^T]\C * (A^T * A) \ A^T * delS
    // We do this partly through brute force and partly through SVD.

    cout << "\nLEAST SQUARES SOLUTION:\n" << X << endl;
    // while (X.norm() >= 14*(X.size()/6)) { // heuristic -- don't want to move by more than the panel gap
    if (alignFrac < 1.) {
        cout << "+++ WARNING +++ You requested fractional motion: will move fractionally by " << alignFrac
             << " of the above:" << endl;
        X *= alignFrac;
        cout << X << endl;
    }

    // make sure we don't move what we're not supposed to
    m_PanelsToMove.clear();

    /* MOVE ACTUATORS */
    PasPanel *pCurPanel;
    int j = 0;
    for (const auto &panelPair : m_ChildrenPositionMap.at(PAS_PanelType)) {
        if ((panelPair.first == panelpos) == moveit) { // clever but not clear...
            pCurPanel = static_cast<PasPanel *>(m_pChildren.at(PAS_PanelType).at(panelPair.second));
            auto nACT = pCurPanel->getActuatorCount();
            // print out to make sure
            cout << "Will move actuators of "
                 << pCurPanel->getId().name << " by\n" << X.segment(j, 6) << endl;

            UaVariantArray deltas;
            deltas.create(nACT);
            for (unsigned i = 0; i < nACT; i++)
                deltas[i].Value.Float = X(j++); // X has dimension of 6*nPanelsToMove !

            status = pCurPanel->Operate(PAS_PanelType_MoveDeltaLengths, deltas);
            if (!status.isGood()) return status;
            m_PanelsToMove.push_back(panelPair.second);
        }
    }

    // arbitrary
    if (X.array().abs().maxCoeff() >= 0.05)
        m_isAligned = false;
    else
        m_isAligned = true;

    cout << "\nYou can also call " << m_ID.name << ".Move to move the panel(s).\n" << endl;

    return status;
}

// Get response matrix for the panel defined by 'panelpos'
// this is the response of the sensors on this edge to the motion of the requested panel
const MatrixXd &EdgeController::getResponseMatrix(unsigned panelpos) {
    auto &pMPES = m_pChildren.at(PAS_MPESType);
    unsigned maxMPES = pMPES.size();
    unsigned nACT = static_cast<PasMPES *>(pMPES.front())->getResponseMatrix().cols();

    for (const auto &panelPair : m_ChildrenPositionMap.at(PAS_PanelType)) {
        unsigned panel = panelPair.first;

        unsigned visibleMPES = 0;
        m_ResponseMatMap[panel] = MatrixXd(maxMPES * 2, nACT);
        m_ResponseMatMap.at(panel).setZero();
        for (unsigned nMPES = 0; nMPES < maxMPES; nMPES++) {
//        for (const auto& mpes : m_ChildrenPositionMap.at(PAS_MPESType)) {
            // do not check if the sensor is visible here -- return the full response matrix
//            if ( !static_cast<PasMPES *>(pMPES.at(nMPES))->isVisible() ) continue;

            auto panelside = static_cast<PasMPES *>(pMPES.at(nMPES))->getPanelSide(panel);
            // if this is nonzero (so either 'l' or 'w'), add it to the edge response matrix
            if (panelside) {
                const auto &curresponse = static_cast<PasMPES *>(pMPES.at(nMPES))->getResponseMatrix(panelside);
                m_ResponseMatMap.at(panel).block(2 * visibleMPES, 0, curresponse.rows(),
                                                 curresponse.cols()) = curresponse;
            }
            ++visibleMPES;
        }
        m_ResponseMatMap.at(panel).conservativeResize(2 * visibleMPES, nACT);
    }

    // no need to try-catch -- panelpos here is always valid
    return m_ResponseMatMap.at(panelpos);
}


const VectorXd &EdgeController::getAlignedReadings() {
    auto &pMPES = m_pChildren.at(PAS_MPESType);
    unsigned maxMPES = pMPES.size();
    unsigned visibleMPES = 0;

    m_AlignedReadings = VectorXd(2 * pMPES.size());
    for (unsigned nMPES = 0; nMPES < maxMPES; nMPES++) {
        if (!static_cast<PasMPES *>(pMPES.at(nMPES))->isVisible()) continue;
//    for (const auto& mpes : m_ChildrenPositionMap.at(PAS_MPESType)) {
//        if ( !static_cast<PasMPES *>(pMPES.at(mpes.second))->isVisible() ) continue;

        auto mpes_response = static_cast<PasMPES *>(pMPES.at(nMPES))->getAlignedReadings();
        m_AlignedReadings.segment(2 * visibleMPES, 2) = mpes_response;
        ++visibleMPES;
    }
    m_AlignedReadings.conservativeResize(2 * visibleMPES);

    return m_AlignedReadings;
}

const VectorXd &EdgeController::getSystematicOffsets() {
    auto &pMPES = m_pChildren.at(PAS_MPESType);
    unsigned maxMPES = pMPES.size();
    unsigned visibleMPES = 0;

    m_systematicOffsets = VectorXd(2 * pMPES.size());
    for (unsigned nMPES = 0; nMPES < maxMPES; nMPES++) {
        if (!static_cast<PasMPES *>(pMPES.at(nMPES))->isVisible()) continue;
//    for (const auto& mpes : m_ChildrenPositionMap.at(PAS_MPESType)) {
//        if ( !static_cast<PasMPES *>(pMPES.at(mpes.second))->isVisible() ) continue;

        m_systematicOffsets.segment(2 * visibleMPES, 2) =
                (static_cast<PasMPES *>(pMPES.at(nMPES)))->getSystematicOffsets();
        ++visibleMPES;
    }
    m_systematicOffsets.conservativeResize(2 * visibleMPES);

    return m_systematicOffsets;
}

const Eigen::VectorXd &EdgeController::getCurrentReadings() {
    // edge should have at least one sensor by definition -- otherwise it wouldn't be created.
    // so this is safe.
    auto &pMPES = m_pChildren.at(PAS_MPESType);
    unsigned maxMPES = pMPES.size();
    unsigned visibleMPES = 0;

    m_CurrentReadings = VectorXd(2 * maxMPES);
    m_CurrentReadingsSD = VectorXd(2 * maxMPES);

    UaVariant vtmp;
    for (unsigned nMPES = 0; nMPES < maxMPES; nMPES++) {
        pMPES.at(nMPES)->operate();
        if (!static_cast<PasMPES *>(pMPES.at(nMPES))->isVisible()) {
            cout << "+++ WARNING +++ " << pMPES.at(nMPES)->getId().name
                 //for (const auto& mpes : m_ChildrenPositionMap.at(PAS_MPESType)) {
                 //    pMPES.at(mpes.second)->Operate();
                 //    if ( !static_cast<PasMPES *>(pMPES.at(mpes.second))->isVisible() ) {
                 //        cout << "+++ WARNING +++ " << pMPES.at(mpes.second)->getId().name
                 << " is not in the field of view! Will ignore it." << endl;
            continue;
        }

        pMPES.at(nMPES)->getData(PAS_MPESType_xCentroidAvg, vtmp);
        vtmp.toDouble(m_CurrentReadings(visibleMPES * 2));
        pMPES.at(nMPES)->getData(PAS_MPESType_yCentroidAvg, vtmp);
        vtmp.toDouble(m_CurrentReadings(visibleMPES * 2 + 1));

        pMPES.at(nMPES)->getData(PAS_MPESType_xCentroidSD, vtmp);
        vtmp.toDouble(m_CurrentReadingsSD(visibleMPES * 2));
        pMPES.at(nMPES)->getData(PAS_MPESType_yCentroidSD, vtmp);
        vtmp.toDouble(m_CurrentReadingsSD(visibleMPES * 2 + 1));

        ++visibleMPES;
    }
    m_CurrentReadings.conservativeResize(2 * visibleMPES);
    m_CurrentReadingsSD.conservativeResize(2 * visibleMPES);

    return m_CurrentReadings;
