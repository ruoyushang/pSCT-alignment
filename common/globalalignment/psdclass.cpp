#include "common/globalalignment/psdclass.h"
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <iostream>
#include <sys/ioctl.h>
#include <limits>

GASPSD::~GASPSD()
{
    if (m_fdOut) {
        fclose(m_fdOut);
        m_fdOut = nullptr;
    }
}


void GASPSD::setPort(std::string port)
{
    m_Portname = port;
}

int GASPSD::Initialize(bool isSim)
{
    if ( (m_isSim = isSim) ) return 0;

    // set the calibration constants
    set_calibration(); 

    m_fd = open(m_Portname.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (m_fd < 0) {
        printf ("error %d opening %s: %s", errno, m_Portname.c_str(), strerror (errno));
        return -1;
    }

    set_interface_attribs (m_fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
    set_blocking (m_fd, 0);                    // set no blocking
    write(m_fd, "d", 1); // debug
    // write(m_fd, "s", 1); // std dev output enabled by default
    write(m_fd, "m", 1); // psd readings

    m_fdOut = fopen(m_fOut.c_str(), "w");

    return 0;
}

void GASPSD::Update()
{
    if (!m_isSim && (m_fd >= 0) ) {
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
            printf("%s\n", buf);
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
        for (int i = 0; i < 8; i++)
           fprintf(m_fdOut,"%9.5f ", m_data[i]);
        fprintf(m_fdOut, "%5.2f\n", m_data[8]);
    }

    else
        simulate(true);
        //simulate(m_fd >= 0);
}

void GASPSD::simulate(bool hasPSD)
{

    if (hasPSD)
    {
        for (int pos = 0; pos < 4; pos++)
            m_data[pos] = static_cast<double>(rand() % 10000) / 100.;
        for (int pos = 4; pos < 8; pos++)
            m_data[pos] = static_cast<double>(rand() % 100) / 100.;
        m_data[9] = 10. + static_cast<double>(rand() % 20);
    }
    else
    {
        for (int pos = 0; pos < 9; pos++)
            m_data[pos] = std::numeric_limits<int>::min();
    }
}

void GASPSD::setNominalValues(int offset, double value)
{

    return;
}

int GASPSD::set_interface_attribs (int fd, int speed, int parity)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        printf ("error %d from tcgetattr", errno);
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
        printf ("error %d from tcsetattr", errno);
        return -1;
    }

    usleep(200000);
    tcflush(fd, TCIOFLUSH);

    return 0;
}

void GASPSD::set_blocking (int fd, int should_block)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        printf ("error %d from tggetattr", errno);
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
        printf ("error %d setting term attributes", errno);
}

void GASPSD::set_calibration()
{   /*
    m_AlphaNeg[0] = -9.909807;
    m_AlphaNeg[1] = 10.103729;
    m_AlphaNeg[2] = -9.794860;
    m_AlphaNeg[3] = 10.016967;

    m_AlphaPos[0] = -9.906882;
    m_AlphaPos[1] = 10.142797;
    m_AlphaPos[2] = -9.771347;
    m_AlphaPos[3] = 9.933996;

    m_Beta[0] = 0.007212;
    m_Beta[1] = 0.100659;
    m_Beta[2] = 0.021057;
    m_Beta[3] = 0.098714;

    m_Theta[0] = 0.;
    m_Theta[1] = 0.;
    */

    m_AlphaNeg[0] = -9.710132;
    m_AlphaNeg[1] = 9.871400;
    m_AlphaNeg[2] = -9.604298;
    m_AlphaNeg[3] = 10.071177;

    m_AlphaPos[0] = -9.618614;
    m_AlphaPos[1] = 9.658531;
    m_AlphaPos[2] = -9.382091;
    m_AlphaPos[3] = 9.934893;

    m_Beta[0] = 0.011341;
    m_Beta[1] = 0.118089;
    m_Beta[2] = 0.023689;
    m_Beta[3] = 0.116461;

    m_Theta[0] = 0.;
    m_Theta[1] = 0.;

}
