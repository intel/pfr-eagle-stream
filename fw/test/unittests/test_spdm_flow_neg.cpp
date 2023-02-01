#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class PFRSpdmFlowNegativeTest : public testing::Test
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

TEST_F(PFRSpdmFlowNegativeTest, test_corrupt_mctp_get_version_message_cpld_as_responder)
{
    // Obtain the example message for get_version
    alt_u8 get_ver_mctp_pkt[11] = MCTP_SPDM_GET_VERSION_MSG;
    // Modify the SOM/EOM
    get_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
    get_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    // Corrupt the get version message
    get_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_IDX] = 0;

    // Begin Test
    ut_send_in_bmc_mctp_packet(m_memory, get_ver_mctp_pkt, 11, 0, 0);

   // Initialize context
    MCTP_CONTEXT mctp_ctx;
    MCTP_CONTEXT* mctp_ctx_ptr = (MCTP_CONTEXT*) &mctp_ctx;

    // Clear buffer
    reset_buffer((alt_u8*)mctp_ctx_ptr, sizeof(MCTP_CONTEXT));

    // Set Challenger host in order to point to correct location
    mctp_ctx_ptr->challenger_type = BMC;

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    receive_i2c_message(mctp_ctx_ptr, 1);
    cpld_checks_request_from_host(mctp_ctx_ptr);

}

/***
 * Sends in two get_version message consequtively.
 * The first get_version message has SOM set to high
 * Second one has SOM and EOM set to high
 *
 */
