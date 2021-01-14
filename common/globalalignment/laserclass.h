#ifndef _LASERCLASS_H_
#define _LASERCLASS_H_

#include "common/globalalignment/PSD/piusb.hpp"
#include "common/alignment/device.hpp"

class GASLaser : public Device
{
    public:
        explicit GASLaser(Device::Identity identity) : Device::Device(std::move(identity)) {}
        void setState(bool state);
        void setPower(bool state);
        bool initialize() override;
        bool isOn() override {return m_isOn; };
        bool isPowered() {return m_isPowered; };
        Device::ErrorDefinition getErrorCodeDefinition(int errorCode) override;
        int getNumErrors() override;
        void turnOff() override;
        void turnOn() override;

    private:
        bool m_isOn{};
        bool m_isPowered{};
};

class DummyGASLaser : public GASLaser
{
public:
    explicit DummyGASLaser(Device::Identity identity) : GASLaser(std::move(identity)) {}
    bool initialize() override;

};
#endif
