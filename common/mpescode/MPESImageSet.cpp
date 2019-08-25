#include "MPESImageSet.h"
#include <opencv2/opencv.hpp> // all opencv includes
#include <cstdio>  // fprintf() etc
#include <ctime>   // time_t, time()
#include <cstdlib> // abs
#include <dirent.h> // DIR

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

#ifdef WITH_OPENCV3
#define CV_CAP_PROP_FRAME_WIDTH CAP_PROP_FRAME_WIDTH
#define CV_CAP_PROP_FRAME_HEIGHT CAP_PROP_FRAME_HEIGHT
#endif


using namespace std;
using namespace cv;

bool MPESImageSet::has_suffix(const string& s, const string& suffix)
{
    return (s.size() >= suffix.size()) && equal(suffix.rbegin(), suffix.rend(), s.rbegin());
}

MPESImageSet::MPESImageSet(const char * in_dir, int thresh, bool in_verbosity) :
    imagesToCapture(0),
    capturedImages(0),
    iThresh(thresh),
    dir(in_dir),
    verbosity(in_verbosity)
{
}

MPESImageSet::MPESImageSet(MPESDevice* in_device, int N, const char *in_dir, int thresh, bool in_verbosity) :
    device(in_device),
    imagesToCapture(N),
    capturedImages(0),
    iThresh(thresh),
    dir(in_dir),
    verbosity(in_verbosity)
{
    device->switchOn();
}

// returns the number of images captured.
// check the value to test if everything is working fine.
int MPESImageSet::Capture()
{
    // clean up the datasetvector
    datasetvec.clear();

    SetData.cleaningThreshold = iThresh;
    int reject_img = 0;

    if (!device || !device->isOn()) { // read from folder
        DIR * path = opendir(dir); 
        if(!path) {
            fprintf(stderr, "directory not found \n ");
            return capturedImages;
        }

        dirent *entry;
        while((entry = readdir(path)))
        {
            if(this->has_suffix(entry->d_name,".jpg"))
            {
                if(verbosity)
                {
                        fprintf(stderr, "%s\n",entry->d_name);
                }
                char fullpath[500];
                sprintf(fullpath,"%s/%s",dir, entry->d_name);
                MPESImage singleimage = imread(fullpath,1);
                singleimage.analyze(iThresh);
                device->SetResolution(singleimage.cols);
                datasetvec.push_back(singleimage.datavec);
            }
        }
    }
    else { // read from camera
        reject_img = 3;   //number to capture at start, but ignore from calculation
        int save_img = 2; //number to save, starting to last going backwards

        if(verbosity)
        {
            fprintf(stderr, "Device ID: %i ",device->GetID());
            fprintf(stderr, "\nDevice Resolution: %i x %0.0f ", device->GetResolution(), (device->GetResolution()*(3./4.)));
            fprintf(stderr, "\nDevice Exposure: %d" ,device->GetExposure());
            fprintf(stderr, "\nCleaning Threshold: %i \n", iThresh );
        }

        VideoCapture capture(device->GetID());

        if(!capture.isOpened())
        {
            fprintf(stderr, "Could not open camera\n");
            return capturedImages;
        }
        if (verbosity)
            fprintf(stderr,"adjusting resolution...\n");

        capture.set(CV_CAP_PROP_FRAME_WIDTH, device->GetResolution());
        capture.set(CV_CAP_PROP_FRAME_HEIGHT, device->GetResolution() *(3./4.));
        char buffer[500];
        sprintf(buffer,"v4l2-ctl -d%i --set-ctrl=white_balance_temperature_auto=0 \n", device->GetID());
        if (verbosity)
            fprintf(stderr,"%s",buffer);
        system(buffer);
        if (verbosity)
            fprintf(stderr,"adjusting exposure...\n");
        system(buffer);
        sprintf(buffer,"v4l2-ctl -d%i --set-ctrl=exposure_auto=1 \n", device->GetID());
        if (verbosity)
            fprintf(stderr, "%s",buffer);
        system(buffer);
        sprintf(buffer,"v4l2-ctl -d%i --set-ctrl=exposure_absolute=%i \n", device->GetID(), device->GetExposure());
        if (verbosity)
            fprintf(stderr,"%s",buffer);
        system(buffer);

        if (verbosity) {
            fprintf(stderr,"camera opened\n");
            fprintf(stderr, "taking picture...\n");
        }

        for(int img = 0; img < imagesToCapture + reject_img + 1 ; img++)
        {
            MPESImage capturedimage;
            capture >> capturedimage;
            time_t t = time(0);
            struct tm *now = localtime(&t);
            char imagefile[200];
            sprintf(imagefile,
                    "%s/edge_cam%i_pic%i_%04d-%02d-%02d_%02d-%02d-%02d.jpg",
                    dir, device->GetID(), img, now->tm_year+1900,
                    now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min,
                    now->tm_sec);
            if(img>reject_img)
            {
                if (img > imagesToCapture + reject_img - save_img){
                    imwrite(imagefile, capturedimage);
                }
                capturedimage.analyze(iThresh);
                datasetvec.push_back(capturedimage.datavec);
                sleep(device->GetLapse()); 
            }
        }
        capture.release();
    }

    int ignored;
    ignored = 0;
    for (vector<vector<MPESImageData> >::iterator it = datasetvec.begin(); it != datasetvec.end();)
    {
        if(!(it->at(0).CleanedIntensity>0))
        {
            it=datasetvec.erase(it);
            ignored++;
        }
        else
            it++;
    }
    if(ignored)
    {
        fprintf(stderr,"%i images ignored because of unphysical results.",ignored);
        if(ignored == imagesToCapture) 
            fprintf(stderr," \n All of your images were ignored, check your device!! \nMaybe the laser is not bright enough, or pointed away from the FOV.\n");
    }
    
    if(verbosity) this->ImageProperties(stdout);
                
    m_Calibrated = 0;

    // makeArrays updates capturedImages
    makeArrays();
    return capturedImages;

}

