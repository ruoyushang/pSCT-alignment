#include "client/controllers/panelcontroller.hpp"

#include <string>
#include <chrono>

#include <Eigen/Dense>

#include "client/clienthelper.h"

// 1 s update interval for panels
int PanelController::kUpdateInterval = 1000;

PanelController::PanelController(Identity identity, Client *pClient) :
        PasCompositeController(identity, pClient, kUpdateInterval),
        m_inCoordsUpdated(false) {
    m_ID.name = std::string("Panel_") + std::to_string(m_ID.position);
    m_state = PASState::On;
    m_SP.SetPanelType(StewartPlatform::PanelType::OPT);

    // define possible children types
    m_ChildrenTypes = {PAS_ACTType, PAS_MPESType, PAS_EdgeType};

    // make sure things update on the first boot up
    // duration takes seconds -- hence the conversion with the 1/1000 ratio
    m_lastUpdateTime = TIME::now() - std::chrono::duration<int, ratio < 1, 1000>>
    (m_UpdateInterval_ms);
}

PanelController::~PanelController() {
    m_pClient = nullptr;

    for (auto &devVector : m_pChildren)
        for (auto &dev : devVector.second)
            dev = nullptr;

    m_state = PASState::Off;
}

unsigned PanelController::getActuatorCount() {
    try {
        return m_pChildren.at(PAS_ACTType).size();
    }
    catch (std::out_of_range) {
        return 0;
    }
}

UaStatus PanelController::getState(PASState &state) {
    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    UaVariant val;
    unsigned intState;

    vector <std::string> vec_curread{"ns=2;s=Panel_0.State"};
    status = m_pClient->read(vec_curread, &val);
    if (!status.isGood())
        return status;

    val.toUInt32(intState);
    switch (intState) {
        case static_cast<unsigned>(PASState::On) :
            m_state = PASState::On;
            break;
        case static_cast<unsigned>(PASState::Off) :
            m_state = PASState::Off;
            break;
        case static_cast<unsigned>(PASState::Busy) :
            m_state = PASState::Busy;
            break;
        case static_cast<unsigned>(PASState::OperableError) :
            m_state = PASState::OperableError;
            break;
        case static_cast<unsigned>(PASState::FatalError) :
            m_state = PASState::FatalError;
            break;
        default:
            return OpcUa_BadInvalidState;
    }

    state = m_state;

    return status;

}

UaStatus PanelController::setState(PASState state) {
    if (state == PASState::OperableError || state == PASState::FatalError) {
        return OpcUa_BadInvalidArgument;
    }

    UaMutexLocker lock(&m_mutex);
    m_state = state;
    return OpcUa_Good;
}

UaStatus PanelController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    UaMutexLocker lock(&m_mutex);
    UaStatusCode status = OpcUa_Good;

    if (getActuatorCount() == 0) {
        std::cout << m_ID << "::getData() : no actuators, data undefined." << std::endl;
        return OpcUa_Good;
    }

    if (offset >= PAS_PanelType_x && offset <= PAS_PanelType_zRot) {
        // update current coordinates
        updateCoords();
        int dataOffset = offset - PAS_PanelType_x;
        value.setDouble(m_curCoords[dataOffset]);
    } else if (offset == PAS_PanelType_IntTemperature)
        status = m_pClient->read({"ns=2;s=Panel_0.InternalTemperature"}, &value);
    else if (offset == PAS_PanelType_ExtTemperature)
        status = m_pClient->read({"ns=2;s=Panel_0.ExternalTemperature"}, &value);

    else
        status = OpcUa_BadInvalidArgument;

    return status;
}

UaStatus PanelController::setData(OpcUa_UInt32 offset, UaVariant value) {
    UaMutexLocker lock(&m_mutex);
    return OpcUa_BadInvalidArgument;
}

