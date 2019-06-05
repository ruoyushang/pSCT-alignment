#include "client/controllers/panelcontroller.hpp"

#include <chrono>
#include <string>

#include <Eigen/Dense>

#include "common/alignment/device.hpp"

#include "client/clienthelper.hpp"
#include "client/objects/panelobject.hpp"


PanelController::PanelController(Device::Identity identity, Client *pClient) :
    PasCompositeController(std::move(identity), pClient, 5000) {
    m_ID.name = std::string("Panel_") + std::to_string(m_ID.position);
    m_SP.SetPanelType(StewartPlatform::PanelType::OPT);

    // define possible children types
    m_ChildrenTypes = {PAS_ACTType, PAS_MPESType, PAS_EdgeType};

    // make sure things update on the first boot up
    // duration takes seconds -- hence the conversion with the 1/1000 ratio
    m_lastUpdateTime = TIME::now() - std::chrono::duration<int, std::ratio<1, 1000>>
            (m_kUpdateInterval_ms);
}

unsigned PanelController::getActuatorCount() {
    try {
        return m_pChildren.at(PAS_ACTType).size();
    }
    catch (std::out_of_range &e) {
        return 0;
    }
}

UaStatus PanelController::getState(Device::DeviceState &state) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;
    UaVariant value;
    int v;
    
    std::vector<std::string> vec_curread{"ns=2;s=Panel_0.State"};
    status = m_pClient->read(vec_curread, &value);
    value.toInt32(v);

    state = static_cast<Device::DeviceState>(v);

    return status;
}

UaStatus PanelController::setState(Device::DeviceState state) {
    return OpcUa_BadNotWritable;
}

UaStatus PanelController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (getActuatorCount() == 0) {
        std::cout << m_ID << " :: PanelController::getData() : no actuators, data undefined." << std::endl;
        return OpcUa_Good;
    }

    if (PanelObject::ERRORS.count(offset) > 0) {
        return getError(offset, value);
    } else if (offset >= PAS_PanelType_x && offset <= PAS_PanelType_zRot) {
        // update current coordinates
        if (__expired()) {
            status = updateCoords();
            if (status.isBad()) {
                std::cout << m_ID << " :: PanelController::operate() : Failed to update coordinates." << std::endl;
                return status;
            }
        }
        int dataOffset = offset - PAS_PanelType_x;
        value.setDouble(m_curCoords[dataOffset]);
    } else if (offset == PAS_PanelType_IntTemperature)
        status = m_pClient->read({"ns=2;s=Panel_0.InternalTemperature"}, &value);
    else if (offset == PAS_PanelType_ExtTemperature)
        status = m_pClient->read({"ns=2;s=Panel_0.ExternalTemperature"}, &value);
    else if (offset == PAS_PanelType_SafetyRadius) {
        value.setDouble(m_safetyRadius);
    } else if (offset == PAS_PanelType_Position) {
        status = m_pClient->read({"ns=2;s=Panel_0.Position"}, &value);
    } else if (offset == PAS_PanelType_Serial) {
        status = m_pClient->read({"ns=2;s=Panel_0.Serial"}, &value); 
    } else if (offset == PAS_PanelType_ErrorState) {
        status = m_pClient->read({"ns=2;s=Panel_0.ErrorState"}, &value);
    }
    else
        status = OpcUa_BadInvalidArgument;

    if (status == OpcUa_BadInvalidState) {
        std::cout << m_ID << " :: PanelController::operate() : Device is in a bad state (busy, off, error) and "
                             "could not execute command. Check state and try again. \n";
    }

    return status;
}

UaStatus PanelController::getError(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (PanelObject::ERRORS.count(offset) > 0) {
        std::string varName = std::get<0>(PanelObject::ERRORS.at(offset));
        std::vector<std::string> varsToRead = {std::string("ns=2;s=Panel_0.") + varName};
        status = m_pClient->read(varsToRead, &value);
    } else {
        status = OpcUa_BadInvalidArgument;
    }
    return status;
}

