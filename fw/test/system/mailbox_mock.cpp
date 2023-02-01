// Test headers
#include "bsp_mock.h"
#include "mailbox_mock.h"

// Code headers
#include "mailbox_utils.h"

// Static data

// Type definitions

// Constructor/Destructor
MAILBOX_MOCK::MAILBOX_MOCK() {}
MAILBOX_MOCK::~MAILBOX_MOCK() {}

// Class methods

void MAILBOX_MOCK::reset()
{

    this->flush_fifo();
    m_mailbox_reg_file.reset();
#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)
    this->flush_mctp_write_fifo();
    this->flush_mctp_pch_fifo();
    this->flush_mctp_bmc_fifo();
    this->flush_mctp_pcie_fifo();

    // reset bmc related states
    for (alt_u32 i = 0; i < 50; i++)
    {
        bmc_avail_error_flag[i] = 0;
        pch_avail_error_flag[i] = 0;
        pcie_avail_error_flag[i] = 0;

        m_bmc_ls4b_avail[i] = 0;
        m_pch_ls4b_avail[i] = 0;
        m_pcie_ls4b_avail[i] = 0;
        m_bmc_ms4b_error[i] = 0;
        m_pch_ms4b_error[i] = 0;
        m_pcie_ms4b_error[i] = 0;
    }

    m_bmc_byte_count = 0;
    m_bmc_fifo_counter = 1;
    m_bmc_pkt_no = 0;
    m_bmc_error_track_bit = 0;
    m_bmc_finish_transaction = 0;
    m_bmc_pkt_error = 0;
    m_bmc_byte_cnt_tracker = 0;

    m_pch_byte_count = 0;
    m_pch_fifo_counter = 1;
    m_pch_pkt_no = 0;
    m_pch_error_track_bit = 0;
    m_pch_finish_transaction = 0;
    m_pch_pkt_error = 0;
    m_pch_byte_cnt_tracker = 0;

    m_pcie_byte_count = 0;
    m_pcie_fifo_counter = 1;
    m_pcie_pkt_no = 0;
    m_pcie_error_track_bit = 0;
    m_pcie_finish_transaction = 0;
    m_pcie_pkt_error = 0;
    m_pcie_byte_cnt_tracker = 0;

    m_bmc_idx = 0;
    m_pch_idx = 0;
    m_pcie_idx = 0;
    m_staged_idx = 0;
    time_unit = 0;
#endif
}

bool MAILBOX_MOCK::is_addr_in_range(void* addr)
{
    return m_mailbox_reg_file.is_addr_in_range(addr);
}

alt_u32 MAILBOX_MOCK::get_mem_word(void* addr)
{
    // Dispatch to the appropriate handler based on the address
    if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_UFM_WRITE_FIFO << 2)))
    {
        if (!m_fifo.empty())
            return m_fifo.back();
        return 0;
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_UFM_READ_FIFO << 2)))
    {
        if (!m_fifo.empty())
        {
            alt_u32 pop_val = m_fifo.front();
            m_fifo.pop();
            return pop_val;
        }
        return 0;
    }
