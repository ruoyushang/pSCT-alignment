#include "client/controllers/panelcontroller.hpp"

#include <chrono>
#include <sstream>
#include <string>

#include <Eigen/Dense>

#include "common/alignment/device.hpp"

#include "client/clienthelper.hpp"
#include "client/objects/panelobject.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"


PanelController::PanelController(Device::Identity identity, Client *pClient, std::string mode) :
    PasCompositeController(std::move(identity), pClient, 5000), m_mode(mode) {
    m_SP.SetPanelType(StewartPlatform::PanelType::OPT);

    // define possible children types
    m_ChildrenTypes = {PAS_ACTType, PAS_MPESType, PAS_EdgeType};

    // make sure things update on the first boot up
    // duration takes seconds -- hence the conversion with the 1/1000 ratio
    m_LastUpdateTime = TIME::now() - std::chrono::duration<int, std::ratio<1, 1000>>
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

    std::vector<std::string> vec_curread{m_pClient->getDeviceNodeId(m_Identity) + ".State"};
    status = m_pClient->read(vec_curread, &value);
    value.toInt32(v);

    state = static_cast<Device::DeviceState>(v);
    spdlog::trace("{} : Read device state => ({})", m_Identity, (int)state);

    return status;
}

UaStatus PanelController::setState(Device::DeviceState state) {
    return OpcUa_BadNotWritable;
}

UaStatus PanelController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (PanelObject::ERRORS.count(offset) > 0) {
        return getError(offset, value);
    } else if (offset >= PAS_PanelType_x && offset <= PAS_PanelType_zRot) {
        // update current coordinates
        if (__expired()) {
            spdlog::debug("{} : PanelController::operate() : Panel coordinate data is expired, updating coordinates...",
                          m_Identity);
            status = updateCoords();
            if (status.isBad()) {
                spdlog::error("{} : PanelController::operate() : Failed to update coordinate data.", m_Identity);
                return status;
            }
        }
        int dataOffset = offset - PAS_PanelType_x;
        value.setDouble(m_curCoords[dataOffset]);

        switch (offset) {
            case PAS_PanelType_x:
                spdlog::trace("{} : Read x value => ({})", m_Identity, m_curCoords[0]);
                break;
            case PAS_PanelType_y:
                spdlog::trace("{} : Read y value => ({})", m_Identity, m_curCoords[1]);
                break;
            case PAS_PanelType_z:
                spdlog::trace("{} : Read z value => ({})", m_Identity, m_curCoords[2]);
                break;
            case PAS_PanelType_xRot:
                spdlog::trace("{} : Read xRot value => ({})", m_Identity, m_curCoords[3]);
                break;
            case PAS_PanelType_yRot:
                spdlog::trace("{} : Read yRot value => ({})", m_Identity, m_curCoords[4]);
                break;
            case PAS_PanelType_zRot:
                spdlog::trace("{} : Read zRot value => ({})", m_Identity, m_curCoords[5]);
                break;
            default:
                break;
        }
    } else if (offset == PAS_PanelType_IntTemperature) {
        status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + ".InternalTemperature"}, &value);
        double temp;
        value.toDouble(temp);
        spdlog::trace("{} : Read Internal Temperature value => ({})", m_Identity, temp);
    } else if (offset == PAS_PanelType_ExtTemperature) {
        status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + ".ExternalTemperature"}, &value);
        double temp;
        value.toDouble(temp);
        spdlog::trace("{} : Read External Temperature value => ({})", m_Identity, temp);
    } else if (offset == PAS_PanelType_SafetyRadius) {
        value.setDouble(m_safetyRadius);
        spdlog::trace("{} : Read safety radius value => ({})", m_Identity, m_safetyRadius);
    } else if (offset == PAS_PanelType_Position) {
        status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + ".Position"}, &value);
        int temp;
        value.toInt32(temp);
        spdlog::trace("{} : Read Position value => ({})", m_Identity, temp);
    } else if (offset == PAS_PanelType_Serial) {
        status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + ".Serial"}, &value);
        int temp;
        value.toInt32(temp);
        spdlog::trace("{} : Read Serial value => ({})", m_Identity, temp);
    } else if (offset == PAS_PanelType_ErrorState) {
        status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + ".ErrorState"}, &value);
        int temp;
        value.toInt32(temp);
        spdlog::trace("{} : Read ErrorState value => ({})", m_Identity, Device::errorStateNames.at(static_cast<Device::ErrorState>(temp)));
    }
    else
        status = OpcUa_BadInvalidArgument;

    if (status == OpcUa_BadInvalidState) {
        spdlog::error("{} : PanelController::operate() : Device is in a bad state (busy, off, error) and "
                      "could not read data. Check state and try again.", m_Identity);
    }

    return status;
}

