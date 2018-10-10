/*
 * Maps layout of UCLA Board to Overo GPIO pins
 */

#ifndef LAYOUT_HPP
#define LAYOUT_HPP

#define N(x) (sizeof(x)/sizeof(*x))
#define GPIODIR_IN  0
#define GPIODIR_OUT 1

namespace Layout
{
    // Returns GPIO Input/Output Direction for given GPIO
    int gpioConfiguration(unsigned ipin);

    // Returns LAYOUT independent GPIO number 0-192 for Each Signal
    extern unsigned igpioN_M_RESET     ;
    extern unsigned igpioEN_IO         ;

    extern unsigned igpioPowerADC      ; //PowerADC                 OUTPUT
    extern unsigned igpioADCSel1       ; //ADCSel1                  OUTPUT
    extern unsigned igpioADCSel2       ; //ADCSel2                  OUTPUT

    extern unsigned igpioEncoderEnable ; //EncoderEnable

    extern unsigned igpioConsoleRXD    ; //ConsoleRXD
    extern unsigned igpioConsoleTXD    ; //ConsoleTXD

    extern unsigned igpioTP1           ;
    extern unsigned igpioTP2           ;
    extern unsigned igpioTP3           ;
    extern unsigned igpioTP4           ;
    extern unsigned igpioTP5           ;
    extern unsigned igpioTP6           ;
    extern unsigned igpioTP7           ;

    extern unsigned igpioUSBReset      ; //USBReset
    extern unsigned igpioUSBOff1       ; //USB1EnableBar            OUTPUT
    extern unsigned igpioUSBOff2       ; //USB2EnableBar            OUTPUT
    extern unsigned igpioUSBOff3       ; //USB3EnableBar            OUTPUT
    extern unsigned igpioUSBOff4       ; //USB4EnableBar            OUTPUT
    extern unsigned igpioUSBOff5       ; //USB5EnableBar            OUTPUT
    extern unsigned igpioUSBOff6       ; //USB6EnableBar            OUTPUT
    extern unsigned igpioUSBOff7       ; //USB7EnableBar            OUTPUT

    extern unsigned igpioMS1           ; //DRMS1                    OUTPUT
    extern unsigned igpioMS2           ; //DRMS2                    OUTPUT

    extern unsigned igpioPwrIncBar     ; //DRPowerIncreaseBar       OUTPUT
    extern unsigned igpioSR            ; //DRSRBar                  OUTPUT
    extern unsigned igpioReset         ; //DRResetBar               OUTPUT
    extern unsigned igpioSleep         ; //DRSleepBar               OUTPUT

    extern unsigned igpioStep1         ; //DR1Step                  OUTPUT
    extern unsigned igpioStep2         ; //DR2Step                  OUTPUT
    extern unsigned igpioStep3         ; //DR3Step                  OUTPUT
    extern unsigned igpioStep4         ; //DR4Step                  OUTPUT
    extern unsigned igpioStep5         ; //DR5Step                  OUTPUT
    extern unsigned igpioStep6         ; //DR6Step                  OUTPUT

    extern unsigned igpioDir1          ; //DR1Dir                   OUTPUT
    extern unsigned igpioDir2          ; //DR2Dir                   OUTPUT
    extern unsigned igpioDir3          ; //DR3Dir                   OUTPUT
    extern unsigned igpioDir4          ; //DR4Dir                   OUTPUT
    extern unsigned igpioDir5          ; //DR5Dir                   OUTPUT
    extern unsigned igpioDir6          ; //DR6Dir                   OUTPUT

    extern unsigned igpioEnable1       ; //DR1EnableBar             OUTPUT
    extern unsigned igpioEnable2       ; //DR2EnableBar             OUTPUT
    extern unsigned igpioEnable3       ; //DR3EnableBar             OUTPUT
    extern unsigned igpioEnable4       ; //DR4EnableBar             OUTPUT
    extern unsigned igpioEnable5       ; //DR5EnableBar             OUTPUT
    extern unsigned igpioEnable6       ; //DR6EnableBar             OUTPUT

