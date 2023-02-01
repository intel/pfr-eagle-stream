#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class PFRSpdmNegTest : public testing::Test
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

TEST_F(PFRSpdmNegTest, test_responder_cpld_respond_version_bad_request_code)
{
    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);

    // Expect first message from host to request for version
    spdm_context_ptr->spdm_msg_state = SPDM_VERSION_FLAG;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    alt_u8* pkt = (alt_u8*)&mctp_context_ptr->processed_message.buffer;
    alt_u8 mctp_pkt[4] = SPDM_GET_VERSION_MSG;

    // Corrupt request code
    mctp_pkt[1] = 0;
    alt_u8_memcpy(pkt, (alt_u8*)&mctp_pkt[0], 4);

    EXPECT_EQ(responder_cpld_respond_version(spdm_context_ptr, mctp_context_ptr, 4), (alt_u32)SPDM_RESPONSE_ERROR);

}

TEST_F(PFRSpdmNegTest, test_requester_cpld_get_version_corrupt_byte_count)
{
    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);

    // Expect first message from host to request for version
    spdm_context_ptr->spdm_msg_state = 0;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    /**
     *
     * Test preparation
     *
     */

    // Obtain the example packet
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;

    // Corrupt byte count
    succ_ver_mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x55;
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 0, 0);

    // Expect error to be caught
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x11);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);
    // Expect timeout in this case
    EXPECT_EQ(requester_cpld_get_version(spdm_context_ptr, mctp_context_ptr), alt_u32(SPDM_RESPONSE_ERROR_TIMEOUT));

    alt_u8 expected_get_ver_msg[11] = MCTP_SPDM_GET_VERSION_MSG;
    expected_get_ver_msg[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    expected_get_ver_msg[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    expected_get_ver_msg[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(expected_get_ver_msg[i], IORD(m_mctp_memory, 0));
    }

}

TEST_F(PFRSpdmNegTest, test_responder_cpld_respond_capability_unsupported_spdm_version)
{
    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);

    // Expect first message from host to request for version
    spdm_context_ptr->spdm_msg_state = SPDM_CAPABILITIES_FLAG;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    alt_u8* pkt = (alt_u8*)&mctp_context_ptr->processed_message.buffer;
    alt_u8 mctp_pkt[4] = SPDM_GET_CAPABILITY_MSG;

    // Corrupt version
    mctp_pkt[0] = 0x55;

    alt_u8_memcpy(pkt, (alt_u8*)&mctp_pkt[0], 4);

    EXPECT_EQ(responder_cpld_respond_capabilities(spdm_context_ptr, mctp_context_ptr, 4), (alt_u32)SPDM_RESPONSE_ERROR);
}

TEST_F(PFRSpdmNegTest, test_requester_cpld_get_capabilities_incorrect_packet_info)
{
    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);

    // Expect first message from host to request for version
    spdm_context_ptr->spdm_msg_state = SPDM_CAPABILITIES_FLAG;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    /**
     *
     * Test preparation
     *
     */

    // Obtain the example packet
    alt_u8 succ_cap_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_CAPABILITY_MSG;
    succ_cap_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = LAST_PKT;

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, succ_cap_mctp_pkt, 19, 0, 0);

    EXPECT_EQ(requester_cpld_get_capability(spdm_context_ptr, mctp_context_ptr), alt_u32(SPDM_RESPONSE_ERROR_TIMEOUT));

    alt_u8 expected_get_cap_msg[11] = MCTP_SPDM_GET_CAPABILITY_MSG;
    expected_get_cap_msg[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    expected_get_cap_msg[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    expected_get_cap_msg[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(expected_get_cap_msg[i], IORD(m_mctp_memory, 0));
    }

}

TEST_F(PFRSpdmNegTest, test_requester_cpld_negotiate_algorithm_very_small_size)
{
    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);

    // Expect first message from host to request for version
    spdm_context_ptr->spdm_msg_state = SPDM_ALGORITHM_FLAG;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    /**
     *
     * Test preparation
     *
     */
    alt_u8 mctp_pkt[51] = MCTP_SPDM_SUCCESSFUL_ALGORITHM;
    mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;

    // Corrupt byte count to simulate wrong SPDM size
    mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 25;
    mctp_pkt[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    mctp_pkt[UT_MCTP_SPDM_ALGORITHM_LENGTH] = 0;
    alt_u16* length_ptr = (alt_u16*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_LENGTH];
    *length_ptr = sizeof(SUCCESSFUL_ALGORITHMS);
    mctp_pkt[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_SPEC] = SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF;

    alt_u32* succ_algo_ptr = (alt_u32*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_HASH_ALGO];
    *succ_algo_ptr = MEAS_HASH_ALGO_TPM_ALG_SHA_256 |
                     MEAS_HASH_ALGO_TPM_ALG_SHA_384;
    succ_algo_ptr = (alt_u32*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_BASE_ASYM_SEL];
    *succ_algo_ptr = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;
    succ_algo_ptr = (alt_u32*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_BASE_HASH_SEL];
    *succ_algo_ptr = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    // Send in message size as recorded in byte count in the MCTP layer
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt, 26, 0, 0);

    // Since the size are less, SPDM layer should catch this
    EXPECT_EQ(requester_cpld_negotiate_algorithm(spdm_context_ptr, mctp_context_ptr), alt_u32(SPDM_RESPONSE_ERROR));

    alt_u8 expected_nego_algo[55] = MCTP_SPDM_NEGOTIATE_ALGORITHM;
    expected_nego_algo[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    expected_nego_algo[UT_MCTP_BYTE_COUNT_IDX] = 38;
    expected_nego_algo[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    expected_nego_algo[UT_MCTP_SPDM_ALGORITHM_LENGTH] = 0;
    length_ptr = (alt_u16*)&expected_nego_algo[UT_MCTP_SPDM_ALGORITHM_LENGTH];
    *length_ptr = sizeof(NEGOTIATE_ALGORITHMS);
    expected_nego_algo[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_SPEC] = SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF;

    alt_u32* nego_algo_ptr = (alt_u32*)&expected_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_ASYM_ALGO];
    *nego_algo_ptr = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;
    nego_algo_ptr = (alt_u32*)&expected_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_HASH_ALGO];
    *nego_algo_ptr = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 39; i++)
    {
        EXPECT_EQ(expected_nego_algo[i], IORD(m_mctp_memory, 0));
    }
}

TEST_F(PFRSpdmNegTest, test_responder_cpld_respond_algorithm_massive_spdm_size)
{
    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);

    // Expect first message from host to request for version
    spdm_context_ptr->spdm_msg_state = SPDM_ALGORITHM_FLAG;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    alt_u8* pkt = (alt_u8*)&mctp_context_ptr->processed_message.buffer;
    alt_u8 mctp_pkt_nego_algo[40] = SPDM_NEGOTIATE_ALGORITHM_SHA_ECDSA_256_AND_384_CPLD_AS_RESPONDER;

    // Corrupt the size of spdm message
    mctp_pkt_nego_algo[UT_MCTP_SPDM_ALGORITHM_LENGTH - 7] = 0;
    alt_u16* length_ptr = (alt_u16*)&mctp_pkt_nego_algo[UT_MCTP_SPDM_ALGORITHM_LENGTH - 7];
    *length_ptr = sizeof(NEGOTIATE_ALGORITHMS) + sizeof(NEGOTIATE_ALGORITHMS);
    mctp_pkt_nego_algo[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_SPEC - 7] = SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF;

    alt_u32* nego_algo_ptr = (alt_u32*)&mctp_pkt_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_ASYM_ALGO - 7];
    *nego_algo_ptr = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;
    nego_algo_ptr = (alt_u32*)&mctp_pkt_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_HASH_ALGO - 7];
    *nego_algo_ptr = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;

    // Copy twice to simulate massive spdm size
    alt_u8_memcpy(pkt, (alt_u8*)&mctp_pkt_nego_algo[0], 40);
    alt_u8_memcpy(&pkt[40], (alt_u8*)&mctp_pkt_nego_algo[0], 40);

    EXPECT_EQ(responder_cpld_respond_algorithm(spdm_context_ptr, mctp_context_ptr, 63), (alt_u32)SPDM_RESPONSE_ERROR);
}

