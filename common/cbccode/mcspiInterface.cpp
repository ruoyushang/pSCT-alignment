//------------------------------------------------------------------------------
// Opens CPU Physical Memory at /dev/mem
// Maps physical memory into virtual address space (unix command mmap)
//------------------------------------------------------------------------------

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>

#include "common/cbccode/mcspiInterface.hpp"

#define DEBUG 0
#define debug_print(fmt, ...) \
    do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#define MMAPFAIL ((void*)-1)

#define MUNMAP(VIRT) munmap(const_cast<void*>(VIRT), 4096)
#define MUNMAP(VIRT) \
    munmap(const_cast<void*>(VIRT), 4096)

#define BIT(nr)                  (1UL << (nr))

#define ENABLE_INTERFACE_CLOCK   BIT(18)
#define ENABLE_FUNCTIONAL_CLOCK  BIT(18)
#define RESETDONE                BIT(0)
#define CHANNEL_ENABLE           BIT(0)
#define MASTER_SLAVE             BIT(2)
#define PHASE                    BIT(0)
#define POLARITY                 BIT(1)
#define CLOCK_DIVIDER            (0xF << 2)
#define EPOL                     BIT(6)
#define CS_POLARITY              BIT(6)
#define SPI_WORD_LENGTH          (0x1F << 7)
#define SPI_TRANSFER_MODE        (0x7 << 12)
#define INPUT_SELECT             BIT(18)
#define TX_EN0                   BIT(16)
#define TX_EN1                   BIT(17)
#define FORCE                    BIT(20)

#define SYSCONFIG_SOFTRESET      BIT(1)
#define SYSCONFIG_AUTOIDLE       BIT(0)
#define SYSCONFIG_ENAWAKEUP      BIT(2)
#define SYSCONFIG_SMARTIDLE      BIT(4)

#define WAKEUPENABLE            BIT(0)

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
        __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)

//------------------------------------------------------------------------------
// Public Members
//------------------------------------------------------------------------------

// constructor
mcspiInterface::mcspiInterface() :
    cm_fclken1_core   (NULL),
    cm_iclken1_core   (NULL),
    mcspi_sysconfig   (NULL),
    mcspi_sysstatus   (NULL),
    mcspi_wakeupenable(NULL),
    mcspi_modulctrl   (NULL),
    mcspi_chconf      (NULL),
    mcspi_chstat      (NULL),
    mcspi_chctrl      (NULL),
    mcspi_tx          (NULL),
    mcspi_rx          (NULL),
    mcspi_irqstatus   (NULL),
    mcspi_irqenable   (NULL),
    m_mmap_fd(-1),
    m_cm_core_base(NULL),
    m_mcspi1_base(NULL)
    //m_padconf(),
{
    debug_print("%s\n", "Start of MCSPI Constructor");


    // open /dev/mem and check for failure
    m_mmap_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if(m_mmap_fd<0)
    {
        perror("open(\"/dev/mem\")");
        exit(EXIT_FAILURE);
    }

    //      virtual adr     physical adr
    m_mcspi1_base  = makeMap(ADR_MCSPI_BASE);
    m_cm_core_base = makeMap(ADR_CM_CORE_BASE);


    cm_fclken1_core    = mappedAddress(OFF_CM_FCLKEN1_CORE, m_cm_core_base);
    cm_iclken1_core    = mappedAddress(OFF_CM_ICLKEN1_CORE, m_cm_core_base);

    mcspi_sysconfig    = mappedAddress(OFF_MCSPI_SYSCONFIG , m_mcspi1_base);
    mcspi_sysstatus    = mappedAddress(OFF_MCSPI_SYSSTATUS , m_mcspi1_base);
    mcspi_wakeupenable = mappedAddress(OFF_MCSPI_WAKEUPENABLE , m_mcspi1_base);
    mcspi_modulctrl    = mappedAddress(OFF_MCSPI_MODULCTRL, m_mcspi1_base);
    mcspi_chconf       = mappedAddress(OFF_MCSPI_CHCONF, m_mcspi1_base);
    mcspi_chstat       = mappedAddress(OFF_MCSPI_CHSTAT, m_mcspi1_base);
    mcspi_chctrl       = mappedAddress(OFF_MCSPI_CHCTRL, m_mcspi1_base);
    mcspi_tx           = mappedAddress(OFF_MCSPI_TX, m_mcspi1_base);
    mcspi_rx           = mappedAddress(OFF_MCSPI_RX, m_mcspi1_base);
    mcspi_irqstatus    = mappedAddress(OFF_MCSPI_IRQSTATUS, m_mcspi1_base);
    mcspi_irqenable    = mappedAddress(OFF_MCSPI_IRQENABLE, m_mcspi1_base);

    //makeMap(m_padconf,      ADR_PADCONF_BASE);
    debug_print("%s\n", "End of MCSPI Constructor");

    Reset();
}

