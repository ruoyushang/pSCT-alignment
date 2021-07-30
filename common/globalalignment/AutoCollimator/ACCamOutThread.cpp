/*
 * Written by Patrick Wilcox
 * University of Iowa
 * Stable Work in progress
 * 
 * Camera Output Thread (CamOutThread)
 * 
 * Work horse class to take an image,
 * find the AutoCollimator spot.
 * 
 * Single method 'cycle' runs the analysis
 * using the given camera and Cam inputs
 * creating a fresh output each time.
 *
 */

#include "common/globalalignment/AutoCollimator/ACCamOutThread.hpp"

using namespace std;

ACCamOutThread::ACCamOutThread(AravisCamera *camera, const ACCamInputs *ACin) :
        pfCamera(camera),
        imgWidth(ACin->CCDWIDTH),
        imgHeight(ACin->CCDHEIGHT),
        pfACin(ACin) {
}

ACCamOutThread::~ACCamOutThread() = default;

bool ACCamOutThread::cycle() {
//Timestamp
    time_t t = time(nullptr);
    struct tm *theTime = gmtime(&t);
    char strTime[16];
    strftime(strTime, 16, "%Y%m%d%H%M%S", theTime);

//Take and save picture
//
    // save the current camera frame to a vector
    spdlog::trace("Status of stream: {}",pfCamera->isReady());
    vector<unsigned char> theFrame = pfCamera->captureFrame();
    spdlog::trace("Frame captured");
    int troubleshoot_tries(0);
    //troubleshoot a missing frame
    if (theFrame.empty()) {
        spdlog::warn("ACCamOutThread: frame not received, resending request");
        theFrame = pfCamera->captureFrame();
        while (theFrame.empty()) {// troubleshoot missing frame again and again...
            spdlog::warn("... trying again up to 3 times (frame not received)");
            theFrame = pfCamera->captureFrame();
            troubleshoot_tries++;
            if (troubleshoot_tries > 3) {
                spdlog::warn("Did not succeed in capturing frame after 3 tries. Ending cycle.");
                return false;
            }
        }
    } // end if empty frame
    spdlog::trace("Frame not empty, moving to create Image.");
    //save the frame to the Image class
    Image theImage(theFrame);
    theImage.setImageDir("/home/ctauser/Pictures/GAS_CCD/");

    //save to disk
    if (pfACin->SAVEIMAGE) {
        if (pfACin->SAVEFORMAT == "fits")
            theImage.saveFITSImage();
        if (pfACin->SAVEFORMAT == "raw")
            theImage.saveRawImage(strTime);
    }
    //end taking/saving picture

    //Do Image analysis if not requested to only capture image
    if (pfACin->CAPTUREONLY)
        return false;

    //Analysis below:
    theImage.pixels();


    // save filtered image
    if (pfACin->SAVEIMAGE) {
        theImage.savefilteredFITSImage();
    }
    return true;
    //End image analysis
}
