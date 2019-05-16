#ifndef CBCCODE_CBC_HPP
#define CBCCODE_CBC_HPP

#include <vector>
#include <climits>

/*!
 * The CBC class is responsible for the control of all mirror control board functions.
 *
 * It is divided into structures corresponding to different hardware units. All
 * control and readout of a hardware unit is done through its accompanying
 * structure.
 *
 * Initialization of the board is performed in the constructor of the class.
 */
class CBC
{
    public:
        /*!
         *
         * Constructor performs initialization of the CBC board, and accepts
         * a configuration struct CBC::Config to override default construction
         * parameters.
         */

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

            /* @param microsteps                      Number of microsteps [1,2,4 or 8]
             * @param steppingFrequency               Stepping frequency [in Hertz]
             * @param highCurrentMode                 Stepper motor High Current Mode [true/false]
             * @param driveSR                         Drive synchronous rectification mode [true/false]
             * @param adcReadDelay                    Delay inserted between subsequent ADC reads [any integer]
             * @param defaultADCSamples               Set a global default number of ADC samples. Can be overrode for individual measurements.
             * @param usbEnable                       Integer bitmask to enable USB channels according to the simple scheme:
             *                                        <UL>
             *                                        <LI> (0x00) 000000 Disable All
             *                                        <LI> (0x01) 000001 Enable USB 1
             *                                        <LI> (0x02) 000010 Enable USB 2
             *                                        <LI> (0x04) 000100 Enable USB 3
             *                                        <LI> (0x08) 001000 Enable USB 4
             *                                        <LI> (0x10) 010000 Enable USB 5
             *                                        <LI> (0x20) 100000 Enable USB 6
             *                                        <LI> (0x3F) 111111 Enable All
             *                                        </UL>
             *                                        You enable arbitrary combinations of USBs by just forming the 'bitwise or'
             *                                        of the individual masks. Use a calculator or just put "usbEnable = (0x1 | 0x2 | 0x4)", for example...
             * @param driveEnable                     Integer bitmask to enable encoder drives, working ala usbEnable
             * @param delayTime                       Microseconds delay to pad between stepping, reading encoders, enable/disable motors
             * @param encoderVoltageSlope             C++ std::vector containing 6 floats for correcting the encoder readings (c.f. libcbc.cpp for notes on how the correction is applied)
             * @param encoderVoltageOffset            C++ std::vector containing 6 floats for correcting the encoder readings (c.f. libcbc.cpp for notes on how the correction is applied)
             * @param encoderTemperatureSlope         C++ std::vector containing 6 floats for correcting the encoder readings (c.f. libcbc.cpp for notes on how the correction is applied)
             * @param encoderTemperatureOffset        C++ std::vector containing 6 floats for correcting the encoder readings (c.f. libcbc.cpp for notes on how the correction is applied)

             */

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

        static struct Config config_default;

    virtual void configure(struct Config config);

    explicit CBC(struct Config config = CBC::config_default);

    ~CBC() = default;

        /*! Power down CBC
         *
         * Puts the CBC board into a power-down state, decreasing power consumption. Specifically:
         *      <UL>
         *      <LI> Turn off auxillary sensor power
         *      <LI> Turn off encoders power
         *      <LI> Put motor drivers into a sleep state
         *      <LI> Put ADCs into a sleep state
         *      <LI> Turn off all USB connections (excluding ethernet)
         *      </UL>
         */
        void powerDown();

        /*! Power up CBC
         *
         * Performs the reverse sequence of CBC powerDown, but does NOT:
         * Turn On all USBs
         * Enable all Drives
         * This is called automatically when the CBC class is constructed.
         */
        void powerUp();

        //////////////////////////////////////////////////////////////////////////////
        ///USB Control
        //////////////////////////////////////////////////////////////////////////////
        struct USB {
            ///@{
            /*! @name USB Control
             * Functions to control the CBC USB Ports.
             */

            /** @brief Enable USB 1-6 */
            void enable     (int usb);
            /** @brief Enable all USBs */
            void enableAll  ();
            /** @brief Disable USB 1-6 */
            void disable    (int usb);
            /** @brief Disable all USBs */
            void disableAll ();
            /** @brief Return enabled/disabled status of USB 1-6 */
            bool isEnabled  (int usb);

            ///@}

            ///@{
            /*! @name Ethernet Control
             * Functions to control the USB ethernet dongle. */

            /*! @brief Resets the USB Ethernet Dongle by toggling it off, waiting 1 second, then toggling it back on. */
            void resetEthernet();

