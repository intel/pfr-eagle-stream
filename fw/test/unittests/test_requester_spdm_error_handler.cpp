#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class PFRRequesterSpdmErrorHandlerTest : public testing::Test
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

TEST_F(PFRRequesterSpdmErrorHandlerTest, DISABLED_test_get_version_delay)
{
    /**
     *
     * Test preparation
     *
     */
    // Obtain the example packet
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    // Set last param to a high value to simulate a delay of packet arriving
    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 0, 40);

    // Initialise SPDM context
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;
    // Reset context
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);
    spdm_context_ptr->spdm_msg_state = SPDM_VERSION_FLAG;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    // Set timeout to be 20 ms (1 unit equals 20 ms)
    spdm_context_ptr->timeout = 1;

    // Expect error due to timeout
    EXPECT_EQ(requester_cpld_get_version(spdm_context_ptr, mctp_context_ptr), (alt_u32)SPDM_RESPONSE_ERROR_TIMEOUT);

    alt_u8 get_ver_mctp_pkt[11] = MCTP_SPDM_GET_VERSION_MSG;
    // Modify the SOM/EOM
    get_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    get_ver_mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    get_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    // Expect no reply from cpld as this is due to timeout error rather than busy or respond not ready error
    //EXPECT_EQ((alt_u32)0, IORD(m_mctp_memory, 0));

    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));
    init_spdm_context(spdm_context_ptr);

    // Set timeout to be 100 ms (1 unit equals 20 ms)
    // By waiting a little longer, loosen the margin for failure
    spdm_context_ptr->timeout = 10;
    // Reset state back to first message
    spdm_context_ptr->spdm_msg_state = SPDM_VERSION_FLAG;

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    // Set last param to a high value to simulate a delay of packet arriving
    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 0, 40);

    // Expect error due to timeout
    EXPECT_EQ(requester_cpld_get_version(spdm_context_ptr, mctp_context_ptr), (alt_u32)SPDM_RESPONSE_SUCCESS);

    // Expect no spdm protocol error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), (alt_u32) 0);

    // This is the expected message sent back to the hosts
    // The message is sent in byte by byte format
    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(get_ver_mctp_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(get_ver_mctp_pkt[i], IORD(m_mctp_memory, 0));
    }
}

