#ifndef _PSDCLASS_H_
#define _PSDCLASS_H_

#include <fstream>
#include <string>

#include "common/alignment/device.hpp"

class GASPSD : public virtual Device
{
public:
    explicit GASPSD(Device::Identity identity) : Device::Device(std::move(identity)), m_Identity(std::move(identity)), m_fd(0),
                                                 m_data() {} // nothing to do -- everything is set in initialize()
    ~GASPSD();

    static const std::vector<Device::ErrorDefinition> ERROR_DEFINITIONS;

    virtual bool initialize();

    double getOutput(int offset) { return m_data[offset]; }
    void setNominalValues(int offset, double value);

    std::string getName() const { return m_Identity.name; }

    int getSerial() const { return m_Identity.serialNumber; }

    std::string getPort() const { return m_Identity.eAddress; }

    virtual void update();

    void turnOn() override;
    void turnOff() override;
    int getNumErrors() override { return GASPSD::ERROR_DEFINITIONS.size(); };
    Device::ErrorDefinition getErrorCodeDefinition(int errorCode) override;
    bool isOn() override;


protected:
    Device::Identity m_Identity;

    virtual int setInterfaceAttribs(int fd, int spede, int parity);

    virtual void setBlocking(int fd, int should_block);

    virtual void setCalibration();

    // calibration constants
    double m_AlphaNeg[4] = {-9.710132, 9.871400, -9.604298, 10.071177}; // {x1Neg, y1Neg, x2Neg, y2Neg}
    double m_AlphaPos[4] = {-9.618614, 9.658531, -9.382091, 9.934893}; // {x1Pos, y1Pos, x2Pos, y2Pos}
    double m_Beta[4] = {0.011341, 0.118089, 0.023689, 0.116461}; // {{x1, y1, x2, y2}
    double m_Theta[2] = {0.0, 0.0}; // {x, y} ? seemingly unused

    int m_fd; // file descriptor to access the serial device
    std::string m_logFilename = "PSD-Cam.log"; // file to log into
    std::ofstream m_logOutputStream;

    double m_data[9]; // x1, y1, x2, y2, dx1, dy1, dx2, dy2, temperature
    bool m_On = false;
};

class DummyGASPSD : public virtual GASPSD {
public:
    explicit DummyGASPSD(Device::Identity identity) : Device(std::move(identity)),
                                                      GASPSD(std::move(identity)) {} // nothing to do -- everything is set in initialize()
    bool initialize() override;

    void update() override;

private:
    int setInterfaceAttribs(int fd, int spede, int parity) override;

    void setBlocking(int fd, int should_block) override;

};

#endif
