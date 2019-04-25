#include "server/controllers/panelcontroller.hpp"

#include <array>
#include <iostream>
#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"

#include "common/opcua/passervertypeids.hpp"

// @details Sets the state to On.
PanelController::PanelController(Identity identity, std::shared_ptr<Platform> pPlatform) : PasController(identity,
                                                                                                         std::move(pPlatform)) {
    m_state = PASState::On;
}

// @details Sets the state to Off.
PanelController::~PanelController() {
    m_state = PASState::Off;
}

// @details Locks the shared mutex while reading the state.
UaStatus PanelController::getState(PASState &state) {
    //UaMutexLocker lock(&m_mutex);
    updateState();
    state = m_state;
    return OpcUa_Good;
}

// @details Locks the shared mutex while updating the state.
UaStatus PanelController::updateState() {
    //UaMutexLocker lock(&m_mutex);
    // update internal state to match the underlying platform object
    switch (m_pPlatform->getState()) {
        case PlatformState::On :
            m_state = PASState::On;
            break;
        case PlatformState::Off :
            m_state = PASState::Off;
            break;
        case PlatformState::Busy :
            m_state = PASState::Busy;
            break;
        case PlatformState::OperableError :
            m_state = PASState::OperableError;
            break;
        case PlatformState::FatalError :
            m_state = PASState::FatalError;
            break;
        default :
            return OpcUa_BadInvalidState;
    }

    return OpcUa_Good;
}

// @details Locks the shared mutex wihle setting the state. Only the On and Off states are allowed to be set manually.
UaStatus PanelController::setState(PASState state) {
    //UaMutexLocker lock(&m_mutex);

    switch (state) {
        case PASState::On:
            m_state = state;
            m_pPlatform->setState(PlatformState::On);
            break;
        case PASState::Off:
            m_state = state;
            m_pPlatform->setState(PlatformState::Off);
            break;
        case PASState::FatalError:
            return OpcUa_BadInvalidArgument;
        case PASState::OperableError:
            return OpcUa_BadInvalidArgument;
        case PASState::Busy:
            return OpcUa_BadInvalidArgument;
        default:
            return OpcUa_BadInvalidArgument;
    }

    return OpcUa_Good;
}

/// @details Locks the shared mutex while setting the state. In SIMMODE,
/// when reading temperature values, dummy values are generated from a normal distribution.
UaStatus PanelController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);

#ifdef SIMMODE
    std::random_device rd{};
    std::mt19937 generator{rd()};

    std::normal_distribution<double> internalTempDistribution(30.0, 2.0);
    std::normal_distribution<double> externalTempDistribution(20.0, 2.0);

    double newValue;
    if (offset == PAS_PanelType_ExtTemperature) {
        newValue = externalTempDistribution(generator);
        value.setFloat(newValue);
    } else if (offset == PAS_PanelType_IntTemperature) {
        newValue = internalTempDistribution(generator);
        value.setFloat(newValue);
    } else {
        return OpcUa_BadInvalidArgument;
    }

#else
    if (offset == PAS_PanelType_ExtTemperature) {
        value.setFloat(m_pPlatform->ReadExternalTemperature());
    }
    else if (offset == PAS_PanelType_IntTemperature) {
        value.setFloat(m_pPlatform->ReadInternalTemperature());
    }
    else {
        return OpcUa_BadInvalidArgument;
    }
#endif
    return OpcUa_Good;
}

/// @details This method does nothing as panels have no writeable variables.
UaStatus PanelController::setData(OpcUa_UInt32 offset, UaVariant value) {
    return OpcUa_BadNotImplemented;
}

/// @details Updates the state and then checks it before attempting to call any methods. When the state is FatalError or Busy, prevents panel operation.
/// If the stop method is called, sets the platform state to off in order to halt any motion.
UaStatus PanelController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    UaStatus status;

    updateState(); // Update the current state

    if (m_state == PASState::FatalError) {
        std::cout << "PanelController::Operate(): Panel in fatal error state! "
                  << "Check what's wrong, fix it, and try again.\n";
        return OpcUa_BadInvalidState;
    }

    if (offset == PAS_PanelType_MoveDeltaLengths) {
        if (args.length() != 6) {
            return OpcUa_BadInvalidArgument;
        }
        if (m_state == PASState::Off)
            setState(PASState::On);
        else if (m_state == PASState::Busy) {
            std::cout << "PanelController::Operate(): Busy at the moment. "
                      << "Wait for the current operation to finish and try again.\n";
            return OpcUa_BadInvalidState;
        }

        m_state = PASState::Busy; // set the state immediately

        std::array<float, 6> deltaLengths{};

        std::cout << "Received MoveDeltaLengths command with delta lengths: " << std::endl;
        UaVariant dL;
	float targetLength;
        for (int i = 0; i < 6; i++) {
            dL = UaVariant(args[i]);
            dL.toFloat(deltaLengths[i]);
            status = m_pActuators.at(i)->getData(PAS_ACTType_CurrentLength, dL);
            dL.toFloat(targetLength);
	    targetLength += deltaLengths[i];
	    m_pActuators.at(i)->setTargetLength(targetLength);
            std::cout << deltaLengths[i] << std::endl;
        }
        std::cout << std::endl;

       	deltaLengths = m_pPlatform->MoveDeltaLengths(deltaLengths);
        
	std::cout << "Missed targets by: " << std::endl;
	float remainingLength;
	for (int i=0; i < 6; i++) {
            m_pActuators.at(i)->setDeltaLength(deltaLengths[i]);
            status = m_pActuators.at(i)->getData(PAS_ACTType_DeltaLength, dL);
	    dL.toFloat(remainingLength);
            std::cout << remainingLength << std::endl;
        }
	std::cout << std::endl;
        status = OpcUa_Good;
    } 
    else if (offset == PAS_PanelType_MoveToLengths) {
        if (args.length() != 6) {
            return OpcUa_BadInvalidArgument;
        }
        if (m_state == PASState::Off)
            setState(PASState::On);
        else if (m_state == PASState::Busy) {
            std::cout << "PanelController::Operate(): Busy at the moment. "
                      << "Wait for the current operation to finish and try again.\n";
            return OpcUa_Good;
        }

        m_state = PASState::Busy; // set the state immediately

        std::cout << "Called Panel::MoveToLengths with target lengths: " << std::endl;
	
        std::array<float, 6> lengths{};
        UaVariant len;
	float l;
        for (int i = 0; i < 6; i++) {
            len = UaVariant(args[i]);
            len.toFloat(lengths[i]);
	    m_pActuators.at(i)->setTargetLength(lengths[i]);
	    std::cout << lengths[i] << std::endl;
        }
	std::cout << std::endl;

        lengths = m_pPlatform->MoveToLengths(lengths);

        std::cout << "Missed targets by: " << std::endl;
	for (int i=0; i < 6; i++) {
            m_pActuators.at(i)->setDeltaLength(lengths[i]);
            status = m_pActuators.at(i)->getData(PAS_ACTType_DeltaLength, len);
	    len.toFloat(l);
            std::cout << l << std::endl;
        }
	std::cout << std::endl;

        status = OpcUa_Good;
    } else if (offset == PAS_PanelType_Stop) {
        std::cout << "PanelController::Operate(): Attempting to gracefully stop the motion.\n";
        status = setState(PASState::Off);
    } else {
        status = OpcUa_BadInvalidArgument;
    }

    updateState(); // update the state again

    return status;
}
