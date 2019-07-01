#include "stewartplatform.hpp"

#include "TVector.h"
#include "TMatrixD.h"
#include "TDecompLU.h"

const double StewartPlatform::kMirrorDistance = 8701.56;

StewartPlatform::StewartPlatform()
{
    // assign coords of the pads in the triangle/panel frames.
    for (int i = 0; i < 3; i++) {
        bx[2*i] = bx[2*i - 1 + 6*(i==0)] = Rb*cos(2*i*Pi/3);
        by[2*i] = by[2*i - 1 + 6*(i==0)] = Rb*sin(2*i*Pi/3);
        px[2*i] = px[2*i + 1] = kRp*cos(2*i*Pi/3 + Pi/3);
        py[2*i] = py[2*i + 1] = kRp*sin(2*i*Pi/3 + Pi/3);
    }

    // assign initial actuator lengths
    for (int i = 0; i < 6; i++) fActL[i] = initL;
}

/*
 * STEWART PLATFORM NEWTON-RAPHSON
 */
void StewartPlatform::ComputeStewart(const double *actL, double eps)
// *actL is an array of the wanted actuator lengths, needs to be supplied.
// *platA is the platform orientation vector: {x, y, z, alpha, beta, gamma}
// the angles are the RPY angles in the Z-Y-X (5->4->3) convention
// or the Euler angles in the Z-X-Z (5->4->3) convention
// (*act)[3] is the array of coords of actuator ends on the payload platform:
//                      {{x0,y0,z0}, ..., {x5,y5,z5}}
// eps is the required tolerance
{

    // the collection of actuator lengths is set by the caller.
    // this is stored in actL

    double axisL[6];
    double a[6] = {5., 0., 0., 0.1, 0.1, 0.1}; // a = (x,y,z,phi,theta,psi)
    for (int i = 0; i < 6; i++) {
        axisL[i] = actL[i] + 2*bracketT; // this is the axis-to-axis distance we actually use
        a[2] += axisL[i]/6.;
    }
    //a[0] = (actL[2] + actL[3] - actL[0] - actL[5])/2.;
    //a[1] = (actL[3] + actL[4] + actL[5] - actL[0] - actL[1] - actL[2])/3.;

    /* PRE-COMPUTE */
    // compute the pads' norms w.r.t. actuator axes in initial position
    //
    // first, assign initial pad coords -- these are pad coords in SP frame
    for (int i = 0; i < 3; i++) {
        fPadCoords[i][0] = px[i*2];
        fPadCoords[i][1] = py[i*2];
        fPadCoords[i][2] = 0.;
    }

    // get the axes coords -- these are translated from the pads by (jointT+padT+panelT)
    // in the directions of the respective norms 
    double axCoords[3][3];
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            __sum(fPadCoords[i], fPadNorms[fPanel][i], axCoords[i], jointT + padT + panelT);

    // get the frame of reference attached to the axes 
    double axFrame[3][3];
    __getframe(axCoords, axFrame);

    // pick the norm vectors we're interested in
    // and transform them to the the axes frame of reference
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            fPadNorm_Axs[i][j] = fPadNorms[fPanel][i][j];
        __MatrixMultiply(fPadNorm_Axs[i], axFrame);
    }

    // find axes coordinates in the axes' frame -- these serve as the new payload coordinates
    // to perform the Newton-Raphson comutation with
    double ax[6], ay[6];
    double axOrigin[] = {0.,0.,0.};
    for (int i = 0; i < 3; i++)
        __sum(axOrigin, axCoords[i], axOrigin, 1./3.);

    for (int i = 0; i < 3; i++) {
        __sum(axCoords[i], axOrigin, axCoords[i], -1.); 
        ax[2*i] = ax[2*i + 1] = __inner(axCoords[i], axFrame[0]);
        ay[2*i] = ay[2*i + 1] = __inner(axCoords[i], axFrame[1]);
    }

    /* MOVE ON TO THE ACTUAL COMPUTATION */
    int iter = 0;
    double u[6], v[6], w[6];
    double act[6][3]; // actuator coordinates

    while (1) {
        // the following are all auxiliary variables
        double xbar[6], ybar[6];
        
        // Z->X->Z
    /*      double ux = cos(a[3])*cos(a[5]) - sin(a[3])*cos(a[4])*sin(a[5]), // 1
                 uy =-cos(a[3])*sin(a[5]) - sin(a[3])*cos(a[4])*cos(a[5];), // 0
                 vx = sin(a[3])*cos(a[5]) + cos(a[3])*cos(a[4])*sin(a[5]), // 0
                 vy =-sin(a[3])*sin(a[5]) + cos(a[3])*cos(a[4])*cos(a[5]), // 1
                 wx = sin(a[4])*sin(a[5]), // 0
                 wy = sin(a[4])*cos(a[5]); // 0 
    */
        // X->Y->Z
    /*    double ux = cos(a[3])*cos(a[4]), // 1
                 uy =-sin(a[3])*cos(a[5]) + cos(a[3])*sin(a[4])*sin(a[5]), // 0
                 vx = sin(a[3])*cos(a[4]), // 0
                 vy = cos(a[3])*cos(a[5]) + sin(a[3])*sin(a[4])*sin(a[5]), // 1
                 wx = -sin(a[4]), // 0
                 wy = cos(a[4])*sin(a[5]); // 0
    */
        // Z->Y->X
    /*    double ux = cos(a[4])*cos(a[5]),
                 uy = -cos(a[4])*sin(a[5]),
                 vx = cos(a[3])*sin(a[5]) + sin(a[3])*sin(a[4])*cos(a[5]),
                 vy = cos(a[3])*cos(a[5]) - sin(a[3])*sin(a[4])*sin(a[5]),
                 wx = sin(a[3])*sin(a[5]) - cos(a[3])*sin(a[4])*cos(a[5]),
                 wy = sin(a[3])*cos(a[5]) + cos(a[3])*sin(a[4])*sin(a[5]);
    */
        // X->Z->Y (Vladimir's convention)
        double ux = cos(a[3])*cos(a[4]),
                 uy = sin(a[3])*sin(a[5]) - cos(a[3])*sin(a[4])*cos(a[5]),
                 vx = sin(a[4]),
                 vy = cos(a[4])*cos(a[5]),
                 wx = -sin(a[3])*cos(a[4]),
                 wy = cos(a[3])*sin(a[5]) + sin(a[3])*sin(a[4])*cos(a[5]);

        // f is the function used in Newton-Raphson method. It is the forward kinematic
        // transformation that finds the lengths of actuators given the location and
        // orientation of the payload panel.
        double f[6];
        for (int i = 0; i < 6; i++)
        {
            xbar[i] = a[0] - bx[i];
            ybar[i] = a[1] - by[i];

            u[i] = ux*ax[i] + uy*ay[i];
            v[i] = vx*ax[i] + vy*ay[i];
            w[i] = wx*ax[i] + wy*ay[i];

            // we actually need -f(a), so the sign in the following expression is reversed
            f[i] = -(xbar[i] + u[i])*(xbar[i] + u[i]) -
                    (ybar[i] + v[i])*(ybar[i] + v[i]) -
                    (a[2] + w[i])*(a[2] + w[i]) + axisL[i]*axisL[i];
        }

        // check if f(a) is close enough to zero
        double fmag = 0.;
        for (int i = 0; i < 6; i++) fmag += fabs(f[i]);
        if (eps > fmag) break;

        // if f(a) isn't close enough to zero, solve Jf(a)*s = -f(a) for s;
        // here, Jf is the Jacobian matrix for f.
        double Jf[6*6];

        for (int i = 0; i < 6; i++)
        {
            Jf[i*6 + 0] = 2*(xbar[i] + u[i]);
            Jf[i*6 + 1] = 2*(ybar[i] + v[i]);
            Jf[i*6 + 2] = 2*(a[2] + w[i]);
            // Z->X->Z; X->Y->Z:
            //Jf[i*6 + 3] = 2*(ybar[i]*u[i] - xbar[i]*v[i]);

            // Z->Y->X:
            //Jf[i*6 + 3] = 2*(a[2]*v[i] - ybar[i]*w[i]);
            
            // Z->Y->X:
            Jf[i*6 + 3] = 2*(xbar[i]*w[i] - a[2]*u[i]);

            // Z->X->Z:
            //Jf[i*6 + 4] = 2*(ax[i]*sin(a[5]) + ay[i]*cos(a[5]))*
            //   (xbar[i]*sin(a[3])*sin(a[4]) - ybar[i]*cos(a[3])*sin(a[4]) + a[2]*cos(a[4]));

            // X->Y->Z:
            //Jf[6*i + 4] = 2*((ybar*sin(a[3]) + xbar*cos(a[3]))*w[i] - 
            //        (ax[i]*cos(a[4]) + ay[i]*sin(a[4])*sin(a[5]))*a[2] );

            // Z->Y->X
            //Jf[i*6 + 4] = 2*( (xbar[i] + u[i])*sin(a[4])*(sin(a[5])*ay[i] - cos(a[5])*ax[i])
            //        + ((ybar[i] + v[i])*sin(a[3]) - (a[2] + w[i])*cos(a[3]))*u[i]);

            // Z->Y->X (Vladimir's convention)
            Jf[i*6 + 4] = 2*( (-xbar[i] - u[i])*(cos(a[3])*sin(a[4])*ax[i]
                                + cos(a[3])*cos(a[4])*cos(a[5])*ay[i])
                            + (ybar[i] + v[i])*(cos(a[4])*ax[i]
                                - sin(a[4])*cos(a[5])*ay[i])
                            + (a[2] + w[i])*(-sin(a[3])*sin(a[4])*ax[i]
                                + sin(a[3])*cos(a[4])*cos(a[5])*ay[i]) );

            // Z->X->Z, Z->Y->X:
            // Jf[i*6 + 5] = 2*(xbar[i]*(uy*ax[i] - ux*ay[i]) + ybar[i]*(vy*ax[i] - vx*ay[i]) +
            //              a[2]*(wy*ax[i] - wx*ay[i]));

            // X->Y->Z
            // Jf[6*i + 5] = 2*ay[i]*((xbar[i]+u[i])*(sin(a[3])*sin(a[5]) + cos(a[3])*sin(a[4])*cos(a[5]))
            //        + (ybar[i]+v[i])*(sin(a[3])*sin(a[4])*cos(a[5]) - cos(a[3])*sin(a[5]) ) 
            //        + (a[2]+w[i])*cos(a[4])*cos(a[5])); 

            // Z->Y->X (Vladimir's convention)
            Jf[6*i + 5] = 2*ay[i]*( (xbar[i] + u[i])*
                              (sin(a[3])*cos(a[5]) + cos(a[3])*sin(a[4])*sin(a[5]))
                            - (ybar[i] + v[i])*cos(a[4])*sin(a[5])
                            + (a[2] + w[i])*
                              (cos(a[3])*cos(a[5]) - sin(a[3])*sin(a[4])*sin(a[5])) );
        }

        // solve  Jf(a)*s = -f(a) for s
        TMatrixD JfMat(6,6,Jf); 
        TVectorD fVec(6,f);
        TDecompLU JfLU(JfMat, eps);
        JfLU.Solve(fVec);
        for (int i = 0; i < 6; i++) f[i] = fVec[i];

        // this gives us the solution to Jf(a)*s = -f(a). Check if this is close to zero:
        fmag = 0.;
        for (int i = 0; i < 6; i++) fmag += fabs(f[i]);
        if (eps > fmag) break;

        for (int i = 0; i < 6; i++) a[i] += f[i];

        ++iter;
    }
    // store the results
    for (int i = 0; i < 6; i++) {
        fPanelCoords[i] = a[i];
        act[i][0] = a[0] + u[i];
        act[i][1] = a[1] + v[i];
        act[i][2] = a[2] + w[i];
    }

    /* AXES' POSITIONS FOUND. NOW FIND THE PADS' POSITIONS */
    // transform pad norms back into the global frame
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            axCoords[i][j] = act[2*i][j];
    __getframe(axCoords, axFrame);
    double pnorm[3];
    for (int i = 0; i < 3; i++) {
        __toGlobal(fPadNorm_Axs[i], axFrame, pnorm);
        __sum(act[2*i], pnorm, fPadCoords[i], -(jointT + padT + panelT));
    }

    // finally, translate the pads by jointT along z -- this is the joint on
    // the triangle side
    for (int i = 0; i < 3; i++) 
        fPadCoords[i][2] += jointT;
               
    /* this no longer works, since the panel gets rotated as well as translated when 
     * local norms are taken into account. However, it still works for Optical tables,
     * so we keep it here with that caveat -- user beware! */
    double vec1[3], vec2[3];
    __sum(act[2], act[0], vec1, -1.);
    __sum(act[4], act[0], vec2, -1.);
    __cross(vec1, vec2, pnorm);
    __normalize(pnorm);

    // translate the panel along the norm by the thickness of (joint + pad + panel).
    // and translate the actuator coords by the thickness of (joint + pad +panel) to get
    // the pad coords.
    __sum(fPanelCoords, pnorm, fPanelCoords, jointT + padT + panelT);
   
    // translate the panel along the z-axis by the thickness of the joint (on the triangle side)
    fPanelCoords[2] += jointT;
    // convert to mm
    for (int i = 3; i < 6; i++) fPanelCoords[i] *= kRp;

    for (int i = 0; i < 3; i++) {
        fPanelCoords_external[i] = fPanelCoords[i];
        fPanelCoords_external[i + 3] = fPanelCoords[6 - fRotOrder[i]];
    }
}


