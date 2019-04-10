#pragma once

// prebuilt Camera interface
#include "common/alignment/ccd/AravisCamera.h"

// image analysis
#include "common/alignment/ccd/StarDetect.h"

// position analysis
#include "common/alignment/ccd/FitLED.h"

// data storage
#include "common/alignment/ccd/LEDinputs.h"
#include "common/alignment/ccd/LEDoutputs.h"
#include <time.h>

// needed for waiting
#include <chrono>
#include <thread> 

class CamOutThread{
        public:
            CamOutThread() : pfCamera(nullptr), imgWidth(0), imgHeight(0), pfLEDsin(nullptr) {}
            CamOutThread(AravisCamera *camera, const LEDinputs* li);
            ~CamOutThread();

            bool cycle(LEDoutputs *pLEDsout);
	
	private:
                AravisCamera *pfCamera;
		int imgWidth, imgHeight;
		const LEDinputs *pfLEDsin;
		const double PI  = 3.141592653589793238463;
	
};
