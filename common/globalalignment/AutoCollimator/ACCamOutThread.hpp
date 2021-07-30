#pragma once

// prebuilt Camera interface
#include "common/globalalignment/ccd/AravisCamera.h"

// image analysis
#include "common/globalalignment/ccd/StarDetect.h"

// data storage
#include "common/globalalignment/AutoCollimator/ACCamInputs.hpp"
#include <time.h>

// needed for waiting
#include <chrono>
#include <thread>

//spdlog
#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

class ACCamOutThread {
public:
    ACCamOutThread() : pfCamera(nullptr), imgWidth(0), imgHeight(0), pfACin(nullptr) {}

    ACCamOutThread(AravisCamera *camera, const ACCamInputs *ACin);

    ~ACCamOutThread();

    bool cycle();

private:
    AravisCamera *pfCamera;
    int imgWidth, imgHeight;
    const ACCamInputs *pfACin;
    const double PI = 3.141592653589793238463;

};