    extern unsigned igpioSPI_Tx        ; //SPI MOSI                 OUTPUT
    extern unsigned igpioSPI_Rx        ; //SPI MISO                 INPUT
    extern unsigned igpioSPI_Sclk      ; //SPI CLK                  OUTPUT
    extern unsigned igpioSPI_SFRM_bar  ;

    // Returns Motor Direction Control Pin GPIO Number for a given idrive
    unsigned igpioDir(const unsigned idrive);

    // Returns Motor Step Pin GPIO Number for a given idrive
    unsigned igpioStep(const unsigned idrive);

    // Returns Motor Enable Pin GPIO Number for a given idrive
    unsigned igpioEnable(const unsigned idrive);

    // Returns USB Power Enable Pin GPIO Number for a given iusb
    unsigned igpioUSBOff(const unsigned iusb);

    //Maps Overo output pins (J1 1-70, J 71-140) to GPIO Pins
    //-1 is returned for a pin140 that is not a GPIO
    int pin140ToGPIO(unsigned ipin140);

    ////////////////////////////////////////////////////////////////////////////////
    // Layout Specific Pin Assignments for Overo
    ////////////////////////////////////////////////////////////////////////////////
    //static const int         ADC_SPI              4

    //Connector J1 (70-pin): LCD, PWM & Analog Signals
    /////// SIGNAL                  PIN
    static const int N_MANUAL_RESET       = 1;
    static const int GPIO71_L_DD01        = 2;
    static const int GPIO70_L_DD00        = 3;
    static const int GPIO73_L_DD03        = 4;
    static const int GPIO75_L_DD05        = 5;
    static const int GPIO72_L_DD02        = 6;
    static const int GPIO74_L_DD04        = 7;
    static const int GPIO_10              = 8;
    static const int GPIO0_WAKEUP         = 9;
    static const int GPIO185_I2C3_SDA     = 10;
    static const int GPIO80_L_DD10        = 11;
    static const int GPIO81_L_DD11        = 12;
    static const int GPIO184_I2C3_SCL     = 13;
    static const int GPIO_186             = 14;
    static const int GPIO92_L_DD22        = 15;
    static const int GPIO147_GPT8_PWM     = 16;
    static const int GPIO83_L_DD13        = 17;
    static const int GPIO144_GPT9_PWM     = 18;
    static const int GPIO84_L_DD14        = 19;
    static const int GPIO85_L_DD15        = 20;
    static const int GPIO146_GPT11_PWM    = 21;
    static const int GPIO163_IR_CTS3      = 22;
    static const int GPIO91_L_DD21        = 23;
    static const int GPIO87_L_DD17        = 24;
    static const int GPIO88_L_DD18        = 25;
    static const int GPIO166_IR_TXD3      = 26;
    static const int GPIO89_L_DD19        = 27;
    static const int GPIO79_L_DD09        = 28;
    static const int GPIO77_L_DD07        = 29;
    static const int GPIO78_L_DD08        = 30;
    static const int GPIO165_IR_RXD3      = 31;
    static const int GPIO66_L_PCLK        = 32;
    static const int GPIO76_L_DD06        = 33;
    static const int GPIO68_L_FCLK        = 34;
    static const int GPIO67_L_LCLK        = 35;
    static const int USBOTG_DP            = 36;
    static const int USBOTG_DM            = 37;
    static const int AUXLF                = 38;
    static const int MIC_SUB_MF           = 39;
    static const int ADCIN4               = 40;
    static const int AUXRF                = 41;
    static const int PWM0                 = 42;
    static const int GPIO69_L_BIAS        = 43;
    static const int GPIO86_L_DD16        = 44;
    static const int GPIO90_L_DD20        = 45;
    static const int USBOTG_ID            = 46;
    static const int GPIO170_HDQ_1WIRE    = 47;
    static const int ADCIN3               = 48;
    static const int PWM1                 = 49;
    static const int AGND                 = 50;
    static const int ADCIN5               = 51;
    static const int VBACKUP              = 52;
    static const int ADCIN6               = 53;
    static const int USBOTG_VBUS          = 54;
    static const int GPIO145_GPT10_PWM    = 55;
    static const int GND1                 = 56;
    static const int MIC_MAIN_MF          = 57;
    static const int ADCIN2               = 58;
    static const int SYSEN                = 59;
    static const int GPIO82_L_DD12        = 60;
    static const int GPIO93_L_DD23        = 61;
    static const int TV_OUT2              = 62;
    static const int TV_OUT1              = 63;
    static const int ADCIN7               = 64;
    static const int POWERON              = 65;
    static const int VSYSTEM1             = 66;
    static const int VSYSTEM2             = 67;
    static const int HSOLF2               = 68;
    static const int HSORF3               = 69;
    static const int GND2                 = 70;

