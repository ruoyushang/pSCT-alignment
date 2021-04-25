#pragma once

#include <vector>
#include <CCfits/PHDU.h>
#include <CCfits/FITSUtilT.h>
#include <CCfits/PHDUT.h>
#include <CCfits/FITS.h>
#include <CCfits/CCfits.h>
#include <time.h>
#include "common/globalalignment/ccd/LEDinputs.h"
#include "common/globalalignment/ccd/LEDoutputs.h"

//spdlog
#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

class Image {
public:
    // Constructor & Destructor
    Image() : pixel_array(nullptr), imgWidth(0), imgHeight(0) {}

    Image(unsigned char *);

    Image(std::vector<unsigned char>);

    void saveRawImage(const char strTime[16]);

    void saveFITSImage();

    void saveFITSImage(const LEDinputs *li, const char strTime[16]);

    void savefilteredFITSImage();

    void savefilteredFITSImage(const LEDoutputs *lo, const char strTime[16]);

    ~Image();

    Image(const Image& that);

    Image &operator=(const Image& that);

    // Accessor methods
    int width() { return imgWidth; }

    int height() { return imgHeight; }

    unsigned char *pixels() { return pixel_array; }

    void setImageDir(std::string dir) { fImageDir = dir; }

private:
    unsigned char *pixel_array;
    int imgWidth, imgHeight;
    std::string fImageDir = "/home/ctauser/Pictures/GAS_CCD/";
};
