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

class GASCCD : public Device {
public:
    GASCCD(Device::Identity identity) :
            pfLEDsOut(nullptr),
            pfCamThread(nullptr),
            pfCamera(nullptr),
            fConfigFile(""),
            Device::Device(std::move(identity)) {}

    ~GASCCD() = default;

    static const std::vector<Device::ErrorDefinition> ERROR_DEFINITIONS;

    Device::ErrorDefinition getErrorCodeDefinition(int errorCode) {
        return GASCCD::ERROR_DEFINITIONS.at(errorCode);
    }

    int getNumErrors() override { return GASCCD::ERROR_DEFINITIONS.size(); }

    virtual bool initialize();

    virtual void setConfig(string config);

    virtual const double *getOutput() { return &(pfLEDsOut->SPACE[0]); };

    void setNominalValues(int offset, double value);

    virtual std::string getName() const { return fLEDsIn.CCDNAME; };

    virtual int getSerial() const { return std::strtol(pfCamera->getID().c_str(), nullptr, 10); };

    std::string getAddress() const { return fAddress; };

    virtual bool update();

    void turnOn() override;

    void turnOff() override;

    bool isOn() override;

    Device::ErrorState getErrorState() override;

protected:
    std::unique_ptr<LEDoutputs> pfLEDsOut;
    LEDinputs fLEDsIn;
    std::unique_ptr<CamOutThread> pfCamThread;
    std::unique_ptr<AravisCamera> pfCamera;
    std::string fAddress;
    std::string fConfigFile;
    bool m_On{};

};

class DummyGASCCD : public GASCCD {
public:
    DummyGASCCD(Device::Identity identity) : GASCCD(std::move(identity)), m_Data() {}

    ~DummyGASCCD() = default;

    int getNumErrors() override { return GASCCD::ERROR_DEFINITIONS.size(); }

    bool initialize() override;

    void setConfig(string config) override;

    const double *getOutput() override;

    std::string getName() const override;

    int getSerial() const override;

    bool update() override;

    void turnOn() override;

    void turnOff() override;

    bool isOn() override;

private:
    double m_Data[6];
};

#endif
