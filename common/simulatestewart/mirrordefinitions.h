#ifndef __MIRRORDEFINITIONS_H__
#define __MIRRORDEFINITIONS_H__

#include <cmath> // pow()

namespace SCT {

    // UNIT DEFINITIONS
    // all units in mm by default
    const double um = 0.1;
    const double mm = 1.;
    const double cm = 10.;
    const double m = 1000.;
    const double inch = 25.4;

    // Common
    const int kNPar       = 8; // 8 parameters
    const double kFp      = 5.5863*m; // Fp ??
    const double kZf      = (3./2. - 1./3.)*kFp; // Focal length
    const double kk1      = -0.8327; // kappa1
    const double kk2      =  4.9950; // kappa2

    const double kActuatorLength = 427.919;
    const double kBaseRadius = 320.;

    // Camera stuff
    const double kPixelSize = 6.125*mm;
    const double kPixelPitch = 6.125*mm;
    const double kMAPMTWidth = 52.0*mm;
    const double kMAPMTLength = 32.7*mm;
    const double kInputWindowThickness = 1.5*mm;
    const double kMirrorThicknessDelta = 0.0*mm;
    const double kMAPMTAngularSize = 4.00/60.;
    const double kCameraDiameter = 0.78*m;


    // primary
    namespace Primary {
        // mirror segmentation -- inner and outer ring
        const int kPanels[2] = {16, 32};

        const double kZ      = 0*m; // Primary Position
        const double kD      = 4.831875*2*m; // Primary diameter
        const double kDm     = 3.4*2*m; // Primary inner ring outer diameter
        const double kDinner = 2.19350*2*m; // Primary Inner diameter
        const double kMirrorThickness = 33.4*mm; // Panel thickness

        // mirror parameters
        const double kz[] = {
            pow(kFp,  1)*1.11147e-05,
            pow(kFp, -1)*0.11092,
            pow(kFp, -3)*-0.0054743,
            pow(kFp, -5)*-0.008944,
            pow(kFp, -7)*0.014779,
            pow(kFp, -9)*-0.026488,
            pow(kFp,-11)*0.02293,
            pow(kFp,-13)*-0.010258
        };

        // Pads coordinates in the telescope frame
        const double PadsCoords[2][3][3] = {
            // P1
            {
                {2.5071922*m, 0.0000000*m, 0.1233165*m},
                {2.9843973*m, 0.2771281*m, 0.1750409*m},
                {2.9843973*m,-0.2771281*m, 0.1750409*m}
            },
            // P2
            {
                {3.8161544*m, 0.0000000*m, 0.2791626*m},
                {4.2909753*m, 0.2771281*m, 0.3494850*m},
                {4.2909753*m, -0.2771281*m, 0.3494850*m}
            }
        };
    };

    // secondary
    namespace Secondary {
        // mirror segmentation -- inner and outer ring
        const int kPanels[2] = {8, 16};

        // notice the origin is at zero! our frame of reference is at the mirror itself
        const double kZ      = 0.*m; // 3./2.*kFp; // Secondary Position in its frame
        const double kD      = 2.7081*2*m; // Secondary diameter 
        const double kDm     = 1.5965*2*m; // Secondary inner ring outer diameter
        const double kDinner = 0.395*2*m; // Secondary inner diameter
        const double kMirrorThickness = 34.2*mm; // Panel thickness

        // mirror parameters -- notice the opposite signs! this is because of our choice of frame
        // of reference centered at the mirror itself!
        const double kz[] = {
            pow(kFp,  1)*5.016585e-06,
            pow(kFp, -1)*0.4169942781,
            pow(kFp, -3)*0.1361436152,
            pow(kFp, -5)*-0.5406127539,
            pow(kFp, -7)*2.135329061,
            pow(kFp, -9)*-5.8095677,
            pow(kFp,-11)*9.94478388,
            pow(kFp,-13)*-7.903283749
        };

        // Pads coordinates in the telescope frame
        const double PadsCoords[2][3][3] = {
            // S1
            {
                {0.7999935*m, 0.000000*m, 0.0480964*m},
                {1.2731926*m, 0.2771281*m, 0.128618*m},
                {1.2731926*m, -0.2771281*m, 0.128618*m}
            },
            // S2
            {
                {1.8782451*m, 0.000000*m, 0.270175*m},
                {2.3321339*m, 0.2771281*m, 0.426331*m},
                {2.3321339*m, -0.2771281*m, 0.426331*m}
            }
        };
    };

};

#endif // #define __MIRRORDEFINITIONS_H__
