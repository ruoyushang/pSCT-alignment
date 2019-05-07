#include "client/controllers/edgecontroller.hpp"

#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <vector>

#include <Eigen/Dense>

#include "client/controllers/mpescontroller.hpp"
#include "client/controllers/panelcontroller.hpp"
#include "client/controllers/actcontroller.hpp"


EdgeController::EdgeController(Device::Identity identity) : PasCompositeController(std::move(identity), nullptr, 0),
                                                            m_isAligned(false) {
    m_ID.name = std::string("Edge_") + m_ID.eAddress;
    m_state = Device::DeviceState::On;
    // defin possible children types
    m_ChildrenTypes = {PAS_MPESType, PAS_PanelType};
    m_Xcalculated = Eigen::VectorXd(0);
}

EdgeController::~EdgeController() {
    m_pClient = nullptr;

    for (auto &devVector : m_pChildren)
        for (auto &dev : devVector.second)
            dev = nullptr;

    m_state = Device::DeviceState::Off;
}

/* ----------------------------------------------------------------------------
    Class        EdgeController
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatus EdgeController::getState(Device::DeviceState &state) {
    //UaMutexLocker lock(&m_mutex);
    state = m_state;
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        EdgeController
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatus EdgeController::setState(Device::DeviceState state) {
    if (state == Device::DeviceState::OperableError || state == Device::DeviceState::FatalError) {
        return OpcUa_BadInvalidArgument;
    }
    if (state == m_state) {
        return OpcUa_BadInvalidState;
    }
    //UaMutexLocker lock(&m_mutex);
    m_state = state;
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        EdgeController
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
UaStatus EdgeController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);

    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        EdgeController
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatus EdgeController::setData(OpcUa_UInt32 offset, UaVariant value) {
    //UaMutexLocker lock(&m_mutex);

    return OpcUa_BadNotWritable;
}

/* ----------------------------------------------------------------------------
    Class        EdgeController
    Method       Operate
    Description  run a method on the actuator
-----------------------------------------------------------------------------*/
UaStatus EdgeController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    unsigned numPanels;
    try {
        numPanels = m_pChildren.at(PAS_PanelType).size();
        m_pChildren.at(PAS_MPESType).size();
    }
    catch (std::out_of_range &e) {
        // this should never happen -- and edge should never exist without panels
        std::cout << m_ID << "::findMatrix() : no panels or sensors, nothing to do. "
                  << "This should never happen, by the way. Just saying." << std::endl;
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
            bool execute;

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

            execute = args[3].Value.Boolean;

            UaVariant(args[2]).toDouble(alignFrac);

            status = align(panel, alignFrac, moveit, execute);
            break;
        }
        case PAS_EdgeType_Read:
            // update current and target readings and print them out
            std::cout << "\n" << m_ID.name << " :" << std::endl;;
            getCurrentReadings();
            getAlignedReadings();

            if (!m_CurrentReadings.size() || !m_AlignedReadings.size()) {
                std::cout
                        << "+++ ERROR +++ No physical sensor readings! Make sure the sensors are in the field of view. Nothing to do for now."
                        << std::endl;
                status = OpcUa_Bad;
                break;
            }

            std::cout << "\nCurrent MPES readings:\n";
            for (unsigned i = 0; i < m_CurrentReadings.size(); i++)
                std::cout << m_CurrentReadings(i) << " +/- " << m_CurrentReadingsSpotWidth(i) << std::endl;

            std::cout << "\nTarget MPES readings:\n" << m_AlignedReadings << std::endl << std::endl;
            std::cout << "\nMisalignment:\n" << m_AlignedReadings - m_CurrentReadings << std::endl << std::endl;
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
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    double stepSize;
    UaVariant(args[0]).toDouble(stepSize);

    // safety of this has already been checked by the caller
    unsigned numPanels = m_pChildren.at(PAS_PanelType).size();

    for (unsigned i = 0; i < numPanels; i++) {
        status = findSingleMatrix(i, stepSize);
        if (!status.isGood())
            return status;
    }

    return status;
}

