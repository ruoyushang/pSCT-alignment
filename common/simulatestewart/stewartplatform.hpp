#ifndef __STEWARTPLATFORM_H__
#define __STEWARTPLATFORM_H__

#include <map>
class StewartPlatform
{
    public:
        StewartPlatform();
        ~StewartPlatform() = default;

        void BuildPlatform();
        const double *GetPadCoords(int pad) const { return fPadCoords[PadMap.at(pad)]; };
        void SetPadCoords(const double padCoords[][3]);
        const double *GetActLengths() const { return fActL; };
        const double *GetPanelCoords() const { return fPanelCoords_external; };
       
        // order of rotational axes -- this is its own inverse in our case, which we utilize
        const unsigned fRotOrder[3] = {1,3,2};
        const double kRp = 320.; // payload radius in mm
        // the actual iterative Newton-Raphson computation -- find the platform position
        // given actuator lengths
        void ComputeStewart(const double *actL, double eps = 1e-12);
        // the inverse of the above -- compute actuator lengths based on a known platform
        // position and a given order of rotations
        void ComputeActsFromPanel(const double *panelCoords);
        // the inverse of the above -- compute actuator lengths based on known pad
        // coordinates
        void ComputeActsFromPads(const double padCoords[][3]);

        const std::map<int, int> PadMap = {{0, 1}, {1, 0}, {2, 2}};

        // the different panel types we have to deal with
        enum PanelType {P1, P2, S1, S2, OPT, PANELNUM};
        void SetPanelType(PanelType P) {fPanel = P;};
        static const double kMirrorDistance; // mm

    private:

        // bx, by, px and py are the coordinates of attachment points of actuators.
        // bx and by are the coordinates on the base platform;
        // px and py are the coordinates on the payload platform;
        // these are all in the frames of reference attached to their respective platforms 
        // (hence no z-coordinates) and are therefore fixed -- they are part of the specification
        // and not modified by the code.
        // we can get these coords either from the radii below or from CAD-generated coords.
        double bx[6], by[6], px[6], py[6];

        // a bunch of helpers
        double __inner(const double vec1[3], const double vec2[3]);
        void __cross(const double vec1[3], const double vec2[3], double prod[3]);
        void __normalize(double x[3]);
        void __sum(double x[3], double y[3], double z[3], double c = 1.);
        void __sum(const double x[3], const double y[3], double z[3], double c = 1.);
        // get coordinate frame defined by three points
        void __getframe(const double padCoords[3][3], double frame[3][3]);
        // tansform vector u from given frame to global and store result in v
        // -- multipliy by the transpose of frame.
        void __toGlobal(const double u[3], const double frame[3][3], double v[3]);
        void __MatrixMultiply(double vec[3], double mat[3][3]);

        // variables with coords and such
        double fPanelCoords[6];
        double fPanelCoords_external[6];
        // pad ordering convention: looking from the base towards the panel
        // and counting COUNTER-CLOCKWISE, with SP1 being the bottom pad
        double fPadCoords[3][3];
        // actuator ordering: looking from the base towards the panel and counting
        // CLOCKWISE, with ACT1 being at the top right (so going towards SP2 from
        // above)
        double fActL[6];
        // to keep track of this discrepancy in ordering, use an internal convention
        // where pads are ordered in the same way as the actuators, and assign a
        // map that switches the order -- this is the public PadMap above.
        // The caller shouldn't care about this difference or need to access PadMap

        // constants defining the dimensions -- hardcoded for now
        const double Pi = 3.14159265358979323846;
        const double r2d = 180./Pi;
        const double Rb = 320.; // base radius in mm
        const double initL = 427.919; // magic init act length in mm
        const double bracketT = 25.4*1.875; // bracket height in mm -- two of these add up to the actuator length to form total axis-to-axis distance
        const double jointT = 73.254; // joint thickness in mm -- actuator axis to pad
        const double padT = 6.2; // pad thickness in mm
        const double panelT = 33.4; // panel thickness in mm
        //const double laserE = 0.0; // laser extension

        // the type of panel we are computing things for
        PanelType fPanel;

        // pre-computed norms at the locations of the pads -- should be in the same
        // order as the panel types in the PanelType enum above.
        // the norms are pointing outside from the back surface of the panel.
        // the computation was done with the ROBAST library written by Akira Okumura.
        // mirror definitions taken from Daniel Nieto.
        //
        // These are given in the SP pads frame, defined as follows:
        // the x axis goes from the bottom pad to the  barycenter of the pads,
        // the z axis is perpendicular to the plane of the pads, pointing towards
        // the panel from the base triangle; z ^ x = y
        //
        // using 15 decimal places in scientific for highest possible accuracy with doubles
        const double fPadNorms[PANELNUM][3][3] = { 
            {{5.097668129476970e-03, 1.047654179548714e-02, -9.999321256223591e-01},
                {-1.139023920803046e-02, 0.000000000000000e+00, -9.999351291212767e-01},
                {5.097668129476970e-03, -1.047654179548714e-02, -9.999321256223591e-01}},
            {{2.282953930408088e-03, 9.607558782302985e-03, -9.999512402790428e-01},
                {-8.576078760768602e-03, 0.000000000000000e+00, -9.999632247603353e-01},
                {2.282953930408088e-03, -9.607558782302985e-03, -9.999512402790428e-01}},
            {{2.416787326798328e-02, 4.165752266588766e-02, -9.988396091000016e-01},
                {-4.825925442124093e-02, 0.000000000000000e+00, -9.988348433863861e-01},
                {2.416787326798328e-02, -4.165752266588766e-02, -9.988396091000016e-01}},
            {{2.207153924068211e-02, 4.109652812313958e-02, -9.989113687068392e-01},
                {-4.637346218008032e-02, 0.000000000000000e+00, -9.989241723000966e-01},
                {2.207153924068211e-02, -4.109652812313958e-02, -9.989113687068392e-01}},
            {{0.,0.,-1.}, {0.,0.,-1.}, {0.,0.,-1.}}
        };
        // norms of the pads in the frame of the actuator axes
        double fPadNorm_Axs[3][3];
};



#endif // #ifndef __STEWARTPLATFORM_H__
