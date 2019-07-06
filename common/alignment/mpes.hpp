/**
 * @file mpes.hpp
 * @brief Header file for the MPES hardware control interface class.
 */

#ifndef ALIGNMENT_MPES_HPP
#define ALIGNMENT_MPES_HPP

#include <cstring>
#include <fstream>
#include <memory>
#include <set>
#include <string>

#include "common/alignment/device.hpp"

class Platform;

class MPESBase : public Device
{
public:
    struct Position {
        Position() : xCentroid(-1), yCentroid(-1), xSpotWidth(-1), ySpotWidth(-1), cleanedIntensity(0), xNominal(-1),
                     yNominal(-1) {}

        float xCentroid;
        float yCentroid;
        float xSpotWidth;
        float ySpotWidth;
        float cleanedIntensity;
        float xNominal;
        float yNominal;
    };

    static const std::vector<Device::ErrorDefinition> ERROR_DEFINITIONS;

    Device::ErrorDefinition getErrorCodeDefinition(int errorCode) override {
        return MPESBase::ERROR_DEFINITIONS.at(errorCode);
    }

    explicit MPESBase(Device::Identity identity) : Device::Device(std::move(identity)),
                                                   m_Calibrate(false) {}

    virtual ~MPESBase() = default;

    int getPortNumber() const { return std::stoi(m_Identity.eAddress); };
    int getSerialNumber() const { return m_Identity.serialNumber; };

    bool initialize() override;

    int setExposure();

    void setxNominalPosition(float x) { m_Position.xNominal = x; }
    void setyNominalPosition(float y) { m_Position.yNominal = y; }

    int updatePosition();

    MPESBase::Position getPosition() const { return m_Position; };

    // Hardcoded constants
    static const int DEFAULT_IMAGES_TO_CAPTURE;
    static const std::string MATRIX_CONSTANTS_DIR_PATH;
    static const std::string CAL2D_CONSTANTS_DIR_PATH;


    static constexpr int NOMINAL_INTENSITY = 150000;
    static constexpr float NOMINAL_SPOT_WIDTH = 10.0;
    static constexpr int MAX_READ_ATTEMPTS = 1;

    void turnOn();

    virtual void turnOff() override = 0;
    virtual bool isOn() override = 0;

protected:
    bool m_Calibrate;

    Position m_Position = Position(); // MPES Reading

    virtual bool __initialize() = 0;

    virtual int __updatePosition() = 0;

    virtual int __setExposure() = 0;
};

#ifndef SIMMODE

#include "common/cbccode/cbc.hpp"
#include "common/mpescode/MPESDevice.h"
#include "common/mpescode/MPESImageSet.h"

class MPES : public MPESBase {
public:
    explicit MPES(std::shared_ptr<CBC> pCBC, Device::Identity identity) : MPESBase::MPESBase(std::move(identity)),
                                                                          m_pCBC(std::move(pCBC)), m_pImageSet(nullptr),
                                                                          m_pDevice(nullptr) {}

    ~MPES() override { turnOff(); };

    void turnOff() override;

    bool isOn() override;

protected:
    std::shared_ptr<CBC> m_pCBC;

    bool __initialize() override;

    int __updatePosition() override;

    int __setExposure() override;

    // helpers
    std::shared_ptr<MPESImageSet> m_pImageSet;
    std::unique_ptr<MPESDevice> m_pDevice;

    static std::set<int> getVideoDevices();
};

#endif

class DummyMPES : public MPESBase
{
public:
    explicit DummyMPES(Device::Identity identity) : MPESBase(std::move(identity)) {};

    ~DummyMPES() override = default;

    bool __initialize() override;
    int __setExposure() override;
    int __updatePosition() override;

    void turnOff() override;

    bool isOn() override;

private:
    bool m_On = false;
};

#endif //ALIGNMENT_MPES_HPP