// helper method for the above -- actually moving the panel and measuring the matrix
UaStatus EdgeController::findSingleMatrix(unsigned panelIdx, double stepSize) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    Eigen::MatrixXd responseMatrix(6, 6);
    responseMatrix.setZero();
    // convenience variable;
    // no need to check with a try/catch block anymore as this has already been done
    // by the caller
    PanelController *pCurPanel = dynamic_cast<PanelController *>(m_pChildren.at(PAS_PanelType).at(panelIdx));
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
    ActController *actuator;
    float missedDelta;

    for (unsigned j = 0; j < nACTs; j++) {
        // extra safety -- initialize array to zero every time
        for (unsigned k = 0; k < nACTs; k++)
            zeroDelta.copyTo(&deltas[k]);

        missedDelta = 0.;

        vector0 = getCurrentReadings();
        std::cout << "+++ CURRENT READINGS:\n" << vector0 << std::endl << "Sleeping for 1s" << std::endl;
        sleep(1); // seconds

        printf("attempting to move actuator %d by %5.3f mm\n", j, stepSize);
        deltaL.copyTo(&deltas[j]);
        status = pCurPanel->operate(PAS_PanelType_MoveDeltaLengths, deltas);
        if (!status.isGood()) return status;
        // Stepping is asynchronous. but here, we want it to actually complete
        // before the next step. So we wait.
        // Need to change stepping to not be asynchronous?
        Device::DeviceState curState = Device::DeviceState::Busy;
        while (curState == Device::DeviceState::Busy) {
            usleep(300 * 1000); // microseconds
            pCurPanel->getState(curState);
        }
        std::cout << "+++ MOTION SHOULD HAVE FINISHED. Sleeping for 2s" << std::endl;
        sleep(2);

        // update missed steps
        actuator = dynamic_cast<ActController *>(pCurPanel->getChildren(PAS_ACTType)[j]);
        actuator->getData(PAS_ACTType_DeltaLength, vtmp);
        vtmp.toFloat(missedDelta);

        printf("actuator %d missed target by %5.3f mm\n", j, missedDelta);

        vector1 = getCurrentReadings();
        std::cout << "+++ CURRENT READINGS:\n" << vector1 << std::endl << "Sleeping for 1s" << std::endl;
        sleep(1); // seconds
        std::cout << "+++ Difference in sensor readings:\n" << vector1 - vector0 << std::endl;
        // move the same actuator back
        printf("moving actuator %d back", j);
        minusdeltaL.copyTo(&deltas[j]);
        status = pCurPanel->operate(PAS_PanelType_MoveDeltaLengths, deltas);
        if (!status.isGood()) return status;
        // Stepping is asynchronous. but here, we want it to actually complete
        // before the next step. So we wait.
        curState = Device::DeviceState::Busy;
        while (curState == Device::DeviceState::Busy) {
            usleep(300 * 1000); // microseconds
            pCurPanel->getState(curState);
        }
        std::cout << "+++ MOTION SHOULD HAVE FINISHED. Sleeping for 2s" << std::endl;
        sleep(2);

        responseMatrix.col(j) = (vector1 - vector0) / (stepSize - missedDelta);
        std::cout << "+++ CURRENT RESPONSE MATRIX:\n" << responseMatrix << std::endl;
    }
    std::cout << "\n+++ ALL DONE FOR THIS PANEL!" << std::endl;
    std::cout << "+++ Response matrix for " << m_ID.eAddress << ": panel " << pCurPanel->getId().position << std::endl;
    std::cout << responseMatrix << std::endl;

    std::string outfilename = "/home/ctauser/PanelAlignmentData/ResponseMatrix_" + m_ID.eAddress + ".txt";
    std::ofstream output(outfilename, std::ofstream::in | std::ofstream::out | std::ofstream::app);
    std::stringstream outputstr;
    output << pCurPanel->getId().position << std::endl << responseMatrix << std::endl;

    output.close();

    return status;
}

