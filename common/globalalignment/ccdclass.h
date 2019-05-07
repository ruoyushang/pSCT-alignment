#ifndef _CCDCLASS_H_
#define _CCDCLASS_H_
// prebuilt Camera interface
#include "common/globalalignment/ccd/AravisCamera.h"
// the work horse class designed to be a single thread
#include "common/globalalignment/ccd/CamOutThread.h"
// the i/o structure (should mirror the database and inputs only written from the start)
#include "common/globalalignment/ccd/LEDinputs.h"
#include "common/globalalignment/ccd/LEDoutputs.h"

#include <string>

class GASCCD
{
    public:
        GASCCD();
        ~GASCCD();
        bool Initialize(bool isSim = false);
        void setConfig(string config);
        const double *getOutput() {return &(pfLEDsOut->SPACE[0]); };
        void setNominalValues(int offset, double value);
        const char *getName() const {return fLEDsIn.CCDNAME.c_str(); };
        int getSerial() const {return atoi(pfCamera->getID().c_str()); };
        const char *getAddress() const { return fAddress.c_str(); };
        bool Update();

    private:
        void simulate(bool hasCamera = true); // simulates camera
        LEDoutputs *pfLEDsOut;
        LEDinputs fLEDsIn;
        CamOutThread *pfCamThread;
        AravisCamera *pfCamera;
        std::string fAddress;
        std::string fConfigFile;
        bool fIsSim; // set if this is a simulation;
};

#endif
