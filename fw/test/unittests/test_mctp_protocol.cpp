#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class PFRMctpTest : public testing::Test
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

TEST_F(PFRMctpTest, test_bmc_send_first_mctp_message)
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
    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)25);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0);
}

TEST_F(PFRMctpTest, test_bmc_send_consequtively_first_mctp_packet)
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

    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)25);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)25);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0);
}

TEST_F(PFRMctpTest, test_bmc_send_repeated_first_mctp_packet)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_SOM_AND_EOM_HIGH_REPLICA;
    alt_u8 bmc_mctp_intermediary_packet[32] = BMC_MCTP_INTERMEDIARY_CHALLENGE_PACKET_REPLICA;
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
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_intermediary_packet, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);

    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)25);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);
}

TEST_F(PFRMctpTest, test_bmc_send_wrong_size_mctp_packet)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[32] = BMC_MCTP_CHALLENGE_PACKET_EXCESS_BYTE_COUNT_REPLICA;
    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));

    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x11);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);

    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0);
}

/**
 * These tests here exercise the corner cases of multi MCTP packet.
 *
 * 1) Host send two correct packet.
 * 2) Host send first packet with excessive byte count and second correct packet.
 * 3) Host send first packet with excessive packet size and correct second packet.
 * 4) Host send correct first packet and excessive byte count in the second packet.
 * 5) Host send correct first packet and excessive packet size in the second packet.
 * 6) Host send two excessive byte count packet
 * 7) Host send two packet with excessive size
 * 8) Host send one excessive byte count packet followed by packet with excessive size
 * 9) Host send packet with excessive size followed by one excessive byte count packet
 * 10) Host send four packets (4 correct packets)
 */
TEST_F(PFRMctpTest, test_bmc_send_two_correct_mctp_packet)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_SOM_HIGH_REPLICA;
    alt_u8 bmc_mctp_intermediary_packet[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_EOM_HIGH_REPLICA;
    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));

    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x1);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_intermediary_packet, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);

    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)50);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0);

}

TEST_F(PFRMctpTest, test_bmc_send_first_packet_with_excess_byte_count_and_second_correct_mctp_packet)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[32] = BMC_MCTP_CHALLENGE_PACKET_EXCESS_BYTE_COUNT_REPLICA;
    alt_u8 bmc_mctp_intermediary_packet[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_EOM_HIGH_REPLICA;
    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));

    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x11);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_intermediary_packet, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x13);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);

    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x00);

}

TEST_F(PFRMctpTest, test_bmc_send_first_packet_with_excess_packet_size_and_second_correct_mctp_packet)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[60] = BMC_MCTP_CHALLENGE_PACKET_EXCESS_PACKET_SIZE_REPLICA;
    alt_u8 bmc_mctp_intermediary_packet[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_EOM_HIGH_REPLICA;
    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));

    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 60, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x11);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_intermediary_packet, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x13);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);

    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x00);

}

TEST_F(PFRMctpTest, test_bmc_send_first_correct_mctp_packet_and_second_packet_with_excess_byte_count)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_SOM_HIGH_REPLICA;
    alt_u8 bmc_mctp_intermediary_packet[32] = BMC_MCTP_CHALLENGE_PACKET_EXCESS_BYTE_COUNT_REPLICA;
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
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_intermediary_packet, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x23);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);

    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0);

}

TEST_F(PFRMctpTest, test_bmc_send_first_correct_mctp_packet_and_second_packet_with_excess_packet_size_count)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_SOM_HIGH_REPLICA;
    alt_u8 bmc_mctp_intermediary_packet[60] = BMC_MCTP_CHALLENGE_PACKET_EXCESS_PACKET_SIZE_REPLICA;
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
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_intermediary_packet, 60, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x23);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);

    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0);

}

TEST_F(PFRMctpTest, test_bmc_send_two_packets_with_excess_byte_count)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[32] = BMC_MCTP_CHALLENGE_PACKET_EXCESS_BYTE_COUNT_REPLICA;
    alt_u8 bmc_mctp_intermediary_packet[32] = BMC_MCTP_CHALLENGE_PACKET_EXCESS_BYTE_COUNT_REPLICA;
    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));

    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x11);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_intermediary_packet, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x33);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);

    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x00);

}

TEST_F(PFRMctpTest, test_bmc_send_two_excessive_packet_size_mctp_packet)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[60] = BMC_MCTP_CHALLENGE_PACKET_EXCESS_PACKET_SIZE_REPLICA;
    alt_u8 bmc_mctp_intermediary_packet[60] = BMC_MCTP_CHALLENGE_PACKET_EXCESS_PACKET_SIZE_REPLICA;
    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));

    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 60, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x11);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_intermediary_packet, 60, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x33);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);

    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x00);

}

TEST_F(PFRMctpTest, test_bmc_send_first_packet_with_excess_byte_count_and_second_excess_size_mctp_packet)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[32] = BMC_MCTP_CHALLENGE_PACKET_EXCESS_BYTE_COUNT_REPLICA;
    alt_u8 bmc_mctp_intermediary_packet[60] = BMC_MCTP_CHALLENGE_PACKET_EXCESS_PACKET_SIZE_REPLICA;
    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));

    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x11);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_intermediary_packet, 60, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x33);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);

    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x00);

}

