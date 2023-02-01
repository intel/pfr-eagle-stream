#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class PFRMctpFlowNegativeTest : public testing::Test
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

/**
 *
 * When host sends two first packet in a row (SOM high), FW
 * should be able to discard the first packet and continue to process
 * the second packet until completion.
 *
 */
TEST_F(PFRMctpFlowNegativeTest, test_bmc_send_two_first_packets_consequtively)
{
    // Obtain the replica of an mctp packet

    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    
    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    alt_u8 bmc_mctp_first_message[32];
    for (alt_u8 i = 0; i < 32; i++)
    {
        bmc_mctp_first_message[i] = 0xaa;
    }
    bmc_mctp_first_message[UT_MCTP_BYTE_COUNT_IDX] = 0x1f;
    bmc_mctp_first_message[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    bmc_mctp_first_message[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_PKT;

    alt_u8 bmc_mctp_second_message[32];
    for (alt_u8 i = 0; i < 32; i++)
    {
    	bmc_mctp_second_message[i] = 0xbb;
    }
    bmc_mctp_second_message[0] = 0x1f;
    bmc_mctp_second_message[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    bmc_mctp_second_message[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_PKT;

    alt_u8 bmc_mctp_third_message[32];
    for (alt_u8 i = 0; i < 32; i++)
    {
    	bmc_mctp_third_message[i] = 0xcc;
    }
    bmc_mctp_third_message[0] = 0x1f;
    bmc_mctp_third_message[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    bmc_mctp_third_message[UT_MCTP_SOM_EOM_BYTE_IDX] = LAST_PKT;

    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_second_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_third_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x07);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);
    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*) mctp_processed_pkt, &dest_addr), (alt_u32)50);

    // It is expected for fw to drop the first packet with SOM high when the next packet also has SOM high
    for (alt_u8 i = 0; i < 25; i++)
    {
        EXPECT_EQ(mctp_processed_pkt[i], bmc_mctp_second_message[7+i]);
    }

    for (alt_u8 i = 0; i < 25; i++)
    {
        EXPECT_EQ(mctp_processed_pkt[i + 25], bmc_mctp_third_message[7+i]);
    }
}

/**
 *
 * Host sends in imcomplete packet and then immediately sends in a complete packet.
 * Incomplete packet => First and middle packet
 * Complete packet => First and last packet
 *
 */
TEST_F(PFRMctpFlowNegativeTest, test_bmc_send_first_pkt_and_middle_pkt_and_then_first_pkt_again)
{
    // Obtain the replica of an mctp packet

    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));
    
    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    alt_u8 bmc_mctp_first_message[32];
    for (alt_u8 i = 0; i < 32; i++)
    {
        bmc_mctp_first_message[i] = 0xaa;
    }
    bmc_mctp_first_message[UT_MCTP_BYTE_COUNT_IDX] = 0x1f;
    bmc_mctp_first_message[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    bmc_mctp_first_message[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_PKT;

    alt_u8 bmc_mctp_second_message[32];
    for (alt_u8 i = 0; i < 32; i++)
    {
    	bmc_mctp_second_message[i] = 0xbb;
    }
    bmc_mctp_second_message[0] = 0x1f;
    bmc_mctp_second_message[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    bmc_mctp_second_message[UT_MCTP_SOM_EOM_BYTE_IDX] = INTERMEDIARY_PKT;

    alt_u8 bmc_mctp_third_message[32];
    for (alt_u8 i = 0; i < 32; i++)
    {
    	bmc_mctp_third_message[i] = 0xcc;
    }
    bmc_mctp_third_message[0] = 0x1f;
    bmc_mctp_third_message[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    bmc_mctp_third_message[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_PKT;

    alt_u8 bmc_mctp_fourth_message[32];
    for (alt_u8 i = 0; i < 32; i++)
    {
    	bmc_mctp_fourth_message[i] = 0xdd;
    }
    bmc_mctp_fourth_message[0] = 0x1f;
    bmc_mctp_fourth_message[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    bmc_mctp_fourth_message[UT_MCTP_SOM_EOM_BYTE_IDX] = LAST_PKT;

    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_second_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_third_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x07);

    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_fourth_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x0f);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);
    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*) mctp_processed_pkt, &dest_addr), (alt_u32)50);

    // Expected for Fw to have dropped the first two incomplete packet and immediately process the next incoming first packet.
    for (alt_u8 i = 0; i < 25; i++)
    {
        EXPECT_EQ(mctp_processed_pkt[i], bmc_mctp_third_message[7+i]);
    }

    for (alt_u8 i = 0; i < 25; i++)
    {
        EXPECT_EQ(mctp_processed_pkt[i + 25], bmc_mctp_fourth_message[7+i]);
    }
}

