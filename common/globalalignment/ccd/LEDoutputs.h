#pragma once

#include "common/globalalignment/ccd/LEDinputs.h"
#include <time.h>

// this is the populated encapsulation of the output file
struct LEDoutputs {

    const LEDinputs *inleds;
    double MJD = 0;
    double LEDPOS[8][2]{{0}}; // [led#][x,y]
    double LEDERR[8][2]{{0}}; // [led#][x,y]
    double LEDVAL[8][2]{{0}}; // [led#][0=intensity,1=fwhm]
    double SPACE[6]{0}; // [0=X,Y,Z,XROT,YROT,5=ZROT]
    double TEMP = 0;
    int FAULT = 0;
    int LEDSCOUNT = 0;
    bool LEDSPRESENT[8]{0};
    bool LEDSMATCH = false;
    char TIME[16]{0};

    LEDoutputs(const LEDinputs *inl) : inleds(inl) {
        spdlog::trace("Created LEDoutputs");
    }
};
