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
            value.setDouble(m_data.x1);
            break;
        case PAS_PSDType_y1:
            value.setDouble(m_data.y1);
            break;
        case PAS_PSDType_x2:
            value.setDouble(m_data.x2);
            break;
        case PAS_PSDType_y2:
            value.setDouble(m_data.y2);
            break;
        case PAS_PSDType_dx1:
            value.setDouble(m_data.dx1);
            break;
        case PAS_PSDType_dy1:
            value.setDouble(m_data.dy1);
            break;
        case PAS_PSDType_dx2:
            value.setDouble(m_data.dx2);
            break;
        case PAS_PSDType_dy2:
            value.setDouble(m_data.dy2);
            break;
        case PAS_PSDType_Temp:
            value.setDouble(m_data.Temperature);
            break;
        default:
            return OpcUa_BadInvalidArgument;
    }

    double temp;
    value.toDouble(temp);
    spdlog::trace("{} : Read data... offset=> {} value => ({})", m_Identity, offset, temp);
    return status;
}

UaStatus PSDController::setData(OpcUa_UInt32 offset, UaVariant value) {
    return OpcUa_BadNotWritable;
}

UaStatus PSDController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    UaStatus status;
    // UaMutexLocker lock(&m_mutex);

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

    std::vector<std::string> varstoread{"x1", "y1", "x2", "y2", "dx1", "dy1", "dx2", "dy2", "Temperature"};
    UaVariant valstoread[9];

    std::transform(varstoread.begin(), varstoread.end(), varstoread.begin(),
                   [this](std::string &str) { return m_pClient->getDeviceNodeId(m_Identity) + "." + str; });

    status = m_pClient->read(varstoread, &valstoread[0]);
    if (!status.isGood())
        return status;

    for (unsigned i = 0; i < varstoread.size(); i++)
        valstoread[i].toDouble(*(reinterpret_cast<OpcUa_Double *>(&m_data) + i));

    m_LastUpdateTime = TIME::now();

    return OpcUa_Good;
}
