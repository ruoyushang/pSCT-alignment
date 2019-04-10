#ifndef __MPESCLASS_H__
#define __MPESCLASS_H__

#ifndef _AMD64
#include "common/cbccode/cbc.hpp"
#else

#include "common/cbccode/dummycbc.hpp"
    #define CBC DummyCBC
#endif

#include "common/mpescode/MPESImageSet.h"
#include "actuator.hpp"
#include "common/mpescode/MPESDevice.h"
#include <string>
#include <fstream>
#include <cstring>
#include <set>

class MPES
{
    public:
        MPES();
        MPES(CBC* input_cbc, int input_USBPortNumber, int input_MPES_ID);
        ~MPES();

        void setUSBPortNumber(int input_USBPortNumber);
        virtual bool Initialize();
        virtual int setExposure();

        virtual int MeasurePosition();
        struct Position {
            float xCenter;
            float yCenter;
            float xStdDev;
            float yStdDev;
            float CleanedIntensity;
            float xNominal;
            float yNominal;
        };
        // return const reference to the private member
        const MPES::Position& getPosition() const { return m_position; };

    int setxNominalPosition(float x);

    int setyNominalPosition(float y);

        int GetPortNumber() const {return m_USBPortNumber; };

    protected:
        CBC* m_pCBC;

        int m_USBPortNumber;
        int m_serialNumber;

        static int sDefaultImagesToCapture;
        static std::string sDefaultDirToSave;
        bool calibrate;
        static std::string matFileString;
        static std::string calFileString;

        // MPES Reading
        Position m_position;
        // helpers
        MPESImageSet *m_pImageSet;
        MPESDevice *m_pDevice;
        std::set<int> __getVideoDevices();

        // sanity
        float Safety_Region_x_min;
        float Safety_Region_x_max;
        float Safety_Region_y_min;
        float Safety_Region_y_max;
};
class DummyMPES : public MPES
{
        public:
        DummyMPES();
        DummyMPES(CBC* input_cbc, int input_USBPortNumber, int input_MPES_ID) : MPES(input_cbc, input_USBPortNumber, input_MPES_ID) {};
        bool Initialize();
        int setExposure();
        int MeasurePosition();

};

#endif
