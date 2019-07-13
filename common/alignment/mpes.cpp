#include "common/alignment/mpes.hpp"

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <unistd.h>

#include "common/alignment/platform.hpp"

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"


const int MPESBase::DEFAULT_IMAGES_TO_CAPTURE = 9;
const std::string MPESBase::MATRIX_CONSTANTS_DIR_PATH = "/home/root/mpesCalibration/";
const std::string MPESBase::CAL2D_CONSTANTS_DIR_PATH = "/home/root/mpesCalibration/";

const std::vector<Device::ErrorDefinition> MPESBase::ERROR_DEFINITIONS = {
    {"Bad connection. No device found",                                                                            Device::ErrorState::FatalError},//error 0
    {"Failed to set exposure, possible select timeout or high temperature.",                                          Device::ErrorState::OperableError},//error 1
    {"Cannot find laser spot (totally dark). Laser dead or not in FoV.",                                           Device::ErrorState::FatalError},//error 2
    {"Too bright. Cleaned Intensity > 1e6. Likely cause: no tube.",                                                Device::ErrorState::FatalError},//error 3
    {"Too bright. 1e6 >Cleaned Intensity > 5e5 and very wide spot width >20",                                      Device::ErrorState::OperableError},//error 4
    {"Very uneven spot. Likely due to being in the reflection region (too close to webcam edges) or a bad laser.", Device::ErrorState::OperableError},//error 5
    {"Uneven spot. Spot is uneven, but not severe. Likely recoverable.",                                           Device::ErrorState::OperableError},//error 6
    {"Intensity deviation from nominal value (more than 20%).",                                                    Device::ErrorState::OperableError},//error 7
};

