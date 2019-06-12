#ifndef _CCDCLASS_H_
#define _CCDCLASS_H_
// prebuilt Camera interface
#include "common/globalalignment/ccd/AravisCamera.h"
// the work horse class designed to be a single thread
#include "common/globalalignment/ccd/CamOutThread.h"
// the i/o structure (should mirror the database and inputs only written from the start)
#include "common/globalalignment/ccd/LEDinputs.h"
#include "common/globalalignment/ccd/LEDoutputs.h"

#include "common/alignment/device.hpp"

#include <memory>
#include <string>

class GASCCD
{
    public:
    GASCCD() :
        pfLEDsOut(nullptr),
        pfCamThread(nullptr),
        pfCamera(nullptr),
        fConfigFile("") {}

    ~GASCCD() = default;

    virtual bool initialize();

    virtual void setConfig(string config);

    virtual const double *getOutput() { return &(pfLEDsOut->SPACE[0]); };
        void setNominalValues(int offset, double value);

    virtual std::string getName() const { return fLEDsIn.CCDNAME; };

    virtual int getSerial() const { return std::strtol(pfCamera->getID().c_str(), nullptr, 10); };

    std::string getAddress() const { return fAddress; };

    virtual bool update();

protected:
    std::unique_ptr<LEDoutputs> pfLEDsOut;
        LEDinputs fLEDsIn;
    std::unique_ptr<CamOutThread> pfCamThread;
    std::unique_ptr<AravisCamera> pfCamera;
        std::string fAddress;
        std::string fConfigFile;
};

class DummyGASCCD : public GASCCD {
public:
    DummyGASCCD() : GASCCD(), m_Data() {}

    ~DummyGASCCD() = default;

    bool initialize() override;

    void setConfig(string config) override;

    const double *getOutput() override;

    std::string getName() const override;

    int getSerial() const override;

    bool update() override;

private:
    double m_Data[6];
};

#endif
