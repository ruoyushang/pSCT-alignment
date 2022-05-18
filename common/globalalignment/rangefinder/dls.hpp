#ifndef _LIBDLS_HPP_
#define _LIBDLS_HPP_

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

class DLS {
public:
    DLS();
    ~DLS();
    int measureDistance ();

    int startTracking();
    int startTrackingDelay(int delay);
    int readTracking();
    int stopTracking();

    int readTemperature();

    int setOffset(int offset);
    int setGain(float gain);

    int laserOn();
    int laserOff();

    int saveConfiguration();

    int kbhit(void);

    int setFD(int fd);

    int setUserCalibrated (bool enabled);

    /*
     *    [a:b] = 00 Normal
     *    [a:b] = 01 Fast
     *    [a:b] = 02 Precise
     *    [a:b] = 03 Natural
     *    [a:b] = 11 Timed
     *    [a:b] = 20 Moving target characteristic with Error Freezing
     *    [a:b] = 21 Moving Target Characteristic without Error Freezing
     *
     */
    int setMeasuringCharacteristic (int a, int b);
    int setOutputFilter(int nsamples, int nspikes, int nerrors);
    int getSignalQuality();

    int help();
private:
    bool userCalibrated_;
    int fd_;

    int setInterfaceAttribs (int speed, int parity);
    void setBlocking (int should_block);
    int rxData();

    int serialRead (char *read_data);
    int serialWrite (char *write_data, int write_size);
    void printErrorMsg(int err);
};

#endif
