#include "rangefinderclass.hpp"

bool GASRangeFinder::isOn() {

}

Device::ErrorDefinition GASRangeFinder::getErrorCodeDefinition(int errorCode) {

}

int GASRangeFinder::getNumErrors() {

}

void GASRangeFinder::turnOff() {

}

void GASRangeFinder::turnOn() {

}

bool GASRangeFinder::initialize() {
    return true;
}

bool DummyGASRangeFinder::initialize() {
    return GASRangeFinder::initialize();
}