            /*! @brief Enable the USB Ethernet Dongle */
            void enableEthernet();
            ///@}

            explicit USB(CBC *cbc);
            private:
            CBC *cbc;

            /*!
             * Disable the USB Ethernet Dongle
             */
            void disableEthernet();
        } usb;

        //////////////////////////////////////////////////////////////////////////////
        ///Motor Driver Control
        /////////////////////////////////////////////////////////////////////////////

        struct Driver {
            public:
                ///@{
                /*! @name Microstep Setting Control
                */
                /*! @brief Set number of micro steps to 1, 2, 4 or 8. */
                void setMicrosteps (int microsteps);
                /*! @brief Returns the current setting for the number of microsteps.  */
                int  getMicrosteps ();
                ///@}

                ///@{
                /*! @name Motor Driver Control */

                /** @brief Enable Motor Driver
                 *  @param drive Motor Driver 1-6
                 */
                void enable     (int drive);

                /** @brief Enable All Motor Drivers */
                void enableAll  ();

                /** @brief Disable Motor Driver
                 *  @param drive Motor Driver 1-6
                 */
                void disable    (int drive);
                /** @brief Disable All Motor Drivers
                */

                void disableAll ();
                /** @brief Check enabled or disabled status of a motor driver
                 *  @param drive Motor Driver 1-6
                 */
                bool isEnabled  (int drive);
                ///@}

                ///@{
                /*! @name Sleep Control
                 *
                 * An active-low control input used to minimize power consumption when
                 * not in use. This disables much of the internal circuitry including
                 * the output DMOS, regulator, and charge pump. A logic high allows
                 * normal operation and startup of the device in the home position.
                 * When coming out of sleep mode, wait 1 ms before issuing a STEP
                 * command to allow the charge pump (gate drive) to stabilize.
                 */
                /*! @brief Put motor drivers to sleep */
                void sleep();
                /*! @brief Wake motor drivers up from sleep */
                void wakeup();
                /*! @brief Check whether motor drivers are sleeping */
                bool isAwake();
                ///@}

                /*! Reset motor driver
                 *
                 * The reset() will briefly toggle OFF then ON the motor drivers RESET
                 * input.
                 *
                 * The RESET input (active low) sets the translator to a predefined
                 * home state (see figures for home state conditions) and turns off all
                 * of the DMOS outputs. The HOME output goes low and all STEP inputs
                 * are ignored until the RESET input goes high.
                 *
                 */
                void reset();

                ///@{
                /*! @name High Current Mode
                 *
                 * Control the motor driver "High Current Mode"
                 */
                /*! @brief Enable High Current Mode */
                void enableHighCurrent();
                /*! @brief Disable High Current Mode */
                void disableHighCurrent();
                /*! @brief Returns status of high current mode*/
                bool isHighCurrentEnabled();
                ///@}

                ///@{
                /*! @name Synchronous Rectification Mode
                 *
                 * Controls the A3977 Synchronous Rectification Mode, which is designed
                 * to reduces power dissipation and elimitate the need for external
                 * Shottky diodes.
                 */
                /*! @brief Enable synchronous rectification mode */
                void enableSR();
                /*! @brief Disable synchronous rectification mode */
                void disableSR();
                /*! @brief Returns status of synchronous rectification mode */
                bool isSREnabled();
                ///@}

                ///@{
                /*! @name Step Drive
                 */

                /*! @brief Step drive using global frequency
                 * @param drive  Motor drive 1-6
                 * @param nsteps Number of MACRO-steps
                 */
                void step (int drive, int nsteps);

                /*! @brief Step drive with configurable frequency
                 *
                 * @param drive  Motor drive 1-6
                 * @param nsteps Number of MACRO-steps
                 * @param frequency OPTIONAL argument to specify a stepping frequency, otherwise the global default will be assumed.
                 */
                void step (int drive, int nsteps, int frequency);
                ///@}


                ///@{
                /*! @name Global stepping frequency
                 *
                 * This value can be overwritten when calling stepping functions, but
                 * setting a default will let you use a global frequency and avoid
                 * having to specify each time you perform stepping.
                 */
                /*! @brief Returns global stepping frequency */
                int getSteppingFrequency() { return m_steppingFrequency; }
                /*! @brief Sets global stepping frequency
                 *  @param frequency Stepping frequency, in macrosteps/second
                 */
                void setSteppingFrequency(int frequency);
                //@}

