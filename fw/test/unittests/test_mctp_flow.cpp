#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class PFRMctpFlowTest : public testing::Test
{
public:
    alt_u32* m_memory = nullptr;
    alt_u32* m_mctp_memory = nullptr;

    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
        m_memory = U_MAILBOX_AVMM_BRIDGE_ADDR;
        m_mctp_memory = U_RFNVRAM_SMBUS_MASTER_ADDR;
    }

    virtual void TearDown() {}
};

/**
 * Different corner cases for message integrity checking:
 *
 * In sequence >
 * 1) SOM > SOM & EOM
 * 2) EOM
 * 3) SOM & EOM
 * 4) SOM > Middle > SOM & EOM
 * 5) Middle > EOM
 * 6) SOM > SOM > EOM
 *
 */
TEST_F(PFRMctpFlowTest, test_bmc_send_som_and_som_and_eom)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_SOM_HIGH_REPLICA;
    alt_u8 bmc_mctp_second_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_SOM_AND_EOM_HIGH_REPLICA;
    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;

    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));
    
    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_second_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);
    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*) mctp_processed_pkt, &dest_addr), (alt_u32)25);

}

TEST_F(PFRMctpFlowTest, test_bmc_send_eom)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_EOM_HIGH_REPLICA;
    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;

    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));
    
    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;
    
    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);
    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*) mctp_processed_pkt, &dest_addr), (alt_u32)0);

}

TEST_F(PFRMctpFlowTest, test_bmc_send_som_and_eom)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_SOM_AND_EOM_HIGH_REPLICA;
    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;

    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));
    
    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;
    
    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);
    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*) mctp_processed_pkt, &dest_addr), (alt_u32)25);

}

TEST_F(PFRMctpFlowTest, test_bmc_send_som_followed_by_middle_and_som_and_eom)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_SOM_HIGH_REPLICA;
    alt_u8 bmc_mctp_second_message[32] = BMC_MCTP_CHALLENGE_MIDDLE_PACKET_REPLICA;
    alt_u8 bmc_mctp_third_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_SOM_AND_EOM_HIGH_REPLICA;
    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;

    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));
    
    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;
    
    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_second_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_third_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x07);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);
    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*) mctp_processed_pkt, &dest_addr), (alt_u32)25);

}

TEST_F(PFRMctpFlowTest, test_bmc_send_middle_and_eom)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[32] = BMC_MCTP_CHALLENGE_MIDDLE_PACKET_REPLICA;
    alt_u8 bmc_mctp_second_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_EOM_HIGH_REPLICA;
    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;

    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));
    
    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;
    
    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_second_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);
    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*) mctp_processed_pkt, &dest_addr), (alt_u32)0);

}

TEST_F(PFRMctpFlowTest, test_bmc_send_som_followed_by_som_and_eom)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_SOM_HIGH_REPLICA;
    alt_u8 bmc_mctp_second_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_SOM_HIGH_REPLICA;
    alt_u8 bmc_mctp_third_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_EOM_HIGH_REPLICA;
    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;

    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));
    
    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;
    
    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_second_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_third_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x07);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);
    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*) mctp_processed_pkt, &dest_addr), (alt_u32)50);

}

TEST_F(PFRMctpFlowTest, test_write_two_mctp_pkt)
{
    alt_u8 test_data[400];
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));

    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    for (alt_u8 i = 0; i < 200; i++)
    {
        test_data[i] = 1;
    }
    for (alt_u8 i = 0; i < 200; i++)
    {
        test_data[i + 200] = 2;
    }

    alt_u8 dest_addr = BMC_SLAVE_ADDRESS;
    mctp_ctx->cached_addr = dest_addr;
    mctp_ctx->msg_type = MCTP_MESSAGE_TYPE;

    alt_u8 mctp_header_som_set[9] = {0};
    alt_u8 mctp_header_eom_set[9] = {0};

    mctp_header_som_set[0] = dest_addr << 1;
    mctp_header_som_set[1] = MCTP_COMMAND_CODE;
    mctp_header_som_set[2] = 255;
    mctp_header_som_set[3] = 0x71;
    mctp_header_som_set[4] = MCTP_HDR_VER;
    mctp_header_som_set[7] = FIRST_PKT;
    mctp_header_som_set[8] = MCTP_MESSAGE_TYPE;

    mctp_header_eom_set[0] = dest_addr << 1;
    mctp_header_eom_set[1] = MCTP_COMMAND_CODE;
    mctp_header_eom_set[2] = 157;
    mctp_header_eom_set[3] = 0x71;
    mctp_header_eom_set[4] = MCTP_HDR_VER;
    mctp_header_eom_set[7] = LAST_PKT | 0x10;
    mctp_header_eom_set[8] = MCTP_MESSAGE_TYPE;

    assemble_and_send_mctp_pkt(mctp_ctx, (alt_u8*)test_data, 400);

    for (alt_u8 i = 0; i < 9; i++)
    {
        EXPECT_EQ(mctp_header_som_set[i], IORD(m_mctp_memory, 0));
    }
    for (alt_u8 i = 0; i < 249; i++)
    {
        EXPECT_EQ(IORD(m_mctp_memory, 0), test_data[i]);
    }
    for (alt_u8 i = 0; i < 9; i++)
    {
        EXPECT_EQ(mctp_header_eom_set[i], IORD(m_mctp_memory, 0));
    }
    for (alt_u8 i = 0; i < 151; i++)
    {
        EXPECT_EQ(IORD(m_mctp_memory, 0), test_data[i + 249]);
    }
}

