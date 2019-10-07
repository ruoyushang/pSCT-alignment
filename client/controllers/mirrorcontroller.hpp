#ifndef __PASMIRROR_H__
#define __PASMIRROR_H__

#include <cfloat>
#include <set>

#include "TObject.h" // to be able to use ROOT's MINUIT implementation

#include <Eigen/Dense> // Eigen3 for linear algebra needs

#include "common/alignment/device.hpp"
#include "common/simulatestewart/stewartplatform.hpp"

#include "client/controllers/pascontroller.hpp"

class AGeoAsphericDisk;

class MirrorController; // need this forward declaration for the friend class
class PanelController;

// This is an interface to be able to use ROOT's MINUIT, which requires a static ChiSq function.
// ROOT Requires that objects inherit from its TObject. I don't want such multiple inheritance
// for MirrorController, therefore creating this wrapper.
//
// Making this a singleton to make sure this doesn't live longer than the actual compute --
// otherwise the static m_pMirror may cause trouble
class MirrorControllerCompute : public TObject
{
public:
    static MirrorControllerCompute &getInstance(MirrorController *mirror) {
        static MirrorControllerCompute instance(mirror); // guarantees destruction
            return instance;
        }

    ~MirrorControllerCompute() override { m_Mirror = nullptr; };

    // make sure we don't create accidental copies -- can't use assignment or copy constructor
    MirrorControllerCompute(MirrorControllerCompute const &) = delete;
    void operator=(MirrorControllerCompute const &) = delete;

    // MINUIT interface
    static void chiSqFCN(int &npar, double *gin, double &f, double *par, int iflag);

    private :
    explicit MirrorControllerCompute(MirrorController *mirror) { m_Mirror = mirror; };
    // and the mirror object accessed by the above
    static MirrorController *m_Mirror;
};


class MirrorController : public PasCompositeController {
    UA_DISABLE_COPY(MirrorController);
public:
    explicit MirrorController(Device::Identity identity, std::string mode = "client");

    // initialize and precompute everything
    bool initialize() override;

    // same as before -- get/set status/data and Operate:
    UaStatus getState(Device::DeviceState &state) override;

    UaStatus getData(OpcUa_UInt32 offset, UaVariant &value) override;

    UaStatus setState(Device::DeviceState state) override;

    UaStatus setData(OpcUa_UInt32 offset, UaVariant value) override;

    UaStatus operate(OpcUa_UInt32 offset, const UaVariantArray &args) override;

    // own implementation
    void addChild(OpcUa_UInt32 deviceType, const std::shared_ptr<PasController> &pController) override;

    // let MirrorControllerCompute access the internal chiSq function
    friend void MirrorControllerCompute::chiSqFCN(int &npar, double *gin, double &f, double *par, int iflag);

protected:
    // compute chiSq for all panels given a perturbation to the mirror
    virtual double chiSq(const Eigen::VectorXd &telDelta);

private:
    std::string m_Mode;

    double m_safetyRadius = 60.0;

    std::set<unsigned> m_selectedPanels;
    std::set<int> m_selectedMPES;
    std::set<std::string> m_selectedEdges;

    // helper method to process the selected children string and convert it into a set
    // of vector indices
    void parseAndSetSelection(const std::string &selectionString, unsigned deviceType);

    unsigned getPanelIndex(unsigned position);
    unsigned getMPESIndex(int serialNumber);

    unsigned getEdgeIndex(const std::string &eAddress);

    // update current mirror coordinates
    UaStatus updateCoords(bool print=true);
    // a bunch of internal implementations
    UaStatus readPositionAll(bool print=true);

    UaStatus savePosition(const std::string &saveFilePath);
    UaStatus saveAlignmentOffset(const std::string &saveFilePath);

    // Align all edges fron need_alignment starting at start_idx and  moving in the direction dir
    UaStatus alignSequential(const std::string &startEdge, const std::string &EndEdge, unsigned dir);

    UaStatus __calculateAlignSector();

