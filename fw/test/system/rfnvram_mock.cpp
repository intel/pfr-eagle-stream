// Test headers
#include "bsp_mock.h"
#include "rfnvram_mock.h"

// Code headers

// Static data

// Type definitions

// Constructor/Destructor
RFNVRAM_MOCK::RFNVRAM_MOCK() {}
RFNVRAM_MOCK::~RFNVRAM_MOCK() {}

// Class methods

void RFNVRAM_MOCK::reset()
{
    m_ram.reset();
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    //bmc_internal_addr = 0;
    //pch_internal_addr = 0;
    //pcie_internal_addr = 0;

    while (!m_read_fifo.empty())
    {
        //m_read_fifo.front();
        m_read_fifo.pop();
    }
#endif
}

bool RFNVRAM_MOCK::is_addr_in_range(void* addr)
{
    return m_ram.is_addr_in_range(addr);
}

alt_u32 RFNVRAM_MOCK::get_mem_word(void* addr)
{
    if (addr == RFNVRAM_RX_FIFO)
    {
        alt_u32 ret_data = (alt_u32) m_read_fifo.front();
        m_read_fifo.pop();
        return ret_data;
    }
    else if (addr == RFNVRAM_TX_FIFO_BYTES_LEFT)
    {
        return m_cmd_fifo.size();
    }
    else if (addr == RFNVRAM_RX_FIFO_BYTES_LEFT)
    {
        return m_read_fifo.size();
    }
    else if (addr == RFNVRAM_MCTP_CONTROL)
    {
        return (smbus_speed | pec_enabled | smbus_soft_reset);
    }
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    else if (addr == RFNVRAM_MCTP_STATUS)
    {
        return (fsm_status | pec_byte_nack | data_byte_nack | address_byte_nack);
    }
    else if (addr == RFNVRAM_MCTP_SMBUS_SLAVE_PORT_ID)
    {
        return smbus_port;
    }
    else if (addr == U_RFNVRAM_SMBUS_MASTER_ADDR)
    {
        if (!m_read_fifo.empty())
        {
            alt_u32 ret_data = (alt_u32) m_read_fifo.front();
            m_read_fifo.pop();
            return ret_data;
        }
        return 0;
    }
    else
    {
        PFR_INTERNAL_ERROR("Undefined handler for address");
    }
#endif
    return 0;
}

void RFNVRAM_MOCK::set_mem_word(void* addr, alt_u32 data)
{
    if (addr == U_RFNVRAM_SMBUS_MASTER_ADDR)
    {
        m_cmd_fifo.push(data);

        // simulate all the i2c commands sent to the IP
        // TODO move this function to a separate thread
        if (!smbus_port)
        {
            simulate_i2c();
        }
#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)
        else
        {
            simulate_mctp_i2c();
        }
#endif
    }
    else if (addr == RFNVRAM_MCTP_SMBUS_SLAVE_PORT_ID)
    {
        // Latch the port ID
        // Might want to upgrade to catch if fw write to wrong port ID
        smbus_port = data;
    }
    else if (addr == RFNVRAM_MCTP_CONTROL)
    {
    	smbus_speed = (data & 0b1);
        pec_enabled = (data & 0b10);
        smbus_soft_reset = (data & 0b10000);
        if (smbus_soft_reset)
        {
            // Do a soft reset to configuration data
            smbus_speed = 0;
            pec_enabled = 0;
            smbus_soft_reset = 0;

            fsm_status = 0;
            pec_byte_nack = 0;
            data_byte_nack = 0;
            address_byte_nack = 0;
        }
    }
#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)
    else if (addr == RFNVRAM_MCTP_STATUS)
    {
        fsm_status = (data & 0b1);
        pec_byte_nack = (data & 0b10);
        data_byte_nack = (data & 0b100);
        address_byte_nack = (data & 0b1000);
    }
#endif
    else
    {
        PFR_INTERNAL_ERROR("Undefined handler for address");
    }
    // else do nothing
}
#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)
void RFNVRAM_MOCK::simulate_mctp_i2c()
{
    int cmd;

    while (!m_cmd_fifo.empty())
    {
        cmd = m_cmd_fifo.front();
        m_cmd_fifo.pop();
        switch (state_2)
        {
        case IDLE_M:
            if ((cmd & (1 << 9)) && (cmd & 0xFF) == BMC_SMBUS_PORT_ADDR)//TODO:update with correct destination address
            {
                bmc_internal_addr = 0;
                state_2 = WRITE_DATA_M;
                m_read_fifo.push(cmd & 0xFF);
                bmc_mem[bmc_internal_addr++] = cmd & 0xFF;
            }
            else if ((cmd & (1 << 9)) && (cmd & 0xFF) == PCH_SMBUS_PORT_ADDR)
            {
                pch_internal_addr = 0;
                state_2 = WRITE_DATA_M;
        	    m_read_fifo.push(cmd & 0xFF);
                pch_mem[pch_internal_addr++] = cmd & 0xFF;
            }
            else if ((cmd & (1 << 9)) && (cmd & 0xFF) == PCIE_SMBUS_PORT_ADDR)
            {
                pcie_internal_addr = 0;
                state_2 = WRITE_DATA_M;
        	    m_read_fifo.push(cmd & 0xFF);
                pcie_mem[pcie_internal_addr++] = cmd & 0xFF;
            }
            break;
        case WRITE_DATA_M:
            if (cmd & (1 << 8))
            {
                state_2 = IDLE_M;
                if (smbus_port == BMC_SMBUS_PORT)
                {
                    m_read_fifo.push(cmd & 0xFF);
                    bmc_mem[bmc_internal_addr++] = cmd & 0xFF;
                }
                else if (smbus_port == PCH_SMBUS_PORT)
                {
            	    m_read_fifo.push(cmd & 0xFF);
                    pch_mem[pch_internal_addr++] = cmd & 0xFF;
                }
                else if (smbus_port == PCIE_SMBUS_PORT)
                {
            	    m_read_fifo.push(cmd & 0xFF);
                    pcie_mem[pcie_internal_addr++] = cmd & 0xFF;
                }
            }
            else
            {
                state_2 = WRITE_DATA_M;
                if (smbus_port == BMC_SMBUS_PORT)
                {
                    m_read_fifo.push(cmd & 0xFF);
                    bmc_mem[bmc_internal_addr++] = cmd & 0xFF;
                }
                else if (smbus_port == PCH_SMBUS_PORT)
                {
            	    m_read_fifo.push(cmd & 0xFF);
                    pch_mem[pch_internal_addr++] = cmd & 0xFF;
                }
                else if (smbus_port == PCIE_SMBUS_PORT)
                {
            	    m_read_fifo.push(cmd & 0xFF);
                    pcie_mem[pcie_internal_addr++] = cmd & 0xFF;
                }
            }
            break;
        default:
            state_2 = IDLE_M;
            break;
        }

    }
}
#endif

