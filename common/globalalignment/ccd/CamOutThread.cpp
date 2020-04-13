/*
 * Written by Patrick Wilcox
 * University of Iowa
 * Stable Work in progress
 * 
 * Camera Output Thread (CamOutThread)
 * 
 * Work horse class to take an image,
 * find the LEDs in the image, and 
 * determine the solid body rotation.
 * 
 * Single method 'cycle' runs the analysis
 * using the given camera and ledinputs 
 * creating a fresh output each time.
 * 
 */

#include "common/globalalignment/ccd/CamOutThread.h"

using namespace std;

CamOutThread::CamOutThread(AravisCamera *camera, const LEDinputs *li) :
        pfCamera(camera),
        imgWidth(li->CCDWIDTH),
        imgHeight(li->CCDHEIGHT),
        pfLEDsin(li) {
}

CamOutThread::~CamOutThread() = default;

bool CamOutThread::cycle(LEDoutputs *pLEDsout) {
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
        spdlog::warn("CamOutThread: frame not received, resending request");
        theFrame = pfCamera->captureFrame();
        while (theFrame.empty()) {// troubleshoot missing frame again and again...
            spdlog::warn("... trying again up to 5 times (frame not received)");
            theFrame = pfCamera->captureFrame();
            troubleshoot_tries++;
            if (troubleshoot_tries > 5) {
                spdlog::warn("Did not succeed in capturing frame after 5 tries. Ending cycle.");
                return false;
            }
        }
    } // end if empty frame
    spdlog::trace("Frame not empty, moving to create Image.");
    //save the frame to the Image class
    Image theImage(theFrame);

    //save to disk
    //save to disk
    if (pfLEDsin->SAVEIMAGE) {
        if (pfLEDsin->SAVEFORMAT == "fits")
            theImage.saveFITSImage(pfLEDsin, strTime);
        if (pfLEDsin->SAVEFORMAT == "raw")
            theImage.saveRawImage(strTime);
    }

//
//end taking/saving picture

//Do Image analysis if not requested to only capture image
    if (pfLEDsin->CAPTUREONLY)
        return false;

    //create our basic output struct
    strcpy(pLEDsout->TIME, strTime);
    vector<ImageStar> theCentroids;
    spdlog::trace("Made ImageStar object.");
    // find the stars
    StarDetect theDetector = StarDetect(theImage, theCentroids, pLEDsout);
    spdlog::trace("Made StarDetect object.");
    // save filtered image
    if (pfLEDsin->SAVEIMAGE)
        theImage.savefilteredFITSImage(pLEDsout, strTime);
    //print the current LED positions
    spdlog::info("TIME: {}", strTime);
    for (auto & theCentroid : theCentroids) {
        spdlog::info("{}, {}", theCentroid.pX(), theCentroid.pY());
    }

    // attempt solid body fit
    FitLED fittheleds(pLEDsout);

    return true;
    //End image analysis
}
