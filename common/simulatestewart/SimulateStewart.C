#include <iostream>

#include "TVector.h"
#include "TMatrixD.h"
#include "TDecompLU.h"
#include "TGeoManager.h"
#include "TGeoCone.h"
#include "TGeoTube.h"
#include "TGeoCompositeShape.h"
#include "TCanvas.h"

#include "common/utilities/spdlog/spdlog.h"

Double_t InnerProduct(Int_t n, Double_t *vec1, Double_t *vec2);
void CrossProduct(Double_t *vec1, Double_t *vec2, Double_t *prod);
void MatrixMultiplyLeft(Int_t n, Double_t A[3][3], Double_t B[3][3]);
void PrintMatrix(int n, Double_t R[6][6]);
void TransformAngles(Double_t *plat);

static Double_t Pi = 3.14159265358979323846;
static Double_t r2d = 180./Pi;
static Double_t Rb = 320.; // base radius in mm
static Double_t Rp = 320.; // payload radius in mm
static Double_t initL = 427.919 + 2*25.4*1.875; // magic init axis-to-axis distance in mm: actuator + two brackets = 523.169mm
static Double_t bracketT = 25.4*1.875; // bracket height in mm -- two of these add up to the actuator length to form total axis-to-axis distance
static Double_t jointT = 73.254; // joint thickness in mm -- actuator axis to pad
static Double_t padT = 6.2; // pad thickness in mm
static Double_t panelT = 33.4; // panel thickness in mm
//static Double_t laserE = 0.0; // laser extension

// bx, by, px and py are the coordinates of attachment points of actuators.
// bx and by are the coordinates on the base platform;
// px and py are the coordinates on the payload platform;
// these are all in the frames of reference attached to their respective platforms 
// (hence no z-coordinates) and are therefore fixed -- they are part of the specification
// and not modified by the code.
// we can get these coords either from the radii above or from CAD-generated coords.
static Double_t bx[6], by[6], px[6], py[6];

/*
 * STEWART PLATFORM NEWTON-RAPHSON
 */
