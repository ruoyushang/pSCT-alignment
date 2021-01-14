#ifndef _RANGEFINDERCLASS_HPP_
#define _RANGEFINDERCLASS_HPP_
#include "common/globalalignment/rangefinder/dls.hpp"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <fcntl.h> // For file handling
#include <getopt.h>  // Argument parsing

#include "common/alignment/device.hpp"

//duplicate dls.cpp methods in this class.
class GASRangeFinder : public Device {
public:
    explicit GASRangeFinder(Device::Identity identity) : Device::Device(std::move(identity)) {}
    Device::ErrorDefinition getErrorCodeDefinition(int errorCode) override;
    int getNumErrors() override;
    bool initialize() override;
    void turnOff() override;
    void turnOn() override;
    bool isOn() override;

private:
};

class DummyGASRangeFinder : public GASRangeFinder{
public:
    explicit DummyGASRangeFinder(Device::Identity identity) : GASRangeFinder(std::move(identity)) {} // nothing to do -- everything is set in initialize()
    bool initialize() override;

};

#endif