#include "client/controllers/panelcontroller.hpp"

#include <chrono>
#include <string>

#include <Eigen/Dense>

#include "common/alignment/device.hpp"

#include "client/clienthelper.hpp"
#include "client/objects/panelobject.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"


PanelController::PanelController(Device::Identity identity, Client *pClient, bool isSubclient) :
    PasCompositeController(std::move(identity), pClient, 5000), m_isSubclient(isSubclient) {
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

    std::vector<std::string> vec_curread{m_pClient->getDeviceNodeId(m_ID) + ".State"};
    status = m_pClient->read(vec_curread, &value);
    value.toInt32(v);

    state = static_cast<Device::DeviceState>(v);
    spdlog::trace("{} : Read device state => ({})", m_ID, Device::deviceStateNames.at(state));

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
        if (__expired() && !m_isSubclient) {
            spdlog::debug("{} : PanelController::operate() : Panel coordinate data is expired, updating coordinates...",
                          m_ID);
            status = updateCoords();
            if (status.isBad()) {
                spdlog::error("{} : PanelController::operate() : Failed to update coordinate data.", m_ID);
                return status;
            }
        }
        int dataOffset = offset - PAS_PanelType_x;
        value.setDouble(m_curCoords[dataOffset]);

        switch (offset) {
            case PAS_PanelType_x:
                spdlog::trace("{} : Read x value => ({})", m_ID, m_curCoords[0]);
                break;
            case PAS_PanelType_y:
                spdlog::trace("{} : Read y value => ({})", m_ID, m_curCoords[1]);
                break;
            case PAS_PanelType_z:
                spdlog::trace("{} : Read z value => ({})", m_ID, m_curCoords[2]);
                break;
            case PAS_PanelType_xRot:
                spdlog::trace("{} : Read xRot value => ({})", m_ID, m_curCoords[3]);
                break;
            case PAS_PanelType_yRot:
                spdlog::trace("{} : Read yRot value => ({})", m_ID, m_curCoords[4]);
                break;
            case PAS_PanelType_zRot:
                spdlog::trace("{} : Read zRot value => ({})", m_ID, m_curCoords[5]);
                break;
            default:
                break;
        }
    } else if (offset == PAS_PanelType_IntTemperature) {
        status = m_pClient->read({m_pClient->getDeviceNodeId(m_ID) + ".InternalTemperature"}, &value);
        spdlog::trace("{} : Read Internal Temperature value => ({})", m_ID, value[0].Value.Double);
    } else if (offset == PAS_PanelType_ExtTemperature) {
        status = m_pClient->read({m_pClient->getDeviceNodeId(m_ID) + ".ExternalTemperature"}, &value);
        spdlog::trace("{} : Read External Temperature value => ({})", m_ID, value[0].Value.Double);
    } else if (offset == PAS_PanelType_SafetyRadius) {
        value.setDouble(m_safetyRadius);
        spdlog::trace("{} : Read safety radius value => ({})", m_ID, m_safetyRadius);
    } else if (offset == PAS_PanelType_Position) {
        status = m_pClient->read({m_pClient->getDeviceNodeId(m_ID) + ".Position"}, &value);
        spdlog::trace("{} : Read position value => ({})", m_ID, value[0].Value.Int32);
    } else if (offset == PAS_PanelType_Serial) {
        status = m_pClient->read({m_pClient->getDeviceNodeId(m_ID) + ".Serial"}, &value);
        spdlog::trace("{} : Read serial value => ({})", m_ID, value[0].Value.Int32);
    } else if (offset == PAS_PanelType_ErrorState) {
        status = m_pClient->read({m_pClient->getDeviceNodeId(m_ID) + ".ErrorState"}, &value);
        spdlog::trace("{} : Read Error State value => ({})", m_ID, value[0].Value.UInt32);
    }
    else
        status = OpcUa_BadInvalidArgument;

    if (status == OpcUa_BadInvalidState) {
        spdlog::error("{} : PanelController::operate() : Device is in a bad state (busy, off, error) and "
                      "could not read data. Check state and try again.", m_ID);
    }

    return status;
}

