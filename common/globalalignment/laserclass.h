#ifndef _LASERCLASS_H_
#define _LASERCLASS_H_

#include "common/globalalignment/PSD/piusb.hpp"
#include "common/alignment/device.hpp"

class GASLaserBase : public Device {
public:
    explicit GASLaserBase(Device::Identity identity) : Device::Device(std::move(identity)) {}

    static const std::vector<Device::ErrorDefinition> ERROR_DEFINITIONS;

    virtual void setState(bool state);

    virtual void setPower(bool state);

    bool initialize() override = 0;

    bool isOn() override { return m_isOn; };

    bool isPowered() const { return m_isPowered; };

    Device::ErrorDefinition getErrorCodeDefinition(int errorCode) override {
        return GASLaserBase::ERROR_DEFINITIONS.at(errorCode);
    };

    int getNumErrors() override { return GASLaserBase::ERROR_DEFINITIONS.size(); };

    void turnOff() override;

    void turnOn() override;

protected:
    bool m_isOn{};
    bool m_isPowered{};
};

#ifndef SIMMODE
#include "common/cbccode/cbc.hpp"

class GASLaser : public GASLaserBase {
public:
    explicit GASLaser(std::shared_ptr<CBC> pCBC, Device::Identity identity) : GASLaserBase(std::move(identity)),
                                                                              m_pCBC(std::move(pCBC)) {}

    void setState(bool state) override;

    void setPower(bool state) override;

    bool initialize() override;

    void turnOff() override;

    void turnOn() override;

protected:
    std::shared_ptr<CBC> m_pCBC;
    std::shared_ptr<Relay> m_relay;
};

#endif

class DummyGASLaser : public GASLaserBase {
public:
    explicit DummyGASLaser(Device::Identity identity) : GASLaserBase(std::move(identity)) {}

    bool initialize() override;

};

#endif
