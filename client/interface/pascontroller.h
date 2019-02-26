/******************************************************************************
** Project: MPES server
**
** Description: implementation of MPES.
******************************************************************************/
#ifndef __PASCONTROLLER_H__
#define __PASCONTROLLER_H__

#include "uabase.h"
#include "statuscode.h"
#include "pascommunicationinterface.h"
#include "passervertypeids.h"
#include "stewartplatform.h"
#include "uamutex.h"
#include "components.h"
#include <vector>
#include <set>
#include <map>
#include <chrono>
#include <iostream>

// Eigen3 for linear algebra needs
#include <Eigen/Dense>

class Client;
class GASCCD;
// we add these forward declarations here to be able to declare friends --
// each class has its parent as a friend.
class PasMirror;
class PasEdge;
class PasPanel;

/*===========================================================================*/
/* PasController class, general class from which other PAS devices inherit.  */
/*===========================================================================*/
class PasController
{
    UA_DISABLE_COPY(PasController);
public:
    // update interval in ms
    static int kUpdateInterval;

    /* construction / destruction */
    PasController(Identity identity, Client *pClient, int updateInterval = 0) :
        m_ID(identity),
        m_pClient(pClient), m_UpdateInterval_ms(updateInterval) { };
    virtual ~PasController() {};

    const Identity& getId() const {return m_ID;}

    /* Get Controller status and data */
    virtual UaStatusCode getState(PASState& state) = 0;
    virtual UaStatusCode getData(OpcUa_UInt32 offset, UaVariant& value) = 0;

    /* set Controller status and data */
    virtual UaStatusCode setState(PASState state) = 0;
    virtual UaStatusCode setData(OpcUa_UInt32 offset, UaVariant value) = 0;
    virtual UaStatusCode Operate(OpcUa_UInt32 offset = 0,
                                  const UaVariantArray &args = UaVariantArray()) = 0;

    virtual bool Initialize() { return true; }

protected:
    UaMutex m_mutex;
    PASState m_state;

    Identity m_ID;
    Client *m_pClient;

    // update interval in milliseconds
    const int m_UpdateInterval_ms;

    // be able to check if data has expired
    typedef std::chrono::system_clock TIME;
    bool __expired() const {return (std::chrono::duration_cast<std::chrono::milliseconds>(TIME::now() - m_lastUpdateTime).count() > m_UpdateInterval_ms); }
    std::chrono::time_point<TIME> m_lastUpdateTime;
};

// helper class for composite devices like panel and edge
class PasCompositeController : public PasController
{
    public:
        PasCompositeController(Identity identity, Client *pClient, int updateInterval = 0) :
            PasController(identity, pClient, updateInterval) {};
        virtual ~PasCompositeController(){}

        // yes, this indeed needs to be the specified type -- a const pointer to a
        // (non-const) PasController. The constness guarantees that the panel doesn't change the
        // pointer -- but other methods we use require that the underlying object itself is not
        // a const.
        // declaring these as virtual, but they should be the same for everything that
        // inherts from here
        virtual void addChild(OpcUa_UInt32 deviceType, PasController *const pController);
        virtual const std::vector<PasController *>& getChildren(unsigned type) const { return m_pChildren.at(type); };

    protected:
        // stores the possbile types of children
        std::set<unsigned> m_ChildrenTypes;
        // deviceType -> vector of Children devices
        std::map< unsigned, std::vector<PasController *> > m_pChildren;
        // deviceType -> {position -> index in m_pChildren.at(deviceType) }
        // m_ChildrenPositionMap.at(deviceType) is oredered by the first index, position.
        // This makes it perfect for cycling through positions.
        std::map< unsigned, std::map<unsigned, unsigned> > m_ChildrenPositionMap;
        // almost a duplicate of the above, but not quite.
        // This guarantees that sensors along the 4-sensor edge are not discarded if their
        // positions are the same; this also provides a map for edges within a mirror
        std::map< unsigned, std::map<Identity, unsigned> > m_ChildrenIdentityMap;
};
/*===========================================================================*/
/* Edge sensor class, part of the PAS family of devices.                     */
/*===========================================================================*/
class PasMPES : public PasController
{
    UA_DISABLE_COPY(PasMPES);
public:
    friend PasEdge;
    friend PasMirror;
    /* construction / destruction */
    PasMPES(Identity identity, Client *pClient);
    ~PasMPES();

