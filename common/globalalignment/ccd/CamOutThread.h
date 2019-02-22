#pragma once

// prebuilt Camera interface
#include "AravisCamera.h"

// image analysis
#include "StarDetect.h"

// position analysis
#include "FitLED.h"

// data storage
#include "LEDinputs.h"
#include "LEDoutputs.h"
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