TEST_F(PFRSpdmNegTest, test_get_digest_as_requester_corrupt_byte_count)
{
    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);

    spdm_context_ptr->connection_info.algorithm.base_hash_algo = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
    //spdm_context_ptr->connection_info.algorithm.base_hash_algo[3] = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;

    // Expect first message from host to request for version
    spdm_context_ptr->spdm_msg_state = SPDM_DIGEST_FLAG;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    alt_u8 mctp_get_digest_pkt[11] = MCTP_SPDM_GET_DIGEST_MSG;
    mctp_get_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    mctp_get_digest_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    alt_u8 mctp_digest_pkt[59] = MCTP_SPDM_DIGEST_MSG;
    mctp_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // corrupt byte count
    mctp_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 44;
    mctp_digest_pkt[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_digest_pkt, 59, 0, 0);

    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);

    EXPECT_EQ(requester_cpld_get_digest(spdm_context_ptr, mctp_context_ptr), alt_u32(SPDM_RESPONSE_ERROR_TIMEOUT));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(mctp_get_digest_pkt[i], IORD(m_mctp_memory, 0));
    }

}

TEST_F(PFRSpdmNegTest, test_responder_cpld_respond_digest_wrong_request_code)
{
    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    alt_u8* pkt = (alt_u8*)&mctp_context_ptr->processed_message.buffer;
    alt_u8 mctp_get_digest_pkt[11] = MCTP_SPDM_GET_DIGEST_MSG;

    // Corrupt request code
    mctp_get_digest_pkt[8] = 0xff;
    mctp_get_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    mctp_get_digest_pkt[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;

    alt_u8_memcpy(pkt, (alt_u8*)&mctp_get_digest_pkt[7], 11 - 7);

    EXPECT_EQ(responder_cpld_respond_digest(spdm_context_ptr, mctp_context_ptr, sizeof(GET_DIGESTS)), (alt_u32)SPDM_RESPONSE_ERROR);

}

TEST_F(PFRSpdmNegTest, test_get_certificate_one_chain_as_requester_corrupt_certificate)
{
    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);

    spdm_context_ptr->connection_info.algorithm.base_hash_algo = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
    //spdm_context_ptr->connection_info.algorithm.base_hash_algo[3] = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;

    // Expect first message from host to request for version
    spdm_context_ptr->spdm_msg_state = SPDM_CERTIFICATE_FLAG;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    alt_u8 mctp_cert_packet_with_cert_chain[143] = {0};

    alt_u8 mctp_get_cert_pkt[15] = MCTP_SPDM_GET_CERTIFICATE_MSG;
    mctp_get_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0e;
    mctp_get_cert_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    //mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1] = (LARGE_SPDM_MESSAGE_MAX_BUFFER_SIZE/4) >> 8;
    alt_u16* mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = CERT_REQUEST_SIZE;

    alt_u8 mctp_cert_pkt[15] = MCTP_SPDM_CERTIFICATE_MSG;
    mctp_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 142;
    mctp_cert_pkt[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK] = 128;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK] = 0;
    alt_u16* mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 128;

    // Corrupt the certificate, last byte to 0xff from 0x22
    const alt_u8 td_crypto_data[128] = {
    		0x13, 0x91, 0x89, 0xca, 0xe6, 0x00, 0x0d, 0xa3, 0xce, 0x06, 0x00, 0xb5,
    		0x2e, 0x0a, 0x36, 0x31, 0xb3, 0x9f, 0xd2, 0x5b, 0x6c, 0x43, 0x1c, 0x07,
    		0x8a, 0x17, 0xf3, 0x64, 0xd6, 0x95, 0x6c, 0x53, 0xd4, 0xd2, 0x4c, 0x77,
    		0x2d, 0x01, 0x3f, 0x87, 0x4d, 0x4c, 0xd0, 0xa2, 0x81, 0x0a, 0xde, 0x59,
    		0x49, 0x44, 0x38, 0xd4, 0xf0, 0x76, 0x7f, 0x6d, 0xcd, 0x90, 0xd6, 0xd0,
			0xab, 0xac, 0xe3, 0xde, 0x11, 0x38, 0x3e, 0x6f, 0xb7, 0xd2, 0x40, 0x89,
    		0x4d, 0x80, 0x4c, 0x63, 0x2f, 0x3f, 0xc9, 0x9a, 0x26, 0xe0, 0x56, 0x5b,
    		0xf0, 0xf6, 0x59, 0xa1, 0xda, 0xc4, 0xd3, 0x38, 0xd3, 0x3a, 0x30, 0x6f,
    		0x0f, 0xda, 0xe2, 0x27, 0x36, 0xe4, 0xae, 0x50, 0x7f, 0x28, 0xed, 0xc5,
    		0x9c, 0x96, 0x88, 0x90, 0x9b, 0x97, 0xf0, 0xe5, 0x61, 0x0f, 0x6a, 0xa6,
    		0x62, 0x87, 0x3d, 0x9c, 0x6b, 0xe3, 0x21, 0xff};

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    for (alt_u8 i = 15; i < 15 + 128; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = td_crypto_data[i - 15];
    }

    spdm_context_ptr->local_context.cert_flag = 1;
    alt_u8* stored_digest_buf_ptr = (alt_u8*)&spdm_context_ptr->local_context.stored_digest_buf[0];

    const alt_u8 td_expected_hash[SHA384_LENGTH] = {
    	0xa5, 0x18, 0xa7, 0x8e, 0x9f, 0x6a, 0x73, 0xdd, 0x61, 0x13, 0x73, 0x0e,
    	0x04, 0xca, 0x9c, 0xba, 0x16, 0x6c, 0x8f, 0x48, 0xa4, 0xf2, 0x45, 0x72,
    	0xe1, 0xc1, 0xf9, 0x5b, 0xbd, 0xa2, 0x09, 0x06, 0xe5, 0x63, 0x75, 0x24,
    	0xcf, 0x67, 0x78, 0x38, 0x64, 0x92, 0xd7, 0x3e, 0x6a, 0xa0, 0x6b, 0x2e};

    for (alt_u8 i = 0; i < SHA384_LENGTH; i++)
    {
        stored_digest_buf_ptr[i] = td_expected_hash[i];
    }

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 143, 0, 0);

    EXPECT_EQ(requester_cpld_get_certificate(spdm_context_ptr, mctp_context_ptr), alt_u32(SPDM_RESPONSE_ERROR));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(mctp_get_cert_pkt[i], IORD(m_mctp_memory, 0));
    }
}

