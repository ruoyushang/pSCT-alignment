#ifndef _RANGEFINDERCLASS_HPP_
#define _RANGEFINDERCLASS_HPP_

#include "common/globalalignment/rangefinder/dls.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <fcntl.h> // For file handling
#include <getopt.h>  // Argument parsing

#include "common/alignment/device.hpp"

class GASRangeFinderBase : public Device {
public:
    explicit GASRangeFinderBase(Device::Identity identity) : Device::Device(std::move(identity)) {}

    static const std::vector<Device::ErrorDefinition> ERROR_DEFINITIONS;

    Device::ErrorDefinition getErrorCodeDefinition(int errorCode) override {
        return GASRangeFinderBase::ERROR_DEFINITIONS.at(errorCode);
    }

    int getNumErrors() override { return GASRangeFinderBase::ERROR_DEFINITIONS.size(); }

    bool initialize() override = 0;

    void turnOff() override;

    void turnOn() override;

    bool isOn() override { return m_isOn; };

private:
    bool m_isOn{};
};

#ifndef SIMMODE
#include "common/cbccode/cbc.hpp"

//duplicate dls.cpp methods in this class.
class GASRangeFinder : public GASRangeFinderBase {
public:
    explicit GASRangeFinder(std::shared_ptr<CBC> pCBC, Device::Identity identity) : GASRangeFinderBase(
            std::move(identity)), m_pCBC(std::move(pCBC)), m_usb_port(-1) {}

    bool initialize() override;

    void turnOff() override;

    void turnOn() override;

protected:
    std::shared_ptr<CBC> m_pCBC;

    int m_usb_port;

    static std::set<int> getUSBDevices();
};
#endif

class DummyGASRangeFinder : public GASRangeFinderBase {
public:
    explicit DummyGASRangeFinder(Device::Identity identity) : GASRangeFinderBase(
            std::move(identity)) {} // nothing to do -- everything is set in initialize()
    bool initialize() override;

};

#endif