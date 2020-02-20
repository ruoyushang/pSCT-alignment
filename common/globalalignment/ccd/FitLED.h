#include "common/globalalignment/ccd/LEDoutputs.h"
#include <nlopt.hpp>
#include <iostream>

//spdlog
#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

// universal scope objective function for the optimization of LED detection
double f_ledfit(const std::vector<double> &x, std::vector<double> &grad, void* f_data);

class FitLED {
    public:
        FitLED(LEDoutputs* lo);
        const LEDinputs* getLEDinputs(){return ledsin;}
        LEDoutputs* getLEDoutputs(){return ledsout;}
        double** moveled(std::vector<double> p);
        double** getledsbuilt(){return ledsbuilt;}
        double** getledsfound(){return ledsfound;}

    private:
        std::vector<double> pinit;
        const LEDinputs* ledsin;
        LEDoutputs* ledsout;
        double** ledsbuilt;
        double** ledsfound;
        const double PI  = 3.141592653589793238463;
        const int NSPACEPARAM = 6;
        // camera plate scale
        // pixscale = 2.2E-6/12.5E-3 # radians/pixel 12.5 mm lens
        // default values to be overwritten during constructor
        double pixscale = 2.2E-6/12.5E-3*(1789.0/1849.0); // radians/pixel 12.5 mm lens calibrated
        double ipixscale = 1.0/pixscale; // pixels/radian
        double cx0 = 2592.0/2.0; // camera center pixel in x
        double cy0 = 1944.0/2.0; // camera center pixel in y

};
