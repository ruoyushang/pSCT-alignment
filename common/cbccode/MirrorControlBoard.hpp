/*
 * Class which implements useful mid-level functionality for the mirror
 * control board.
 */

#ifndef MIRRORCONTROLBOARD_HPP
#define MIRRORCONTROLBOARD_HPP

#include <vector>
#include <stdint.h>

namespace MirrorControlBoard
{
        enum UStep { USTEP_1, USTEP_2, USTEP_4, USTEP_8 };
        enum Dir { DIR_EXTEND, DIR_RETRACT, DIR_NONE };
        enum GPIODir { DIR_OUTPUT, DIR_INPUT};

        void enableIO();
        void disableIO();

        //------------------------------------------------------------------------------
        // Power Control Function Prototypes
        //------------------------------------------------------------------------------

        void adcSleep(int iadc);

        /* USB power enable_bar bit */
        void powerDownUSB(unsigned iusb);
        void powerUpUSB(unsigned iusb);
        bool isUSBPoweredUp(unsigned iusb);

        /* Encoder Enable Bit */
        void powerDownEncoders();
        void powerUpEncoders();
        bool isEncodersPoweredUp();

        /* Sensor Enable Bit */
        void powerUpSensors();
        void powerDownSensors();
        bool isSensorsPoweredUp();

        //------------------------------------------------------------------------------
        // Motor Driver Function Prototypes
        //------------------------------------------------------------------------------

        /* Motor Driver Sleep bit */
        void powerDownDriveControllers();
        void powerUpDriveControllers();
        bool isDriveControllersPoweredUp();

        /*
         * Steps motor a single step in a direction specified by dir, with some
         * delay controlling the IO speed
         */
        void stepOneDrive(unsigned idrive, Dir dir, unsigned frequency = 1000);

        void setPhaseZeroOnAllDrives();

        void enableDriveSR(bool enable = true);
        void disableDriveSR();
        bool isDriveSREnabled();

        void setUStep(UStep ustep);
        UStep getUStep();

        // Enable or Disable Stepper Motors
        void enableDrive(unsigned idrive, bool enable = true);
        void disableDrive(unsigned idrive);
        bool isDriveEnabled(unsigned idrive);

        /* Driver High Current Mode */
        void enableDriveHiCurrent(bool enable = true);
        void disableDriveHiCurrent();
        bool isDriveHiCurrentEnabled();

        //------------------------------------------------------------------------------
        // ADC Function Prototypes
        //------------------------------------------------------------------------------

        // Asserts Correct ADC Chip Select Line
        void selectADC(unsigned iadc);

        // Writes initialization codes to ADC
        void initializeADC(unsigned iadc);

        // Measures ADC and returns result as value
        uint32_t measureADC(unsigned iadc, unsigned ichan);

        // Makes some specified number measurements on ADC and keeps track of sum, sum of squares, min and max for statistics..
        void measureADCStat(unsigned iadc, unsigned ichan, unsigned nmeas, uint32_t& sum, uint64_t& sumsq, uint32_t& min, uint32_t& max, unsigned ndelay=100);

        // --------------------------------------------------------------------------
        // Utility functions
        // --------------------------------------------------------------------------

        // Sleeps for a half-cycle of the frequency given in the argument...
        void waitHalfPeriod(unsigned frequency);

        void setCalibrationConstant(int constant);
        int  getCalibrationConstant();


        static const unsigned m_nusb=7;
};

#endif // defined MIRRORCONTROLBOARD_HPP