bool MPESBase::initialize() {
    if (isBusy()) {
        spdlog::error("{} : MPES::initialize() : Busy, cannot initialize.", m_Identity);
        return false;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    bool status;
    status = __initialize();
    return status;
}

// find and set optimal exposure -- assume I(e) is linear
// returns measured intensity -- check this value to see if things work fine
int MPESBase::setExposure() {
    if (isBusy()) {
        spdlog::error("{} : MPES::setExposure() : Busy, cannot set exposure.", m_Identity);
        return -1;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    int intensity;
    intensity = __setExposure();
    return intensity;
}

// returns intensity of the beam -- 0 if no beam/device.
// -1 if busy
// so check the return value to know if things work fine
int MPESBase::updatePosition() {
    if (isBusy()) {
        spdlog::error("{} : MPES::updatePosition() : Busy, cannot read webcam.", m_Identity);
        return -1;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    spdlog::info("{} : MPES::updatePosition() : Reading webcam...", m_Identity);
    int intensity;
    intensity = __updatePosition();
    spdlog::info("{} : MPES::updatePosition() : Done.", m_Identity);
    return intensity;
}

void MPESBase::turnOn() {
    if (isBusy()) {
        spdlog::error("{} : MPES::turnOn() : Busy, cannot turn on MPES.", m_Identity);
        return;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    __initialize();
    __setExposure();
}

#ifndef SIMMODE

#include "common/cbccode/cbc.hpp"
#include "common/mpescode/MPESImage.h"
#include "common/mpescode/MPESImageSet.h"
#include "common/mpescode/MPESDevice.h"

void MPES::turnOff() {
    if (isBusy()) {
        spdlog::error("{} : MPES::turnOff() : Busy, cannot turn off MPES.", m_Identity);
        return;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    m_pCBC->usb.disable(getPortNumber());
}

bool MPES::isOn() {
    return m_pCBC->usb.isEnabled(getPortNumber());
}

// returns intensity of the sensor image.
// check this value to see if everything is working fine
bool MPES::__initialize() {
    spdlog::info("{} : MPES::initialize() : Initializing...", m_Identity);
    // we toggle the usb port, checking the video devices when it's off and again when it's on.
    // the new video device is the ID of the newly created MPES.

    m_Errors.assign(getNumErrors(), false);

    m_pCBC->usb.disable(getPortNumber()); // make sure our USB is off
    std::set<int> oldVideoDevices = getVideoDevices(); // count video devices

    m_pCBC->usb.enable(getPortNumber()); // switch the usb back on and wait for the video device to show up
    sleep(4);

    std::set<int> newVideoDevices = getVideoDevices(); // check all video devices again

    std::set<int> toggledDevices;
    std::set_difference(newVideoDevices.begin(), newVideoDevices.end(), oldVideoDevices.begin(), oldVideoDevices.end(),
                        std::inserter(toggledDevices, toggledDevices.begin()));

    int newVideoDeviceId;
    if (toggledDevices.size() == 1) {
        newVideoDeviceId = *toggledDevices.begin(); // get the only element in the set -- this is the new device ID
        if (newVideoDeviceId == -1) {
            return false; // make sure this is a valid video device -- i.e., not -1
        }
    } else {
        spdlog::error("{} : MPES::initialize() : Found {} devices, should be exactly 1.", m_Identity,
                      toggledDevices.size());
        setError(0);
        return false; // the list should be just one device at this point
    }

    spdlog::debug("MPES::initialize(): Detected new video device {}.", newVideoDeviceId);
    m_pDevice = std::unique_ptr<MPESDevice>(new MPESDevice(newVideoDeviceId));
    m_pImageSet = std::unique_ptr<MPESImageSet>(new MPESImageSet(m_pDevice.get(), DEFAULT_IMAGES_TO_CAPTURE));

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(6) << getSerialNumber(); // pad serial number to 6 digits with zeros
    std::string MPES_IDstring = oss.str();

    std::string matFileFullPath = MATRIX_CONSTANTS_DIR_PATH + MPES_IDstring + "_MatConstants.txt";
    spdlog::trace("{} : MPES::initialize(): Trying to access {} for Matrix File", m_Identity, matFileFullPath);
    std::string calFileFullPath = CAL2D_CONSTANTS_DIR_PATH + MPES_IDstring + "_2D_Constants.txt";
    spdlog::trace("{} : MPES::initialize(): Trying to access {} for Cal2D File", m_Identity, calFileFullPath);

    std::ifstream matFile(matFileFullPath);
    std::ifstream calFile(calFileFullPath);

    if (matFile.good() && calFile.good()) { // Check if files exist and can be read
        matFile.close();
        calFile.close();

        m_pDevice->LoadCalibration(calFileFullPath.c_str());
        m_pDevice->LoadMatrixTransform(matFileFullPath.c_str());
        spdlog::debug("{} : Read calibration data -- using calibrated values.", m_Identity);
        m_Calibrate = true;
    } else {
        spdlog::debug("{} : Did not read calibration data -- using raw values.", m_Identity);
    }

    spdlog::debug("{} : MPES::initialize() : Done.", m_Identity);
    return true;
}

int MPES::__setExposure() {
    spdlog::debug("{} : MPES::setExposure() : Setting exposure...", m_Identity);
    int intensity;

    int counter = 0;
    while ((intensity = __updatePosition())
           && (!m_pDevice->isWithinIntensityTolerance(intensity))
           && (counter < MPESBase::MAX_SET_EXPOSURE_TRIES)
           && m_pDevice->GetExposure() < MPESBase::MAX_EXPOSURE) {
        spdlog::debug("{} : MPES::setExposure() : Intensity {} ({}). Exposure: {}.", m_Identity, intensity,
                      m_pDevice->GetTargetIntensity(), m_pDevice->GetExposure());
        m_pDevice->SetExposure(
            (int) (m_pDevice->GetTargetIntensity() / intensity * ((float) m_pDevice->GetExposure())));

        if (m_pDevice->GetExposure() >= MPESBase::MAX_EXPOSURE) {
            spdlog::error("{} : MPES::setExposure() : Failed to set exposure, reached maximum limit of {}. Setting Error 1...",
                          m_Identity, std::to_string(MPESBase::MAX_EXPOSURE));
            m_pDevice->SetExposure(MPESBase::MAX_EXPOSURE);
            setError(1);
            intensity = -1;
            break;
        }

        if (++counter >= MPESBase::MAX_SET_EXPOSURE_TRIES) {
            spdlog::error("{} : MPES::setExposure() : Failed to set exposure in 5 attempts, setting Error 1...",
                          m_Identity);
            setError(1);
            intensity = -1;
            break;
        }
    }

    spdlog::debug("{} : MPES::setExposure() : Done.", m_Identity);

    return intensity;
}

int MPES::__updatePosition() {
    // initialize to something obvious in case of failure
    m_Position.xCentroid = -1.;
    m_Position.yCentroid = -1.;
    m_Position.xSpotWidth = -1.;
    m_Position.ySpotWidth = -1.;
    m_Position.cleanedIntensity = 0.;
    m_Position.timestamp = -1;
    m_Position.exposure = -1;

    // read sensor
    if (m_pImageSet->Capture()) {
        if (m_Calibrate) {
            m_pImageSet->Matrix_Transform();
            m_pImageSet->Calibrate2D();
        } else {
            m_pImageSet->simpleAverage();
        }

        m_Position.xCentroid = m_pImageSet->SetData.xCentroid;
        m_Position.yCentroid = m_pImageSet->SetData.yCentroid;
        m_Position.xSpotWidth = m_pImageSet->SetData.xCentroidSD;
        m_Position.ySpotWidth = m_pImageSet->SetData.yCentroidSD;
        m_Position.cleanedIntensity = m_pImageSet->SetData.CleanedIntensity;
    }

    if (std::abs(m_Position.cleanedIntensity - MPESBase::NOMINAL_INTENSITY) >= MPESBase::NOMINAL_INTENSITY * 0.2) {
        setError(7);
    } else if (m_Position.cleanedIntensity > 5e5 && m_Position.cleanedIntensity < 1e6) {
        setError(4);
    } else if (m_Position.cleanedIntensity >= 1e6) {
        setError(3);
    }

    if (std::abs(m_Position.xSpotWidth / m_Position.ySpotWidth - 1) > 0.1) {
        setError(6);
    } else if (std::abs(m_Position.xSpotWidth / m_Position.ySpotWidth - 1) > 0.25) {
        setError(5);
    }

    if (m_Position.xCentroid == -1. || m_Position.yCentroid == -1.) {
        spdlog::error("{} : MPES reading of xCenter or yCenter = -1! Potentially lost beam...", m_Identity);
        setError(2);
    }

    m_Position.exposure = m_pDevice->GetExposure(); 
    m_Position.timestamp = std::time(0); 

    return static_cast<int>(m_Position.cleanedIntensity);
}

std::set<int> MPES::getVideoDevices() {
    std::set<int> videoDevices;

    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir("/dev")) != nullptr) { // Open /dev device directory in filesystem
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != nullptr) {
            std::string currentEntry = ent->d_name;
            size_t pos;
            std::string substringToFind = "video"; // Locate video devices (name including the string "video")
            if ((pos = currentEntry.find(substringToFind)) != std::string::npos) { // Found video device
                pos += substringToFind.length(); // go to immediately after the substring "video"
                int deviceNumber = std::stoi(
                        currentEntry.substr(pos)); // grab remaining part of device name to get the device number
                videoDevices.insert(deviceNumber);
            }
        }
        closedir(dir);
    } else {
        videoDevices = {-1}; // output signalling failure
    }

    return videoDevices;
}

#endif

void DummyMPES::turnOff() {
    m_On = false;
}

bool DummyMPES::isOn() {
    return m_On;
}

bool DummyMPES::__initialize() {
    spdlog::debug("{} : DummyMPES::initialize(): Initializing...", m_Identity);

    m_Errors.assign(getNumErrors(), false);

    m_On = true;

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(6) << getSerialNumber(); // pad serial number to 6 digits with zeros
    std::string MPES_IDstring = oss.str();

    std::string matFileFullPath = MATRIX_CONSTANTS_DIR_PATH + MPES_IDstring + "_MatConstants.txt";
    spdlog::trace("{} : DummyMPES::initialize(): Trying to access {} for Matrix File", m_Identity, matFileFullPath);
    std::string calFileFullPath = CAL2D_CONSTANTS_DIR_PATH + MPES_IDstring + "_2D_Constants.txt";
    spdlog::trace("{} : DummyMPES::initialize(): Trying to access {} for Cal2D File", m_Identity, calFileFullPath);

    std::ifstream matFile(matFileFullPath);
    std::ifstream calFile(calFileFullPath);

    if (matFile.good() && calFile.good()) { // Check if files exist and can be read
        matFile.close();
        calFile.close();

        spdlog::debug("{} : Read calibration data -- using calibrated values.", m_Identity);
        m_Calibrate = true;
    } else {
        spdlog::debug("{} : Did not read calibration data -- using raw values.", m_Identity);
    }

    spdlog::debug("{} : DummyMPES::initialize() : Done.", m_Identity);
    return true;
}

int DummyMPES::__setExposure() {
    spdlog::debug("{} : DummyMPES::setExposure() : Setting exposure...", m_Identity);
    sleep(5);
    int intensity = MPESBase::NOMINAL_INTENSITY; // dummy value
    spdlog::debug("{} : DummyMPES::setExposure() : Done.", m_Identity);
    return intensity;
}

int DummyMPES::__updatePosition() {
    // Set internal position variable to dummy values
    m_Position.xCentroid = 160.;
    m_Position.yCentroid = 120.;
    m_Position.xSpotWidth = MPESBase::NOMINAL_SPOT_WIDTH;
    m_Position.ySpotWidth = MPESBase::NOMINAL_SPOT_WIDTH;
    m_Position.cleanedIntensity = MPESBase::NOMINAL_INTENSITY;

    return static_cast<int>(m_Position.cleanedIntensity);
}