/**
 *
 * Host sends in middle packet without SOM being set first and then immediately sends in a complete packet
 *
 */
TEST_F(PFRMctpFlowNegativeTest, test_bmc_send_middle_packet_followed_by_first_packet)
{
    // Obtain the replica of an mctp packet

    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    
    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    alt_u8 bmc_mctp_first_message[32];
    for (alt_u8 i = 0; i < 32; i++)
    {
        bmc_mctp_first_message[i] = 0xaa;
    }
    bmc_mctp_first_message[UT_MCTP_BYTE_COUNT_IDX] = 0x1f;
    bmc_mctp_first_message[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    bmc_mctp_first_message[UT_MCTP_SOM_EOM_BYTE_IDX] = INTERMEDIARY_PKT;

    alt_u8 bmc_mctp_second_message[32];
    for (alt_u8 i = 0; i < 32; i++)
    {
    	bmc_mctp_second_message[i] = 0xbb;
    }
    bmc_mctp_second_message[0] = 0x1f;
    bmc_mctp_second_message[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    bmc_mctp_second_message[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_PKT;

    alt_u8 bmc_mctp_third_message[32];
    for (alt_u8 i = 0; i < 32; i++)
    {
    	bmc_mctp_third_message[i] = 0xcc;
    }
    bmc_mctp_third_message[0] = 0x1f;
    bmc_mctp_third_message[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    bmc_mctp_third_message[UT_MCTP_SOM_EOM_BYTE_IDX] = LAST_PKT;

    alt_u8 mctp_processed_pkt[1024] = {0};
    alt_u8 dest_addr = 0;

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_second_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_third_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x07);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);
    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*) mctp_processed_pkt, &dest_addr), (alt_u32)50);

    // Additional test
    // Assuming fw is free and come back to the function
    // Since fw has cleared the available bit, it is expected for the bit to be right-shifted.
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0);

    for (alt_u8 i = 0; i < 25; i++)
    {
        EXPECT_EQ(mctp_processed_pkt[i], bmc_mctp_second_message[7+i]);
    }

    for (alt_u8 i = 0; i < 25; i++)
    {
        EXPECT_EQ(mctp_processed_pkt[i + 25], bmc_mctp_third_message[7+i]);
    }
}

/**
 *
 * Host sends in last packet without SOM being set first and then immediately sends in a complete packet
 *
 */
TEST_F(PFRMctpFlowNegativeTest, test_bmc_send_last_packet_followed_by_first_packet)
{
    // Obtain the replica of an mctp packet

    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    
    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    alt_u8 bmc_mctp_first_message[32];
    for (alt_u8 i = 0; i < 32; i++)
    {
        bmc_mctp_first_message[i] = 0xaa;
    }
    bmc_mctp_first_message[UT_MCTP_BYTE_COUNT_IDX] = 0x1f;
    bmc_mctp_first_message[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    bmc_mctp_first_message[UT_MCTP_SOM_EOM_BYTE_IDX] = LAST_PKT;

    alt_u8 bmc_mctp_second_message[32];
    for (alt_u8 i = 0; i < 32; i++)
    {
    	bmc_mctp_second_message[i] = 0xbb;
    }
    bmc_mctp_second_message[0] = 0x1f;
    bmc_mctp_second_message[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    bmc_mctp_second_message[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_PKT;

    alt_u8 bmc_mctp_third_message[32];
    for (alt_u8 i = 0; i < 32; i++)
    {
    	bmc_mctp_third_message[i] = 0xcc;
    }
    bmc_mctp_third_message[0] = 0x1f;
    bmc_mctp_third_message[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    bmc_mctp_third_message[UT_MCTP_SOM_EOM_BYTE_IDX] = LAST_PKT;

    alt_u8 mctp_processed_pkt[1024] = {0};
    alt_u8 dest_addr = 0;

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_second_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_third_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x07);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);
    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*) mctp_processed_pkt, &dest_addr), (alt_u32)50);

    // Additional test
    // Assuming fw is free and come back to the function
    // Since fw has cleared the available bit, it is expected for the bit to be right-shifted.
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x00);

    for (alt_u8 i = 0; i < 25; i++)
    {
        EXPECT_EQ(mctp_processed_pkt[i], bmc_mctp_second_message[7+i]);
    }

    for (alt_u8 i = 0; i < 25; i++)
    {
        EXPECT_EQ(mctp_processed_pkt[i + 25], bmc_mctp_third_message[7+i]);
    }
}
