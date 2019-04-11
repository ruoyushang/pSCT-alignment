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

#include "common/mpescode/MPESImageSet.h"
#include "common/mpescode/MPESDevice.h"

class CBC;

class MPES
{
public:
    struct Position {
        float xCenter;
        float yCenter;
        float xStdDev;
        float yStdDev;
        float cleanedIntensity;
        float xNominal;
        float yNominal;
    };

    MPES(std::shared_ptr<CBC> pCBC, int USBPortNumber, int serialNumber);
    ~MPES();

    int getPortNumber() const { return m_USBPortNumber; };
    void setPortNumber(int USBPortNumber);

    virtual bool initialize();
    virtual int setExposure();

    void setxNominalPosition(float x) { m_Position.xNominal = x; }
    void setyNominalPosition(float y) { m_Position.yNominal = y; }

    virtual int updatePosition();
    MPES::Position getPosition() const { return m_Position; };

protected:
    std::shared_ptr<CBC> m_pCBC;

    int m_USBPortNumber;
    int m_SerialNumber;

    bool m_Calibrate;

    Position m_Position; // MPES Reading

    // helpers
    std::shared_ptr<MPESImageSet> m_pImageSet;
    std::unique_ptr<MPESDevice> m_pDevice;

    std::set<int> getVideoDevices();

    // Hardcoded constants
    static const int DEFAULT_IMAGES_TO_CAPTURE;
    static const std::string MATRIX_CONSTANTS_DIR_PATH;
    static const std::string CAL2D_CONSTANTS_DIR_PATH;

    static const float NOMINAL_INTENSITY;
    static const float NOMINAL_CENTROID_SD;

    static const float SAFETY_REGION_X_MIN;
    static const float SAFETY_REGION_X_MAX;
    static const float SAFETY_REGION_Y_MIN;
    static const float SAFETY_REGION_Y_MAX;
};

class DummyMPES : public MPES
{
public:
    DummyMPES(std::shared_ptr<CBC> pCBC, int portNumber, int serialNumber) : MPES(std::shared_ptr<CBC>

    pCBC,
    int portNumber,
    int serialNumber
    ) {};

    bool initialize() override;

    int setExposure() override;

    int updatePosition() override;
};

#endif //ALIGNMENT_MPES_HPP