// destructor
mcspiInterface::~mcspiInterface()
{
    munmap(m_mcspi1_base, 4096);
    munmap(m_cm_core_base, 4096);
    //MUNMAP(m_padconf);
}

/* Initializes MCSPI Controller: Enables clocks, and performs soft-reset of the
 * MCSPI system
 */

void* mcspiInterface::makeMap(off_t target)
{
    void *map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m_mmap_fd, target & ~MAP_MASK);
    if (map_base == (void*) -1)
        exit(EXIT_FAILURE);

    void *virt_address = static_cast<char *>(map_base) + (target & MAP_MASK);

    return virt_address;
}

volatile uint32_t* mcspiInterface::mappedAddress(off_t offset, void* mappedBaseAddress)
{
    void* mappedAddress = static_cast<char *>(mappedBaseAddress) + offset;
    return reinterpret_cast<volatile uint32_t*>(mappedAddress);
}


void mcspiInterface::EnableClocks()
{
    /*  Clock Enable
     *  Bit 18 0x1 = enable, 0x0 = disable
     */

    /* interface clock */
    *cm_iclken1_core |= ENABLE_INTERFACE_CLOCK;

    /* functional clock */
    *cm_fclken1_core |= ENABLE_FUNCTIONAL_CLOCK;
}

void mcspiInterface::DisableClocks()
{
    /*  Clock Enable
     *  Bit 18 = EN_MCSPI1
     *  Bit 19 = EN_MCSPI2
     *  Bit 20 = EN_MCSPI3
     *  Bit 21 = EN_MCSPI4
     *  0x1 = enable, 0x0 = disable
     */

    /* interface clock */
    *cm_iclken1_core &= ~ENABLE_INTERFACE_CLOCK;

    /* functional clock */
    *cm_fclken1_core &= ~ENABLE_FUNCTIONAL_CLOCK;
}

void mcspiInterface::Reset()
{
    /*  Reset Sequence:
     *  enable Clocks
     *  Write softreset
     *  wait for reset to finish
     *  set autoidle+enawakeup+smartidle
     *
     *  write wakeupenable
     *
     *  set mode to master
     *  disable clocks */
    debug_print("%s\n", "Start Enable Clocks");
    EnableClocks();
    debug_print("%s\n", "Finished Enabling Clocks");

    debug_print("Reset:mcspi_sysconfig%s\n", "");
    *mcspi_sysconfig |= SYSCONFIG_SOFTRESET;
    debug_print("%s\n", "before while loop");

    // SPIm.MCSPI_SYSSTATUS[0] will be set to 1 when the reset is finished
    // wait until it is 1..
    while (true)
    {
        debug_print("%s\n", "waiting while true");
        if ( (*mcspi_sysstatus & RESETDONE)==1 )
            break;
    }

    debug_print("%s\n", "sysconfig");
    *mcspi_sysconfig |= (SYSCONFIG_AUTOIDLE | SYSCONFIG_ENAWAKEUP | SYSCONFIG_SMARTIDLE);

    debug_print("%s\n", "wakeup");
    *mcspi_wakeupenable |= WAKEUPENABLE;

    debug_print("%s\n", "Set Master Mode");
    SetMasterMode();

    debug_print("%s\n", "Disable Clocks");
    //DisableClocks();
}

void mcspiInterface::EnableChannel()
{
    // Start Channel
    *mcspi_chctrl |= CHANNEL_ENABLE;
}

void mcspiInterface::DisableChannel()
{
    //Stop Channel
    *mcspi_chctrl &= ~CHANNEL_ENABLE;
}

void mcspiInterface::SetMasterMode()
{
    *mcspi_modulctrl &= ~MASTER_SLAVE;

    // Manage CS with force
    //*mcspi_modulctrl |= 0x1;
}

void mcspiInterface::ConfigureInterruptMode()
{
    setPhase(1);    //mode 1 --> polarity=0 phase=1
    setPolarity(0);

    setClockDivider(1); // 1 == 24 MHz
    setEPOL(1);
    setSPIMPolarity(1);
    setWordLength(0xF);
    setTransferMode(0);
    setInputSelect (0); // set to 0 for the spi1_somi pin in receive mode

    /*     2. Set the SPI1.MCSPI_CHxCONF[17] DPE1 bit to 0 and the
     *     SPI1.MCSPI_CHxCONF[16] DPE0 bit to 1 for the spi1.simo pin in
     *     transmit mode.
     */
    setDPE01(1,0);
    setChipSelectTimeControl(0x3);
    setTXfifoEnable(0);
    setRXfifoEnable(0);
    //setClockDividerGranularity (0);
}