// move actuators to the preset length or panel to the preset coords
UaStatusCode PanelController::Operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    UaMutexLocker lock(&m_mutex);
    UaStatusCode status;

    if (getActuatorCount() == 0) {
        std::std::cout << m_ID << "::Operate() : No actuators found, nothing to be done." << std::std::endl;
        return OpcUa_Good;
    }

    // update the state
    PASState dummy;
    getState(dummy);

    if (offset != PAS_PanelType_Stop) {
        if (m_state == PASState::FatalError)
            std::cout << m_ID << "::Operate() : Current state is 'FatalError'! This won't do anything." << std::endl;
        if (m_state == PASState::Busy)
            std::cout << m_ID << "::Operate() : Current state is 'Busy'! This won't do anything." << std::endl;
    }

    auto &actuatorPositionMap = m_ChildrenPositionMap.at(PAS_ACTType);
    auto &pACT = m_pChildren.at(PAS_ACTType);

    /************************************************
     * move actuators to the preset lengths         *
     * **********************************************/
    bool collisionFlag = false;
    if (offset == PAS_PanelType_MoveDeltaLengths) {
        bool collision_flag = checkForCollision();
        if (collision_flag) std::cout << "Error: Sensors will go out of range! Motion cancelled." << std::endl;
#ifndef SIMMODE
        if (!collision_flag) {
            status = m_pClient->callMethodAsync(std::string("ns=2;s=Panel_0"), UaString("MoveDeltaLengths"), args);
        }
#else
        for (int i = 0; i < 6; i++)
            m_curCoords[i] = m_inCoords[i];
#endif
    } else if (offset == PAS_PanelType_MoveToLengths) {
        bool collision_flag = checkForCollision();
        if (collision_flag) std::cout << "Error: Sensors will go out of range! Motion cancelled." << std::endl;

#ifndef SIMMODE
        if (!collision_flag) {
            status = m_pClient->callMethodAsync(std::string("ns=2;s=Panel_0"), UaString("MoveToLengths"), args);
        }
#else
        for (int i = 0; i < 6; i++) {
            if (!collision_flag) {
                m_curCoords[i] = m_inCoords[i];
            }
        }
#endif
    } else if (offset == PAS_PanelType_MoveToCoords) {
        std::cout << std::endl << m_ID << ":" << std::endl;
        if (args.length() != pACT.size())
            return OpcUa_BadInvalidArgument;

        updateCoords(); // update coordinates

        std::cout << "\tCurrent panel coordinates (x, y ,z xRot, yRot, zRot):\n\t\t";
        for (int i = 0; i < 6; i++) {
            std::cout << m_curCoords[i] << " ";
        }
        std::cout << std::endl << std::endl;

        double inputCoordinates[6];

        std::cout << "\tTarget panel coordinates (x, y ,z xRot, yRot, zRot):\n\t\t";
        for (int i = 0; i < 6; i++) {
            UaVariant(args[i]).toDouble(inputCoordinates[i]);
            std::cout << inputCoordinates[i] << " ";
        }

        // find actuator lengths needed
        m_SP.ComputeActsFromPanel(inputCoordinates);

        UaVariantArray lengthArgs;
        double actLengths[6];
        UaVariant val;

        bool collision_flag = checkForCollision();
        if (collision_flag) std::cout << "Error: Sensors will go out of range! Motion cancelled." << std::endl;

        if (!collision_flag) {
            // Get actuator lengths for motion
            for (int i = 0; i < 6; i++) {
                lengthArgs[i] = UaVariant(m_SP.GetActLengths()[i])[0];
            }

            status = m_pClient->callMethodAsync(std::string("ns=2;s=Panel_0"), UaString("MoveToLengths"), lengthArgs);
        }
    } else if (offset == PAS_PanelType_ReadAll) {
        UaVariant val;
        updateCoords();
        std::cout << std::endl << m_ID << " :" << std::endl;
        std::cout << "\tCurrent coordinates (x, y ,z xRot, yRot, zRot):\n\t\t";
        for (int i = 0; i < 6; i++)
            std::cout << m_curCoords[i] << " ";
        std::cout << std::endl << std::endl;

        std::cout << "\tCurrent actuator lengths:\n";
        // map is ordered by the first index, in our case, the position
        for (const auto &i : actuatorPositionMap) {
            std::cout << "\t\t(" << pACT.at(i.second)->getId().serialNumber << ", " << i.first << ") : "
                      << m_ActuatorLengths(i.first - 1) << " mm" << std::endl;
        }

        status = OpcUa_Good;
    }
        /************************************************
         * stop the motion in progress                  *
         * **********************************************/
    else if (offset == PAS_PanelType_Stop) {
        std::cout << m_ID << "::Operate() : Attempting to gracefully stop the motion." << std::endl;
        status = m_pClient->callMethod(std::string("ns=2;s=Panel_0"), UaString("Stop"));
    } else
        status = OpcUa_BadInvalidArgument;

    return status;
}

