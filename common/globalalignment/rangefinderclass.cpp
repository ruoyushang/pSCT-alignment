#include "rangefinderclass.hpp"

const std::vector<Device::ErrorDefinition> GASRangeFinderBase::ERROR_DEFINITIONS{
        {"Rangefinder Operable Error", Device::ErrorState::OperableError},
        {"Rangefinder Fatal Error", Device::ErrorState::FatalError}
};

void GASRangeFinderBase::turnOff() {
    spdlog::info("GASRangeFinder::turnOff() - no effect");
}

void GASRangeFinderBase::turnOn() {
    spdlog::info("GASRangeFinder::turnOn() - no effect");
}

#ifndef SIMMODE
bool GASRangeFinder::initialize() {
    int newUSBDeviceId = 0;
    spdlog::info("{} : GASRangeFinder::initialize() : Initializing...", m_Identity);
    // we toggle the usb port, checking the USB devices when it's off and again when it's on.

    m_Errors.assign(getNumErrors(), false);

    for (int test_usb_port = 1; test_usb_port < 7; ++test_usb_port) {

        m_pCBC->usb.disable(test_usb_port); // make sure our USB is off
        std::set<int> oldUSBDevices = getUSBDevices(); // count USB devices

        m_pCBC->usb.enable(test_usb_port); // switch the usb back on and wait for the USB device to show up
        sleep(4);

        std::set<int> newUSBDevices = getUSBDevices(); // check all USB devices again

        std::set<int> toggledDevices;
        std::set_difference(newUSBDevices.begin(), newUSBDevices.end(), oldUSBDevices.begin(), oldUSBDevices.end(),
                            std::inserter(toggledDevices, toggledDevices.begin()));

        if (toggledDevices.size() == 1) {
            newUSBDeviceId = *toggledDevices.begin(); // get the only element in the set -- this is the new device ID
            m_usb_port = test_usb_port;
            break;
        }
    }
    if (m_usb_port == -1){
        spdlog::error("{} : GASRangeFinder::initialize() : Found 0 USB devices, should be exactly 1.", m_Identity);
        setError(0); // fatal
        return false;
    }

    spdlog::debug("GASRangeFinder::initialize(): Detected new USB device {} in port {}.", newUSBDeviceId, m_usb_port);

    m_pCBC->usb.enable(m_usb_port);

    std::string USB_port = "/dev/ttyUSB";
    USB_port += std::to_string(newUSBDeviceId);

    return true;
}

std::set<int> GASRangeFinder::getUSBDevices() {
    std::set<int> USBDevices;

    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir("/dev")) != nullptr) { // Open /dev device directory in filesystem
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != nullptr) {
            std::string currentEntry = ent->d_name;
            size_t pos;
            std::string substringToFind = "ttyUSB"; // Locate USB devices (name including the string "USB")
            if ((pos = currentEntry.find(substringToFind)) != std::string::npos) { // Found USB device
                pos += substringToFind.length(); // go to immediately after the substring "USB"
                int deviceNumber = std::stoi(
                        currentEntry.substr(pos)); // grab remaining part of device name to get the device number
                USBDevices.insert(deviceNumber);
            }
        }
        closedir(dir);
    } else {
        USBDevices = {-1}; // output signalling failure
    }

    return USBDevices;
}

void GASRangeFinder::turnOff() {
    spdlog::info("GASRangeFinder::turnOff() - no effect");
}

void GASRangeFinder::turnOn() {
    spdlog::info("GASRangeFinder::turnOn() - no effect");
}
#endif

bool DummyGASRangeFinder::initialize() {
    spdlog::info("GASRangeFinder::initialize() - no effect");
    return true;
}