// compute actuator lengths based on a known panel position and a given order of rotations
void StewartPlatform::ComputeActsFromPanel(const double *panel_external)
{
    // transform into internal order, (x, y, z, R3, R2, R1)
    double panel[6];
    for (int i = 0; i < 3; i++) {
        panel[i] = panel_external[i];
        panel[i+3] = panel_external[3 + fRotOrder[2-i] - 1];
    }

    // do this in the reverse order from the above, with the benefit that there's no need
    // to involve Newton-Raphson this time

    // get the pad coordinates -- rotate the original ones in the same direction as the panel
    double rot[3][3][3];
    // initialize the matrices

    // assign the rotation matrices
    for (int i = 0; i < 3; i++) {
        int axis = fRotOrder[i] - 1;
        // assign all elements to sines first
        for (int j = 0; j < 3; j++)
            for (int k = 0; k < 3; k++)
                rot[i][j][k] = sin(panel[5-i]/kRp);
        //diagonal elements are cosines
        for (int j = 0; j < 3; j++)
            rot[i][j][j] = cos(panel[5-i]/kRp);
        // the on-axis elements are 0.
        for (int j = 0; j < 3; j++)
            rot[i][axis][j] = rot[i][j][axis] = 0.;
        // the axis itself is 1.
        rot[i][axis][axis] = 1.;
        // assign the minus sign
        rot[i][0][1] *= -1.;
        rot[i][1][2] *= -1.;
        rot[i][2][0] *= -1.;
    }

    // find pad coords
    // first assign the initial ones in the panel reference
    for (int i = 0; i < 3; i++) {
        fPadCoords[i][0] = px[i*2];
        fPadCoords[i][1] = py[i*2];
        fPadCoords[i][2] = 0.;
    }
    // now rotate these into the panel orientation
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            __MatrixMultiply(fPadCoords[i], rot[j]);
    
    // and translate them by the same distance as the panel
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            fPadCoords[i][j] += panel[j];

    // transform local norms at the locations of the pads from the SP frame
    // to the global frame. First, get the frame:
    double SPframe[3][3];
    __getframe(fPadCoords, SPframe);

    // now convert the pad norms from SP frame to the global frame
    double norms[3][3];
    for (int k = 0; k < 3; k++)
        __toGlobal(fPadNorms[fPanel][k], SPframe, norms[k]);

    // now find the coords of the actuator axes
    double actCoords[6][3];
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            actCoords[2*i][j] = actCoords[2*i + 1][j] = fPadCoords[i][j]
                + (padT + jointT + panelT)*norms[i][j]; // + because pad norms point toward the base!

    // subtract the coords of the base of the actuators and find the length
    for (int i = 0; i < 6; i++) {
            actCoords[i][0] -= bx[i];
            actCoords[i][1] -= by[i];
            actCoords[i][2] -= jointT;
            fActL[i] = sqrt(__inner(actCoords[i], actCoords[i])) - 2*bracketT;
    }
}

