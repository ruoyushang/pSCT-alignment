#pragma once
#include <arv.h>
#include <cstdio>
#include <cstddef>
#include <string>
#include <vector>
#include <stdint.h>
#include <iostream>
#include <iterator>


class AravisCamera {
public:
	AravisCamera(const char* device_id = NULL);
	~AravisCamera();
	bool isReady();

	std::vector<unsigned char> captureFrame();

	// getters
	int px() { return imgWidth; }
	int py() { return imgHeight; }
	std::string getID();
	int getPayload();
	int getGain();
	double getExposure();
        double getFrameRate();
	int getPixelDepth();

	// setters
	bool setGain(int gain);
	bool setExposure(double exposure_us);
	void setGainAuto(bool useAuto = true, bool continuous = true);
	void setExposureAuto(bool useAuto = true, bool continuous = true);
	bool setFrameRate(double rate_hz);
	bool setPixelDepth(int depth);

private:
	bool cameraIsReady;
	ArvCamera* camera;
	ArvStream* stream;
	std::string cameraID;
	const int imgWidth, imgHeight, nBuffers;
	int payload, pixelDepth;
};