TEST_F(PFRRequesterSpdmErrorHandlerTest, test_responder_reply_error_response_busy)
{
    SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
    // Reset system mocks and SPI flash
    sys->reset();
    sys->reset_spi_flash_mock();

    // Perform provisioning
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);

    // Reset Nios firmware
    ut_reset_nios_fw();

    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // Load some capsules to flash
    // If there're unexpected updates, unittest can catch them.
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    CRYPTO_MODE sha_mode = CRYPTO_384_MODE;
    alt_u32 sha_size = SHA384_LENGTH;

    LARGE_APPENDED_BUFFER big_buffer;
    LARGE_APPENDED_BUFFER* big_buffer_ptr = &big_buffer;
    reset_buffer((alt_u8*)big_buffer_ptr, sizeof(LARGE_APPENDED_BUFFER));

    alt_u8 priv_key_0[48] = {
        0x6d, 0x21, 0x7e, 0x55, 0x18, 0x5b, 0xfe, 0xa5, 0x54, 0xf7, 0x44, 0x62,
        0x89, 0xb2, 0x40, 0xb1, 0x1d, 0x56, 0xe6, 0x61, 0xf1, 0x16, 0x4f, 0x49,
        0x6f, 0xd9, 0x42, 0x8d, 0xbb, 0xeb, 0x4b, 0xf3, 0x9d, 0x0a, 0x7c, 0x40,
        0xd2, 0xdf, 0x39, 0x12, 0x89, 0xc9, 0x11, 0xb5, 0xa9, 0xaa, 0xef, 0x2b,
    };

    alt_u8 priv_key_1[48] = {
        0xb0, 0x21, 0x7e, 0x55, 0x18, 0x5b, 0x0e, 0xa5, 0x54, 0xf7, 0x44, 0x62,
        0x89, 0xb2, 0x40, 0xb1, 0x1d, 0x56, 0xe6, 0x61, 0xf1, 0x16, 0x4f, 0x49,
        0x6f, 0xd9, 0x42, 0x8d, 0x90, 0xeb, 0x4b, 0xf3, 0x9d, 0x0a, 0x7c, 0x40,
        0xd2, 0xdf, 0x39, 0x12, 0x89, 0xc9, 0x11, 0xb5, 0xa9, 0xd2, 0xef, 0x2b,
    };

    alt_u32 test_pubkey_cx_cy[SHA384_LENGTH/2] = {0};

    // Manually generate the public key here
    generate_pubkey(test_pubkey_cx_cy, test_pubkey_cx_cy + SHA384_LENGTH/4, (alt_u32*)priv_key_0, CRYPTO_384_MODE);

    alt_u32_memcpy(get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_0), test_pubkey_cx_cy, SHA384_LENGTH * 2);

    // Manually generate the public key here
    generate_pubkey(test_pubkey_cx_cy, test_pubkey_cx_cy + SHA384_LENGTH/4, (alt_u32*)priv_key_1, CRYPTO_384_MODE);

    alt_u32_memcpy(get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_1), test_pubkey_cx_cy, SHA384_LENGTH * 2);

    // Cert chain obtained from OPENSPDM responder
	alt_u32 cert_chain_buffer[MAX_CERT_CHAIN_SIZE/4] = {0};

    // Temporarily store the certificate
    alt_u32 cert_res[MAX_CERT_CHAIN_SIZE/4] = {0};
    alt_u32* stored_cert_ptr = (alt_u32*)cert_res;

    alt_u32 res_pub_key[100];

    alt_u32 cert_chain_size = pfr_generate_certificate_chain((alt_u8*) cert_chain_buffer, (alt_u32*) priv_key_0, get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_0), get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_1));
    alt_u32 cert_chain_root_cert_size = pfr_get_certificate_size((alt_u8*) cert_chain_buffer);

    EXPECT_EQ(1, pfr_verify_der_cert_chain((alt_u8*)cert_chain_buffer, cert_chain_size, res_pub_key, (alt_u32*)res_pub_key + 12));

    //build and store first 4 bytes of cert header into UFM
    alt_u32 cert_chain_header_first_word = 0x00000000;
    alt_u16* cert_chain_header_first_word_u16 = (alt_u16*) &cert_chain_header_first_word;
    cert_chain_header_first_word_u16[0] = (alt_u16) (4 + (12*4) + cert_chain_size);
    stored_cert_ptr[0] = cert_chain_header_first_word;
    //calculate and store root cert hash into UFM
    calculate_and_save_sha(&stored_cert_ptr[1], 0x00000000, cert_chain_buffer,
    		cert_chain_root_cert_size, CRYPTO_384_MODE, DISENGAGE_DMA);

    alt_u32 cert_chain_header_size = 52;

    alt_u32 cert_chain_size_padded = cert_chain_size;
    while (cert_chain_size_padded % 4 != 0) {
    	cert_chain_size_padded++;
    }
    alt_u32_memcpy(&stored_cert_ptr[1 + 12], cert_chain_buffer, cert_chain_size_padded);

    alt_u8 cert_chain_digest[SHA384_LENGTH] = {0};

    calculate_and_save_sha((alt_u32*)cert_chain_digest, 0, (alt_u32*)cert_res,
    		          cert_chain_size + cert_chain_header_size, CRYPTO_384_MODE, DISENGAGE_DMA);

    alt_u8 expected_get_ver_msg[11] = MCTP_SPDM_GET_VERSION_MSG;
    expected_get_ver_msg[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    expected_get_ver_msg[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    expected_get_ver_msg[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;

    // Get the error response code
    alt_u8 mctp_error_pkt[11] = MCTP_SPDM_ERROR_RESPONSE_MSG;
    mctp_error_pkt[UT_MCTP_SPDM_ERROR_RESPONSE_ERROR_CODE] = SPDM_ERROR_CODE_BUSY;
    mctp_error_pkt[UT_MCTP_SPDM_ERROR_RESPONSE_ERROR_DATA] = 0x00;
    mctp_error_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    mctp_error_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_error_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_error_pkt, 11, 1, 0);

    //append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_get_ver_msg[7], 4);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_error_pkt, 11, 1, 0);

    //append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_get_ver_msg[7], 4);

    // Obtain the example message for succesful_version
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = 0x02 << 1;

    append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_get_ver_msg[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_ver_mctp_pkt[7], 12);

    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    alt_u8 get_cap_mctp_pkt[11] = MCTP_SPDM_GET_CAPABILITY_MSG;
    get_cap_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    get_cap_mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    alt_u8 succ_cap_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_CAPABILITY_MSG;
    succ_cap_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
    succ_cap_mctp_pkt[UT_MCTP_SOURCE_ADDR] = 0x02 << 1;
    // Modify ct exponent
    succ_cap_mctp_pkt[UT_MCTP_SPDM_CAPABILITY_CT_EXPONENT_IDX] = 0x0c;
    // Modify flags
    alt_u32* cap_ptr = (alt_u32*)&succ_cap_mctp_pkt[UT_MCTP_SPDM_CAPABILITY_FLAGS_IDX];
    *cap_ptr = SUPPORTS_DIGEST_CERT_RESPONSE_MSG_MASK |
               SUPPORTS_CHALLENGE_AUTH_RESPONSE_MSG_MASK |
               SUPPORTS_MEAS_RESPONSE_WITH_SIG_GEN_MSG_MASK;

    append_large_buffer(big_buffer_ptr, (alt_u8*)&get_cap_mctp_pkt[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_cap_mctp_pkt[7], 12);

    // If this is the last message, unset the final parameter of the functions
    ut_send_in_bmc_mctp_packet(m_memory, succ_cap_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    alt_u8 expected_nego_algo[55] = MCTP_SPDM_NEGOTIATE_ALGORITHM;
    expected_nego_algo[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    expected_nego_algo[UT_MCTP_BYTE_COUNT_IDX] = 38;
    expected_nego_algo[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    expected_nego_algo[UT_MCTP_SPDM_ALGORITHM_LENGTH] = 0;
    alt_u16* length_ptr = (alt_u16*)&expected_nego_algo[UT_MCTP_SPDM_ALGORITHM_LENGTH];
    *length_ptr = sizeof(NEGOTIATE_ALGORITHMS);
    expected_nego_algo[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_SPEC] = SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF;
    alt_u32* nego_algo_ptr = (alt_u32*)&expected_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_ASYM_ALGO];
    *nego_algo_ptr = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;
    nego_algo_ptr = (alt_u32*)&expected_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_HASH_ALGO];
    *nego_algo_ptr = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;

    alt_u8 mctp_pkt[51] = MCTP_SPDM_SUCCESSFUL_ALGORITHM;
    mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 50;
    mctp_pkt[UT_MCTP_SOURCE_ADDR] = 0x02 << 1;
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

    append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_nego_algo[7], sizeof(NEGOTIATE_ALGORITHMS));
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt[7], 44);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt, 51, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    alt_u8 mctp_get_digest_pkt[11] = MCTP_SPDM_GET_DIGEST_MSG;
    mctp_get_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    mctp_get_digest_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    alt_u8 mctp_digest_pkt[59] = MCTP_SPDM_DIGEST_MSG;
    mctp_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 58;
    mctp_digest_pkt[UT_MCTP_SOURCE_ADDR] = 0x02 << 1;
    mctp_digest_pkt[UT_MCTP_SPDM_DIGEST_SLOT_MASK] = 0x0;

    for (alt_u8 i = 11; i < 59; i++)
    {
    	mctp_digest_pkt[i] = cert_chain_digest[i - 11];
    }

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_digest_pkt[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_digest_pkt[7], 52);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_digest_pkt, 59, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    cert_chain_size += cert_chain_header_size;
    alt_u8 mctp_cert_packet_with_cert_chain[500] = {0};

    alt_u8 mctp_get_cert_pkt[15] = MCTP_SPDM_GET_CERTIFICATE_MSG;
    mctp_get_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0e;
    mctp_get_cert_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    alt_u16* mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = CERT_REQUEST_SIZE;
    alt_u16* mctp_get_cert_pkt_ptr_2 = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr_2 = 0;

    alt_u8 mctp_cert_pkt[15] = MCTP_SPDM_CERTIFICATE_MSG;
    mctp_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 206 + 8;
    mctp_cert_pkt[UT_MCTP_SOURCE_ADDR] = (0x02 << 1);
    alt_u16* mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 0xc8;
    alt_u16* mctp_get_pkt_ptr_2 = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr_2 = cert_chain_size - 0xc8;

    alt_u8* cert_chain_buffer_ptr = (alt_u8*)cert_res;

    for (alt_u32 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    for (alt_u32 i = 15; i < 15 + 200; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert_chain_buffer_ptr[i - 15];
    }

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 207 + 8, 1, 0);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_cert_pkt[7], 8);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[7], 208);

    // Set remainder
    *mctp_get_pkt_ptr_2 = cert_chain_size - 0xc8 - 0xc8;

    for (alt_u32 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    for (alt_u32 i = 15; i < 15 + 200; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert_chain_buffer_ptr[i - 15 + 200];
    }

    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 207 + 8, 1, 0);

    // Expect that CPLD request the remaining portion of the certificate
    *mctp_get_cert_pkt_ptr = 0xc8;
    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 0xc8;

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_cert_pkt[7], 8);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[7], 208);

    // Set remainder
    *mctp_get_pkt_ptr_2 = cert_chain_size - 0xc8 - 0xc8 - 0xc8;

    for (alt_u32 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    for (alt_u32 i = 15; i < 15 + 200; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert_chain_buffer_ptr[i - 15 + 200 + 200];
    }

    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 207 + 8, 1, 0);

    // Expect that CPLD request the remaining portion of the certificate
    *mctp_get_cert_pkt_ptr = 0xc8;
    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 0xc8 + 0xc8;

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_cert_pkt[7], 8);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[7], 208);

    // Set remainder
    *mctp_get_pkt_ptr_2 = cert_chain_size - 0xc8 - 0xc8 - 0xc8 - 0xc8;

    for (alt_u32 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    for (alt_u32 i = 15; i < 15 + 200; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert_chain_buffer_ptr[i - 15 + 200 + 200 + 200];
    }

    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 207 + 8, 1, 0);

    // Expect that CPLD request the remaining portion of the certificate
    *mctp_get_cert_pkt_ptr = 0xc8;
    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 0xc8 + 0xc8 + 0xc8;

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_cert_pkt[7], 8);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[7], 208);

    // Set remainder
    *mctp_get_pkt_ptr_2 = cert_chain_size - 0xc8 - 0xc8 - 0xc8 - 0xc8 - 0xc8;

    for (alt_u32 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    for (alt_u32 i = 15; i < 15 + 200; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert_chain_buffer_ptr[i - 15 + 200 + 200 + 200 + 200];
    }

    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 207 + 8, 1, 0);

    // Expect that CPLD request the remaining portion of the certificate
    *mctp_get_cert_pkt_ptr = 0xc8;
    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 0xc8 + 0xc8 + 0xc8 + 0xc8;

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_cert_pkt[7], 8);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[7], 208);

    // Set remainder
    *mctp_get_pkt_ptr_2 = cert_chain_size - 0xc8 - 0xc8 - 0xc8 - 0xc8 - 0xc8 - 0xc8;
    *mctp_get_pkt_ptr = 0xc8;

    for (alt_u32 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    for (alt_u32 i = 15; i < 15 + 200; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert_chain_buffer_ptr[i - 15 + 200 + 200 + 200 + 200 + 200];
    }

    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 207 + 8, 1, 0);

    // Expect that CPLD request the remaining portion of the certificate
    *mctp_get_cert_pkt_ptr = 0xc8;
    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 0xc8 + 0xc8 + 0xc8 + 0xc8 + 0xc8;

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_cert_pkt[7], 8);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[7], 208);

    // Set remainder
    *mctp_get_pkt_ptr_2 = 0;
    *mctp_get_pkt_ptr = cert_chain_size - 0xc8 - 0xc8 - 0xc8 - 0xc8 - 0xc8 - 0xc8;
    mctp_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 206 + 8 - 0xc8 + (*mctp_get_pkt_ptr);

    for (alt_u32 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    for (alt_u32 i = 15; i < 15 + (alt_u32)(*mctp_get_pkt_ptr); i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert_chain_buffer_ptr[i - 15 + 200 + 200 + 200 + 200 + 200 + 200];
    }

    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 207 + 8 - 0xc8 + (*mctp_get_pkt_ptr), 1, 0);

    // Expect that CPLD request the remaining portion of the certificate
    *mctp_get_cert_pkt_ptr = cert_chain_size - 0xc8 - 0xc8 - 0xc8 - 0xc8 - 0xc8 - 0xc8;
    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 0xc8 + 0xc8 + 0xc8 + 0xc8 + 0xc8 + 0xc8;

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_cert_pkt[7], 8);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[7], 208 - 0xc8 + (*mctp_get_pkt_ptr));

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
    mctp_pkt_challenge_auth[UT_MCTP_SOURCE_ADDR] = 0x02 << 1;

    alt_u8 meas_hash[SHA384_LENGTH] = {
    0x8d, 0x72, 0xb6, 0x20, 0xe1, 0x59, 0xb8, 0x52, 0xa3, 0x89, 0x31, 0x0e,
    0x03, 0x7c, 0x7e, 0x15, 0x93, 0x69, 0x87, 0xc4, 0xab, 0x82, 0x43, 0xfb,
    0xb3, 0xbb, 0x4b, 0xfd, 0x8b, 0x5a, 0xa7, 0x8d, 0x4b, 0xa2, 0xca, 0x36,
    0x3c, 0xe7, 0x29, 0x9c, 0x61, 0x0e, 0xb0, 0x2a, 0x75, 0x4e, 0x5a, 0x65
    };

    alt_u32* ptr = (alt_u32*) cert_chain_digest;
    alt_u32* pkt_ptr = (alt_u32*)&mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_CERT_CHAIN_HASH_IDX];

    for (alt_u32 i = 0; i < SHA384_LENGTH/4; i++)
    {
        pkt_ptr[i] = ptr[i];
    }

    // Have to manually append CHALLENGE/CHALLENGE_AUTH message here to generate the right hash and signature
    // To clear this before calling challenge function
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt_challenge[7], 36);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt_challenge_auth[7], (232 - 96));

    alt_u8 test_pubkey_cx[SHA384_LENGTH] = {0};
    alt_u8 test_pubkey_cy[SHA384_LENGTH] = {0};
    alt_u8 sig_r[SHA384_LENGTH] = {0};
    alt_u8 sig_s[SHA384_LENGTH] = {0};

    alt_u8* msg_to_hash = (alt_u8*)big_buffer_ptr->buffer;

    // Manually generate the public key here
    generate_pubkey((alt_u32*)test_pubkey_cx, (alt_u32*)test_pubkey_cy, (alt_u32*)priv_key_1, sha_mode);

    // Manually generate signature
    generate_ecdsa_signature((alt_u32*)sig_r, (alt_u32*)sig_s, (alt_u32*)msg_to_hash, big_buffer_ptr->buffer_size,
    	               (alt_u32*)test_pubkey_cx, (alt_u32*)priv_key_1, sha_mode);

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

    // Send in the packet
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt_challenge_auth, 239, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    alt_u8 get_measurement_pkt[43] = MCTP_SPDM_GET_MEASUREMENT_MSG;
    get_measurement_pkt[UT_GET_MEASUREMENT_REQUEST_ATTRIBUTE] = SPDM_GET_MEASUREMENT_REQUEST_GENERATE_SIG;
    get_measurement_pkt[UT_GET_MEASUREMENT_OPERATION_MEASUREMENT_NUMBER] = 0x01;
    get_measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    get_measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 42;
    get_measurement_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    alt_u8 get_measurement_pkt_2[43] = MCTP_SPDM_GET_MEASUREMENT_MSG;
    get_measurement_pkt_2[UT_GET_MEASUREMENT_REQUEST_ATTRIBUTE] = SPDM_GET_MEASUREMENT_REQUEST_GENERATE_SIG;
    get_measurement_pkt_2[UT_GET_MEASUREMENT_OPERATION_MEASUREMENT_NUMBER] = 0x02;
    get_measurement_pkt_2[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    get_measurement_pkt_2[UT_MCTP_BYTE_COUNT_IDX] = 42;
    get_measurement_pkt_2[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

     alt_u8 measurement_pkt[218] = MCTP_SPDM_MEASUREMENT_MSG;
     // Shall test for 1 block of measurement
     measurement_pkt[UT_MEASUREMENT_TOTAL_NUMBER_OF_BLOCKS] = 1;
     measurement_pkt[UT_MEASUREMENT_RECORD_LENGTH] = UT_RECORD_LENGTH;
     measurement_pkt[UT_MEASUREMENT_RECORD_LENGTH + 1] = 0x00;
     measurement_pkt[UT_MEASUREMENT_RECORD_LENGTH + 2] = 0x00;
     measurement_pkt[UT_MEASUREMENT_RECORD] = 1;
     measurement_pkt[UT_MEASUREMENT_RECORD + 1] = 1;
     measurement_pkt[UT_MEASUREMENT_RECORD + 2] = UT_RECORD_LENGTH;
     measurement_pkt[UT_MEASUREMENT_RECORD + 3] = 0x00;
     measurement_pkt[UT_MEASUREMENT_RECORD + 4] = 0x01;
     measurement_pkt[UT_MEASUREMENT_RECORD + 5] = 64;
     measurement_pkt[UT_MEASUREMENT_RECORD + 6] = 0x00;

     // For simplicity, just randomly fill in the measurement hash
     // Do the same for AFM in the ufm after this
     for (alt_u32 i = 0; i < 64; i++)
     {
         if (i > 47)
         {
             measurement_pkt[UT_MEASUREMENT_RECORD + 7 + i] = 0;
         }
         else
         {
         	 measurement_pkt[UT_MEASUREMENT_RECORD + 7 + i] = meas_hash[48 - 1 - i];
         }
     }

     measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH] = UT_OPAQUE_LENGTH;
     measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 1] = 0x0;
     measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 2] = 0xFE;
     measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 3] = 0xCA;
     measurement_pkt[UT_MEASUREMENT_SIGNATURE] = 0;
     measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
     measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 217;
     measurement_pkt[UT_MCTP_SOURCE_ADDR] = 0x02 << 1;

     alt_u32* meas_pkt_ptr = (alt_u32*)&measurement_pkt[UT_MEASUREMENT_SIGNATURE];

     LARGE_APPENDED_BUFFER big_buffer_2;
     LARGE_APPENDED_BUFFER* big_buffer_2_ptr = &big_buffer_2;
     reset_buffer((alt_u8*)big_buffer_2_ptr, sizeof(LARGE_APPENDED_BUFFER));

     append_large_buffer(big_buffer_2_ptr, (alt_u8*)&get_measurement_pkt[7], 36);
     append_large_buffer(big_buffer_2_ptr, (alt_u8*)&measurement_pkt[7], (211 - 96));

     msg_to_hash = (alt_u8*)big_buffer_2_ptr->buffer;

     // Manually generate signature
     generate_ecdsa_signature((alt_u32*)sig_r, (alt_u32*)sig_s, (alt_u32*)msg_to_hash, big_buffer_2_ptr->buffer_size,
     	               (alt_u32*)test_pubkey_cx, (alt_u32*)priv_key_1, sha_mode);

     ptr = (alt_u32*) sig_r;
     for (alt_u32 i = 0; i < (sha_size >> 2); i++)
     {
    	 meas_pkt_ptr[i] = ptr[i];
     }

     meas_pkt_ptr += (SHA384_LENGTH >> 2);
     ptr = (alt_u32*) sig_s;
     for (alt_u32 i = 0; i < (sha_size >> 2); i++)
     {
    	 meas_pkt_ptr[i] = ptr[i];
     }

     ut_send_in_bmc_mctp_packet(m_memory, measurement_pkt, 218, 1, 0);
     EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

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

    reset_buffer((alt_u8*)big_buffer_2_ptr, sizeof(LARGE_APPENDED_BUFFER));

    measurement_pkt[UT_MEASUREMENT_RECORD] = 2;

    for (alt_u32 i = 0; i < 64; i++)
    {
        measurement_pkt[UT_MEASUREMENT_RECORD + 7 + i] = 0x3a;
    }

    append_large_buffer(big_buffer_2_ptr, (alt_u8*)&get_measurement_pkt_2[7], 36);
    append_large_buffer(big_buffer_2_ptr, (alt_u8*)&measurement_pkt[7], (211 - 96));

    msg_to_hash = (alt_u8*)big_buffer_2_ptr->buffer;

    // Manually generate signature
    generate_ecdsa_signature((alt_u32*)sig_r, (alt_u32*)sig_s, (alt_u32*)msg_to_hash, big_buffer_2_ptr->buffer_size,
    	               (alt_u32*)test_pubkey_cx, (alt_u32*)priv_key_1, sha_mode);

    meas_pkt_ptr = (alt_u32*)&measurement_pkt[UT_MEASUREMENT_SIGNATURE];

    ptr = (alt_u32*) sig_r;
    for (alt_u32 i = 0; i < (sha_size >> 2); i++)
    {
    	meas_pkt_ptr[i] = ptr[i];
    }

    meas_pkt_ptr += (SHA384_LENGTH >> 2);
    ptr = (alt_u32*) sig_s;
    for (alt_u32 i = 0; i < (sha_size >> 2); i++)
    {
    	meas_pkt_ptr[i] = ptr[i];
    }

    // Send in the packet
    ut_send_in_bmc_mctp_packet(m_memory, measurement_pkt, 218, 0, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);
    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(250, pfr_main());

    EXPECT_EQ(read_from_mailbox(MB_PROVISION_STATUS_2), (alt_u32)MB_UFM_PROV_ATTESTATION_ENABLED);
    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for authentication
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Expect no spdm protocol error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), (alt_u32) 0);
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), (alt_u32) 0);

    // This is the expected message sent back to the hosts
    // The message is sent in byte by byte format
    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(expected_get_ver_msg[i], IORD(m_mctp_memory, 0));
    }

    // This is the expected message sent back to the hosts
    // The message is sent in byte by byte format
    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(expected_get_ver_msg[i], IORD(m_mctp_memory, 0));
    }

    // This is the expected message sent back to the hosts
    // The message is sent in byte by byte format
    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(expected_get_ver_msg[i], IORD(m_mctp_memory, 0));
    }

    // This is the expected message sent back to the hosts
    // The message is sent in byte by byte format
    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(get_cap_mctp_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 39; i++)
    {
        EXPECT_EQ(expected_nego_algo[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(mctp_get_digest_pkt[i], IORD(m_mctp_memory, 0));
    }

    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr = 0xc8;
    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 0;

    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(mctp_get_cert_pkt[i], IORD(m_mctp_memory, 0));
    }

    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr = 0xc8;
    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 0xc8;

    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(mctp_get_cert_pkt[i], IORD(m_mctp_memory, 0));
    }

    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr = 0xc8;
    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 0xc8 + 0xc8;

    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(mctp_get_cert_pkt[i], IORD(m_mctp_memory, 0));
    }

    // Expect that CPLD request the remaining portion of the certificate
    *mctp_get_cert_pkt_ptr = 0xc8;
    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 0xc8 + 0xc8 + 0xc8;

    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(mctp_get_cert_pkt[i], IORD(m_mctp_memory, 0));
    }

    // Expect that CPLD request the remaining portion of the certificate
    *mctp_get_cert_pkt_ptr = 0xc8;
    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 0xc8 + 0xc8 + 0xc8 + 0xc8;

    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(mctp_get_cert_pkt[i], IORD(m_mctp_memory, 0));
    }

    // Expect that CPLD request the remaining portion of the certificate
    *mctp_get_cert_pkt_ptr = 0xc8;
    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 0xc8 + 0xc8 + 0xc8 + 0xc8 + 0xc8;

    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(mctp_get_cert_pkt[i], IORD(m_mctp_memory, 0));
    }

    // Expect that CPLD request the remaining portion of the certificate
    *mctp_get_cert_pkt_ptr = cert_chain_size - 0xc8 - 0xc8 - 0xc8 - 0xc8 - 0xc8 - 0xc8;
    // Expect CPLD to obtain the offset from previous cert chain
    *mctp_get_cert_pkt_ptr_2 = 0xc8 + 0xc8 + 0xc8 + 0xc8 + 0xc8 + 0xc8;

    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(mctp_get_cert_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(mctp_pkt_challenge[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(get_measurement_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(0x02 << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(get_measurement_pkt_2[i], IORD(m_mctp_memory, 0));
    }
///////////////////////////

    /*
    alt_u8 expected_get_ver_msg[11] = MCTP_SPDM_GET_VERSION_MSG;
    expected_get_ver_msg[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    expected_get_ver_msg[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    expected_get_ver_msg[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;

    alt_u8 get_cap_mctp_pkt[11] = MCTP_SPDM_GET_CAPABILITY_MSG;
    get_cap_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    get_cap_mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    alt_u8 expected_nego_algo[55] = MCTP_SPDM_NEGOTIATE_ALGORITHM;
    expected_nego_algo[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    expected_nego_algo[UT_MCTP_BYTE_COUNT_IDX] = 38;
    expected_nego_algo[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    expected_nego_algo[UT_MCTP_SPDM_ALGORITHM_LENGTH] = 0;
    alt_u16* length_ptr = (alt_u16*)&expected_nego_algo[UT_MCTP_SPDM_ALGORITHM_LENGTH];
    *length_ptr = sizeof(NEGOTIATE_ALGORITHMS);
    expected_nego_algo[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_SPEC] = SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF;
    alt_u32* nego_algo_ptr = (alt_u32*)&expected_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_ASYM_ALGO];
    *nego_algo_ptr = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;
    nego_algo_ptr = (alt_u32*)&expected_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_HASH_ALGO];
    *nego_algo_ptr = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;

    // Obtain the example packet
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    alt_u8 succ_cap_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_CAPABILITY_MSG;
    succ_cap_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
    succ_cap_mctp_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);
    // Modify ct exponent
    succ_cap_mctp_pkt[UT_MCTP_SPDM_CAPABILITY_CT_EXPONENT_IDX] = 0x0c;
    // Modify flags
    alt_u32* cap_ptr = (alt_u32*)&succ_cap_mctp_pkt[UT_MCTP_SPDM_CAPABILITY_FLAGS_IDX];
    *cap_ptr = SUPPORTS_DIGEST_CERT_RESPONSE_MSG_MASK |
               SUPPORTS_CHALLENGE_AUTH_RESPONSE_MSG_MASK |
               SUPPORTS_MEAS_RESPONSE_WITH_SIG_GEN_MSG_MASK;

    alt_u8 mctp_pkt[51] = MCTP_SPDM_SUCCESSFUL_ALGORITHM;
    mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 50;
    mctp_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);
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

    alt_u8 mctp_get_digest_pkt[11] = MCTP_SPDM_GET_DIGEST_MSG;
    mctp_get_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    mctp_get_digest_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    alt_u8 mctp_digest_pkt[59] = MCTP_SPDM_DIGEST_MSG;
    mctp_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 58;
    mctp_digest_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);
    mctp_digest_pkt[UT_MCTP_SPDM_DIGEST_SLOT_MASK] = 0x0;

    const alt_u8 td_expected_hash[SHA384_LENGTH] = {
    	    0x8d, 0x72, 0xb6, 0x20, 0xe1, 0x59, 0xb8, 0x52, 0xa3, 0x89, 0x31, 0x0e,
    	    0x03, 0x7c, 0x7e, 0x15, 0x93, 0x69, 0x87, 0xc4, 0xab, 0x82, 0x43, 0xfb,
    	    0xb3, 0xbb, 0x4b, 0xfd, 0x8b, 0x5a, 0xa7, 0x8d, 0x4b, 0xa2, 0xca, 0x36,
    	    0x3c, 0xe7, 0x29, 0x9c, 0x61, 0x0e, 0xb0, 0x2a, 0x75, 0x4e, 0x5a, 0x65};

    for (alt_u8 i = 11; i < 59; i++)
    {
    	mctp_digest_pkt[i] = td_expected_hash[i - 11];
    }

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
    mctp_cert_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK] = 128;
    mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK] = 0;
    alt_u16* mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 128;

    const alt_u8 td_crypto_data[128] = {
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
      	  0x95, 0xd2, 0xf3, 0x3c, 0x8b, 0xb7, 0x60, 0xd7};

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    for (alt_u8 i = 15; i < 15 + 128; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = td_crypto_data[i - 15];
    }

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
    mctp_pkt_challenge_auth[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

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

    LARGE_APPENDED_BUFFER big_buffer;
    LARGE_APPENDED_BUFFER* big_buffer_ptr = &big_buffer;
    reset_buffer((alt_u8*)big_buffer_ptr, sizeof(LARGE_APPENDED_BUFFER));

    append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_get_ver_msg[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_ver_mctp_pkt[7], 12);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&get_cap_mctp_pkt[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_cap_mctp_pkt[7], 12);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_nego_algo[7], sizeof(NEGOTIATE_ALGORITHMS));
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt[7], 44);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_digest_pkt[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_digest_pkt[7], 52);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_cert_pkt[7], 8);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[7], 136);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt_challenge[7], 36);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt_challenge_auth[7], (232 - 96));
    // Have to manually append CHALLENGE/CHALLENGE_AUTH message here to generate the right hash and signature
    // To clear this before calling challenge function

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
    *((alt_u16*)ufm_afm_pointer) = 2;

    // Set the address to 0x02 (dummy value for BMC)
    ((AFM_BODY*) ufm_afm_pointer)->device_addr = 2;

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

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_error_pkt, 11, 1, 0);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_error_pkt, 11, 1, 0);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 1, 0);

    ut_send_in_bmc_mctp_packet(m_memory, succ_cap_mctp_pkt, 19, 1, 0);

    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt, 51, 1, 0);

    ut_send_in_bmc_mctp_packet(m_memory, mctp_digest_pkt, 59, 1, 0);

    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 143, 1, 0);

    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt_challenge_auth, 239, 0, 0);

    //EXPECT_EQ(request_message_handler(spdm_context_ptr, BMC), alt_u32(0));
    cpld_sends_request_to_host(BMC, 2);

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(expected_get_ver_msg[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(expected_get_ver_msg[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(expected_get_ver_msg[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(get_cap_mctp_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 39; i++)
    {
        EXPECT_EQ(expected_nego_algo[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(mctp_get_digest_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(mctp_get_cert_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(mctp_pkt_challenge[i], IORD(m_mctp_memory, 0));
    }*/
}

TEST_F(PFRRequesterSpdmErrorHandlerTest, test_responder_reply_error_respond_not_ready)
{
    // Get the error response code
    alt_u8 mctp_error_pkt[15] = MCTP_SPDM_ERROR_RESPONSE_EXTENDED_ERROR_MSG;
    mctp_error_pkt[UT_MCTP_SPDM_ERROR_RESPONSE_ERROR_CODE] = SPDM_ERROR_CODE_RESPONSE_NOT_READY;
    mctp_error_pkt[UT_MCTP_SPDM_ERROR_RESPONSE_ERROR_DATA] = 0x00;
    mctp_error_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0e;
    mctp_error_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_error_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);
    mctp_error_pkt[UT_MCTP_SPDM_ERROR_RESPONSE_EXTENDED_ERROR_RDT_EXPONENT] = 0x0a;
    mctp_error_pkt[UT_MCTP_SPDM_ERROR_RESPONSE_EXTENDED_ERROR_REQUEST_CODE] = REQUEST_VERSION;
    mctp_error_pkt[UT_MCTP_SPDM_ERROR_RESPONSE_EXTENDED_ERROR_TOKEN] = 0x01;
    mctp_error_pkt[UT_MCTP_SPDM_ERROR_RESPONSE_EXTENDED_ERROR_RDTM] = 0x28;

    // Get the error response code
    alt_u8 mctp_error_cpld_pkt[11] = MCTP_SPDM_RESPOND_IF_READY_MSG;
    mctp_error_cpld_pkt[UT_MCTP_SPDM_RESPOND_IF_READY_REQUEST_CODE] = REQUEST_VERSION;
    mctp_error_cpld_pkt[UT_MCTP_SPDM_RESPOND_IF_READY_TOKEN] = 0x01;
    mctp_error_cpld_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    mctp_error_cpld_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_error_cpld_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    alt_u8 expected_get_ver_msg[11] = MCTP_SPDM_GET_VERSION_MSG;
    expected_get_ver_msg[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    expected_get_ver_msg[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    expected_get_ver_msg[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;

    // Obtain the example packet
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    // Initialise SPDM context
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;
    // Reset context
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);
    spdm_context_ptr->spdm_msg_state = SPDM_VERSION_FLAG;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    // Set timeout to be 20 ms (1 unit equals 20 ms)
    // Set here the minimum time for CPLD to wait, but CPLD should be able to wait longer by
    // waiting WDTMax
    spdm_context_ptr->timeout = 1;

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_error_pkt, 15, 1, 0);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 0, 0);

    // Handling routine
    //
    EXPECT_EQ(request_message_handler(spdm_context_ptr, mctp_context_ptr), (alt_u32)0);

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(expected_get_ver_msg[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(mctp_error_cpld_pkt[i], IORD(m_mctp_memory, 0));
    }
}

TEST_F(PFRRequesterSpdmErrorHandlerTest, DISABLED_test_requester_cpld_get_version_timeout_retry)
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
    succ_ver_mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x12;
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = BMC_SLAVE_ADDRESS;

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 0, 0);

    EXPECT_EQ(requester_cpld_get_version(spdm_context_ptr, mctp_context_ptr), alt_u32(0));

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
