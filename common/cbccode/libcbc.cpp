/*
 * cbc.cpp Console board control
 */

#include <iomanip>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <chrono>
#include <stdio.h>
#include <iostream>

#include "common/cbccode/cbc.hpp"
#include "common/cbccode/MirrorControlBoard.hpp"
#include "common/cbccode/TLC3548_ADC.hpp"

void usleep2 (int usdelay)
{
    static const int NANOS =  1000000000LL;
    static const int MICROS = 1000000LL;
    int nanodelay = usdelay * NANOS / MICROS;
    struct timespec delay{};
    delay.tv_sec = 0;
    delay.tv_nsec = nanodelay;
    clock_nanosleep(CLOCK_MONOTONIC, 0, &delay, nullptr);
}

//----------------------------------------------------------------------------------------------------------------------
// CBC
//----------------------------------------------------------------------------------------------------------------------

    // Constructor..
    CBC::CBC(struct Config config) : usb(this), driver(this), encoder(this), adc(this), auxSensor(this), m_delay(0)
    {
        configure(std::move(config));
        powerUp();
    }

    void CBC::configure(struct Config config)
    {
        /* Microsteps */
        driver.setMicrosteps(config.microsteps);

        /* Set Stepping Frequency */
        driver.setSteppingFrequency(config.steppingFrequency);

        /* High Current Mode */
        if (config.highCurrentMode)
            driver.enableHighCurrent();
        else
            driver.disableHighCurrent();

        /* Drive SR */
        if (config.driveSR)
            driver.enableSR();
        else
            driver.disableSR();

        /* ADC Read Delay */
        adc.setReadDelay(config.adcReadDelay);

        /* ADC Number of Samples */
        adc.setDefaultSamples(config.defaultADCSamples);
        std::cout << "adc.getDefaultSamples(): " << adc.getDefaultSamples() << std::endl;

        /* Turn on Ethernet Dongle */
        usb.enableEthernet();

        /* Configure USBs */
        for (int i=0; i<6; i++) {
            if ((config.usbEnable >> i) & 0x1)
                usb.enable(i+1);
            else
                usb.disable(i+1);
        }

        /* Configure Drives */
        for (int i=0; i<6; i++) {
            if ((config.driveEnable >> i) & 0x1)
                driver.enable(i+1);
            else
                driver.disable(i+1);
        }

        /* CBC Delay Times */
        setDelayTime(config.delayTime);

        /* Encoder Calibration */
        for (int i=0; i<6; i++) {
            adc.setEncoderTemperatureSlope  (i+1, config.encoderTemperatureSlope  [i]);
            adc.setEncoderTemperatureOffset (i+1, config.encoderTemperatureOffset [i]);
            adc.setEncoderVoltageSlope      (i+1, config.encoderVoltageSlope      [i]);
            adc.setEncoderVoltageOffset     (i+1, config.encoderVoltageOffset     [i]);
        }
    }

    void CBC::powerUp()
    {
        // Configure GPIOs
        //gpio->ConfigureAll();

        // turn on level shifters
        MirrorControlBoard::enableIO();

        driver.wakeup();
        //driver.reset();
        encoder.enable();
        auxSensor.enable();

        MirrorControlBoard::initializeADC(0);
        MirrorControlBoard::initializeADC(1);
    }

    void CBC::powerDown() {
        driver.sleep();
        encoder.disable();
        auxSensor.disable();
        driver.disableAll();
        usb.disableAll();
    }

    void CBC::setDelayTime(int delay)
    {
        if (delay >= 0)
            m_delay = delay;
    }

    int CBC::getDelayTime()
    {
        return m_delay;
    }