    /* Get Controller status and data */
    UaStatusCode getState(PASState& state);
    UaStatusCode getData(OpcUa_UInt32 offset, UaVariant& value);

    /* set Controller status and data */
    UaStatusCode setState(PASState state);
    UaStatusCode setData(OpcUa_UInt32 offset, UaVariant value);
    UaStatusCode Operate(OpcUa_UInt32 offset = 0, const UaVariantArray &args = UaVariantArray());
    // test if current panel is this sensor's webcam-side panel
    char getPanelSide(unsigned panelpos);
    bool isVisible() const {return m_isVisible;};
    const Eigen::Matrix<double, 2, 6>& getResponseMatrix(char panelside = 'w') const { return m_ResponseMatMap.at(panelside); }
    const Eigen::Vector2d& getAlignedReadings() const {return m_AlignedReadings;}
    const Eigen::Vector2d& getSystematicOffsets() const {return SystematicOffsets;}

private:
    bool m_updated;
    bool m_isVisible;
    struct MPESData {
        OpcUa_Double m_xCentroidAvg;
        OpcUa_Double m_yCentroidAvg;
        OpcUa_Double m_xCentroidSD;
        OpcUa_Double m_yCentroidSD;
        OpcUa_Double m_CleanedIntensity;
        OpcUa_Double m_xNominal;
        OpcUa_Double m_yNominal;
    } data;
    static float kNominalIntensity;
    static float kNominalCentroidSD;

    Identity m_wPanelId; // id of the webcam-side panel
    // actuator response matrix map -- {panel position -> matrix}
    std::map< char,  Eigen::Matrix<double, 2, 6> > m_ResponseMatMap;
    // aligned readings
    Eigen::Vector2d m_AlignedReadings;
    Eigen::Vector2d SystematicOffsets;
    // which side the panel is on { panel position -> side ('w' or 'l')
    std::map< unsigned, char > m_PanelSideMap;
    UaStatus read();
};

/*===========================================================================*/
/* Actuator class, part of the PAS family of devices.                        */
/*===========================================================================*/
class PasACT : public PasController
{
    UA_DISABLE_COPY(PasACT);
public:
    friend PasPanel;
    // construction / destruction
    PasACT(Identity identity, Client *pClient);
    virtual ~PasACT();

    // Get Controller status and data
    UaStatusCode getState(PASState& state);
    UaStatusCode getData(OpcUa_UInt32 offset, UaVariant& value);

    // set Controller status and data
    UaStatusCode setState(PASState state);
    UaStatusCode setData(OpcUa_UInt32 offset, UaVariant value);
    UaStatusCode Operate(OpcUa_UInt32 offset = 0, const UaVariantArray &args = UaVariantArray());

private:
    OpcUa_Float m_DeltaL;

    UaStatus moveDelta();
};
/*===========================================================================*/
/* Panel class, part of the PAS family of devices.                        */
/*===========================================================================*/
class PasPanel : public PasCompositeController
{
    UA_DISABLE_COPY(PasPanel);
public:
    friend PasMirror; // for convenient access to internal methods for mirror computations
    friend PasEdge; // access internal methods for compute

    // construction / destruction
    PasPanel(Identity identity, Client *pClient);
    virtual ~PasPanel();

    // Get Controller status and data
    UaStatusCode getState(PASState& state);
    UaStatusCode getData(OpcUa_UInt32 offset, UaVariant& value);

    // set Controller status and data
    UaStatusCode setState(PASState state);
    UaStatusCode setData(OpcUa_UInt32 offset, UaVariant value);
    UaStatusCode Operate(OpcUa_UInt32 offset = 0, const UaVariantArray &args = UaVariantArray());

    unsigned getActuatorCount();
    void getActuatorSteps(UaVariantArray &args) const;

    // update interval in ms
    static int kUpdateInterval;

private:
    // methods
    UaStatus __moveTo();

    // helper
    void __updateCoords(bool printout = false);

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