TEST_F(PFRSpdmNegTest, test_get_certificate_multiple_chain_as_requester_corrupt_fourth_cert_chain)
{
    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);

    spdm_context_ptr->connection_info.algorithm.base_hash_algo = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
    //spdm_context_ptr->connection_info.algorithm.base_hash_algo[3] = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;

    // Expect first message from host to request for version
    spdm_context_ptr->spdm_msg_state = SPDM_CERTIFICATE_FLAG;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    alt_u8 mctp_cert_packet_with_cert_chain[500] = {0};

    alt_u8 mctp_get_cert_pkt[15] = MCTP_SPDM_GET_CERTIFICATE_MSG;
    mctp_get_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0e;
    mctp_get_cert_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    //mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    alt_u16* mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = CERT_REQUEST_SIZE;
    alt_u16* mctp_get_cert_pkt_ptr_2 = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr_2 = 0;

    alt_u8 mctp_cert_pkt[15] = MCTP_SPDM_CERTIFICATE_MSG;
    mctp_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 206;
    mctp_cert_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);
    //mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK] = 128;
    //mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK] = 0;
    alt_u16* mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 192;
    alt_u16* mctp_get_pkt_ptr_2 = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr_2 = 576;

    // Corrupt the last byte from 0xf1 to 0x11
	const alt_u8 test_data[768] = {
			  0x09, 0x1c, 0x81, 0x91, 0xc4, 0x1d, 0x8b, 0x0f, 0x94, 0x65, 0x51, 0xa9,
			  0x5f, 0xcf, 0x95, 0xa5, 0x77, 0x7f, 0x9b, 0xa5, 0x73, 0xde, 0xf4, 0xf7,
			  0xfd, 0xf2, 0x0d, 0xe5, 0x49, 0xe1, 0x68, 0x41, 0xeb, 0x0a, 0x10, 0x19,
			  0x67, 0x86, 0xed, 0x4d, 0x3d, 0xeb, 0x64, 0xda, 0x00, 0x31, 0x68, 0x76,
			  0xf1, 0x2d, 0x8e, 0x41, 0xaf, 0x76, 0xee, 0x69, 0xbb, 0x97, 0x8c, 0xc8,
			  0xf0, 0x74, 0x7e, 0x8a, 0x11, 0x7a, 0x96, 0x1c, 0x49, 0x89, 0x3d, 0x6d,
			  0xb1, 0x5c, 0xaf, 0x2c, 0x18, 0xe6, 0xc6, 0xbd, 0x47, 0x3e, 0x16, 0xcb,
			  0x71, 0xb2, 0x56, 0x97, 0xbe, 0xa7, 0xe9, 0x5b, 0x18, 0x51, 0xaa, 0x14,
			  0xaa, 0xbc, 0xd0, 0x4c, 0x4d, 0xa0, 0x09, 0x41, 0xe0, 0x85, 0x89, 0x1f,
			  0x21, 0xf8, 0x63, 0x00, 0xaa, 0x4d, 0x4c, 0xed, 0x06, 0x4d, 0x67, 0xf9,
			  0x2d, 0x51, 0xa5, 0xac, 0xf5, 0x7b, 0x32, 0x77,
			  0xc7, 0x71, 0xe9, 0xb9, 0x6b, 0x60, 0x41, 0x41, 0xde, 0xbe, 0x6d, 0xb8,
			  0xbf, 0xb3, 0x72, 0xea, 0xf2, 0x61, 0x78, 0xe1, 0x03, 0x81, 0x7e, 0x41,
			  0x6f, 0xc1, 0xdd, 0x27, 0xe8, 0x64, 0x5f, 0xfd, 0x37, 0xb5, 0x85, 0x93,
			  0xc9, 0x13, 0x8e, 0x47, 0x53, 0x95, 0xdb, 0xee, 0xc4, 0x29, 0x87, 0x9a,
			  0xca, 0xcf, 0x67, 0x8d, 0x65, 0xd0, 0xe6, 0xe9, 0xf7, 0x09, 0x8a, 0xc9,
			  0x04, 0x90, 0xec, 0xd1, 0x18, 0x79, 0xc3, 0x38, 0x9f, 0xee, 0x8c, 0x5b,
			  0x1b, 0x9a, 0x0d, 0xb9, 0xf9, 0xff, 0xe5, 0x18, 0x8d, 0x76, 0x0a, 0x10,
			  0x19, 0x1f, 0x4a, 0x9a, 0xdd, 0x1e, 0x56, 0xdd, 0xca, 0x13, 0xa7, 0xe3,
			  0x31, 0xdd, 0x1a, 0xe6, 0x94, 0x77, 0x93, 0x08, 0x6a, 0x5f, 0x45, 0x82,
			  0x06, 0x63, 0x68, 0x25, 0x88, 0x86, 0x9f, 0x7b, 0x04, 0xe3, 0xa3, 0xd5,
			  0x3b, 0x5f, 0x1d, 0x9a, 0x39, 0xc8, 0xf5, 0xa2,
			  0x49, 0xfd, 0x14, 0x87, 0xbf, 0x27, 0x86, 0x89, 0x0e, 0x4a, 0x8a, 0x6a,
			  0x48, 0xb8, 0x08, 0xd5, 0x1b, 0xb5, 0x3a, 0x26, 0xbd, 0x5e, 0xdc, 0x8b,
			  0x2a, 0xeb, 0x71, 0xdc, 0x5c, 0x9e, 0x69, 0x5c, 0xdd, 0x5a, 0x8c, 0x1a,
			  0x3f, 0x62, 0xb1, 0x6c, 0xf4, 0x36, 0x74, 0x45, 0xa0, 0xc8, 0xe8, 0x86,
			  0x11, 0xa2, 0x5e, 0x30, 0x22, 0x00, 0xf2, 0xc7, 0x17, 0x48, 0xc8, 0xdd,
			  0x09, 0x66, 0xa0, 0x64, 0x69, 0x05, 0x25, 0x1c, 0x7a, 0x20, 0x68, 0x85,
			  0x86, 0xfd, 0xfe, 0xfe, 0x11, 0xb0, 0x2c, 0x11, 0x9e, 0x45, 0xe0, 0x47,
			  0xf1, 0xc1, 0x2f, 0xe2, 0xd3, 0xa2, 0xa6, 0x62, 0xf3, 0x40, 0x94, 0xdd,
			  0xac, 0x5b, 0x09, 0x53, 0x94, 0xdb, 0xa9, 0xe6, 0xc4, 0x36, 0xb3, 0x0b,
			  0x3f, 0x77, 0xfd, 0xa9, 0x98, 0x99, 0x53, 0x9b, 0x82, 0x65, 0x9c, 0xbe,
			  0xac, 0x60, 0x7b, 0x3e, 0x3b, 0xec, 0x9e, 0x79,
			  0xa4, 0x9c, 0x05, 0x20, 0x40, 0x06, 0xa8, 0x57, 0x81, 0xbe, 0xe6, 0x34,
			  0x8d, 0xd3, 0x63, 0x66, 0x7e, 0x82, 0x5f, 0x21, 0x57, 0x16, 0x74, 0xd4,
			  0x55, 0x3d, 0xa2, 0x38, 0xc7, 0x3b, 0xba, 0x18, 0x77, 0x89, 0xd4, 0xbb,
			  0xbb, 0x28, 0x80, 0xe2, 0x8a, 0xbd, 0x42, 0x3e, 0x11, 0x38, 0x59, 0x42,
			  0x5a, 0x70, 0x06, 0xa1, 0x1c, 0x32, 0x00, 0x95, 0xfb, 0x73, 0x47, 0x0e,
			  0xa0, 0x7b, 0x1b, 0x47, 0x06, 0x83, 0xec, 0x09, 0x3a, 0xa5, 0xe3, 0x67,
			  0x27, 0xfa, 0xd9, 0x76, 0x18, 0x61, 0xc0, 0x33, 0x83, 0x9b, 0x04, 0x87,
			  0xf3, 0xb9, 0xc8, 0x88, 0xf8, 0xc2, 0x34, 0x20, 0xfa, 0x91, 0xae, 0x40,
			  0x3e, 0x7b, 0x4c, 0x1b, 0xf7, 0xf9, 0x38, 0xb2, 0x4a, 0xc9, 0x0a, 0xc6,
			  0xed, 0x61, 0x5e, 0x86, 0x2d, 0x5d, 0x1b, 0x3e, 0x8a, 0xd9, 0x6b, 0x53,
			  0x2d, 0xbf, 0x95, 0x28, 0x43, 0xd4, 0x66, 0xe9,
			  0x7b, 0x80, 0xe0, 0x81, 0x45, 0x20, 0x94, 0x11, 0x4a, 0xcf, 0x80, 0x36,
			  0xb0, 0x8e, 0xbc, 0xe9, 0xe0, 0x4d, 0xb1, 0xf5, 0x0e, 0x78, 0xf7, 0x57,
			  0x73, 0xce, 0x87, 0x7c, 0xc9, 0x4d, 0x2b, 0xd9, 0x81, 0x48, 0xcd, 0x20,
			  0x33, 0xb1, 0xee, 0xa5, 0x69, 0x20, 0x9c, 0x6c, 0x0b, 0xdc, 0xbf, 0x5c,
			  0xb2, 0xf1, 0x0f, 0x20, 0xba, 0x41, 0x21, 0xfd, 0x13, 0xff, 0xd6, 0x5c,
			  0x92, 0x31, 0x8c, 0xbc, 0x72, 0xf4, 0xd2, 0xcb, 0x37, 0x1c, 0x79, 0x83,
			  0x5b, 0x27, 0x27, 0xc3, 0x7a, 0xe9, 0x99, 0x46, 0x08, 0x4e, 0xe6, 0x70,
			  0x87, 0x17, 0x82, 0x66, 0x1f, 0x67, 0xf8, 0x9f, 0x88, 0x2e, 0x1f, 0x13,
			  0x9b, 0xb1, 0xfe, 0xd2, 0x63, 0x7b, 0xac, 0x42, 0xda, 0x45, 0x4f, 0x61,
			  0x6d, 0xb5, 0x5f, 0x08, 0xd4, 0xdb, 0x54, 0x38, 0x24, 0xc2, 0x0c, 0xca,
			  0x8e, 0xfb, 0x0c, 0x6e, 0x21, 0x8a, 0x9a, 0xdd,
			  0x23, 0xf9, 0xe8, 0xcb, 0xed, 0xb0, 0x34, 0x30, 0xed, 0x6a, 0x86, 0xa5,
			  0xaf, 0xd9, 0xf2, 0x34, 0x99, 0x81, 0x62, 0x63, 0x07, 0xf5, 0x2b, 0x19,
			  0x1f, 0xd1, 0x08, 0xe6, 0x98, 0x6f, 0x3f, 0xf3, 0x9d, 0xba, 0x96, 0x2e,
			  0x72, 0x65, 0xd3, 0x6b, 0x71, 0x85, 0x40, 0x7c, 0x54, 0x64, 0x77, 0xe4,
			  0x45, 0xa7, 0x87, 0x99, 0xbf, 0x87, 0x22, 0xaa, 0x07, 0xa7, 0x86, 0x52,
			  0xd6, 0x3e, 0xf7, 0x56, 0x4c, 0x84, 0x14, 0xe5, 0x5a, 0x2f, 0xb8, 0x69,
			  0x2c, 0xd8, 0x34, 0x34, 0xa9, 0x78, 0xb7, 0xb4, 0x4b, 0x6a, 0xbc, 0x8a,
			  0xa8, 0x13, 0x94, 0xda, 0x3f, 0x65, 0xf0, 0xc1, 0x84, 0x59, 0x65, 0x71,
			  0x6b, 0x8e, 0xd9, 0xfc, 0x40, 0x36, 0x18, 0xf7, 0xfb, 0x3e, 0xc4, 0x2c,
			  0xc8, 0xd8, 0xaa, 0x50, 0xba, 0x9d, 0x3c, 0x38, 0xb4, 0xbd, 0xf1, 0x7a,
			  0x5d, 0x06, 0xe9, 0x84, 0x94, 0x01, 0x57, 0x11
	};

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    for (alt_u8 i = 15; i < 15 + 192; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = test_data[i - 15];
    }

    spdm_context_ptr->local_context.cert_flag = 1;
    alt_u8* stored_digest_buf_ptr = (alt_u8*)&spdm_context_ptr->local_context.stored_digest_buf[0];

	const alt_u8 expected_hash[SHA384_LENGTH] = {
			  0xd1, 0x61, 0xbe, 0xce, 0xf2, 0x91, 0x4c, 0xb9, 0xd7, 0x32, 0x31, 0xac,
			  0x4f, 0xe0, 0x44, 0xc1, 0xfc, 0xa6, 0xee, 0xe2, 0x58, 0x8b, 0x41, 0x8c,
			  0x89, 0xb7, 0xfa, 0x56, 0x4f, 0x62, 0x14, 0x4b, 0xca, 0xa8, 0x2a, 0xdb,
			  0x3d, 0xa1, 0x64, 0xc1, 0xc2, 0xbe, 0xff, 0x08, 0x66, 0xea, 0x89, 0x28
	};

    for (alt_u8 i = 0; i < SHA384_LENGTH; i++)
    {
        stored_digest_buf_ptr[i] = expected_hash[i];
    }

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 207, 1, 0);

    // Set remainder
    *mctp_get_pkt_ptr_2 = 384;

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    for (alt_u8 i = 15; i < 15 + 192; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = test_data[i - 15 + 192];
    }

    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 207, 1, 0);

    // Set remainder
    *mctp_get_pkt_ptr_2 = 192;

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    for (alt_u8 i = 15; i < 15 + 192; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = test_data[i - 15 + 192 + 192];
    }

    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 207, 1, 0);

    // Set remainder to zero
    *mctp_get_pkt_ptr_2 = 0;

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    for (alt_u8 i = 15; i < 15 + 192; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = test_data[i - 15 + 192 + 192 + 192];
    }

    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 207, 0, 0);

    EXPECT_EQ(requester_cpld_get_certificate(spdm_context_ptr, mctp_context_ptr), alt_u32(SPDM_RESPONSE_ERROR));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(mctp_get_cert_pkt[i], IORD(m_mctp_memory, 0));
    }

    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 192;

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(mctp_get_cert_pkt[i], IORD(m_mctp_memory, 0));
    }

    // Expect that CPLD request the remaining portion of the certificate
    *mctp_get_cert_pkt_ptr = 200;
    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 384;

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(mctp_get_cert_pkt[i], IORD(m_mctp_memory, 0));
    }

    // Expect that CPLD request the remaining portion of the certificate
    *mctp_get_cert_pkt_ptr = 192;
    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 576;

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(mctp_get_cert_pkt[i], IORD(m_mctp_memory, 0));
    }
}