bool PanelController::checkForCollision() {
    ///
    /// This part predicts the new laser spot positions before we move the actuators.
    /// Will make a new function of this and allow all actuator operations to call this function before moving.
    ///
    Eigen::MatrixXd M_response; // response matrix
    Eigen::VectorXd Act_delta; // actuator delta length
    Eigen::VectorXd Sen_current; // sensor current position
    Eigen::VectorXd Sen_delta; // sensor delta
    Eigen::VectorXd Sen_new; // sensor new position = Sen_delta + current sensor reading
    Eigen::VectorXd Sen_aligned; // aligned sensor reading
    Eigen::VectorXd Sen_deviation; // deviated sensor reading

    Eigen::VectorXd newLengths(6);
    for (unsigned i = 0; i < 6; i++)
        newLengths(i) = m_SP.GetActLengths()[i];
    std::cout << "New Act length is \n" << newLengths << std::endl;
    std::cout << "Current Act length is \n" << m_ActuatorLengths << std::endl;
    Act_delta = newLengths - m_ActuatorLengths;
    std::cout << "Delta Act length will be \n" << Act_delta << std::endl;

    if (m_pChildren.count(PAS_EdgeType) > 0) {
        for (auto &e : m_pChildren.at(PAS_EdgeType)) {
            EdgeController *edge = static_cast<EdgeController *>(e);
            M_response = edge->getResponseMatrix(m_ID.position);
            Sen_current = edge->getCurrentReadings();
            std::cout << "Looking at edge " << edge->getId() << std::endl;
            std::cout << "\nCurrent MPES readings:\n" << Sen_current << std::endl << std::endl;
            Sen_aligned = edge->getAlignedReadings();
            std::cout << "\nTarget MPES readings:\n" << Sen_aligned << std::endl << std::endl;
            std::cout << "\nActuator response matrix for this edge:\n" << M_response << std::endl;
            Sen_delta = M_response * Act_delta;
            Sen_new = Sen_delta + Sen_current;
            std::cout << "The new sensor coordinates (x, y) will be:\n" << Sen_new << std::endl;
            Sen_deviation = Sen_new - Sen_aligned;
            std::cout << "\n will deviate from the aligned position by\n" << Sen_new - Sen_aligned << std::endl;
            double deviation = 0;
            for (unsigned i = 0; i < 6; i++)
                deviation += pow(Sen_deviation(i), 2);
            deviation = pow(deviation, 0.5);
            if (deviation > 40) return true;
        }
    }

    return false;

}

void PanelController::updateCoords(bool printout) {
    // do nothing if values haven't expired
    if (!__expired())
        return;

    UaVariant val;
    m_ActuatorLengths.resize(getActuatorCount());

    auto &actuatorPositionMap = m_ChildrenPositionMap.at(PAS_ACTType);
    auto &pACT = m_pChildren.at(PAS_ACTType);

    for (const auto &i : actuatorPositionMap) {
        pACT.at(i.second)->getData(PAS_ACTType_CurrentLength, val);
        val.toDouble(m_ActuatorLengths(i.first - 1));

        if (printout) {
            std::cout << "Actual length of actuator " << pACT.at(i.second)->getId().serialNumber
                      << " at position " << i.first << " is " << m_ActuatorLengths(i.first - 1) << " mm" << std::endl;
        }
    }
    // update current coordinates
    m_SP.ComputeStewart(m_ActuatorLengths.data());
    // panel coords
    for (int i = 0; i < 6; i++)
        m_curCoords[i] = m_SP.GetPanelCoords()[i];

    // pad coords -- each column corresponds to a pad
    for (int pad = 0; pad < 3; pad++)
        // populate panel frame pad coordinates
        for (int coord = 0; coord < 3; coord++)
            m_PadCoords.col(pad)(coord) = m_SP.GetPadCoords(pad)[coord];

    m_lastUpdateTime = TIME::now();
}