void ComputeStewart(Double_t *actL, Double_t *plat, Double_t act[6][3],
        Double_t eps = 1e-12)
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

    Double_t a[6] = {5., 0., 0., 0.1, 0.1, 0.1}; // a = (x,y,z,phi,theta,psi)
    for (Int_t i = 0; i < 6; i++) a[2] += actL[i]/6.;
    //a[0] = (actL[2] + actL[3] - actL[0] - actL[5])/2.;
    //a[1] = (actL[3] + actL[4] + actL[5] - actL[0] - actL[1] - actL[2])/3.;

    Int_t iter = 0;
    Double_t u[6], v[6], w[6];

    while (1) {
        // the following are all auxiliary variables
        Double_t xbar[6], ybar[6];
        
        // Z->X->Z
    /*      Double_t ux = cos(a[3])*cos(a[5]) - sin(a[3])*cos(a[4])*sin(a[5]), // 1
                 uy =-cos(a[3])*sin(a[5]) - sin(a[3])*cos(a[4])*cos(a[5]), // 0
                 vx = sin(a[3])*cos(a[5]) + cos(a[3])*cos(a[4])*sin(a[5]), // 0
                 vy =-sin(a[3])*sin(a[5]) + cos(a[3])*cos(a[4])*cos(a[5]), // 1
                 wx = sin(a[4])*sin(a[5]), // 0
                 wy = sin(a[4])*cos(a[5]); // 0 
    */
        // X->Y->Z
    /*    Double_t ux = cos(a[3])*cos(a[4]), // 1
                 uy =-sin(a[3])*cos(a[5]) + cos(a[3])*sin(a[4])*sin(a[5]), // 0
                 vx = sin(a[3])*cos(a[4]), // 0
                 vy = cos(a[3])*cos(a[5]) + sin(a[3])*sin(a[4])*sin(a[5]), // 1
                 wx = -sin(a[4]), // 0
                 wy = cos(a[4])*sin(a[5]); // 0
    */
        // Z->Y->X
        Double_t ux = cos(a[4])*cos(a[5]),
                 uy = -cos(a[4])*sin(a[5]),
                 vx = cos(a[3])*sin(a[5]) + sin(a[3])*sin(a[4])*cos(a[5]),
                 vy = cos(a[3])*cos(a[5]) - sin(a[3])*sin(a[4])*sin(a[5]),
                 wx = sin(a[3])*sin(a[5]) - cos(a[3])*sin(a[4])*cos(a[5]),
                 wy = sin(a[3])*cos(a[5]) + cos(a[3])*sin(a[4])*sin(a[5]);

        // f is the function used in Newton-Raphson method. It is the forward kinematic
        // transformation that finds the lengths of actuators given the location and
        // orientation of the payload panel.
        Double_t f[6];
        for (Int_t i = 0; i < 6; i++)
        {
            xbar[i] = a[0] - bx[i];
            ybar[i] = a[1] - by[i];

            u[i] = ux*px[i] + uy*py[i];
            v[i] = vx*px[i] + vy*py[i];
            w[i] = wx*px[i] + wy*py[i];

            // we actually need -f(a), so the sign in the following expression is reversed
            f[i] = -(xbar[i] + u[i])*(xbar[i] + u[i]) -
                    (ybar[i] + v[i])*(ybar[i] + v[i]) -
                    (a[2] + w[i])*(a[2] + w[i]) + actL[i]*actL[i];
        }

        // check if f(a) is close enough to zero
        Double_t fmag = 0.;
        for (Int_t i = 0; i < 6; i++) fmag += fabs(f[i]);
        if (eps > fmag) break;

        // if f(a) isn't close enough to zero, solve Jf(a)*s = -f(a) for s;
        // here, Jf is the Jacobian matrix for f.
        Double_t Jf[6*6];

        for (Int_t i = 0; i < 6; i++)
        {
            Jf[i*6 + 0] = 2*(xbar[i] + u[i]);
            Jf[i*6 + 1] = 2*(ybar[i] + v[i]);
            Jf[i*6 + 2] = 2*(a[2] + w[i]);
            // Z->X->Z; X->Y->Z:
            //Jf[i*6 + 3] = 2*(ybar[i]*u[i] - xbar[i]*v[i]);

            // Z->Y->X:
            Jf[i*6 + 3] = 2*(a[2]*v[i] - ybar[i]*w[i]);
            
            // Z->X->Z:
            //Jf[i*6 + 4] = 2*(px[i]*sin(a[5]) + py[i]*cos(a[5]))*
            //   (xbar[i]*sin(a[3])*sin(a[4]) - ybar[i]*cos(a[3])*sin(a[4]) + a[2]*cos(a[4]));

            // X->Y->Z:
            //Jf[6*i + 4] = 2*((ybar*sin(a[3]) + xbar*cos(a[3]))*w[i] - 
            //        (px[i]*cos(a[4]) + py[i]*sin(a[4])*sin(a[5]))*a[2] );

            // Z->Y->X
            Jf[i*6 + 4] = 2*( (xbar[i] + u[i])*sin(a[4])*(sin(a[5])*py[i] - cos(a[5])*px[i])
                    + ((ybar[i] + v[i])*sin(a[3]) - (a[2] + w[i])*cos(a[3]))*u[i]);

            // Z->X->Z, Z->Y->X:
            Jf[i*6 + 5] = 2*(xbar[i]*(uy*px[i] - ux*py[i]) + ybar[i]*(vy*px[i] - vx*py[i]) + 
                          a[2]*(wy*px[i] - wx*py[i]));

            // X->Y->Z
            // Jf[6*i + 5] = 2*py[i]*((xbar[i]+u[i])*(sin(a[3])*sin(a[5]) + cos(a[3])*sin(a[4])*cos(a[5]))
            //        + (ybar[i]+v[i])*(sin(a[3])*sin(a[4])*cos(a[5]) - cos(a[3])*sin(a[5]) ) 
            //        + (a[2]+w[i])*cos(a[4])*cos(a[5])); 
        }

        // solve  Jf(a)*s = -f(a) for s
        TMatrixD JfMat(6,6,Jf); 
        TVectorD fVec(6,f);
        TDecompLU JfLU(JfMat, eps);
        JfLU.Solve(fVec);
        for (Int_t i = 0; i < 6; i++) f[i] = fVec[i];

        // this gives us the solution to Jf(a)*s = -f(a). Check if this is close to zero:
        fmag = 0.;
        for (Int_t i = 0; i < 6; i++) fmag += fabs(f[i]);
        if (eps > fmag) break;

        for (Int_t i = 0; i < 6; i++) a[i] += f[i];

        ++iter;
    }
    spdlog::debug("Took {} iterations.", iter + 1);
    // store the results
    for (Int_t i = 0; i < 6; i++) {
        plat[i] = a[i];
        act[i][0] = a[0] + u[i];
        act[i][1] = a[1] + v[i];
        act[i][2] = a[2] + w[i];
    }
    // compute the norm to the plane of the pads
    Double_t p_norm[3];
    Double_t vec1[3] = {act[2][0] - act[0][0], act[2][1] - act[0][1],
                        act[2][2] - act[0][2]};
    Double_t vec2[3] = {act[4][0] - act[0][0], act[4][1] - act[0][1],
                        act[4][2] - act[0][2]};
    CrossProduct(vec1, vec2, p_norm);
    Double_t normNN = sqrt(InnerProduct(3, p_norm, p_norm));
    for (Int_t i = 0; i < 3; i++) p_norm[i] /= normNN;
    // translate the panel along the norm by the thickness of (joint + pad + panel)
    for (Int_t i = 0; i < 3; i++)
        plat[i] += (jointT + padT + panelT)*p_norm[i];
    // translate the panel along the z-axis by the thickness of the joint (on the triangle side)
    plat[2] += jointT;

}

