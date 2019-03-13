#ifndef _PSDCLASS_H_
#define _PSDCLASS_H_

#include <string>

class GASPSD
{
    public:
        GASPSD() : m_fdOut(nullptr) {}; // nothing to do -- everything is set in Initialize()
        ~GASPSD();
        int Initialize(bool isSim = false);
        void setPort(std::string port = "/dev/ttyACM0");
        double getOutput(int offset) { return m_data[offset]; };
        void setNominalValues(int offset, double value);
        const char *getName() const { return "none"; };
        int getSerial() const { return -1; };
        void Update();

    private:
        int set_interface_attribs(int fd, int spede, int parity);
        void set_blocking(int fd, int should_block);
        void set_calibration();

        void simulate(bool hasCamera = true); // simulates camera

        // calibration constants
        double m_AlphaNeg[4]; // {x1Neg, y1Neg, x2Neg, y2Neg}
        double m_AlphaPos[4]; // {x1Pos, y1Pos, x2Pos, y2Pos}
        double m_Beta[4]; // {{x1, y1, x2, y2}
        double m_Theta[2]; // {x, y} ? seemingly unused

        std::string m_configFile;
        bool m_isSim; // set if this is a simulation;
        
        std::string m_Portname;
        int m_fd; // file descriptor to access the serial device
        std::string m_fOut = "PSD-Cam.log"; // file to log into
        FILE * m_fdOut;

        double m_data[9]; // x1, y1, x2, y2, dx1, dy1, dx2, dy2, temperature
};

#endif
