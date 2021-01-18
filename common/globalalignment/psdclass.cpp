#include "common/globalalignment/psdclass.hpp"

#include <errno.h>
#include <termios.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <sys/ioctl.h>
#include <limits>
#include <random>
#include <chrono>
#include <unistd.h>

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"


const std::vector<Device::ErrorDefinition> GASPSDBase::ERROR_DEFINITIONS = {
        {"Error opening port.",            Device::ErrorState::FatalError},//error 0
        {"Error from tcgetattr.",          Device::ErrorState::FatalError},//error 1
        {"Error from tcsetattrr.",         Device::ErrorState::FatalError},//error 2
        {"Error from tggetattr.",          Device::ErrorState::FatalError},//error 3
        {"Error setting term attributes.", Device::ErrorState::FatalError}//error 4
};

void GASPSDBase::turnOn() {
    spdlog::info("{}: Turning on - no effect", m_Identity);
    m_On = true;
}

void GASPSDBase::turnOff() {
    spdlog::info("{}: Turning off - no effect", m_Identity);
    m_On = false;
}

bool GASPSDBase::isOn() {
    return m_On;
}

Device::ErrorDefinition GASPSDBase::getErrorCodeDefinition(int errorCode) {
    return GASPSDBase::ERROR_DEFINITIONS.at(errorCode);
}

void GASPSDBase::setCalibration() {
    spdlog::trace("GASPSD Calibrating");
}

void GASPSDBase::setNominalValues(int offset, double value) {
    spdlog::trace("GASPSD setNominalValues - no effect ");
}


#ifndef SIMMODE
GASPSD::~GASPSD()
{
    m_logOutputStream.close();
}

bool GASPSD::initialize()
{
    int newACMDeviceId = 0;
    spdlog::info("{} : GASPSD::initialize() : Initializing...", m_Identity);
    // we toggle the usb port, checking the ACM devices when it's off and again when it's on.

    m_Errors.assign(getNumErrors(), false);

    m_pCBC->usb.disableAll(); // make sure all USBs are off
    std::set<int> oldACMDevices = getACMDevices(); // count ACM devices

    m_pCBC->usb.enableAll(); // switch the usb back on and wait for the ACM device to show up
    sleep(4);

    std::set<int> newACMDevices = getACMDevices(); // check all ACM devices again

    std::set<int> toggledDevices;
    std::set_difference(newACMDevices.begin(), newACMDevices.end(), oldACMDevices.begin(), oldACMDevices.end(),
                    std::inserter(toggledDevices, toggledDevices.begin()));

    if (!toggledDevices.empty()) {
        newACMDeviceId = *toggledDevices.begin(); // get the only element in the set -- this is the new device ID
        m_usb_port = 1;
    }
    else if (newACMDevices.size()==1) {
        newACMDeviceId = *newACMDevices.begin(); // get the only element in the set -- this is the new device ID
        m_usb_port = newACMDeviceId + 1;
    }

    if (m_usb_port == -1){
        spdlog::error("{} : GASPSD::initialize() : Found 0 ACM devices, should be exactly 1.", m_Identity);
        setError(0); // fatal
        return false;
    }

    spdlog::debug("GASPSD::initialize(): Detected new ACM device {} in port {}.", newACMDeviceId, m_usb_port);

    std::string ACM_port = "/dev/ttyACM";
    ACM_port += std::to_string(newACMDeviceId);

    // set the calibration constants
    setCalibration();

    m_fd = open(ACM_port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (m_fd < 0) {
        spdlog::error("error {} opening {}: {}", errno, ACM_port.c_str(), strerror(errno));
        setError(0);
        return -1;
    }

    setInterfaceAttribs(m_fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    setBlocking(m_fd, 0);                    // set no blocking
    write(m_fd, "d", 1); // debug
    // write(m_fd, "s", 1); // std dev output enabled by default
    write(m_fd, "m", 1); // psd readings

    m_logOutputStream.open(m_logFilename);

    return true;
}

void GASPSD::update() {
    spdlog::debug("GASPSD::update()");
    if (m_fd >= 0) {
        char buf[110];
        char tmp = '\0';
        int new_bytes = 0;

        // align on opening
        int count = 0;

        int lines_read = 0 ;
        int max_lines_read_during_update = 2 * 1;
        bool did_not_read = false;

        // skip lines that start with '#'
        do {
            memset(buf, '\0', sizeof buf);
            // read one byte until hit newline
            do {
                new_bytes = read(m_fd, &tmp, 1);
            } while (tmp != '\n');

            while (tmp != '\r') {
                new_bytes = read(m_fd, &tmp, 1);
                buf[count] = tmp;
                count += new_bytes;
            }
            spdlog::debug("{}\n", buf);
            count = 0;
            lines_read++;
            if (lines_read > max_lines_read_during_update) {
                did_not_read = true;
                break;
            }
        } while (buf[0] == '#');
        if (did_not_read){
            return;
        }

        new_bytes = sscanf(buf, "%lf %lf %lf %lf %lf %lf %lf %lf %lf",
                           &m_data[0], &m_data[1], &m_data[2], &m_data[3],
                           &m_data[4], &m_data[5], &m_data[6], &m_data[7],
                           &m_data[8]);

        for (int i = 0; i < 4; i++) {
            if (m_data[i] < 0) {
                m_data[i] = (m_data[i]*m_AlphaNeg[i]) - m_Beta[i];
                m_data[i + 4] = m_data[i + 4]*m_AlphaNeg[i];
            }
            else {
                m_data[i] = (m_data[i]*m_AlphaPos[i]) - m_Beta[i];
                m_data[i + 4] = m_data[i + 4]*m_AlphaPos[i];
            }
    }

        // log locally
        for (int i = 0; i < 8; i++) {
            m_logOutputStream.precision(5);
            m_logOutputStream << m_data[i] << " ";
        }
        m_logOutputStream.precision(2);
        m_logOutputStream << m_data[8] << std::endl;
    }
}

void GASPSD::setNominalValues(int offset, double value)
{
    spdlog::trace("GASPSD setNominalValues - no effect.");
}

int GASPSD::setInterfaceAttribs(int fd, int speed, int parity) {
    struct termios tty{};
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        spdlog::error("error {} from tcgetattr", errno);
        setError(1);
        return -1;
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 10;            // 1.0 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
    // enable reading
    //tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag &= ~(PARENB);      // shut off parity
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_lflag |= ICANON; // Canonical Mode
    tty.c_iflag |= IGNCR; // Ignore carriage returns

    cfmakeraw (&tty);

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        spdlog::error("error {} from tcsetattr", errno);
        setError(2);
        return -1;
    }

    usleep(200000);
    tcflush(fd, TCIOFLUSH);

    return 0;
}

void GASPSD::setBlocking(int fd, int should_block) {
    struct termios tty{};
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        spdlog::error("error {} from tggetattr", errno);
        setError(3);
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0) {
        spdlog::error("error {} setting term attributes", errno);
        setError(4);
    }
}