            explicit Driver(CBC *cbc);
            private:
                CBC *cbc;
                /*!
                 * Global default stepping frequency
                 */
                int  m_steppingFrequency;

                ///@{
                /*!
                 * Some parameters to set a maximum and minimum
                 * allowable stepping frequency, that will forcefully limit stepping to
                 * within its allowed range of values.
                 *
                 * Right now it is just set to a very large range, but hopefully some sane limits
                 * would be chosen in the future.
                 */
                static const int maximumSteppingFrequency = INT_MAX;
                static const int  minimumSteppingFrequency = 0;
                ///@}
        } driver;

        //////////////////////////////////////////////////////////////////////////////
        ///Encoder Power Control
        //////////////////////////////////////////////////////////////////////////////

        struct Encoder {
            ///@{
            /*! @name Encoder Power Control
            */
            /*! @brief Enable power to encoders. */
            void enable();
            /*! @brief Disable power to encoders. */
            void disable();
            /*! @brief Returns power enabled status of encoders. */
            bool isEnabled();
            ///@}

            explicit Encoder(CBC *cbc);
            private:
            CBC *cbc;
        } encoder;

        //////////////////////////////////////////////////////////////////////////////
        ///ADC Control/Readout
        //////////////////////////////////////////////////////////////////////////////

        struct ADC {
            public:
                //////////////////////////////////////////////////////////////////////////////
                ///Structure to hold statistical data from ADC measurements
                //////////////////////////////////////////////////////////////////////////////
                struct adcData {
                    /*! Averaged voltage reading */
                    float voltage;
                    /*! Standard deviation of voltage measurement */
                    float stddev;
                    /*! Minimum value from distribution of voltage readings */
                    float voltageMin;
                    /*! Maximum value from distribution of voltage readings */
                    float voltageMax;
                    /*! Defined as stddev / sqrt(N) */
                    float voltageError;

                    /*! Uncorrected voltage reading */
                    float rawVoltage;
                    /*! Uncorrected voltage Min reading */
                    float rawVoltageMin;
                    /*! Uncorrected voltage Max reading */
                    float rawVoltageMax;
                };

                ///@{
                /*! @name ADC Measurement
                 *
                 * Returns an adcData struct containing statistical information
                 * measured from any combination of ADC 0 or 1, on channels 0-10.
                 *
                 * The mapping of occupied ADC channels is:
                 *      <UL>
                 *      <LI> Encoders: ADC0, Channels 0-5
                 *      <LI> Onboard temperature sensor: ADC0, Channel 6
                 *      <LI> External temperature sensor: ADC0, Channel 7
                 *      <LI> Auxially sensors: ADC1, Channel 5-6
                 *      <LI> LOW-, MID-, and HIGH- point voltage references: Channels 9-11
                 *      </UL>
                 *
                 */

                /*! @brief Measure from ADC channel using the global default number of samples.
                 *  @param adc Select ADC 0 or 1
                 *  @param channel Select ADC channel 0-11
                 */
                adcData measure(int adc_num, int channel) { return measure(adc_num, channel, m_defaultSamples); }
                /*! @brief Measure from ADC channel with a specified number of samples
                 *  @param adc Select ADC 0 or 1
                 *  @param channel Measure from ADC channel 0-11
                 *  @param nsamples Number of samples to take.
                 */
                adcData measure(int adc, int channel, int nsamples);
                ///@}

                ///@{
                /*! @name Measure encoder voltage
                */

                /*! @brief Read encoder with global default number of ADC samples.
                 *  @param encoder Select from encoders 1-6
                 */
                adcData readEncoder (int iencoder);
                /*! @brief Read encoder with specified number of ADC samples.
                 *  @param encoder Select from encoders 1-6
                 *  @param nsamples Number of ADC Samples to average
                 */
                adcData readEncoder (int iencoder, int nsamples);
                float readEncoderVoltage (int iencoder);
                ///@}


                ///@{
                /*! @name Temperature Sensor Readout
                */

                float readTemperature();

                float readTemperature(int nsamples);

                /*! @brief Read Onboard temperature sensor voltage with default number of samples. */
                adcData readTemperatureVolts  ();
                /*! @brief Read Onboard temperature sensor voltage with configurable number of samples.
                 *  @param nsamples Number of ADC Samples to average*/
                adcData readTemperatureVolts  (int nsamples);