void StewartPlatform::SetPadCoords(const double padCoords[3][3])
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            fPadCoords[i][j] = padCoords[PadMap.at(i)][j];

}

// compute actuator lengths based on known pad coords
void StewartPlatform::ComputeActsFromPads(const double padCoords[3][3])
{
    // do this just like above, with the added simplification that pad coords
    // are already known

    SetPadCoords(padCoords);
    // transform local norms from the SP frame to the global frame
    // first, get the SP frame
    double SPframe[3][3];
    __getframe(fPadCoords, SPframe);

    // now convert the pad norms from SP frame to the global frame
    double norms[3][3];
    for (int k = 0; k < 3; k++)
        __toGlobal(fPadNorms[fPanel][k], SPframe, norms[k]);

    // now find the coords of the actuator axes
    double actCoords[6][3];
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            actCoords[2*i][j] = actCoords[2*i + 1][j] = fPadCoords[i][j] 
                + (padT + jointT + panelT)*norms[i][j]; // + because pad norms point toward the base!

    // subtract the coords of the base of the actuators and find the length
    for (int i = 0; i < 6; i++) { 
            actCoords[i][0] -= bx[i];
            actCoords[i][1] -= by[i];
            actCoords[i][2] -= jointT;
            fActL[i] = sqrt(__inner(actCoords[i], actCoords[i])) - 2*bracketT;
    }
}