    bool __willSensorsBeOutOfRange();
};


/*===========================================================================*/
/* Edge class, part of the PAS family of devices.                        */
/*===========================================================================*/
class PasEdge : public PasCompositeController
{
    UA_DISABLE_COPY(PasEdge);
public:
    friend PasMirror;
    // construction / destruction
    PasEdge(Identity identity);
    virtual ~PasEdge();

    // Get Controller status and data
    UaStatusCode getState(PASState& state);
    UaStatusCode getData(OpcUa_UInt32 offset, UaVariant& value);

    // set Controller status and data
    UaStatusCode setState(PASState state);
    UaStatusCode setData(OpcUa_UInt32 offset, UaVariant value);
    UaStatusCode Operate(OpcUa_UInt32 offset = 0, const UaVariantArray &args = UaVariantArray());

    const Eigen::MatrixXd& getResponseMatrix(unsigned panelpos);
    const Eigen::VectorXd& getAlignedReadings();
    const Eigen::VectorXd& getSystematicOffsets();
    const Eigen::VectorXd& getCurrentReadings();

    bool isAligned() { return m_isAligned; }

private:
    // internal variables
    OpcUa_Float m_DeltaL;
    OpcUa_Float m_AlignFrac;

    // methods
    UaStatus align(unsigned panel_pos, bool moveit);
    UaStatus findMatrix();
    // helpers for the above
    UaStatus __findSingleMatrix(unsigned panelidx);
    UaStatus __alignSinglePanel(unsigned panelpos, bool moveit = true);

    // keep track of the panels we want to move
    std::vector<unsigned> m_PanelsToMove;
    // maps panel position to its response matrix
    std::map<unsigned, Eigen::MatrixXd> m_ResponseMatMap;
    Eigen::VectorXd m_AlignedReadings;
    Eigen::VectorXd m_systematicOffsets;
    Eigen::VectorXd m_CurrentReadings;
    Eigen::VectorXd m_CurrentReadingsSD;

    bool m_isAligned;
};

/*===========================================================================*/
/* CCD class, part of the PAS family of devices.                        */
/*===========================================================================*/
class PasCCD : public PasController
{
    UA_DISABLE_COPY(PasCCD);
public:
    friend PasMirror;
    /* construction / destruction */
    PasCCD(Identity identity);
    ~PasCCD();

    /* Get Controller status and data */
    UaStatusCode getState(PASState& state);
    UaStatusCode getData(OpcUa_UInt32 offset, UaVariant& value);

    /* set Controller status and data */
    UaStatusCode setState(PASState state);
    UaStatusCode setData(OpcUa_UInt32 offset, UaVariant value);
    UaStatusCode Operate(OpcUa_UInt32 offset = 0, const UaVariantArray &args = UaVariantArray());

private:
    PASState m_state = PASState::PAS_Off;
    bool m_updated = false;

    UaStatus read();

    GASCCD *m_pCCD;
};

class PasPSD : public PasController
{
    UA_DISABLE_COPY(PasPSD);
public:
    friend PasMirror;
    // construction / destruction
    PasPSD(Identity identity, Client *pClient);
    virtual ~PasPSD();

    // Get Controller status and data
    UaStatusCode getState(PASState& state);
    UaStatusCode getData(OpcUa_UInt32 offset, UaVariant& value);

    // set Controller status and data
    UaStatusCode setState(PASState state);
    UaStatusCode setData(OpcUa_UInt32 offset, UaVariant value);
    UaStatusCode Operate(OpcUa_UInt32 offset = 0, const UaVariantArray &args = UaVariantArray());
    UaStatus step(OpcUa_Double steps);

    // update interval in ms
    static int kUpdateInterval;

private:
    struct PSDData {
        OpcUa_Double x1;
        OpcUa_Double y1;
        OpcUa_Double x2;
        OpcUa_Double y2;
        OpcUa_Double dx1;
        OpcUa_Double dy1;
        OpcUa_Double dx2;
        OpcUa_Double dy2;
        OpcUa_Double Tempereature;
    } data;

    UaStatus read();
};

#endif // #ifndef __PASCONTROLLER_H__