                /*! @brief Read External temperature sensor voltage with default number of samples. */
                adcData readExternalTemp ();
                /*! @brief Read External temperature sensor voltage with configurable number of samples.
                 *  @param nsamples Number of ADC Samples to average*/
                adcData readExternalTemp (int nsamples);
                ///@}

                ///@{
                /*! @name ADC Reference Voltage Readout
                */

                /*! @brief Returns low point voltage reference, nominally 0V, using a default number of ADC samples */
                adcData readRefLow  (int adc);
                /*! @brief Returns low point voltage reference, nominally 0V, using a configurable number of ADC samples
                 *  @param nsamples Number of ADC Samples to average */
                adcData readRefLow  (int adc, int nsamples);

                /*! @brief Returns midpoint voltage reference, nominally (RefLow + RefHigh)/2, using a default number of ADC samples. */
                adcData readRefMid  (int adc);
                /*! @brief Returns midpoint voltage reference, nominally (RefLow + RefHigh)/2, using a configurable number of ADC samples.
                 *  @param nsamples Number of ADC Samples to average */
                adcData readRefMid  (int adc, int nsamples);

                /*! @brief Returns High point voltage reference, nominally 5V, using a default number of ADC samples. */
                adcData readRefHigh (int adc);
                /*! @brief Returns High point voltage reference, nominally 5V, using a configurable number of ADC samples
                 *  @param nsamples Number of ADC Samples to average */
                adcData readRefHigh (int adc, int nsamples);

                ///@}

                ///@{
                /*! @name ADC Read Delay
                 *
                 *  Controls a delay which is inserted between successive ADC reads, to slow down
                 *  measurement.
                 */
                /*! @brief Returns current ADC read delay. */
                int  getReadDelay();
                /*! @brief Sets ADC read delay (in uncalibrated for-loop cycles).
                 *  @param delay Integer delay, counted in "number of for-loop cycles" */
                void setReadDelay(int delay);
                ///@}

                ///@{
                /*! @name Default ADC Samples
                 *
                 * Control the default number of ADC samples to average in a measurement.
                 */
                /*! @brief Set default number of samples.
                 *  @param nsamples Number of ADC Samples to average. */
                void setDefaultSamples(int nsamples);
                /*! @brief Returns the current default number of samples. */
                int getDefaultSamples() { return m_defaultSamples; }
                ///@}

                float getEncoderTemperatureSlope  ( int iencoder ) ;
                float getEncoderTemperatureOffset ( int iencoder ) ;
                float getEncoderTemperatureRef    (              ) ;
                float getEncoderVoltageSlope      ( int iencoder ) ;
                float getEncoderVoltageOffset     ( int iencoder ) ;

                void  setEncoderTemperatureSlope  ( int iencoder, float slope  ) ;
                void  setEncoderTemperatureOffset ( int iencoder, float offset ) ;
                void  setEncoderTemperatureRef    ( float reference            ) ;
                void  setEncoderVoltageSlope      ( int iencoder, float slope  ) ;
                void  setEncoderVoltageOffset     ( int iencoder, float offset ) ;


            explicit ADC(CBC *cbc);

            private:
                CBC *cbc;
                int m_readDelay;
                int m_defaultSamples;

                float m_encoderTemperatureOffset [6];
                float m_encoderTemperatureSlope  [6];
                float m_encoderTemperatureRef       ;
                float m_encoderVoltageOffset     [6];
                float m_encoderVoltageSlope      [6];
        } adc;

        //////////////////////////////////////////////////////////////////////////////
        ///Auxillary Sensors Control
        //////////////////////////////////////////////////////////////////////////////

        /*! Controls power on/off to the auxillary sensor connections
         *
         *  These connectors are two three-pin headers which provide GND, +5V, and a connection
         *  to the ADC (specifically, channels 5 and 6 on the second ADC). */
        struct AUXsensor {
            public:
                /*! Enable power to auxillary sensors */
                void enable();
                /*! Disable power to auxillary sensors */
                void disable();
                /*! Returns status of power to auxillary sensors */
                bool isEnabled();

            explicit AUXsensor(CBC *cbc);
            private:
                CBC *cbc;
        } auxSensor;


        /* delay inserted after enabling, before stepping, before
         * disabling, before reading encoders */
        int m_delay; // in milliseconds
    virtual void setDelayTime(int delay);

    virtual int getDelayTime();
};

/*!
 * The DummyCBC class is responsible for the control of all mirror control board functions.
 *
 * This is a dummy class with no functionality meant to be a drop-in replacement for the real thing
 * to emulate DummyCBC on an AMD64 machine.
 */