uint16_t mcspiInterface::WriteRead(uint16_t data)
{
    return((uint16_t) WriteReadInterruptMode(data));
}

uint32_t mcspiInterface::WriteReadInterruptMode(uint32_t data)
{
    ///* 20.6.2.6.3 Programming in Interrupt Mode
    // * This section follows the flow of Figure 20-26.
    // * 1. Initialize software variables: WRITE_COUNT = 0 and READ_COUNT = 0.
    // */
    int         write_count = 0;
    int         read_count  = 0;
    int         nwrite      = 1;
    int         nread       = 1;

    uint32_t    readdata    = 0x0;

    bool finished_reading = false;
    while (!finished_reading) {
        debug_print("%s\n", "Start Reset");
        //Reset();
        debug_print("%s\n", "End Reset");

        ///* 2. Initialize interrupts: Write 0x7 in the SPI1.MCSPI_IRQSTATUS[3:0]
        //*     field and set the SPI1.MCSPI_IRQENABLE[3:0] field to 0x7.
        //*/
        *mcspi_irqenable &= ~0x7;
        *mcspi_irqenable |=  0x7;

        debug_print("%s\n", "irqenabled");

        *mcspi_irqstatus &= ~0x7;
        *mcspi_irqstatus |=  0x7;

        debug_print("%s %08x\n", "irqstatus updated", *mcspi_irqstatus);

        /* 3. Follow the steps described in Section 20.6.2.6.2.1.1, Mode Selection.  */
        ConfigureInterruptMode();

        /* Clock Initialization and spi1_cs0 Enable
         * In master mode, the SPI must provide the clock and enable the channel:
         * Set the SPI1.MCSPI_MODULCTRL[2] MS bit to 0 to provide the clock.
         */
        SetMasterMode();

        /*     Set the INPUTENABLE bit of the corresponding CONTROL_PADCONF_x
         *     register to achieve the synchronous clock by using loopback through
         *     the output and input buffers of the pad.
         *
         *     CONTROL_PADCONF_MCSPI1_CLK[15:0]
         *     0x4800 21C8
         *     Section 13.4.4
         *     Bit 8 == Input Enable
         */

        //this should be uncommented
        //*mcspiPadConf |= 0x1 << 8;


        /* Set the SPI1.MCSPI_CHxCTRL[0] EN bit to 1 (where x = 0) to enable channel 0. */
        EnableChannel();

        // assert !CS
        //*mcspi_chconf &= ~(FORCE);

        /* 4. If WRITE_COUNT = w and READ_COUNT = w, write SPI1.MCSPI_CHxCTRL[0] = 0x0 (x = 0) to stop
         *    the channel.  This interrupt routine follows the flow of Table
         *    20-16 and Figure 20-28.
         *      1. Read the SPI1.MCSPI_IRQSTATUS[3:0] field.
         *      2. If the SPI1.MCSPI_IRQSTATUS[0] TX0_EMPTY bit is set to 1:
         */

        int read_attempts = 0;
        while (write_count < nwrite)
        {
            /*  2. If the SPI1.MCSPI_IRQSTATUS[0] TX0_EMPTY bit is set to 1: */
            if ((*mcspi_irqstatus & 0x1)==1)
            {
                //printf("Writing Data: %04x\n", data);
                /* (a) Write the command/address or data value in SPI1.MCSPI_TXx (where x = 0). */
                *mcspi_tx = data;
                /* (b) WRITE_COUNT + = 1 */
                write_count++;
                /* (c) Write SPI1.MCSPI_IRQSTATUS[0] = 0x1. */
                *mcspi_irqstatus |= 0x1;
            }
        }

        while (read_count < nread)
        {
            //usleep(1);
            //printf("Waiting for read\n");
            /* 3. If the SPI1.MCSPI_IRQSTATUS[2] RX0_FULL bit is set to 1: */
            if (((*mcspi_irqstatus >> 2) & 0x1)==1)
            {
                /* a) Read SPI1.MCSPI_RXx (where x = 0) */
                //readdata = (*mcspi_rx >> 16) & 0xFFFF;
                readdata = (*mcspi_rx & 0xFFFF);
                //printf("Read Data: %04x\n",readdata);
                /* b) READ_COUNT += 1 */
                read_count++;
                /* c) Write SPI1.MCSPI_IRQSTATUS[2] = 0x1 */
                *mcspi_irqstatus |= (0x1 << 2);
                finished_reading = true;
            }
            else {
                read_attempts += 1;
                if (read_attempts > 5) {
                    debug_print("%s\n","Stalled... reset");
                    write_count = 0;
                    break;
                }
            }
        }
        DisableChannel();
    }

    return readdata;

    // deassert !CS
    //*mcspi_chconf |= (FORCE);
}