//----------------------------------------------------------------------------------------------------------------------
// USB Control
//----------------------------------------------------------------------------------------------------------------------

    CBC::USB::USB (CBC *thiscbc) : cbc(thiscbc)
    {
    }

    void CBC::USB::enable(int iusb)
    {
        if((iusb<1)||(iusb>6))
            return;
        MirrorControlBoard::powerUpUSB(iusb);
    }

    void CBC::USB::disable(int iusb)
    {
        if((iusb<1)||(iusb>6))
            return;
        MirrorControlBoard::powerDownUSB(iusb);
    }

    void CBC::USB::enableAll()
    {
        for (int i=1; i<7; i++)
            enable(i);
    }

    void CBC::USB::disableAll()
    {
        for (int i=1; i<7; i++)
            disable(i);
    }

    bool CBC::USB::isEnabled (int iusb)
    {
        if ((iusb<1) | (iusb>6))
            return(-1);
        else
            return (MirrorControlBoard::isUSBPoweredUp(iusb));
    }

    void CBC::USB::enableEthernet()
    {
        MirrorControlBoard::powerUpUSB(0);
    }

    void CBC::USB::disableEthernet()
    {
        MirrorControlBoard::powerDownUSB(0);
    }

    void CBC::USB::resetEthernet()
    {
        disableEthernet();
        sleep(1);
        enableEthernet();
    }

//----------------------------------------------------------------------------------------------------------------------
// Motor Driver Control
//----------------------------------------------------------------------------------------------------------------------

CBC::Driver::Driver(CBC *thiscbc) : cbc(thiscbc), m_steppingFrequency()
    {
    }

    void CBC::Driver::setMicrosteps(int usint)
    {
        MirrorControlBoard::UStep us=MirrorControlBoard::USTEP_1;
        switch(usint) {
            case 1:
                us = MirrorControlBoard::USTEP_1;
                break;
            case 2:
                us = MirrorControlBoard::USTEP_2;
                break;
            case 4:
                us = MirrorControlBoard::USTEP_4;
                break;
            case 8:
                us = MirrorControlBoard::USTEP_8;
                break;
            default:
                return;
        }
        MirrorControlBoard::setUStep(us);
    }

    int CBC::Driver::getMicrosteps()
    {
        unsigned us = MirrorControlBoard::getUStep();
        int steps = 0;
        switch(us) {
            case 0:
                steps = 1;
                break;
            case 1:
                steps = 2;
                break;
            case 2:
                steps = 4;
                break;
            case 3:
                steps = 8;
                break;
            default:
                return(0);
        }
        return (steps);
    }

    void CBC::Driver::enable(int drive)
    {
        // whine if drive is out of bounds
        if ((drive<1)||(drive>6))
            return;

        //enable drive
        MirrorControlBoard::enableDrive(drive-1); //MCB counts from zero
        usleep2(cbc->getDelayTime());
    }

    void CBC::Driver::disable(int drive)
    {
        // whine if drive is out of bounds
        if ((drive<1)||(drive>6))
            return;

        //disable drive
        usleep2(cbc->getDelayTime());
        MirrorControlBoard::disableDrive(drive-1); //MCB counts from zero
    }

    void CBC::Driver::enableAll()
    {
        for (int i=1; i<7; i++)
            enable(i);
    }

    void CBC::Driver::disableAll()
    {
        for (int i=1; i<7; i++)
            disable(i);
    }

    bool CBC::Driver::isEnabled(int drive)
    {
        return (MirrorControlBoard::isDriveEnabled(drive-1)); // MCB counts from zero
    }

    void CBC::Driver::sleep()
    {
        MirrorControlBoard::powerDownDriveControllers();
    }

    void CBC::Driver::wakeup()
    {
        MirrorControlBoard::powerUpDriveControllers();
    }

    bool CBC::Driver::isAwake()
    {
        return(MirrorControlBoard::isDriveControllersPoweredUp());
    }

    bool CBC::Driver::isHighCurrentEnabled()
    {
        return(MirrorControlBoard::isDriveHiCurrentEnabled());
    }

    void CBC::Driver::enableHighCurrent ()
    {
        MirrorControlBoard::enableDriveHiCurrent();
    }

    void CBC::Driver::disableHighCurrent ()
    {
        MirrorControlBoard::disableDriveHiCurrent();
    }

    bool CBC::Driver::isSREnabled()
    {
        return(MirrorControlBoard::isDriveSREnabled());
    }

    void CBC::Driver::enableSR()
    {
        MirrorControlBoard::enableDriveSR();
    }

    void CBC::Driver::disableSR()
    {
        MirrorControlBoard::disableDriveSR();
    }

    void CBC::Driver::setSteppingFrequency (int frequency)
    {
        m_steppingFrequency = frequency;
    }

    void CBC::Driver::reset()
    {
        MirrorControlBoard::setPhaseZeroOnAllDrives();
    }

    void CBC::Driver::step(int drive, int nsteps)
    {
        step(drive, nsteps, m_steppingFrequency);
    }

    void CBC::Driver::step(int drive, int nsteps, int frequency)
    {
        /* Check frequency limits */
        if (frequency > maximumSteppingFrequency)
            frequency = maximumSteppingFrequency;
        else if (frequency < minimumSteppingFrequency)
            frequency = minimumSteppingFrequency;

        /* whine if invalid actuator number is used */
        if ((drive<1)||(drive>6))
            return;

        usleep2(cbc->getDelayTime());
        if (isEnabled(drive)) {
            /* MCB counts from 0 */
            drive = drive - 1;

            /* if nstep > 0, extend. If nstep < 0, retract */
            MirrorControlBoard::Dir dir = MirrorControlBoard::DIR_EXTEND;
            if (nsteps<0)
                dir = MirrorControlBoard::DIR_RETRACT, nsteps=-nsteps;

            /* Convert from microsteps to macrosteps */
            unsigned microsteps = nsteps * getMicrosteps();

            /* loop over number of micro Steps */
            for (unsigned istep=0; istep<microsteps; istep++) {
                /* Give this thread higher priority to improve timing stability */
                pthread_t this_thread = pthread_self();
                struct sched_param params{};
                params.sched_priority = sched_get_priority_max(SCHED_FIFO);
                pthread_setschedparam(this_thread, SCHED_FIFO, &params);

                /* Step the drive */
                MirrorControlBoard::stepOneDrive(drive, dir, frequency * getMicrosteps());
            }
            sched_yield();
            return;
        }
    }

