#pragma once

// prebuilt Camera interface
#include "common/globalalignment/ccd/AravisCamera.h"

// image analysis
#include "common/globalalignment/ccd/StarDetect.h"

// position analysis
#include "common/globalalignment/ccd/FitLED.h"

// data storage
#include "common/globalalignment/ccd/LEDinputs.h"
#include "common/globalalignment/ccd/LEDoutputs.h"
#include <time.h>

// needed for waiting
#include <chrono>
#include <thread>

//spdlog
#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

class CamOutThread {
public:
    CamOutThread() : pfCamera(nullptr), imgWidth(0), imgHeight(0), pfLEDsin(nullptr) {}

    CamOutThread(AravisCamera *camera, const LEDinputs *li);

    ~CamOutThread();

    bool cycle(LEDoutputs *pLEDsout);

private:
    AravisCamera *pfCamera;
    int imgWidth, imgHeight;
    const LEDinputs *pfLEDsin;
    const double PI = 3.141592653589793238463;

};
