//
// Created by Deivid Ribeiro on 4/23/21.
//

#ifndef ALIGNMENT_AUTOCOLLCLASS_HPP
#define ALIGNMENT_AUTOCOLLCLASS_HPP

// prebuilt Camera interface
#include "common/globalalignment/ccd/AravisCamera.h"
// the work horse class designed to be a single thread
#include "common/globalalignment/AutoCollimator/ACCamOutThread.hpp"
// the i/o structure (should mirror the database and inputs only written from the start)
#include "common/globalalignment/AutoCollimator/ACCamInputs.hpp"

#include "common/alignment/device.hpp"

#include <memory>
#include <string>

class GASAC: public Device {
public:
    struct Position {
        Position() : xCentroid(-1), yCentroid(-1), xSpotWidth(-1), ySpotWidth(-1), cleanedIntensity(0), xNominal(-1),
                     yNominal(-1), exposure(-1), nSat(-1), timestamp(-1), last_img("") {}

        float xCentroid;
        float yCentroid;
        float xSpotWidth;
        float ySpotWidth;
        float cleanedIntensity;
        float xNominal;
        float yNominal;
        int exposure;
        int nSat;
        std::time_t timestamp;
        std::string last_img;
    };

    explicit GASAC(Device::Identity identity) :
            Device::Device(std::move(identity)),
            pfCamThread(nullptr),
            pfCamera(nullptr) {}

    ~GASAC() = default;

    static const std::vector<Device::ErrorDefinition> ERROR_DEFINITIONS;

    Device::ErrorDefinition getErrorCodeDefinition(int errorCode) override {
        return GASAC::ERROR_DEFINITIONS.at(errorCode);
    }

    int getNumErrors() override { return (int) GASAC::ERROR_DEFINITIONS.size(); }

    bool initialize() override;

    virtual void setConfig(string config);

    Position getPosition();

    virtual int getSerial() const { return std::stoi(pfCamera->getID(), nullptr, 10); };

    std::string getAddress() const { return fAddress; };

    virtual bool update();

    void turnOn() override;

    void turnOff() override;

    bool isOn() override;

    Device::ErrorState getErrorState() override;

    Position m_pPosition = Position(); //GASAC values
protected:
    ACCamInputs pfACin;
    std::unique_ptr<ACCamOutThread> pfCamThread;
    std::unique_ptr<AravisCamera> pfCamera;
    std::string fAddress;
    bool m_On{};
    std::string fConfigFile;

};

class DummyGASAC : public GASAC {
public:
    explicit DummyGASAC(Device::Identity identity) : GASAC(std::move(identity)) {}

    ~DummyGASAC() = default;

    int getNumErrors() override { return (int) GASAC::ERROR_DEFINITIONS.size(); }

    bool initialize() override;

    int getSerial() const override;

    bool update() override;

    void turnOn() override;

    void turnOff() override;

    bool isOn() override;

private:
};


#endif //ALIGNMENT_AUTOCOLLCLASS_HPP