TEST_F(PFRSpdmNegTest, test_responder_cpld_respond_certificate_single_chain_invalid_slot_mask)
{
    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    //alt_u8 mctp_cert_packet_with_cert_chain[143] = {0};

    alt_u8* pkt = (alt_u8*)&mctp_context_ptr->processed_message.buffer;
    alt_u8 mctp_get_cert_pkt[15] = MCTP_SPDM_GET_CERTIFICATE_MSG;
    mctp_get_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0e;
    mctp_get_cert_pkt[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    //mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1] = (LARGE_SPDM_MESSAGE_MAX_BUFFER_SIZE/4) >> 8;
    alt_u16* mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 128;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0;

    // Corrupt slot mask
    mctp_get_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_SLOT_NUM_MASK] = 5;

    alt_u8_memcpy(pkt, (alt_u8*)&mctp_get_cert_pkt[7], 15 - 7);

    EXPECT_EQ(responder_cpld_respond_certificate(spdm_context_ptr, mctp_context_ptr, sizeof(GET_CERTIFICATE)), alt_u32(SPDM_RESPONSE_ERROR));
}

TEST_F(PFRSpdmNegTest, test_responder_cpld_respond_certificate_multiple_chain_bad_request_code_on_second_request)
{
    // Reset Nios firmware
    ut_reset_nios_fw();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    // Run Nios FW through PFR/Recovery Main to generate keys
    ut_run_main(CPLD_CFM0, true);

    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    alt_u8 mctp_cert_packet_with_cert_chain[500] = {0};

    alt_u8* pkt = (alt_u8*)&mctp_context_ptr->processed_message.buffer;
    alt_u8 mctp_get_cert_pkt[15] = MCTP_SPDM_GET_CERTIFICATE_MSG;
    mctp_get_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0e;
    mctp_get_cert_pkt[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;
    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    //mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1] = (LARGE_SPDM_MESSAGE_MAX_BUFFER_SIZE/4) >> 8;
    alt_u16* mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 128;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0;

    alt_u8_memcpy(pkt, (alt_u8*)&mctp_get_cert_pkt[7], 15 - 7);

    alt_u8 mctp_cert_pkt[15] = MCTP_SPDM_CERTIFICATE_MSG;
    mctp_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 142;
    mctp_cert_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK] = 0;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK] = 0;
    alt_u16* mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 128;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    alt_u16 cert_length = *((alt_u16*)get_ufm_cpld_cert_ptr());
    *mctp_get_pkt_ptr = cert_length - 128;

    alt_u8* cert = (alt_u8*) get_ufm_cpld_cert_ptr();
    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    for (alt_u8 i = 15; i < 15 + 128; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert[i - 15];
    }

    EXPECT_EQ(responder_cpld_respond_certificate(spdm_context_ptr, mctp_context_ptr, sizeof(GET_CERTIFICATE)), alt_u32(0));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 143; i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    // Corrupt byte count on second chain request
    mctp_get_cert_pkt[7] = 0x0;
    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    //mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1] = (LARGE_SPDM_MESSAGE_MAX_BUFFER_SIZE/4) >> 8;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 128;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 128;

    alt_u8_memcpy(pkt, (alt_u8*)&mctp_get_cert_pkt[7], 15 - 7);

    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK] = 0;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK] = 0;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 128;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = cert_length - 128 - 128;

    cert = (alt_u8*) get_ufm_cpld_cert_ptr();
    cert += 128;

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    for (alt_u8 i = 15; i < 15 + 128; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert[i - 15];
    }

    EXPECT_EQ(responder_cpld_respond_certificate(spdm_context_ptr, mctp_context_ptr, sizeof(GET_CERTIFICATE)), alt_u32(SPDM_RESPONSE_ERROR));
}

