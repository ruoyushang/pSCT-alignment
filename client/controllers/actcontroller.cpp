#include "client/controllers/actcontroller.hpp"

#include "client/clienthelper.h"

ActController::ActController(Identity identity, Client *pClient) : PasController(identity, pClient) {
    m_state = PASState::On;
}

ActController::~ActController() {
    m_pClient = nullptr;
    m_state = PASState::Off;
}

/* ----------------------------------------------------------------------------
    Class        ActController
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatus ActController::getState(PASState &state) {
    state = m_state;
    return OpcUa_Good;
}

/* ----------------------------------------------------------------------------
    Class        ActController
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatus ActController::setState(PASState state) {
    // don't lock the object -- might want to change state while operating the device!
    // UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (state == m_state) {
        return OpcUa_BadInvalidState;
    }
    m_state = state;
    if (state == PASState::Off)
        status = m_pClient->callMethod(m_ID.eAddress, UaString("Stop"));
    else if (state == PASState::On)
        status = m_pClient->callMethod(m_ID.eAddress, UaString("Start"));
    else
        status = OpcUa_BadInvalidArgument;

    return status;
}
/* ----------------------------------------------------------------------------
    Class        ActController
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
// Temporary - move ActController to common
UaStatus ActController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    UaMutexLocker lock(&m_mutex);
    UaStatusCode status;

    std::string varName;

    if (ACTObject::ERRORS.count(offset) > 0) {
        return getError(offset, value);
    } else {
        switch (offset) {
            case PAS_ACTType_DeltaLength:
                varName = "DeltaLength";
                break;
            case PAS_ACTType_CurrentLength:
                varName = "CurrentLength";
                break;
            case PAS_ACTType_TargetLength:
                varName = "TargetLength";
                break;
            default:
                return OpcUa_BadInvalidArgument;
        }
        std::vector<std::string> varsToRead = {m_ID.eAddress + "." + varName};
        status = m_pClient->read(varsToRead, &value);
    }

    return status;
}

// Temporary - move ActController to common.
UaStatusCode ActController::getError(OpcUa_UInt32 offset, UaVariant &value) {
    UaMutexLocker lock(&m_mutex);
    UaStatus status;
    bool errorStatus;

    OpcUa_UInt32 errorNum = offset - PAS_ACTType_Error0;
    // Temporary
    if (errorNum >= 0 && errorNum < 14) {
        std::string varName = "Error";
        varName += std::to_string(errorNum);
        std::vector<std::string> varsToRead = {m_ID.eAddress + "." + varName};
        status = m_pClient->read(varsToRead, &value);
        std::cout << "ActController::getError -> value = " << value.toString().toUtf8() << std::endl;
    } else {
        status = OpcUa_BadInvalidArgument;
    }
    return status;
}

/* ----------------------------------------------------------------------------
    Class        ActController
    Method       setData
    Description  Set Controller data.
-----------------------------------------------------------------------------*/
UaStatus ActController::setData(OpcUa_UInt32 offset, UaVariant value) {
    return OpcUa_BadNotWritable;
}

/* ----------------------------------------------------------------------------
    Class        ActController
    Method       Operate
    Description  run a method on the actuator
-----------------------------------------------------------------------------*/
UaStatusCode ActController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    UaStatusCode status;

    // don't lock the object -- might want to change state while operating the device!
    // UaMutexLocker lock(&m_mutex);
    float deltaLength;
    switch (offset) {
        case PAS_ACTType_MoveDeltaLength:
            UaVariant(args[0]).toFloat(deltaLength);
            std::cout << "Stepping actuator " << m_ID.serialNumber << "by " << deltaLength << " mm\n";
            status = m_pClient->callMethodAsync(m_ID.eAddress, UaString("MoveDeltaLength"), args);
            break;
        case PAS_ACTType_MoveToLength:
            if (args.length() != 1) {
                return OpcUa_BadInvalidArgument;
            }
            status = m_pClient->callMethodAsync(m_ID.eAddress, UaString("MoveToLength"), args);
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}