TEST_F(PFRSpdmFlowNegativeTest, test_multiple_get_version_message)
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
    ut_run_main(CPLD_CFM1, false);

    // Obtain the example message for get_version
    alt_u8 get_ver_mctp_pkt[11] = MCTP_SPDM_GET_VERSION_MSG;
    // Modify the SOM/EOM
    get_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_PKT;
    // Modify the source address
    get_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    // Obtain the example message for get_version
    alt_u8 get_ver_mctp_pkt_2[11] = MCTP_SPDM_GET_VERSION_MSG;
    // Modify the SOM/EOM
    get_ver_mctp_pkt_2[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
    get_ver_mctp_pkt_2[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    // Obtain the example packet
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    succ_ver_mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 14;
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX] = 0x01;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX + 1] = 0;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX + 2] = 0x10;

    ut_send_in_bmc_mctp_packet(m_memory, get_ver_mctp_pkt, 11, 0, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    ut_send_in_bmc_mctp_packet(m_memory, get_ver_mctp_pkt_2, 11, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    alt_u8 get_cap_msg[11] = MCTP_SPDM_GET_CAPABILITY_MSG;
    get_cap_msg[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    get_cap_msg[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);
    get_cap_msg[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;

    alt_u8 succ_cap_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_CAPABILITY_MSG;
    succ_cap_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    succ_cap_mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 18;
    succ_cap_mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    succ_cap_mctp_pkt[12] = 1;
    alt_u32* cap_ptr = (alt_u32*)&succ_cap_mctp_pkt[15];
    *cap_ptr = SUPPORTS_DIGEST_CERT_RESPONSE_MSG_MASK |
               SUPPORTS_CHALLENGE_AUTH_RESPONSE_MSG_MASK |
               SUPPORTS_MEAS_RESPONSE_WITH_SIG_GEN_MSG_MASK;

    // Stage next message
    ut_send_in_bmc_mctp_packet(m_memory, get_cap_msg, 11, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    alt_u8 mctp_pkt_nego_algo[55] = MCTP_SPDM_NEGOTIATE_ALGORITHM;
    mctp_pkt_nego_algo[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_pkt_nego_algo[UT_MCTP_BYTE_COUNT_IDX] = 38;
    mctp_pkt_nego_algo[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);
    mctp_pkt_nego_algo[UT_MCTP_SPDM_ALGORITHM_LENGTH] = 0;
    alt_u16* length_ptr = (alt_u16*)&mctp_pkt_nego_algo[UT_MCTP_SPDM_ALGORITHM_LENGTH];
    *length_ptr = sizeof(NEGOTIATE_ALGORITHMS);
    mctp_pkt_nego_algo[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_SPEC] = SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF;
    alt_u32* nego_algo_ptr = (alt_u32*)&mctp_pkt_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_ASYM_ALGO];
    *nego_algo_ptr = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;
    nego_algo_ptr = (alt_u32*)&mctp_pkt_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_HASH_ALGO];
    *nego_algo_ptr = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;

    alt_u8 mctp_pkt[51] = MCTP_SPDM_SUCCESSFUL_ALGORITHM;
    mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 42;
    mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    mctp_pkt[UT_MCTP_SPDM_ALGORITHM_LENGTH] = 0;
    length_ptr = (alt_u16*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_LENGTH];
    *length_ptr = sizeof(SUCCESSFUL_ALGORITHMS);
    mctp_pkt[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_SPEC] = SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF;
    alt_u32* succ_algo_ptr = (alt_u32*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_HASH_ALGO];
    *succ_algo_ptr = MEAS_HASH_ALGO_TPM_ALG_SHA_384;
    succ_algo_ptr = (alt_u32*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_BASE_ASYM_SEL];
    *succ_algo_ptr = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
    succ_algo_ptr = (alt_u32*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_BASE_HASH_SEL];
    *succ_algo_ptr = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;

    mctp_pkt[UT_MCTP_SPDM_ALGORITHM_EXT_ASYM_SEL_COUNT] = 0;
    mctp_pkt[UT_MCTP_SPDM_ALGORITHM_EXT_HASH_SEL_COUNT] = 0;

    // Last message, need not stage next message
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt_nego_algo, 39, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    alt_u8 mctp_get_digest_pkt[11] = MCTP_SPDM_GET_DIGEST_MSG;
    mctp_get_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    mctp_get_digest_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    alt_u8 mctp_digest_pkt[59] = MCTP_SPDM_DIGEST_MSG;
    mctp_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 58;
    mctp_digest_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    mctp_digest_pkt[UT_MCTP_SPDM_DIGEST_SLOT_MASK] = 0x1;

    alt_u8 td_expected_hash[SHA384_LENGTH] = {0};
    alt_u8* cptr = (alt_u8*)get_ufm_cpld_cert_ptr();
    CERT_CHAIN_FORMAT* cert_chain_digest = (CERT_CHAIN_FORMAT*)cptr;

    alt_u32 cert_length_digest = cert_chain_digest->length;

    calculate_and_save_sha((alt_u32*)td_expected_hash, 0, get_ufm_cpld_cert_ptr(), (alt_u32)cert_length_digest,
    		CRYPTO_384_MODE, DISENGAGE_DMA);

    for (alt_u8 i = 0; i < SHA384_LENGTH; i++)
    {
        mctp_digest_pkt[UT_MCTP_SPDM_DIGEST_SLOT_MASK + 1 + i] = td_expected_hash[i];
    }

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_digest_pkt, 11, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);
//////////////////////////////////////////////////////////////////

    alt_u8 mctp_cert_packet_with_cert_chain[500] = {0};

    alt_u8 mctp_get_cert_pkt[15] = MCTP_SPDM_GET_CERTIFICATE_MSG;
    mctp_get_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0e;
    mctp_get_cert_pkt[UT_MCTP_SOURCE_ADDR] = ((BMC_SLAVE_ADDRESS << 1) | 0b1);
    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;

    alt_u16* mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0;

    alt_u8 mctp_cert_pkt[15] = MCTP_SPDM_CERTIFICATE_MSG;
    mctp_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0xd6;
    mctp_cert_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK] = 0;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK] = 0;
    alt_u16* mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 0xc8;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    alt_u16 cert_length = *((alt_u16*)get_ufm_cpld_cert_ptr());
    *mctp_get_pkt_ptr = cert_length - 0xc8;

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 1, 0);

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;

    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0xc8;

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 1, 0);

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;

    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x190;

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 1, 0);

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x258;

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 1, 0);

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x320;

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 1, 0);

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x3e8;

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 1, 0);

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x4b0;

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 1, 0);

    alt_u8 mctp_pkt_challenge[43] = MCTP_SPDM_CHALLENGE_MSG;
    mctp_pkt_challenge[UT_CHALLENGE_SPDM_SLOT_NUMBER_IDX] = 0;
    mctp_pkt_challenge[UT_CHALLENGE_SPDM_MEAS_SUMMARY_HASH_TYPE_IDX] = 0xff;
    mctp_pkt_challenge[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_pkt_challenge[UT_MCTP_BYTE_COUNT_IDX] = 42;
    mctp_pkt_challenge[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

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

    alt_u8 get_measurement_pkt[43] = MCTP_SPDM_GET_MEASUREMENT_MSG;
    get_measurement_pkt[UT_GET_MEASUREMENT_REQUEST_ATTRIBUTE] = SPDM_GET_MEASUREMENT_REQUEST_GENERATE_SIG;
    get_measurement_pkt[UT_GET_MEASUREMENT_OPERATION_MEASUREMENT_NUMBER] = 0x01;
    get_measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    get_measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 42;
    get_measurement_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

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
        meas_pkt[i] = cfm1_hash[i];;
    }

    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH - 16] = UT_OPAQUE_LENGTH;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 1 - 16] = 0x0;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 2 - 16] = 0xfe;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 3 - 16] = 0xca;
    measurement_pkt[UT_MEASUREMENT_SIGNATURE - 16] = 0;
    measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 201;
    measurement_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    LARGE_APPENDED_BUFFER large_buf;
    LARGE_APPENDED_BUFFER* large_buf_ptr = &large_buf;


    reset_buffer((alt_u8*)large_buf_ptr, sizeof(LARGE_APPENDED_BUFFER));

    // Append the message for signature verification
    append_large_buffer(large_buf_ptr, (alt_u8*)&get_measurement_pkt[7], 43 - 7);

    // Do not append the signature here
    append_large_buffer(large_buf_ptr, (alt_u8*)&measurement_pkt[7], 202 - 7 - 96);

    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt_challenge, 43, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    ut_send_in_bmc_mctp_packet(m_memory, get_measurement_pkt, 43, 0, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

   // Initialize context
    MCTP_CONTEXT mctp_ctx;
    MCTP_CONTEXT* mctp_ctx_ptr = (MCTP_CONTEXT*) &mctp_ctx;

    // Clear buffer
    reset_buffer((alt_u8*)mctp_ctx_ptr, sizeof(MCTP_CONTEXT));

    // Set Challenger host in order to point to correct location
    mctp_ctx_ptr->challenger_type = BMC;

    receive_i2c_message(mctp_ctx_ptr, 1);
    // Cpld should be able to detect the request from the RXFIFO for the BMC
    cpld_checks_request_from_host(mctp_ctx_ptr);

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(succ_ver_mctp_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 19; i++)
    {
        EXPECT_EQ(succ_cap_mctp_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(mctp_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 59; i++)
    {
        EXPECT_EQ(mctp_digest_pkt[i], IORD(m_mctp_memory, 0));
    }
////////////////////////////////////////

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;

    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0;

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    alt_u8* cert = (alt_u8*) get_ufm_cpld_cert_ptr();
    for (alt_u8 i = 15; i < 15 + 0xc8; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert[i - 15];
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15 + 0xc8; i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;

    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0xc8;

    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK] = 0;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK] = 0;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 0xc8;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = cert_length - 0xc8 - 0xc8;

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    cert += 0xc8;

    for (alt_u8 i = 15; i < 15 + 0xc8; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert[i - 15];
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15 + 0xc8; i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;

    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x190;

    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK] = 0;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK] = 0;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 0xc8;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = cert_length - 0xc8 - 0xc8 - 0xc8;

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    cert += 0xc8;

    for (alt_u8 i = 15; i < 15 + 0xc8; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert[i - 15];
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15 + 0xc8; i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x258;

    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK] = 0;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK] = 0;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 0xc8;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = cert_length - 0xc8 - 0xc8 - 0xc8 - 0xc8;

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    cert += 0xc8;

    for (alt_u8 i = 15; i < 15 + 0xc8; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert[i - 15];
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15 + 0xc8; i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x320;

    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK] = 0;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK] = 0;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 0xc8;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = cert_length - 0xc8 - 0xc8 - 0xc8 - 0xc8 - 0xc8;

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    cert += 0xc8;

    for (alt_u8 i = 15; i < 15 + 0xc8; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert[i - 15];
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15 + 0xc8; i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x3e8;

    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK] = 0;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK] = 0;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 0xc8;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = cert_length - 0xc8 - 0xc8 - 0xc8 - 0xc8 - 0xc8 - 0xc8;

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    cert += 0xc8;

    for (alt_u8 i = 15; i < 15 + 0xc8; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert[i - 15];
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15 + 0xc8; i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x4b0;

    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK] = 0;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK] = 0;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = cert_length - 0xc8 - 0xc8 - 0xc8 - 0xc8 - 0xc8 - 0xc8;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 0;

    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];

    mctp_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = (alt_u8)(*mctp_get_pkt_ptr) + 0x0e;

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    cert += 0xc8;

    for (alt_u8 i = 15; i < 15 + (alt_u32)(*mctp_get_pkt_ptr); i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert[i - 15];
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15 + (alt_u32)(*mctp_get_pkt_ptr); i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 239 - 96; i++)
    {
        EXPECT_EQ(mctp_pkt_challenge_auth[i], IORD(m_mctp_memory, 0));
    }

    // Read out the signature
    for (alt_u32 i = 0; i < 96; i++)
    {
        IORD(m_mctp_memory, 0);
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 202 - 96; i++)
    {
        EXPECT_EQ(measurement_pkt[i], IORD(m_mctp_memory, 0));
    }

    //alt_u8 sig_read[96] = {0};
    // Read out the signature
    for (alt_u32 i = 0; i < 96; i++)
    {
        IORD(m_mctp_memory, 0);
    }

