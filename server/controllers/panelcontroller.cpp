#include "server/controllers/panelcontroller.hpp"

#include <array>
#include <iostream>
#include <memory>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uamutex.h"
#include "uabase/uastring.h"

#include "common/opcua/passervertypeids.h"

// no data for now, so just let these be
PanelController::PanelController(int ID, std::shared_ptr<Platform> pPlatform) : PasController(ID, pPlatform)
{
    m_state = PASState::On;
}

PanelController::~PanelController()
{
    m_state = PASState::Off;
}

UaStatus PanelController::getState(PASState& state)
{
    UaMutexLocker lock(&m_mutex);
    updateState();
    state = m_state;
    return OpcUa_Good;
}

UaStatus PanelController::updateState()
{
    UaMutexLocker lock(&m_mutex);
    // update internal state to match teh underlying platform object
    switch ( m_pPlatform->getState() ) {
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

UaStatus PanelController::setState(PASState state)
{
    UaMutexLocker lock(&m_mutex);

    switch ( state ) {
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

UaStatus PanelController::getData(OpcUa_UInt32 offset, UaVariant& value)
{
#ifdef SIMMODE
    std::random_device rd{};
    std::mt19937 generator{rd()};

    std::normal_distribution<double> internalTempDistribution(30.0,2.0);
    std::normal_distribution<double> externalTempDistribution(20.0,2.0);

    double newValue;
    if (offset == PAS_PanelType_ExtTemperature) {
        newValue = externalTempDistribution(generator);
        value.setFloat(newValue);
    }
    else if (offset == PAS_PanelType_IntTemperature) {
        newValue = internalTempDistribution(generator);
        value.setFloat(newValue);
    }
    else {
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

/// @details Does nothing, panel has no writeable variables.
UaStatus PanelController::setData(OpcUa_UInt32 offset, UaVariant value)
{
    return OpcUa_BadNotImplemented;
}

UaStatus PanelController::Operate(OpcUa_UInt32 offset, const UaVariantArray& args)
{
    UaStatus status;

    updateState(); // Update the current state

    if (m_state == PASState::FatalError) {
        std::cout << "PasPanel::Operate(): Panel in fatal error state! "
            << "Check what's wrong, fix it, and try again.\n";
        return OpcUa_BadInvalidState;
    }

    if (offset == PAS_PanelType_StepAll) {
        if (m_state == PASState::Off)
            setState(PASState::On);
        else if (m_state == PASState::Busy) {
            std::cout << "PanelController::Operate(): Busy at the moment. "
                << "Wait for the current operation to finish and try again.\n";
            return OpcUa_Good;
        }

        m_state = PASState::Busy; // set the state immadiately

        std::array<float, 6> deltaLengths;

        UaVariant vTemp;
        for (int i = 0; i < 6; i++)
        {
            m_pActuators.at(i)->getData(PAS_ACTType_Steps, vTemp);
            vTemp.toFloat(deltaLengths[i]);
        }
        deltaLengths = m_pPlatform->MoveDeltaLengths(deltaLengths);
        // update missed lengths
        for (int i = 0; i < 6; i++)
        {
            vTemp.setFloat(deltaLengths[i]);
            m_pActuators.at(i)->setData(PAS_ACTType_Steps, vTemp);
        }

        status = OpcUa_Good;
    }
    else if (offset == PAS_PanelType_MoveTo_Acts) {
        if (m_state == PASState::Off)
            setState(PASState::On);
        else if (m_state == PASState::Busy) {
            std::cout << "PanelController::Operate(): Busy at the moment. "
                << "Wait for the current operation to finish and try again.\n";
            return OpcUa_Good;
        }

        m_state = PASState::Busy; // set the state immeadiately

        std::array<float, 6> lengths;
        UaVariant vTemp;
        for (int i = 0; i < 6; i++) {
            m_pActuators.at(i)->getData(PAS_ACTType_inLength_mm, vTemp);
            vTemp.toFloat(lengths[i]);
        }
        lengths = m_pPlatform->MoveToLengths(lengths);

        status = OpcUa_Good;
    }
    else if (offset == PAS_PanelType_Stop) {
        std::cout << "PanelController::Operate(): Attempting to gracefully stop the motion.\n";
        status = setState(PASState::Off);
    }
    else
        status = OpcUa_BadInvalidArgument;

    updateState(); // update the state again

    return status;
}
