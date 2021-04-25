//
// Created by Deivid Ribeiro on 4/23/21.
//

#include "autocollclass.hpp"

#include <utility>

const std::vector<Device::ErrorDefinition> GASAC::ERROR_DEFINITIONS = {
        {"Bad connection. No device found", Device::ErrorState::FatalError},//error 0
};

void GASAC::setConfig(string config) {
    fConfigFile = std::move(config);

    // constant for most LEDs possible
    const int MAXLED = 8;

    // Set LEDsIn configuration
    std::ifstream fin(fConfigFile.c_str());
    string line;
    while (std::getline(fin, line)) {
        if (line[0] != '#') {
            // set sin to the value after the equality sign
            std::istringstream sin(line.substr(line.find("=") + 1));
            // check what exactly we're reading
            if (line.find("CCDNAME") != string::npos)
                pfACin.CCDNAME = sin.str();
            else if (line.find("CCDACTIVE") != string::npos)
                sin >> pfACin.CCDACTIVE;
            else if (line.find("CCDGAIN") != string::npos)
                sin >> pfACin.CCDGAIN;
            else if (line.find("CCDEXP") != string::npos)
                sin >> pfACin.CCDEXP;
            else if (line.find("CCDHEIGHT") != string::npos)
                sin >> pfACin.CCDHEIGHT;
            else if (line.find("CCDWIDTH") != string::npos)
                sin >> pfACin.CCDWIDTH;
            else if (line.find("TEMP") != string::npos)
                sin >> pfACin.TEMP;
            else if (line.find("PIXSIZE") != string::npos)
                sin >> pfACin.PIXSIZE;
            else if (line.find("LENSFL") != string::npos)
                sin >> pfACin.LENSFL;
            else if (line.find("LENSSCALE") != string::npos)
                sin >> pfACin.LENSSCALE;
            else if (line.find("CCDMAC") != string::npos)
                sin >> pfACin.CCDMAC;
            else if (line.find("THRESHOLD") != string::npos)
                sin >> pfACin.THRESHOLD;
            else if (line.find("SAVEIMAGE") != string::npos)
                sin >> pfACin.SAVEIMAGE;
            else if (line.find("SAVEFORMAT") != string::npos)
                sin >> pfACin.SAVEFORMAT;
            else if (line.find("VERBOSE") != string::npos)
                sin >> pfACin.VERBOSE;
            else if (line.find("CAPTUREONLY") != string::npos)
                sin >> pfACin.CAPTUREONLY;
        } //end if not a # comment
    } // end while loop
    fin.close();
}

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

bool GASAC::initialize() {
    m_Errors.assign(getNumErrors(), false);
    //initialize Camera
    //make the string camera_name -> char* char_camera_name
    const char *char_camera_name = pfACin.CCDNAME.empty() ? nullptr : pfACin.CCDNAME.c_str();

    int errorTries(0);

    while (!pfCamera || !pfCamera->isReady()) {
        //attempt to initialize camera
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        pfCamera = std::unique_ptr<AravisCamera>(new AravisCamera(char_camera_name));
        if (!pfCamera->isReady()) {
            std::string strout =
                    "GASAC::initialize(): Camera [" + pfACin.CCDNAME + "] reports not ready. Trying again...\n";
            spdlog::warn(strout);

            errorTries++;

            if (errorTries > 3) {
                setError(0);
                return false;
            }

            continue;
        }
        if (!pfCamera->setGain(pfACin.CCDGAIN)) {
            std::string strout = "+++ WARNING: GASAC::initialize(): Error setting gain, using stock value\n";
            spdlog::warn(strout);
        }
        if (!pfCamera->setExposure(pfACin.CCDEXP)) {
            std::string strout = "+++ WARNING: GASAC::initialize(): Error setting exposure, using stock value\n";
            spdlog::warn(strout);
        }
    }

    // Create camera working thread 
    // threading necessary for adaptation for multicamera input
    pfCamThread = std::unique_ptr<ACCamOutThread>(new ACCamOutThread(pfCamera.get(), &pfACin));

    if (!pfCamThread)
        return false;

    return true;
}

bool GASAC::update() {
    if (pfCamera->isReady()) {
        return pfCamThread->cycle();
    } else {
        return false;
    }
}

Device::ErrorState GASAC::getErrorState() {
    return Device::getErrorState();
}

GASAC::Position GASAC::getPosition() {
    return m_pPosition;
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