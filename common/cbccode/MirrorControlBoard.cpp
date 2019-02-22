/*
 *  MirrorControlBoard.cpp - Class which implements useful mid-level
 *  functionality for the mirror control board.
 */

#include <cassert>
#include <MirrorControlBoard.hpp>
#include <GPIOInterface.hpp>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <cstdlib>
#include <pthread.h>
#include <stdio.h>
#include <iostream>
#include <chrono>

// local includes
#include <SpiInterface.hpp>
#include <TLC3548_ADC.hpp>
#include <GPIOInterface.hpp>
#include <mcspiInterface.hpp>
#include <Layout.hpp>

GPIOInterface gpio;
mcspiInterface spi;

namespace MirrorControlBoard
{
    void enableIO ()
    {
        gpio.WriteLevel(Layout::igpioEN_IO, 1);
    }

    void disableIO ()
    {
        gpio.WriteLevel(Layout::igpioEN_IO, 0);
    }

    void adcSleep (int iadc)
    {
        // Set on-board ADC into sleep mode
        selectADC(iadc);
        //spi.Configure();
        spi.WriteRead(TLC3548::codeSWPowerDown());

        selectADC(iadc);
        //spi.Configure();
        spi.WriteRead(TLC3548::codeSWPowerDown());
    }

    void powerDownUSB(unsigned iusb)
    {
        gpio.WriteLevel(Layout::igpioUSBOff(iusb),1);
    }

    void powerUpUSB(unsigned iusb)
    {
        gpio.WriteLevel(Layout::igpioUSBOff(iusb),0);
    }

    bool isUSBPoweredUp(unsigned iusb)
    {
        return gpio.ReadLevel(Layout::igpioUSBOff(iusb))?false:true;
    }

    void powerDownDriveControllers()
    {
        gpio.WriteLevel(Layout::igpioSleep,0);
    }

    void powerUpDriveControllers()
    {
        gpio.WriteLevel(Layout::igpioSleep,1);
    }

    bool isDriveControllersPoweredUp()
    {
        return gpio.ReadLevel(Layout::igpioSleep)?true:false;
    }

    void powerDownEncoders()
    {
        gpio.WriteLevel(Layout::igpioEncoderEnable,0);
    }

    void powerUpEncoders()
    {
        gpio.WriteLevel(Layout::igpioEncoderEnable,1);
    }

    bool isEncodersPoweredUp()
    {
        return gpio.ReadLevel(Layout::igpioEncoderEnable)?true:false;
    }

    void powerUpSensors()
    {
        gpio.WriteLevel(Layout::igpioPowerADC,1);
    }

    void powerDownSensors()
    {
        gpio.WriteLevel(Layout::igpioPowerADC,0);
    }

    bool isSensorsPoweredUp()
    {
        return gpio.ReadLevel(Layout::igpioPowerADC)?true:false;
    }

    void enableDriveSR(bool enable)
    {
        gpio.WriteLevel(Layout::igpioSR, enable?0:1);
    }


    void disableDriveSR()
    {
        enableDriveSR(false);
    }

    bool isDriveSREnabled()
    {
        return gpio.ReadLevel(Layout::igpioSR)?false:true;
    }

    void setUStep(UStep ustep)
    {
        unsigned mslog2 = 0;
        switch(ustep) {
            case USTEP_1:
                mslog2 = 0x0;
                break;
            case USTEP_2:
                mslog2 = 0x1;
                break;
            case USTEP_4:
                mslog2 = 0x2;
                break;
            case USTEP_8:
                mslog2 = 0x3;
                break;
        }
        gpio.WriteLevel(Layout::igpioMS1, mslog2 & 0x1);
        gpio.WriteLevel(Layout::igpioMS2, mslog2 & 0x2);
    }

    UStep getUStep()
    {
        if(gpio.ReadLevel(Layout::igpioMS2))
            return gpio.ReadLevel(Layout::igpioMS1)?USTEP_8:USTEP_4;
        else
            return gpio.ReadLevel(Layout::igpioMS1)?USTEP_2:USTEP_1;
    }