////////////////////////////////////////////////////////////////////////////////////////////////////
    /*
    alt_u8 get_cap_mctp_pkt[11] = MCTP_SPDM_GET_CAPABILITY_MSG;
    get_cap_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    get_cap_mctp_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    alt_u8 mctp_pkt_nego_algo[55] = MCTP_SPDM_NEGOTIATE_ALGORITHM;
    mctp_pkt_nego_algo[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_pkt_nego_algo[UT_MCTP_BYTE_COUNT_IDX] = 38;
    mctp_pkt_nego_algo[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);
    mctp_pkt_nego_algo[UT_MCTP_SPDM_ALGORITHM_LENGTH] = 0;
    alt_u16* length_ptr = (alt_u16*)&mctp_pkt_nego_algo[UT_MCTP_SPDM_ALGORITHM_LENGTH];
    *length_ptr = sizeof(NEGOTIATE_ALGORITHMS);
    mctp_pkt_nego_algo[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_SPEC] = SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF;
    alt_u32* nego_algo_ptr = (alt_u32*)&mctp_pkt_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_ASYM_ALGO];
    *nego_algo_ptr = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;
    nego_algo_ptr = (alt_u32*)&mctp_pkt_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_HASH_ALGO];
    *nego_algo_ptr = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;


    alt_u8 mctp_get_digest_pkt[11] = MCTP_SPDM_GET_DIGEST_MSG;
    mctp_get_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    mctp_get_digest_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    alt_u8 mctp_cert_packet_with_cert_chain[143] = {0};

    alt_u8 mctp_get_cert_pkt[15] = MCTP_SPDM_GET_CERTIFICATE_MSG;
    mctp_get_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0e;
    mctp_get_cert_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);
    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;

    alt_u16* mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 128;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0;

    alt_u8 mctp_cert_pkt[15] = MCTP_SPDM_CERTIFICATE_MSG;
    mctp_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 142;
    mctp_cert_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK] = 0;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK] = 0;
    alt_u16* mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 128;
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = FULL_CERT_LENGTH - 128;

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    alt_u8* cert = (alt_u8*) get_ufm_cpld_cert_ptr();
    for (alt_u8 i = 15; i < 15 + 128; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert[i - 15];
    }
    // Begin Test
    // To simulate the actual scenario in a T0 loop when CPLD (as a responder)
    // checks for any incoming requests from hosts, the unit test system will
    // inject beforehand all the necessary messages. In other words, the unit test
    // is required to staged all the necessary messages.

    // Once the firmware is done with one message, the firmware will indicate the
    // unit test system mock to prepare for the next staged message.
    // This is designed in this way so that the function can be called only once
    // for the entire message services.

    // A host starts a transaction by sending a Get_version message to CPLD
    // To stage the next message, set the last parameter of the function below
    // Staging the message in the unit test means the mock system will be waiting for
    // the completion of the processing of the first message before sending in the second message.
    ut_send_in_bmc_mctp_packet(m_memory, get_ver_mctp_pkt, 11, 0, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Stage this message
    ut_send_in_bmc_mctp_packet(m_memory, get_ver_mctp_pkt_2, 11, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    // Stage this message
    ut_send_in_bmc_mctp_packet(m_memory, get_cap_mctp_pkt, 11, 1, 0);
    //SYSTEM_MOCK::get()->stage_next_spdm_message();
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    // Stage this message
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt_nego_algo, 39, 1, 0);
    //SYSTEM_MOCK::get()->stage_next_spdm_message();
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    // If this is the last message, unset the final parameter of the functions
    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_digest_pkt, 11, 1, 0);
    //SYSTEM_MOCK::get()->stage_next_spdm_message();
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 1, 0);
    //SYSTEM_MOCK::get()->stage_next_spdm_message();
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    // Obtain the example packet
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    succ_ver_mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 14;
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX] = 0x01;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX + 1] = 0;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX + 2] = 0x10;

    alt_u8 succ_cap_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_CAPABILITY_MSG;
    succ_cap_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    succ_cap_mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 18;
    succ_cap_mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    succ_cap_mctp_pkt[12] = 1;
    alt_u32* cap_ptr = (alt_u32*)&succ_cap_mctp_pkt[15];
    *cap_ptr = SUPPORTS_DIGEST_CERT_RESPONSE_MSG_MASK |
               SUPPORTS_CHALLENGE_AUTH_RESPONSE_MSG_MASK |
               SUPPORTS_MEAS_RESPONSE_WITH_SIG_GEN_MSG_MASK;

    alt_u8 mctp_pkt[51] = MCTP_SPDM_SUCCESSFUL_ALGORITHM;
    mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 42;
    mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    mctp_pkt[UT_MCTP_SPDM_ALGORITHM_LENGTH] = 0;
    length_ptr = (alt_u16*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_LENGTH];
    *length_ptr = sizeof(SUCCESSFUL_ALGORITHMS);
    mctp_pkt[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_SPEC] = SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF;
    alt_u32* succ_algo_ptr = (alt_u32*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_HASH_ALGO];
    *succ_algo_ptr = MEAS_HASH_ALGO_TPM_ALG_SHA_384;
    succ_algo_ptr = (alt_u32*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_BASE_ASYM_SEL];
    *succ_algo_ptr = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
    succ_algo_ptr = (alt_u32*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_BASE_HASH_SEL];
    *succ_algo_ptr = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;

    mctp_pkt[UT_MCTP_SPDM_ALGORITHM_EXT_ASYM_SEL_COUNT] = 0;
    mctp_pkt[UT_MCTP_SPDM_ALGORITHM_EXT_HASH_SEL_COUNT] = 0;

    alt_u8 mctp_digest_pkt[59] = MCTP_SPDM_DIGEST_MSG;
    mctp_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 58;
    mctp_digest_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    mctp_digest_pkt[UT_MCTP_SPDM_DIGEST_SLOT_MASK] = 0x1;

    alt_u8 td_expected_hash[SHA384_LENGTH] = {0};
    alt_u8* cptr = (alt_u8*)get_ufm_cpld_cert_ptr();
    CERT_CHAIN_FORMAT* cert_chain = (CERT_CHAIN_FORMAT*)cptr;

    alt_u32 cert_length = cert_chain->length;
    calculate_and_save_sha((alt_u32*)td_expected_hash, 0, get_ufm_cpld_cert_ptr(), cert_length,
    		CRYPTO_384_MODE, DISENGAGE_DMA);

    for (alt_u8 i = 0; i < SHA384_LENGTH; i++)
    {
        mctp_digest_pkt[UT_MCTP_SPDM_DIGEST_SLOT_MASK + 1 + i] = td_expected_hash[i];
    }

    alt_u8 mctp_pkt_challenge[43] = MCTP_SPDM_CHALLENGE_MSG;
    mctp_pkt_challenge[UT_CHALLENGE_SPDM_SLOT_NUMBER_IDX] = 0;
    mctp_pkt_challenge[UT_CHALLENGE_SPDM_MEAS_SUMMARY_HASH_TYPE_IDX] = 0xff;
    mctp_pkt_challenge[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_pkt_challenge[UT_MCTP_BYTE_COUNT_IDX] = 42;
    mctp_pkt_challenge[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    alt_u8 mctp_pkt_challenge_auth[239] = MCTP_SPDM_CHALLENGE_AUTH_MSG;
    mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_SLOT_NUMBER_IDX] = 0x0;
    mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_SLOT_MASK_IDX] = 0x01;

    alt_u16* mctp_pkt_challenge_auth_ptr = (alt_u16*)&mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_OPAQUE_LENGTH_IDX];
    *mctp_pkt_challenge_auth_ptr = 0x02;
    mctp_pkt_challenge_auth_ptr = (alt_u16*)&mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_OPAQUE_LENGTH_IDX + 2];
    *mctp_pkt_challenge_auth_ptr = 0xCAFE;

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

    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt_challenge, 43, 1, 0);
    //SYSTEM_MOCK::get()->stage_next_spdm_message();
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    alt_u8 get_measurement_pkt[43] = MCTP_SPDM_GET_MEASUREMENT_MSG;
    get_measurement_pkt[UT_GET_MEASUREMENT_REQUEST_ATTRIBUTE] = SPDM_GET_MEASUREMENT_REQUEST_GENERATE_SIG;
    get_measurement_pkt[UT_GET_MEASUREMENT_OPERATION_MEASUREMENT_NUMBER] = 0x01;
    get_measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    get_measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 42;
    get_measurement_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

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
    for (alt_u32 i = 0; i < 48; i++)
    {
        measurement_pkt[UT_MEASUREMENT_RECORD + 7 + i] = 0x11;
    }*//*
    for (alt_u32 i = 0; i < 12; i++)
    {
        meas_pkt[i] = cfm1_hash[i];;
    }

    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH - 16] = UT_OPAQUE_LENGTH;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 1 - 16] = 0x0;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 2 - 16] = 0xfe;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 3 - 16] = 0xca;
    measurement_pkt[UT_MEASUREMENT_SIGNATURE - 16] = 0;
    measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 201;
    measurement_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    ut_send_in_bmc_mctp_packet(m_memory, get_measurement_pkt, 43, 0, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x03);

    cpld_checks_request_from_host(BMC);

    // Expect no spdm protocol error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), (alt_u32) 0);

    // This is the expected message sent back to the hosts
    // The message is sent in byte by byte format
    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(succ_ver_mctp_pkt[i], IORD(m_mctp_memory, 0));
    }

    // This is the expected message sent back to the hosts
    // The message is sent in byte by byte format
    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 19; i++)
    {
        EXPECT_EQ(succ_cap_mctp_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(mctp_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 59; i++)
    {
        EXPECT_EQ(mctp_digest_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 143; i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 239 - 96; i++)
    {
        EXPECT_EQ(mctp_pkt_challenge_auth[i], IORD(m_mctp_memory, 0));
    }

    // Read out the signature
    for (alt_u32 i = 0; i < 96; i++)
    {
        IORD(m_mctp_memory, 0);
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 202 - 96; i++)
    {
        EXPECT_EQ(measurement_pkt[i], IORD(m_mctp_memory, 0));
    }

    // Read out the signature
    for (alt_u32 i = 0; i < 96; i++)
    {
        IORD(m_mctp_memory, 0);
    }*/
}

