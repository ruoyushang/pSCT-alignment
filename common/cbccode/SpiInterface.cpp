#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>

#include <linux/spi/spidev.h>
#include "common/cbccode/SpiInterface.hpp"

const char*     SpiInterface::device = "/dev/spidev1.0";

SpiInterface::SpiInterface(): mode(SPI_MODE_1), bits(16), speed(8000000),
    delay(0)
{
}

SpiInterface::~SpiInterface()
{
    close(fd);
}

void SpiInterface::pabort(const char *s)
{
    perror(s);
    abort();
}

uint32_t SpiInterface::transfer(uint32_t data)
{

    uint32_t tx []= {data};
    uint32_t rx []= {0};

    struct spi_ioc_transfer tr;
    memset( (void *) & tr, 0, sizeof(struct spi_ioc_transfer));
    tr.tx_buf           = (unsigned long) tx;
    tr.rx_buf           = (unsigned long) rx;
    tr.len              = 4;
    tr.delay_usecs      = delay;
    tr.speed_hz         = speed;
    tr.bits_per_word    = bits;


    ioctl(fd, SPI_IOC_MESSAGE(1), &tr);

    uint32_t read = rx[0];

    return read;
}

void SpiInterface::Configure ()
{
    fd = open(device, O_RDWR);
    int ret = 0;

    //printf("\nOpening SPI device %s", device);
    fd = open(device, O_RDWR);
    if (fd < 0)
        pabort("spidev driver error: can't open spi device");

    // set spi mode
    //printf("\nSetting SPI Mode 0x%02X: ", mode);
    ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
    if (ret == -1)
        pabort("spidev driver error: can't set spi mode");

    // read back spi mode
    ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
    //printf("\nReading SPI Mode 0x%02X: ", mode);
    if (ret == -1)
        pabort("spidev driver error: can't get spi mode");

    // set bits per word
    ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    if (ret == -1)
        pabort("spidev driver error: can't set bits per word");

    ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    if (ret == -1)
        pabort("spidev driver error: can't get bits per word");

    // set speed in hertz
    ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        pabort("spidev driver error: can't set max speed hz");

    ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if (ret == -1)
        pabort("spidev driver error: can't get max speed hz");

    //printf("spi mode: %d\n", mode);
    //printf("bits per word: %d\n", bits);
    //printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
}

uint32_t SpiInterface::WriteRead (uint32_t data)
{
    uint32_t read = transfer(data);
    return (read);
}