    void  stepOneDrive(unsigned idrive, Dir dir, unsigned frequency)
    {
        /* Give this thread higher priority to improve timing stability */
        pthread_t this_thread = pthread_self();
        struct sched_param params;
        params.sched_priority = sched_get_priority_max(SCHED_FIFO);
        pthread_setschedparam(this_thread, SCHED_FIFO, &params);

        /* Write Direction to the DIR pin */
        gpio.WriteLevel(Layout::igpioDir(idrive),(dir==DIR_RETRACT)?1:0);

        /* Writes one step to STEP pin */
        unsigned igpio = Layout::igpioStep(idrive);
        gpio.WriteLevel(igpio,(dir==DIR_NONE)?0:1);

        /* a delay */
        waitHalfPeriod(frequency);

        /* Toggle pin back to low */
        gpio.WriteLevel(igpio,0);

        /* a delay */
        waitHalfPeriod(frequency);
        sched_yield();
    }

    void setPhaseZeroOnAllDrives()
    {
        gpio.WriteLevel(Layout::igpioReset,0);
        waitHalfPeriod(400);
        gpio.WriteLevel(Layout::igpioReset,1);
    }

    void enableDrive(unsigned idrive, bool enable)
    {
        gpio.WriteLevel(Layout::igpioEnable(idrive), enable?0:1);
    }

    void disableDrive(unsigned idrive)
    {
        enableDrive(idrive, false);
    }

    bool isDriveEnabled(unsigned idrive)
    {
        return gpio.ReadLevel(Layout::igpioEnable(idrive))?false:true;
    }

    void enableDriveHiCurrent(bool enable)
    {
        gpio.WriteLevel(Layout::igpioPwrIncBar, enable?0:1);
    }

    void disableDriveHiCurrent()
    {
        enableDriveHiCurrent(false);
    }


    bool isDriveHiCurrentEnabled()
    {
        return gpio.ReadLevel(Layout::igpioPwrIncBar)?false:true;
    }

    //------------------------------------------------------------------------------
    // ADCs
    //------------------------------------------------------------------------------

    void initializeADC(unsigned iadc)
    {
        selectADC(iadc);                                        // Assert Chip Select for ADC in question
        spi.WriteRead(TLC3548::codeInitialize());
        spi.WriteRead(TLC3548::codeConfig());
    }

    void selectADC(unsigned iadc)
    {
        gpio.WriteLevel(Layout::igpioADCSel1, iadc==0?1:0);
        gpio.WriteLevel(Layout::igpioADCSel2, iadc==1?1:0);
    }

    uint32_t measureADC(unsigned iadc, unsigned ichan)
    {

        initializeADC(iadc);

        // Assert Chip Select
        selectADC(iadc);

        // ADC Channel Select
        uint32_t code = TLC3548::codeSelect(ichan);
        spi.WriteRead(code);

        // Read ADC
        uint32_t datum = spi.WriteRead(TLC3548::codeReadFIFO());

        return TLC3548::decodeUSB(datum);
    }

