/*
 * Originally written by Patrick Wilcox
 * University of Iowa
 * Stable Work in progress
 * 
 * LED Input Parameters (LEDinputs)
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

struct LEDinputs {

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
    double CCDPOS[6] {0}; // CCD position on optical table (unused)
    //Lens details
    double LENSFL = 50E-3; //m
    double LENSSCALE = 1.0;
    std::string LENSSN = "";
    //Panel details
    int NLED = 6;
    double LEDCCD[8][2] = {{1421.68, -197.88},{849.961, -197.123},{849.23, -767.432},{1427.38, -768.4},
           {847.921, -1333.06},{1433.88, -1332.21},{0,0},{0,0}}; // [led#][x,y] in CCD (0,0 is upper left)
    double LED[8][3] = {{-199.76, 204.52, 0.0},  {-199.4, 0.0, 0.0},  {0.0, 0.0, 0.0},
           {0.0, 204.52, 0.0},{200.6, 0.0, 0.0},{200.76, 204.52, 0.0},{0,0,0},{0,0,0}};
    //Bounds of the panel rotation
    double LEDUB[6] = {1000.0,1000.0,10000.0,0.2,0.2,3.1415}; //[x,y,z,xrot,yrot,zrot]
    double LEDLB[6] = {-1000.0,-1000.0,0,-0.2,-0.2,-1.0*3.1415}; //[x,y,z,xrot,yrot,zrot]
    //Other values
    double THRESHOLD = 2.0; // multiplicity above mean image value used for erosion/dilation and led detection
    int TEMP = 1; 

// END DATABASE 'MIRROR'
    
// Other parameters 
    bool VERBOSE = false;
    bool SAVEIMAGE = false;
    bool CAPTUREONLY = false;
    std::string SAVEFORMAT = "fits";

    void printall(std::ofstream &fout)
    {
        // need to add all parameters in an easy to read format.
        fout << "CCDNAME=" << CCDNAME << std::endl;
        fout << "CCDACTIVE=" << CCDACTIVE << std::endl;
        fout << "CCDGAIN=" << CCDGAIN << std::endl;
        fout << "CCDEXP=" << CCDEXP << " (microsec)" << std::endl;
        fout << "CCDHEIGHT=" << CCDHEIGHT << " (px)" << std::endl;
        fout << "CCDWIDTH=" << CCDWIDTH << " (px)" << std::endl;
        fout << "NLED=" << NLED << std::endl;
        for(int i = 0; i<NLED; i++) fout << "LED" << i+1 << "CCD=(" << LEDCCD[i][0] << "," << LEDCCD[i][1] << ")" << std::endl;

    }

    void printall()
    {
        // need to add all parameters in an easy to read format.
        spdlog::info("CCDNAME={}", CCDNAME);
        spdlog::info("CCDACTIVE={}", CCDACTIVE);
        spdlog::info("CCDGAIN={}", CCDGAIN);
        spdlog::info("CCDEXP={} (microsec)", CCDEXP);
        spdlog::info("CCDHEIGHT={} (px)", CCDHEIGHT);
        spdlog::info("CCDWIDTH={} (px)", CCDWIDTH);
        spdlog::info("NLED={}", NLED);
        for(int i = 0; i<NLED; i++) {
            spdlog::info("LED {} CCD=({},{})", i+1, LEDCCD[i][0], LEDCCD[i][1]);
        }

    }

};


