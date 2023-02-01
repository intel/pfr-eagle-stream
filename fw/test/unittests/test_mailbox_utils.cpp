#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class PFRMailboxTest : public testing::Test
{
public:
    alt_u32* m_memory = nullptr;

    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
        m_memory = U_MAILBOX_AVMM_BRIDGE_ADDR;
    }

    virtual void TearDown() {}
};

TEST_F(PFRMailboxTest, test_basic_rw)
{
    initialize_mailbox();
    EXPECT_EQ(IORD(m_memory, MB_CPLD_STATIC_ID), (alt_u32) 0xDE);
    IOWR(m_memory, MB_RECOVERY_COUNT, 5);
    EXPECT_EQ(IORD(m_memory, MB_RECOVERY_COUNT), (alt_u32) 5);
    EXPECT_EQ(IORD(m_memory, MB_PANIC_EVENT_COUNT), (alt_u32) 0);
}

TEST_F(PFRMailboxTest, basic_fifo_test)
{
    IOWR(m_memory, MB_UFM_WRITE_FIFO, 1);
    EXPECT_EQ(IORD(m_memory, MB_UFM_WRITE_FIFO), (alt_u32) 1);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 1);
    EXPECT_EQ(IORD(m_memory, MB_UFM_WRITE_FIFO), (alt_u32) 0);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 0);
}

TEST_F(PFRMailboxTest, fifo_test_width)
{
    IOWR(m_memory, MB_UFM_WRITE_FIFO, 0xDEADBEEF);
    EXPECT_EQ(IORD(m_memory, MB_UFM_WRITE_FIFO), (alt_u32) 0xEF);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 0xEF);
}

TEST_F(PFRMailboxTest, fifo_test)
{
    for (int i = 1; i < 6; i++)
    {
        IOWR(m_memory, MB_UFM_WRITE_FIFO, i);
        EXPECT_EQ(IORD(m_memory, MB_UFM_WRITE_FIFO), (alt_u32) i);
    }
    IOWR(m_memory, MB_UFM_CMD_TRIGGER, 1 << 3);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 1);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 2);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 3);
    IOWR(m_memory, MB_UFM_CMD_TRIGGER, 1 << 1);
    EXPECT_EQ(IORD(m_memory, MB_UFM_WRITE_FIFO), (alt_u32) 0);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 0);
}

TEST_F(PFRMailboxTest, test_flush_mailbox_fifo)
{
    for (int i = 1; i < 6; i++)
    {
        IOWR(m_memory, MB_UFM_WRITE_FIFO, i);
        EXPECT_EQ(IORD(m_memory, MB_UFM_WRITE_FIFO), (alt_u32) i);
    }
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 1);
    flush_mailbox_fifo();
    EXPECT_EQ(IORD(m_memory, MB_UFM_WRITE_FIFO), (alt_u32) 0);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 0);
}

TEST_F(PFRMailboxTest, test_read_from_mailbox_fifo)
{
    IOWR(m_memory, MB_UFM_WRITE_FIFO, 0xDEADBEEF);
    IOWR(m_memory, MB_UFM_WRITE_FIFO, 0xABCDEFFF);
    IOWR(m_memory, MB_UFM_WRITE_FIFO, 0x16DC8B90);

    EXPECT_EQ(read_from_mailbox_fifo(), (alt_u32) 0xEF);
    EXPECT_EQ(read_from_mailbox_fifo(), (alt_u32) 0xFF);
    EXPECT_EQ(read_from_mailbox_fifo(), (alt_u32) 0x90);
}

TEST_F(PFRMailboxTest, test_write_to_mailbox_fifo)
{
    write_to_mailbox_fifo(0xDEADBEEF);
    write_to_mailbox_fifo(0xABCDEFFF);
    write_to_mailbox_fifo(0x16DC8B90);

    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 0xEF);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 0xFF);
    EXPECT_EQ(IORD(m_memory, MB_UFM_READ_FIFO), (alt_u32) 0x90);
}

#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)

TEST_F(PFRMailboxTest, test_read_from_mailbox_mctp_fifo_bmc)
{
    // Starts transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 0);
    // This is byte count
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x03);
    // Source slave adress
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x01);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // End the this transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 255);

    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0x01);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_BYTE_COUNT), (alt_u32)0x03);
}

TEST_F(PFRMailboxTest, test_read_from_mailbox_mctp_fifo_bmc_packet_size_smaller_than_byte_count)
{
    // Test scenario where the packet size is less than the provided byte count
    // Starts transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 0);
    // This is byte count
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x03);
    // Source slave adress
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x01);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // End the this transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 255);

    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0x01);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x11);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_BYTE_COUNT), (alt_u32)0x02);
}

TEST_F(PFRMailboxTest, test_read_from_mailbox_mctp_fifo_bmc_packet_size_bigger_than_byte_count)
{
    // Test scenario where the packet size is less than the provided byte count
    // Starts transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 0);
    // This is byte count
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x03);
    // Source slave adress
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x01);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // End the this transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 255);

    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0x01);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x11);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_BYTE_COUNT), (alt_u32)0x03);
}

TEST_F(PFRMailboxTest, test_read_from_mailbox_mctp_fifo_bmc_send_repeated_packet)
{
    // Test scenario where the packet size is less than the provided byte count
    // Starts first transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 0);
    // This is byte count
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x03);
    // Source slave adress
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x01);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // End the this transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 255);

    // Starts second transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 0);
    // This is byte count
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x03);
    // Source slave adress
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x01);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // End the this transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 255);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x3);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_BYTE_COUNT), (alt_u32)0x03);

    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0x01);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);

    // Write 1 to clear and shift the available and error bit to the right
    IOWR(m_memory, MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR, 1);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x1);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_BYTE_COUNT), (alt_u32)0x03);

    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0x01);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
}