    void measureADCStat(unsigned iadc, unsigned ichan, unsigned nmeas, uint32_t& sum, uint64_t& sumsq, uint32_t& min, uint32_t& max, unsigned ndelay)
    {
        //spi.Configure();
        initializeADC(iadc);
        selectADC(iadc);
        uint32_t code   = TLC3548::codeSelect(ichan);
        unsigned nburn  = 1;
        unsigned nloop  = nburn + nmeas;
        sum             = 0;
        sumsq           = 0;
        max             = 0;
        min             = ~max;

        uint32_t measurement [nmeas];

        /* Increase Thread Priority */
        pthread_t this_thread = pthread_self();
        struct sched_param params;
        params.sched_priority = sched_get_priority_max(SCHED_FIFO);
        pthread_setschedparam(this_thread, SCHED_FIFO, &params);

        uint32_t datum;

        /* Loop over number of measurements */
        for(unsigned iloop=0; iloop < nloop; iloop++) {
            // Read data
            datum = spi.WriteRead(code);
            /* Decode data and accumulate statistics*/
            if (iloop >= nburn) {
                datum = TLC3548::decodeUSB(datum);
                measurement[iloop-nburn] = datum;
                if(datum>max) max=datum;
                if(datum<min) min=datum;

                sum  +=datum;
                sumsq+=static_cast<uint64_t>(datum) * static_cast<uint64_t>(datum);
            }
            for (volatile unsigned i=0; i<ndelay; i++);
        }

        /* Read last FIFO, Clear Buffer */
        datum = spi.WriteRead(TLC3548::codeReadFIFO());

        float voltage_range = TLC3548::voltData((max-min));

        bool at_home;
        if (voltage_range>1.) at_home = true;
        else                  at_home = false;

        int cnt_high=0;
        int cnt_low =0;
        uint32_t encoder_midpoint = 5734; /* 1.75 volts */

        /* Determine whether the high side or low side has more measurements
         * for the case that the encoder is at 0 degrees */
        if (at_home) {

            for (unsigned iloop=0; iloop < nmeas; iloop++) {
                if (measurement[iloop] > encoder_midpoint)
                    cnt_high++;
                else if (measurement[iloop] <= encoder_midpoint)
                    cnt_low++;
            }

            bool meas_high;
            if   (cnt_high>cnt_low) meas_high = true;
            else                    meas_high = false;

            int nmeas_used=0;

            max   = 0;
            min   = ~max;
            sum   = 0;
            sumsq = 0;

            /* Loop over the data again and accumulate statistics only in the case that the data meets criteria,
             * viz that if there are more low than high measurements, we only accept the low
             * and if there are more high than low measurements, we accept only the high */
            for (unsigned iloop=0; iloop<nmeas; iloop++) {
                datum = measurement[iloop];
                if ((datum>encoder_midpoint && meas_high) || (datum<encoder_midpoint && !meas_high)) {
                    sum  +=datum;
                    sumsq+=static_cast<uint64_t>(datum) * static_cast<uint64_t>(datum);

                    nmeas_used++;

                    if(datum>max) max=datum;
                    if(datum<min) min=datum;
                }
            }

            /* Compensate for the Fact that We Didn't Really Take nmeas Samples */
            sum   = sum   * nmeas/nmeas_used;
            sumsq = sumsq * nmeas/nmeas_used;
        }

        sched_yield();
    }

    //------------------------------------------------------------------------------
    // General Purpose Utilities
    //------------------------------------------------------------------------------

    void waitHalfPeriod(unsigned frequency)
    {

        static const int NANOS = 1000000000LL;
        /*
         * Method 1: Sleep Method
         */
        //long halfperiod = NANOS/ ( 2*frequency);
        //struct timespec delay;
        //delay.tv_sec = 0;
        //delay.tv_nsec = halfperiod/2;
        //clock_nanosleep(CLOCK_MONOTONIC, 0, &delay, NULL);



        /*
         * Method 2: Waiting for a clock to pass
         */

        //long long halfperiod = NANOS /(1000*2*frequency);
        //long long start, elapsed, microseconds;
        //struct timespec begin, current;

        ///* set up start time data */
        //if (clock_gettime(CLOCK_MONOTONIC, &begin)) {
        //    return;
        //}

        ///* Start time in nanoseconds */
        //start = begin.tv_sec*NANOS + begin.tv_nsec;

        //while (true) {
        //    /* Elapsed time in nanoseconds */
        //    if (clock_gettime(CLOCK_MONOTONIC, &current)) {
        //        return;
        //    }

        //    elapsed = current.tv_sec*NANOS + current.tv_nsec - start;
        //    microseconds = elapsed / 1000; // + (elapsed % 1000 >= 500); // round up halves

        //    if (microseconds >= halfperiod)
        //        break;
        //}

        /*
         * Method 3: std::chrono (note! requires C++11)
         */
        using std::chrono::nanoseconds;
        using std::chrono::duration_cast;
        long int halfperiod = (NANOS / ( 2*frequency));
        auto start = duration_cast<nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        while (true) {
            /* Give this thread higher priority to improve timing stability */
            pthread_t this_thread = pthread_self();
            struct sched_param params;
            params.sched_priority = sched_get_priority_max(SCHED_FIFO);
            pthread_setschedparam(this_thread, SCHED_FIFO, &params);

            auto now = duration_cast<nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            auto diff = now-start;
            if (diff > halfperiod)
                break;
        }
        sched_yield();
    }
}