TEST_F(PFRSpdmNegTest, test_cpld_requester_challenge_public_key_compromised)
{
    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);

    spdm_context_ptr->connection_info.algorithm.base_hash_algo = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
    spdm_context_ptr->connection_info.algorithm.base_asym_algo = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
    spdm_context_ptr->capability.flags |= SUPPORTS_MEAS_RESPONSE_WITH_SIG_GEN_MSG_MASK;

    // Set uuid 2
    spdm_context_ptr->uuid = 2;

    // Expect first message from host to request for version
    spdm_context_ptr->spdm_msg_state = SPDM_CHALLENGE_FLAG;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    alt_u8 mctp_pkt_challenge[43] = MCTP_SPDM_CHALLENGE_MSG;
    mctp_pkt_challenge[UT_CHALLENGE_SPDM_SLOT_NUMBER_IDX] = 0;
    mctp_pkt_challenge[UT_CHALLENGE_SPDM_MEAS_SUMMARY_HASH_TYPE_IDX] = 0xff;
    mctp_pkt_challenge[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_pkt_challenge[UT_MCTP_BYTE_COUNT_IDX] = 42;
    mctp_pkt_challenge[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    alt_u8 mctp_pkt_challenge_auth[239] = MCTP_SPDM_CHALLENGE_AUTH_MSG;
    mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_SLOT_NUMBER_IDX] = 0x0;
    mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_SLOT_MASK_IDX] = 0x01;
    // TODO: Might need to fix endianness
    //mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_OPAQUE_LENGTH_IDX] = 0;
    alt_u16* mctp_pkt_challenge_auth_ptr = (alt_u16*)&mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_OPAQUE_LENGTH_IDX];
    *mctp_pkt_challenge_auth_ptr = 0x02;
    //mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_OPAQUE_LENGTH_IDX + 1] = 0x02;
    mctp_pkt_challenge_auth[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_pkt_challenge_auth[UT_MCTP_BYTE_COUNT_IDX] = 238;
    mctp_pkt_challenge_auth[UT_MCTP_SOURCE_ADDR] = 0x02;

    alt_u8 cert_chain_hash[SHA384_LENGTH] = {
    0x8d, 0x72, 0xb6, 0x20, 0xe1, 0x59, 0xb8, 0x52, 0xa3, 0x89, 0x31, 0x0e,
    0x03, 0x7c, 0x7e, 0x15, 0x93, 0x69, 0x87, 0xc4, 0xab, 0x82, 0x43, 0xfb,
    0xb3, 0xbb, 0x4b, 0xfd, 0x8b, 0x5a, 0xa7, 0x8d, 0x4b, 0xa2, 0xca, 0x36,
    0x3c, 0xe7, 0x29, 0x9c, 0x61, 0x0e, 0xb0, 0x2a, 0x75, 0x4e, 0x5a, 0x65
    };

    alt_u32* ptr = (alt_u32*) cert_chain_hash;
    alt_u32* pkt_ptr = (alt_u32*)&mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_CERT_CHAIN_HASH_IDX];

    for (alt_u32 i = 0; i < SHA384_LENGTH/4; i++)
    {
        pkt_ptr[i] = ptr[i];
    }

    alt_u8 priv_key[48] = {
      	  0xc9, 0x9a, 0x06, 0xda, 0x8b, 0xad, 0x33, 0xd6, 0xee, 0x2d, 0x62, 0x36,
      	  0x7b, 0x9c, 0x5a, 0x4e, 0xf7, 0x0d, 0xc8, 0x6f, 0x54, 0xd8, 0x82, 0x27,
      	  0xa5, 0x1d, 0x5a, 0x71, 0xe3, 0x95, 0x9a, 0xdf, 0xc3, 0xd1, 0x52, 0x71,
      	  0x43, 0x95, 0xe3, 0x63, 0xc7, 0xe0, 0xc0, 0xd5, 0x49, 0x14, 0xf7, 0xbc
    };

    CRYPTO_MODE sha_mode = CRYPTO_384_MODE;
    alt_u32 sha_size = SHA384_LENGTH;
