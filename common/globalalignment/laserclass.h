#ifndef _LASERCLASS_H_
#define _LASERCLASS_H_

#include "common/alignment/device.hpp"

class GASLaser : public virtual Device
{
    public:
        // no constructors
        void setState(bool state);
        void setPower(bool state);
        bool isOn() override {return m_isOn; };
        bool isPowered() {return m_isPowered; };

    private:
        bool m_isOn;
        bool m_isPowered;
};

#endif
