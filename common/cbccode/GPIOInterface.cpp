#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <cstdio>
#include <stdio.h>
#include <GPIOInterface.hpp>

#define MMAPFAIL ((void*)-1)

#define MUNMAP(VIRT) \
    munmap(const_cast<void*>(VIRT), 4096)

//------------------------------------------------------------------------------
// Constructor + Destructor
//------------------------------------------------------------------------------

GPIOInterface::GPIOInterface():
    m_mmap_fd(-1),
    m_gpio1_base(),
    m_gpio2_base(),
    m_gpio3_base(),
    m_gpio4_base(),
    m_gpio5_base(),
    m_gpio6_base()
{

    m_mmap_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if(m_mmap_fd<0)
    {
        perror("open(\"/dev/mem\")");
        exit(EXIT_FAILURE);
    }

    //      virtual adr   physical adr
    makeMap(m_gpio1_base, ADR_GPIO1_BASE);
    makeMap(m_gpio2_base, ADR_GPIO2_BASE);
    makeMap(m_gpio3_base, ADR_GPIO3_BASE);
    makeMap(m_gpio4_base, ADR_GPIO4_BASE);
    makeMap(m_gpio5_base, ADR_GPIO5_BASE);
    makeMap(m_gpio6_base, ADR_GPIO6_BASE);
}

GPIOInterface::~GPIOInterface()
{
    // Unmap everything
    MUNMAP(m_gpio1_base);
    MUNMAP(m_gpio2_base);
    MUNMAP(m_gpio3_base);
    MUNMAP(m_gpio4_base);
    MUNMAP(m_gpio5_base);
    MUNMAP(m_gpio6_base);
}

//------------------------------------------------------------------------------
// Public Members
//------------------------------------------------------------------------------

bool GPIOInterface::ReadLevel(int ipin)
{
    bool level = *(ptrGPIOReadLevel(ipin)) & MaskPin(ipin);
    //printf("Read %i from pin %i",level,ipin);
    return level;
}

void GPIOInterface::WriteLevel(int ipin, bool level)
{
    if (level)
        SetLevel(ipin);
    else
        ClrLevel(ipin);
}

bool GPIOInterface::GetDirection(int ipin)
{
    volatile uint32_t* reg = ptrGPIODirection(ipin);
    uint32_t val = *reg;
    bool dir = !!(val & MaskPin(ipin));
    return dir;
}

void GPIOInterface::SetDirection(int ipin, bool dir)
{
    volatile uint32_t* reg = ptrGPIODirection(ipin);
    if(dir==1)
        *reg |= (MaskPin(ipin));
    else
        *reg &= ~(MaskPin(ipin));
    //printf("\ngpioSetDirection :: Writing %04X", val);
}

void GPIOInterface::ConfigureAll()
{
    FILE * configfile;
    configfile = fopen (".gpioconf","w");

    for (int i=0; i<192; i++)
    {
        int config = Layout::gpioConfiguration(i);
        if (config==0)   // output
        {
            fprintf(configfile,"Configuring gpio %i as output\n", i);
            SetDirection(i, 0);
        }
        if (config==1)    // input
        {
            fprintf(configfile,"Configuring gpio %i as input\n", i);
            SetDirection(i, 1);
        }
    }
    fclose(configfile);
}

//------------------------------------------------------------------------------
// Private Members
//------------------------------------------------------------------------------

inline volatile uint32_t* GPIOInterface::ptrGPIODirection(int ipin)
{
    return phys2VirtGPIO32(physGPIODirection(ipin),ipin);
}

inline volatile uint32_t* GPIOInterface::ptrGPIOSetLevel(int ipin)
{
    return phys2VirtGPIO32(physGPIOSetLevel(ipin),ipin);
}

inline volatile uint32_t* GPIOInterface::ptrGPIOReadLevel(int ipin)
{
    return phys2VirtGPIO32(physGPIOReadLevel(ipin),ipin);
}

inline void GPIOInterface::SetLevel(int ipin)
{
    // Write a One to the bit specified by ipin
    *(ptrGPIOSetLevel(ipin)) = *(ptrGPIOReadLevel(ipin)) | MaskPin(ipin);
}

inline void GPIOInterface::ClrLevel(int ipin)
{
    // Write a zero to the bit specified by ipin
    *(ptrGPIOSetLevel(ipin)) = *(ptrGPIOReadLevel(ipin)) & ~(MaskPin(ipin));
}

volatile uint32_t* GPIOInterface::phys2VirtGPIO32(off_t physical_addr, int ipin)
{
    if (ipin<32)
        return phys2Virt32(physical_addr,m_gpio1_base,ADR_GPIO1_BASE);
    else if (ipin<64)
        return phys2Virt32(physical_addr,m_gpio2_base,ADR_GPIO2_BASE);
    else if (ipin<96)
        return phys2Virt32(physical_addr,m_gpio3_base,ADR_GPIO3_BASE);
    else if (ipin<128)
        return phys2Virt32(physical_addr,m_gpio4_base,ADR_GPIO4_BASE);
    else if (ipin<160)
        return phys2Virt32(physical_addr,m_gpio5_base,ADR_GPIO5_BASE);
    else if (ipin<192)
        return phys2Virt32(physical_addr,m_gpio6_base,ADR_GPIO6_BASE);
    else
        return 0;
}

inline volatile uint32_t* GPIOInterface::phys2Virt32(off_t phys, volatile void* map_base_virt, off_t map_base_phys)
{
    // map offset is the difference between the physical address and the physical base address
    off_t adr_offset = phys - map_base_phys;

    // virtual address the base of the virtual address + the adr_offset with
    // some fancy typecasting we static cast the void pointer map_base_virt
    // into a pointer to an 8 bit integer...  then offset that pointer by some
    // number of bytes, then reinterpret the data at that memory as a pointer
    // to a 32bit unsigned integer, and return that pointer!
    volatile uint32_t* adr_virtual = reinterpret_cast<volatile
        uint32_t*>(static_cast<volatile uint8_t*>(map_base_virt) + adr_offset);

    return adr_virtual;
}

off_t GPIOInterface::offset2adrGPIO(int ipin, off_t offset)
{
    if (ipin<0)
        return (0);
    else if (ipin<32)
        return(ADR_GPIO1_BASE+offset);
    else if (ipin<64)
        return(ADR_GPIO2_BASE+offset);
    else if (ipin<96)
        return(ADR_GPIO3_BASE+offset);
    else if (ipin<128)
        return(ADR_GPIO4_BASE+offset);
    else if (ipin<160)
        return(ADR_GPIO5_BASE+offset);
    else if (ipin<192)
        return(ADR_GPIO6_BASE+offset);
    else
        return(0);
}

off_t GPIOInterface::physGPIOReadLevel(int ipin)
{
    return offset2adrGPIO(ipin,OFF_GPIO_DATAIN);
}

off_t GPIOInterface::physGPIODirection(int ipin)
{
    return offset2adrGPIO(ipin,OFF_GPIO_OE);
}

off_t GPIOInterface::physGPIOSetLevel(int ipin)
{
    return offset2adrGPIO(ipin, OFF_GPIO_DATAOUT);
}

void GPIOInterface::makeMap(volatile void*& virtual_addr, off_t physical_addr, size_t length)
{
    virtual_addr = mmap(0, length, PROT_READ|PROT_WRITE, MAP_SHARED, m_mmap_fd, physical_addr);

    // Error Handling
    if (&virtual_addr==MMAPFAIL)
        exit(EXIT_FAILURE);
}

uint32_t GPIOInterface::MaskPin (int ipin) {
    return (0x1 << (ipin % 32));
}