void MPESImageSet::ImageProperties(FILE * file)
{
    for(vector<vector<MPESImageData> >::iterator it = datasetvec.begin(); it != datasetvec.end(); ++it)
    {
	fprintf(file, "%i %f %e %e %f %f %f %f %i\n", device->GetID(), it->at(0).MJD, it->at(0).Intensity, it->at(0).CleanedIntensity, it->at(0).xCentroid, it->at(0).yCentroid, it->at(0).xSpotSD, it->at(0).ySpotSD, it->at(0).nSat);
    }
}

void MPESImageSet::makeArrays()
{
    capturedImages = datasetvec.size();

    if(capturedImages)
    {
		for(int i=0; i<capturedImages; i++)
		{
			x[i] = datasetvec[i].at(0).xCentroid;
			y[i] = datasetvec[i].at(0).yCentroid;
			cIntensity[i] = datasetvec[i].at(0).CleanedIntensity;
			tIntensity[i] = datasetvec[i].at(0).Intensity;
			xwidth[i] = datasetvec[i].at(0).xSpotSD;
			ywidth[i] = datasetvec[i].at(0).ySpotSD;
			nsat[i] = datasetvec[i].at(0).nSat;
		}
	}
	else
	{
		SetData.CleanedIntensity = 0;
		SetData.CleanedIntensitySD = 0;
		SetData.Intensity = 0;
		SetData.IntensitySD = 0;
		SetData.xCentroid = 0;
		SetData.yCentroid = 0;
		SetData.xCentroidSD = 0;
		SetData.yCentroidSD = 0 ;
		SetData.xSpotSD = 0;
		SetData.ySpotSD = 0;
		SetData.cleaningThreshold = iThresh;
	}
}

