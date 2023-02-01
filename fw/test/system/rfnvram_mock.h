
#ifndef SYSTEM_RFNVRAM_MOCK_H
#define SYSTEM_RFNVRAM_MOCK_H

// Standard headers
#include <memory>
#include <string>
#include <unordered_map>
#include <queue>

// Mock headers
#include "alt_types_mock.h"
#include "memory_mock.h"
#include "unordered_map_memory_mock.h"
#include "rfnvram_utils.h"

// PFR system
#include "pfr_sys.h"

typedef enum
{
    IDLE = 1,
    WRITE_ADDR_UPPER = 2,
    WRTIE_ADDR_LOWER = 3,
    WRITE_DATA = 4,
    READ_DATA = 5
} state_t;

typedef enum
{
    IDLE_M = 1,
    WRITE_DATA_M = 2,
} state_m;

class RFNVRAM_MOCK : public MEMORY_MOCK_IF
{
public:
    static RFNVRAM_MOCK* get();

    void reset() override;

    alt_u32 get_mem_word(void* addr) override;
    void set_mem_word(void* addr, alt_u32 data) override;

    bool is_addr_in_range(void* addr) override;

    RFNVRAM_MOCK();
    virtual ~RFNVRAM_MOCK();

private:
    // Memory area for SMBus relays
    UNORDERED_MAP_MEMORY_MOCK<U_RFNVRAM_SMBUS_MASTER_BASE, U_RFNVRAM_SMBUS_MASTER_SPAN> m_ram;
    char rfnvram_mem[RFNVRAM_INTERNAL_SIZE];
    std::queue<alt_u32> m_cmd_fifo;
    std::queue<alt_u8> m_read_fifo;
    void simulate_i2c();
    void simulate_mctp_i2c();
    state_t state = IDLE;
    state_m state_2 = IDLE_M;

    // internal addr is 10 bits wide written in 2 x 1 byte transactions
    int internal_addr = 0;
    // SMBus slave port ID
    // Default 0 means slave is rfnvram
    alt_u32 smbus_port = 0;

    // MCTP packet support
    // Control register bit
    alt_u32 smbus_speed = 0;
    alt_u32 pec_enabled = 0;
    alt_u32 smbus_soft_reset = 0;

    // Status register bit
    alt_u32 fsm_status = 0;
    alt_u32 pec_byte_nack = 0;
    alt_u32 data_byte_nack = 0;
    alt_u32 address_byte_nack = 0;

#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)

    // The assumption is that the max size of a single mctp message is 1 K byte
    // These array stores the spdm message sent from cpld whether cpld is a responder/requester
    alt_u8 bmc_mem[1024];
    alt_u8 pch_mem[1024];
    alt_u8 pcie_mem[1024];

    alt_u8 bmc_internal_addr = 0;
    alt_u8 pch_internal_addr = 0;
    alt_u8 pcie_internal_addr = 0;

#endif
};

#endif /* SYSTEM_RFNVRAM_MOCK_H_ */
