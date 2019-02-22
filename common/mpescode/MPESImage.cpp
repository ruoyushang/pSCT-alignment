/*!
October 30th, 2014.

Written by Deivid Ribeiro, adapted from draft originally written by Emma Sarachan. Uses method from cutprojection program written by Daniel Nieto.

This class initializes an Image object, an opencv Mat object, and populates a MPESImageData class of results.
 
*/

#include "MPESImage.h"
#include <cstdlib> // abs
#include <ctime> // time_t, time()

using namespace cv;
using namespace std;

MPESImage& MPESImage::operator=(const MPESImage& img)
{
    Mat::operator=(img);
    datavec = img.datavec;
    return *this;
}

void MPESImage::analyze(int thresh)
{
    MPESImageData data;
    data.cleaningThreshold = thresh;

    Vec3b pixel;
    unsigned int pixelintensity = 0;
    unsigned int intensity = 0;
    unsigned int totalintensity = 0;
    unsigned int cleanedIntensity = 0 ;

    unsigned int rowvalue = 0;
    unsigned int columnvalue = 0;
    float a = 0;
    float b = 0;
    float c = 0;
    float d = 0;
    float isum = 0;
    float jsum = 0;
    int nSat = 0;
    int SatPoint = 255;


    for(int i = 0; i < this->rows; i++)
    {
		rowvalue = 0;
		for(int j = 0; j < this->cols; j++)
		{
			pixel = this->at<Vec3b>(i, j);
			pixelintensity = (pixel.val[0] + pixel.val[1] + pixel.val[2]) / 3.;
			intensity += pixelintensity;
			if ((int)pixelintensity > thresh) 
			{
			rowvalue += pixelintensity;
			}
			if(pixel.val[2] > (SatPoint-1))
			{
				nSat++;
			}
		}
	
		totalintensity += intensity; 
		cleanedIntensity+= rowvalue;
		a += (rowvalue * (i+1) * (i+1));
        b += (rowvalue * (i+1));
    }

    for(int i = 0; i < this->cols; i++)
    {
        columnvalue = 0;
        for(int j = 0; j < this->rows; j++)
        {
            pixel = this->at<Vec3b>(j, i);
			pixelintensity = (pixel.val[0] + pixel.val[1] + pixel.val[2]) / 3.;
			if ((int)pixelintensity > thresh)
			{
                columnvalue += pixelintensity;
                isum += i * pixelintensity;
			jsum += j * pixelintensity;
			}
        }
	
        c += (columnvalue * (i+1) * (i+1));
        d += (columnvalue * (i+1));
    }
    
    data.nSat = nSat;
    data.CleanedIntensity = cleanedIntensity;
    if(!(data.CleanedIntensity>0)) data.CleanedIntensity = -1;
    data.Intensity = totalintensity;
    a = a/cleanedIntensity;
    b = (b/cleanedIntensity) * (b/cleanedIntensity);
    data.ySpotSD = sqrt(abs(a-b));
    if(!(data.ySpotSD>0)) data.ySpotSD =-1;
    

    c = c/cleanedIntensity;
    d = (d/cleanedIntensity) * (d/cleanedIntensity);
    data.xSpotSD = sqrt(abs(c-d));
    if(!(data.xSpotSD>0)) data.xSpotSD = -1;

    data.xCentroid = (isum / cleanedIntensity);
    data.yCentroid = (jsum / cleanedIntensity);
    if(!(data.xCentroid>0)) data.xCentroid = -1;
    if(!(data.yCentroid>0)) data.yCentroid = -1;

    time_t t = time(0);
    double mjd = ((t)/86400.0)+40587.;
    data.MJD = mjd;
   
    datavec.push_back(data);
}

MPESImageData *MPESImage::dataForThresh(int thresh)
{
    for(vector<MPESImageData>::iterator it = datavec.begin(); it != datavec.end(); ++it)
    {
	if(it->cleaningThreshold == thresh)
	    return (MPESImageData *)&(*it);
    }
    analyze(thresh);
    return dataForThresh(thresh);
}

