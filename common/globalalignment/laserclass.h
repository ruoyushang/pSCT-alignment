#ifndef _LASERCLASS_H_
#define _LASERCLASS_H_

class GASLaser
{
    public:
        // no constructors
        void setState(bool state);
        void setPower(bool state);
        bool isOn() {return m_isOn; };
        bool isPowered() {return m_isPowered; };

    private:
        bool m_isOn;
        bool m_isPowered;
};

#endif