void MPESImageSet::simpleAverage()
{
    SetData.CleanedIntensity = 0;
    SetData.CleanedIntensitySD = 0;
    SetData.Intensity = 0;
    SetData.IntensitySD = 0;
    SetData.xCentroid = 0;
    SetData.yCentroid = 0;
    SetData.xCentroidSD = 0;
    SetData.yCentroidSD = 0;
    SetData.xSpotSD = 0;
    SetData.ySpotSD = 0;
    SetData.nSat = 0;
    SetData.cleaningThreshold = iThresh;
    if(capturedImages)
    {
		for(int i=0;i<capturedImages; i++)
		{
			SetData.CleanedIntensity += cIntensity[i];
			SetData.Intensity += tIntensity[i];
			SetData.xCentroid += x[i];
			SetData.yCentroid += y[i];
			SetData.xSpotSD += xwidth[i]*xwidth[i];
			SetData.ySpotSD += ywidth[i]*ywidth[i];
			SetData.nSat += nsat[i];
		}
		SetData.CleanedIntensity = SetData.CleanedIntensity/capturedImages;
		SetData.Intensity = SetData.Intensity/capturedImages;
		SetData.xCentroid = SetData.xCentroid/capturedImages;
		SetData.yCentroid = SetData.yCentroid/capturedImages;
		SetData.nSat = SetData.nSat/capturedImages;
		
		for(int i=0;i<capturedImages; i++)
		{
			SetData.CleanedIntensitySD += (cIntensity[i]-SetData.CleanedIntensity)*(cIntensity[i]-SetData.CleanedIntensity);
			SetData.IntensitySD += (tIntensity[i]-SetData.Intensity)*(tIntensity[i]-SetData.Intensity);
			SetData.xCentroidSD += (x[i]-SetData.xCentroid)*(x[i]-SetData.xCentroid);
			SetData.yCentroidSD += (y[i]-SetData.yCentroid)*(y[i]-SetData.yCentroid);
		}
		SetData.CleanedIntensitySD = sqrt(abs(SetData.CleanedIntensitySD)/capturedImages);   
		SetData.IntensitySD = sqrt(abs(SetData.IntensitySD)/capturedImages);
		SetData.xCentroidSD = sqrt(abs(SetData.xCentroidSD)/capturedImages);
		SetData.yCentroidSD = sqrt(abs(SetData.yCentroidSD)/capturedImages);
		SetData.xSpotSD = sqrt(SetData.xSpotSD/capturedImages);
		SetData.ySpotSD = sqrt(SetData.ySpotSD/capturedImages);
		SetData.Xcal = 0.0;
		SetData.Ycal = 0.0;    
		time_t t = time(0);
		double mjd = ((t)/86400.0)+40587.;
		SetData.MJD = mjd;
		//SetData.ID = device->ID ;
		printSetProperties(stderr);
	}
	else
	{
		SetData.CleanedIntensity = -1;
		SetData.CleanedIntensitySD = -1;
		SetData.Intensity = -1;
		SetData.IntensitySD = -1;
		SetData.xCentroid = -1;
		SetData.yCentroid = -1;
		SetData.xCentroidSD = -1;
		SetData.yCentroidSD = -1;
		SetData.xSpotSD = -1;
		SetData.ySpotSD = -1;
		SetData.nSat = -1;
		SetData.cleaningThreshold = iThresh;
	}
}

void MPESImageSet::printSetProperties(FILE * file) { /**
    fprintf(stderr, "Device ID: %i \n",device->GetID());
    fprintf(stderr, "Width: %i \n",device->GetResolution());
    fprintf(stderr, "Cleaning Threshold: %i \n" ,iThresh);
    fprintf(stderr, "Time (MJD): %f\n", SetData.MJD);
    fprintf(stderr, "%i images captured\n", capturedImages); 
    fprintf(file, "%e %e %e %e %f %f %f %f %f %f %f %f %i\n", SetData.Intensity, SetData.IntensitySD, SetData.CleanedIntensity, SetData.CleanedIntensitySD, SetData.xCentroid, SetData.yCentroid, SetData.xCentroidSD, SetData.yCentroidSD, SetData.xSpotSD, SetData.ySpotSD, SetData.Xcal, SetData.Ycal, SetData.nSat);
*/
    spdlog::trace("\n"
                  "Device ID: {}\n"
                  "Width: {} \n"
                  "Cleaning Threshold: {} \n"
                  "Time (MJD): {}\n"
                  "{} images captured\n"
                  "{} {} {} {} {} {} {} {} {} {} {} {} {}\n",
                  device->GetID(),
                  device->GetResolution(),
                  iThresh,
                  SetData.MJD,
                  capturedImages,
                  SetData.Intensity, SetData.IntensitySD, SetData.CleanedIntensity, SetData.CleanedIntensitySD,
                  SetData.xCentroid, SetData.yCentroid, SetData.xCentroidSD, SetData.yCentroidSD, SetData.xSpotSD,
                  SetData.ySpotSD, SetData.Xcal, SetData.Ycal, SetData.nSat);
}