/*
 * BUILD THE STEWART PLATFORM 
 */
void BuildStewart()
{
    // create the world, materials and such
    TGeoManager * geom = new TGeoManager();
    TGeoMaterial *mat = new TGeoMaterial("Vacuum",0,0,0);
    TGeoMedium *med = new TGeoMedium("Vacuum",1,mat);
    TGeoVolume *top = geom->MakeBox("world", med, 1000., 1000., 1000.);
    geom->SetTopVolume(top);

    // make the axes
    TGeoTube * axisShape = new TGeoTube("axis",0,2,600);
    TGeoCone * axisArrow = new TGeoCone("arrow",6,0,4,0,0);
    TGeoTranslation * trB = new TGeoTranslation("trB",0,0,606);
    trB->RegisterYourself();
    TGeoCompositeShape * axis = new TGeoCompositeShape("axis+(arrow:trB)"); 
    // z-axis:
    TGeoVolume * ZaxisVolume = new TGeoVolume("ZaxisV",axis,med);
    ZaxisVolume->SetLineColor(kGray+3); // kBlack doesn't seem to work
    top->AddNode(ZaxisVolume,1);
    // x-axis
    TGeoVolume * XaxisVolume = new TGeoVolume("XaxisV",axis,med);
    XaxisVolume->SetLineColor(kBlue);
    top->AddNode(XaxisVolume,1,new TGeoRotation("",90,90,0));
    // y-axis
    TGeoVolume * YaxisVolume = new TGeoVolume("YaxisV",axis,med);
    YaxisVolume->SetLineColor(kRed);
    top->AddNode(YaxisVolume,1,new TGeoRotation("",0,-90,0));
    // done with axes

    // make the platforms
    TGeoTube *baseS = new TGeoTube("baseS", 0, Rb, 5);
    TGeoTube *payloadS = new TGeoTube("payloadS", 0, Rp, 5);

    TGeoVolume *baseV = new TGeoVolume("baseV", baseS, med);
    baseV->SetLineColor(kBlue);
    TGeoVolume *payloadAssembly = new TGeoVolumeAssembly("payloadAssembly");
    TGeoVolume *payloadV = new TGeoVolume("payloadV", payloadS, med);
    payloadV->SetLineColor(kRed);
    payloadAssembly->AddNode(payloadV, 1);
    // add axes on the payload assembly
    payloadAssembly->AddNode(XaxisVolume, 1, new TGeoRotation("", 90, 90, 0));
    payloadAssembly->AddNode(YaxisVolume, 1, new TGeoRotation("", 0, -90, 0));

    top->AddNode(baseV, 1, new TGeoTranslation("", 0, 0, -5));
    // done with platforms

    // assign actuator coords. all these are static variables, so needs to be
    // done only once.
    for (Int_t i = 0; i < 3; i++) {
        bx[2*i] = bx[2*i - 1 + 6*(i==0)] = Rb*cos(2*i*Pi/3);
        by[2*i] = by[2*i - 1 + 6*(i==0)] = Rb*sin(2*i*Pi/3);
        px[2*i] = px[2*i + 1] = Rp*cos(2*i*Pi/3 + Pi/3);
        py[2*i] = py[2*i + 1] = Rp*sin(2*i*Pi/3 + Pi/3);
    }

    // assign initial actuator lengths
    Double_t actL[6], platA[6], actCoords[6][3];
    for (Int_t i = 0; i < 6; i++) actL[i] = initL;

    // make an actuator shape
    TGeoTube *actuatorS = new TGeoTube("actS", 0, 20, initL/2);
    // compute orientation of the payload platform
    ComputeStewart(actL, platA, actCoords);
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 3; j++)
            spdlog::debug("{} ", actCoords[i][j]);
        spdlog::debug("\n");
    }
    spdlog::debug("{}", platA[2]);

    // rotations should all be zeros at this point
    TGeoRotation *payloadR = new TGeoRotation("", platA[3]*r2d, platA[4]*r2d, platA[5]*r2d);
    // add the payload platform to the world
    top->AddNode(payloadAssembly, 1, new TGeoCombiTrans(platA[0], platA[1], platA[2], payloadR));

    TGeoVolumeAssembly *StewartPlatform = new TGeoVolumeAssembly("StewartPlatform");
    for (Int_t i = 0; i < 6; i++) {
        TGeoVolume *actuatorV = new TGeoVolume("actV", actuatorS, med);
        actuatorV->SetLineColor(Color_t(i));
        TGeoTranslation *actTr = new TGeoTranslation("", 
                (actCoords[i][0] + bx[i])/2., (actCoords[i][1] + by[i])/2., 
                (actCoords[i][2] + jointT)/2.);
        actTr->RegisterYourself();
        
        Double_t theta = acos(actCoords[i][2]/actL[i]);
        Double_t phi = atan2(actCoords[i][1] - by[i], actCoords[i][0] - bx[i]);
        TGeoRotation *actRot = new TGeoRotation("", r2d*phi + 90., r2d*theta, 0.);
        actRot->RegisterYourself();

        StewartPlatform->AddNode(actuatorV, i, new TGeoCombiTrans(*actTr, *actRot));
    }
    top->AddNode(StewartPlatform, 1);

    geom->CloseGeometry();
    TCanvas *canGeometry = new TCanvas("canGeometry", "canGeometry", 800, 800);
    top->Draw("ogl");

    // COMPUTE THE ALIGNMENT MATRIX *
    double AlignmentMatrix[6*6];
    
    //TransformAngles(platA);
    std::string temp;
    for (int j = 0; j < 6; j++) 
    {
        for (int i = 0; i < 3; i++)
        {
              AlignmentMatrix[i*6 + j] = -platA[i+3];
              AlignmentMatrix[(i+3)*6 + j] = -platA[i];
        }
        for (int i = 0; i < 6; i++)
            temp += AlignmentMatrix[i * 6 + j] + " ";
        temp += "\n";
    }
    spdlog::debug(temp);
    //PrintMatrix(1, AlignmentMatrix);
  
    for (int j = 0; j < 6; j++)
    {
        actL[j] += 0.001;
        ComputeStewart(actL, platA, actCoords);
        actL[j] -= 0.001;
        //TransformAngles(platA);
        for (int i = 0; i < 3; i++) 
        {
            AlignmentMatrix[i*6 + j] += platA[i+3];
            AlignmentMatrix[(i+3)*6 + j] += platA[i];
        }
    }

    temp.clear();
    for (int i = 0; i  < 6; i++)
    {
        for (int j = 0; j < 6; j++)
            temp += 1000 * AlignmentMatrix[i * 6 + j] + " ";
        temp += "\n";
    }
    spdlog::debug(temp);

    TMatrixD AlignmentMat(6,6,AlignmentMatrix); 
    AlignmentMat *= 1000;
    AlignmentMat.Invert();
    temp.clear();
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 6; j++)
            temp += AlignmentMat.GetMatrixArray()[i * 6 + j] + " ";
        temp += "\n";
    }
    spdlog::debug("and its inverse:\n {}", temp)

    /* END OF ALIGNMENT MATRIX COMPUTATION */


    /* INTERACTIVELY COMPUTE PLATFORM POSITION BASED ON SET ACTUATOR LENGTHS */
    /*
//    while(1)
//    {
        for (Int_t i = 0; i < 6; i++) std::cin >> actL[i];
        ComputeStewart(actL, platA, actCoords);
        // grab a hold of the payload platform
        char *path = "/world_1/payloadAssembly_1";
        TGeoPhysicalNode *pnode = new TGeoPhysicalNode(path);
        TGeoHMatrix *newTransf = new TGeoHMatrix;

        std::string temp;
        for (Int_t i=0; i<6; i++) temp += platA[i] + ", ";
        temp += "\n";

        spdlog::debug("New actuator lengths: {}", temp);
        
        // making rotations explicit: Z->Y->X (gamma->beta->alpha)
        newTransf->RotateZ(platA[5]*r2d);
        newTransf->RotateY(platA[4]*r2d);
        newTransf->RotateX(platA[3]*r2d);

        newTransf->SetTranslation(platA);

        pnode->Align(newTransf);
        pnode->Refresh();

        TGeoPhysicalNode *actPnode[6];
        TGeoTube *newActS[6];
        TGeoHMatrix *newActTransf[6];
        TGeoRotation *newActRot[6];
        for (Int_t i = 0; i < 6; i++) 
        {
            path = Form("/world_1/StewartPlatform_1/actV_%d", i);
            actPnode[i] = new TGeoPhysicalNode(path);

            newActS[i] = new TGeoTube("actS", 0, 20, actL[i]/2);
            
            Double_t newActCoords[3] = {(actCoords[i][0] + bx[i])/2., 
                (actCoords[i][1] + by[i])/2., (actCoords[i][2] + jointT)/2.};

            newActTransf[i] = new TGeoHMatrix;
            newActTransf[i]->SetTranslation(newActCoords);

            theta = acos(actCoords[i][2]/actL[i]);
            phi = atan2(actCoords[i][1] - by[i], actCoords[i][0] - bx[i]);
            newActRot[i] = new TGeoRotation("", r2d*phi + 90., r2d*theta, 0.);
            newActTransf[i]->SetRotation(newActRot[i]->GetRotationMatrix());

            actPnode[i]->Align(newActTransf[i], newActS[i]);
            actPnode[i]->Refresh();
        }
//    }
    */

}

