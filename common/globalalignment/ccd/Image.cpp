#include "common/globalalignment/ccd/Image.h"
#include <fstream>
#include <memory>

using namespace std;

// Constructor
Image::Image(unsigned char *frame) : imgWidth(2592), imgHeight(1944) {
    pixel_array = new unsigned char[imgWidth * imgHeight];
    for (int i = 0; i < imgWidth * imgHeight; i++)
        pixel_array[i] = frame[i];
}

Image::Image(std::vector<unsigned char> frame) : imgWidth(2592), imgHeight(1944) {
    pixel_array = new unsigned char[imgWidth * imgHeight];
    for (int i = 0; i < imgWidth * imgHeight; i++)
        pixel_array[i] = frame[i];
}

void Image::saveRawImage(const char strTime[16]) {
    string fileName = strTime;
    fileName += ".raw";

    // just dump the pixel array into file
    ofstream rawdump(fileName.c_str(), ofstream::out);
    for (int i = 0; i < imgWidth * imgHeight; i++)
        rawdump << pixel_array[i];

    rawdump.close();

}

void Image::saveFITSImage() {

    time_t t = time(NULL);
    struct tm *theTime = gmtime(&t);
    char strTime[16];
    strftime(strTime, 16, "%Y%m%d%H%M%S", theTime);
    string fileName = strTime;
    fileName += ".fits";
    //save image for diagnostics
    // use auto-pointer for automatic garbage collection
    unique_ptr<CCfits::FITS> pFits(nullptr);

    // attempt to create a FITS file
    try {
        long naxes[2] = {imgWidth, imgHeight};
        pFits.reset(new CCfits::FITS(fileName, BYTE_IMG, 2, naxes));
    } catch (CCfits::FITS::CantCreate) {
        spdlog::warn("Error saving FITS image! A FITS file could not be opened for writing.");
        return;
    }

    valarray<unsigned char> *rawImage;
    rawImage = new valarray<unsigned char>(imgWidth * imgHeight);
    //pFits->pHDU().addKey("OBSDATE", strTime, "date and time when image was taken (UTC)");
    for (int i = 0; i < imgWidth * imgHeight; i++) {
        (*rawImage)[i] = pixel_array[i];
    }


    pFits->pHDU().addKey("INFORMATION", 0, "No information provided");
    pFits->pHDU().write(1, imgWidth * imgHeight, *rawImage);

    delete rawImage;

}

