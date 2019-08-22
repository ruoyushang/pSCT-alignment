// MPES device definitions file

#include "MPESDevice.h"
#include <fstream> // ifstream
#include <sstream> // istringstream
#include <string>  // string
#include <cmath>   // fabs()

// default constructor -- load default values
MPESDevice::MPESDevice() :
    m_id {0},
    m_on {false}, m_calLoaded {false}, m_matLoaded {false},
    m_resolution {320},
    m_exposure {500},
    m_imgLapse {0},
    m_targetIntensity {kNOMINAL_INTENSITY},
    m_targetIntensityTolerance {kINTENSITY_RATIO_TOLERANCE}
{
}

MPESDevice::MPESDevice(int ID) :
    m_id {ID},
    m_on {false}, m_calLoaded {false}, m_matLoaded {false},
    m_resolution {320},
    m_exposure {500},
    m_imgLapse {0},
    m_targetIntensity {kNOMINAL_INTENSITY},
    m_targetIntensityTolerance {kINTENSITY_RATIO_TOLERANCE}
{
}

// expanded constructor
MPESDevice::MPESDevice(int ID, int resolution, int exposure, float lapse, float targetIntensity, float intensityTolerance) :
    m_id {ID},
    m_on {false}, m_calLoaded {false}, m_matLoaded {false},
    m_resolution {resolution},
    m_exposure {exposure},
    m_imgLapse {lapse},
    m_targetIntensity {targetIntensity},
    m_targetIntensityTolerance {intensityTolerance}
{
}

MPESDevice::~MPESDevice()
{
}

bool MPESDevice::isWithinIntensityTolerance(float intensity)
{
     return (fabs(intensity/m_targetIntensity - 1) <= m_targetIntensityTolerance);
}

int MPESDevice::LoadCalibration(const char *calFile)
{
    std::ifstream file(calFile);
    std::string line;
    if (!file.good())
        perror ("Error opening file");
    else 
    {
        int i = 0;
        float X, Y, xcal, ycal;
        while (getline(file,line))
        {
            std::istringstream ss(line);
            ss >> X >> Y >> xcal >> ycal;
            
            m_Calibration.XfromMat[i]=X*GetResolution();
            m_Calibration.YfromMat[i]=Y*GetResolution()*0.75;
            m_Calibration.xcalfromMat[i]=xcal*GetResolution();
            m_Calibration.ycalfromMat[i]=xcal*GetResolution()*0.75;
            i++;	
        }
        m_Calibration.calLen = i;
        m_calLoaded = true;
    }

    return m_calLoaded;
}

int MPESDevice::LoadMatrixTransform(const char *matFile)
{
    std::ifstream file(matFile);
    std::string line;
    if (!file.good())
        perror ("Error opening file");
    else 
    {
        while (getline(file, line))
        {
            std::istringstream ss(line);
            ss >> m_Calibration.Angle 
               >> m_Calibration.Tx 
               >> m_Calibration.Ty 
               >> m_Calibration.Tz;	
        }
    m_matLoaded = true;
    }
    
    return m_matLoaded;
}