/*-------------------------------------------------------------------------
    Class        EdgeController
    Method       align
    Description  Align this edge to the nominal position using the found matrix
-----------------------------------------------------------------------------*/
UaStatus EdgeController::align(unsigned panel_pos, double alignFrac, bool moveit, bool execute) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    std::cout << "\nAligning " << m_ID.name << ": ";
    if (moveit)
        std::cout << "Will align panel " << panel_pos << std::endl;
    else
        std::cout << "Will keep fixed panel " << panel_pos << std::endl;

    status = alignSinglePanel(panel_pos, alignFrac, moveit, execute);


/*
    unsigned numPanels;
    try {
        numPanels = m_pChildren.at(PAS_PanelType).size();
        m_pChildren.at(PAS_MPESType).size();
    }
    catch (out_of_range) {
        // this should never happen -- and edge should never exist without panels
        std::cout << m_ID << "::align() : no panels or sensors, nothing to do. "
            << "This should never happen, by the way. Just saying." << std::endl;
        return OpcUa_BadInvalidState;
    }

    // FOR NOW: move only the panel with the lower position value
    // (this will move P1/S1 in the case of the P1-P2/S1-S2 edge)
    for (const auto& panelPair : m_ChildrenPositionMap.at(PAS_PanelType)) {
        std::cout << "Will align panel " << panelPair.first << std::endl;
        status = __alignSinglePanel(panelPair.second);
        // return after the first panel
        //if (!status.isGood())
            return status;
    }
*/
    return status;
}

