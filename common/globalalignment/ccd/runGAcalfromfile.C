/*
 * Written by Patrick Wilcox
 * University of Iowa
 * Stable Work in progress
 *
 * Run Global Alignment calibration from a file (runGAcalfromfile)
 *
 * Single camera executible to output rotation
 * given the LED and spatial positions in a
 * file to be specified during execution.
 *
 *
 */



// file manipulation and console i/o
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// timing for troubleshooting
#include <chrono>

// prebuilt Camera interface
#include "AravisCamera.h"

// the work horse class designed to be a single thread
#include "CamOutThread.h"

// the i/o structure (should mirror the database and inputs only written from the start)
#include "LEDinputs.h"
#include "LEDoutputs.h"

// save some typing with sin/endl/cout
using namespace std;

int main(int argc, char *argv[]) {

//Set and initialize a few default values
    // constant for most LEDs possible
    const int MAXLED=8;
    // parameters to be potentially changed from command line
    bool saveimage = false;
        bool captureonly = false;
        string saveformat = "fits";
    bool verbose = false;
    int repeat = 1;

    // our input class, set to defaults (and expected to be populated from an input file, default values for testing included)
    LEDinputs leds;

    //pi (for the outputs)
    const double PI  = 3.141592653589793238463;

    // Read input parameters from command line
    if (argc == 1)
        {//use default values for camera
        //change nothing
        }
    else if (argc >= 2) {
            for(int j=1; j<argc; j++) {
                if(!strcmp(argv[j],"-f")) {
                    // single file processing
                    cout << "Reading parameters from file: " << argv[j+1] << endl;
                    std::ifstream fin(argv[j+1]);
                    string line;
                    while (std::getline(fin, line)) {
                        if (line[0] != '#') {
                            std::istringstream sin(line.substr(line.find("=") + 1));
                            if (line.find("CCDNAME") != -1)
                                    sin >> leds.CCDNAME;
                            else if (line.find("CCDACTIVE") != -1)
                                    sin >> leds.CCDACTIVE;
                            else if (line.find("CCDGAIN") != -1)
                                    sin >> leds.CCDGAIN;
                            else if (line.find("CCDEXP") != -1)
                                    sin >> leds.CCDEXP;
                            else if (line.find("CCDHEIGHT") != -1)
                                    sin >> leds.CCDHEIGHT;
                            else if (line.find("CCDWIDTH") != -1)
                                    sin >> leds.CCDWIDTH;
                            else if (line.find("NLED") != -1)
                                    sin >> leds.NLED;
                            else if (line.find("TEMP") != -1)
                                    sin >> leds.TEMP;
                            else if (line.find("PIXSIZE") != -1)
                                    sin >> leds.PIXSIZE;
                            else if (line.find("LENSFL") != -1)
                                    sin >> leds.LENSFL;
                            else if (line.find("LENSSCALE") != -1)
                                    sin >> leds.LENSSCALE;
                            else if (line.find("CCDMAC") != -1)
                                    sin >> leds.CCDMAC;
                            else if (line.find("THRESHOLD") != -1)
                                    sin >> leds.THRESHOLD;
                            else if (line.find("LEDUB") != -1) {
                                if(line.find("XPOS") != -1)
                                        sin >> leds.LEDUB[0];
                                else if(line.find("YPOS") != -1)
                                        sin >> leds.LEDUB[1];
                                else if(line.find("ZPOS") != -1)
                                        sin >> leds.LEDUB[2];
                                else if(line.find("XROT") != -1)
                                        sin >> leds.LEDUB[3];
                                else if(line.find("YROT") != -1)
                                        sin >> leds.LEDUB[4];
                                else if(line.find("ZROT") != -1)
                                        sin >> leds.LEDUB[5];
                            } // end else find LEDUB
                            else if (line.find("LEDLB") != -1) {
                                if(line.find("XPOS") != -1)
                                        sin >> leds.LEDLB[0];
                                else if(line.find("YPOS") != -1)
                                        sin >> leds.LEDLB[1];
                                else if(line.find("ZPOS") != -1)
                                        sin >> leds.LEDLB[2];
                                else if(line.find("XROT") != -1)
                                        sin >> leds.LEDLB[3];
                                else if(line.find("YROT") != -1)
                                        sin >> leds.LEDLB[4];
                                else if(line.find("ZROT") != -1)
                                        sin >> leds.LEDLB[5];
                            } // end else find LEDLB
                            else if (line.find("LED") != -1) {
                                for(int i = 1; i<=MAXLED; i++) {
                                    if(line.find("LED"+to_string(i)+"CCDX") != -1)
                                            sin >> leds.LEDCCD[i-1][0];
                                    if(line.find("LED"+to_string(i)+"CCDY") != -1)
                                            sin >> leds.LEDCCD[i-1][1];
                                    if(line.find("LED"+to_string(i)+"X") != -1)
                                            sin >> leds.LED[i-1][0];
                                    if(line.find("LED"+to_string(i)+"Y") != -1)
                                            sin >> leds.LED[i-1][1];
                                    if(line.find("LED"+to_string(i)+"Z") != -1)
                                            sin >> leds.LED[i-1][2];
                                } // end for LED counts 1-8
                            } // end else find LED
                        } //end if not a # comment
                    } // end while loop
                    fin.close();
                }  // end -f file processing
                else if (!strcmp(argv[j],"-s")) {
                    if ( (argc > j+1) && !strcmp(argv[j+1], "raw") )
                        saveformat="raw";
                    std::cout << "Will be saving image to ." << saveformat << " format" << std::endl;
                    saveimage=true;
                    leds.SAVEIMAGE=true;
                }
                else if (!strcmp(argv[j], "-A")) { // do NOT analyze
                    if ( (argc > j+1) && !strcmp(argv[j+1], "raw") )
                        saveformat="raw";
                    std::cout << "Will be saving image to ." << saveformat << " format" << std::endl;
                    saveimage=true;
                    leds.SAVEIMAGE=true;
                    captureonly = true;
                }
                else if (!strcmp(argv[j],"-v")) {
                    verbose=true;
                    leds.VERBOSE=true;
                }
                else if (!strcmp(argv[j-1],"-f") || ((!strcmp(argv[j-1], "-s") || !strcmp(argv[j-1], "-A")) && (argv[j][0] != '-' )) ) {
                        //ignore the file name as a parameter during the search (eg: previous value is -f)
                        //ignore the file format as a parameter during the search (eg: previous value is -s or -A)
                }
                else if (!strcmp(argv[j],"-r")) {
                        repeat = stoi(argv[j+1]);
                }
                else if (!strcmp(argv[j-1],"-r")) {
                        //ignore the repeat paramter if after repeat
                }
                else {
                    cout << "Syntax: " << endl;
                    cout << argv[0] << ""<< endl;
                    cout << "Options: " << endl;
                    cout << "-f <filename> (for using parameters in <filename>, default = 50mm test CCD/LENS)" << endl;
                    cout << "-s (save fits or raw image with current datetime.fits/raw as file name, default = does not save)" << endl;
                    cout << "-v (verbose output on , default = verbose output off)" << endl;
                    cout << "-r <#> (repeat <#> times sequentially, -1 repeats without end, 0 takes no images, default = 1)" << endl;
                    cout << "-A (do NOT analyze image, capture and save only)" << endl;
                    return 1;
                }
            }// end for j loop
        } // end argc >= 2 if
    //list the parameters present in our input leds struct
    if(verbose==true)
        leds.printall();
//
//End reading command line input

        // CREATE CAMERA OBJECT AND INITIALIZE IT
        AravisCamera *camera = nullptr;

        char * char_camera_name = new char[leds.CCDNAME.size() + 1];
        copy(leds.CCDNAME.begin(), leds.CCDNAME.end(), char_camera_name);
        char_camera_name[leds.CCDNAME.size()] = '\0';

        while (!camera || !camera->isReady()) {
            //attempt to initialize camera
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            camera = new AravisCamera(char_camera_name);
            if (!camera->isReady()) {
        cout << "AravisCamera: Camera [" << leds.CCDNAME << "] reports not ready, trying again..." << endl;
                continue;
            }
            if (!camera->setGain(leds.CCDGAIN))
                cout << "AravisCamera: Error setting gain, using stock value" << endl;
            if (!camera->setExposure(leds.CCDEXP))
                cout << "AravisCamera: Error setting exposure, using stock value" << endl;
        }// end while (!camera || !camera ready)

        // DONE INITIALIZING CAMERA

//Take and process image
    // threading necessary for adaptation for multicamera input (this is a single camera program, though)
    CamOutThread theThread = CamOutThread(camera, &leds);
    for(int i=repeat;i!=0; i--) { // any repeat value <0 continues indefinitly
            const LEDoutputs* ledsout = theThread.cycle(saveimage, saveformat, captureonly);
//Output final results
            //print solid body fit parameters if doing analysis
            if (!captureonly) {
                cout << "x =" << ledsout->SPACE[0] << endl;
                cout << "y =" << ledsout->SPACE[1] << endl;
                cout << "z =" << ledsout->SPACE[2] << endl;
                cout << "psi =" << ledsout->SPACE[3]*180.0/PI << endl;
                cout << "theta =" << ledsout->SPACE[4]*180.0/PI << endl;
                cout << "phi =" << ledsout->SPACE[5]*180.0/PI << endl;
                cout << "@OUTPUT " << ledsout->TIME << " " << ledsout->SPACE[0] << " " << ledsout->SPACE[1] << " " << ledsout->SPACE[2] << " " << ledsout->SPACE[3] << " " << ledsout->SPACE[4] << " " << ledsout->SPACE[5] << endl;
            }
        }

        if (camera) {
            delete camera;
            camera = nullptr;
        }

    return 0;
}
