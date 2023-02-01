
#ifndef SYSTEM_MAILBOX_MOCK_H
#define SYSTEM_MAILBOX_MOCK_H

// Standard headers
#include <memory>
#include <string>
#include <unordered_map>
#include <queue>

// Mock headers
#include "alt_types_mock.h"
#include "memory_mock.h"
#include "unordered_map_memory_mock.h"

// PFR system
#include "pfr_sys.h"

class MAILBOX_MOCK : public MEMORY_MOCK_IF
{
public:

    MAILBOX_MOCK();
    virtual ~MAILBOX_MOCK();
    void reset() override;

    alt_u32 get_mem_word(void* addr) override;
    void set_mem_word(void* addr, alt_u32 data) override;

    bool is_addr_in_range(void* addr) override;

private:
    // Memory area for SMBus Mailbox Register File
    UNORDERED_MAP_MEMORY_MOCK<U_MAILBOX_AVMM_BRIDGE_BASE, U_MAILBOX_AVMM_BRIDGE_SPAN>
        m_mailbox_reg_file;

    std::queue<alt_u8> m_fifo;
    void flush_fifo();
#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)

    std::queue<alt_u8> m_mctp_write_fifo;
    std::queue<alt_u8> m_mctp_pch_fifo;
    std::queue<alt_u8> m_mctp_bmc_fifo;
    std::queue<alt_u8> m_mctp_pcie_fifo;
    std::queue<alt_u8> m_bmc_byte_count_fifo;
    std::queue<alt_u8> m_pch_byte_count_fifo;
    std::queue<alt_u8> m_pcie_byte_count_fifo;

    alt_u32 pch_avail_error_flag[50] = {0};
    alt_u32 bmc_avail_error_flag[50] = {0};
    alt_u32 pcie_avail_error_flag[50] = {0};

    alt_u32 m_bmc_ls4b_avail[50] = {0};
    alt_u32 m_pch_ls4b_avail[50] = {0};
    alt_u32 m_pcie_ls4b_avail[50] = {0};
    alt_u32 m_bmc_ms4b_error[50] = {0};
    alt_u32 m_pch_ms4b_error[50] = {0};
    alt_u32 m_pcie_ms4b_error[50] = {0};

    void flush_mctp_pch_fifo();
    void flush_mctp_bmc_fifo();
    void flush_mctp_pcie_fifo();
    void flush_mctp_write_fifo();

    alt_u32 m_bmc_fifo_counter = 1;
    alt_u32 m_pch_fifo_counter = 0;
    alt_u32 m_pcie_fifo_counter = 0;

    alt_u32 m_bmc_pkt_no = 0;
    alt_u32 m_pch_pkt_no = 0;
    alt_u32 m_pcie_pkt_no = 0;

    alt_u32 m_bmc_finish_transaction = 0;
    alt_u32 m_pch_finish_transaction = 0;
    alt_u32 m_pcie_finish_transaction = 0;

    alt_u32 m_bmc_error_track_bit = 0;
    alt_u32 m_pch_error_track_bit = 0;
    alt_u32 m_pcie_error_track_bit = 0;

    alt_u32 m_bmc_byte_count = 0;
    alt_u32 m_pch_byte_count = 0;
    alt_u32 m_pcie_byte_count = 0;

    alt_u32 m_bmc_pkt_error = 0;
    alt_u32 m_pch_pkt_error = 0;
    alt_u32 m_pcie_pkt_error = 0;

    alt_u32 m_bmc_byte_cnt_tracker = 0;
    alt_u32 m_pch_byte_cnt_tracker = 0;
    alt_u32 m_pcie_byte_cnt_tracker = 0;

    // Host types
    alt_u32 m_bmc = 255;
    alt_u32 m_pch = 254;
    alt_u32 m_pcie = 253;

    // Staged next message
    alt_u32 m_staged_idx = 0;
    alt_u32 m_bmc_idx = 0;
    alt_u32 m_pch_idx = 0;
    alt_u32 m_pcie_idx = 0;

    // Time
    alt_u32 time_unit = 0;
#endif
};

#endif /* SYSTEM_MAILBOX_MOCK_H_ */
