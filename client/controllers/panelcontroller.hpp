#ifndef CLIENT_PANELCONTROLLER_HPP
#define CLIENT_PANELCONTROLLER_HPP

#include <Eigen/Dense>

#include "client/controllers/pascontroller.hpp"
#include "client/controllers/mirrorcontroller.hpp"
#include "client/controllers/edgecontroller.hpp"

class PanelController : public PasCompositeController {
    UA_DISABLE_COPY(PanelController);

public:
    friend MirrorController; // for convenient access to internal methods for mirror computations
    friend EdgeController; // access internal methods for compute

    // construction / destruction
    PanelController(Identity identity, Client *pClient);

    virtual ~PanelController();

    // Get Controller status and data
    UaStatus getState(PASState &state);

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value);

    // set Controller status and data
    UaStatus setState(PASState state);

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value);

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args = UaVariantArray());

    unsigned getActuatorCount();

    UaStatus moveToLengths(const UaVariantArray &args);
    UaStatus moveDeltaLengths(const UaVariantArray &args);

    Eigen::VectorXd getActuatorLengths();
    bool checkForCollision(const Eigen::VectorXd &deltaLengths);
private:
    int m_kUpdateInterval = 1000;

    // helper
    void updateCoords(bool printout = false);

    // x, y, z, xRot, yRot, zRot
    double m_curCoords[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    double m_safetyRadius = 80.0;

    StewartPlatform m_SP;

    // helper to be able to run ChiSq minimization
    Eigen::Matrix3d getPadCoords() { return m_PadCoords; };

    // pad coords -- column per pad
    Eigen::Matrix3d m_PadCoords;
};

#endif //CLIENT_PANELCONTROLLER_HPP
