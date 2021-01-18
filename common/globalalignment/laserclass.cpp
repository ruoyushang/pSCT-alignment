#include "common/globalalignment/laserclass.h"

const std::vector<Device::ErrorDefinition> GASLaserBase::ERROR_DEFINITIONS{
        {"Laser Operable Error", Device::ErrorState::OperableError},
        {"Laser Fatal Error", Device::ErrorState::FatalError},
};

void GASLaserBase::setState(bool state) {
    spdlog::info("GASLaser::setPower() = {} - no effect", state);
}

void GASLaserBase::setPower(bool state) {
    spdlog::info("GASLaser::setPower() = {} - no effect", state);
}

void GASLaserBase::turnOff() {
    spdlog::info("GASLaser::turnOff() - no effect");
}

void GASLaserBase::turnOn() {
    spdlog::info("GASLaser::turnOn() - no effect");
}

#ifndef SIMMODE
void GASLaser::setState(bool state) {
    if (m_isOn == state) return;

    int laser = 0;
    m_relay->setState(laser, state);

    m_isOn = state;
}

void GASLaser::setPower(bool state) {
    if (m_isPowered == state) return;

    int power = 1;
    m_relay->setState(power, state);

    m_isPowered = state;
}

bool GASLaser::initialize() {
    spdlog::info("GASLaser::initialize() - no effect");
    // TODO tie this to GASPSD so that init is not repeated
    return true;
}

void GASLaser::turnOff() {
    setPower(false);
    setState(false);
    spdlog::info("GASLaser::turnOff()");
}

void GASLaser::turnOn() {
    setPower(true);
    setState(true);
    spdlog::info("GASLaser::turnOn()  ");
    spdlog::warn("TURN LASER OFF AFTER 4 HOURS TO AVOID OVERHEATING");
}

#endif

bool DummyGASLaser::initialize() {
    spdlog::info("DummyGASLaser::initialize() - no effect");
    return true;
}