//bool mcspiInterface::txFifoFull ()
//{
//    //Read 0x0: FIFO Transmit Buffer is not full
//    //Read 0x1: FIFO Transmit Buffer is full
//    return (0x1 & (*ptrMCSPI_chstat() >> 4));
//}
//
void mcspiInterface::setPhase(int phase) {
    *mcspi_chconf &= ~(PHASE);
    if (phase) {
        *mcspi_chconf |=  (PHASE);        //mcspi_chxconf[0]
    }
}
void mcspiInterface::setPolarity(int polarity)
{
    *mcspi_chconf &= ~(POLARITY);
    if (polarity)
    {
        *mcspi_chconf |=  (POLARITY); //mcspi_chxconf[1]
    }
}

void mcspiInterface::setClockDivider(int divider)
{
    divider &= 0xF;
    *mcspi_chconf &= ~(CLOCK_DIVIDER);         //mcspi_chxconf[5..2]
    *mcspi_chconf |= (divider << 2);            //mcspi_chxconf[5..2]
}

//spim_csx polarity
//0x0 => spim_csx high during active state
//0x1 => spim_csx low  during active state
void mcspiInterface::setSPIMPolarity(int polarity)
{
    *mcspi_chconf &= ~(CS_POLARITY);
    *mcspi_chconf |=  (polarity << 6); //mcspi_chxconf[6]
}
/*     5. Set the SPI1.MCSPI_CHxCONF[6] EPOL bit to 1 for spi1_cs0 activated low
 *     during active state.  clock.
 */
void mcspiInterface::setEPOL(int epol) {
    *mcspi_chconf &= ~EPOL;
    *mcspi_chconf |= epol;
}
//set spi word length
void mcspiInterface::setWordLength(int length)
{
    length &= 0xF;
    int spi_word_length = length << 7;  //16 bit word length
    *mcspi_chconf &= ~(SPI_WORD_LENGTH);
    *mcspi_chconf |=  (spi_word_length); //mcspi_chxconf[11..7]
}

void mcspiInterface::setTransferMode(int mode)
{
    mode &= 0x3;
    mode = mode << 12;
    *mcspi_chconf &= ~(SPI_TRANSFER_MODE);
    *mcspi_chconf |=  (mode); //mcspi_chxconf[13..12]
}

void mcspiInterface::setInputSelect (int input_select)
{
    *mcspi_chconf &= ~(INPUT_SELECT);
    *mcspi_chconf |= (input_select << 18);
}

void mcspiInterface::setDPE01 (int dpe0, int dpe1)
{
    dpe0 &= 0x1;
    dpe0  = dpe0 << 16;

    dpe1 &= 0x1;
    dpe1  = dpe1 << 17;

    *mcspi_chconf &= ~(TX_EN1);
    *mcspi_chconf &= ~(TX_EN0);
    *mcspi_chconf |=  (dpe0);
    *mcspi_chconf |=  (dpe1);
}

void mcspiInterface::setChipSelectTimeControl (int time)
{
    time &= 0x3;
    *mcspi_chconf &= ~(0x3 << 25);
    *mcspi_chconf |=  (time << 25);
}

void mcspiInterface::setTXfifoEnable (bool enable)
{
    int tx_fifo_enable = enable << 27;
    *mcspi_chconf &= ~(0x1 << 27);
    *mcspi_chconf |= tx_fifo_enable;
}

void mcspiInterface::setRXfifoEnable (bool enable)
{
    int rx_fifo_enable = enable << 27;
    *mcspi_chconf &= ~(0x1 << 28);
    *mcspi_chconf |= rx_fifo_enable;
}

void mcspiInterface::setClockDividerGranularity (int granularity)
{
    int clock_divider_granularity = granularity << 29;
    *mcspi_chconf &= (~0x1 << 29);
    *mcspi_chconf |= clock_divider_granularity;
}

void mcspiInterface::forceCS(bool state)
{
    if (state)
        *mcspi_chconf |=  FORCE;
    if (!state)
        *mcspi_chconf &= ~FORCE;
}
