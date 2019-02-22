#include <cbc.hpp>

#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <stdio.h>

enum UStep { USTEP_1, USTEP_2, USTEP_4, USTEP_8 };
enum Dir { DIR_EXTEND, DIR_RETRACT, DIR_NONE };
enum GPIODir { DIR_OUTPUT, DIR_INPUT};


void usage();

int main(int argc, const char** argv)
{
    argv++, argc--;

    CBC cbc;

    if(argc == 0)
        usage();

    std::string command = *argv;
    argv++, argc--;

    if (command == "power")
    {
        if(argc==0)
            usage();

        if (strcmp(*argv,"up")==0)
            cbc.powerUp();
        else if (strcmp(*argv,"down")==0)
            cbc.powerDown();
    }
    else if (command == "power_down")
        cbc.powerDown();
    else if (command == "power_up")
        cbc.powerUp();
    else if (command == "power_down_encoders")
        cbc.encoder.disable();
    else if (command == "power_up_encoders")
        cbc.encoder.enable();
    else if (command == "power_down_sensors")
        cbc.auxSensor.disable();
    else if (command == "power_up_sensors")
        cbc.auxSensor.enable();
    else if (command == "power_down_a3977")
        cbc.driver.sleep();
    else if (command == "power_up_a3977")
        cbc.driver.wakeup();
    else if (command == "reset")
        cbc.driver.reset();
    else if (command == "resetEthernet")
        cbc.usb.resetEthernet();
    else if (command == "enable_sr")
        cbc.driver.enableSR();
    else if (command == "disable_sr")
        cbc.driver.disableSR();
    else if (command == "set_microstep")
    {
        if(argc==0)
            usage();
        unsigned usint = atoi(*argv);

        cbc.driver.setMicrosteps(usint);
    }
    else if (command == "enable_hi_current")
        cbc.driver.enableHighCurrent();
    else if (command == "disable_hi_current")
        cbc.driver.disableHighCurrent();
    else if (command == "enable")
    {
        if(argc==0)
            usage();

        unsigned idrive;
        if (strcmp(*argv,"all")==0)
            cbc.driver.enableAll();
        else {
            idrive = atoi(*argv);
            cbc.driver.enable(idrive);
        }
    }
    else if (command == "disable")
    {
        if(argc==0)
            usage();

        unsigned idrive;
        if (strcmp(*argv,"all")==0)
            cbc.driver.disableAll();
        else {
            idrive = atoi(*argv);
            cbc.driver.disable(idrive);
        }
    }
    else if ( (command == "enableusb") | (command == "enableUSB") )
    {
        if(argc==0)
            usage();

        unsigned iusb;
        if (strcmp(*argv,"all")==0) {
            cbc.usb.enableAll();
        }
        else {
            iusb = atoi(*argv);
            cbc.usb.enable(iusb);
        }

    }
    else if ((command == "disableusb") | (command == "disableUSB"))
    {
        if(argc==0)
            usage();

        unsigned iusb;
        if (strcmp(*argv,"all")==0) {
            cbc.usb.disableAll();
        }
        else {
            iusb = atoi(*argv);
            cbc.usb.disable(iusb);
        }

    }

    else if (command == "step")
    {
        if(argc<2)
            usage();

        // actuator number
        unsigned idrive = atoi(*argv);
        argc--, argv++;

        // number of setps
        int nstep = atoi(*argv);
        argc--, argv++;

        cbc.driver.step(idrive, nstep);
    }
    else if (command == "status")
    {
        printf("Mirror Control Board status: \n");

        // Motor Driver Status
        printf("\nA3977 Drive Controllers:        ");
        if (cbc.driver.isAwake())
            printf("Enabled");
        else
            printf("Disabled");

        // SR Status
        printf("\nSynchonous Rectification (SR):  ");
        if (cbc.driver.isSREnabled())
            printf("Enabled");
        else
            printf("Disabled");

        // Microstep Status
        printf("\nDrive microstep set to:         ");
        switch(cbc.driver.getMicrosteps())
        {
            case USTEP_8:
                printf("8-uSTEP");
                break;
            case USTEP_4:
                printf("4-uSTEP");
                break;
            case USTEP_2:
                printf("2-uSTEP");
                break;
            case USTEP_1:
                printf("1-uSTEP");
                break;
        }

        // Hi Current Status
        printf("\nHi-Current Mode:                ");
        if (cbc.driver.isHighCurrentEnabled())
            printf("Enabled");
        else
            printf("Disabled");

        // Encoder Status
        printf("\nEncoders:                       ");
        if (cbc.encoder.isEnabled())
            printf("Enabled");
        else
            printf("Disabled");

        // Sensor Power Status
        printf("\nSensor Enabled:             ");
        if (cbc.auxSensor.isEnabled())
            printf("Enabled");
        else
            printf("Disabled");

        // Print Drive Status
        printf("\n");
        for(int idrive=0; idrive<6; idrive++)
        {
            if (cbc.driver.isEnabled(idrive))
                printf("\nDrive %i                         Enabled",  idrive+1);
            else
                printf("\nDrive %i                         Disabled", idrive+1);
        }

        // Print USB Status
        printf("\n");
        for (int iusb=0; iusb<7; iusb++)
        {
            if (cbc.usb.isEnabled(iusb))
                printf("\nUSB %i                           Enabled",  iusb+1);
            else
                printf("\nUSB %i                           Disabled",  iusb+1);
        }


        printf("\n");
    }
    else if (command == "readTemperature")
    {
        CBC::ADC::adcData data;

        data = cbc.adc.readTemperatureVolts();
        printf("Onboard Voltage: %5.4f\n", data.voltage);
        printf("Onboard Stddev:  %5.4f\n", data.stddev);
        printf("Max   :  %5.4f\n", data.voltageMax);
        printf("Min   :  %5.4f\n", data.voltageMin);

        data = cbc.adc.readExternalTemp();
        printf("External Voltage: %5.4f\n", data.voltage);
        printf("External Stddev:  %5.4f\n", data.stddev);
        printf("Max   :  %5.4f\n", data.voltageMax);
        printf("Min   :  %5.4f\n", data.voltageMin);
    }
    else if (command == "readEncoder")
    {
        if(argc<1)
            usage();

        // channel
        if(argc==0)
            usage();
        int chan = atoi(*argv);
        argc--, argv++;

        CBC::ADC::adcData data;
        data = cbc.adc.readEncoder(chan);

        printf("Voltage: %5.4f\n", data.voltage);
        printf("Stddev:  %5.4f\n", data.stddev);
        printf("Max   :  %5.4f\n", data.voltageMax);
        printf("Min   :  %5.4f\n", data.voltageMin);
    }
    else if (command == "readRefLow")
    {
        CBC::ADC::adcData data;

        data = cbc.adc.readRefLow(0);
        printf("Voltage: %5.4f\n", data.voltage);
        printf("Stddev:  %5.4f\n", data.stddev);
        printf("Max   :  %5.4f\n", data.voltageMax);
        printf("Min   :  %5.4f\n", data.voltageMin);

    }
    else if (command == "readRefMid")
    {
        CBC::ADC::adcData data;

        data = cbc.adc.readRefMid(0);
        printf("Voltage: %5.4f\n", data.voltage);
        printf("Stddev:  %5.4f\n", data.stddev);
        printf("Max   :  %5.4f\n", data.voltageMax);
        printf("Min   :  %5.4f\n", data.voltageMin);

    }
    else if (command == "readRefHigh")
    {
        CBC::ADC::adcData data;

        data = cbc.adc.readRefHigh(0);
        printf("Voltage: %5.4f\n", data.voltage);
        printf("Stddev:  %5.4f\n", data.stddev);
        printf("Max   :  %5.4f\n", data.voltageMax);
        printf("Min   :  %5.4f\n", data.voltageMin);

    }
    else
        usage();
}

