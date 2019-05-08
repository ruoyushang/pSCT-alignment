#ifndef __MPESIMAGESET_H__
#define __MPESIMAGESET_H__

#include "MPESImage.h"
#include "MPESDevice.h"
#include <vector>
#include <string>
#include <unistd.h>

struct MPESSetData: MPESImageData
{
    float IntensitySD; 
    float CleanedIntensitySD; 
    float xCentroidSD; 
    float yCentroidSD;
    float MJD;
    float Xcal;
    float Ycal;
};

class MPESImageSet
{
    private:
        MPESDevice *device = nullptr; // Information about device (ID, exposure, resolution, etc).
                            // this needs to be a pointer as we can configure the device from
                            // outside of this class
        int imagesToCapture;
        int capturedImages;
        int iThresh;
        float x[200]; // the 200 lengths are for the max number of images in this setup.
        float y[200];
        float cIntensity[200];
        float tIntensity[200];
        float xwidth[200];
        float ywidth[200];
        float xcal[200];
        float ycal[200];
        float nsat[200];
        void makeArrays();
        const char * dir;
        bool m_Calibrated;
        bool verbosity; /// Bool to print all results to stderr.

    public:
        bool has_suffix(const std::string& s, const std::string& suffix);
        std::vector< std::vector<MPESImageData> > datasetvec; ///< Nested <vector> of groups of MPESImage data.
        MPESSetData SetData; ///< Accumulated data from all the images, containing statistical information.
        MPESImageSet() {}; ///< Default Constructor.
        
	    /// Constructor
/*!
@param dir - location of directory to write new images. Directory must already exit. 
@param thresh - Threshold cleaning value for analysis, minimum value of pixel to include in calculation.
@param exposure - Camera exposure, default to 625, or from optimum exposure calculation unique to camera (ranges 1 to 5000).
@param verbosity - Bool to print all results to stderr. 
*/
        MPESImageSet(const char * dir, int thresh, bool verbosity=0);
        /// Constructor
/*!
DEPRECATED!
@param ID - ID of camera in /dev/video(ID)
@param N -number of images to create
@param l - lapse between each image captured, in seconds.
@param dir - location of directory to write new images. Directory must already exit. 
@param thresh - Threshold cleaning value for analysis, minimum value of pixel to include in calculation.
@param resolution - Camera resolution, either 640 or 320.
@param exposure - Camera exposure, default to 625, or from optimum exposure calculation unique to camera (ranges 1 to 5000).
@param verbosity - Bool to print all results to stderr. 
*/
//        MPESImageSet(int ID, int N, float l = 0,const char *dir = NULL,  double resolution=320, int exposure=625, int thresh=50, bool verbosity=0);

/// Constructor
/*!
@param Device - already-existing MPES Device
@param N -number of images to create
@param dir - location of directory to write new images. Directory must already exit. 
@param thresh - Threshold cleaning value for analysis, minimum value of pixel to include in calculation.
@param verbosity - Bool to print all results to stderr. 
*/
        MPESImageSet(MPESDevice* in_device, int N, const char *dir = NULL, int thresh=50, bool verbosity=0);

        ///  Captures the images for this set
        int Capture();
        /*! 
         Run this after constructing the ImageSet object to capture images. Creates new images from camera and writes to a directory or simply reads images from a directory, depending on the constructor used. Then analyzes each image, populating a vector of objects for each image. Returns the number of images captured.
         */
        ///  Prints properties for each image.
        void ImageProperties(FILE * file);
        /*! Calculates average values for centroids, RMS and intensities, populating results to SetData object. 

This method calculates Averages and Standard deviations for the multiple parameters in the system (Intensities, Centroids, SpotSD). For average, it uses the classic average calculation \f$ \sum^{N} \frac{Z_{N}}{N}\f$. For standard deviation, \f$ SD= \sqrt{ \frac{ \sum (X_i - \bar{X})^2 }{N}}\f$. However, specifically for SpotSD, \f$ \text{SpotSD}_{x} = \sqrt{\frac{\sum SD_i^2 }{N_{\text{images}}} } \f$. 

*/
        void simpleAverage();
        /// Prints properties for set of images.
        void printSetProperties(FILE * file);
	
        /// Uses setData (averaged results) and calibration matrix to find calibration constants by interpolation, and updates Centroids.
	void Matrix_Transform();
	void Calibrate();
	void Calibrate2D();
};

#endif