/*
void Image::saveFITSImage(const LEDinputs* li) {

    time_t t = time(NULL);
    struct tm* theTime = gmtime(&t);
    char strTime[16];
    strftime(strTime,16,"%Y%m%d%H%M%S",theTime);
    string fileName = strTime;
    fileName+="raw.fits";
    //save image for diagnostics
    // use auto-pointer for automatic garbage collection
       auto_ptr<CCfits::FITS> pFits(0);

       // attempt to create a FITS file
       try {
           long naxes[2] = {imgWidth, imgHeight};
           pFits.reset(new CCfits::FITS(fileName, BYTE_IMG, 2, naxes));
       } catch (CCfits::FITS::CantCreate) {
           cout << "Error saving FITS image! A FITS file could not be opened for writing." << endl;
           return;
       }

    valarray<unsigned char>* rawImage;
    rawImage = new valarray<unsigned char>(imgWidth*imgHeight);
    //pFits->pHDU().addKey("OBSDATE", strTime, "date and time when image was taken (UTC)");
    for(int i = 0; i < imgWidth*imgHeight; i++){
        (*rawImage)[i] = pixel_array[i];
    }


    pFits->pHDU().addKey("CCDNAME", li->CCDNAME, "The common name of the CCD");
    pFits->pHDU().addKey("CCDEXP", li->CCDEXP, "CCD Exposure (us)");
    pFits->pHDU().addKey("CCDGAIN", li->CCDGAIN, "CCD Gain");
    pFits->pHDU().addKey("PIXSIZE", li->PIXSIZE, "Pixel size of the CCD (m)");
    pFits->pHDU().addKey("CCDSN", li->CCDSN, "Serial Number of the CCD");
    pFits->pHDU().addKey("LENSFL", li->LENSFL, "Uncorrected Focal Length (m)");
    pFits->pHDU().addKey("LENSSCALE", li->LENSSCALE, "Correction to focal length");
    pFits->pHDU().addKey("LENSSN", li->LENSSN, "Serial Number of the Lens used");
    pFits->pHDU().addKey("NLED", li->NLED, "Number of LEDs expected");
    pFits->pHDU().addKey("ANALYSIS", 0, "Was the analysis completed?");
    pFits->pHDU().write(1, imgWidth*imgHeight, *rawImage);

}
*/
void Image::saveFITSImage(const LEDinputs *li, const char strTime[16]) {

    string fileName = strTime;
    fileName += "raw.fits";
    //save image for diagnostics
    // use auto-pointer for automatic garbage collection
    unique_ptr<CCfits::FITS> pFits(nullptr);

    // attempt to create a FITS file
    try {
        long naxes[2] = {imgWidth, imgHeight};
        pFits.reset(new CCfits::FITS(fileName, BYTE_IMG, 2, naxes));
    } catch (CCfits::FITS::CantCreate) {
        spdlog::warn("Error saving FITS image! A FITS file could not be opened for writing.");
        return;
    }

    valarray<unsigned char> *rawImage;
    rawImage = new valarray<unsigned char>(imgWidth * imgHeight);
    //pFits->pHDU().addKey("OBSDATE", strTime, "date and time when image was taken (UTC)");
    for (int i = 0; i < imgWidth * imgHeight; i++) {
        (*rawImage)[i] = pixel_array[i];
    }

    pFits->pHDU().addKey("DATETIME", std::string(strTime), "YYYYMMDDHHMMSS(.)ss");
    pFits->pHDU().addKey("CCDNAME", li->CCDNAME, "The common name of the CCD");
    pFits->pHDU().addKey("CCDEXP", li->CCDEXP, "CCD Exposure (us)");
    pFits->pHDU().addKey("CCDGAIN", li->CCDGAIN, "CCD Gain");
    pFits->pHDU().addKey("PIXSIZE", li->PIXSIZE, "Pixel size of the CCD (m)");
    pFits->pHDU().addKey("CCDSN", li->CCDSN, "Serial Number of the CCD");
    pFits->pHDU().addKey("LENSFL", li->LENSFL, "Uncorrected Focal Length (m)");
    pFits->pHDU().addKey("LENSSCALE", li->LENSSCALE, "Correction to focal length");
    pFits->pHDU().addKey("LENSSN", li->LENSSN, "Serial Number of the Lens used");
    pFits->pHDU().addKey("NLED", li->NLED, "Number of LEDs expected");
    pFits->pHDU().addKey("ANALYSIS", "0", "Was the analysis completed?");
    pFits->pHDU().write(1, imgWidth * imgHeight, *rawImage);

    delete rawImage;
}

void Image::savefilteredFITSImage() {

    time_t t = time(NULL);
    struct tm *theTime = gmtime(&t);
    char strTime[16];
    strftime(strTime, 16, "%Y%m%d%H%M%S", theTime);
    string fileName = strTime;
    fileName += "filtered.fits";
    //save image for diagnostics
    // use auto-pointer for automatic garbage collection
    unique_ptr<CCfits::FITS> pFits(nullptr);

    // attempt to create a FITS file
    try {
        long naxes[2] = {imgWidth, imgHeight};
        pFits.reset(new CCfits::FITS(fileName, BYTE_IMG, 2, naxes));
    } catch (CCfits::FITS::CantCreate) {
        spdlog::warn("Error saving FITS image! A FITS file could not be opened for writing.");
        return;
    }

    valarray<unsigned char> *rawImage;
    rawImage = new valarray<unsigned char>(imgWidth * imgHeight);
    //pFits->pHDU().addKey("OBSDATE", strTime, "date and time when image was taken (UTC)");
    for (int i = 0; i < imgWidth * imgHeight; i++) {
        (*rawImage)[i] = pixel_array[i];
    }

    pFits->pHDU().addKey("INFORMATION", 0, "No information provided");
    pFits->pHDU().write(1, imgWidth * imgHeight, *rawImage);

    delete rawImage;
}

