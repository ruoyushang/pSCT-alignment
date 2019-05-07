#include "common/alignment/mpes.hpp"

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

#include "common/alignment/platform.hpp"
#include "common/cbccode/cbc.hpp"
#include "common/mpescode/MPESImage.h"
#include "common/mpescode/MPESImageSet.h"
#include "common/mpescode/MPESDevice.h"

const int MPES::DEFAULT_IMAGES_TO_CAPTURE = 9;
const std::string MATRIX_CONSTANTS_DIR_PATH = "/home/root/mpesCalibration/";
const std::string CAL2D_CONSTANTS_DIR_PATH = "/home/root/mpesCalibration/";

const float SAFETY_REGION_X_MIN = 60.0;
const float SAFETY_REGION_X_MAX = 260.0;
const float SAFETY_REGION_Y_MIN = 40.0;
const float SAFETY_REGION_Y_MAX = 200.0;

const float NOMINAL_INTENSITY = 150000.;
const float NOMINAL_CENTROID_SD = 20.;

const std::vector<Device::ErrorDefinition> MPES::ERROR_DEFINITIONS = {
    {"Bad connection. No device found",                                                                            Device::DeviceState::FatalError},//error 0
    {"Intermittent connection, select timeout.",                                                                   Device::DeviceState::FatalError},//error 1
    {"Cannot find laser spot (totally dark). Laser dead or not in FoV.",                                           Device::DeviceState::OperableError},//error 2
    {"Too bright. Cleaned Intensity > 1e6. Likely cause: no tube.",                                                Device::DeviceState::OperableError},//error 3
    {"Too bright. 1e6 >Cleaned Intensity > 5e5 and very wide spot width >20",                                      Device::DeviceState::FatalError},//error 4
    {"Very uneven spot. Likely due to being in the reflection region (too close to webcam edges) or a bad laser.", Device::DeviceState::OperableError},//error 5
    {"Uneven spot. Spot is uneven, but not severe. Likely recoverable.",                                           Device::DeviceState::OperableError},//error 6
    {"Intensity deviation from nominal value (more than 20%).",                                                    Device::DeviceState::OperableError},//error 7
};

MPES::MPES(std::shared_ptr<CBC> pCBC, Device::Identity identity) : Device::Device(std::move(pCBC), std::move(identity)),
                                                                   m_Calibrate(false)
{
}

MPES::~MPES() {
    m_pCBC->usb.disable(getPortNumber());
}

// returns intensity of the sensor image.
// check this value to see if everything is working fine
bool MPES::initialize()
{
    // we toggle the usb port, checking the video devices when it's off and again when it's on.
    // the new video device is the ID of the newly created MPES.

    m_Errors.assign(getNumErrors(), false);

    m_pCBC->usb.disable(getPortNumber()); // make sure our USB is off
    std::set<int> oldVideoDevices = getVideoDevices(); // count video devices

    m_pCBC->usb.enable(getPortNumber()); // switch the usb back on and wait for the video device to show up
    sleep(4);

    std::set<int> newVideoDevices = getVideoDevices(); // check all video devices again

    std::set<int> toggledDevices;
    std::set_difference(newVideoDevices.begin(), newVideoDevices.end(), oldVideoDevices.begin(), oldVideoDevices.end(),std::inserter(toggledDevices, toggledDevices.begin()) );

    int newVideoDeviceId;
    if (toggledDevices.size() == 1) {
        newVideoDeviceId = *toggledDevices.begin(); // get the only element in the set -- this is the new device ID
        if (newVideoDeviceId == -1) {
            return false; // make sure this is a valid video device -- i.e., not -1
        }
    }
    else {
        setError(0);
        return false; // the list should be just one device at this point
    }

    std::cout << "MPES::initialize(): Detected new video device " << newVideoDeviceId << std::endl;
    m_pDevice = std::unique_ptr<MPESDevice>(new MPESDevice(newVideoDeviceId));
    m_pImageSet = std::unique_ptr<MPESImageSet>(new MPESImageSet(m_pDevice.get(), DEFAULT_IMAGES_TO_CAPTURE));

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(6) << getSerialNumber(); // pad serial number to 6 digits with zeros
    std::string MPES_IDstring = oss.str();

    std::string matFileFullPath = MATRIX_CONSTANTS_DIR_PATH + MPES_IDstring + "_MatConstants.txt";
    std::cout << "Trying to access " << matFileFullPath << " for Matrix File" << std::endl;
    std::string calFileFullPath = CAL2D_CONSTANTS_DIR_PATH + MPES_IDstring + "_2D_Constants.txt";
    std::cout << "Trying to access " << calFileFullPath << " for Cal2D File" << std::endl;

    std::ifstream matFile(matFileFullPath);
    std::ifstream calFile(calFileFullPath);

    if ( matFile.good() && calFile.good() ) { // Check if files exist and can be read
        matFile.close();
        calFile.close();

        m_pDevice->LoadCalibration(calFileFullPath.c_str());
        m_pDevice->LoadMatrixTransform(matFileFullPath.c_str());
        std::cout << "Read calibration data OK -- using calibrated values\n";
        m_Calibrate = true;
    }
    else {
        std::cout << "Failed to read calibration data -- using raw values\n";
    }

    return true;
}