UaStatus PanelController::setData(OpcUa_UInt32 offset, UaVariant value) {
    //UaMutexLocker lock(&m_mutex);

    if (offset == PAS_PanelType_SafetyRadius) {
        value.toDouble(m_safetyRadius);
        return OpcUa_Good;
    }
    return OpcUa_BadInvalidArgument;
}

// move actuators to the preset length or panel to the preset coords
UaStatus PanelController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (getActuatorCount() == 0) {
        std::cout << m_ID << "::Operate() : No actuators found, nothing to be done." << std::endl;
        return OpcUa_Good;
    }

    if (offset == PAS_PanelType_MoveToLengths || offset == PAS_PanelType_MoveToCoords || offset == PAS_PanelType_MoveDeltaLengths) {
        if (__getErrorState() == Device::ErrorState::FatalError || __getDeviceState() != Device::DeviceState::On) {
            std::cout << m_ID << " :: PanelController::operate() : Device is in a bad state (busy, off, error) and "
                                 "could not execute command. Check state and try again. \n";
            return OpcUa_BadInvalidState;
        }
        // Update coordinates before doing any calculations
        status = updateCoords();
    }

    /************************************************
     * move actuators to the preset lengths         *
     * **********************************************/
    float deltaLength;
    float targetLength;
    Eigen::VectorXd deltaLengths(6);
    Eigen::VectorXd targetLengths(6);
    
    if (offset == PAS_PanelType_MoveDeltaLengths) {
        for (int i = 0; i < 6; i++) {
            UaVariant(args[i]).toFloat(deltaLength);
            deltaLengths(i) = (double)deltaLength;
        }
        std::cout << "Calling Panel::MoveDeltaLengths() with delta lengths: \n";
        std::cout << deltaLengths << std::endl << std::endl;
        if (checkForCollision(deltaLengths)) {
            std::cout << "Error: Sensors may go out of range! Motion cancelled." << std::endl;
            return OpcUa_Bad;
        }
        else {
            status = m_pClient->callMethodAsync(std::string("ns=2;s=Panel_0"), UaString("MoveDeltaLengths"), args);
        }
    } else if (offset == PAS_PanelType_MoveToLengths) {
        for (int i = 0; i < 6; i++) {
            UaVariant(args[i]).toFloat(targetLength);
            targetLengths(i) = (double)targetLength;
        }
        std::cout << "Calling Panel::MoveToLengths() with target lengths: \n";
        std::cout << targetLengths << std::endl << std::endl;
        Eigen::VectorXd currentLengths = getActuatorLengths();
        deltaLengths = targetLengths - currentLengths;
        if (checkForCollision(deltaLengths)) {
            std::cout << "Error: Sensors may go out of range! Motion cancelled." << std::endl;
            return OpcUa_Bad;
        }
        else {
            status = m_pClient->callMethodAsync(std::string("ns=2;s=Panel_0"), UaString("MoveToLengths"), args);
        }
    } else if (offset == PAS_PanelType_MoveToCoords) {
        std::cout << "\tCurrent panel coordinates (x, y ,z xRot, yRot, zRot):\n\t\t";
        for (auto coord : m_curCoords) {
            std::cout << coord << " ";
        }
        std::cout << std::endl << std::endl;

        double inputCoordinates[6];
        std::cout << "\tTarget panel coordinates (x, y ,z xRot, yRot, zRot):\n\t\t";
        for (int i = 0; i < 6; i++) {
            UaVariant(args[i]).toDouble(inputCoordinates[i]);
            std::cout << inputCoordinates[i] << " ";
        }
        std::cout << std::endl << std::endl;

        // find actuator lengths needed
        m_SP.ComputeActsFromPanel(inputCoordinates);

        UaVariantArray lengthArgs;
        lengthArgs.create(6);
        UaVariant val;

        // Get actuator lengths for motion
        std::cout << "Moving actuators to lengths:" << std::endl;
        for (int i = 0; i < 6; i++) {
            targetLengths(i) = (float) m_SP.GetActLengths()[i];
            val.setFloat(targetLengths(i));
            val.copyTo(&lengthArgs[i]);
            std::cout << lengthArgs[i].Value.Float << std::endl;
        }
        
        Eigen::VectorXd currentLengths = getActuatorLengths();
        deltaLengths = targetLengths - currentLengths;
        if (checkForCollision(deltaLengths)) {
            std::cout << "Error: Sensors may go out of range! Motion cancelled." << std::endl;
            return OpcUa_Bad;
        }
        else {
            status = m_pClient->callMethodAsync(std::string("ns=2;s=Panel_0"), UaString("MoveToLengths"), lengthArgs);
        }
    } else if (offset == PAS_PanelType_ReadPosition) {
        status = updateCoords(false);
        std::cout << std::endl << m_ID << " :" << std::endl;
        std::cout << "\tCurrent coordinates (x, y ,z xRot, yRot, zRot):\n\t\t";
        for (auto coord : m_curCoords) {
            std::cout << coord << " ";
        }
        std::cout << std::endl << std::endl;

        std::cout << "\tCurrent actuator lengths:\n";
        std::cout << getActuatorLengths() << std::endl;
    }
        /************************************************
         * stop the motion in progress                  *
         * **********************************************/
    else if (offset == PAS_PanelType_Stop) {
        std::cout << m_ID << "::Operate() : Attempting to gracefully stop the motion." << std::endl;
        status = m_pClient->callMethodAsync(std::string("ns=2;s=Panel_0"), UaString("Stop"));
    } else if (offset == PAS_PanelType_TurnOn) {
        std::cout << m_ID << "::Operate() : Turning on..." << std::endl;
        status = m_pClient->callMethod(std::string("ns=2;s=Panel_0"), UaString("TurnOn"));
    } else if (offset == PAS_PanelType_TurnOff) {
        std::cout << m_ID << "::Operate() : Turning off..." << std::endl;
        status = m_pClient->callMethod(std::string("ns=2;s=Panel_0"), UaString("TurnOff"));
    } else if (offset == PAS_PanelType_FindHome) {
        if (__getDeviceState() != Device::DeviceState::On) {
            std::cout << m_ID << " :: PanelController::operate() : Device is in a bad state (busy, off) and "
                                 "could not execute command. Check state and try again. \n";
            return OpcUa_BadInvalidState;
        }
        std::cout << m_ID << "::Operate() : Finding home position." << std::endl;
        status = m_pClient->callMethodAsync(std::string("ns=2;s=Panel_0"), UaString("FindHome"), args);
    } else if (offset == PAS_PanelType_ClearError) {
        status = m_pClient->callMethod(std::string("ns=2;s=Panel_0"), UaString("ClearError"), args);
    } else if (offset == PAS_PanelType_ClearAllErrors) {
        status = m_pClient->callMethod(std::string("ns=2;s=Panel_0"), UaString("ClearAllErrors"));
    } else if (offset == PAS_PanelType_ClearActuatorErrors) {
        status = m_pClient->callMethod(std::string("ns=2;s=Panel_0"), UaString("ClearActuatorErrors"));
    } else if (offset == PAS_PanelType_ClearPlatformErrors) {
        status = m_pClient->callMethod(std::string("ns=2;s=Panel_0"), UaString("ClearPlatformErrors"));
    } else {
        status = OpcUa_BadInvalidArgument;
    }

    return status;
}

