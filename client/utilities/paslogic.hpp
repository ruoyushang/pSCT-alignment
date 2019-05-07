#ifndef __PASLOGIC_H__
#define __PASLOGIC_H__

/*************************
* PAS main logic thread
*************************/

#include "uathread.h"
#include "stewartplatform.h"
#include "common/alignment/device.hpp"
#include "pascommunicationinterface.hpp"
#include <string>
#include <chrono>
#include <iostream>
#include <fstream>

// output to two streams at once to enable stdout and logging
class tee_device
{
    public:
    explicit tee_device(std::ostream &os2) : m_os2(os2) {}

        // handle std::endl
        tee_device& operator<<(std::ostream& (*manip)(std::ostream &)) {
            manip(std::cout);
            manip(m_os2);
            return *this;
        }

        template<class T>
            tee_device& operator<<(const T& x) {
                std::cout << x;
                m_os2 << x;
                return *this;
            }

    private:
        std::ostream& m_os2;
};

class PasLogic : public UaThread
{
    public :
    explicit PasLogic(PasCommunicationInterface *pCommIf);

    ~PasLogic() override = default;

    void run() override;
        void stop() {m_terminate = true; wait();}
    private:
        // this is to get all the data from our devices 
        PasCommunicationInterface *m_pCommIf;

        // this tells when to stop!
        bool m_terminate;
        // this keep track if aligned
        bool m_aligned;

        // this is to compute platform motion
        StewartPlatform m_SP;

        // align primary OT through PSD readings 
        void __align_PSD(unsigned psdNo = 0);
        UaStatus __move_OT(unsigned otNo, const double coords[6]);
        bool __PSDmisaligned(unsigned psdNo = 0, bool verbose = true);
        const double m_kPSDDistance;
        const double m_kMisalignmentCriterion;
        const double m_kAngularScale[2];
        double m_nominalPSDReadings[4];
        // align secondary OT through PSD readings, after the primary has been aligned
        bool m_secondaryAligned;
        bool __telescopeMoving();
        void __readPSD(double readings[4]);
        float __telescopeEl();
        const std::chrono::time_point<std::chrono::system_clock> m_kInitTime;
        std::string __timeStamp();

        std::ofstream m_logstream;
        tee_device tee;

    Device::Identity m_positionerId;
    Device::Identity m_psdId;
    Device::Identity m_optId[2];
};

#endif // __PASLOGIC_H__
