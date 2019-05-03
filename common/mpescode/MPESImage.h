#ifndef __MPESIMAGE_H__
#define __MPESIMAGE_H__

#include <vector>
#include <opencv2/core/core.hpp> // cv::Mat, cv::Size

struct MPESImageData
{
    int cleaningThreshold; /// Minimum pixel value used 
    float Intensity; /// Total intensity of image without cleaning
    float CleanedIntensity; ///Sum of all pixel values excluding those below cleaning threshold.
    float xCentroid; ///X coordinate of centroid
    float yCentroid; ///Y coordinate of centroid
    float xSpotSD; ///< This is width of laser point
    float ySpotSD; /// Y width of centroid
    int nSat; /// Number of saturated pixels in image
    int SatPoint; /// Value of Saturation point, default 255
    double MJD; ///Time of populating this struct.
};

class MPESImage: public cv::Mat
{
    //friend class MPESImageData;

    public:
	std::vector<MPESImageData> datavec;

	MPESImage() : cv::Mat() {}
	MPESImage(int rows, int cols, int type) : cv::Mat(rows, cols, type) {}
	MPESImage(cv::Size size, int type) : cv::Mat(size, type) {}
	MPESImage(const cv::Mat& m) : cv::Mat(m) {}
	MPESImage(const MPESImage& i) : cv::Mat(i), datavec(i.datavec) {}

	MPESImage& operator=(const MPESImage& img);
	
        /// \brief Calculates Centroid and RMS values - populates datavec with results, including intensities and threshold value.
        /** Calculates parameters for image.
        Parses image in Mat format and calculates total intensity as sum of all pixels. If there is a cleaning threshold, sums only pixels with values greater than the threshold. With intensity values calculates width of spot by find the RMS of the  projected intensities along an axis. Calculates the centroid by computing  weighted average along the axis.

Using the X values as an example:
        
X Centroid calculation (using pixels i,j): 
        
        \f$ Centroid_{x} = \sum_{j=0}^{N_{row}} \sum_{i=0}^{N_{rows}} i \times p_{ij} + x_{cal} \f$

X Spot width:

        \f$ \text{columnvalue}_{x} = \sum_{i=0}^{N_{cols}} p_{ij} \f$

        \f$ c = \frac{1}{\text{Intensity}_{\text{Cleaned}}} \sum_{j=0}^{N_{rows}} \text{columnvalue} \times (i+1)^2 \f$

        \f$ d = \left( \frac{1}{\text{Intensity}_{\text{Cleaned}}} \sum_{j=0}^{N_{rows}} \text{columnvalue} \times (i+1)\right)^2 \f$

        \f$ Spot_{y} = \sqrt{\left|c-d\right|}\f$.*/
        /*! \fn analyze
        @param thresh - Threshold value, minimum value accepted for each pixel.
        @return datavec vector element with MPESImageData populated.
        */
	void analyze(int thresh);
        ///Populates parameter results for a specific instance of Threshold value, appends data to a vector of MPESImageData elements.
	MPESImageData *dataForThresh(int thresh);
};



#endif