UaStatus PanelController::getError(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (PanelObject::ERRORS.count(offset) > 0) {
        std::string varName = std::get<0>(PanelObject::ERRORS.at(offset));
        std::vector<std::string> varsToRead = {m_pClient->getDeviceNodeId(m_Identity) + "." + varName};
        status = m_pClient->read(varsToRead, &value);
        unsigned char temp;
        value.toBool(temp);
        spdlog::trace("{} : Read error {} value => ({})", m_Identity, offset, (bool)temp);
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

    if (offset == PAS_PanelType_MoveToLengths || offset == PAS_PanelType_MoveToCoords || offset == PAS_PanelType_MoveDeltaLengths) {
        if (__getErrorState() == Device::ErrorState::FatalError || __getDeviceState() != Device::DeviceState::On) {
            spdlog::error(
                "{} : PanelController::operate() : Device is in a bad state (busy, off, error). Method call aborted. Check state and try again.",
                m_Identity);
            return OpcUa_BadInvalidState;
        }
        // Update coordinates before doing any calculations
        spdlog::trace("{} : PanelController::operate() : Updating coordinate data before calling method.", m_Identity);
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
        spdlog::debug("{} : PanelController::operate() : Executing moveDeltaLengths() with delta lengths :\n{}\n\n",
                      m_Identity,
                      deltaLengths);
        if (m_mode == "client" && checkForCollision(deltaLengths)) {
            return OpcUa_Bad;
        }
        else {
            status = m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_Identity), UaString("MoveDeltaLengths"),
                                                args);
        }

        UaThread::sleep(2.0);
        bool stillMoving = true;
        while (stillMoving) {
            stillMoving = false;
            if (__getDeviceState() != Device::DeviceState::On)
            {
                stillMoving = true;
                spdlog::trace("{}: PanelController::operate() : still moving...\n", m_Identity);
                UaThread::sleep(2.0);
            }
        }
        spdlog::debug("{}: PanelController::operate() : Successfully finished MoveDeltaLengths.\n", m_Identity);

    } else if (offset == PAS_PanelType_MoveToLengths) {
        for (int i = 0; i < 6; i++) {
            UaVariant(args[i]).toFloat(targetLength);
            targetLengths(i) = (double)targetLength;
        }
        spdlog::debug("{} : PanelController::operate() : Executing moveToLengths() with target lengths :\n{}\n\n",
                      m_Identity,
                      targetLengths);
        Eigen::VectorXd currentLengths;
        status = __getActuatorLengths(currentLengths);

        if (status.isBad()) {
            spdlog::error("{}: Unable to moveToLengths, failed to get actauator lengths.", m_Identity);
            return OpcUa_Bad;
        }

        deltaLengths = targetLengths - currentLengths;
        if (m_mode == "client" && checkForCollision(deltaLengths)) {
            return OpcUa_Bad;
        }
        else {
            status = m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_Identity), UaString("MoveToLengths"),
                                                args);
        }
    } else if (offset == PAS_PanelType_MoveToCoords) {
        spdlog::debug(
            "{} : PanelController::operate() : Current panel coordinates (x, y ,z xRot, yRot, zRot):\n{} {} {} {} {} {}\n",
            m_Identity, m_curCoords[0], m_curCoords[1], m_curCoords[2], m_curCoords[3], m_curCoords[4], m_curCoords[5]);

        double inputCoordinates[6];
        for (int i = 0; i < 6; i++) {
            UaVariant(args[i]).toDouble(inputCoordinates[i]);
        }
        spdlog::debug(
            "{} : PanelController::operate() : Target panel coordinates (x, y ,z xRot, yRot, zRot):\n{} {} {} {} {} {}\n",
            m_Identity, inputCoordinates[0], inputCoordinates[1], inputCoordinates[2], inputCoordinates[3],
            inputCoordinates[4], inputCoordinates[5]);

        // find actuator lengths needed
        m_SP.ComputeActsFromPanel(inputCoordinates);

        UaVariantArray lengthArgs;
        lengthArgs.create(6);
        UaVariant val;

        // Get actuator lengths for motion
        for (int i = 0; i < 6; i++) {
            targetLengths(i) = (float) m_SP.GetActLengths()[i];
            val.setFloat(targetLengths(i));
            val.copyTo(&lengthArgs[i]);
        }
        spdlog::debug("{} : PanelController::operate() : Moving actuators to lengths:\n{}\n\n", m_Identity,
                      targetLengths);

        Eigen::VectorXd currentLengths;
        status = __getActuatorLengths(currentLengths);

        if (status.isBad()) {
            spdlog::error("{}: Unable to moveToCoords, failed to get actauator lengths.", m_Identity);
            return OpcUa_Bad;
        }

        deltaLengths = targetLengths - currentLengths;
        if (m_mode == "client" && checkForCollision(deltaLengths)) {
            return OpcUa_Bad;
        } else {
            status = m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_Identity), UaString("MoveToLengths"),
                                                lengthArgs);
        }
    } else if (offset == PAS_PanelType_MoveDeltaCoords) {

        status = updateCoords();

        spdlog::debug(
            "{} : PanelController::operate() : Current panel coordinates (x, y ,z xRot, yRot, zRot):\n{} {} {} {} {} {}\n",
            m_Identity, m_curCoords[0], m_curCoords[1], m_curCoords[2], m_curCoords[3], m_curCoords[4], m_curCoords[5]);

        double inputDeltaCoordinates[6];
        for (int i = 0; i < 6; i++) {
            UaVariant(args[i]).toDouble(inputDeltaCoordinates[i]);
        }
        spdlog::debug(
            "{} : PanelController::operate() : Delta panel coordinates (x, y ,z xRot, yRot, zRot):\n{} {} {} {} {} {}\n",
            m_Identity, inputDeltaCoordinates[0], inputDeltaCoordinates[1], inputDeltaCoordinates[2],
            inputDeltaCoordinates[3],
            inputDeltaCoordinates[4], inputDeltaCoordinates[5]);

        double targetCoordinates[6];
        for (int i = 0; i < 6; i++) {
            targetCoordinates[i] = m_curCoords[i] + inputDeltaCoordinates[i];
        }

        spdlog::debug(
            "{} : PanelController::operate() : Target panel coordinates (x, y ,z xRot, yRot, zRot):\n{} {} {} {} {} {}\n",
            m_Identity, targetCoordinates[0], targetCoordinates[1], targetCoordinates[2], targetCoordinates[3],
            targetCoordinates[4], targetCoordinates[5]);

        // find actuator lengths needed
        m_SP.ComputeActsFromPanel(targetCoordinates);

        UaVariantArray lengthArgs;
        lengthArgs.create(6);
        UaVariant val;

        // Get actuator lengths for motion
        for (int i = 0; i < 6; i++) {
            targetLengths(i) = (float) m_SP.GetActLengths()[i];
            val.setFloat(targetLengths(i));
            val.copyTo(&lengthArgs[i]);
        }
        spdlog::debug("{} : PanelController::operate() : Moving actuators to lengths:\n{}\n\n", m_Identity,
                      targetLengths);

        Eigen::VectorXd currentLengths;
        status = __getActuatorLengths(currentLengths);

        if (status.isBad()) {
            spdlog::error("{}: Unable to moveDeltaCoords, failed to get actauator lengths.", m_Identity);
            return OpcUa_Bad;
        }

        deltaLengths = targetLengths - currentLengths;
        if (m_mode == "client" && checkForCollision(deltaLengths)) {
            return OpcUa_Bad;
        } else {
            status = m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_Identity), UaString("MoveToLengths"),
                                                lengthArgs);
        }
    } else if (offset == PAS_PanelType_ReadPosition) {
        status = updateCoords(true);
        spdlog::info(
            "{} : PanelController::operate() : Current panel coordinates (x, y ,z xRot, yRot, zRot):\n{}\n{}\n{}\n{}\n{}\n{}",
            m_Identity, m_curCoords[0], m_curCoords[1], m_curCoords[2], m_curCoords[3], m_curCoords[4], m_curCoords[5]);
    }
        /************************************************
         * stop the motion in progress                  *
         * **********************************************/
    else if (offset == PAS_PanelType_Stop) {
        spdlog::info("{} : PanelController calling stop()", m_Identity);
        status = m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_Identity), UaString("Stop"));
    } else if (offset == PAS_PanelType_TurnOn) {
        spdlog::info("{} : PanelController calling turnOn()", m_Identity);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("TurnOn"));
    } else if (offset == PAS_PanelType_TurnOff) {
        spdlog::info("{} : PanelController calling turnOff()", m_Identity);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("TurnOff"));
    } else if (offset == PAS_PanelType_FindHome) {
        if (__getDeviceState() != Device::DeviceState::On) {
            spdlog::error("{} : PanelController::operate() : Device is in a bad state (busy, off, error) and "
                          "could not execute findHome command. Check state and try again.", m_Identity);
            return OpcUa_BadInvalidState;
        }
        int dir;
        UaVariant(args[0]).toInt32(dir);
        spdlog::info("{} : PanelController calling findHome() with direction {}", m_Identity, dir);
        status = m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_Identity), UaString("FindHome"), args);
    } else if (offset == PAS_PanelType_ClearError) {
        int errorCode;
        UaVariant(args[0]).toInt32(errorCode);
        spdlog::info("{} : PanelController calling clearError() for error {}", m_Identity, errorCode);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("ClearError"), args);
    } else if (offset == PAS_PanelType_ClearAllErrors) {
        spdlog::info("{} : PanelController calling clearAllErrors()", m_Identity);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("ClearAllErrors"));
    } else if (offset == PAS_PanelType_ClearActuatorErrors) {
        spdlog::info("{} : PanelController calling clearActuatorErrors()", m_Identity);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("ClearActuatorErrors"));
    } else if (offset == PAS_PanelType_ClearPlatformErrors) {
        spdlog::info("{} : PanelController calling clearPlatformErrors()", m_Identity);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("ClearPlatformErrors"));
    } else {
        status = OpcUa_BadInvalidArgument;
    }

    return status;
}

