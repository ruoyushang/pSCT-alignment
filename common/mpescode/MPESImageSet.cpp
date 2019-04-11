#include "MPESImageSet.h"
#include <opencv2/opencv.hpp> // all opencv includes
#include <cstdio>  // fprintf() etc
#include <ctime>   // time_t, time()
#include <cstdlib> // abs
#include <dirent.h> // DIR
#include <TLine.h>
#include <TF1.h>
#include <TText.h>
#include <TCanvas.h>
#include <TStyle.h> // gStyle
#include <TDirectory.h> // gDirectory
#include <TGraph.h>
#include <TH2F.h>

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
        reject_img = 10;

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

        for(int img = 0; img < imagesToCapture + reject_img +1 ; img++)
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
                imwrite(imagefile, capturedimage);
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

void MPESImageSet::printSetProperties(FILE * file)
{
    fprintf(stderr, "Device ID: %i \n",device->GetID());
    fprintf(stderr, "Width: %i \n",device->GetResolution());
    fprintf(stderr, "Cleaning Threshold: %i \n" ,iThresh);
    fprintf(stderr, "Time (MJD): %f\n", SetData.MJD);
    fprintf(stderr, "%i images captured\n", capturedImages); 
    fprintf(file, "%e %e %e %e %f %f %f %f %f %f %f %f %i\n", SetData.Intensity, SetData.IntensitySD, SetData.CleanedIntensity, SetData.CleanedIntensitySD, SetData.xCentroid, SetData.yCentroid, SetData.xCentroidSD, SetData.yCentroidSD, SetData.xSpotSD, SetData.ySpotSD, SetData.Xcal, SetData.Ycal, SetData.nSat);
}