void Image::savefilteredFITSImage(const LEDoutputs *lo, const char strTime[16]) {

    string fileName = strTime;
    fileName += "filtered.fits";
    //save image for diagnostics
    // use auto-pointer for automatic garbage collection
    unique_ptr<CCfits::FITS> pFits(nullptr);

    // attempt to create a FITS file
    try {
        long naxes[2] = {imgWidth, imgHeight};
        pFits.reset(new CCfits::FITS(fileName, BYTE_IMG, 2, naxes));
    } catch (CCfits::FITS::CantCreate) {
        spdlog::warn("Error saving FITS image! A FITS file could not be opened for writing.");
        return;
    }

    valarray<unsigned char> *rawImage;
    rawImage = new valarray<unsigned char>(imgWidth * imgHeight);
    //pFits->pHDU().addKey("OBSDATE", strTime, "date and time when image was taken (UTC)");
    for (int i = 0; i < imgWidth * imgHeight; i++) {
        (*rawImage)[i] = pixel_array[i];
    }


    pFits->pHDU().addKey("DATETIME", std::string(strTime), "YYYYMMDDHHMMSS(.)ss");
    pFits->pHDU().addKey("CCDNAME", lo->inleds->CCDNAME, "The common name of the CCD");
    pFits->pHDU().addKey("CCDEXP", lo->inleds->CCDEXP, "CCD Exposure (us)");
    pFits->pHDU().addKey("CCDGAIN", lo->inleds->CCDGAIN, "CCD Gain");
    pFits->pHDU().addKey("PIXSIZE", lo->inleds->PIXSIZE, "Pixel size of the CCD (m)");
    pFits->pHDU().addKey("CCDSN", lo->inleds->CCDSN, "Serial Number of the CCD");
    pFits->pHDU().addKey("LENSFL", lo->inleds->LENSFL, "Uncorrected Focal Length (m)");
    pFits->pHDU().addKey("LENSSCALE", lo->inleds->LENSSCALE, "Correction to focal length");
    pFits->pHDU().addKey("LENSSN", lo->inleds->LENSSN, "Serial Number of the Lens used");
    pFits->pHDU().addKey("NLED", lo->inleds->NLED, "Number of LEDs expected");
    pFits->pHDU().addKey("ANALYSIS", 1, "What stage of the analysis was completed?");
    pFits->pHDU().addKey("LEDSCOUNT", lo->LEDSCOUNT, "How many LEDs were found");
    for (int i = 0; i < lo->inleds->NLED; i++) {
        if (lo->LEDSPRESENT[i]) {
            pFits->pHDU().addKey("LEDPOS" + to_string(i) + "X", lo->LEDPOS[i][0],
                                 "X Pixel location of LED " + to_string(i));
            pFits->pHDU().addKey("LEDPOS" + to_string(i) + "Y", lo->LEDPOS[i][1],
                                 "Y Pixel location of LED " + to_string(i));
        }
    }
    pFits->pHDU().addKey("POS0", lo->SPACE[0], "X translation of panel");
    pFits->pHDU().addKey("POS1", lo->SPACE[1], "Y translation of panel");
    pFits->pHDU().addKey("POS2", lo->SPACE[2], "Z translation of panel");
    pFits->pHDU().addKey("ROT0", lo->SPACE[3], "X-axis rotation of panel");
    pFits->pHDU().addKey("ROT1", lo->SPACE[4], "Y-axis rotation of panel");
    pFits->pHDU().addKey("ROT2", lo->SPACE[5], "Z-axis rotation of panel");
    pFits->pHDU().write(1, imgWidth * imgHeight, *rawImage);

    delete rawImage;
}

Image::~Image() {
    delete[] pixel_array;
    pixel_array = nullptr;
}
