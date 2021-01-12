#include "common/globalalignment/psdclass.h"
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

#include "common/utilities/spdlog/spdlog.h"
#include "common/utilities/spdlog/fmt/ostr.h"

GASPSD::~GASPSD()
{
    m_logOutputStream.close();
}

const std::vector<Device::ErrorDefinition> GASPSD::ERROR_DEFINITIONS = {
        {"Error opening port.",Device::ErrorState::FatalError},//error 0
        {"Error from tcgetattr.",Device::ErrorState::FatalError},//error 1
        {"Error from tcsetattrr.",Device::ErrorState::FatalError},//error 2
        {"Error from tggetattr.",Device::ErrorState::FatalError},//error 3
        {"Error setting term attributes.",Device::ErrorState::FatalError}//error 4
};

bool GASPSD::initialize()
{
    // set the calibration constants
    setCalibration();

    m_fd = open(getPort().c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (m_fd < 0) {
        spdlog::error("error %d opening %s: %s", errno, getPort().c_str(), strerror(errno));
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
    if (m_fd >= 0) {
        char buf[110];
        char tmp = '\0';
        int n = 0;

        // align on opening
        int count = 0;

        // skip lines that start with '#'
        do {
            memset(buf, '\0', sizeof buf);
            // read one byte until hit newline
            do {
                n = read(m_fd, &tmp, 1);
            } while (tmp != '\n');

            while (tmp != '\r') {
                n = read(m_fd, &tmp, 1);
                buf[count] = tmp;
                count += n;
            }
            spdlog::info("%s\n", buf);
            count = 0;
        } while (buf[0] == '#');

        n = sscanf(buf, "%lf %lf %lf %lf %lf %lf %lf %lf %lf",
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
    // Currently no effect
}

int GASPSD::setInterfaceAttribs(int fd, int speed, int parity) {
    struct termios tty{};
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        spdlog::error("error %d from tcgetattr", errno);
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
    tty.c_cc[VTIME] = 20;            // 0.5 seconds read timeout

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
        spdlog::error("error %d from tcsetattr", errno);
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
        spdlog::error("error %d from tggetattr", errno);
        setError(3);
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0) {
        spdlog::error("error %d setting term attributes", errno);
        setError(4);
    }
}

void GASPSD::setCalibration() {
    // Current unused
}

void GASPSD::turnOn() {
    spdlog::info("{}: Turning on" ,m_Identity);
    m_On = true;
}

void GASPSD::turnOff() {
    spdlog::info("{}: Turning off", m_Identity);
    m_On = false;
}

bool GASPSD::isOn() {
    return m_On;
}

Device::ErrorDefinition GASPSD::getErrorCodeDefinition(int errorCode) {
        return GASPSD::ERROR_DEFINITIONS.at(errorCode);
    }

bool DummyGASPSD::initialize() {
    spdlog::debug("Initializing DummyGASPSD");
    // set the calibration constants
    setCalibration();

    m_logOutputStream.open(m_logFilename);

    return true;
}

void DummyGASPSD::update() {
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


