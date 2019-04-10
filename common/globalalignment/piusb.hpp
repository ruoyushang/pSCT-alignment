#ifndef PIUSB_H
#define PIUSB_H
#include "common/globalalignment/libusb.h"
/*
 * Methods common to the Picard USB Communications
 * devices
 */
class Picard {
    public:
        /* Opens a USB device with the given vid/pid */
        int usbOpen(int vid, int pid);
        /* Detach and close the USB device handle */
        int usbClose();

        /* Writes a USB Bulk Data Transfer */
        int usbWrite(unsigned char *data, int length);
        /* Reads a USB Bulk Data Transfer */
        int usbRead (unsigned char *data, int length);

    private:
        libusb_device_handle *dev_handle;
        libusb_context *ctx;
};

/* Class for USB-MO Linear Motor */
class Motor : public Picard {
    public:
        Motor();
        ~Motor();

        /* Sets motor stepping velocity in units from 1-10
         * 10 = 3ms per step
         * 09 = 4ms per step
         * 08 = 5ms per step
         * 07 = 6ms per step
         * 08 = 7ms per step
         * 06 = 8ms per step
         * 05 = 9ms per step
         * 04 = 10ms per step
         * 03 = 11ms per step
         * 02 = 12ms per step
         * 01 = 13ms per step
         *
         * Speeds above 10 are disabled since they can cause the motor to slip.
         * This is known by Picard..
         */
        int setVelocity(int velocity);

        /* Sends the stepper motor to the given position (0-1900) */
        int setPosition(int position);

        /* Returns the stepper motor position (0-1900) */
        int getPosition();

        /* Retracts the motor until it hits the hall-sensed home position and
         * resets the internal position counter to Zero */
        int goHome();
    private:
        /* Holds the configured velocity */
        int velocity_;

        /* USB-MO VID/PID */
        static const int vendor_id  = 0x0461;
        static const int product_id = 0x0020;
};

/* Class for USB-Twister II Rotary Motor */
class Twister : public Picard {
    public:
        Twister();
        ~Twister();

        /* Sets motor stepping velocity in units from 1-10 */
        int setVelocity(int velocity);

        /* Sends the stepper motor to the given position (range ?) */
        int setPosition(int position);

        /* Returns the stepper motor position (range? ) */
        int getPosition();

        /* Sets the Current Position to be Zero */
        int setZero();
    private:
        /* Holds the configured velocity */
        int velocity_;

        /* USB-Twister II VID/PID */
        static const int vendor_id  = 0x0461;
        static const int product_id = 0x0021;
};

/* Class for Relay Board */
class Relay : public Picard {
    public:
        Relay();
        ~Relay();

        /* Returns integer bitmask of the switch status. i.e.
         * 0 = 0b0000 : All off
         * 1 = 0b0001 : Relay 0   On; Others Off
         * 2 = 0b0010 : Relay 1   On; Others Off
         * 3 = 0b0011 : Relay 0,1 On; Others Off
         * etc..
         */
        int getState ();
        /* Set switches into the status according to the scheme
         * prescribed by getState */
        int setState (int status);

        /* Lets you switch a single switch. i.e.
         * setState (0, 1) => turn on Relay 0
         * setState (1, 1) => turn on Relay 1
         * setState (1, 0) => turn off Relay 1
         * etc.
         */
        int setState (int relay, bool on);
    private:
        /* USB-RELAY VID/PID */
        static const int vendor_id  = 0x0461;
        static const int product_id = 0x0010;
};

/* Class for USB Laser */
class Laser : public Picard {
    public:
        Laser();
        ~Laser();
        void setOn();
        void setOff();
    private:
        static const int vendor_id = 0x0461;
        static const int product_id = 0x0011;
};
#endif