// extract small "CARTESIAN" angles from Euler angles 
// (CARTESIAN here means angles of rotation around fixed axes, not axes attached to the body)
void TransformAngles(Double_t *plat)
// [alpha, beta, gamma] in degrees
// plat = [dx, dy, dz, Phi, Theta, Psi] -> [dx, dy, dz, alpha, beta, gamma]
{
    //
    double Rt[3][3], R[3][3];
    // set R to Rz:
    R[0][0] = R[1][1] = cos(plat[3]);
    R[2][2] = 1.0;
    R[1][0] = sin(plat[3]);
    R[0][1] = -sin(plat[3]);
    R[0][2] = R[2][0] = R[1][2] = R[2][1] = 0.0;

    // set Rt to Rx -- see Euler angle convention for ROOT:
    Rt[1][1] = Rt[2][2] = cos(plat[4]);
    Rt[0][0] = 1.0;
    Rt[2][1] = sin(plat[4]);
    Rt[1][2] = -sin(plat[4]);
    Rt[0][1] = Rt[1][0] = Rt[0][2] = Rt[2][0] = 0.0;

    // multiply the matrices
    MatrixMultiplyLeft(3, R, Rt);

    // set Rt to Rz:
    Rt[0][0] = Rt[1][1] = cos(plat[5]);
    Rt[2][2] = 1.0;
    Rt[1][0] = sin(plat[5]);
    Rt[0][1] = -sin(plat[5]);
    Rt[0][2] = Rt[2][0] = Rt[1][2] = Rt[2][1] = 0.0;

    MatrixMultiplyLeft(3, R, Rt);

    double tanalpha = R[1][0]/R[0][0];
    plat[5] = atan2(R[1][0], R[0][0]);
    plat[4] = atan2(-R[0][2], R[0][0]*sqrt(1+tanalpha*tanalpha));
    plat[3] = atan2(R[2][1], R[2][2]);
}

