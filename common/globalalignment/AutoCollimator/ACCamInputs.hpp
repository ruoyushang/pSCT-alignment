/*
 * Originally written by Patrick Wilcox
 * University of Iowa
 * Stable Work in progress
 * 
 * AC Cam Input Parameters (ACCamInputs)
 * 
 * A data structure to mirror the input 
 * database parameters for the panel/CCD pairs.
 *  
 */


#pragma once

// string manipulation and console i/o
#include <string>
#include <iostream>
#include <fstream>

//spdlog
#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

struct ACCamInputs {

    //default values set to Lab testing camera
    // DATABASE 'MIRROR'
    //CCD Identification
    double STARTDATE = 0.0;
    std::string CCDNAME = "LED_Alignment";
    bool CCDACTIVE = 1;
    //CCD Details
    int CCDGAIN = 0;
    double CCDEXP = 650000; //microsec
    int CCDHEIGHT = 1944; //px
    int CCDWIDTH = 2592; //px
    double PIXSIZE = 2.2E-6; //m
    std::string CCDMAC = "00074805579C";
    std::string CCDSN = "02410108";
    //Lens details
    double LENSFL = 50E-3; //m
    double LENSSCALE = 1.0;
    std::string LENSSN = "";
    //Other values
    double THRESHOLD = 2.0; // multiplicity above mean image value used for erosion/dilation and led detection
    int TEMP = 1;

// END DATABASE 'MIRROR'

// Other parameters 
    bool VERBOSE = false;
    bool SAVEIMAGE = false;
    bool CAPTUREONLY = false;
    std::string SAVEFORMAT = "fits";

    void printall(std::ofstream &fout) {
        // need to add all parameters in an easy to read format.
        fout << "CCDNAME=" << CCDNAME << std::endl;
        fout << "CCDACTIVE=" << CCDACTIVE << std::endl;
        fout << "CCDGAIN=" << CCDGAIN << std::endl;
        fout << "CCDEXP=" << CCDEXP << " (microsec)" << std::endl;
        fout << "CCDHEIGHT=" << CCDHEIGHT << " (px)" << std::endl;
        fout << "CCDWIDTH=" << CCDWIDTH << " (px)" << std::endl;
    }

    void printall() {
        // need to add all parameters in an easy to read format.
        spdlog::info("CCDNAME={}", CCDNAME);
        spdlog::info("CCDACTIVE={}", CCDACTIVE);
        spdlog::info("CCDGAIN={}", CCDGAIN);
        spdlog::info("CCDEXP={} (microsec)", CCDEXP);
        spdlog::info("CCDHEIGHT={} (px)", CCDHEIGHT);
        spdlog::info("CCDWIDTH={} (px)", CCDWIDTH);
    }

};


