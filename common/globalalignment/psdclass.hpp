#ifndef _PSDCLASS_HPP_
#define _PSDCLASS_HPP_

#include <fstream>
#include <string>
#include <set>
#include <dirent.h>
#include <iostream>
#include <fcntl.h>
#include <sstream>

#include "common/alignment/device.hpp"


class GASPSDBase : public Device
{
public:
    explicit GASPSDBase(Device::Identity identity) : Device::Device(std::move(identity)),
                                                 m_data() {} // nothing to do -- everything is set in initialize()
    ~GASPSDBase() = default;

    static const std::vector<Device::ErrorDefinition> ERROR_DEFINITIONS;

    bool initialize() override = 0;

    double getOutput(int offset) { return m_data[offset]; }

    virtual void setNominalValues(int offset, double value);

    std::string getName() const { return m_Identity.name; }

    int getSerial() const { return m_Identity.serialNumber; }

    int getPort() const { return std::stoi(m_Identity.eAddress); }

    virtual void update() = 0;

    void turnOn() override;
    void turnOff() override;
    int getNumErrors() override { return GASPSDBase::ERROR_DEFINITIONS.size(); };
    Device::ErrorDefinition getErrorCodeDefinition(int errorCode) override;
    bool isOn() override;


protected:
    virtual int setInterfaceAttribs(int fd, int speed, int parity) = 0;

    virtual void setBlocking(int fd, int should_block) = 0;

    virtual void setCalibration();

    // calibration constants
    double m_AlphaNeg[4] = {-9.710132, 9.871400, -9.604298, 10.071177}; // {x1Neg, y1Neg, x2Neg, y2Neg}
    double m_AlphaPos[4] = {-9.618614, 9.658531, -9.382091, 9.934893}; // {x1Pos, y1Pos, x2Pos, y2Pos}
    double m_Beta[4] = {0.011341, 0.118089, 0.023689, 0.116461}; // {{x1, y1, x2, y2}
    double m_Theta[2] = {0.0, 0.0}; // {x, y} ? seemingly unused

    std::string m_logFilename = "PSD-measurements.log"; // file to log into
    std::ofstream m_logOutputStream;

    double m_data[9]; // x1, y1, x2, y2, dx1, dy1, dx2, dy2, temperature
    bool m_On = false;
};

#ifndef SIMMODE
#include "common/cbccode/cbc.hpp"

class GASPSD : public GASPSDBase
{
public:
    explicit GASPSD(std::shared_ptr<CBC> pCBC, Device::Identity identity) : GASPSDBase(std::move(identity)), m_pCBC(std::move(pCBC)), m_fd(0) {} // nothing to do -- everything is set in initialize()
    ~GASPSD();

    bool initialize() override;

    void setNominalValues(int offset, double value) override;

    void update() override;

    void turnOn() override;
    void turnOff() override;

protected:
    int setInterfaceAttribs(int fd, int speed, int parity) override;

    void setBlocking(int fd, int should_block) override;

    void setCalibration() override;

    static std::set<int> getACMDevices();

    std::shared_ptr<CBC> m_pCBC;

    int m_usb_port = -1;

    int m_fd; // file descriptor to access the serial device
    std::string m_logFilename = std::string(getenv("HOME")) + std::string("/logs/") + "PSD-measurements.log"; // file to log into
    std::ofstream m_logOutputStream;

};
#endif

class DummyGASPSD : public GASPSDBase {
public:
    explicit DummyGASPSD(Device::Identity identity) : GASPSDBase(std::move(identity)) {} // nothing to do -- everything is set in initialize()
    bool initialize() override;

    void update() override;

private:
    int setInterfaceAttribs(int fd, int speed, int parity) override;

    void setBlocking(int fd, int should_block) override;

};

#endif