/*#if defined(GTEST_ATTEST_256)

    sha_mode = CRYPTO_256_MODE;
    sha_size = SHA256_LENGTH;

#endif*/
    // Need to make sure hash match so will inject data to transcript manually
    const alt_u8 test_data[128] = {
    	  0xc9, 0x9a, 0x06, 0xda, 0x8b, 0xad, 0x33, 0xd6, 0xee, 0x2d, 0x62, 0x36,
    	  0x7b, 0x9c, 0x5a, 0x4e, 0xf7, 0x0d, 0xc8, 0x6f, 0x54, 0xd8, 0x82, 0x27,
    	  0xa5, 0x1d, 0x5a, 0x71, 0xe3, 0x95, 0x9a, 0xdf, 0xc3, 0xd1, 0x52, 0x71,
    	  0x43, 0x95, 0xe3, 0x63, 0xc7, 0xe0, 0xc0, 0xd5, 0x49, 0x14, 0xf7, 0xbc,
    	  0xa2, 0x2a, 0x80, 0xb6, 0x69, 0x38, 0x61, 0xad, 0x4b, 0xf8, 0x42, 0x6d,
    	  0xc8, 0x3e, 0x2a, 0x7d, 0xb1, 0x21, 0x00, 0x0b, 0x09, 0x56, 0x01, 0x9f,
    	  0xd1, 0xd2, 0x83, 0x92, 0x5a, 0x0b, 0xd9, 0x97, 0xba, 0x20, 0x47, 0x3e,
    	  0xa1, 0xec, 0xfd, 0xe6, 0xe8, 0x3b, 0x81, 0x9e, 0x31, 0x9e, 0x05, 0x57,
    	  0x89, 0xa2, 0xa2, 0xe9, 0x20, 0xd4, 0xce, 0xc1, 0x1f, 0x3e, 0xb7, 0xd4,
    	  0xe2, 0x97, 0xc4, 0xc6, 0x56, 0x6a, 0xd3, 0x66, 0xe0, 0x2f, 0x3f, 0xd5,
    	  0x95, 0xd2, 0xf3, 0x3c, 0x8b, 0xb7, 0x60, 0xd7
    };

    alt_u8* test_data_ptr = (alt_u8*)test_data;
    /*append_small_buffer(&spdm_context_ptr->transcript.message_a, test_data_ptr, 64);

    test_data_ptr += 64;
    append_large_buffer(&spdm_context_ptr->transcript.message_b, test_data_ptr, 64);

    append_small_buffer(&spdm_context_ptr->transcript.message_c, (alt_u8*)&mctp_pkt_challenge[7], 36);
    append_small_buffer(&spdm_context_ptr->transcript.message_c, (alt_u8*)&mctp_pkt_challenge_auth[7], 232 - 96);*/

    append_large_buffer(&spdm_context_ptr->transcript.m1_m2,
                        test_data_ptr, 64);
    test_data_ptr += 64;
    append_large_buffer(&spdm_context_ptr->transcript.m1_m2,
                        test_data_ptr, 64);
    append_large_buffer(&spdm_context_ptr->transcript.m1_m2,
                        (alt_u8*)&mctp_pkt_challenge[7], 36);
    append_large_buffer(&spdm_context_ptr->transcript.m1_m2,
                        (alt_u8*)&mctp_pkt_challenge_auth[7], 232 - 96);
    /*
    append_large_buffer(&spdm_context_ptr->transcript.m1_m2,
                        (alt_u8*)&spdm_context_ptr->transcript.message_a.buffer[0], spdm_context_ptr->transcript.message_a.buffer_size);
    append_large_buffer(&spdm_context_ptr->transcript.m1_m2,
                        (alt_u8*)&spdm_context_ptr->transcript.message_b.buffer[0], spdm_context_ptr->transcript.message_b.buffer_size);
    append_large_buffer(&spdm_context_ptr->transcript.m1_m2,
                        (alt_u8*)&spdm_context_ptr->transcript.message_c.buffer[0], spdm_context_ptr->transcript.message_c.buffer_size);
*/
    alt_u8 test_pubkey_cx[SHA384_LENGTH] = {0};
    alt_u8 test_pubkey_cy[SHA384_LENGTH] = {0};
    alt_u8 sig_r[SHA384_LENGTH] = {0};
    alt_u8 sig_s[SHA384_LENGTH] = {0};

    alt_u8* msg_to_hash = (alt_u8*)&spdm_context_ptr->transcript.m1_m2.buffer[0];

    // Manually generate the public key here
    generate_pubkey((alt_u32*)test_pubkey_cx, (alt_u32*)test_pubkey_cy, (alt_u32*)priv_key, sha_mode);

    // Manually generate signature
    generate_ecdsa_signature((alt_u32*)sig_r, (alt_u32*)sig_s, (alt_u32*)msg_to_hash, spdm_context_ptr->transcript.m1_m2.buffer_size,
    	               (alt_u32*)test_pubkey_cx, (alt_u32*)priv_key, sha_mode);

    // Simulate case where attacker replace sender's own public keys
    test_pubkey_cx[0] = 0xff;
    test_pubkey_cx[1] = 0xff;
    test_pubkey_cx[2] = 0xff;
    test_pubkey_cx[3] = 0xff;

    pkt_ptr = (alt_u32*)&mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_SIGNATURE_IDX];

    ptr = (alt_u32*) sig_r;
    for (alt_u32 i = 0; i < (sha_size >> 2); i++)
    {
        pkt_ptr[i] = ptr[i];
    }

    pkt_ptr += (SHA384_LENGTH >> 2);
    ptr = (alt_u32*) sig_s;
    for (alt_u32 i = 0; i < (sha_size >> 2); i++)
    {
        pkt_ptr[i] = ptr[i];
    }

    // Need to inject the public key into UFM
    // Need to set the uuid

    // Begin the test by having an empty UFM on the AFM storage region
    alt_u32* ufm_afm_pointer = get_ufm_ptr_with_offset(UFM_AFM_1);
    EXPECT_EQ(*ufm_afm_pointer, (alt_u32)0xffffffff);

    // Now, we filled in the AFM with UUID. Assume UUID is 2
    // We filled in the first region in the UFM.
    *((alt_u16*)ufm_afm_pointer) = spdm_context_ptr->uuid;

    // For testing, store the public key into the ufm
    // For simplicity just tied this test to uuid of 2
    ufm_afm_pointer = ((alt_u32*)ufm_afm_pointer) + 10;
    alt_u32* key = (alt_u32*)test_pubkey_cx;
    for (alt_u32 i = 0; i < sha_size/4; i++)
    {
        ufm_afm_pointer[i] = key[i];
    }

    ufm_afm_pointer += sha_size/4;
    key = (alt_u32*)test_pubkey_cy;
    for (alt_u32 i = 0; i < sha_size/4; i++)
    {
        ufm_afm_pointer[i] = key[i];
    }

    alt_u32 msg_a_and_msg_b_size = spdm_context_ptr->transcript.m1_m2.buffer_size - (36 + 232 - 96);

    // Clear buffer m1m2 and msg c as this will be appended by the function internally as well
    reset_buffer((alt_u8*)&spdm_context_ptr->transcript.m1_m2.buffer[msg_a_and_msg_b_size], 36 + 232 - 96);
    spdm_context_ptr->transcript.m1_m2.buffer_size -= (36 + 232 - 96);
    //reset_buffer((alt_u8*)&spdm_context_ptr->transcript.message_c, sizeof(SMALL_APPENDED_BUFFER));

    // Send in the packet
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt_challenge_auth, 239, 0, 0);

    EXPECT_EQ(requester_cpld_challenge(spdm_context_ptr, mctp_context_ptr), (alt_u32)SPDM_RESPONSE_ERROR_CHAL_AUTH);

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(mctp_pkt_challenge[i], IORD(m_mctp_memory, 0));
    }
}

