#pragma once

#include <vector>

/*!
 * The DummyCBC class is responsible for the control of all mirror control board functions.
 *
 * This is a dummy class with no functionality meant to be a drop-in replacement for the real thing
 * to emulate DummyCBC on an AMD64 machine.
 */
class DummyCBC
{
    public:
        struct Config
        {
            int  steppingFrequency ;
            bool highCurrentMode   ;
            bool driveSR           ;
            int  adcReadDelay      ;
            int  defaultADCSamples ;
            int  usbEnable         ;
            int  driveEnable       ;
            int  microsteps        ;
            int  delayTime         ;

            std::vector<float>  encoderVoltageSlope      = {0,0,0,0,0,0};
            std::vector<float>  encoderVoltageOffset     = {0,0,0,0,0,0};
            std::vector<float>  encoderTemperatureSlope  = {0,0,0,0,0,0};
            std::vector<float>  encoderTemperatureOffset = {0,0,0,0,0,0};

            float encoderTemperatureRef;

            Config                   () :
            steppingFrequency        (400),
            highCurrentMode          (false),
            driveSR                  (true),
            adcReadDelay             (0),
            defaultADCSamples        (1000),
            usbEnable                (0),
            driveEnable              (0),
            microsteps               (8),
            delayTime                (25000),
            encoderVoltageSlope      {0,0,0,0,0,0},
            encoderVoltageOffset     {0,0,0,0,0,0},
            encoderTemperatureSlope  {0,0,0,0,0,0},
            encoderTemperatureOffset {0,0,0,0,0,0},
            encoderTemperatureRef    (0.75)
            {}
        };
        static struct Config dummy_config;

        void configure(struct Config config) { return; };

        DummyCBC(struct Config config=DummyCBC::dummy_config) : usb(this), driver(this), encoder(this), adc(this), auxSensor(this) {configure(config);};
        ~DummyCBC() {};

        struct USB {
            void enable     (int usb) {return;};
            void enableAll  () {return;};
            void disable    (int usb) {return;};
            void disableAll () {return;};
            bool isEnabled  (int usb) {return false;};

            USB(DummyCBC *thisDummyCBC) : cbc(thisDummyCBC) {};
            private:
            DummyCBC *cbc;

        } usb;

        struct Driver {
            public:
                void setMicrosteps (int microsteps) {return;};
                int  getMicrosteps () {return 0;};
                void enable     (int drive) {return;};
                void enableAll  () {return; };
                void disable    (int drive) {return; };
                void disableAll () {return; };
                bool isEnabled  (int drive) {return false;};
                void sleep() {return;};
                void wakeup() {return;};
                bool isAwake() {return false;};
                void reset() {return;};
                void enableHighCurrent() {return;};
                void disableHighCurrent() {return;};
                bool isHighCurrentEnabled(){return false;};
                void enableSR(){return;};
                void disableSR(){return;};
                bool isSREnabled(){return false;};
                void step (int drive, int nsteps){return;};
                void step (int drive, int nsteps, int frequency){return;};
                int  getSteppingFrequency(){return 0;};
                void setSteppingFrequency(int frequency){return;};

                Driver(DummyCBC *thisDummyCBC) : cbc(thisDummyCBC) {};
            private:
                DummyCBC *cbc;
                int  m_steppingFrequency;
                static const int  maximumSteppingFrequency = 0x1 << 16;
                static const int  minimumSteppingFrequency = 0;
        } driver;

        struct Encoder {

            Encoder(DummyCBC *thisDummyCBC) : cbc(thisDummyCBC) {};
            private:
            DummyCBC *cbc;
        } encoder;

        struct ADC {
            public:
                struct adcData {
                    float voltage;
                    float stddev;
                    float voltageMin;
                    float voltageMax;
                    float voltageError;
                    float rawVoltage;
                    float rawVoltageMin;
                    float rawVoltageMax;
                } dummyData;
                adcData measure(int adc, int channel) {return dummyData;};
                adcData measure(int adc, int channel, int nsamples) {return dummyData;};
                adcData readEncoder (int iencoder) {return dummyData;};
                adcData readEncoder (int iencoder, int nsamples) {return dummyData;};
                float readEncoderVoltage (int iencoder) {return 0;};
                float readTemperature() {return 0;};
                float readTemperature(int nsamples) {return 0;};

                adcData readTemperatureVolts  () {return dummyData;};
                adcData readTemperatureVolts  (int nsamples) {return dummyData;};

                adcData readExternalTemp () {return dummyData;};
                adcData readExternalTemp (int nsamples) {return dummyData;};
                adcData readRefLow  (int adc) {return dummyData;};
                adcData readRefLow  (int adc, int nsamples) {return dummyData;};
                adcData readRefMid  (int adc) {return dummyData;};
                adcData readRefMid  (int adc, int nsamples) {return dummyData;};
                adcData readRefHigh (int adc) {return dummyData;};
                adcData readRefHigh (int adc, int nsamples) {return dummyData;};
                int  getReadDelay() {return 0;};
                void setReadDelay(int delay) {return;};
                void setDefaultSamples(int nsamples) {return;};
                int  getDefaultSamples() {return 0;};

                float getEncoderTemperatureSlope  ( int iencoder ) {return 0;} ;
                float getEncoderTemperatureOffset ( int iencoder ) {return 0;} ;
                float getEncoderTemperatureRef    (              ) {return 0;};
                float getEncoderVoltageSlope      ( int iencoder ) {return 0;};
                float getEncoderVoltageOffset     ( int iencoder ) {return 0;};

                void  setEncoderTemperatureSlope  ( int iencoder, float slope  ) {return;};
                void  setEncoderTemperatureOffset ( int iencoder, float offset ) {return;};
                void  setEncoderTemperatureRef    ( float reference            ) {return;};
                void  setEncoderVoltageSlope      ( int iencoder, float slope  ) {return;};
                void  setEncoderVoltageOffset     ( int iencoder, float offset ) {return;};


                ADC(DummyCBC *thisDummyCBC) : cbc(thisDummyCBC) {};

            private:
                DummyCBC *cbc;
                int m_readDelay;
                int m_defaultSamples;

                float m_encoderTemperatureOffset [6];
                float m_encoderTemperatureSlope  [6];
                float m_encoderTemperatureRef       ;
                float m_encoderVoltageOffset     [6];
                float m_encoderVoltageSlope      [6];
        } adc;
        struct AUXsensor {
            public:
                bool isEnabled() {return false;};

                AUXsensor(DummyCBC *thisDummyCBC) : cbc(thisDummyCBC) {};
            private:
                DummyCBC *cbc;
        } auxSensor;


        int m_delay; // in milliseconds
        void setDelayTime(int delay) {return;};
        int getDelayTime() {return 0;};
};