void PrintMatrix(int n, Double_t R[6][6])
{
    std::string matrix;
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 6; j++)
            matrix += n * R[i][j];
        matrix += "\n";
    }
    matrix += "\n";

    spdlog::debug(matrix);
}

// multiply two square matrices
void MatrixMultiplyLeft(Int_t n, Double_t A[3][3], Double_t B[3][3])
// n = dimension of the matrices
// will assign A = B*A
{
    // initialize 2D array, which is array of pointers to arrays:
    double **C = new double*[3];

    for (int i = 0; i < 3; i++)
    {
        C[i] = new double[3];
        for (int j = 0; j < 3; j++)
        {
            C[i][j] = 0.0;
            for (int k = 0; k < 3; k++)
                C[i][j] += B[i][k]*A[k][j];
        }
    }

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
            A[i][j] = C[i][j];
        // clean up this row:
        delete[] C[i];
    }

    delete[] C;
}

Double_t InnerProduct(Int_t n, Double_t *vec1, Double_t *vec2)
{
    // returns the inner product of vectors vec1 and vec2 of size n
    Double_t res = 0.;
    for (Int_t i=0; i<n; i++) res += vec1[i]*vec2[i];
    
    return res;
}

void CrossProduct(Double_t *vec1, Double_t *vec2, Double_t *prod)
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