TEST_F(PFRSpdmNegTest, test_cpld_responder_challenge_massive_request_size)
{
    // Reset Nios firmware
    ut_reset_nios_fw();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    // Run Nios FW through PFR/Recovery Main to generate keys
    ut_run_main(CPLD_CFM0, true);

    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);

    // Set uuid 2
    spdm_context_ptr->uuid = 2;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    alt_u8* pkt = (alt_u8*)&mctp_context_ptr->processed_message.buffer;

    alt_u8 mctp_pkt_challenge[43] = MCTP_SPDM_CHALLENGE_MSG;
    mctp_pkt_challenge[UT_CHALLENGE_SPDM_SLOT_NUMBER_IDX] = 0;
    mctp_pkt_challenge[UT_CHALLENGE_SPDM_MEAS_SUMMARY_HASH_TYPE_IDX] = 0xff;
    mctp_pkt_challenge[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_pkt_challenge[UT_MCTP_BYTE_COUNT_IDX] = 42;
    mctp_pkt_challenge[UT_MCTP_SOURCE_ADDR] = 0x02;

    alt_u8_memcpy(pkt, (alt_u8*)&mctp_pkt_challenge[7], 43 - 7);

    alt_u8 mctp_pkt_challenge_auth[239] = MCTP_SPDM_CHALLENGE_AUTH_MSG;
    mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_SLOT_NUMBER_IDX] = 0x0;
    mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_SLOT_MASK_IDX] = 0x01;
    // TODO: Might need to fix endianness
    //mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_OPAQUE_LENGTH_IDX] = 0;
    alt_u16* mctp_pkt_challenge_auth_ptr = (alt_u16*)&mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_OPAQUE_LENGTH_IDX];
    *mctp_pkt_challenge_auth_ptr = 0x02;
    mctp_pkt_challenge_auth_ptr = (alt_u16*)&mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_OPAQUE_LENGTH_IDX + 2];
    *mctp_pkt_challenge_auth_ptr = 0xCAFE;
    //mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_OPAQUE_LENGTH_IDX + 1] = 0x02;
    mctp_pkt_challenge_auth[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_pkt_challenge_auth[UT_MCTP_BYTE_COUNT_IDX] = 238;
    mctp_pkt_challenge_auth[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    // This value is fixed in unit test to match pre-set values in firmware so that the hash over these values
    // can be computed correctly.
    for (alt_u32 i = 0; i < SPDM_NONCE_SIZE; i++)
    {
        mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_NONCE_IDX + i] = 0x11;
    }

    for (alt_u32 i = 0; i < SHA384_LENGTH; i++)
    {
        mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_CERT_MEAS_SUMMARY_HASH_IDX + i] = 0x11;
    }

    for (alt_u32 i = 0; i < SHA384_LENGTH; i++)
    {
        mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_CERT_CHAIN_HASH_IDX + i] = 0x11;
    }

    // Append the message for signature verification
    append_large_buffer(&spdm_context_ptr->transcript.l1_l2, (alt_u8*)&mctp_pkt_challenge[7], 43 - 7);

    // Do not append the signature here
    append_large_buffer(&spdm_context_ptr->transcript.l1_l2, (alt_u8*)&mctp_pkt_challenge_auth[7], 239 - 7 - 96);
    // For this test, we need to extract the public key manually in the test

    // When the spdm request size is very large
    EXPECT_EQ(responder_cpld_respond_challenge_auth(spdm_context_ptr, mctp_context_ptr, sizeof(CHALLENGE) + 50), alt_u32(SPDM_RESPONSE_ERROR));
}

TEST_F(PFRSpdmNegTest, test_cpld_requester_get_measurement_attacker_use_own_signature)
{
    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);

    spdm_context_ptr->connection_info.algorithm.base_hash_algo = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
    spdm_context_ptr->connection_info.algorithm.base_asym_algo = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
    spdm_context_ptr->capability.flags |= SUPPORTS_MEAS_RESPONSE_WITH_SIG_GEN_MSG_MASK;
    //spdm_context_ptr->capability.flags[3] |= SUPPORTS_MEAS_RESPONSE_WITH_SIG_GEN_MSG_MASK;
    //spdm_context_ptr->connection_info.algorithm.base_hash_algo[3] = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
    //spdm_context_ptr->connection_info.algorithm.base_asym_algo[3] = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;

    // Set uuid 2
    spdm_context_ptr->uuid = 2;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    alt_u8 get_measurement_pkt[43] = MCTP_SPDM_GET_MEASUREMENT_MSG;
    get_measurement_pkt[UT_GET_MEASUREMENT_REQUEST_ATTRIBUTE] = SPDM_GET_MEASUREMENT_REQUEST_GENERATE_SIG;
    get_measurement_pkt[UT_GET_MEASUREMENT_OPERATION_MEASUREMENT_NUMBER] = 0x01;
    get_measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    get_measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 42;
    get_measurement_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    alt_u8 measurement_pkt[218] = MCTP_SPDM_MEASUREMENT_MSG;
    // Shall test for 1 block of measurement
    measurement_pkt[UT_MEASUREMENT_TOTAL_NUMBER_OF_BLOCKS] = 1;
    measurement_pkt[UT_MEASUREMENT_RECORD_LENGTH] = UT_RECORD_LENGTH;
    measurement_pkt[UT_MEASUREMENT_RECORD_LENGTH + 1] = 0x00;
    measurement_pkt[UT_MEASUREMENT_RECORD_LENGTH + 2] = 0x00;
    measurement_pkt[UT_MEASUREMENT_RECORD] = 0;

    // For simplicity, just randomly fill in the measurement hash
    // Do the same for AFM in the ufm after this
    for (alt_u32 i = 0; i < UT_RECORD_LENGTH; i++)
    {
        measurement_pkt[UT_MEASUREMENT_RECORD + i] = 0xbb;
    }

    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH] = UT_OPAQUE_LENGTH;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 1] = 0x0;
    measurement_pkt[UT_MEASUREMENT_SIGNATURE] = 0;
    measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 217;
    measurement_pkt[UT_MCTP_SOURCE_ADDR] = 0x02;

    alt_u32* pkt_ptr = (alt_u32*)&measurement_pkt[UT_MEASUREMENT_SIGNATURE];

    CRYPTO_MODE sha_mode = CRYPTO_384_MODE;
    alt_u32 sha_size = SHA384_LENGTH;

    alt_u8 priv_key[48] = {
      	  0xc9, 0x9a, 0x06, 0xda, 0x8b, 0xad, 0x33, 0xd6, 0xee, 0x2d, 0x62, 0x36,
      	  0x7b, 0x9c, 0x5a, 0x4e, 0xf7, 0x0d, 0xc8, 0x6f, 0x54, 0xd8, 0x82, 0x27,
      	  0xa5, 0x1d, 0x5a, 0x71, 0xe3, 0x95, 0x9a, 0xdf, 0xc3, 0xd1, 0x52, 0x71,
      	  0x43, 0x95, 0xe3, 0x63, 0xc7, 0xe0, 0xc0, 0xd5, 0x49, 0x14, 0xf7, 0xbc
    };

    LARGE_APPENDED_BUFFER big_buffer;
    LARGE_APPENDED_BUFFER* big_buffer_ptr = &big_buffer;
    reset_buffer((alt_u8*)big_buffer_ptr, sizeof(LARGE_APPENDED_BUFFER));

    append_large_buffer(big_buffer_ptr, (alt_u8*)&get_measurement_pkt[7], 36);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&measurement_pkt[7], (211 - 96));

    alt_u8 test_pubkey_cx[SHA384_LENGTH] = {0};
    alt_u8 test_pubkey_cy[SHA384_LENGTH] = {0};
    alt_u8 sig_r[SHA384_LENGTH] = {0};
    alt_u8 sig_s[SHA384_LENGTH] = {0};

    alt_u8* msg_to_hash = (alt_u8*)big_buffer_ptr->buffer;

    // Manually generate the public key here
    generate_pubkey((alt_u32*)test_pubkey_cx, (alt_u32*)test_pubkey_cy, (alt_u32*)priv_key, sha_mode);

    // Manually generate signature
    generate_ecdsa_signature((alt_u32*)sig_r, (alt_u32*)sig_s, (alt_u32*)msg_to_hash, big_buffer_ptr->buffer_size,
    	               (alt_u32*)test_pubkey_cx, (alt_u32*)priv_key, sha_mode);

    // Change the signature R
    sig_r[0] = 0xff;
    sig_r[1] = 0xee;
    sig_r[2] = 0xdd;
    sig_r[3] = 0xcc;

    alt_u32* ptr = (alt_u32*) sig_r;
    for (alt_u32 i = 0; i < (sha_size >> 2); i++)
    {
        pkt_ptr[i] = ptr[i];
    }

    pkt_ptr += (sha_size >> 2);
    ptr = (alt_u32*) sig_s;
    for (alt_u32 i = 0; i < (sha_size >> 2); i++)
    {
        pkt_ptr[i] = ptr[i];
    }

    // Begin the test by having an empty UFM on the AFM storage region
    alt_u32* ufm_afm_pointer = get_ufm_ptr_with_offset(UFM_AFM_1);
    EXPECT_EQ(*ufm_afm_pointer, (alt_u32)0xffffffff);

    // Now, we filled in the AFM with UUID. Assume UUID is 2
    // We filled in the first region in the UFM.
    *((alt_u16*)ufm_afm_pointer) = 2;

    // For testing, store the public key into the ufm
    // For simplicity just tied this test to uuid of 2
    ufm_afm_pointer = ((alt_u32*)ufm_afm_pointer) + 10;
    alt_u32* key = (alt_u32*)test_pubkey_cx;
    for (alt_u32 i = 0; i < sha_size/4; i++)
    {
        ufm_afm_pointer[i] = key[i];
    }

    ufm_afm_pointer += sha_size/4;
    key = (alt_u32*)test_pubkey_cy;
    for (alt_u32 i = 0; i < sha_size/4; i++)
    {
        ufm_afm_pointer[i] = key[i];
    }

    ufm_afm_pointer = get_ufm_ptr_with_offset(UFM_AFM_1);
    // For testing, store measurement hash into the ufm
    // Set total measurement to 1 for one index only
    ufm_afm_pointer = ufm_afm_pointer + 140;
    *ufm_afm_pointer = 1;
    // Make sure to set the possible measurement number correctly
    alt_u8* alt_u8_ufm_afm_pointer = (alt_u8*) ufm_afm_pointer;
    alt_u8_ufm_afm_pointer = alt_u8_ufm_afm_pointer + 1;
    // Set 1 since there are one measurement
    *alt_u8_ufm_afm_pointer = 1;

    // Get the offset to the measurement hash
    alt_u8_ufm_afm_pointer = alt_u8_ufm_afm_pointer + 3;
    for (alt_u32 i = 0; i < 64; i++)
    {
        // This is just a random chosen value. It can be any value as long as it is consistent for hash calculation
    	alt_u8_ufm_afm_pointer[i] = 0xbb;
    }

    // Send in the packet
    ut_send_in_bmc_mctp_packet(m_memory, measurement_pkt, 218, 0, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    EXPECT_EQ(requester_cpld_get_measurement(spdm_context_ptr, mctp_context_ptr, 1), (alt_u32)SPDM_RESPONSE_ERROR);

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(get_measurement_pkt[i], IORD(m_mctp_memory, 0));
    }

}

