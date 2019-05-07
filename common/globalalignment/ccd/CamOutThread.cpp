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

CamOutThread::CamOutThread(AravisCamera *camera, const LEDinputs* li) : 
    pfCamera(camera),
    imgWidth(li->CCDWIDTH),
    imgHeight(li->CCDHEIGHT),
    pfLEDsin(li)
{
}

CamOutThread::~CamOutThread()
{
}

bool CamOutThread::cycle(LEDoutputs *pLEDsout)
{
//Timestamp
    time_t t = time(NULL);
    struct tm* theTime = gmtime(&t);
    char strTime[16];
    strftime(strTime,16,"%Y%m%d%H%M%S",theTime);

//Take and save picture
//
    // save the current camera frame to a vector
    vector<unsigned char> theFrame = pfCamera->captureFrame();
    //troubleshoot a missing frame
    if(theFrame.size()==0) {
        cout << "CamOutThread: frame not received, resending request" << endl;
        theFrame = pfCamera->captureFrame();
        while(theFrame.size()==0) {// troubleshoot missing frame again and again...
            cout << "... trying again (frame not received)" << endl;
            theFrame = pfCamera->captureFrame();
        }
    } // end if empty frame
    
    //save the frame to the Image class
    Image theImage(theFrame);

    //save to disk
    //save to disk
    if(pfLEDsin->SAVEIMAGE == true) {
        if (pfLEDsin->SAVEFORMAT == "fits")
            theImage.saveFITSImage(pfLEDsin,strTime);
        if (pfLEDsin->SAVEFORMAT == "raw")
            theImage.saveRawImage(strTime);
    }

//
//end taking/saving picture

//Do Image analysis if not requested to only capture image
    if (pfLEDsin->CAPTUREONLY)
        return false;

    //create our basic output struct
    strcpy(pLEDsout->TIME,strTime);
    vector<ImageStar> theCentroids;
    // find the stars
    StarDetect theDetector = StarDetect(theImage, theCentroids, pLEDsout);
    // save filtered image
    if(pfLEDsin->SAVEIMAGE == true)
        theImage.savefilteredFITSImage(pLEDsout,strTime);
    //print the current LED positions
    cout << "TIME: " << strTime << endl;
    for(size_t i =0; i< theCentroids.size(); i++) {
        cout << theCentroids[i].pX() << ", " << theCentroids[i].pY() << endl;
    }
    
    // attempt solid body fit
    FitLED fittheleds(pLEDsout);
    
    return true;
    //End image analysis
}