TEST_F(PFRMailboxTest, test_read_from_mailbox_mctp_fifo_bmc_send_one_wrong_and_one_right_packet_1)
{
    // Test scenario where the packet size is less than the provided byte count
    // Starts first transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 0);
    // This is byte count
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x03);
    // Source slave adress
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x01);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // End the this transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 255);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x11);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_BYTE_COUNT), (alt_u32)0x02);

    // Starts second transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 0);
    // This is byte count
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x03);
    // Source slave adress
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x01);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // End the this transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 255);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x13);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_BYTE_COUNT), (alt_u32)0x03);

    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0x01);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);

    // Write 1 to clear and shift the available and error bit to the right
    IOWR(m_memory, MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR, 1);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x1);

    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0x01);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
}

TEST_F(PFRMailboxTest, test_read_from_mailbox_mctp_fifo_bmc_send_one_wrong_and_one_right_packet_2)
{
    // Test scenario where the packet size is more than the provided byte count
    // Starts first transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 0);
    // This is byte count
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x03);
    // Source slave adress
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x01);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // End the this transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 255);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x11);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_BYTE_COUNT), (alt_u32)0x03);

    // Starts second transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 0);
    // This is byte count
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x03);
    // Source slave adress
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x01);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // End the this transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 255);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x13);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_BYTE_COUNT), (alt_u32)0x03);

    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0x01);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);

    // Write 1 to clear and shift the available and error bit to the right
    IOWR(m_memory, MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR, 1);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x1);

    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0x01);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
}

TEST_F(PFRMailboxTest, test_read_from_mailbox_mctp_fifo_bmc_send_one_right_and_one_wrong_packet_1)
{
    // Test scenario where the packet size is less than the provided byte count
    // Starts first transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 0);
    // This is byte count
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x03);
    // Source slave adress
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x01);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // End the this transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 255);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_BYTE_COUNT), (alt_u32)0x03);

    // Starts second transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 0);
    // This is byte count
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x03);
    // Source slave adress
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x01);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // End the this transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 255);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x23);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_BYTE_COUNT), (alt_u32)0x02);

    //EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0x03);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0x01);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);

    // Write 1 to clear and shift the available and error bit to the right
    IOWR(m_memory, MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR, 1);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x11);

    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0x01);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0);
}


TEST_F(PFRMailboxTest, test_read_from_mailbox_mctp_fifo_bmc_send_one_right_and_one_wrong_packet_2)
{
    // Test scenario where the packet size is more than the provided byte count
    // Starts first transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 0);
    // This is byte count
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x03);
    // Source slave adress
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x01);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // End the this transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 255);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_BYTE_COUNT), (alt_u32)0x03);

    // Starts second transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 0);
    // This is byte count
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x03);
    // Source slave adress
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0x01);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO, 0xff);
    // End the this transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 255);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x23);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_BYTE_COUNT), (alt_u32)0x03);

    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0x01);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);

    // Write 1 to clear and shift the available and error bit to the right
    IOWR(m_memory, MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR, 1);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x11);

    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0x01);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_BMC_PACKET_READ_RXFIFO), (alt_u32) 0);
}

TEST_F(PFRMailboxTest, test_read_from_mailbox_mctp_fifo_pch)
{
    // Starts first transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 0);

    IOWR(m_memory, MB_MCTP_PCH_PACKET_READ_RXFIFO, 0x03);
    // This is the mctp command code
    IOWR(m_memory, MB_MCTP_PCH_PACKET_READ_RXFIFO, 0x0F);
    // This is byte count
    IOWR(m_memory, MB_MCTP_PCH_PACKET_READ_RXFIFO, 0xFF);
    // Source slave adress
    IOWR(m_memory, MB_MCTP_PCH_PACKET_READ_RXFIFO, 0x02);

    // End the this transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 254);

    EXPECT_EQ(IORD(m_memory, MB_MCTP_PCH_PACKET_READ_RXFIFO), (alt_u32) 0x0f);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_PCH_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_PCH_PACKET_READ_RXFIFO), (alt_u32) 0x02);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_PCH_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_PCH_BYTE_COUNT), (alt_u32)0x03);
}

TEST_F(PFRMailboxTest, test_read_from_mailbox_mctp_fifo_pcie)
{
    // Starts first transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 0);
    // This is byte count
    IOWR(m_memory, MB_MCTP_PCIE_PACKET_READ_RXFIFO, 0x03);
    // Source slave adress
    IOWR(m_memory, MB_MCTP_PCIE_PACKET_READ_RXFIFO, 0x01);
    // Write random data
    IOWR(m_memory, MB_MCTP_PCIE_PACKET_READ_RXFIFO, 0xff);
    // Write random data
    IOWR(m_memory, MB_MCTP_PCIE_PACKET_READ_RXFIFO, 0xff);
    // End the this transaction
    IOWR(m_memory, MB_MCTP_PACKET_WRITE_RXFIFO, 253);

    EXPECT_EQ(IORD(m_memory, MB_MCTP_PCIE_PACKET_READ_RXFIFO), (alt_u32) 0x01);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_PCIE_PACKET_READ_RXFIFO), (alt_u32) 0xff);
    EXPECT_EQ(IORD(m_memory, MB_MCTP_PCIE_PACKET_READ_RXFIFO), (alt_u32) 0xff);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_PCIE_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_PCIE_BYTE_COUNT), (alt_u32)0x03);
}

#endif