void MPESImageSet::Calibrate2D()
{
    if(!device->CalibrationLoaded()) fprintf(stderr, "\n Constants not loaded. Must load calibration constants first!\n");
    else
    {
        if(!m_Calibrated)
        {
            int CalLen = device->GetCalibration().calLen;
            float *XfromMat = device->GetCalibration().XfromMat;
            float *YfromMat = device->GetCalibration().YfromMat;
            float *xcalfromMat = device->GetCalibration().xcalfromMat;
            float *ycalfromMat = device->GetCalibration().ycalfromMat;

            fprintf(stderr,"Not calibrated, calibrating now...\n");
            int first,second,third,fourth;
            first=second=third=fourth=0;
            float distance,distance1,distance2,distance3;
            distance1=distance2=distance3 =500000.0;
            // Found top 3 closest calibration constants in grid
            for(int i=0; i<CalLen;i++)
            {
                distance = sqrt((SetData.xCentroid-(XfromMat[i]))*(SetData.xCentroid-(XfromMat[i]))+(SetData.yCentroid-(YfromMat[i]))*(SetData.yCentroid-(YfromMat[i])));
                if(distance< distance1)
                {
                    third = second;
                    second= first;
                    first = i;
                    distance3=distance2;
                    distance2=distance1;
                    distance1=distance;					
                }
                else if (distance< distance2)
                {
                    third = second;
                    second = i;
                    distance3 = distance2;
                    distance2 = distance;
                }
                else if( distance<distance3)
                {
                    third = i;
                    distance3 =distance;
                }
            }
                //measure distance for each point to centroid and find top 3 closest points - set index for them
                //use index from each to plug into plane equation
                //solve for z value in x
                //solve for z value in y
                
            float xplane_i, xplane_j, xplane_k,xplane_d, yplane_i, yplane_j, yplane_k,yplane_d;
            xplane_i = ((YfromMat[second]-YfromMat[first])*(xcalfromMat[third]-xcalfromMat[first]))-((xcalfromMat[second]-xcalfromMat[first])*(YfromMat[third]-YfromMat[first]));
            xplane_j = ((XfromMat[second]-XfromMat[first])*(xcalfromMat[third]-xcalfromMat[first]))-((xcalfromMat[second]-xcalfromMat[first])*(XfromMat[third]-XfromMat[first]));
            xplane_k = ((XfromMat[second]-XfromMat[first])*(YfromMat[third]-YfromMat[first]))-((YfromMat[second]-YfromMat[first])*(XfromMat[third]-XfromMat[first]));
            xplane_d = -(xplane_i*XfromMat[first] + xplane_j*YfromMat[first] +xplane_k*xcalfromMat[first]);
            yplane_i = ((YfromMat[second]-YfromMat[first])*(ycalfromMat[third]-ycalfromMat[first]))-((ycalfromMat[second]-ycalfromMat[first])*(YfromMat[third]-YfromMat[first]));
            yplane_j = ((XfromMat[second]-XfromMat[first])*(ycalfromMat[third]-ycalfromMat[first]))-((ycalfromMat[second]-ycalfromMat[first])*(XfromMat[third]-XfromMat[first]));
            yplane_k = ((XfromMat[second]-XfromMat[first])*(YfromMat[third]-YfromMat[first]))-((YfromMat[second]-YfromMat[first])*(XfromMat[third]-XfromMat[first]));
            yplane_d = -(yplane_i*XfromMat[first] + yplane_j*YfromMat[first] +yplane_k*ycalfromMat[first]);
            float CalConst_x,CalConst_y;
            CalConst_x = -(xplane_i*SetData.xCentroid +xplane_j*SetData.yCentroid + xplane_d)/xplane_k;
            CalConst_y = -(yplane_i*SetData.xCentroid +yplane_j*SetData.yCentroid + yplane_d)/yplane_k;
            printf("\nConstants from Residuals: %f, %f \n", CalConst_x,CalConst_y);
            
            if(((SetData.xCentroid + CalConst_x)>device->GetResolution())|((SetData.xCentroid + CalConst_x)<0))
            {
                fprintf(stderr,"\nError: calibration constants would take centroid out of FOV");
                CalConst_x = 0;
            } 
            else
                    SetData.xCentroid = SetData.xCentroid + CalConst_x;
            
            if(((SetData.yCentroid + CalConst_y)>(device->GetResolution()*0.75))|((SetData.yCentroid + CalConst_y)<0))
            {
                fprintf(stderr,"\nError: calibration constants would take centroid out of FOV");
                CalConst_y = 0;
            } 
            else
                    SetData.yCentroid = SetData.yCentroid + CalConst_y;
    
            
            SetData.Xcal = CalConst_x + SetData.Xcal;
            SetData.Ycal = CalConst_y + SetData.Ycal;
            if(verbosity) printf("Total Calibration Constants so far : [%f, %f] \n",SetData.Xcal, SetData.Ycal);
            m_Calibrated = 1;
        }
        else
            fprintf(stderr,"Object already calibrated \n");
    }
}

