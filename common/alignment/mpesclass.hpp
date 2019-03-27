#ifndef __MPESCLASS_H__
#define __MPESCLASS_H__

#include <cstring>
#include <fstream>
#include <set>
#include <string>

#ifndef _AMD64
    #include "common/cbccode/cbc.hpp"
#else
    #include "common/cbccode/dummycbc.hpp"
    #define CBC DummyCBC
#endif

#include "common/alignment/actuator.hpp"
#include "common/mpescode/MPESImageSet.h"
#include "common/mpescode/MPESDevice.h"

class MPES
{
    public:
        struct Position {
            float xCenter;
            float yCenter;
            float xStdDev;
            float yStdDev;
            float cleanedIntensity;
            float xNominal;
            float yNominal;
        };

        MPES(std::shared_ptr<CBC> pCBC, int USBPortNumber, int serialNumber);
        ~MPES();

        int getPortNumber() const { return m_USBPortNumber; };
        void setPortNumber(int USBPortNumber);

        virtual bool initialize();
        virtual int setExposure();

        void setxNominalPosition(float x) { m_Position.xNominal = x; }
        void setyNominalPosition(float y) { m_Position.yNominal = y; }

        virtual int measurePosition();

        MPES::Position getPosition() const { return m_Position; };

    protected:
        std::shared_ptr<CBC> m_pCBC;

        int m_USBPortNumber;
        int m_SerialNumber;

        bool m_Calibrate;

        Position m_Position; // MPES Reading

        // helpers
        std::shared_ptr<MPESImageSet> m_pImageSet;
        std::unique_ptr<MPESDevice> m_pDevice;
        std::set<int> getVideoDevices();

        // Hardcoded constants
        static const int DEFAULT_IMAGES_TO_CAPTURE;
        static const std::string MATRIX_CONSTANTS_DIR_PATH;
        static const std::string CAL2D_CONSTANTS_DIR_PATH;

        static const float SAFETY_REGION_X_MIN;
        static const float SAFETY_REGION_X_MAX;
        static const float SAFETY_REGION_Y_MIN;
        static const float SAFETY_REGION_Y_MAX;
};

class DummyMPES : public MPES
{
public:
    DummyMPES(std::shared_ptr<CBC> pCBC, int USBPortNumber, int serialNumber) : MPES(std::shared_ptr<CBC> pCBC, int USBPortNumber, int serialNumber) {};

    bool initialize();
    int setExposure();
    int measurePosition();
};

#endif
