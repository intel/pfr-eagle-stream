// Test headers
#include "bsp_mock.h"
#include "ufm_mock.h"

// Code headers

// Static data
UFM_MOCK* UFM_MOCK::s_inst = nullptr;

// Type definitions

// Return the singleton instance of spi flash mock
UFM_MOCK* UFM_MOCK::get()
{
    if (s_inst == nullptr)
    {
        s_inst = new UFM_MOCK();
    }
    return s_inst;
}

// Constructor/Destructor
UFM_MOCK::UFM_MOCK()
{
    m_flash_mem = new alt_u32[U_UFM_DATA_SPAN/4];
}

UFM_MOCK::~UFM_MOCK()
{
    delete[] m_flash_mem;
}

// Class methods

void UFM_MOCK::reset()
{
    // SPI flash contains all FFs when empty
    alt_u32* flash_mem_ptr =  m_flash_mem;
    for (int i = 0; i < U_UFM_DATA_SPAN / 4; i++)
    {
        flash_mem_ptr[i] = 0xffffffff;
    }
}