bool PanelController::checkForCollision(const Eigen::VectorXd &deltaLengths) {
    spdlog::debug("{} : Doing collision check for motion (delta lengths) of :\n{}\n.", m_Identity, deltaLengths);

    Eigen::MatrixXd M_response; // response matrix
    Eigen::VectorXd Sen_current; // sensor current position
    Eigen::VectorXd Sen_delta; // sensor delta
    Eigen::VectorXd Sen_new; // sensor new position = Sen_delta + current sensor reading
    Eigen::VectorXd Sen_center(6); // center of sensor camera
    Sen_center << 160.0, 120.0, 160.0, 120.0, 160.0, 120.0; // Hardcoded
    Eigen::VectorXd Sen_deviation; // deviated sensor reading

    Eigen::VectorXd currentLengths(6);
    UaStatus status = __getActuatorLengths(currentLengths);

    if (status.isBad()) {
        spdlog::error("{} : Collision check failed due to inability to read actuator lengths. Motion disallowed.",
                      m_Identity);
        return true;
    }

    std::ostringstream os;
    for (int i=0; i<6; i++) {
        os << currentLengths(i) << " (" << deltaLengths(i) << ") => " << currentLengths(i) + deltaLengths(i)
           << std::endl;
    }
    spdlog::debug("{} : Current length (Delta length) => Target length\n {}", m_Identity, os.str());
    os.str(""); // Clear ostringstream

    bool collision = false;
    if (m_pChildren.count(PAS_EdgeType) > 0) {
        os << "\n" << m_pChildren.count(PAS_EdgeType) << " edges found." << std::endl;
        for (const auto &e : m_pChildren.at(PAS_EdgeType)) {     
            std::shared_ptr<EdgeController> edge = std::dynamic_pointer_cast<EdgeController>(e);
            M_response = edge->getResponseMatrix(m_Identity.position);
            Sen_current = edge->getCurrentReadings().first;
            os << "Looking at edge " << edge->getIdentity() << std::endl;
            if (Sen_current.size() == 0) {
                os << "No MPES found for edge " << edge->getIdentity() << ", skipping..." << std::endl;
                continue;
            }
            os << "\nCurrent MPES readings:\n" << Sen_current << std::endl << std::endl;
            os << "Actuator response matrix for this edge:\n" << M_response << std::endl;
            Sen_delta = M_response * deltaLengths;
            Sen_new = Sen_delta + Sen_current;
            Sen_deviation = Sen_new - Sen_center;
            os << "\nNew sensor readings (pixels off from center)\n";
            for (int i=0; i<6; i++) {
                os << Sen_new(i) << " (" << Sen_deviation(i) << ")\n";;
            }
            os << "\nOffset of laser dot from camera center by sensor (px) [safety limit]: \n";
            double deviation;
            for (unsigned i = 0; i < Sen_deviation.size() / 2; i++) {
                deviation = pow(pow(Sen_deviation(2 * i), 2) + pow(Sen_deviation(2 * i + 1), 2), 0.5);
                os << deviation << " [" << m_safetyRadius << "]";
                if (deviation > m_safetyRadius) {
                    os << " (WARNING: Deviation is greater than safety radius)";
                    collision = true;
                }
                os << std::endl;
            }
        }
    }
    spdlog::debug(os.str());

    if (collision) {
        spdlog::error("{} : Collision check indicates that sensors may go out of range! Motion disallowed.",
                      m_Identity);
    }
    else {
        spdlog::debug("{} : Collision check passed, motion should be safe.", m_Identity);
    }
    return collision;
}