    UaStatus __calculateAlignRing(int fixPanel);

    UaStatus __calculateMoveDeltaCoords(const Eigen::VectorXd &targetCoords);

    UaStatus __calculateMoveToCoords(const Eigen::VectorXd &targetCoords);

    UaStatus __calculateLoadPosition(const std::string &loadFilePath);

    UaStatus __calculateLoadDeltaCoords(const std::string &loadFilePath);

    UaStatus testActuators(float moveDistance = 0.4, float epsilonLength = 0.016);

    // mirror coords -- x/y/z, xRot, yRot, zRot
    Eigen::VectorXd m_curCoords, m_curCoordsErr, m_sysOffsetsMPES;
    // map ring -> {MPES Position -> Systematic Offset}
    std::map<unsigned, std::map<unsigned, Eigen::VectorXd> > SystematicOffsetsMPESMap;

    // in our coordinate system, the ideal panel is located along the x axis, between
    // the second and third quadrant
    // we keep Azimuthal offset of x1x1 from this ideal panel
    std::map<unsigned, double> m_PanelOffset;

    // angular panel width in radians
    // ring -> {panel width}
    std::map<unsigned, double> m_PanelWidth;

    // precomputed ideal panel origin in the telescope frame -- both the inner and outer rings
    // ring->{location vector}
    // currently setting to zeros and populating in initialize()
    std::map<unsigned, Eigen::Vector3d> m_PanelOriginTelFrame;

    // pad positions in the telescope frame for both rings -
    // convenience copy of SCT::<Mirror>::PadsCoords. Pad per column of the matrix
    std::map<unsigned, Eigen::Matrix3d> m_PadCoordsTelFrame;

    // precomputed ideal panel reference frame (basis vectors) in the telescope frame,
    // both the inner and outer rings. This to facilitate coordinate transformations
    // ring->{Column Matrix}
    // Populated in initialize()
    std::map<unsigned, Eigen::Matrix3d> m_PanelFrame;

    // the direction of the norm of the whole mirror: +1 for Primary, -1 for secondary
    double m_SurfaceNorm = 1.;

    // COORDINATE TRANSFORMATION HELPERS
    // reference frame tansformations:
    double __getAzOffset(unsigned pos);

    static Eigen::Matrix3d __rotMat(int axis, double a);
    // to panel reference frame (from telescope). Order of rotations -- z -> x-> y
    Eigen::Vector3d __toPanelRF(unsigned pos, const Eigen::Vector3d &in_coords);
    // to telescope reference frame (from panel)
    Eigen::Vector3d __toTelRF(unsigned pos, const Eigen::Vector3d &in_coords);
    // move a vector by in_coords in the current reference frame.
    // The current reference frame is the frame in which in_vec coords and tr_coords
    // transformations are given.
    // the input is a 3D vector and the 6 coords by which to transform,
    // tr_coords -- x, y, z, xRot, yRot, zRot
    // the output is the transformed 3D vector
    Eigen::Vector3d __moveInCurrentRF(const Eigen::Vector3d &in_vec, const Eigen::VectorXd &tr_coords);

    Eigen::MatrixXd __computeSystematicsMatrix(unsigned pos1, unsigned pos2);

    /**** SIMULATED OBJECTS WE RELY ON FOR COMPUTE *****/
    // A simulated surface of this mirror
    std::shared_ptr<AGeoAsphericDisk> m_pSurface;
    // A simulated stewart platform
    std::shared_ptr<StewartPlatform> m_pStewartPlatform;

    Eigen::VectorXd m_Xcalculated;
    std::vector<std::shared_ptr<PanelController>> m_panelsToMove;
    unsigned m_previousCalculatedMethod;
    float m_lastSetAlignFrac;

    static const std::string SAVEFILE_DELIMITER;

    UaStatus __moveSelectedPanels(unsigned methodTypeId, double alignFrac);
    UaStatus __setAlignFrac(double alignFrac);
};

#endif // #ifndef __PASMIRROR_H__
