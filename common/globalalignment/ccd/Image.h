#pragma once
#include <vector>
#include <CCfits/PHDU.h>
#include <CCfits/FITSUtilT.h>
#include <CCfits/PHDUT.h>
#include <CCfits/FITS.h>
#include <CCfits/CCfits.h>
#include <time.h>
#include "common/alignment/ccd/LEDinputs.h"
#include "common/alignment/ccd/LEDoutputs.h"

class Image {
public:
    // Constructor & Destructor
    Image() : pixel_array(nullptr), imgWidth(0), imgHeight(0) {}
    Image(unsigned char*);
    Image(std::vector<unsigned char>);
    void saveRawImage(const char strTime[16]);
    void saveFITSImage();
    void saveFITSImage(const LEDinputs* li, const char strTime[16]);
    void savefilteredFITSImage();
    void savefilteredFITSImage(const LEDoutputs* lo, const char strTime[16]);
    ~Image();

    // Accessor methods
    int width() { return imgWidth; }
    int height() { return imgHeight; }
    unsigned char* pixels() { return pixel_array; }
private:
    unsigned char* pixel_array;
    int imgWidth, imgHeight;
};