//----------------------------------------------------------------------------------------------------------------------
// Encoder Control
//----------------------------------------------------------------------------------------------------------------------

    CBC::Encoder::Encoder (CBC *thiscbc) : cbc(thiscbc)
    {
    }

    void CBC::Encoder::enable()
    {
        MirrorControlBoard::powerUpEncoders();
    }

    void CBC::Encoder::disable()
    {
        MirrorControlBoard::powerDownEncoders();
    }

    bool CBC::Encoder::isEnabled()
    {
        return (MirrorControlBoard::isEncodersPoweredUp());
    }

//----------------------------------------------------------------------------------------------------------------------
// ADC Readout
//----------------------------------------------------------------------------------------------------------------------

    // Constructor
    //---------------------------------------------

CBC::ADC::ADC(CBC *thiscbc) : cbc(thiscbc), m_readDelay(0),
                              m_defaultSamples(), m_encoderTemperatureOffset(),
                              m_encoderTemperatureSlope(), m_encoderTemperatureRef(),
                              m_encoderVoltageOffset(), m_encoderVoltageSlope()
    {
    }

    // Generic ADC Readout
    //---------------------------------------------

CBC::ADC::adcData CBC::ADC::measure(int adc_num, int channel, int nsamples)
    {
        uint32_t sum;
        uint64_t sumsq;
        uint32_t min;
        uint32_t max;

        /* initialize to zero */
        adcData data{};
        memset(&data, 0, sizeof(adcData));

        /* Make sure we are doing something sensible */
        if ((adc_num > 1) | (adc_num < 0))
            return(data);
        if ((channel > 10) | (channel < 0 ))
            return(data);
        if (nsamples < 0)
            return(data);

        MirrorControlBoard::measureADCStat(adc_num, channel, nsamples, sum, sumsq, min, max, m_readDelay);

        float mean   = double(sum)/nsamples;
        float var    = double((1.0*sumsq) - ((1.0*sum*sum)/nsamples))/nsamples;
        float stddev = sqrtf(var);

        data.voltage      = TLC3548::voltData(mean);
        data.stddev       = TLC3548::voltData(stddev);
        data.voltageMin   = TLC3548::voltData(min);
        data.voltageMax   = TLC3548::voltData(max);
        data.voltageError = TLC3548::voltData(stddev/sqrt(nsamples));

        // raw copies
        data.rawVoltage    = data.voltage;
        data.rawVoltageMin = data.voltageMin;
        data.rawVoltageMax = data.voltageMax;

        return (data);
    }

    // Encoder Readout
    //---------------------------------------------

    float CBC::ADC::readEncoderVoltage (int iencoder)
    {
        return (readEncoder(iencoder).voltage);
    }

    CBC::ADC::adcData CBC::ADC::readEncoder (int iencoder)
    {
        std::cout << "adc.m_defaultSamples: " << m_defaultSamples << std::endl;
        return (readEncoder(iencoder, m_defaultSamples));
    }

    CBC::ADC::adcData CBC::ADC::readEncoder (int iencoder, int nsamples)
    {
        adcData data{};

        assert(iencoder>0);
        assert(iencoder<7);

        /* we count from zero in MCB */
        iencoder = (iencoder-1);

        usleep2(cbc->getDelayTime());
        data = measure(0,iencoder,nsamples);

        /* Encoder Voltage Voltage Circuit Offset and Slope Correction
        *  Note: Let's say that the voltage we read is a polynomic function of the "actual voltage",
        *  i.e., the voltage that we should be reading based on the actual physical position of the encoder.
        *
        *  This is dependent on the temperature of the encoder, as well as the properties of a voltage conditioning
        *  circuit that shapes the encoder voltage for input to the ADC.
        *
        *  So we say,
        *
        *  V_meas = a + (1+b)*V_act + c*(T-T0) + d*V_act*(T-T0)
        *
        *  we solve for V_act = (V_meas - a - c(T-T0)) / (1+b+d(T-T0))
        *
        *  let's name these parameters:
        *
        *  a = voltage_offset
        *  b = voltage_slope CORRECTION
        *  c = temperature_slope
        *  d = temperature_offset
        *
        *  This correction is applied below.
        */

        float voltage_offset     = getEncoderVoltageOffset     (iencoder+1);  // we count from one in CBC
        float voltage_slope      = getEncoderVoltageSlope      (iencoder+1);  // we count from one in CBC

        float temperature_offset = getEncoderTemperatureOffset (iencoder+1);  // we count from one in CBC
        float temperature_slope  = getEncoderTemperatureSlope  (iencoder+1);  // we count from one in CBC

        float temperature_diff = (readTemperatureVolts().voltage - getEncoderTemperatureRef());

        // correct data
        data.voltage    = (data.voltage    - voltage_offset - temperature_offset*temperature_diff ) /
                            (1+voltage_slope + temperature_slope*temperature_diff);
        data.voltageMin = (data.voltageMin - voltage_offset - temperature_offset*temperature_diff ) /
                            (1+voltage_slope + temperature_slope*temperature_diff);
        data.voltageMax = (data.voltageMax - voltage_offset - temperature_offset*temperature_diff ) /
                            (1+voltage_slope + temperature_slope*temperature_diff);

        return(data);
    }

    // Encoder Calibration Parameters
    //---------------------------------------------

    float CBC::ADC::getEncoderTemperatureSlope(int iencoder)
    {
        assert(iencoder>0);
        assert(iencoder<7);

        iencoder = (iencoder-1);
        return (m_encoderTemperatureSlope[iencoder]);
    }

    void CBC::ADC::setEncoderTemperatureRef   (float ref)
    {
        m_encoderTemperatureRef = ref;
    }

    float CBC::ADC::getEncoderTemperatureRef ()
    {
        return (m_encoderTemperatureRef);
    }

    void CBC::ADC::setEncoderTemperatureSlope (int iencoder, float slope)
    {
        assert(iencoder>0);
        assert(iencoder<7);

        iencoder = (iencoder-1);
        m_encoderTemperatureSlope[iencoder] = slope;
    }

    float CBC::ADC::getEncoderTemperatureOffset(int iencoder)
    {
        assert(iencoder>0);
        assert(iencoder<7);

        iencoder = (iencoder-1);
        return (m_encoderTemperatureOffset[iencoder]);
    }

    void CBC::ADC::setEncoderTemperatureOffset (int iencoder, float offset)
    {
        assert(iencoder>0);
        assert(iencoder<7);

        iencoder = (iencoder-1);
        m_encoderTemperatureOffset[iencoder] = offset;
    }

    float CBC::ADC::getEncoderVoltageSlope(int iencoder)
    {
        assert(iencoder>0);
        assert(iencoder<7);

        iencoder = (iencoder-1);
        return (m_encoderVoltageSlope[iencoder]);
    }

    void CBC::ADC::setEncoderVoltageSlope (int iencoder, float slope)
    {
        assert(iencoder>0);
        assert(iencoder<7);

        iencoder = (iencoder-1);
        m_encoderVoltageSlope[iencoder] = slope;
    }

    float CBC::ADC::getEncoderVoltageOffset(int iencoder)
    {
        assert(iencoder>0);
        assert(iencoder<7);

        iencoder = (iencoder-1);
        return (m_encoderVoltageOffset[iencoder]);
    }

    void CBC::ADC::setEncoderVoltageOffset (int iencoder, float offset)
    {
        assert(iencoder>0);
        assert(iencoder<7);

        iencoder = (iencoder-1);
        m_encoderVoltageOffset[iencoder] = offset;
    }


    // Temperature Sensors
    //---------------------------------------------

    float CBC::ADC::readTemperature()
    {
        return((readTemperatureVolts().voltage-0.5)*100);
    }

    float CBC::ADC::readTemperature(int nsamples)
    {
        return((readTemperatureVolts(nsamples).voltage-0.5)*100);
    }

    CBC::ADC::adcData CBC::ADC::readTemperatureVolts ()
    {
        return(readTemperatureVolts(m_defaultSamples));
    }

    CBC::ADC::adcData CBC::ADC::readTemperatureVolts (int nsamples)
    {
        return(measure(0,6,nsamples));
    }

    CBC::ADC::adcData CBC::ADC::readExternalTemp ()
    {
        return(readExternalTemp(m_defaultSamples));
    }

    CBC::ADC::adcData CBC::ADC::readExternalTemp (int nsamples)
    {
        return(measure(0,7,nsamples));
    }

    // Internal Voltage References
    //---------------------------------------------