UaStatus PanelController::getError(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (PanelObject::ERRORS.count(offset) > 0) {
        std::string varName = std::get<0>(PanelObject::ERRORS.at(offset));
        std::vector<std::string> varsToRead = {m_pClient->getDeviceNodeId(m_ID) + varName};
        status = m_pClient->read(varsToRead, &value);
        spdlog::trace("{} : Read error {} value => ({})", m_ID, offset, value[0].Value.Boolean);
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
                m_ID);
            return OpcUa_BadInvalidState;
        }
        // Update coordinates before doing any calculations
        spdlog::trace("{} : PanelController::operate() : Updating coordinate data before calling method.", m_ID);
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
        spdlog::debug("{} : PanelController::operate() : Executing moveDeltaLengths() with delta lengths :\n{}\n", m_ID,
                      deltaLengths);
        if (checkForCollision(deltaLengths)) {
            return OpcUa_Bad;
        }
        else {
            status = m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_ID), UaString("MoveDeltaLengths"), args);
        }
    } else if (offset == PAS_PanelType_MoveToLengths) {
        for (int i = 0; i < 6; i++) {
            UaVariant(args[i]).toFloat(targetLength);
            targetLengths(i) = (double)targetLength;
        }
        spdlog::debug("{} : PanelController::operate() : Executing moveToLengths() with target lengths :\n{}\n", m_ID,
                      targetLengths);
        Eigen::VectorXd currentLengths = getActuatorLengths();
        deltaLengths = targetLengths - currentLengths;
        if (checkForCollision(deltaLengths)) {
            return OpcUa_Bad;
        }
        else {
            status = m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_ID), UaString("MoveToLengths"), args);
        }
    } else if (offset == PAS_PanelType_MoveToCoords) {
        spdlog::debug(
            "{} : PanelController::operate() : Current panel coordinates (x, y ,z xRot, yRot, zRot):\n {} {} {} {} {} {}\n",
            m_ID, m_curCoords[0], m_curCoords[1], m_curCoords[2], m_curCoords[3], m_curCoords[4], m_curCoords[5]);

        double inputCoordinates[6];
        for (int i = 0; i < 6; i++) {
            UaVariant(args[i]).toDouble(inputCoordinates[i]);
        }
        spdlog::debug(
            "{} : PanelController::operate() : Target panel coordinates (x, y ,z xRot, yRot, zRot):\n {} {} {} {} {} {}\n",
            m_ID, inputCoordinates[0], inputCoordinates[1], inputCoordinates[2], inputCoordinates[3],
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
        spdlog::debug("{} : PanelController::operate() : Moving actuators to lengths:\n {}\n", m_ID, targetLengths);
        
        Eigen::VectorXd currentLengths = getActuatorLengths();
        deltaLengths = targetLengths - currentLengths;
        if (checkForCollision(deltaLengths)) {
            return OpcUa_Bad;
        }
        else {
            status = m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_ID), UaString("MoveToLengths"),
                                                lengthArgs);
        }
    } else if (offset == PAS_PanelType_ReadPosition) {
        status = updateCoords(false);
        spdlog::info(
            "{} : PanelController::operate() : Current panel coordinates (x, y ,z xRot, yRot, zRot):\n {} {} {} {} {} {}\n",
            m_ID, m_curCoords[0], m_curCoords[1], m_curCoords[2], m_curCoords[3], m_curCoords[4], m_curCoords[5]);

        spdlog::info("{} : PanelController::operate() : Current Actuator Lengths :\n{}\n", m_ID, getActuatorLengths());
    }
        /************************************************
         * stop the motion in progress                  *
         * **********************************************/
    else if (offset == PAS_PanelType_Stop) {
        spdlog::info("{} : Panel controller calling stop()", m_ID);
        status = m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_ID), UaString("Stop"));
    } else if (offset == PAS_PanelType_TurnOn) {
        spdlog::info("{} : Panel controller calling turnOn()", m_ID);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("TurnOn"));
    } else if (offset == PAS_PanelType_TurnOff) {
        spdlog::info("{} : Panel controller calling turnOff()", m_ID);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("TurnOff"));
    } else if (offset == PAS_PanelType_FindHome) {
        if (__getDeviceState() != Device::DeviceState::On) {
            spdlog::error("{} : PanelController::operate() : Device is in a bad state (busy, off, error) and "
                          "could not execute findHome command. Check state and try again.", m_ID);
            return OpcUa_BadInvalidState;
        }
        spdlog::info("{} : Panel controller calling findHome() with direction {}", m_ID, args[0].Value.Int32);
        status = m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_ID), UaString("FindHome"), args);
    } else if (offset == PAS_PanelType_ClearError) {
        spdlog::info("{} : Panel controller calling clearError() for error {}", m_ID, args[0].Value.Int32);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("ClearError"), args);
    } else if (offset == PAS_PanelType_ClearAllErrors) {
        spdlog::info("{} : Panel controller calling clearAllErrors()", m_ID);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("ClearAllErrors"));
    } else if (offset == PAS_PanelType_ClearActuatorErrors) {
        spdlog::info("{} : Panel controller calling clearActuatorErrors()", m_ID);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("ClearActuatorErrors"));
    } else if (offset == PAS_PanelType_ClearPlatformErrors) {
        spdlog::info("{} : Panel controller calling clearPlatformErrors()", m_ID);
        status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_ID), UaString("ClearPlatformErrors"));
    } else {
        status = OpcUa_BadInvalidArgument;
    }

    return status;
}