#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_PCH_PACKET_AVAIL_AND_RX_PACKET_ERROR << 2)))
    {
        if (time_unit != 0)
        {
            time_unit--;
            return 0;
        }
        return pch_avail_error_flag[m_staged_idx];
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR << 2)))
    {
        if (time_unit != 0)
        {
            time_unit--;
            return 0;
        }
        return bmc_avail_error_flag[m_staged_idx];
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_PCIE_PACKET_AVAIL_AND_RX_PACKET_ERROR << 2)))
    {
        if (time_unit != 0)
        {
            time_unit--;
            return 0;
        }
        return pcie_avail_error_flag[m_staged_idx];
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_PACKET_WRITE_RXFIFO << 2)))
    {
        if (!m_mctp_write_fifo.empty())
            return m_mctp_write_fifo.back();
        return 0;
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_PCH_PACKET_READ_RXFIFO << 2)))
    {
        if (!m_mctp_pch_fifo.empty())
        {
            alt_u32 pop_val = m_mctp_pch_fifo.front();
            m_mctp_pch_fifo.pop();
            return pop_val;
        }
        return 0;
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_BMC_PACKET_READ_RXFIFO << 2)))
    {
        if (!m_mctp_bmc_fifo.empty())
        {
            alt_u32 pop_val = m_mctp_bmc_fifo.front();
            m_mctp_bmc_fifo.pop();
            return pop_val;
        }
        return 0;
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_PCIE_PACKET_READ_RXFIFO << 2)))
    {
        if (!m_mctp_pcie_fifo.empty())
        {
            alt_u32 pop_val = m_mctp_pcie_fifo.front();
            m_mctp_pcie_fifo.pop();
            return pop_val;
        }
        return 0;
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_PCH_BYTE_COUNT << 2)))
    {
        alt_u32 pop_val = m_pch_byte_count_fifo.front();
        m_pch_byte_count_fifo.pop();
        return pop_val;
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_BMC_BYTE_COUNT << 2)))
    {
        alt_u32 pop_val = m_bmc_byte_count_fifo.front();
        m_bmc_byte_count_fifo.pop();
        return pop_val;
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_PCIE_BYTE_COUNT << 2)))
    {
        alt_u32 pop_val = m_pcie_byte_count_fifo.front();
        m_pcie_byte_count_fifo.pop();
        return pop_val;
    }
#endif
    else
    {
        return m_mailbox_reg_file.get_mem_word(addr);
    }
    return 0;
}

void MAILBOX_MOCK::set_mem_word(void* addr, alt_u32 data)
{
    if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_UFM_WRITE_FIFO << 2)))
    {
        // By design, the fifo has significantly more space than is needed and will never be full
        m_fifo.push((alt_u8) data);
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_UFM_READ_FIFO << 2)))
    {
        // Throw away writes to the read fifo address
        // Do nothing
    }
