#include "client/controllers/psdcontroller.hpp"

#include <algorithm>
#include <vector>
#include <string>

#include "common/alignment/device.hpp"

#include "client/clienthelper.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

PSDController::PSDController(Device::Identity identity, Client *pClient) :
    PasController(std::move(identity), pClient, 500), m_data() {

    spdlog::trace("{} : Creating PSD controller... ", m_Identity);
    m_LastUpdateTime = TIME::now() - std::chrono::duration<int, std::ratio<1, 1000>>(m_kUpdateInterval_ms);
}

UaStatus PSDController::getState(Device::DeviceState &state) {
    state = m_State;
    spdlog::trace("{} : Read device state => ({})", m_Identity, Device::deviceStateNames.at(state));
    return OpcUa_Good;
}

UaStatus PSDController::setState(Device::DeviceState state) {
    //UaMutexLocker lock(&m_mutex);
    return OpcUa_BadNotWritable;
}

UaStatus PSDController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    //UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (__expired()) // if cached values expired, update them
        status = read();

    switch (offset) {
        case PAS_PSDType_x1:
            status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "x1"}, &value);
            value.setDouble(m_data.x1);
            break;
        case PAS_PSDType_y1:
            status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "y1"}, &value);
            value.setDouble(m_data.y1);
            break;
        case PAS_PSDType_x2:
            status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "x2"}, &value);
            value.setDouble(m_data.x2);
            break;
        case PAS_PSDType_y2:
            status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "y2"}, &value);
            value.setDouble(m_data.y2);
            break;
        case PAS_PSDType_dx1:
            status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "dx1"}, &value);
            value.setDouble(m_data.dx1);
            break;
        case PAS_PSDType_dy1:
            status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "dy1"}, &value);
            value.setDouble(m_data.dy1);
            break;
        case PAS_PSDType_dx2:
            status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "dx2"}, &value);
            value.setDouble(m_data.dx2);
            break;
        case PAS_PSDType_dy2:
            status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "dy2"}, &value);
            value.setDouble(m_data.dy2);
            break;
        case PAS_PSDType_Temp:
            status = m_pClient->read({m_pClient->getDeviceNodeId(m_Identity) + "." + "Temperature"}, &value);
            value.setDouble(m_data.Temperature);
            break;
        default:
            return OpcUa_BadInvalidArgument;
    }
    return status;
}

UaStatus PSDController::setData(OpcUa_UInt32 offset, UaVariant value) {
    return OpcUa_BadNotWritable;
}

UaStatus PSDController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    UaStatus status;
    // UaMutexLocker lock(&m_mutex);

    if (getDeviceState() == Device::DeviceState::Busy) {
        spdlog::error("{} : GASPSD is busy, operate call failed.", m_Identity);
        return OpcUa_BadInvalidState;
    }

    if (offset == PAS_PSDType_Read) {
        spdlog::trace("{} : PSDController calling read()", m_Identity);
        status = read();
    } else {
        spdlog::error("{} : Invalid method call with offset {}", m_Identity, offset);
        status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/* ----------------------------------------------------------------------------
    END Class    PSDController
==============================================================================*/

UaStatus PSDController::read() {
    spdlog::trace("Calling read on PSD {} (updating controller data and last update time)", m_Identity);

    UaStatus status;

    status = m_pClient->callMethod(m_pClient->getDeviceNodeId(m_Identity), UaString("Read"), UaVariantArray());
    m_LastUpdateTime = TIME::now();

    if (!status.isGood()) {
        spdlog::error("{} : PSDController::read() : Call to read PSD failed.", m_Identity);
        return status;
    }
    else{
        spdlog::info("{} Current readings ({} deg C):\n\tPSD0 ({},{}) +/- ({},{})\n\tPSD1 ({},{}) +/- ({},{})",
                     m_Identity, getPSDposition().Temperature,
                     getPSDposition().x1, getPSDposition().y1, getPSDposition().dx1, getPSDposition().dy1,
                     getPSDposition().x2, getPSDposition().y2, getPSDposition().dx2, getPSDposition().dy2);
    }

    return status;
}

Device::ErrorState PSDController::getErrorState() {
    UaVariant var;
    int err;
    getData(PAS_PSDType_ErrorState, var);
    var.toInt32(err);
    return static_cast<Device::ErrorState>(err);
}