void MPESImageSet::Calibrate()
{	
	if(!device->CalibrationLoaded()) fprintf(stderr, "\n Constants not loaded. Must load calibration constants first!\n");
	else
	{
		if(!m_Calibrated)
		{
                        int CalLen = device->GetCalibration().calLen;
                        float *XfromMat = device->GetCalibration().XfromMat;
                        float *YfromMat = device->GetCalibration().YfromMat;
                        float *xcalfromMat = device->GetCalibration().xcalfromMat;
                        float *ycalfromMat = device->GetCalibration().ycalfromMat;

			fprintf(stderr,"Not calibrated, calibrating now...");
			float xcalposmin, xcalposmax, ycalposmin, ycalposmax, xcalmin, xcalmax, ycalmin, ycalmax;
            xcalposmin=xcalposmax=ycalposmin=ycalposmax=xcalmin=xcalmax=ycalmin= ycalmax =0;
			float intervalx= 0.0, intervaly=0.0;
			for(int r=0;r<CalLen;r++)
			{
				if(r==0)
				{ 
					intervalx = 1000;
					intervaly = 1000;
				}
				if(abs(SetData.xCentroid-XfromMat[r]) < intervalx)
				{
					intervalx = abs(SetData.xCentroid-XfromMat[r]);
					xcalposmin = XfromMat[r];
					xcalposmax = XfromMat[r+1];
					xcalmin = xcalfromMat[r];
					xcalmax = xcalfromMat[r+1];			
				}

				if(abs(SetData.yCentroid-YfromMat[r]) < intervaly)
				{
					intervaly = abs(SetData.yCentroid-YfromMat[r]);
					ycalposmin = YfromMat[r];
					ycalposmax = YfromMat[r+1];
					ycalmin = ycalfromMat[r];
					ycalmax = ycalfromMat[r+1];
				}
			}

			if(verbosity) printf("Resolution: %d x %.0f \n", device->GetResolution(), (device->GetResolution()*(0.75)));
            if (verbosity)
                printf("position ranges x: [%f - %f] y: [%f - %f] \n", xcalposmin, xcalposmax, ycalposmin, ycalposmax);
			if(verbosity) printf("Constant ranges x: [%f - %f] y: [%f - %f] \n", xcalmin, xcalmax, ycalmin,ycalmax);
			float slopex,slopey, Bx, By;
			slopex = ((xcalmax-xcalmin))/((xcalposmax-xcalposmin));
			Bx = xcalmin-(slopex*xcalposmin);
			slopey = ((ycalmax-ycalmin))/((ycalposmax-ycalposmin));
			By = ycalmin-(slopey*ycalposmin);
			
//			float Xcal, Ycal;
			SetData.Xcal = slopex * SetData.xCentroid + Bx ;
			SetData.Ycal = slopey * SetData.yCentroid + By ;
			
			if(abs(SetData.Xcal )> device->GetResolution()) SetData.Xcal = 0.0;
			if(SetData.Xcal != SetData.Xcal) SetData.Xcal = 0.0;
			if(abs(SetData.Ycal) > device->GetResolution()) SetData.Ycal = 0.0;
			if(SetData.Ycal != SetData.Ycal) SetData.Ycal = 0.0;
			
			if(verbosity) printf("Calibration Constants: [%f, %f] \n",SetData.Xcal, SetData.Ycal);
			SetData.xCentroid = SetData.xCentroid + SetData.Xcal;
			SetData.yCentroid = SetData.yCentroid + SetData.Ycal;
			m_Calibrated = 1;
		}
		else
		{
			fprintf(stderr,"Object already calibrated \n");
		}
	}
}

void MPESImageSet::Matrix_Transform()
{
    if(!device->MatrixLoaded()) fprintf(stderr, "\n Constants not loaded. Must load Matrix Rotation constants first!\n");
    else
    {
        float Angle = device->GetCalibration().Angle;
        float Tx = device->GetCalibration().Tx; 
        float Ty = device->GetCalibration().Ty; 
        float Tz = device->GetCalibration().Tz; 

        if(verbosity) fprintf(stdout,"Rotating...\n");
        float Transformed_X,Transformed_Y;
        Transformed_X=(this->SetData.xCentroid*cos(Angle)-this->SetData.yCentroid*sin(Angle)) - Tx; 
        Transformed_Y=(this->SetData.xCentroid*sin(Angle)+this->SetData.yCentroid*cos(Angle)) - Ty;
        this->SetData.Xcal = this->SetData.Xcal+(Transformed_X-(this->SetData.xCentroid));
        this->SetData.Ycal = this->SetData.Ycal+(Transformed_Y-(this->SetData.yCentroid));
        this->SetData.xCentroid=this->SetData.xCentroid+this->SetData.Xcal;
        this->SetData.yCentroid=this->SetData.yCentroid+this->SetData.Ycal;
        if(verbosity) printf("Rotation Constants: [%f, %f] \n",SetData.Xcal, SetData.Ycal);
    }
}