class DummyCBC : public CBC {
public:
    static struct Config dummy_config;

    void configure(struct Config config) override {};

    explicit DummyCBC(struct Config config = DummyCBC::dummy_config) : usb(this), driver(this), encoder(this),
                                                                       adc(this),
                                                                       auxSensor(this), m_delay() {
        configure(std::move(config));
    };

    ~DummyCBC() = default;

    struct USB {
        void enable(int usb_num) {};

        void enableAll() {};

        void disable(int usb_num) {};

        void disableAll() {};

        bool isEnabled(int usb_num) { return false; };

        explicit USB(DummyCBC *thisDummyCBC) : cbc(thisDummyCBC) {};
    private:
        DummyCBC *cbc;

    } usb;

    struct Driver {
    public:
        void setMicrosteps(int microsteps) {};

        int getMicrosteps() { return 0; };

        void enable(int drive) {};

        void enableAll() {};

        void disable(int drive) {};

        void disableAll() {};

        bool isEnabled(int drive) { return false; };

        void sleep() {};

        void wakeup() {};

        bool isAwake() { return false; };

        void reset() {};

        void enableHighCurrent() {};

        void disableHighCurrent() {};

        bool isHighCurrentEnabled() { return false; };

        void enableSR() {};

        void disableSR() {};

        bool isSREnabled() { return false; };

        void step(int drive, int nsteps) {};

        void step(int drive, int nsteps, int frequency) {};

        int getSteppingFrequency() { return 0; };

        void setSteppingFrequency(int frequency) {};

        explicit Driver(DummyCBC *thisDummyCBC) : cbc(thisDummyCBC) {};
    private:
        DummyCBC *cbc;
    } driver;

    struct Encoder {

        explicit Encoder(DummyCBC *thisDummyCBC) : cbc(thisDummyCBC) {};
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

        adcData measure(int adc_num, int channel) { return dummyData; };

        adcData measure(int adc_num, int channel, int nsamples) { return dummyData; };

        adcData readEncoder(int iencoder) { return dummyData; };

        adcData readEncoder(int iencoder, int nsamples) { return dummyData; };

        float readEncoderVoltage(int iencoder) { return 0; };

        float readTemperature() { return 0; };

        float readTemperature(int nsamples) { return 0; };

        adcData readTemperatureVolts() { return dummyData; };

        adcData readTemperatureVolts(int nsamples) { return dummyData; };

        adcData readExternalTemp() { return dummyData; };

        adcData readExternalTemp(int nsamples) { return dummyData; };

        adcData readRefLow(int adc_num) { return dummyData; };

        adcData readRefLow(int adc_num, int nsamples) { return dummyData; };

        adcData readRefMid(int adc_num) { return dummyData; };

        adcData readRefMid(int adc_num, int nsamples) { return dummyData; };

        adcData readRefHigh(int adc_num) { return dummyData; };

        adcData readRefHigh(int adc_num, int nsamples) { return dummyData; };

        int getReadDelay() { return 0; };

        void setReadDelay(int delay) {};

        void setDefaultSamples(int nsamples) {};

        int getDefaultSamples() { return 0; };

        float getEncoderTemperatureSlope(int iencoder) { return 0; };

        float getEncoderTemperatureOffset(int iencoder) { return 0; };

        float getEncoderTemperatureRef() { return 0; };

        float getEncoderVoltageSlope(int iencoder) { return 0; };

        float getEncoderVoltageOffset(int iencoder) { return 0; };

        void setEncoderTemperatureSlope(int iencoder, float slope) {};

        void setEncoderTemperatureOffset(int iencoder, float offset) {};

        void setEncoderTemperatureRef(float reference) {};

        void setEncoderVoltageSlope(int iencoder, float slope) {};

        void setEncoderVoltageOffset(int iencoder, float offset) {};


        explicit ADC(DummyCBC *thisDummyCBC) : cbc(thisDummyCBC), dummyData(adcData()) {};
    private:
        DummyCBC *cbc;
    } adc;

    struct AUXsensor {
    public:
        bool isEnabled() { return false; };

        explicit AUXsensor(DummyCBC *thisDummyCBC) : cbc(thisDummyCBC) {};
    private:
        DummyCBC *cbc;
    } auxSensor;


    int m_delay; // in milliseconds
    void setDelayTime(int delay) override {};

    int getDelayTime() override { return 0; };
};

#endif //CBCCODE_CBC_HPP