bool PanelController::checkForCollision(const Eigen::VectorXd &deltaLengths) {
    std::cout << "Checking for collision...\n";
    ///
    /// This part predicts the new laser spot positions before we move the actuators.
    /// Will make a new function of this and allow all actuator operations to call this function before moving.
    ///
    Eigen::MatrixXd M_response; // response matrix
    Eigen::VectorXd Sen_current; // sensor current position
    Eigen::VectorXd Sen_delta; // sensor delta
    Eigen::VectorXd Sen_new; // sensor new position = Sen_delta + current sensor reading
    Eigen::VectorXd Sen_center(6); // center of sensor camera
    Sen_center << 160.0, 120.0, 160.0, 120.0, 160.0, 120.0;
    Eigen::VectorXd Sen_deviation; // deviated sensor reading

    Eigen::VectorXd currentLengths = getActuatorLengths();

    std::cout << "Current Act lengths are: \n" << currentLengths << std::endl;
    std::cout << "Delta Act lengths are: \n" << deltaLengths << std::endl;
    std::cout << "New Act lengths are: \n" << currentLengths + deltaLengths << std::endl;

    bool collision = false;
    if (m_pChildren.count(PAS_EdgeType) > 0) {
        std::cout << m_pChildren.count(PAS_EdgeType) << " edges found." << std::endl;
        for (const auto &e : m_pChildren.at(PAS_EdgeType)) {     
            std::shared_ptr<EdgeController> edge = std::dynamic_pointer_cast<EdgeController>(e);
            M_response = edge->getResponseMatrix(m_ID.position);

            Sen_current = edge->getCurrentReadings();
            std::cout << "Looking at edge " << edge->getId() << std::endl;
            if (Sen_current.size() == 0) {
                std::cout << "No MPES found for edge " << edge->getId() << ", skipping..." << std::endl;
                continue;
            }
            std::cout << "\nCurrent MPES readings:\n" << Sen_current << std::endl << std::endl;
            std::cout << "\nActuator response matrix for this edge:\n" << M_response << std::endl;
            Sen_delta = M_response * deltaLengths;
            Sen_new = Sen_delta + Sen_current;
            std::cout << "The new sensor coordinates (x, y) will be:\n" << Sen_new << std::endl;
            Sen_deviation = Sen_new - Sen_center;
            std::cout << "\n will deviate from the center position by\n" << Sen_deviation << std::endl;
            std::cout << "The absolute distance from the center for each sensor is: \n";
            double deviation;
            for (unsigned i = 0; i < Sen_deviation.size() / 2; i++) {
                deviation = pow(pow(Sen_deviation(2 * i), 2) + pow(Sen_deviation(2 * i + 1), 2), 0.5);
                std::cout << deviation;
                if (deviation > m_safetyRadius) {
                    std::cout << " [WARNING: Deviation is greater than safety radius (" << m_safetyRadius << " px)";
                    collision = true;
                }
                std::cout << std::endl;
            }
        }
    }
    return collision;
}