void RFNVRAM_MOCK::simulate_i2c()
{
    int cmd;

    while (!m_cmd_fifo.empty())
    {
        cmd = m_cmd_fifo.front();
        m_cmd_fifo.pop();
        switch (state)
        {
            case IDLE:
                // if it is start + write go into write internal addr state
                if ((cmd & (1 << 9)) && (cmd & 0xFF) == RFNVRAM_SMBUS_ADDR)
                {

                    state = WRITE_ADDR_UPPER;
                }
                else if ((cmd & (1 << 9)) && (cmd & 0xFF) == (RFNVRAM_SMBUS_ADDR | 0x1))
                {
          ;
                    state = READ_DATA;
                }
                break;
            case WRITE_ADDR_UPPER:
                internal_addr = (cmd & 0x3) << 8;
                // return to idle if stop is detected
                if (cmd & (1 << 8))
                    state = IDLE;
                else
                    state = WRTIE_ADDR_LOWER;
                break;
            case WRTIE_ADDR_LOWER:
                internal_addr = internal_addr | (cmd & 0xFF);
                if (cmd & (1 << 8))
                    state = IDLE;
                else if ((cmd & (1 << 9)) && (cmd & 0xFF) == (RFNVRAM_SMBUS_ADDR | 0x1))
                    state = READ_DATA;
                else
                    state = WRITE_DATA;
                break;
            case WRITE_DATA:
                if (cmd & (1 << 9))
                {
                    if ((cmd & 0xFF) == (RFNVRAM_SMBUS_ADDR | 0x1))
                    {
                        state = READ_DATA;
                    }
                    else
                    {
                        // start condition but no addr match
                        state = IDLE;
                    }
                }
                else if (cmd & (1 << 8))
                {

                    if (smbus_port == 0)
                    {

                        rfnvram_mem[internal_addr++] = cmd & 0xFF;
                    }
                    //rfnvram_mem[internal_addr++] = cmd & 0xFF;
                    state = IDLE;
                }
                else
                {
                    if (smbus_port == 0)
                    {
                        rfnvram_mem[internal_addr++] = cmd & 0xFF;
                    }
                    state = WRITE_DATA;
                }
                break;

            case READ_DATA:
                if (smbus_port == 0)
                {
                    m_read_fifo.push(rfnvram_mem[internal_addr++]);
                }
                if (cmd & (1 << 8))
                    state = IDLE;
                else if ((cmd & (1 << 9)) && (cmd & 0xFF) == (RFNVRAM_SMBUS_ADDR))
                    state = WRITE_DATA;
                else
                    state = READ_DATA;
                break;

            default:
                state = IDLE;
                break;
        }
    }
}
