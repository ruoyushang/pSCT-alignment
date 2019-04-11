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

MPES::MPES(std::shared_ptr<CBC> pCBC, int USBPortNumber, int serialNumber) :
    m_pCBC {pCBC},
    m_SerialNumber(serialNumber),
    m_Calibrate(false)
{
    setPortNumber(USBPortNumber);
}

MPES::~MPES()
{
    m_pCBC->usb.disable(m_USBPortNumber);
}

// returns intensity of the sensor image.
// check this value to see if everything is working fine
bool MPES::initialize()
{
    // we toggle the usb port, checking the video devices when it's off and again when it's on.
    // the new video device is the ID of the newly created MPES.

    m_pCBC->usb.disable(m_USBPortNumber); // make sure our USB is off
    std::set<int> oldVideoDevices = getVideoDevices(); // count video devices

    m_pCBC->usb.enable(m_USBPortNumber); // switch the usb back on and wait for the video device to show up
    sleep(4);

    std::set<int> newVideoDevices = getVideoDevices(); // check all video devices again

    std::set<int> toggledDevices;
    std::set_difference(newVideoDevices.begin(), newVideoDevices.end(), oldVideoDevices.begin(), oldVideoDevices.end(), toggledDevices.begin());

    if (toggledDevices.size() == 1) {
        int newVideoDeviceId = *toggledDevices.begin(); // get the only element in the set -- this is the new device ID
        if (newVideoDeviceId == -1) {
            return false; // make sure this is a valid video device -- i.e., not -1
        }
    }
    else {
        return false; // the list should be just one device at this point
    }

    std::cout << "MPES::initialize(): Detected new video device " << newVideoDeviceId << std::endl;
    m_pDevice = std::shared_ptr<MPESDevice>(new MPESDevice(newVideoDeviceId));
    m_pImageSet = std::unique_ptr<MPESImageSet>(new MPESImageSet(m_pDevice, DEFAULT_IMAGES_TO_CAPTURE));

    std::ostringstream oss;
    std::oss << std::setfill('0') << std::setw(6) << m_SerialNumber; // pad serial number to 6 digits with zeros
    std::string MPES_IDstring = oss.str();

    std::string matFileFullPath = MATRIX_CONSTANTS_DIR_PATH + MPES_IDstring + "_MatConstants.txt";
    std::cout << "Trying to access " << matFileFullPath << " for Matrix File" << std::endl;
    std::string calFileFullPath = CAL2D_CONSTANTS_DIR_PATH + MPES_IDstring + "_2D_Constants.txt";
    std::cout << "Trying to access " << calFileFullPath << " for Cal2D File" << std::endl;

    std::ifstream matFile(matFileFullPath);
    std::ifstream calFile(calFileFullPath);

    if ( matFile.good() && calFile.good() ) { // Check if files exist and can be read
        matFile.close()
        calFile.close()

        m_pDevice->loadCalibration(calFileFullPath.c_str());
        m_pDevice->loadMatrixTransform(matFileFullPath.c_str());
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
    std::cout << "MPES:: Setting exposure for device at USB " << m_USBPortNumber << std::endl;
    int intensity;

    int counter = 0;
    while ( (intensity = measurePosition())
            && (!m_pDevice->isWithinIntensityTolerance(intensity))
            && (counter <= 5))
    {
        m_pDevice->setExposure((int)(m_pDevice->getTargetIntensity() / intensity * ((float)m_pDevice->getExposure())));

        ++counter;
    }

    std::cout << "MPES:: setExposure() DONE" << std::endl;
    return intensity;
}

void MPES::setPortNumber(int USBPortNumber)
{
    if ((USBPortNumber >= 1) && (USBPortNumber <= 6)) {
        m_USBPortNumber = usbPortNumber;
    }
    else {
        std::cout << "MPES:: Invalid USB port " << USBPortNumber << " (USB Port must be between 1-6). USB port not set" << std::endl;
    }
}

// returns intensity of the beam -- 0 if no beam/device.
// so check the return value to know if things work fine
int MPES::measurePosition()
{
    // initialize to something obvious in case of failure
    m_Position.xCenter = -1.;
    m_Position.yCenter = -1.;
    m_Position.xStdDev = -1.;
    m_Position.yStdDev = -1.;
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

        m_Position.xCenter = m_pImageSet->SetData.xCentroid;
        m_Position.yCenter = m_pImageSet->SetData.yCentroid;
        m_Position.xStdDev = m_pImageSet->SetData.xCentroidSD;
        m_Position.yStdDev = m_pImageSet->SetData.yCentroidSD;
        m_Position.CleanedIntensity = m_pImageSet->SetData.CleanedIntensity;
    }

    if (m_Position.xCenter == -1. || m_Position.yCenter == -1. ) {
        std::cout << "MPES reading of xCenter or yCenter = -1! Potentially lost beam..." << std::endl;
    }

    return static_cast<int>(m_Position.cleanedIntensity);
}

std::set<int> MPES::getVideoDevices()
{
    std::set<int> videoDevices;

    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir("/dev")) != NULL) { // Open /dev device directory in filesystem
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != NULL) {
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
    std::cout << "+++ Dummy MPES: Setting exposure for device at USB " << m_USBPortNumber << std::endl;
    int intensity = NOMINAL_INTENSITY; // dummy value
    return intensity;
}

int DummyMPES::measurePosition()
{
    // Set internal position variable to dummy values
    m_Position.xCenter = 160.;
    m_Position.yCenter = 80.;
    m_Position.xStdDev = 10.;
    m_Position.yStdDev = 10.;
    m_Position.cleanedIntensity = NOMINAL_INTENSITY;

    return static_cast<int>(m_Position.cleanedIntensity);
}
