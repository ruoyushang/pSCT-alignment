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

    UaStatus operate(OpcUa_UInt32 offset = 0, const UaVariantArray &args = UaVariantArray());

    unsigned getActuatorCount();

    void getActuatorSteps(UaVariantArray &args) const;

    // update interval in ms
    static int kUpdateInterval;

private:
    // methods
    UaStatus __moveTo();

    // helper
    void updateCoords(bool printout = false);

    // x, y, z, xRot, yRot, zRot
    double m_curCoords[6], m_inCoords[6];
    StewartPlatform m_SP;

    // helper to be able to run ChiSq minimization
    Eigen::Matrix3d getPadCoords() { return m_PadCoords; };

    // helpers to be able to run ChiSq minimization
    Eigen::VectorXd m_ActuatorLengths;
    // pad coords -- column per pad
    Eigen::Matrix3d m_PadCoords;

    // track if inCoords have been updated on initial boot
    bool m_inCoordsUpdated;

    bool checkForCollision();
};

#endif //CLIENT_PANELCONTROLLER_HPP
