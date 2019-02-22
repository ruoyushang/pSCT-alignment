/*
 * Interface to access GPIO Moduels of Overo EarthSTORM COM, Texas Instruments
 * AM3703 CPU Directly accesses CPU Physical Memory at /dev/mem by mapping into
 * virtual address space.
 */

#ifndef GPIOINTERFACE_H
#define GPIOINTERFACE_H

#include <sys/types.h>
#include <stdint.h>
#include <sys/mman.h>
#include <Layout.hpp>

class GPIOInterface
{
public:
    GPIOInterface();
    ~GPIOInterface();

    // Read GPIO by ipin (0-191)
    bool ReadLevel(int ipin);

    // Write GPIO by ipin (0-191)
    void WriteLevel(int ipin, bool level);

    // Get GPIO Direction In/Out
    bool GetDirection(int ipin);

    // Set GPIO Direction In/Out (0-191)
    void SetDirection(int ipin, bool dir);

    // Configure Input/Output directions for ALL GPIOs
    void ConfigureAll();

private:
    // Functions to return pointers to mapped GPIO registers
    volatile uint32_t* ptrGPIOReadLevel(int ipin);
    volatile uint32_t* ptrGPIODirection(int ipin);
    volatile uint32_t* ptrGPIOSetLevel(int ipin);

    void SetLevel(int ipin);
    void ClrLevel(int ipin);

    // --------------------------------------------------------------------------
    // GPIO register (PHYSICAL) Address Definitions
    // --------------------------------------------------------------------------

    const off_t ADR_GPIO1_BASE            = 0x48310000;
    const off_t ADR_GPIO2_BASE            = 0x49050000;
    const off_t ADR_GPIO3_BASE            = 0x49052000;
    const off_t ADR_GPIO4_BASE            = 0x49054000;
    const off_t ADR_GPIO5_BASE            = 0x49056000;
    const off_t ADR_GPIO6_BASE            = 0x49058000;

    //const off_t OFF_GPIO_CTRL          = 0x030;
    const off_t OFF_GPIO_OE            = 0x034;  //enable the pins output capabilities. Its only function is to carry the pads configuration.
    const off_t OFF_GPIO_DATAIN        = 0x038;  //register the data that is read from the GPIO pins
    const off_t OFF_GPIO_DATAOUT       = 0x03C;  //setting the value of the GPIO output pins

    // --------------------------------------------------------------------------
    // Functions to Return Addresses for GPIO pins
    // --------------------------------------------------------------------------

    // Adds register offset to correct base address to produce physical address
    off_t offset2adrGPIO(int ipin, off_t offset);

    // Returns Virtual (memory mapped) address for a given GPIO pin
    volatile uint32_t* phys2VirtGPIO32(off_t phys, int ipin);

    // Takes Physical Address and Returns pointer to memory mapped virtual address
    volatile uint32_t* phys2Virt32(off_t phys, volatile void* map_base_virt, off_t map_base_phys);

    // Returns physical address of GPIO Read Register for a given GPIO pin
    off_t physGPIOReadLevel(int ipin);

    // Returns physical address of Output Enable Register for a given GPIO pin
    off_t physGPIODirection(int ipin);

    // Returns physical address of Output Write Register for a given GPIO pin
    off_t physGPIOSetLevel(int ipin);

    // Create a virtual addressing space for a given physical address
    void makeMap(volatile void* &virtual_addr, off_t physical_addr, size_t length=4096);

    // Memory mapped file descriptor
    int             m_mmap_fd;

    // Base addresses of memory mapped address space
    volatile void*  m_gpio1_base;
    volatile void*  m_gpio2_base;
    volatile void*  m_gpio3_base;
    volatile void*  m_gpio4_base;
    volatile void*  m_gpio5_base;
    volatile void*  m_gpio6_base;

    uint32_t MaskPin (int ipin);
};
#endif
