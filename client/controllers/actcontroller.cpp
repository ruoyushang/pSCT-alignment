#include "client/controllers/actcontroller.hpp"

#include <iostream>
#include <string>
#include <vector>

#include "uabase/statuscode.h"
#include "uabase/uabase.h"
#include "uabase/uastring.h"
#include "uabase/uavariant.h"

#include "common/alignment/device.hpp"
#include "common/opcua/pascominterfacecommon.hpp"
#include "common/opcua/pasobject.hpp"
#include "common/opcua/passervertypeids.hpp"

#include "client/clienthelper.hpp"
#include "client/controllers/pascontroller.hpp"

ActController::ActController(Device::Identity identity, std::shared_ptr<Client> pClient) : PasController(
    std::move(identity), std::move(pClient)) {
}

/* ----------------------------------------------------------------------------
    Class        ActController
    Method       getState
    Description  Get Controller status.
-----------------------------------------------------------------------------*/
UaStatus ActController::getState(Device::DeviceState &state) {
    //UaMutexLocker lock(&m_mutex);    
    UaStatus status;
    UaVariant value;
    int v;
    
    m_pClient->read({m_ID.eAddress + "." + "State"}, &value);
    value.toInt32(v);

    state = static_cast<Device::DeviceState>(v);

    return status;
}

/* ----------------------------------------------------------------------------
    Class        ActController
    Method       setState
    Description  Set Controller status.
-----------------------------------------------------------------------------*/
UaStatus ActController::setState(Device::DeviceState state) {
    return OpcUa_BadNotWritable;
}
/* ----------------------------------------------------------------------------
    Class        ActController
    Method       getData
    Description  Get Controller data.
-----------------------------------------------------------------------------*/
// Temporary - move ActController to common
UaStatus ActController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

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
            case PAS_ACTType_Position:
                varName = "Position";
                break;
            case PAS_ACTType_Serial:
                varName = "Serial";
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
UaStatus ActController::getError(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (ACTObject::ERRORS.count(offset) > 0) {
        std::string varName = std::get<0>(ACTObject::ERRORS.at(offset));
        std::vector<std::string> varsToRead = {m_ID.eAddress + "." + varName};
        status = m_pClient->read(varsToRead, &value);
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
UaStatus ActController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    UaStatus status;

    // don't lock the object -- might want to change state while operating the device!
    // UaMutexLocker lock(&m_mutex);

    switch (offset) {
        case PAS_ACTType_MoveDeltaLength:
            if (args.length() != 1) {
                return OpcUa_BadInvalidArgument;
            }
            float deltaLength;
            UaVariant(args[0]).toFloat(deltaLength);
            std::cout << "Stepping actuator " << m_ID.serialNumber << "by " << deltaLength << " mm\n";
            status = m_pClient->callMethodAsync(m_ID.eAddress, UaString("MoveDeltaLength"), args);
            break;
        case PAS_ACTType_MoveToLength:
            if (args.length() != 1) {
                return OpcUa_BadInvalidArgument;
            }
            float targetLength;
            UaVariant(args[0]).toFloat(targetLength);
            std::cout << "Stepping actuator " << m_ID.serialNumber << "to target length " << targetLength << " mm\n";
            status = m_pClient->callMethodAsync(m_ID.eAddress, UaString("MoveToLength"), args);
            break;
        case PAS_ACTType_ForceRecover:
            status = m_pClient->callMethod(m_ID.eAddress, UaString("ForceRecover"));
            break;
        case PAS_ACTType_ClearError:
            status = m_pClient->callMethod(m_ID.eAddress, UaString("ClearError"), args);
            break;
        case PAS_ACTType_ClearAllErrors:
            status = m_pClient->callMethod(m_ID.eAddress, UaString("ClearAllErrors"));
            break;
        case PAS_ACTType_TurnOn:
            status = m_pClient->callMethod(m_ID.eAddress, UaString("TurnOn"));
            break;
        case PAS_ACTType_TurnOff:
            status = m_pClient->callMethod(m_ID.eAddress, UaString("TurnOff"));
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}
