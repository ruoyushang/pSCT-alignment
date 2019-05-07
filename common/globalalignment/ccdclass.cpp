// file manipulation and console i/o
#include "common/globalalignment/ccdclass.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <stdlib.h>
#include <limits>

// timing for troubleshooting
#include <chrono>

// save some typing with sin/endl/cout
using namespace std;

GASCCD::GASCCD() :
    pfLEDsOut(nullptr),
    pfCamThread(nullptr),
    pfCamera(nullptr),
    fConfigFile(""),
    fIsSim(false)
{
}

GASCCD::~GASCCD()
{
    // if anything still exists, delete it and clean up
    if (pfCamera) {
        delete pfCamera;
        pfCamera = nullptr;
    }
    if (pfCamThread) {
        delete pfCamThread;
        pfCamThread = nullptr;
    }
    if (pfLEDsOut) {
        delete pfLEDsOut;
        pfLEDsOut = nullptr;
    }
}


void GASCCD::setConfig(string config)
{
    fConfigFile = config;

    // constant for most LEDs possible
    const int MAXLED=8;
    
    // Set LEDsIn configuration
    std::ifstream fin(fConfigFile.c_str());
    string line;
    while (std::getline(fin, line)) {
        if (line[0] != '#') { 
            // set sin to the value after the equality sign
            std::istringstream sin(line.substr(line.find("=") + 1));
            // check what exactly we're reading
            if (line.find("CCDNAME") != string::npos)
                fLEDsIn.CCDNAME = sin.str();
            else if (line.find("CCDACTIVE") != string::npos)
                sin >> fLEDsIn.CCDACTIVE;
            else if (line.find("CCDGAIN") != string::npos)
                sin >> fLEDsIn.CCDGAIN;
            else if (line.find("CCDEXP") != string::npos)
                sin >> fLEDsIn.CCDEXP;
            else if (line.find("CCDHEIGHT") != string::npos)
                sin >> fLEDsIn.CCDHEIGHT;
            else if (line.find("CCDWIDTH") != string::npos)
                sin >> fLEDsIn.CCDWIDTH;
            else if (line.find("NLED") != string::npos)
                sin >> fLEDsIn.NLED;	
            else if (line.find("TEMP") != string::npos)
                sin >> fLEDsIn.TEMP;
            else if (line.find("PIXSIZE") != string::npos)
                sin >> fLEDsIn.PIXSIZE;
            else if (line.find("LENSFL") != string::npos)
                sin >> fLEDsIn.LENSFL;
            else if (line.find("LENSSCALE") != string::npos)
                sin >> fLEDsIn.LENSSCALE;
            else if (line.find("CCDMAC") != string::npos)
                sin >> fLEDsIn.CCDMAC;
            else if (line.find("THRESHOLD") != string::npos)
                sin >> fLEDsIn.THRESHOLD;
            else if (line.find("LEDUB") != string::npos) {
                if(line.find("XPOS") != string::npos)
                    sin >> fLEDsIn.LEDUB[0];
                else if(line.find("YPOS") != string::npos)
                    sin >> fLEDsIn.LEDUB[1];	
                else if(line.find("ZPOS") != string::npos)
                    sin >> fLEDsIn.LEDUB[2];	
                else if(line.find("XROT") != string::npos)
                    sin >> fLEDsIn.LEDUB[3];	
                else if(line.find("YROT") != string::npos)
                    sin >> fLEDsIn.LEDUB[4];	
                else if(line.find("ZROT") != string::npos)
                    sin >> fLEDsIn.LEDUB[5];
            } // end else find LEDUB
            else if (line.find("LEDLB") != string::npos) {
                if(line.find("XPOS") != string::npos)
                    sin >> fLEDsIn.LEDLB[0];
                else if(line.find("YPOS") != string::npos)
                    sin >> fLEDsIn.LEDLB[1];	
                else if(line.find("ZPOS") != string::npos)
                    sin >> fLEDsIn.LEDLB[2];	
                else if(line.find("XROT") != string::npos)
                    sin >> fLEDsIn.LEDLB[3];	
                else if(line.find("YROT") != string::npos)
                    sin >> fLEDsIn.LEDLB[4];	
                else if(line.find("ZROT") != string::npos)
                    sin >> fLEDsIn.LEDLB[5];
            } // end else find LEDLB	
            else if (line.find("LED") != string::npos) {
                for(int i = 1; i<=MAXLED; i++) {
                    if(line.find("LED"+to_string(i)+"CCDX") != string::npos)
                        sin >> fLEDsIn.LEDCCD[i-1][0];
                    if(line.find("LED"+to_string(i)+"CCDY") != string::npos)
                        sin >> fLEDsIn.LEDCCD[i-1][1];
                    if(line.find("LED"+to_string(i)+"X") != string::npos)
                        sin >> fLEDsIn.LED[i-1][0];
                    if(line.find("LED"+to_string(i)+"Y") != string::npos)
                        sin >> fLEDsIn.LED[i-1][1];
                    if(line.find("LED"+to_string(i)+"Z") != string::npos)
                        sin >> fLEDsIn.LED[i-1][2];
                } // end for LED counts 1-8
            } // end else find LED								
            else if (line.find("SAVEIMAGE") != string::npos)
                sin >> fLEDsIn.SAVEIMAGE;
            else if (line.find("SAVEFORMAT") != string::npos)
                sin >> fLEDsIn.SAVEFORMAT;
            else if (line.find("VERBOSE") != string::npos)
                sin >> fLEDsIn.VERBOSE;
            else if (line.find("CAPTUREONLY") != string::npos)
                sin >> fLEDsIn.CAPTUREONLY;
        } //end if not a # comment
    } // end while loop
    fin.close();
}