    //Connector J4 (70-pin): Extended Memory Bus & MMC Signals
    ///////  SIGNAL                 PIN
    static const int VSYSTEM3             = 71;
    static const int VSYSTEM4             = 72;
    static const int GND3                 = 73;
    static const int EM_NCS5_ETH0         = 74;
    static const int EM_NCS4              = 75;
    static const int EM_NWE               = 76;
    static const int EM_NADV_ALE          = 77;
    static const int EM_NOE               = 78;
    static const int GPIO65_ETH1_IRQ1     = 79;
    static const int GPIO64_ETH0_NRESET   = 80;
    static const int EM_A2                = 81;
    static const int EM_A8                = 82;
    static const int EM_A5                = 83;
    static const int EM_A7                = 84;
    static const int EM_D2                = 85;
    static const int EM_D10               = 86;
    static const int EM_D3                = 87;
    static const int EM_D11               = 88;
    static const int EM_D4                = 89;
    static const int EM_D12               = 90;
    static const int EM_D5                = 91;
    static const int EM_D15               = 92;
    static const int GPIO13_MMC3_CMD      = 93;
    static const int GPIO148_TXD1         = 94;
    static const int GPIO176_ETH0_IRQ     = 95;
    static const int GPIO18_MMC3_D0       = 96;
    static const int GPIO174_SPI1_CS0     = 97;
    static const int GPIO168_USBH_CPEN    = 98;
    static const int GPIO14_MMC3_DAT4     = 99;
    static const int GPIO21_MMC3_DAT7     = 100;
    static const int GPIO17_MMC3_D3       = 101;
    static const int USBH_VBUS            = 102;
    static const int GND4                 = 103;
    static const int USBH_DP              = 104;
    static const int USBH_DM              = 105;
    static const int GPIO19_MMC3_D1       = 106;
    static const int GPIO22_MMC3_DAT6     = 107;
    static const int GPIO23_MMC3_DAT5     = 108;
    static const int GPIO20_MMC3_D2       = 109;
    static const int GPIO12_MMC3_CLK      = 110;
    static const int GPIO114_SPI1_NIRQ    = 111;
    static const int GPIO175_SPI1_CS1     = 112;
    static const int GPIO171_SPI1_CLK     = 113;
    static const int GPIO172_SPI1_MOSI    = 114;
    static const int GPIO173_SPI1_MISO    = 115;
    static const int _4030GP2_N_MMC3_CD   = 116;
    static const int GPIO150_MMC3_WP      = 117;
    static const int GPIO151_RXD1         = 118;
    static const int EM_D7                = 119;
    static const int EM_D14               = 120;
    static const int EM_D6                = 121;
    static const int EM_D13               = 122;
    static const int EM_D1                = 123;
    static const int EM_D8                = 124;
    static const int EM_D9                = 125;
    static const int EM_D0                = 126;
    static const int EM_A6                = 127;
    static const int EM_A1                = 128;
    static const int EM_A3                = 129;
    static const int EM_A10               = 130;
    static const int EM_A4                = 131;
    static const int EM_A9                = 132;
    static const int EM_NWP               = 133;
    static const int EM_NCS1              = 134;
    static const int EM_NBE0              = 135;
    static const int EM_NCS0              = 136;
    static const int EM_NCS6              = 137;
    static const int EM_WAIT0             = 138;
    static const int EM_NBE1              = 139;
    static const int EM_CLK               = 140;
};

#endif // ndef LAYOUT_HPP
