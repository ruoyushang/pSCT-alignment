#ifndef __MPESDEVICE_H__
#define __MPESDEVICE_H__

static constexpr float kNOMINAL_INTENSITY = 110000.0; // default was 150,000.
static constexpr float kNOMINAL_SPOT_WIDTH = 10.0;
static constexpr int kMAX_EXPOSURE = 5000-1;
static constexpr int kMIN_EXPOSURE = 50;
static constexpr float kINTENSITY_RATIO_TOLERANCE = 0.3;
static constexpr float kPRECISION = kINTENSITY_RATIO_TOLERANCE + 1;

struct MPESCalibrationData
{
    // matrix stuff
    float Angle;
    float Tx;
    float Ty;
    float Tz;

    // 2D calibration stuff
    // The 500 lengths are for the max number of grid points from the calibration stage
    int calLen;
    float XfromMat[500];
    float YfromMat[500];
    float xcalfromMat[500];
    float ycalfromMat[500];
};

class MPESDevice
{
    public:
        MPESDevice();
        MPESDevice(int ID);
        MPESDevice(int ID, int resolution, int exposure, float lapse, float targetIntensity, float intensityTolerance);
        ~MPESDevice();

        // inline setters and getters
        void switchOn() {m_on = 1;};
        bool isOn() {return m_on;};
        bool isWithinIntensityTolerance(float intensity);
        bool CalibrationLoaded() {return m_calLoaded;};
        bool MatrixLoaded() {return m_matLoaded;};
        int GetID() {return m_id;};
        void SetID(int ID) {m_id = ID;};
        void SetTolerance(float tolerance) {m_targetIntensityTolerance = tolerance;}
        void SetResolution(int in_resolution) {m_resolution = in_resolution;};
        int GetResolution() {return m_resolution;};
        void SetExposure(int in_exposure) {m_exposure = in_exposure;};
        int GetExposure() {return m_exposure;};
        float GetTargetIntensity() {return m_targetIntensity;};
        void SetLapse(float lapse) {m_imgLapse = lapse;};
        float GetLapse() {return m_imgLapse;};
        double GetTemperature() {return m_temperature;};
        double GetDate() {return m_MJD;};

        MPESCalibrationData GetCalibration() {return m_Calibration;};

        // calibration methods
        int LoadCalibration(const char *calFile);
        int LoadMatrixTransform(const char *matFile);

    private:

        int m_id; ///< Id number for device, found at /dev/video$(ID)
        bool m_on; ///< Is the program being used at the moment?
        bool m_calLoaded; ///< Has the calibration data been loaded?
        bool m_matLoaded; ///< Has the calibration matrix been loaded?
        int m_resolution; ///< Resolution of the image, where 640x480 is high res and 320x280 is low res.
        int m_exposure; ///<Value determined by another program for best possible camera resolution, set manually
        float m_imgLapse;
        float m_targetIntensity; ///< Optimal sensor image intensity that results in maximum stability
        float m_targetIntensityTolerance; ///< tolerance abs(cur/target - 1) within which current intensity is acceptable. If current intensity is outside of the tolerance range, exposure needs to be reset
        double m_temperature; ///< Temperature of the edge sensor, currently not used
        double m_MJD; ///< Time of initiallizing program.

        MPESCalibrationData m_Calibration;
};
        
#endif