#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_PACKET_WRITE_RXFIFO << 2)))
    {
        m_bmc_finish_transaction = m_pch_finish_transaction = m_pcie_finish_transaction = data;

        // Handles the left-shifting of available and error bit
        // The function will remember the states of the bits until cleared by firmware
        // Able to handle the mctp packets for the 3 hosts in a mutual exclusive manner
        if (m_bmc_finish_transaction == m_bmc)
        {
            m_bmc_pkt_no = 0;
            m_bmc_ls4b_avail[m_bmc_idx] = (m_bmc_ls4b_avail[m_bmc_idx] << 1) | 1;
            if (m_bmc_fifo_counter)
            {
                m_bmc_byte_count -= m_bmc_fifo_counter;
                m_bmc_byte_count_fifo.push((alt_u8)m_bmc_byte_count);
                m_bmc_byte_count_fifo.pop();
                m_bmc_ms4b_error[m_bmc_idx] |= (1 << m_bmc_error_track_bit);
            }
            else if (m_bmc_pkt_error)
            {
                m_bmc_ms4b_error[m_bmc_idx] |= (1 << m_bmc_error_track_bit);
            }
            m_bmc_error_track_bit++;
            bmc_avail_error_flag[m_bmc_idx] = (m_bmc_ms4b_error[m_bmc_idx] << 4) | m_bmc_ls4b_avail[m_bmc_idx];
            m_bmc_fifo_counter = 1;
            m_bmc_byte_cnt_tracker = 0;
        }

        if (m_pch_finish_transaction == m_pch)
        {
            m_pch_pkt_no = 0;
            m_pch_ls4b_avail[m_pch_idx] = (m_pch_ls4b_avail[m_pch_idx] << 1) | 1;
            if (m_pch_fifo_counter)
            {
                m_pch_byte_count -= m_pch_fifo_counter;
                m_pch_byte_count_fifo.push((alt_u8)m_pch_byte_count);
                m_pch_byte_count_fifo.pop();
                m_pch_ms4b_error[m_pch_idx] |= (1 << m_pch_error_track_bit);
            }
            else if (m_pch_pkt_error)
            {
            	m_pch_ms4b_error[m_pch_idx] |= (1 << m_pch_error_track_bit);
            }
            m_pch_error_track_bit++;
            pch_avail_error_flag[m_pch_idx] = (m_pch_ms4b_error[m_pch_idx] << 4) | m_pch_ls4b_avail[m_pch_idx];
            m_pch_fifo_counter = 1;
            m_pch_byte_cnt_tracker = 0;
        }

        if (m_pcie_finish_transaction == m_pcie)
        {
            m_pcie_pkt_no = 0;
            m_pcie_ls4b_avail[m_pcie_idx] = (m_pcie_ls4b_avail[m_pcie_idx] << 1) | 1;
            if (m_pcie_fifo_counter)
            {
                m_pcie_byte_count -= m_pcie_fifo_counter;
                m_pcie_byte_count_fifo.push((alt_u8)m_pcie_byte_count);
                m_pcie_byte_count_fifo.pop();
                m_pcie_ms4b_error[m_pcie_idx] |= (1 << m_pcie_error_track_bit);
            }
            else if (m_pcie_pkt_error)
            {
            	m_pcie_ms4b_error[m_pcie_idx] |= (1 << m_pcie_error_track_bit);
            }
            m_pcie_error_track_bit++;
            pcie_avail_error_flag[m_pcie_idx] = (m_pcie_ms4b_error[m_pcie_idx] << 4) | m_pcie_ls4b_avail[m_pcie_idx];
            m_pcie_fifo_counter = 1;
            m_pcie_byte_cnt_tracker = 0;
        }

        if (m_bmc_finish_transaction == 201)
        {
            m_bmc_error_track_bit = 0;
            m_bmc_idx++;
        }
        if (m_pch_finish_transaction == 202)
        {
            m_pch_error_track_bit = 0;
            m_pch_idx++;
        }
        if (m_pcie_finish_transaction == 203)
        {
            m_pcie_error_track_bit = 0;
            m_pcie_idx++;
        }
        if (data == 220)
        {
            m_staged_idx++;
        }
        if (data <= 200)
        {
            // It is important to note that this figure is just a projection and
            // does not have to be accurate. As long as user set a large enough time unit in unit test
            // to time out the spdm flow, this will be sufficient to test timeout error handling of the code
            // As unit test mock system is ran in a sequential manner, the mock will have to do loop cycle to
            // simulate timeout.
            time_unit = 1000*data;
        }

    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR << 2)))
    {
        // When firmware writes to this register, the avail/error byte will be right-shifted
        m_bmc_ms4b_error[m_staged_idx] >>= 1;
        m_bmc_ls4b_avail[m_staged_idx] >>= 1;
        bmc_avail_error_flag[m_staged_idx] = (m_bmc_ms4b_error[m_staged_idx] << 4) | m_bmc_ls4b_avail[m_staged_idx];
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_PCH_PACKET_AVAIL_AND_RX_PACKET_ERROR << 2)))
    {
        // When firmware writes to this register, the avail/error byte will be right-shifted
        m_pch_ms4b_error[m_staged_idx] >>= 1;
        m_pch_ls4b_avail[m_staged_idx] >>= 1;
        pch_avail_error_flag[m_staged_idx] = (m_pch_ms4b_error[m_staged_idx] << 4) | m_pch_ls4b_avail[m_staged_idx];
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_PCIE_PACKET_AVAIL_AND_RX_PACKET_ERROR << 2)))
    {
        // When firmware writes to this register, the avail/error byte will be right-shifted
        m_pcie_ms4b_error[m_staged_idx] >>= 1;
        m_pcie_ls4b_avail[m_staged_idx] >>= 1;
        pcie_avail_error_flag[m_staged_idx] = (m_pcie_ms4b_error[m_staged_idx] << 4) | m_pcie_ls4b_avail[m_staged_idx];
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_PCH_PACKET_READ_RXFIFO << 2)))
    {
        // Handles incoming mctp packets.
        // Error conditions are: packet size < byte count && packet size > byte count

        if (m_pch_fifo_counter && m_pch_byte_cnt_tracker)
        {
            m_mctp_pch_fifo.push((alt_u8) data);
        }

        if (!m_pch_pkt_no)
        {
            m_pch_byte_count = data;
            m_pch_byte_count_fifo.push((alt_u8) data);
            m_pch_fifo_counter = m_pch_byte_count + 1;
            m_pch_pkt_no = 1;
            m_pch_pkt_error = 0;
            // As the RTL only relay the information of byte count directly to byte count register and not on the read RXFIFO
            m_pch_byte_cnt_tracker = 1;
        }

        if (!m_pch_fifo_counter)
        {
            m_pch_pkt_error = 1;
        }
        else
        {
        	m_pch_pkt_error = 0;
            m_pch_fifo_counter--;
        }
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_BMC_PACKET_READ_RXFIFO << 2)))
    {
        // Handles incoming mctp packets.
        // Error conditions are: packet size < byte count && packet size > byte count
        if (m_bmc_fifo_counter && m_bmc_byte_cnt_tracker)
        {
            m_mctp_bmc_fifo.push((alt_u8) data);
        }

        if (!m_bmc_pkt_no)
        {
            m_bmc_byte_count = data;
            m_bmc_byte_count_fifo.push((alt_u8) data);
            m_bmc_fifo_counter = m_bmc_byte_count + 1;
            m_bmc_pkt_no = 1;
            m_bmc_pkt_error = 0;
            // As the RTL only relay the information of byte count directly to byte count register and not on the read RXFIFO
            m_bmc_byte_cnt_tracker = 1;
        }

        if (!m_bmc_fifo_counter)
        {
            m_bmc_pkt_error = 1;
        }
        else
        {
        	m_bmc_pkt_error = 0;
            m_bmc_fifo_counter--;
        }
    }
    else if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_MCTP_PCIE_PACKET_READ_RXFIFO << 2)))
    {
        // Handles incoming mctp packets.
        // Error conditions are: packet size < byte count && packet size > byte count
        if (m_pcie_fifo_counter && m_pcie_byte_cnt_tracker)
        {
            m_mctp_pcie_fifo.push((alt_u8) data);
        }

        if (!m_pcie_pkt_no)
        {
            m_pcie_byte_count = data;
            m_pcie_byte_count_fifo.push((alt_u8) data);
            m_pcie_fifo_counter = m_pcie_byte_count + 1;
            m_pcie_pkt_no = 1;
            m_pcie_pkt_error = 0;
            // As the RTL only relay the information of byte count directly to byte count register and not on the read RXFIFO
            m_pcie_byte_cnt_tracker = 1;
        }

        if (!m_pcie_fifo_counter)
        {
            m_pcie_pkt_error = 1;
        }
        else
        {
        	m_pcie_pkt_error = 0;
            m_pcie_fifo_counter--;
        }
    }
