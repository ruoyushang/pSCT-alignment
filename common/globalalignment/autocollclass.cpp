//
// Created by Deivid Ribeiro on 4/23/21.
//

#include "autocollclass.hpp"

const std::vector<Device::ErrorDefinition> GASAC::ERROR_DEFINITIONS = {
        {"Bad connection. No device found", Device::ErrorState::FatalError},//error 0
};

void GASAC::turnOn() {
    if (isBusy() || isOn()) {
        spdlog::warn("{} : GASAC::turnOn() : Busy, cannot turn on CCD.", m_Identity.name);
        return;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    pfCamera.reset();
    pfCamThread.reset();
    initialize();
    m_On = true;
}

void GASAC::turnOff() {
    spdlog::info("{} : GASAC :: Turning off power to CCD...", m_Identity.name);
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    pfCamera.reset();
    pfCamThread.reset();
    m_On = false;
}

bool GASAC::isOn() {
    return m_On;
}

bool GASAC::initialize()
{
    m_Errors.assign(getNumErrors(), false);
    //initialize Camera
    //make the string camera_name -> char* char_camera_name
    const char * char_camera_name = fLEDsIn.CCDNAME.empty() ? NULL : fLEDsIn.CCDNAME.c_str();

    int errorTries(0);

    while (!pfCamera || !pfCamera->isReady()) {
        //attempt to initialize camera
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        pfCamera = std::unique_ptr<AravisCamera>(new AravisCamera(char_camera_name));
        if (!pfCamera->isReady()) {
            std::string strout =
                    "GASAC::initialize(): Camera [" + fLEDsIn.CCDNAME + "] reports not ready. Trying again...\n";
            spdlog::warn(strout);

            errorTries++;

            if (errorTries > 3){
                setError(0);
                return false;
            }

            continue;
        }
        if (!pfCamera->setGain(fLEDsIn.CCDGAIN)) {
            std::string strout = "+++ WARNING: GASAC::initialize(): Error setting gain, using stock value\n";
            spdlog::warn(strout);
        }
        if (!pfCamera->setExposure(fLEDsIn.CCDEXP)) {
            std::string strout = "+++ WARNING: GASAC::initialize(): Error setting exposure, using stock value\n";
            spdlog::warn(strout);
        }
    }

    // Create camera working thread 
    // threading necessary for adaptation for multicamera input
    pfCamThread = std::unique_ptr<CamOutThread>(new CamOutThread(pfCamera.get(), &fLEDsIn));

    if (!pfCamThread)
        return false;

    return true;
}

bool GASAC::update() {
    if ( pfCamera->isReady()) {
        return pfCamThread->cycle();
    } else {
        return false;
    }
}

Device::ErrorState GASAC::getErrorState() {
    return Device::getErrorState();
}


bool DummyGASAC::initialize() {
    m_Errors.assign(getNumErrors(), false);
    spdlog::debug("DummyGASAC :: initialize() - should do nothing");
    return true;
}

bool DummyGASAC::update() {
    std::uniform_real_distribution<double> coordDistribution(0, 1000);
    std::default_random_engine re(std::chrono::system_clock::now().time_since_epoch().count());
    sleep(3);

    return true;
}

int DummyGASAC::getSerial() const {
    return 0;
}
void DummyGASAC::turnOn() {
    if (isBusy() || isOn()) {
        spdlog::error("{} : GASAC::turnOn() : Busy, cannot turn on CCD.", m_Identity.name);
        return;
    }
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    m_On = true;
}

void DummyGASAC::turnOff() {
    spdlog::info("{} : GASAC :: Turning off power to CCD...", m_Identity.name);
    Device::CustomBusyLock lock = Device::CustomBusyLock(this);
    m_On = false;
}

bool DummyGASAC::isOn() {
    return m_On;
}