CBC::ADC::adcData CBC::ADC::readRefLow(int adc_num)
    {
        return (readRefLow(adc_num, m_defaultSamples));
    }

CBC::ADC::adcData CBC::ADC::readRefLow(int adc_num, int nsamples)
    {
        return (measure(adc_num, 10, nsamples));
    }

CBC::ADC::adcData CBC::ADC::readRefMid(int adc_num)
    {
        return (readRefMid(adc_num, m_defaultSamples));
    }

CBC::ADC::adcData CBC::ADC::readRefMid(int adc_num, int nsamples)
    {
        return (measure(adc_num, 9, nsamples));
    }

CBC::ADC::adcData CBC::ADC::readRefHigh(int adc_num)
    {
        return (readRefHigh(adc_num, m_defaultSamples));
    }

CBC::ADC::adcData CBC::ADC::readRefHigh(int adc_num, int nsamples) {
    return (measure(adc_num, 8, nsamples));
    }

    // ADC Measurement Options
    //---------------------------------------------

    int  CBC::ADC::getReadDelay()
    {
        return (m_readDelay);
    }

    void CBC::ADC::setReadDelay(int delay)
    {
        if (delay >= 0)
            m_readDelay = delay;
    }

    void CBC::ADC::setDefaultSamples(int nsamples) {
        if (nsamples > 0)
            m_defaultSamples = nsamples;
    }

//----------------------------------------------------------------------------------------------------------------------
// Sensor Control
//----------------------------------------------------------------------------------------------------------------------

    CBC::AUXsensor::AUXsensor (CBC *thiscbc) : cbc(thiscbc)
    {
    }

    void CBC::AUXsensor::enable()
    {
        MirrorControlBoard::powerUpSensors();
    }

    void CBC::AUXsensor::disable()
    {
        MirrorControlBoard::powerDownSensors();
    }

    bool CBC::AUXsensor::isEnabled()
    {
        return(MirrorControlBoard::isSensorsPoweredUp());
    }