TEST_F(PFRMctpFlowTest, test_write_four_mctp_pkt)
{
    alt_u8 test_data[900];
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));

    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    for (alt_u8 i = 0; i < 200; i++)
    {
        test_data[i] = 1;
    }
    for (alt_u8 i = 0; i < 200; i++)
    {
        test_data[i + 200] = 2;
    }
    for (alt_u8 i = 0; i < 200; i++)
    {
        test_data[i + 400] = 3;
    }
    for (alt_u32 i = 0; i < 300; i++)
    {
        test_data[i + 600] = 4;
    }

    alt_u8 dest_addr = BMC_SLAVE_ADDRESS;
    mctp_ctx->cached_addr = dest_addr;
    mctp_ctx->msg_type = MCTP_MESSAGE_TYPE;

    alt_u8 mctp_header_som_set[9] = {0};
    alt_u8 mctp_header_eom_set[9] = {0};
    alt_u8 mctp_header_middle[9] = {0};
    alt_u8 mctp_header_middle_2[9] = {0};

    mctp_header_som_set[0] = dest_addr << 1;
    mctp_header_som_set[1] = MCTP_COMMAND_CODE;
    mctp_header_som_set[2] = 255;
    mctp_header_som_set[3] = 0x71;
    mctp_header_som_set[4] = MCTP_HDR_VER;
    mctp_header_som_set[7] = FIRST_PKT;
    mctp_header_som_set[8] = MCTP_MESSAGE_TYPE;

    mctp_header_middle[0] = dest_addr << 1;
    mctp_header_middle[1] = MCTP_COMMAND_CODE;
    mctp_header_middle[2] = 255;
    mctp_header_middle[3] = 0x71;
    mctp_header_middle[4] = MCTP_HDR_VER;
    mctp_header_middle[7] = INTERMEDIARY_PKT | 0x10;
    mctp_header_middle[8] = MCTP_MESSAGE_TYPE;

    mctp_header_middle_2[0] = dest_addr << 1;
    mctp_header_middle_2[1] = MCTP_COMMAND_CODE;
    mctp_header_middle_2[2] = 255;
    mctp_header_middle_2[3] = 0x71;
    mctp_header_middle_2[4] = MCTP_HDR_VER;
    mctp_header_middle_2[7] = INTERMEDIARY_PKT | 0x20;
    mctp_header_middle_2[8] = MCTP_MESSAGE_TYPE;

    mctp_header_eom_set[0] = dest_addr << 1;
    mctp_header_eom_set[1] = MCTP_COMMAND_CODE;
    mctp_header_eom_set[2] = 159;
    mctp_header_eom_set[3] = 0x71;
    mctp_header_eom_set[4] = MCTP_HDR_VER;
    mctp_header_eom_set[7] = LAST_PKT | 0x30;
    mctp_header_eom_set[8] = MCTP_MESSAGE_TYPE;

    assemble_and_send_mctp_pkt(mctp_ctx, (alt_u8*)test_data, 900);

    for (alt_u8 i = 0; i < 9; i++)
    {
        EXPECT_EQ(mctp_header_som_set[i], IORD(m_mctp_memory, 0));
    }
    for (alt_u8 i = 0; i < 249; i++)
    {
        EXPECT_EQ(IORD(m_mctp_memory, 0), test_data[i]);
    }
    for (alt_u8 i = 0; i < 9; i++)
    {
        EXPECT_EQ(mctp_header_middle[i], IORD(m_mctp_memory, 0));
    }
    for (alt_u8 i = 0; i < 249; i++)
    {
        EXPECT_EQ(IORD(m_mctp_memory, 0), test_data[i + 249]);
    }
    for (alt_u8 i = 0; i < 9; i++)
    {
        EXPECT_EQ(mctp_header_middle_2[i], IORD(m_mctp_memory, 0));
    }
    for (alt_u8 i = 0; i < 249; i++)
    {
        EXPECT_EQ(IORD(m_mctp_memory, 0), test_data[i + 498]);
    }
    for (alt_u8 i = 0; i < 9; i++)
    {
        EXPECT_EQ(mctp_header_eom_set[i], IORD(m_mctp_memory, 0));
    }
    for (alt_u8 i = 0; i < 153; i++)
    {
        EXPECT_EQ(IORD(m_mctp_memory, 0), test_data[i + 747]);
    }
}
