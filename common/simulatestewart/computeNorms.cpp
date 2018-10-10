#include "../SegmentedSC_panel.h" 
#include "AGeoAsphericDisk.h"
#include <iostream>
#include <string>
#include <iomanip>


// the root coordinate system has its z-axis from the primary to the secondary,
// the secondary is curved towards the origin, while the primary is curved
// away from the origin

// PRIMARY PADS
double PadsCoordsP1[3][3] = {
    {2.9843973*m, 0.2771281*m, 0.1750409*m},
    {2.5071922*m, 0.0000000*m, 0.1233165*m},
    {2.9843973*m,-0.2771281*m, 0.1750409*m}
};

double PadsCoordsP2[3][3] = {
    {4.2909753*m, 0.2771281*m, 0.3494850*m},
    {3.8161544*m, 0.0000000*m, 0.2791626*m},
    {4.2909753*m, -0.2771281*m, 0.3494850*m}
};


// SECONDARY PADS
double PadsCoordsS1[3][3] = {
    {1.2731926*m, -0.2771281*m, 8.250887211*m - 3./2.*SCTelescope::kFp},
    {0.7999935*m, 0.000000*m, 8.33140952*m - 3./2.*SCTelescope::kFp},
    {1.2731926*m, 0.2771281*m, 8.250887211*m - 3./2.*SCTelescope::kFp}
};

double PadsCoordsS2[3][3] = {
    {2.3321339*m, -0.2771281*m, 7.953174941*m - 3./2.*SCTelescope::kFp},
    {1.8782451*m, 0.000000*m, 8.109331132*m - 3./2.*SCTelescope::kFp},
    {2.3321339*m, 0.2771281*m, 7.953174941*m - 3./2.*SCTelescope::kFp}
};

double inner(double x[3], double y[3])
{
    double sum = 0.;
    for (int i = 0; i < 3; i++)
        sum += x[i]*y[i];

    return sum;
}

void cross(double x[3], double y[3], double z[3])
{
    z[0] = x[1]*y[2] - x[2]*y[1];
    z[1] = x[2]*y[0] - x[0]*y[2];
    z[2] = x[0]*y[1] - x[1]*y[0];
}

void sum(double x[3], double y[3], double z[3], double c = 1.)
{
    for (int i = 0; i < 3; i++)
        z[i] = x[i] + c*y[i];
}

void normalize(double x[3])
{
    double norm = sqrt(inner(x, x));
    for (int i = 0; i < 3; i++)
        x[i] /= norm;
}

using namespace std;