TEST_F(PFRSpdmNegTest, test_respond_measurement_as_cpld_responder_without_supporting_signature_but_still_requested)
{
    // Reset Nios firmware
    ut_reset_nios_fw();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    // Run Nios FW through PFR/Recovery Main to generate keys
    ut_run_main(CPLD_CFM0, true);

    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);

    spdm_context_ptr->connection_info.algorithm.base_hash_algo = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
    spdm_context_ptr->connection_info.algorithm.base_asym_algo = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;

    // Case wehre CPLD DO NOT support measurement with signature
    // thus, cancel it from the spdm context
    //spdm_context_ptr->capability.flags &= ~SUPPORTS_MEAS_RESPONSE_WITH_SIG_GEN_MSG_MASK;
    spdm_context_ptr->capability.flags = SUPPORTS_MEAS_RESPONSE_WITHOUT_SIG_GEN_MSG_MASK;

    // Set uuid 2
    spdm_context_ptr->uuid = 2;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    alt_u8* pkt = (alt_u8*)&mctp_context_ptr->processed_message.buffer;

    alt_u8 get_measurement_pkt[43] = MCTP_SPDM_GET_MEASUREMENT_MSG;

    // If cpld do not support signature generation, requester should not request for signature
    get_measurement_pkt[UT_GET_MEASUREMENT_REQUEST_ATTRIBUTE] = SPDM_GET_MEASUREMENT_REQUEST_GENERATE_SIG;
    get_measurement_pkt[UT_GET_MEASUREMENT_OPERATION_MEASUREMENT_NUMBER] = 0x01;
    get_measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    get_measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 42;
    get_measurement_pkt[UT_MCTP_SOURCE_ADDR] = 0x02;

    alt_u8_memcpy(pkt, (alt_u8*)&get_measurement_pkt[7], 43 - 7);

    // 202
    alt_u8 measurement_pkt[218] = MCTP_SPDM_MEASUREMENT_MSG;
    // Shall test for 1 block of measurement
    measurement_pkt[UT_MEASUREMENT_TOTAL_NUMBER_OF_BLOCKS] = 1;
    measurement_pkt[UT_MEASUREMENT_RECORD_LENGTH] = 55;
    measurement_pkt[UT_MEASUREMENT_RECORD_LENGTH + 1] = 0x00;
    measurement_pkt[UT_MEASUREMENT_RECORD_LENGTH + 2] = 0x00;

    measurement_pkt[UT_MEASUREMENT_RECORD] = 1;
    measurement_pkt[UT_MEASUREMENT_RECORD + 1] = 1;
    measurement_pkt[UT_MEASUREMENT_RECORD + 2] = 51;
    measurement_pkt[UT_MEASUREMENT_RECORD + 3] = 0x00;
    measurement_pkt[UT_MEASUREMENT_RECORD + 4] = 0x01;
    measurement_pkt[UT_MEASUREMENT_RECORD + 5] = 48;
    measurement_pkt[UT_MEASUREMENT_RECORD + 6] = 0x00;

    alt_u32* cfm1_hash = get_ufm_ptr_with_offset(UFM_CPLD_CFM1_STORED_HASH);
    alt_u32* meas_pkt = (alt_u32*) &measurement_pkt[UT_MEASUREMENT_RECORD + 7];
    // For simplicity, just randomly fill in the measurement hash
    // Do the same for AFM in the ufm after this
    /*for (alt_u32 i = 0; i < 48; i++)
    {
        measurement_pkt[UT_MEASUREMENT_RECORD + 7 + i] = 0x11;
    }*/
    for (alt_u32 i = 0; i < 12; i++)
    {
        meas_pkt[i] = cfm1_hash[i];
    }

    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH - 16] = UT_OPAQUE_LENGTH;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 1 - 16] = 0x0;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 2 - 16] = 0xfe;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 3 - 16] = 0xca;
    measurement_pkt[UT_MEASUREMENT_SIGNATURE - 16] = 0;
    measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 201;
    measurement_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    // Append the message for signature verification
    append_large_buffer(&spdm_context_ptr->transcript.m1_m2, (alt_u8*)&get_measurement_pkt[7], 43 - 7);

    // Do not append the signature here
    append_large_buffer(&spdm_context_ptr->transcript.m1_m2, (alt_u8*)&measurement_pkt[7], 202 - 7 - 96);
    // For this test, we need to extract the public key manually in the test

    EXPECT_EQ(responder_cpld_respond_measurement(spdm_context_ptr, mctp_context_ptr, sizeof(GET_MEASUREMENT)), alt_u32(SPDM_RESPONSE_ERROR));

}