UaStatus PanelController::updateCoords(bool print) {
    Eigen::VectorXd currentLengths(6);

    UaStatus status = __getActuatorLengths(currentLengths);
    if (status.isBad()) {
        spdlog::error("{} : PanelController::updateCoords() : Failed to get actuator lengths.", m_Identity);
        return status;
    }

    if (print) {
        std::ostringstream os;
        for (int i = 0; i < 6; i++) {
            os << getChildAtPosition(PAS_ACTType, i)->getIdentity() << ": " << currentLengths(i) << std::endl;
        }

        spdlog::info("{} : PanelController::updateCoords() : Current Actuator Lengths :\n{}\n\n", m_Identity,
                     os.str());
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

    m_LastUpdateTime = TIME::now();
}

Eigen::VectorXd PanelController::getActuatorLengths() {
    UaStatus status;

    Eigen::VectorXd actuatorLengths(6);
    status = __getActuatorLengths(actuatorLengths);

    return actuatorLengths;
}

UaStatus PanelController::__getActuatorLengths(Eigen::VectorXd &lengths) {
    UaStatus status;

    spdlog::trace("{} : PanelController : Getting actuator lengths...", m_Identity);

    UaVariant val;
    auto &actuatorPositionMap = m_ChildrenPositionMap.at(PAS_ACTType);
    double l;

    lengths.resize(6);
    for (const auto &pair : actuatorPositionMap) {
        status = pair.second->getData(PAS_ACTType_CurrentLength, val);
        if (status.isBad()) {
            for (int i = 0; i < 6; i++) {
                lengths(i) = -1;
            }
            return status;
        }
        val.toDouble(l);
        lengths(pair.first - 1) = l;
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