UaStatus EdgeController::alignSinglePanel(unsigned panelpos, double alignFrac, bool moveit, bool execute) {

    //UaMutexLocker lock(&m_mutex);
    UaStatus status;
    if (!execute) {
        Eigen::MatrixXd A; // response matrix
        Eigen::MatrixXd C; // constraint matrix
        Eigen::MatrixXd B; // complete matrix we want to solve for
        Eigen::VectorXd current_read = getCurrentReadings();
        Eigen::VectorXd aligned_read = getAlignedReadings() - getSystematicOffsets();
        if (!current_read.size() || !aligned_read.size()) {
            std::cout
                    << "+++ ERROR +++ No physical sensor readings! Make sure the sensors are in the field of view. Nothing to do for now."
                    << std::endl;
            return OpcUa_Bad;
        }

        Eigen::VectorXd X; // solutions vector
        Eigen::VectorXd Y; // sensor misalignment vector, we want to fit to this

        if (moveit) {
            A = getResponseMatrix(panelpos);
            C.setZero();

            B = A;
            Y = aligned_read - current_read;
        } else { // this panel kept fixed -- we have three panels
            // get the response matrix, which is [A1 | A2]
            std::vector<Eigen::MatrixXd> responseMats;
            for (const auto &panelPair : m_ChildrenPositionMap.at(PAS_PanelType))
                if (panelPair.first != panelpos)
                    responseMats.push_back(getResponseMatrix(panelPair.first)); // get response from the two other panels
            auto totalRows = max_element(responseMats.begin(), responseMats.end(),
                                         [](Eigen::MatrixXd a, Eigen::MatrixXd b) { return a.rows() < b.rows(); })->rows();
            auto totalCols = accumulate(responseMats.begin(), responseMats.end(), 0,
                                        [](int curval, Eigen::MatrixXd a) { return curval + a.cols(); });

            A = Eigen::MatrixXd(totalRows, totalCols);
            A.setZero();

            int curcol = 0;
            for (const auto &responseMat : responseMats) {
                A.block(0, curcol, responseMat.rows(), responseMat.cols()) = responseMat;
                curcol += responseMat.cols();
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

            std::vector<MPESController *> overlapMPES;
            for (const auto &panelPair : m_ChildrenPositionMap.at(PAS_PanelType)) {
                if (panelPair.first == panelpos)
                    continue;

                auto pMPES = dynamic_cast<PanelController *>(m_pChildren.at(PAS_PanelType).at(
                        panelPair.second))->getChildren(
                        PAS_MPESType);
                for (const auto &mpes : pMPES)
                    if (dynamic_cast<MPESController *>(mpes)->getPanelSide(twopanels[0])
                        && dynamic_cast<MPESController *>(mpes)->getPanelSide(twopanels[1]))
                        overlapMPES.push_back(dynamic_cast<MPESController *>(mpes));
            }
            auto blockRows = overlapMPES.front()->getResponseMatrix().rows();
            auto blockCols = overlapMPES.front()->getResponseMatrix().cols();

            // get misalignment of overlap sensors
            Eigen::VectorXd overlap_current(overlapMPES.size() * 2);
            Eigen::VectorXd overlap_aligned(overlapMPES.size() * 2);
            UaVariant vtmp;
            unsigned visible = 0;
            for (auto &mpes : overlapMPES) {
                mpes->read(false);
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

            C = Eigen::MatrixXd(visible * blockRows, 2 * blockCols);
            C.setZero();

            for (int j = 0; j < 2; j++) {
                int k = 0;
                for (const auto &mpes : overlapMPES) {
                    if (!mpes->isVisible()) continue;
                    auto panelside = mpes->getPanelSide(twopanels[j]);
                    C.block(blockRows * k, blockCols * j, blockRows, blockCols) = mpes->getResponseMatrix(panelside);
                    k++;
                }
            }

            // B = |A|
            //     |C|
            // Y = |y1|, where y1,2 = aligned - current
            //     |y2|
            B = Eigen::MatrixXd(A.rows() + C.rows(), A.cols());
            B.block(0, 0, A.rows(), A.cols()) = A;
            B.block(A.rows(), 0, C.rows(), A.cols()) =
                    sqrt(2.) * C; // increase the weight of the constraint due to the degenaracy along the P1-P2 edge
            Y = Eigen::VectorXd(aligned_read.size() + overlap_current.size());
            Y.head(aligned_read.size()) = aligned_read - current_read;
            Y.tail(overlap_current.size()) = sqrt(2.) * (overlap_aligned - overlap_current);

            aligned_read.conservativeResize(aligned_read.size() + overlap_aligned.size());
            aligned_read.tail(overlap_aligned.size()) = overlap_aligned;
            current_read.conservativeResize(current_read.size() + overlap_current.size());
            current_read.tail(overlap_current.size()) = overlap_current;
        }

        // make sure we have enough constraints to solve this
        if (Y.size() < B.cols()) {
            std::cout << "+++ ERROR! +++ Not enough sensors to constrain the motion. Won't do anything!" << std::endl;
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

        std::cout << "\nCurrent MPES readings:\n" << current_read << std::endl;
        std::cout << "\nTarget MPES readings (accounting for systematics):\n" << aligned_read << std::endl;
        std::cout << "\nActuator response matrix for this edge:\n" << A << std::endl;
        if (!C.isZero(0))
            std::cout << "\nConstraint matrix for this edge:\n" << C << std::endl;

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

        for (const auto &panelPair : m_ChildrenPositionMap.at(PAS_PanelType)) {
            if ((panelPair.first == panelpos) == moveit) { // clever but not clear...
                PanelController *pCurPanel = dynamic_cast<PanelController *>(m_pChildren.at(PAS_PanelType).at(panelPair.second)); 
                if(pCurPanel->checkForCollision((X * alignFrac).segment(0,6))) {
                    std::cout << "Error: Sensors may go out of range! Disallowed motion, please recalculate or relax safety radius." << std::endl;
                    return OpcUa_Bad;
                }
            }
        }
        std::cout << "\nLEAST SQUARES SOLUTION:\n" << X << std::endl;
        // while (X.norm() >= 14*(X.size()/6)) { // heuristic -- don't want to move by more than the panel gap
        if (alignFrac < 1.) {
            std::cout << "+++ WARNING +++ You requested fractional motion: will move fractionally by " << alignFrac
                      << " of the above:" << std::endl;
            X *= alignFrac;
            std::cout << X << std::endl;
        }
        m_Xcalculated = X;
        std::cout << "Calculation done! You should call the method again with execute=True to actually execute the motion." << std::endl;
    }
    else {
        if (m_Xcalculated.isZero(0)) {
            std::cout << "No calculated motion found.  Call Edge.align with execute=false once first to calculate the motion to execute." << std::endl;
            return OpcUa_Bad;
        }
        else {
            /* MOVE ACTUATORS */
            PanelController *pCurPanel;
            int j = 0;
            for (const auto &panelPair : m_ChildrenPositionMap.at(PAS_PanelType)) {
                if ((panelPair.first == panelpos) == moveit) { // clever but not clear...
                    pCurPanel = dynamic_cast<PanelController *>(m_pChildren.at(PAS_PanelType).at(panelPair.second));
                    auto nACT = pCurPanel->getActuatorCount();
                    // print out to make sure
                    std::cout << "Will move actuators of "
                              << pCurPanel->getId().name << " by\n" << m_Xcalculated.segment(j, nACT) << std::endl;
                    
                    UaVariantArray deltas;
                    deltas.create(nACT);
                    UaVariant var;
                    for (int i = 0; i < (int)nACT; i++) {
                        var.setFloat(m_Xcalculated(j++));
                        var.copyTo(&deltas[i]);
                    }
                    status = pCurPanel->moveDeltaLengths(deltas);
                    if (!status.isGood()) return status;
                }
            }
        }
        m_Xcalculated.setZero();
    }

    // arbitrary
    if (m_Xcalculated.array().abs().maxCoeff() >= 0.05)
        m_isAligned = false;
    else
        m_isAligned = true;

    return status;
}

// Get response matrix for the panel defined by 'panelpos'
// this is the response of the sensors on this edge to the motion of the requested panel
const Eigen::MatrixXd &EdgeController::getResponseMatrix(unsigned panelpos) {
    auto &pMPES = m_pChildren.at(PAS_MPESType);
    unsigned maxMPES = pMPES.size();
    unsigned nACT = dynamic_cast<MPESController *>(pMPES.front())->getResponseMatrix().cols();

    for (const auto &panelPair : m_ChildrenPositionMap.at(PAS_PanelType)) {
        unsigned panel = panelPair.first;

        unsigned visibleMPES = 0;
        m_ResponseMatMap[panel] = Eigen::MatrixXd(maxMPES * 2, nACT);
        m_ResponseMatMap.at(panel).setZero();
        for (unsigned nMPES = 0; nMPES < maxMPES; nMPES++) {
//        for (const auto& mpes : m_ChildrenPositionMap.at(PAS_MPESType)) {
            // do not check if the sensor is visible here -- return the full response matrix
//            if ( !static_cast<MPESController *>(pMPES.at(nMPES))->isVisible() ) continue;
            auto panelside = dynamic_cast<MPESController *>(pMPES.at(nMPES))->getPanelSide(panel);
            // if this is nonzero (so either 'l' or 'w'), add it to the edge response matrix
            if (panelside) {
                const auto &curresponse = dynamic_cast<MPESController *>(pMPES.at(nMPES))->getResponseMatrix(panelside);
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


const Eigen::VectorXd &EdgeController::getAlignedReadings() {
    auto &pMPES = m_pChildren.at(PAS_MPESType);
    unsigned maxMPES = pMPES.size();
    unsigned visibleMPES = 0;

    m_AlignedReadings = Eigen::VectorXd(2 * pMPES.size());
    for (unsigned nMPES = 0; nMPES < maxMPES; nMPES++) {
        dynamic_cast<MPESController *>(pMPES.at(nMPES))->getState(state);
        if (!dynamic_cast<MPESController *>(pMPES.at(nMPES))->isVisible() || state == Device::DeviceState::FatalError ||
            state == Device::DeviceState::Off)
            continue;
//    for (const auto& mpes : m_ChildrenPositionMap.at(PAS_MPESType)) {
//        if ( !static_cast<MPESController *>(pMPES.at(mpes.second))->isVisible() ) continue;

        auto mpes_response = dynamic_cast<MPESController *>(pMPES.at(nMPES))->getAlignedReadings();
        m_AlignedReadings.segment(2 * visibleMPES, 2) = mpes_response;
        ++visibleMPES;
    }
    m_AlignedReadings.conservativeResize(2 * visibleMPES);

    return m_AlignedReadings;
}

const Eigen::VectorXd &EdgeController::getSystematicOffsets() {
    auto &pMPES = m_pChildren.at(PAS_MPESType);
    unsigned maxMPES = pMPES.size();
    unsigned visibleMPES = 0;

    m_systematicOffsets = Eigen::VectorXd(2 * pMPES.size());
    for (unsigned nMPES = 0; nMPES < maxMPES; nMPES++) {
        if (!dynamic_cast<MPESController *>(pMPES.at(nMPES))->isVisible()) continue;
//    for (const auto& mpes : m_ChildrenPositionMap.at(PAS_MPESType)) {
//        if ( !static_cast<MPESController *>(pMPES.at(mpes.second))->isVisible() ) continue;

        m_systematicOffsets.segment(2 * visibleMPES, 2) =
            (dynamic_cast<MPESController *>(pMPES.at(nMPES)))->getSystematicOffsets();
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

    m_CurrentReadings = Eigen::VectorXd(2 * maxMPES);
    m_CurrentReadingsSpotWidth = Eigen::VectorXd(2 * maxMPES);

    Device::DeviceState state;

    UaVariant vtmp;
    for (unsigned nMPES = 0; nMPES < maxMPES; nMPES++) {
        dynamic_cast<MPESController*>(pMPES.at(nMPES))->read(false);
        dynamic_cast<MPESController *>(pMPES.at(nMPES))->getState(state);
        if (!dynamic_cast<MPESController *>(pMPES.at(nMPES))->isVisible() || state == Device::DeviceState::FatalError ||
            state == Device::DeviceState::Off) {
            std::cout << "+++ WARNING +++ " << pMPES.at(nMPES)->getId().name
                      //for (const auto& mpes : m_ChildrenPositionMap.at(PAS_MPESType)) {
                      //    pMPES.at(mpes.second)->Operate();
                      //    if ( !static_cast<MPESController *>(pMPES.at(mpes.second))->isVisible() ) {
                      //        std::cout << "+++ WARNING +++ " << pMPES.at(mpes.second)->getId().name
                      << " is not in the field of view! Will ignore it." << std::endl;
            continue;
        }

        pMPES.at(nMPES)->getData(PAS_MPESType_xCentroidAvg, vtmp);
        vtmp.toDouble(m_CurrentReadings(visibleMPES * 2));
        pMPES.at(nMPES)->getData(PAS_MPESType_yCentroidAvg, vtmp);
        vtmp.toDouble(m_CurrentReadings(visibleMPES * 2 + 1));

        pMPES.at(nMPES)->getData(PAS_MPESType_xCentroidSpotWidth, vtmp);
        vtmp.toDouble(m_CurrentReadingsSpotWidth(visibleMPES * 2));
        pMPES.at(nMPES)->getData(PAS_MPESType_yCentroidSpotWidth, vtmp);
        vtmp.toDouble(m_CurrentReadingsSpotWidth(visibleMPES * 2 + 1));

        ++visibleMPES;
    }
    m_CurrentReadings.conservativeResize(2 * visibleMPES);
    m_CurrentReadingsSpotWidth.conservativeResize(2 * visibleMPES);

    return m_CurrentReadings;
}