void usage () {
    std::string usage_text =
        "CBC v2.3.0 Usage:"
        "\n    command                {required arguments} [optional arguments]\n"
        "\n"
        "\n    power down             Go into power saving mode. Power down encoders, USB and A3977."
        "\n    power up               Power up all on-board and off-board electronics."
        "\n    "
        "\n    power_down_encoders    Power down encoders."
        "\n    power_up_encoders      Power up encoders."
        "\n    "
        "\n    power_down_a3977       Power down A3977 motor controllers."
        "\n    power_up_a3977         Power up A3977 motor controllers."
        "\n    "
        "\n    reset                  Resets phase to zero on all drives."
        "\n    "
        "\n    enable_sr              Enable Synchronous Rectification (SR) mode."
        "\n    disable_sr             Disable Synchronous Rectification (SR) mode."
        "\n    "
        "\n    set_microstep          {MS 1,2,4,8}"
        "\n                           Set number of micro steps to 1, 2, 4 or 8."
        "\n    "
        "\n    enable_hi_current      Enable high current mode on all drives."
        "\n    disable_hi_current     Disable high current mode on all drives."
        "\n    "
        "\n    enable                 {DR 1-6, or all)"
        "\n                           Enable motor driver."
        "\n    disable                {DR 1-6, or all}"
        "\n                           Disable motor driver"
        "\n    "
        "\n    enableusb              {USB 1-7, or all}"
        "\n                           Enable USB (USB)."
        "\n    disableusb             {USB 1-7, or all"
        "\n                           Disable USB (USB)."
        "\n    "
        "\n    step                   {DR 1-6} {NSTEPS} [Frequency=4000]"
        "\n                           Step drive some number of steps (positive to"
        "\n                           extend, negative to retract) with frequency in Hz"
        "\n    "
        "\n    status                 Print drive status information"
        "\n    "
        "\n    measure                {ADC 1-8} {CHAN 0-11} [MEAS=1 BURN=0 DELAY=0 SCALE=5.00]"
        "\n                           Measure voltage of the specified ADC channel. Channel can be"
        "\n                           given as zero to specify all channels on one ADC. Channels 9, 10"
        "\n                           and 11 correspond to internal reference voltages on the ADC."
        "\n                           Prints out statistics."
        "\n                           Note that delay does nothing but you have to put"
        "\n                           it anyway if you want to chance the scale."
        "\n"
        "\n    measure_full           {ADC 1-8} {CHAN 0-11} [MEAS=1 DELAY=0 SCALE=5.00]"
        "\n                           Measure voltage of the specified ADC channel. Channel can be"
        "\n                           given as zero to specify all channels on one ADC. Channels 9, 10"
        "\n                           and 11 correspond to internal reference voltages on the ADC."
        "\n                           Prints out raw data."
        "\n                           Note that delay does nothing but you have to put"
        "\n                           it anyway if you want to chance the scale."
        "\n"
        "\n    calibrate              {DR 1-6} [NSTEP=10000 NCYCLE=0 FREQUENCY=4000 ADC=7 MEAS=1]"
        "\n                           Step drive and read ADC after each step, printing its value to"
        "\n                           the terminal. If NCYCLE is 0 or 1 then it specifies the direction"
        "\n                           of the travel, otherwise it specifies the number of half cycles"
        "\n                           of expansion and contraction to perform."
        "\nCBC v2.3.0"
        "\n";
    std::cout << usage_text;
}