bool GASCCD::Initialize(bool isSim)
{

    fIsSim = isSim;

    // Print out all the params if needed
    std::ofstream logout;
    if (fLEDsIn.VERBOSE) {
        std::string fname = "Log/";
        fname += fLEDsIn.CCDNAME + ".log";
        logout.open(fname);

        logout << "Reading input config from " << fConfigFile << std::endl;
        logout << "------------------INPUTS------------------\n";
        fLEDsIn.printall(logout);
        logout << "------------------------------------------\n";
    }

    //initialize Camera
    //make the string camera_name -> char* char_camera_name
    const char * char_camera_name = fLEDsIn.CCDNAME.empty() ? NULL : fLEDsIn.CCDNAME.c_str();

    while (!pfCamera || !pfCamera->isReady()) {
        //attempt to initialize camera
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        pfCamera = new AravisCamera(char_camera_name);
        if (!pfCamera->isReady()) {
            std::string strout =
                    "GASCCD::initialize(): Camera [" + fLEDsIn.CCDNAME + "] reports not ready. Trying again...\n";
            std::cout << strout;

            if(fLEDsIn.VERBOSE) {
                logout << strout;
            }

            continue;
        }
        if (!pfCamera->setGain(fLEDsIn.CCDGAIN)) {
            std::string strout = "+++ WARNING: GASCCD::initialize(): Error setting gain, using stock value\n";
            std::cout << strout;

            if(fLEDsIn.VERBOSE) {
                logout << strout;
            }
        }
        if (!pfCamera->setExposure(fLEDsIn.CCDEXP)) {
            std::string strout = "+++ WARNING: GASCCD::initialize(): Error setting exposure, using stock value\n";
            std::cout << strout;

            if(fLEDsIn.VERBOSE) {
                logout << strout;
            }
        }
    }

    //End Camera Init	
    if (fLEDsIn.VERBOSE) {
        std::string strout = "GASCCD::initialize(): Set camera gain to "
                             + std::to_string(pfCamera->getGain()) + "\n";
        strout += "GASCCD::initialize(): Set camera exposure to "
                  + std::to_string(pfCamera->getExposure()) + "\n";
        logout << strout;
    }

    // Create output object
    pfLEDsOut = new LEDoutputs(&fLEDsIn);
    // Create camera working thread 
    // threading necessary for adaptation for multicamera input
    pfCamThread = new CamOutThread(pfCamera, &fLEDsIn);

    if (fLEDsIn.VERBOSE) {
        std::string strout = "GASCCD::initialize(): Set up a new camera working thread.\n";
        logout << strout;
    }

    if (fLEDsIn.VERBOSE) {
        logout.close();
    }

    if (!pfCamThread)
        return false;

    return true;
}

bool GASCCD::Update()
{
//    if (!fIsSim && pfCamera->isReady())
    return pfCamThread->cycle(pfLEDsOut);
//    else
//        pfLEDsOut = simulate(pfCamera->isReady());
}

void GASCCD::simulate(bool hasCamera)
{
/*
    if (hasCamera)
    {
        for (int pos = 0; pos < 6; pos++)
            pfLEDsOut->SPACE[pos] = static_cast<double>(rand() % 100000) / 100.;
    }
    else
    {
        for (int pos = 0; pos < 6; pos++)
            pfLEDsOut->SPACE[pos] = std::numeric_limits<int>::min();
    }
*/
}

void GASCCD::setNominalValues(int offset, double value)
{

    return;
}
