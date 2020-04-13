#include "common/globalalignment/ccd/FitLED.h"

using namespace std;

FitLED::FitLED(LEDoutputs *lo) : ledsin(lo->inleds), ledsout(lo) {


    pixscale = ledsin->PIXSIZE / (ledsin->LENSFL * ledsin->LENSSCALE); // radians/pixel calibrated to the given scale
    ipixscale = 1.0 / pixscale; // pixels/radian
    cx0 = ledsin->CCDWIDTH / 2.0; // camera center pixel in x
    cy0 = ledsin->CCDHEIGHT / 2.0; // camera center pixel in y

    ledsbuilt = new double *[ledsin->NLED];
    for (int i = 0; i < ledsin->NLED; i++) {
        ledsbuilt[i] = new double[2];
    }
    for (int i = 0; i < ledsin->NLED; i++) {
        ledsbuilt[i][0] = ledsin->LEDCCD[i][0] - cx0;
        ledsbuilt[i][1] = ledsin->LEDCCD[i][1] + cy0;
    }
    ledsfound = new double *[ledsin->NLED];
    for (int i = 0; i < ledsin->NLED; i++) {
        ledsfound[i] = new double[2];
    }
    for (int i = 0; i < ledsin->NLED; i++) {
        ledsfound[i][0] = ledsout->LEDPOS[i][0] - cx0;
        ledsfound[i][1] = ledsout->LEDPOS[i][1] + cy0;
    }

    // create optimization object using non-derivative BOBYQA with NSPACEPARAM number of free parameters
    nlopt::opt opt(nlopt::LN_BOBYQA, NSPACEPARAM);
    //populate upper and lower bounds
    vector<double> lb(NSPACEPARAM);
    vector<double> ub(NSPACEPARAM);
    for (int i = 0; i < NSPACEPARAM; i++) {
        lb[i] = ledsin->LEDLB[i];
        ub[i] = ledsin->LEDUB[i];
    }
    opt.set_lower_bounds(lb);
    opt.set_upper_bounds(ub);
    //define objective
    opt.set_min_objective(f_ledfit, this);
    //define end condition
    opt.set_xtol_rel(1e-6);
    // initial guess populated by expected panel position in space
    std::vector<double> x(NSPACEPARAM); //position in mm and rotation in rads
    x[0] = 0;
    x[1] = 0;
    x[2] = 8500;
    x[3] = 0;
    x[4] = 0;
    x[5] = PI / 2.0;
    double minf;
    opt.optimize(x, minf);

//	nlopt::result result = opt.optimize(x, minf);

    double **c = moveled(x);

    spdlog::info("min offset: {}", minf);
    spdlog::info("led offsets (ledpos - best guess)");

    for (int i = 0; i < ledsin->NLED; i++)
        if (ledsout->LEDSPRESENT[i]) {
            spdlog::info("LED # {}: {}, {}", i + 1, ledsfound[i][0] - c[i][0], ledsfound[i][1] - c[i][1]);
        }
    for (int i = 0; i < NSPACEPARAM; i++)
        ledsout->SPACE[i] = x[i];
}

double **FitLED::moveled(vector<double> p) {

    double psi = p[3];
    double theta = p[4];
    double phi = p[5];
    double rot[3][3] = {{cos(theta) * cos(phi),
                                sin(psi) * sin(theta) * cos(phi) - cos(psi) * sin(phi),
                                cos(psi) * sin(theta) * cos(phi) + sin(psi) * sin(phi)},
                        {cos(theta) * sin(phi),
                                sin(psi) * sin(theta) * sin(phi) + cos(psi) * cos(phi),
                                cos(psi) * sin(theta) * sin(phi) - sin(psi) * cos(phi)},
                        {-1.0 * sin(theta),
                                sin(psi) * cos(theta),
                                cos(psi) * cos(theta)}};

    double w[ledsin->NLED][3]; // store the rotated spatial guess

    for (int i = 0; i < ledsin->NLED; i++) {
        for (int j = 0; j < 3; j++) {
            //rotate and translate the original spatial coords to guess at the new position
            w[i][j] = p[j] +
                      (rot[j][0] * ledsin->LED[i][0] + rot[j][1] * ledsin->LED[i][1] + rot[j][2] * ledsin->LED[i][2]);
        }
    }

    // initialize 2d array to store the new led positions
    double **c = new double *[ledsin->NLED];
    for (int i = 0; i < ledsin->NLED; i++) {
        c[i] = new double[2];
    }

    // output the coordinates as LED positions
    for (int i = 0; i < ledsin->NLED; i++) {
        if (ledsout->LEDSPRESENT[i]) {
            c[i][0] = w[i][0] / w[i][2] * ipixscale;
            c[i][1] = w[i][1] / w[i][2] * ipixscale;
        }
    }

    return c;
}

double f_ledfit(const std::vector<double> &x, std::vector<double> &grad, void *f_data) {

    FitLED *thetest = (FitLED *) f_data;
    double **y = thetest->moveled(x);
    double **ledsf = thetest->getledsfound();
    double offset = 0.0;
    for (int i = 0; i < thetest->getLEDinputs()->NLED; i++) {
        if (thetest->getLEDoutputs()->LEDSPRESENT[i]) offset += pow(ledsf[i][0] - y[i][0], 2) +
                                                                pow(ledsf[i][1] - y[i][1], 2);
    }
    return offset;

}
