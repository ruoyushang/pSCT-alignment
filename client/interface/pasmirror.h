#ifndef __PASMIRROR_H__
#define __PASMIRROR_H__

#include "pascontroller.h"
#include "TObject.h" // to be able to use ROOT's MINUIT implementation
// these are included in pascontroller.h, but we add them just in case something changes there
#include "stewartplatform.h"

#include <set>
// Eigen3 for linear algebra needs
#include <Eigen/Dense>

class AGeoAsphericDisk;

class PasMirror; // need this forward declaration for the friend class

// This is an interface to be able to use ROOT's MINUIT, which requires a static ChiSq function.
// ROOT Requires that objects inherit from its TObject. I don't want such multiple inheritance
// for PasMirror, therefore creating this wrapper.
//
// Making this a singleton to make sure this doesn't live longer than the actual compute --
// otherwise the static m_pMirror may cause trouble
class PasMirrorCompute : public TObject
{
    public:
        static PasMirrorCompute& getInstance(PasMirror *mirror) {
            static PasMirrorCompute instance(mirror); // guarantees destruction
            return instance;
        }

        ~PasMirrorCompute() { Mirror = nullptr; };
        // make sure we don't create accidental copies -- can't use assignment or copy constructor
        PasMirrorCompute(PasMirrorCompute const&) = delete;
        void operator=(PasMirrorCompute const&) = delete;

        // MINUIT interface
        static void chiSqFCN(int &npar, double *gin, double &f, double *par, int iflag);

        // and the mirror object accessed by the above
        static PasMirror *Mirror;

    private :
        PasMirrorCompute(PasMirror *mirror) { Mirror = mirror; };
};


class PasMirror : public PasCompositeController
{
    UA_DISABLE_COPY(PasMirror);
public:
    PasMirror(Identity identity);
    virtual ~PasMirror();

    // initialize and precompute everything
    bool Initialize();

    // same as before -- get/set status/data and Operate:
    UaStatusCode getState(PASState& state);
    UaStatusCode getData(OpcUa_UInt32 offset, UaVariant& value);
    UaStatusCode setState(PASState state);
    UaStatusCode setData(OpcUa_UInt32 offset, UaVariant value);
    UaStatusCode Operate(OpcUa_UInt32 offset = 0, const UaVariantArray &args = UaVariantArray());

    // own implementation
    void addChild(OpcUa_UInt32 deviceType, PasController *const pController);

    // let PasMirrorCompute access the internal chiSq function
    friend void PasMirrorCompute::chiSqFCN(int &npar, double *gin, double &f, double *par, int iflag);

protected:
    // compute chiSq for all panels given a perturbation to the mirror
    virtual double chiSq(Eigen::VectorXd telDleta);

private:
    // private methods for the actions we can take
    UaStatus __align();
    UaStatus __moveTo();

    // helper method to process the selected children string and convert it into a set
    // of vector indices
    void __updateSelectedChildren(unsigned deviceType);
    // update current mirror coordinates
    void __updateCoords();
    // a bunch of internal implementations
    void __stopAll();
    void __readPositionAll();
    void __readAlignmentAll();
    // Align all edges fron need_alignment starting at start_idx and  moving in the direction dir
    void __alignAll(unsigned start_idx, const std::set<unsigned>& need_alignment, bool dir);
    void __move();
    // the children that we have selected to perform common actions on
    // if this is empty, no action is performed
    // maps deviceType->{indices of selected children in the m_pChildren.at(deviceType) vector}
    std::map<unsigned, std::set<unsigned>> m_SelectedChildren;
    // same as above, only the list of children is a comma-/space-/semicolon-separated string
    std::map<unsigned, std::string> m_SelectedChildrenString;

    // mirror coords -- x/y/z, xRot, yRot, zRot
    Eigen::VectorXd m_curCoords, m_curCoordsErr, m_inCoords, m_sysOffsets;

    // in our coordinate system, the ideal panel is located along the x axis, between
    // the second and third quadrant
    // we keep Azimuthal offset of x1x1 from this ideal panel
    std::map<unsigned, double> m_PanelOffset;

    // angular panel width in radians
    // ring -> {panel width}
    std::map<unsigned, double> m_PanelWidth;

    // precomputed ideal panel origin in the telescope frame -- both the inner and outer rings
    // ring->{location vector}
    // currently setting to zeros and populating in Initialize()
    std::map<unsigned, Eigen::Vector3d> m_PanelOriginTelFrame;

    // pad positions in the telescope frame for both rings -
    // convenience copy of SCT::<Mirror>::PadsCoords. Pad per column of the matrix
    std::map<unsigned, Eigen::Matrix3d> m_PadCoordsTelFrame;

    // precomputed ideal panel reference frame (basis vectors) in the telescope frame,
    // both the inner and outer rings. This to facilitate coordinate transformations
    // ring->{Column Matrix}
    // Populated in Initialize()
    std::map<unsigned, Eigen::Matrix3d> m_PanelFrame;

    // the direction of the norm of the whole mirror: +1 for Primary, -1 for secondary
    double m_SurfaceNorm = 1.;

    // COORDINATE TRANSFORMATION HELPERS
    // reference frame tansformations:
    double __getAzOffset(unsigned pos);
    Eigen::Matrix3d __rotMat(int axis, double a);
    // to panel reference frame (from telescope). Order of rotations -- z -> x-> y
    Eigen::Vector3d __toPanelRF(unsigned pos, Eigen::Vector3d in_coords);
    // to telescope reference frame (from panel)
    Eigen::Vector3d __toTelRF(unsigned pos, Eigen::Vector3d in_coords);
    // move a vector by in_coords in the current reference frame.
    // The current reference frame is the frame in which in_vec coords and tr_coords
    // transformations are given.
    // the input is a 3D vector and the 6 coords by which to transform,
    // tr_coords -- x, y, z, xRot, yRot, zRot
    // the output is the transformed 3D vector
    Eigen::Vector3d __moveInCurrentRF(Eigen::Vector3d in_vec, Eigen::VectorXd tr_coords);

    Eigen::MatrixXd __computeSystematicsMatrix(unsigned pos1, unsigned pos2);

    /**** SIMULATED OBJECTS WE RELY ON FOR COMPUTE *****/
    // A simulated surface of this mirror
    AGeoAsphericDisk *m_pSurface;
    // A simulated stewart platform
    StewartPlatform m_SP;
};

#endif // #ifndef __PASMIRROR_H__
