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

class CBC;
class Platform;
class MPESImageSet;
class MPESDevice;


class MPES : public Device
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

    std::vector<Device::ErrorDefinition> getErrorCodeDefinitions() override { return MPES::ERROR_DEFINITIONS; }

    MPES(std::shared_ptr<CBC> pCBC, Device::Identity identity);
    ~MPES();

    int getPortNumber() const { return std::stoi(m_Identity.eAddress); };

    int getSerialNumber() const { return m_Identity.serialNumber; };

    bool initialize() override;
    virtual int setExposure();

    void setxNominalPosition(float x) { m_Position.xNominal = x; }
    void setyNominalPosition(float y) { m_Position.yNominal = y; }

    virtual int updatePosition();
    MPES::Position getPosition() const { return m_Position; };

    // Hardcoded constants
    static const int DEFAULT_IMAGES_TO_CAPTURE;
    static const std::string MATRIX_CONSTANTS_DIR_PATH;
    static const std::string CAL2D_CONSTANTS_DIR_PATH;

    static const int NOMINAL_INTENSITY;
    static const float NOMINAL_CENTROID_SD;

    static const float SAFETY_REGION_X_MIN;
    static const float SAFETY_REGION_X_MAX;
    static const float SAFETY_REGION_Y_MIN;
    static const float SAFETY_REGION_Y_MAX;

protected:
    bool m_Calibrate;

    Position m_Position = Position(); // MPES Reading

    // helpers
    std::shared_ptr<MPESImageSet> m_pImageSet;
    std::unique_ptr<MPESDevice> m_pDevice;

    static std::set<int> getVideoDevices();
};

class DummyMPES : public MPES
{
public:
    DummyMPES(std::shared_ptr<CBC> pCBC, Device::Identity identity) : MPES(std::move(pCBC), std::move(identity)) {};

    bool initialize() override;
    int setExposure() override;
    int updatePosition() override;
};

#endif //ALIGNMENT_MPES_HPP