#endif
    else
    {
        // If bits[2:1] of the command trigger are set, flush the fifo
        if ((std::uintptr_t) addr == (U_MAILBOX_AVMM_BRIDGE_BASE + (MB_UFM_CMD_TRIGGER << 2)) &&
            (data & 0x6))
        {
            this->flush_fifo();
        }
        m_mailbox_reg_file.set_mem_word(addr, data);
    }
}

void MAILBOX_MOCK::flush_fifo()
{
    while (!m_fifo.empty())
        m_fifo.pop();
}

#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)
void MAILBOX_MOCK::flush_mctp_pch_fifo()
{
    while (!m_mctp_pch_fifo.empty())
        m_mctp_pch_fifo.pop();
}

void MAILBOX_MOCK::flush_mctp_bmc_fifo()
{
    while (!m_mctp_bmc_fifo.empty())
    {
        m_mctp_bmc_fifo.pop();
    }
    while (!m_bmc_byte_count_fifo.empty())
    {
        m_bmc_byte_count_fifo.pop();
    }
}

void MAILBOX_MOCK::flush_mctp_pcie_fifo()
{
    while (!m_mctp_pcie_fifo.empty())
    {
        m_mctp_pcie_fifo.pop();
    }
    while (!m_pcie_byte_count_fifo.empty())
    {
        m_pcie_byte_count_fifo.pop();
    }
}

void MAILBOX_MOCK::flush_mctp_write_fifo()
{
    while (!m_mctp_write_fifo.empty())
    {
        m_mctp_write_fifo.pop();
    }
    while (!m_pch_byte_count_fifo.empty())
    {
        m_pch_byte_count_fifo.pop();
    }
}
#endif
