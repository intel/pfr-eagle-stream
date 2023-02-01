#ifndef SYSTEM_UFM_MOCK_H
#define SYSTEM_UFM_MOCK_H

// Standard headers
#include <memory>
#include <string>
#include <unordered_map>
#include <fstream>
#include <string.h>

// Mock headers
#include "alt_types_mock.h"
#include "memory_mock.h"
#include "unordered_map_memory_mock.h"

// PFR system
#include "pfr_sys.h"
#include "ufm.h"


class UFM_MOCK
{
public:
    static UFM_MOCK* get();

    void reset();

    // Expose the x86 addresses of the flash memory
    alt_u32* get_flash_ptr() { return m_flash_mem; }

    void erase_page(alt_u32 addr)
    {
        alt_u32* page_start = m_flash_mem + ((addr / UFM_FLASH_PAGE_SIZE) * UFM_FLASH_PAGE_SIZE) / 4;
        std::fill(page_start, page_start + (UFM_FLASH_PAGE_SIZE / 4), 0xffffffff);
    }

    void erase_sector(alt_u32 sector_id)
    {
        alt_u32* page_start = m_flash_mem + (m_ufm_sector_start_addrs[sector_id - 1] / 4);
        alt_u32* page_end = m_flash_mem + ((m_ufm_sector_end_addrs[sector_id - 1]) / 4);

        std::fill(page_start, page_end, 0xffffffff);
    }

    void write_data(alt_u32 addr, alt_u32* data, alt_u32 nbytes)
    {
        alt_u32* dest_ptr = m_flash_mem + (addr / 4);
        for (alt_u32 i = 0; i < nbytes / 4; i++)
        {
            dest_ptr[i] = data[i];
        }
    }

private:
    // Singleton inst
    static UFM_MOCK* s_inst;

    UFM_MOCK();
    ~UFM_MOCK();

    // Memory area for the flash memory
    alt_u32 *m_flash_mem = nullptr;

    alt_u32 m_ufm_sector_start_addrs[5] = {
            U_UFM_DATA_SECTOR1_START_ADDR,
            U_UFM_DATA_SECTOR2_START_ADDR,
            U_UFM_DATA_SECTOR3_START_ADDR,
            U_UFM_DATA_SECTOR4_START_ADDR,
            U_UFM_DATA_SECTOR5_START_ADDR};

    alt_u32 m_ufm_sector_end_addrs[5] = {
            U_UFM_DATA_SECTOR1_END_ADDR + 1,
            U_UFM_DATA_SECTOR2_END_ADDR + 1,
            U_UFM_DATA_SECTOR3_END_ADDR + 1,
            U_UFM_DATA_SECTOR4_END_ADDR + 1,
            U_UFM_DATA_SECTOR5_END_ADDR + 1};
};

#endif /* SYSTEM_UFM_MOCK_H */