/*
 * BUILD THE STEWART PLATFORM 
 */
void StewartPlatform::BuildPlatform()
{
    /* making rotations explicit: Z->Y->X (gamma->beta->alpha)
        newTransf->RotateZ(platA[5]*r2d);
        newTransf->RotateY(platA[4]*r2d);
        newTransf->RotateX(platA[3]*r2d);
    */

}

// multiply a vector by a square matrix -- transform coords
void StewartPlatform::__MatrixMultiply(double vec[3], double mat[3][3])
// will assign vec = mat*vec
{
    double temp[3];

    for (int i = 0; i < 3; i++)
    {
        temp[i] = 0.;
        for (int j = 0; j < 3; j++)
            temp[i] += mat[i][j]*vec[j];
    }

    for (int i = 0; i < 3; i++)
        vec[i] = temp[i];
}

void StewartPlatform::__sum(double x[3], double y[3], double z[3], double c)
{
    for (int i = 0; i < 3; i++)
        z[i] = x[i] + c*y[i];
}

void StewartPlatform::__sum(const double x[3], const double y[3], double z[3], double c)
{
    for (int i = 0; i < 3; i++)
        z[i] = x[i] + c*y[i];
}

double StewartPlatform::__inner(const double vec1[3], const double vec2[3])
{
    // returns the inner product of vectors vec1 and vec2 of size n
    double res = 0.;
    for (int i=0; i<3; i++) res += vec1[i]*vec2[i];

    return res;
}

