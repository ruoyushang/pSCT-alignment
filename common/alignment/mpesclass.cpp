/*
 * mpesclass.cpp MPES Control
 */

#include "MPESImage.h"
#include "MPESDevice.h"
#include "mpesclass.hpp"
#include "actuator.hpp"
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstdlib> // NULL, atoi
#include <dirent.h> // DIR

using namespace std;

int MPES::sDefaultImagesToCapture = 9;
string MPES::matFileString = "/home/root/mpesCalibration/";
string MPES::calFileString = "/home/root/mpesCalibration/";

//Default Constructor
////MPES::MPES() : cbc(0) {}

MPES::MPES(CBC* input_cbc, int input_USBPortNumber, int input_MPES_ID) :
    m_pCBC {input_cbc},
    m_serialNumber(input_MPES_ID),
    calibrate(false)
{
    setUSBPortNumber(input_USBPortNumber);
}

//Destructor
MPES::~MPES() 
{
    m_pCBC->usb.disable(m_USBPortNumber);
}


// returns intensity of the sensor image. 
// check this value to see if everything is working fine
bool MPES::Initialize()
{
    // we toggle the usb port, checking the video devices when it's off and again when it's on.
    // the new video device is the ID of the newly created MPES.
    //
    // make sure our USB is off 
    m_pCBC->usb.disable(m_USBPortNumber);
    // count video devices
    auto old_video_device_set = __getVideoDevices();

    // switch the usb back on and wait for the video device to show up
    m_pCBC->usb.enable(m_USBPortNumber);
    sleep(4);
    // count video devices again
    auto new_video_device_set = __getVideoDevices();
    // find the new ones compared to the old set
    for (const auto& dev : old_video_device_set) new_video_device_set.erase(dev);

    // the list should be just one device at this point
    if (new_video_device_set.size() != 1) return false;

    // get the only element in the set -- this is the new device ID
    int new_video_device_id = *new_video_device_set.begin();
    // make sure this is a valid video device -- i.e., not -1
    if (new_video_device_id == -1) return false;

    cout << "MPES::Initialize(): Detected new video device " << new_video_device_id << endl;
    m_pDevice = new MPESDevice(new_video_device_id);
    m_pImageSet = new MPESImageSet(m_pDevice, sDefaultImagesToCapture);

    ostringstream temposs;
    temposs << setfill('0') << setw(6) << m_serialNumber;
    string MPES_IDstring = temposs.str();
    string matFileFullPath = matFileString + MPES_IDstring + "_MatConstants.txt";
    cout << "trying to access " << matFileFullPath << " for Matrix File" << endl;
    string calFileFullPath = calFileString + MPES_IDstring + "_2D_Constants.txt";
    cout << "trying to access " << calFileFullPath << " for Cal2D File" << endl;

    FILE *matFile, *calFile;
    if ( (matFile = fopen(matFileFullPath.c_str(), "r")) &&
         (calFile = fopen(calFileFullPath.c_str(), "r")) )
    {
        fclose(matFile);
        fclose(calFile);

        m_pDevice->LoadCalibration(calFileFullPath.c_str());
        m_pDevice->LoadMatrixTransform(matFileFullPath.c_str());
        cout << "Read calibration data OK -- using calibrated values" << endl;
        calibrate = true;
    }
    else 
        cout << "Failed to read calibration data -- using raw values" << endl;

    Safety_Region_x_min = 60.0;	
    Safety_Region_x_max = 260.0;	
    Safety_Region_y_min = 40.0;	
    Safety_Region_y_max = 200.0;	

    return true;
}

// find and set optimal exposure -- assume I(e) is linear
// returns measured intensity -- check this value to see if things work fine
int MPES::setExposure()
{
    cout << "+++ MPES: Setting exposure for device at USB " << m_USBPortNumber << endl;
    int intensity;
    // need to check if intensity is non-zero!
    int counter = 0;
    while ( (intensity = MeasurePosition()) 
            && !m_pDevice->isWithinIntensityTolerance(intensity) )
    {
        m_pDevice->SetExposure((int)(m_pDevice->GetTargetIntensity()/intensity
                   *(float)m_pDevice->GetExposure()));

	if (++counter > 5) break;
    }
    cout << "setExposure(): DONE" << endl;
    return intensity;
}

void MPES::setUSBPortNumber(int input_USBPortNumber)
{
    if ((input_USBPortNumber < 1) || (input_USBPortNumber > 6))
    {
        cout << "USB Port must be between 1-6" << endl;
        throw 1;
    }
    m_USBPortNumber=input_USBPortNumber;
}
 
// returns intensity of the beam -- 0 if no beam/device.
// so check the return value to know if things work fine
int MPES::MeasurePosition()
{
    // initialize to something obvious in case of failure
    m_position.xCenter = -1.;
    m_position.yCenter = -1.;
    m_position.xStdDev = -1.;
    m_position.yStdDev = -1.;
    m_position.CleanedIntensity = 0.;

    // read sensor
    if (m_pImageSet->Capture()) {
        if (calibrate) {
	    m_pImageSet->Matrix_Transform();
	    m_pImageSet->Calibrate2D();
        }
        else
	    m_pImageSet->simpleAverage();
    
        m_position.xCenter = m_pImageSet->SetData.xCentroid;
        m_position.yCenter = m_pImageSet->SetData.yCentroid;
        m_position.xStdDev = m_pImageSet->SetData.xCentroidSD;
        m_position.yStdDev = m_pImageSet->SetData.yCentroidSD;
        m_position.CleanedIntensity = m_pImageSet->SetData.CleanedIntensity;
    }

    if (m_position.xCenter == -1. || m_position.yCenter == -1. )
        cout << "mpes reading -1! potentially lost beam" << endl;

    return static_cast<int>(m_position.CleanedIntensity);
}

set<int> MPES::__getVideoDevices()
{
    set<int> device_set;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ("/dev")) != NULL) {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            string curentry = ent->d_name;
            size_t index;
            string dev_to_find = "video";
            if ( (index = curentry.find(dev_to_find)) != string::npos) {
                index += dev_to_find.length();
                int curdev = atoi(curentry.substr(index).c_str());
                device_set.insert(curdev);
            }
        }
        closedir (dir);
    }
    else
        return {-1};

    return device_set;
}
