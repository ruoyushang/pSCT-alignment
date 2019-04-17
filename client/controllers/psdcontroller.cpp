#include "client/controllers/psdcontroller.hpp"

#include "client/clienthelper.hpp"

#include <vector>
#include <string>


// 500 ms update interval for PSDs
int PSDController::kUpdateInterval = 500;

PSDController::PSDController(Identity identity, Client *pClient) :
        PasController(std::move(identity), pClient, kUpdateInterval) {
    m_state = PASState::On;
    std::cout << "PSDController: update interval set to " << m_UpdateInterval_ms << " ms" << std::endl;

    m_lastUpdateTime = TIME::now() - std::chrono::duration<int, std::ratio<1, 1000>>(m_UpdateInterval_ms);
}

PSDController::~PSDController() {
    m_pClient = nullptr;
    m_state = PASState::Off;
}

UaStatus PSDController::getState(PASState &state) {
    state = m_state;
    return OpcUa_Good;
}

UaStatus PSDController::setState(PASState state) {
    UaMutexLocker lock(&m_mutex);

    return OpcUa_BadNotWritable;
}

UaStatus PSDController::getData(OpcUa_UInt32 offset, UaVariant &value) {
    int dataoffset = offset - PAS_PSDType_x1;
    if (dataoffset >= 9) {
        return OpcUa_BadInvalidArgument;
    }

    UaMutexLocker lock(&m_mutex);
    UaStatus status;

    if (__expired()) // if cached values expired, update them
        status = read();

    value.setDouble(*(reinterpret_cast<OpcUa_Double *>(&data) + dataoffset));

    return status;
}

UaStatus PSDController::setData(OpcUa_UInt32 offset, UaVariant value) {
    return OpcUa_BadNotWritable;
}

UaStatus PSDController::operate(OpcUa_UInt32 offset, const UaVariantArray &args) {
    UaStatus status;

    if (offset >= 1)
        return OpcUa_BadInvalidArgument;

    // don't lock the object -- might want to change state while operating the device!
    // UaMutexLocker lock(&m_mutex);
    switch (offset) {
        case 0:
            status = read();
            break;
        default:
            status = OpcUa_BadInvalidArgument;
    }

    return status;
}

/* ----------------------------------------------------------------------------
    END Class    PSDController
==============================================================================*/

UaStatus PSDController::read() {
    UaStatus status;

    std::vector<std::string> varstoread{"x1", "y1", "x2", "y2", "dx1", "dy1", "dx2", "dy2", "Temperature"};
    UaVariant valstoread[9];

    transform(varstoread.begin(), varstoread.end(), varstoread.begin(),
              [this](std::string &str) { return m_ID.eAddress + "." + str; });

    status = m_pClient->read(varstoread, &valstoread[0]);
    if (!status.isGood())
        return status;

    m_lastUpdateTime = TIME::now();
    for (unsigned i = 0; i < varstoread.size(); i++)
        valstoread[i].toDouble(*(reinterpret_cast<OpcUa_Double *>(&data) + i));

    return OpcUa_Good;
}
