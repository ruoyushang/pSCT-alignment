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
#include <math.h>

#include <mysql_connection.h>
#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include "common/alignment/device.hpp"
#include "common/mpescode/MPESDevice.h"

class Platform;

class MPESBase : public Device
{
public:
    struct Position {
        Position() : xCentroid(-3), yCentroid(-3), xCentroidErr(-3), yCentroidErr(-3), xSpotWidth(-3), ySpotWidth(-3), cleanedIntensity(-3), xNominal(-3),
                     yNominal(-3), exposure(-3), nSat(-3), timestamp(std::time(nullptr)), last_img("") {}

        float xCentroid;
        float yCentroid;
        float xCentroidErr;
	    float yCentroidErr;
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

    static const std::vector<Device::ErrorDefinition> ERROR_DEFINITIONS;

    Device::ErrorDefinition getErrorCodeDefinition(int errorCode) override {
        return MPESBase::ERROR_DEFINITIONS.at(errorCode);
    }

    int getNumErrors() override { return MPESBase::ERROR_DEFINITIONS.size(); }

    explicit MPESBase(Device::Identity identity, Device::DBInfo DBInfo = Device::DBInfo());

    virtual ~MPESBase() = default;

    int getPortNumber() const { return std::stoi(m_Identity.eAddress); };
    int getSerialNumber() const { return m_Identity.serialNumber; };
    std::string getLastImage() const { return m_Position.last_img; };

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
    static const std::string DEFAULT_IMAGES_SAVE_DIR_PATH;
    static const std::string BACKUP_IMAGE_FILEPATH;


    static constexpr float NOMINAL_INTENSITY = kNOMINAL_INTENSITY; // default was 150,000.
    static constexpr float NOMINAL_SPOT_WIDTH = kNOMINAL_SPOT_WIDTH;
    static constexpr int MAX_EXPOSURE = kMAX_EXPOSURE;
    static constexpr int MIN_EXPOSURE = kMIN_EXPOSURE;
    static constexpr float INTENSITY_RATIO_TOLERANCE = kINTENSITY_RATIO_TOLERANCE;
    static constexpr float PRECISION = kPRECISION;

    void turnOn();

    virtual void turnOff() override = 0;
    virtual bool isOn() override = 0;

    void saveMPESStatustoDB();
protected:
    bool m_Calibrate;

    Position m_Position = Position(); // MPES Reading

    virtual bool __initialize() = 0;

    virtual int __updatePosition() = 0;

    virtual int __setExposure() = 0;

    Device::DBInfo m_DBInfo;

    void setDBInfo(DBInfo DBInfo);
};

#ifndef SIMMODE

#include "common/cbccode/cbc.hpp"
#include "common/mpescode/MPESDevice.h"
#include "common/mpescode/MPESImageSet.h"

class MPES : public MPESBase {
public:
    explicit MPES(std::shared_ptr<CBC> pCBC, Device::Identity identity,
                  Device::DBInfo DBInfo = Device::DBInfo()):
                  MPESBase::MPESBase(std::move(identity), std::move(DBInfo)),
                  m_pCBC(std::move(pCBC)), m_pImageSet(nullptr), m_pDevice(nullptr) {}

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
    explicit DummyMPES(Device::Identity identity, Device::DBInfo DBInfo = Device::DBInfo()) :
        MPESBase(std::move(identity), std::move(DBInfo)) {};

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
