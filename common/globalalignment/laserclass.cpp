#include "common/globalalignment/laserclass.h"
#include "common/globalalignment/piusb.hpp"

void GASLaser::setState(bool state)
{
    if (m_isOn == state) return;

    int laser = 0;
    Relay relay;
    relay.setState(laser, state);

    m_isOn = state;
}

void GASLaser::setPower(bool state)
{
    if (m_isPowered == state) return;

    int power = 1;
    Relay relay;
    relay.setState(power, state);

    m_isPowered = state;
}
