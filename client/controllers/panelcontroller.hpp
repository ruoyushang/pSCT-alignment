#ifndef CLIENT_PANELCONTROLLER_HPP
#define CLIENT_PANELCONTROLLER_HPP

#include <Eigen/Dense>

#include "common/alignment/device.hpp"

#include "client/controllers/edgecontroller.hpp"
#include "client/controllers/mirrorcontroller.hpp"
#include "client/controllers/pascontroller.hpp"


class PanelController : public PasCompositeController {
    UA_DISABLE_COPY(PanelController);

public:
    friend MirrorController; // for convenient access to internal methods for mirror computations
    friend EdgeController; // access internal methods for compute

    // construction / destruction
    PanelController(Device::Identity identity, Client *pClient, std::string mode = "client");

    // Get Controller status and data
    UaStatus getState(Device::DeviceState &state) override;

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    UaStatus getError(OpcUa_UInt32 offset, UaVariant &value);

    // set Controller status and data
    UaStatus setState(Device::DeviceState state) override;

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args = UaVariantArray()) override;

    unsigned getActuatorCount();

    Eigen::VectorXd getActuatorLengths();

    bool checkForCollision(const Eigen::VectorXd &deltaLengths, bool read = true);

    // helper
    UaStatus updateCoords(bool printout = false);

private:
    std::string m_mode;

    // x, y, z, xRot, yRot, zRot
    double m_curCoords[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    double m_safetyRadius = 60.0;

    StewartPlatform m_SP;

    // helper to be able to run ChiSq minimization
    Eigen::Matrix3d getPadCoords() { return m_PadCoords; };

    // pad coords -- column per pad
    Eigen::Matrix3d m_PadCoords;

    UaStatus __getActuatorLengths(Eigen::VectorXd &lengths);

    Device::ErrorState __getErrorState();

    Device::DeviceState __getDeviceState();

    UaStatus __moveToLengths(const UaVariantArray &args) {
        return m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_Identity), UaString("MoveToLengths"), args);
    }

    UaStatus __moveDeltaLengths(const UaVariantArray &args) {
        return m_pClient->callMethodAsync(m_pClient->getDeviceNodeId(m_Identity), UaString("MoveDeltaLengths"), args);
    }
};

#endif //CLIENT_PANELCONTROLLER_HPP