// find and set optimal exposure -- assume I(e) is linear
// returns measured intensity -- check this value to see if things work fine
int MPES::setExposure()
{
    std::cout << "MPES:: Setting exposure for device at USB " << getPortNumber() << std::endl;
    int intensity;

    int counter = 0;
    while ((intensity = updatePosition())
           && (!m_pDevice->isWithinIntensityTolerance(intensity))
           && (counter <= 5))
    {
        m_pDevice->SetExposure(
            (int) (m_pDevice->GetTargetIntensity() / intensity * ((float) m_pDevice->GetExposure())));

        if (++counter > 5) {
            setError(1);
            intensity = -1;
            break;
        }
    }

    std::cout << "MPES:: setExposure() DONE" << std::endl;
    return intensity;
}

// returns intensity of the beam -- 0 if no beam/device.
// so check the return value to know if things work fine
int MPES::updatePosition()
{
    // initialize to something obvious in case of failure
    m_Position.xCentroid = -1.;
    m_Position.yCentroid = -1.;
    m_Position.xSpotWidth = -1.;
    m_Position.ySpotWidth = -1.;
    m_Position.cleanedIntensity = 0.;

    // read sensor
    if (m_pImageSet->Capture()) {
        if (m_Calibrate) {
    	    m_pImageSet->Matrix_Transform();
    	    m_pImageSet->Calibrate2D();
        }
        else {
	        m_pImageSet->simpleAverage();
        }

        m_Position.xCentroid = m_pImageSet->SetData.xCentroid;
        m_Position.yCentroid = m_pImageSet->SetData.yCentroid;
        m_Position.xSpotWidth = m_pImageSet->SetData.xCentroidSD;
        m_Position.ySpotWidth = m_pImageSet->SetData.yCentroidSD;
        m_Position.cleanedIntensity = m_pImageSet->SetData.CleanedIntensity;
    }

    if (std::abs(m_Position.cleanedIntensity - NOMINAL_INTENSITY) >= NOMINAL_INTENSITY * 0.2) {
        setError(7);
    } else if (m_Position.cleanedIntensity > 5e5 && m_Position.cleanedIntensity < 1e6) {
        setError(4);
    } else if (m_Position.cleanedIntensity >= 1e6) {
        setError(3);
    }

    if (std::abs(m_Position.xSpotWidth / m_Position.xSpotWidth - 1) > 0.1) {
        setError(6);
    } else if (std::abs(m_Position.xSpotWidth / m_Position.xSpotWidth - 1) > 0.25) {
        setError(5);
    }

    if (m_Position.xCentroid == -1. || m_Position.yCentroid == -1.) {
        std::cout << "MPES reading of xCenter or yCenter = -1! Potentially lost beam..." << std::endl;
        setError(2);
    }


    return static_cast<int>(m_Position.cleanedIntensity);
}

std::set<int> MPES::getVideoDevices()
{
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
                int deviceNumber = std::stoi(currentEntry.substr(pos)); // grab remaining part of device name to get the device number
                videoDevices.insert(deviceNumber);
            }
        }
        closedir(dir);
    }
    else {
        videoDevices = {-1}; // output signalling failure
    }

    return videoDevices;
}

bool DummyMPES::initialize()
{
    std::cout << "DummyMPES::initialize(): Creating new video device " << std::endl;
    return true;
}

int DummyMPES::setExposure()
{
    std::cout << "+++ Dummy MPES: Setting exposure for device at USB " << getPortNumber() << std::endl;
    int intensity = NOMINAL_INTENSITY; // dummy value
    return intensity;
}

int DummyMPES::updatePosition()
{
    // Set internal position variable to dummy values
    m_Position.xCentroid = 160.;
    m_Position.yCentroid = 80.;
    m_Position.xSpotWidth = 10.;
    m_Position.ySpotWidth = 10.;
    m_Position.cleanedIntensity = NOMINAL_INTENSITY;

    return static_cast<int>(m_Position.cleanedIntensity);
}