int main(int argc, char *argv[])
{
    double padsXYZ[3][3];
    bool onPrimary = true;

    cout << scientific << setprecision(15);
    if (string(argv[1]) == "P1") {
        cout << "computing P1 norms" << endl;
        for (int pad = 0; pad < 3; pad++)
           for (int coord = 0; coord < 3; coord++)
              padsXYZ[pad][coord] = PadsCoordsP1[pad][coord]; 
    }
    if (string(argv[1]) == "P2") {
        cout << "computing P2 norms" << endl;
        for (int pad = 0; pad < 3; pad++)
           for (int coord = 0; coord < 3; coord++)
              padsXYZ[pad][coord] = PadsCoordsP2[pad][coord]; 
    }
    if (string(argv[1]) == "S1") {
        cout << "computing S1 norms" << endl;
        for (int pad = 0; pad < 3; pad++)
           for (int coord = 0; coord < 3; coord++)
              padsXYZ[pad][coord] = PadsCoordsS1[pad][coord]; 
        onPrimary = false;
    }
    if (string(argv[1]) == "S2") {
        cout << "computing S2 norms" << endl;
        for (int pad = 0; pad < 3; pad++)
           for (int coord = 0; coord < 3; coord++)
              padsXYZ[pad][coord] = PadsCoordsS2[pad][coord]; 
        onPrimary = false;
    }


    AGeoAsphericDisk *mirror;

    double dir[3] = {0., 0., 1.};

    if (onPrimary)
    {
        double thck = SCTelescope::kPMirrorThickness;
        mirror = new AGeoAsphericDisk("primaryV",
           SCTelescope::kZp + SCTelescope::kzp[0], 0,
           SCTelescope::kZp + SCTelescope::kzp[0] + thck, 0,
           SCTelescope::kDp/2., 0);
        mirror->SetPolynomials(SCTelescope::kNPar - 1, &SCTelescope::kzp[1],
                SCTelescope::kNPar - 1, &SCTelescope::kzp[1]);
        dir[2] = -1.;
    }
    else
    {
        double thck = SCTelescope::kSMirrorThickness;
        mirror = new AGeoAsphericDisk("secondaryV",
           SCTelescope::kZs + SCTelescope::kzs[0] - thck, 0,
           SCTelescope::kZs + SCTelescope::kzs[0], 0,
           SCTelescope::kDs/2., 0.);
        mirror->SetPolynomials(SCTelescope::kNPar - 1, &SCTelescope::kzs[1],
                SCTelescope::kNPar - 1, &SCTelescope::kzs[1]);
    }

    double m_norm[3][3];
    cout << "Mirror frame norms:" << endl;
    for (int pad = 0; pad < NPADS; pad++)
    {
        double z;
        double r = sqrt(padsXYZ[pad][0]*padsXYZ[pad][0] + padsXYZ[pad][1]*padsXYZ[pad][1]);
        if (onPrimary)
            z = mirror->CalcF1(r);
        else
            z = mirror->CalcF2(r);

        padsXYZ[pad][2] = z;
        mirror->ComputeNormal(padsXYZ[pad], dir, m_norm[pad]);
        for (int coord = 0; coord < 3; coord++)
            cout << m_norm[pad][coord] << " ";
        cout << endl;
    }

    // convert mirror frame norm into SP frame
    // This is the frame attached to the pads. the x axis goes from the bottom pad to the 
    // barycenter of the pads, the z axis is perpendicular to the plane of the pads 
    // towards the panel from the base triangle; z ^ x = y
    double origin[3] = {0., 0., 0.};
    for (int i = 0; i < 3; i++)
        sum(origin, padsXYZ[i], origin, 1./3.);
    
    double SPframe[3][3];
    // get the x-axis. 
    sum(origin, padsXYZ[1], SPframe[0], -1.);
    normalize(SPframe[0]);
    // now the z
    double vec1[3], vec2[3];
    sum(padsXYZ[2], padsXYZ[1], vec1, -1.);
    sum(padsXYZ[0], padsXYZ[1], vec2, -1.);
    cross(vec1, vec2, SPframe[2]);
    normalize(SPframe[2]);
    // and the y
    cross(SPframe[2], SPframe[0], SPframe[1]);

    cout << endl;
    cout << "SP Frame:" << endl;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            cout << SPframe[i][j] << " ";
        cout << endl;
    }
    cout << endl;

    cout << "SP frame norms:" << endl;
    double m_SPnorm[3][3];
    for (int i = 0; i < 3; i++) {
        cout << "{";
        for (int j = 0; j < 3; j++) {
            m_SPnorm[i][j] = inner(SPframe[j], m_norm[i]);
            cout << m_SPnorm[i][j] << ", ";
        }
        cout << "}, ";
    }

    cout << endl;
    cout << "Consistency check (should be all zeros):" << endl;
    // transform back SP frame norms to global frame for self-consistency:
    for (int k = 0; k < 3; k++) {
        double gnorm[3] = {0.,0.,0.};
        for (int i = 0; i < 3; i++) 
            for (int j = 0; j < 3; j++) 
                gnorm[i] += SPframe[j][i]*m_SPnorm[k][j];
        sum(gnorm, m_norm[k], gnorm, -1.);
        cout << inner(gnorm, gnorm) << endl;
    }

    // find the norms in the frame of the axes' ends
    cout << endl;
    double jointT = 80.;
    // get the axes frame
    double axesXYZ[3][3];
    for (int i = 0; i < 3; i++)
        sum(padsXYZ[i], m_norm[i], axesXYZ[i], jointT);

    cout << "Pads coords:" << endl;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            cout << padsXYZ[i][j] << " ";
        cout << endl;
    }
    
    cout << "Axes coords:" << endl;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            cout << axesXYZ[i][j] << " ";
        cout << endl;
    }

    for (int i = 0; i < 3; i++)
        origin[i] = 0.;
    for (int i = 0; i < 3; i++)
        sum(origin, axesXYZ[i], origin, 1./3.);
   
    double AXframe[3][3];
    // get the x-axis. 
    sum(origin, axesXYZ[1], AXframe[0], -1.);
    normalize(AXframe[0]);
    // now the z
    sum(axesXYZ[2], axesXYZ[1], vec1, -1.);
    sum(axesXYZ[0], axesXYZ[1], vec2, -1.);
    cross(vec1, vec2, AXframe[2]);
    normalize(AXframe[2]);
    // and the y
    cross(AXframe[2], AXframe[0], AXframe[1]);

    cout << endl;
    cout << "AX Frame:" << endl;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            cout << AXframe[i][j] << " ";
        cout << endl;
    }
    cout << endl;

    cout << "AX frame norms:" << endl;
    double m_AXnorm[3][3];
    for (int i = 0; i < 3; i++) {
        cout << "{";
        for (int j = 0; j < 3; j++) {
            m_AXnorm[i][j] = inner(AXframe[j], m_norm[i]);
            cout << m_AXnorm[i][j] << ", ";
        }
        cout << "}, ";
    }
    cout << endl;

    return 0;
}