void MPESImageSet::fitGauss()
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
    if(capturedImages)
    {
        delete gDirectory->FindObject("hgrey");
        delete gDirectory->FindObject("hgrey_px");
        delete gDirectory->FindObject("hgrey_py");

        Float_t width =  device->GetResolution();
        Float_t length = device->GetResolution()*(3./4.);
        Int_t binning = 20;
        Int_t bwidth = width*binning;
        Int_t blength = length*binning;
        Bool_t verbosity = kFALSE;
        
        TH2F *hgrey = new TH2F("hgrey","Centroids",bwidth,0,width,blength,0,length);
        
        TGraph *ggreyint = new TGraph();
        ggreyint->SetTitle("Intensity");
        ggreyint->GetXaxis()->SetTitle("Frame #");
        TGraph *ggreycint = new TGraph();
        ggreycint->SetTitle("Cleaned Intensity");
        ggreycint->GetXaxis()->SetTitle("Frame #");
        TGraph *ggreycorrx = new TGraph();
        ggreycorrx->SetTitle("X-centroid vs Intensity");
        TGraph *ggreycorry = new TGraph();
        ggreycorry->SetTitle("Y-centroid vs Intensity");
        
        TGraph * ggreyx = new TGraph(); 
        ggreyx->SetTitle("X-centroid drift");
        ggreyx->GetXaxis()->SetTitle("Frame #");
        TGraph * ggreyy = new TGraph();
        ggreyy->SetTitle("Y-centroid drift"); 
        
        for(int m=0;m<capturedImages;m++)
        {
            hgrey->Fill(x[m],y[m]);
            ggreyx->SetPoint(m,m,x[m]);
            ggreyy->SetPoint(m,m,y[m]);
            ggreyint->SetPoint(m,m,tIntensity[m]);
            ggreycint->SetPoint(m,m,cIntensity[m]);
            ggreycorrx->SetPoint(m,cIntensity[m],x[m]);
            ggreycorry->SetPoint(m,cIntensity[m],y[m]);
        }
        Int_t xmaxg, ymaxg, zmaxg;
        Int_t zoom = binning;
        Float_t zoomRMS = 20;
        Float_t rxming, rxmaxg, ryming, rymaxg;
        hgrey->GetMaximumBin(xmaxg, ymaxg, zmaxg);
        rxming = hgrey->GetXaxis()->GetBinCenter(xmaxg-zoom);
        if (xmaxg-zoom < 0)
         rxming = hgrey->GetXaxis()->GetBinCenter(0);
        rxmaxg = hgrey->GetXaxis()->GetBinCenter(xmaxg+zoom);
        if (xmaxg+zoom >= bwidth)
         rxmaxg = hgrey->GetXaxis()->GetBinCenter(bwidth-1);
        ryming = hgrey->GetYaxis()->GetBinCenter(ymaxg-zoom);
        if (ymaxg-zoom < 0)
         ryming = hgrey->GetYaxis()->GetBinCenter(0);
        rymaxg = hgrey->GetYaxis()->GetBinCenter(ymaxg+zoom);
        if (ymaxg+zoom >= blength)
         rymaxg = hgrey->GetYaxis()->GetBinCenter(blength-1);
        hgrey->GetXaxis()->SetRangeUser(rxming, rxmaxg);
        hgrey->GetYaxis()->SetRangeUser(ryming, rymaxg);
        ggreyx->GetYaxis()->SetRangeUser(rxming, rxmaxg);
        ggreyy->GetYaxis()->SetRangeUser(ryming, rymaxg);
        
        hgrey->GetXaxis()->SetTitle("X pixel");
        hgrey->GetYaxis()->SetTitle("Y pixel");
        ggreyx->GetYaxis()->SetTitle("X pixel");
        ggreyy->GetYaxis()->SetTitle("Y pixel");
        ggreyx->GetXaxis()->SetTitle("Frame no.");
        ggreyy->GetXaxis()->SetTitle("Frame no.");
        ggreyint->GetXaxis()->SetTitle("Frame no.");
        ggreyint->GetYaxis()->SetTitle("Total intensity");
        ggreycint->GetXaxis()->SetTitle("Frame no.");
        ggreycint->GetYaxis()->SetTitle("Total Cleaned intensity");
        ggreycorrx->GetXaxis()->SetTitle("Intensity");
        ggreycorrx->GetYaxis()->SetTitle("X Pixel");
        ggreycorry->GetXaxis()->SetTitle("Intensity");
        ggreycorry->GetYaxis()->SetTitle("Y Pixel");
        
        gStyle->SetPalette(1);
        gStyle->SetOptFit();
        
        ////////////////////////////////////////////////////////////////////////////
        // FULL RANGE.
        ////////////////////////////////////////////////////////////////////////////
        fprintf(stderr,"\nFitting to gaussian...");
        float full_i=0, full_ic=0, full_si=0, full_sic=0, full_x=0, full_sx=0, full_y=0, full_sy=0, full_sxspot = 0, full_syspot = 0;
        
        TCanvas *c1 = new TCanvas("c1","Full",1500,600);
        c1->Divide(5,2);
        c1->cd(1);
        hgrey->Draw("surf2");
        
        c1->cd(6);
        c1->cd(6)->SetGrid();
        hgrey->Draw("COLZ");
        
        c1->cd(2);
        TH1D* hgrey_px = hgrey->ProjectionX();
        hgrey_px->Draw();
        hgrey_px->Fit("gaus","Q");
        TF1 *gausgreyx = hgrey_px->GetFunction("gaus");
        TText textgrayx;
        Double_t xgreymean = gausgreyx->GetParameter(1);
        textgrayx.DrawTextNDC(0.5,0.5,Form("Mean = %.3f", (Double_t)xgreymean));
        full_x = xgreymean;
        full_sx = gausgreyx->GetParameter(2);
        
        c1->cd(7);
        TH1D* hgrey_py = hgrey->ProjectionY();
        hgrey_py->Draw();
        hgrey_py->Fit("gaus", "Q");
        TF1 *gausgreyy = hgrey_py->GetFunction("gaus");
        TText textgrayy;
        Double_t ygreymean = gausgreyy->GetParameter(1);
        textgrayy.DrawTextNDC(0.5,0.5,Form("Mean = %.3f", (Double_t)ygreymean));
        full_y = ygreymean;
        full_sy = gausgreyy->GetParameter(2);
        
        c1->cd(3);
        ggreyx->Draw("AP");
        TLine *lgreyx = new TLine(1,gausgreyx->GetParameter(1),capturedImages,gausgreyx->GetParameter(1));
        lgreyx->SetLineColor(kRed);
        lgreyx->Draw("same");
        
        c1->cd(8);
        ggreyy->Draw("AP");
        TLine *lgreyy = new TLine(1,gausgreyy->GetParameter(1),capturedImages,gausgreyy->GetParameter(1));
        lgreyy->SetLineColor(kRed);
        lgreyy->Draw("same");
        
        c1->cd(5);
        ggreyint->Draw("AP");
        TLine *lgreyint = new TLine(1,ggreyint->GetMean(2),capturedImages,ggreyint->GetMean(2));
        ggreyint->GetYaxis()->SetRangeUser(ggreyint->GetMean(2)-zoomRMS*ggreyint->GetRMS(2),ggreyint->GetMean(2)+zoomRMS*ggreyint->GetRMS(2));
        lgreyint->SetLineColor(kRed);
        lgreyint->Draw("same");
        full_i = ggreyint->GetMean(2);
        full_si = ggreyint->GetRMS(2);
        
        c1->cd(10);
        ggreycint->Draw("AP");
        TLine *lgreycint = new TLine(1,ggreycint->GetMean(2),capturedImages,ggreycint->GetMean(2));
        ggreycint->GetYaxis()->SetRangeUser(ggreycint->GetMean(2)-zoomRMS*ggreycint->GetRMS(2),ggreycint->GetMean(2)+zoomRMS*ggreycint->GetRMS(2));
        lgreycint->SetLineColor(kRed);
        lgreycint->Draw("same");
        full_ic = ggreycint->GetMean(2);
        full_sic = ggreycint->GetRMS(2);
        
        c1->cd(4);
        ggreycorrx->Draw("AP");
        ggreycorrx->GetYaxis()->SetRangeUser(ggreycorrx->GetMean(2)-zoomRMS*ggreycorrx->GetRMS(2),ggreycorrx->GetMean(2)+zoomRMS*ggreycorrx->GetRMS(2));
        ggreycorrx->GetXaxis()->SetRangeUser(ggreycorrx->GetMean(1)-zoomRMS*ggreycorrx->GetRMS(1),ggreycorrx->GetMean(1)+zoomRMS*ggreycorrx->GetRMS(1));
        
        c1->cd(9);
        ggreycorry->Draw("AP");
        ggreycorry->GetYaxis()->SetRangeUser(ggreycorry->GetMean(2)-zoomRMS*ggreycorry->GetRMS(2),ggreycorry->GetMean(2)+zoomRMS*ggreycorry->GetRMS(2));
        ggreycorry->GetXaxis()->SetRangeUser(ggreycorry->GetMean(1)-zoomRMS*ggreycorry->GetRMS(1),ggreycorry->GetMean(1)+zoomRMS*ggreycorry->GetRMS(1));
        
        float sumx=0;
        float sumy=0;
        for(int m=0;m<capturedImages;m++)
        {
            sumx += xwidth[m]*xwidth[m];
            sumy += ywidth[m]*ywidth[m];
        }
        full_sxspot = sqrt(sumx/capturedImages);
        full_syspot = sqrt(sumy/capturedImages); 
        
        time_t t = time(0);
        double mjd = ((t)/86400.0)+40587.;
        SetData.MJD = mjd;
        //SetData.ID = device->ID ;
        SetData.cleaningThreshold = iThresh;
        SetData.Intensity = full_i ;
        SetData.IntensitySD = full_si;
        SetData.CleanedIntensity = full_ic;
        SetData.CleanedIntensitySD = full_sic; 
        SetData.xCentroid = full_x ;
        SetData.xCentroidSD = full_sx;
        SetData.yCentroid = full_y;
        SetData.yCentroidSD = full_sy;
        SetData.xSpotSD = full_sxspot;
        SetData.ySpotSD = full_syspot;
        SetData.Xcal = 0.0;
        SetData.Ycal = 0.0;
        
        printSetProperties(stderr);
        
        if (verbosity)
        {
            char buffer[300];
            sprintf(buffer,"%s/outfile.png", dir);
            c1->SaveAs(buffer);
            sprintf(buffer,"%s/outfile.root", dir);
            c1->SaveAs(buffer);
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