void GASPSD::setCalibration() {
    spdlog::trace("GASPSD Calibrating - no effect.");
}


std::set<int> GASPSD::getACMDevices() {
    std::set<int> ACMDevices;

    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir("/dev")) != nullptr) { // Open /dev device directory in filesystem
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != nullptr) {
            std::string currentEntry = ent->d_name;
            size_t pos;
            std::string substringToFind = "ttyACM"; // Locate ACM devices (name including the string "ttyACM")
            if ((pos = currentEntry.find(substringToFind)) != std::string::npos) { // Found ACM device
                pos += substringToFind.length(); // go to immediately after the substring "ACM"
                int deviceNumber = std::stoi(
                        currentEntry.substr(pos)); // grab remaining part of device name to get the device number
                ACMDevices.insert(deviceNumber);
            }
        }
        closedir(dir);
    } else {
        ACMDevices = {-1}; // output signalling failure
    }

    return ACMDevices;
}


void GASPSD::turnOn() {
    spdlog::info("{}: Turning on" ,m_Identity);
    m_On = true;
    if (initialize()){
        spdlog::info("Initialized successfully.");
    }
    else{
        spdlog::error("Failed to initialize.");
        m_pCBC->usb.enableAll();
        spdlog::warn("Enabled all USB anyways...");
    }
}

void GASPSD::turnOff() {
    spdlog::info("{}: Turning off", m_Identity);
    m_pCBC->usb.disableAll();
    m_On = false;
}


#endif

bool DummyGASPSD::initialize() {
    spdlog::debug("Initializing DummyGASPSD");
    // set the calibration constants
    setCalibration();

    m_logOutputStream.open(m_logFilename);

    return true;
}

void DummyGASPSD::update() {
    spdlog::debug("DummyGASPSD::update()");
    std::uniform_real_distribution<double> coordDistribution(0, 100);
    std::uniform_real_distribution<double> stddevDistribution(0, 1);
    std::normal_distribution<double> tempDistribution(20.0, 2.0);
    std::default_random_engine re(std::chrono::system_clock::now().time_since_epoch().count());

    for (int pos = 0; pos < 4; pos++)
        m_data[pos] = coordDistribution(re);
    for (int pos = 4; pos < 8; pos++)
        m_data[pos] = stddevDistribution(re);
    m_data[8] = tempDistribution(re);

    // log locally
    for (int i = 0; i < 8; i++) {
        m_logOutputStream.precision(5);
        m_logOutputStream << m_data[i] << " ";
    }
    m_logOutputStream.precision(2);
    m_logOutputStream << m_data[8] << std::endl;
}

int DummyGASPSD::setInterfaceAttribs(int fd, int speed, int parity) {

    spdlog::debug("DummyGASPSD :: setInterfaceAttribs() - No effect.");
    usleep(200000);
    return 0;
}

void DummyGASPSD::setBlocking(int fd, int should_block) {
    spdlog::debug("DummyGASPSD :: SetBlocking() - No effect.");
}