TEST_F(PFRMctpTest, test_bmc_send_first_packet_with_excess_packet_size_and_second_excess_byte_count_mctp_packet)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[60] = BMC_MCTP_CHALLENGE_PACKET_EXCESS_PACKET_SIZE_REPLICA;
    alt_u8 bmc_mctp_intermediary_packet[32] = BMC_MCTP_CHALLENGE_PACKET_EXCESS_BYTE_COUNT_REPLICA;
    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));

    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 60, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x11);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_intermediary_packet, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x33);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);

    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x00);

}

TEST_F(PFRMctpTest, DISABLED_test_bmc_send_four_correct_mctp_packet)
{
    // Obtain the replica of an mctp packet
    alt_u8 bmc_mctp_first_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_SOM_HIGH_REPLICA;
    alt_u8 bmc_mctp_intermediary_packet[32] = BMC_MCTP_CHALLENGE_MIDDLE_PACKET_REPLICA;
    alt_u8 bmc_mctp_last_message[32] = BMC_MCTP_CHALLENGE_PACKET_WITH_EOM_HIGH_REPLICA;
    alt_u8 mctp_processed_pkt[1024];
    alt_u8 dest_addr = 0;
    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));

    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x1);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_intermediary_packet, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_intermediary_packet, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x07);

    // Write the mctp packet into the Write RXFIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_last_message, 32, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0xf);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);

    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)100);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0);

}

TEST_F(PFRMctpTest, test_bmc_send_multiple_packets_of_a_single_message)
{
    // Message which are too big will be divided into smaller mctp packets
    // This test simulates the scenario where host sends in multiple packets to the mailbox FIFO.

    MCTP_CONTEXT ctx;
    MCTP_CONTEXT* mctp_ctx = &ctx;
    reset_buffer((alt_u8*)mctp_ctx, sizeof(MCTP_CONTEXT));

    // Set Challenger host in order to point to correct location
    mctp_ctx->challenger_type = BMC;

    alt_u8 bmc_mctp_first_packet[256] = {0};
    alt_u8 dest_addr = 0;
    // MCTP packet header but without destination address and mctp command code.
    // This is to follow the actual RTL processing
    bmc_mctp_first_packet[0] = 255;
    bmc_mctp_first_packet[5] = FIRST_PKT;
    bmc_mctp_first_packet[6] = MCTP_MESSAGE_TYPE;
    for (alt_u8 i = 0; i < 249; i++)
    {
    	bmc_mctp_first_packet[i + 7] = 0xaa;
    }

    alt_u8 bmc_mctp_intermediary_packet[256] = {0};
    bmc_mctp_intermediary_packet[0] = 255;
    bmc_mctp_intermediary_packet[5] = INTERMEDIARY_PKT;
    bmc_mctp_intermediary_packet[6] = MCTP_MESSAGE_TYPE;
    for (alt_u8 i = 0; i < 249; i++)
    {
    	bmc_mctp_intermediary_packet[i + 7] = 0xbb;
    }

    alt_u8 bmc_mctp_last_packet[98] = {0};
    bmc_mctp_last_packet[0] = 97;
    bmc_mctp_last_packet[5] = LAST_PKT;
    bmc_mctp_last_packet[6] = MCTP_MESSAGE_TYPE;
    for (alt_u8 i = 0; i < 91; i++)
    {
    	bmc_mctp_last_packet[i + 7] = 0xcc;
    }

    alt_u8 mctp_processed_pkt[1024] = {0};

    // Begin data transaction
    // BMC starts sending in the first MCTP packet to FIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_first_packet, 256, 0, 0);

    // Upon receiving the packet, if the packet did not violate the byte count, no error bit should be set
    // In either cases, the available will be set starting from LSB
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // In the platform level, the RTL will re-process the byte count value for FW to read
    // The mailbox mock is designed to do the same.
    // BMC starts sending in the second MCTP packet to FIFO
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_intermediary_packet, 256, 0, 0);

    // Upon receiving the packet, if the packet did not violate the byte count, no error bit should be set
    // In either cases, the available will be set starting from LSB
    // As the available bit is not set yet, the bit for the second packet is push following the available bit of the first packet
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    // In the platform level, the RTL will re-process the byte count value for FW to read
    // The mailbox mock is designed to do the same.
    // Finally, BMC sends in the final packet
    ut_send_in_bmc_mctp_packet(m_memory, bmc_mctp_last_packet, 98, 0, 0);

    // Upon receiving the packet, if the packet did not violate the byte count, no error bit should be set
    // In either cases, the available will be set starting from LSB
    // As the available bit is not set yet, the bit for the final packet is push following the available bit of the second packet
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x07);

    // In the platform level, the RTL will re-process the byte count value for FW to read
    // The mailbox mock is designed to do the same.

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);
    // At this stage, the BMC FIFO is filled with the MCTP packets and waiting for FW to read and pop out the data.
    EXPECT_EQ(responder_cpld_process_mctp_pkt(mctp_ctx, (alt_u8*)mctp_processed_pkt, &dest_addr), (alt_u32)589);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0);

    // FW will be processing the SOM and EOM to make sure the packets are in the right order: first > middle > last
    // As the order is correct, we expect no error reported in the mailbox

    for (alt_u32 i = 0; i < 249; i++)
    {
        EXPECT_EQ(mctp_processed_pkt[i], bmc_mctp_first_packet[i + 7]);
    }
    for (alt_u32 i = 0; i < 249; i++)
    {
        EXPECT_EQ(mctp_processed_pkt[i + 249], bmc_mctp_intermediary_packet[i + 7]);
    }
    for (alt_u32 i = 0; i < 91; i++)
    {
        EXPECT_EQ(mctp_processed_pkt[i + 249 + 249], bmc_mctp_last_packet[i + 7]);
    }
}