UaStatus PanelController::updateCoords(bool printout) {
    Eigen::VectorXd currentLengths(6);

    UaStatus status = __getActuatorLengths(currentLengths);
    if (status.isBad()) {
        std::cout << m_ID << " :: PanelController::updateCoords() : Failed to get actuator lengths." << std::endl;
        return status;
    }

    if (printout) {
        std::cout << m_ID << ": " << std::endl;
        std::cout << "Current actuator lengths are:\n";
        std::cout << currentLengths << std::endl;
    }
    // update current coordinates
    m_SP.ComputeStewart(currentLengths.data());
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

Eigen::VectorXd PanelController::getActuatorLengths() {
    UaStatus status;

    UaVariant val;
    auto &actuatorPositionMap = m_ChildrenPositionMap.at(PAS_ACTType);
    auto &pACT = m_pChildren.at(PAS_ACTType);

    double l;

    Eigen::VectorXd actuatorLengths(6);
    for (const auto &i : actuatorPositionMap) {
        status = pACT.at(i.second)->getData(PAS_ACTType_CurrentLength, val);
        val.toDouble(l);
        actuatorLengths(i.first - 1) = l;
    }

    return actuatorLengths;
}

UaStatus PanelController::__getActuatorLengths(Eigen::VectorXd &lengths) {
    UaStatus status;

    UaVariant val;
    auto &actuatorPositionMap = m_ChildrenPositionMap.at(PAS_ACTType);
    auto &pACT = m_pChildren.at(PAS_ACTType);

    double l;

    lengths.resize(6);
    for (const auto &i : actuatorPositionMap) {
        status = pACT.at(i.second)->getData(PAS_ACTType_CurrentLength, val);
        if (status.isBad()) {
            return status;
        }
        val.toDouble(l);
        lengths(i.first - 1) = l;
    }

    return status;
}

Device::DeviceState PanelController::__getDeviceState() {
    Device::DeviceState state;
    getState(state);

    return state;
}

Device::ErrorState PanelController::__getErrorState() {
    UaVariant var;
    int err;
    getData(PAS_ACTType_ErrorState, var);
    var.toInt32(err);
    return static_cast<Device::ErrorState>(err);
}