void StewartPlatform::__cross(const double vec1[3], const double vec2[3], double prod[3])
{
    // gives the cross product of vectors vec1 and vec2 in prod.
    // works only in 3D -- returns product of the first three components of vec1 and vec2
    // (x1,y1,z1)x(x2,y2,z2) = |i  j  k | = (y1*z2 - z1*y2, z1*x2-z2*x1, x1*y2-y1*x2)
    //                         |x1 y1 z1|
    //                         |x2 y2 z2|

    prod[0] = vec1[1]*vec2[2] - vec1[2]*vec2[1];
    prod[1] = vec1[2]*vec2[0] - vec1[0]*vec2[2];
    prod[2] = vec1[0]*vec2[1] - vec1[1]*vec2[0];
}

void StewartPlatform::__normalize(double x[3])
{
    double norm = sqrt(__inner(x, x));
    for (int i = 0; i < 3; i++)
        x[i] /= norm;
}

// get coordinate frame defined by three points
void StewartPlatform::__getframe(const double padCoords[3][3], double frame[3][3])
{
    // transform local norms from the SP frame to the global frame
    // first, get the SP frame
    double origin[3] = {0.,0.,0.};
    for (int i = 0; i < 3; i++)
        __sum(origin, padCoords[i], origin, 1./3.);

    // X
    __sum(origin, padCoords[1], frame[0], -1.);
    __normalize(frame[0]);
    // Z
    double vec1[3], vec2[3];
    __sum(padCoords[2], padCoords[1], vec1, -1.);
    __sum(padCoords[0], padCoords[1], vec2, -1.);
    __cross(vec1, vec2, frame[2]);
    __normalize(frame[2]);
    // Y
    __cross(frame[2], frame[0], frame[1]);
}

void StewartPlatform::__toGlobal(const double u[3], const double frame[3][3], double v[3])
{
    for (int i = 0; i < 3; i++) {
        v[i] = 0.;
        for (int j = 0; j < 3; j++)
            v[i] += frame[j][i]*u[j];
    }
}