bool PanelController::checkForCollision(const Eigen::VectorXd &deltaLengths) {
    spdlog::debug("{} : Doing collision check for motion (delta lengths) of :\n {}.", m_ID, deltaLengths);

    Eigen::MatrixXd M_response; // response matrix
    Eigen::VectorXd Sen_current; // sensor current position
    Eigen::VectorXd Sen_delta; // sensor delta
    Eigen::VectorXd Sen_new; // sensor new position = Sen_delta + current sensor reading
    Eigen::VectorXd Sen_center(6); // center of sensor camera
    Sen_center << 160.0, 120.0, 160.0, 120.0, 160.0, 120.0; // Hardcoded
    Eigen::VectorXd Sen_deviation; // deviated sensor reading

    Eigen::VectorXd currentLengths = getActuatorLengths();

    std::ostringstream os;
    for (int i=0; i<6; i++) {
        os << currentLengths(i) << " (" << deltaLengths(i) << ") => " << currentLengths(i) + deltaLengths(i)
           << std::endl;
    }
    spdlog::debug("{} : Current length (Delta length) => Target length\n {}", m_ID, os.str());
    os.str(""); // Clear ostringstream

    bool collision = false;
    if (m_pChildren.count(PAS_EdgeType) > 0) {
        os << "\n" << m_pChildren.count(PAS_EdgeType) << " edges found." << std::endl;
        for (const auto &e : m_pChildren.at(PAS_EdgeType)) {     
            std::shared_ptr<EdgeController> edge = std::dynamic_pointer_cast<EdgeController>(e);
            M_response = edge->getResponseMatrix(m_ID.position);
            Sen_current = edge->getCurrentReadings().first;
            os << "Looking at edge " << edge->getId() << std::endl;
            if (Sen_current.size() == 0) {
                os << "No MPES found for edge " << edge->getId() << ", skipping..." << std::endl;
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
        spdlog::error("{} : Collision check indicates that sensors may go out of range! Motion cancelled.", m_ID);
    }
    else {
        spdlog::debug("{} : Collision check passed, motion should be safe.", m_ID);
    }
    return collision;
}

UaStatus PanelController::updateCoords(bool print) {
    Eigen::VectorXd currentLengths(6);

    UaStatus status = __getActuatorLengths(currentLengths);
    if (status.isBad()) {
        spdlog::error("{} : PanelController::updateCoords() : Failed to get actuator lengths.", m_ID);
        return status;
    }

    if (print) {
        spdlog::info("{} : PanelController::updateCoords() : Current Actuator Lengths :\n{}\n", m_ID, currentLengths);
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

    Eigen::VectorXd actuatorLengths(6);
    __getActuatorLengths(actuatorLengths);

    return actuatorLengths;
}

UaStatus PanelController::__getActuatorLengths(Eigen::VectorXd &lengths) {
    UaStatus status;

    spdlog::trace("{} : PanelController : Getting actuator lengths...", m_ID);

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
