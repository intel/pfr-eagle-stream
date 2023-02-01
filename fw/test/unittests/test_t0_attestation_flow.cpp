#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "ut_checks.h"


class T0AttestationFlowTest : public testing::Test
{
public:

    alt_u32* m_memory = nullptr;
    alt_u32* m_mctp_memory = nullptr;
    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        // Reset system mocks and SPI flash
        sys->reset();
        sys->reset_spi_flash_mock();

        m_memory = U_MAILBOX_AVMM_BRIDGE_ADDR;
        m_mctp_memory = U_RFNVRAM_SMBUS_MASTER_ADDR;
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
    }

    virtual void TearDown() {}
};

TEST_F(T0AttestationFlowTest, test_t0_flow_single_afm_with_attestation_status_set)
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
}


TEST_F(T0AttestationFlowTest, DISABLED_test_t0_flow_with_challenge_status_set_single_afm)
{

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Obtain the example message for succesful_version
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
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
    alt_u16* length_ptr = (alt_u16*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_LENGTH];
    *length_ptr = sizeof(SUCCESSFUL_ALGORITHMS);
    mctp_pkt[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_SPEC] = SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF;
    alt_u32* succ_algo_ptr = (alt_u32*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_HASH_ALGO];
    *succ_algo_ptr = MEAS_HASH_ALGO_TPM_ALG_SHA_384;
    succ_algo_ptr = (alt_u32*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_BASE_ASYM_SEL];
    *succ_algo_ptr = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
    succ_algo_ptr = (alt_u32*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_BASE_HASH_SEL];
    *succ_algo_ptr = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;

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
    length_ptr = (alt_u16*)&expected_nego_algo[UT_MCTP_SPDM_ALGORITHM_LENGTH];
    *length_ptr = sizeof(NEGOTIATE_ALGORITHMS);
    expected_nego_algo[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_SPEC] = SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF;
    alt_u32* nego_algo_ptr = (alt_u32*)&expected_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_ASYM_ALGO];
    *nego_algo_ptr = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;
    nego_algo_ptr = (alt_u32*)&expected_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_HASH_ALGO];
    *nego_algo_ptr = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;

    alt_u8 mctp_get_digest_pkt[11] = MCTP_SPDM_GET_DIGEST_MSG;
    mctp_get_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    mctp_get_digest_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    alt_u8 mctp_digest_pkt[59] = MCTP_SPDM_DIGEST_MSG;
    mctp_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 58;
    mctp_digest_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);
    mctp_digest_pkt[UT_MCTP_SPDM_DIGEST_SLOT_MASK] = 0x0;

    /*const alt_u8 td_expected_hash[SHA384_LENGTH] = {
    	0xa5, 0x18, 0xa7, 0x8e, 0x9f, 0x6a, 0x73, 0xdd, 0x61, 0x13, 0x73, 0x0e,
    	0x04, 0xca, 0x9c, 0xba, 0x16, 0x6c, 0x8f, 0x48, 0xa4, 0xf2, 0x45, 0x72,
    	0xe1, 0xc1, 0xf9, 0x5b, 0xbd, 0xa2, 0x09, 0x06, 0xe5, 0x63, 0x75, 0x24,
    	0xcf, 0x67, 0x78, 0x38, 0x64, 0x92, 0xd7, 0x3e, 0x6a, 0xa0, 0x6b, 0x2e};*/
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

    /*const alt_u8 td_crypto_data[128] = {
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
    		0x62, 0x87, 0x3d, 0x9c, 0x6b, 0xe3, 0x21, 0x22};*/
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
        0xb0, 0x21, 0x7e, 0x55, 0x18, 0x5b, 0x0e, 0xa5, 0x54, 0xf7, 0x44, 0x62,
        0x89, 0xb2, 0x40, 0xb1, 0x1d, 0x56, 0xe6, 0x61, 0xf1, 0x16, 0x4f, 0x49,
        0x6f, 0xd9, 0x42, 0x8d, 0x90, 0xeb, 0x4b, 0xf3, 0x9d, 0x0a, 0x7c, 0x40,
        0xd2, 0xdf, 0x39, 0x12, 0x89, 0xc9, 0x11, 0xb5, 0xa9, 0xd2, 0xef, 0x2b,
      	  //0xc9, 0x9a, 0x06, 0xda, 0x8b, 0xad, 0x33, 0xd6, 0xee, 0x2d, 0x62, 0x36,
      	  //0x7b, 0x9c, 0x5a, 0x4e, 0xf7, 0x0d, 0xc8, 0x6f, 0x54, 0xd8, 0x82, 0x27,
      	  //0xa5, 0x1d, 0x5a, 0x71, 0xe3, 0x95, 0x9a, 0xdf, 0xc3, 0xd1, 0x52, 0x71,
      	  //0x43, 0x95, 0xe3, 0x63, 0xc7, 0xe0, 0xc0, 0xd5, 0x49, 0x14, 0xf7, 0xbc
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
         	 measurement_pkt[UT_MEASUREMENT_RECORD + 7 + i] = cert_chain_hash[48 - 1 - i];
         }
     }

     measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH] = UT_OPAQUE_LENGTH;
     measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 1] = 0x0;
     measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 2] = 0xFE;
     measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 3] = 0xCA;
     measurement_pkt[UT_MEASUREMENT_SIGNATURE] = 0;
     measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
     measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 217;
     measurement_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

     alt_u32* meas_pkt_ptr = (alt_u32*)&measurement_pkt[UT_MEASUREMENT_SIGNATURE];

     LARGE_APPENDED_BUFFER big_buffer_2;
     LARGE_APPENDED_BUFFER* big_buffer_2_ptr = &big_buffer_2;
     reset_buffer((alt_u8*)big_buffer_2_ptr, sizeof(LARGE_APPENDED_BUFFER));

     append_large_buffer(big_buffer_2_ptr, (alt_u8*)&get_measurement_pkt[7], 36);
     append_large_buffer(big_buffer_2_ptr, (alt_u8*)&measurement_pkt[7], (211 - 96));


     msg_to_hash = (alt_u8*)big_buffer_2_ptr->buffer;

     // Manually generate signature
     generate_ecdsa_signature((alt_u32*)sig_r, (alt_u32*)sig_s, (alt_u32*)msg_to_hash, big_buffer_2_ptr->buffer_size,
     	               (alt_u32*)test_pubkey_cx, (alt_u32*)priv_key, sha_mode);

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
    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // If this is the last message, unset the final parameter of the functions
    ut_send_in_bmc_mctp_packet(m_memory, succ_cap_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt, 51, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_digest_pkt, 59, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 143, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Send in the packet
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt_challenge_auth, 239, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Send in the packet
    ut_send_in_bmc_mctp_packet(m_memory, measurement_pkt, 218, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

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
    	               (alt_u32*)test_pubkey_cx, (alt_u32*)priv_key, sha_mode);

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
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    EXPECT_EQ(read_from_mailbox(MB_PROVISION_STATUS_2), (alt_u32)MB_UFM_PROV_ATTESTATION_ENABLED);
    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for authentication
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Expect no spdm protocol error
    // TODO: debug this error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), (alt_u32) 0);
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), (alt_u32) 0);

    // This is the expected message sent back to the hosts
    // The message is sent in byte by byte format
    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(expected_get_ver_msg[i], IORD(m_mctp_memory, 0));
    }

    // This is the expected message sent back to the hosts
    // The message is sent in byte by byte format
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
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(get_measurement_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(get_measurement_pkt_2[i], IORD(m_mctp_memory, 0));
    }
}

TEST_F(T0AttestationFlowTest, DISABLED_test_t0_flow_with_challenge_status_set_two_afms)
{

    /*
     * Flow preparation
     */
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN2_TWO_AFM_UUID1_UUID2_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN2_TWO_AFM_UUID1_UUID2_FILE_SIZE);

    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // SET 1:
    // This is the expected response from PFR for the first AFM in the image

    // Obtain the example message for succesful_version
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
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
    alt_u16* length_ptr = (alt_u16*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_LENGTH];
    *length_ptr = sizeof(SUCCESSFUL_ALGORITHMS);
    mctp_pkt[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_SPEC] = SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF;
    alt_u32* succ_algo_ptr = (alt_u32*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_HASH_ALGO];
    *succ_algo_ptr = MEAS_HASH_ALGO_TPM_ALG_SHA_384;
    succ_algo_ptr = (alt_u32*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_BASE_ASYM_SEL];
    *succ_algo_ptr = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
    succ_algo_ptr = (alt_u32*)&mctp_pkt[UT_MCTP_SPDM_ALGORITHM_BASE_HASH_SEL];
    *succ_algo_ptr = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;

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
    length_ptr = (alt_u16*)&expected_nego_algo[UT_MCTP_SPDM_ALGORITHM_LENGTH];
    *length_ptr = sizeof(NEGOTIATE_ALGORITHMS);
    expected_nego_algo[UT_MCTP_SPDM_ALGORITHM_MEASUREMENT_SPEC] = SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF;
    alt_u32* nego_algo_ptr = (alt_u32*)&expected_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_ASYM_ALGO];
    *nego_algo_ptr = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;
    nego_algo_ptr = (alt_u32*)&expected_nego_algo[UT_MCTP_SPDM_NEGOTIATE_ALGORITHM_BASE_HASH_ALGO];
    *nego_algo_ptr = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                     BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;

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
        0xb0, 0x21, 0x7e, 0x55, 0x18, 0x5b, 0x0e, 0xa5, 0x54, 0xf7, 0x44, 0x62,
        0x89, 0xb2, 0x40, 0xb1, 0x1d, 0x56, 0xe6, 0x61, 0xf1, 0x16, 0x4f, 0x49,
        0x6f, 0xd9, 0x42, 0x8d, 0x90, 0xeb, 0x4b, 0xf3, 0x9d, 0x0a, 0x7c, 0x40,
        0xd2, 0xdf, 0x39, 0x12, 0x89, 0xc9, 0x11, 0xb5, 0xa9, 0xd2, 0xef, 0x2b,
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
         measurement_pkt[UT_MEASUREMENT_RECORD + 7 + i] = 0x3a;
     }

     measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH] = UT_OPAQUE_LENGTH;
     measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 1] = 0x0;
     measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 2] = 0xFE;
     measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 3] = 0xCA;
     measurement_pkt[UT_MEASUREMENT_SIGNATURE] = 0;
     measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
     measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 217;
     measurement_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

     alt_u32* meas_pkt_ptr = (alt_u32*)&measurement_pkt[UT_MEASUREMENT_SIGNATURE];

     LARGE_APPENDED_BUFFER big_buffer_2;
     LARGE_APPENDED_BUFFER* big_buffer_2_ptr = &big_buffer_2;
     reset_buffer((alt_u8*)big_buffer_2_ptr, sizeof(LARGE_APPENDED_BUFFER));

     append_large_buffer(big_buffer_2_ptr, (alt_u8*)&get_measurement_pkt[7], 36);
     append_large_buffer(big_buffer_2_ptr, (alt_u8*)&measurement_pkt[7], (211 - 96));


     msg_to_hash = (alt_u8*)big_buffer_2_ptr->buffer;

     // Manually generate signature
     generate_ecdsa_signature((alt_u32*)sig_r, (alt_u32*)sig_s, (alt_u32*)msg_to_hash, big_buffer_2_ptr->buffer_size,
     	               (alt_u32*)test_pubkey_cx, (alt_u32*)priv_key, sha_mode);

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
    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // If this is the last message, unset the final parameter of the functions
    ut_send_in_bmc_mctp_packet(m_memory, succ_cap_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt, 51, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_digest_pkt, 59, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 143, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Send in the packet
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt_challenge_auth, 239, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Send in the packet
    ut_send_in_bmc_mctp_packet(m_memory, measurement_pkt, 218, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    reset_buffer((alt_u8*)big_buffer_2_ptr, sizeof(LARGE_APPENDED_BUFFER));

    measurement_pkt[UT_MEASUREMENT_RECORD] = 2;

    append_large_buffer(big_buffer_2_ptr, (alt_u8*)&get_measurement_pkt_2[7], 36);
    append_large_buffer(big_buffer_2_ptr, (alt_u8*)&measurement_pkt[7], (211 - 96));

    msg_to_hash = (alt_u8*)big_buffer_2_ptr->buffer;

    // Manually generate signature
    generate_ecdsa_signature((alt_u32*)sig_r, (alt_u32*)sig_s, (alt_u32*)msg_to_hash, big_buffer_2_ptr->buffer_size,
    	               (alt_u32*)test_pubkey_cx, (alt_u32*)priv_key, sha_mode);

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
    ut_send_in_bmc_mctp_packet(m_memory, measurement_pkt, 218, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);


    // Shall test for 1 block of measurement
    alt_u8 measurement_pkt_2[218] = MCTP_SPDM_MEASUREMENT_MSG;
    measurement_pkt_2[UT_MEASUREMENT_TOTAL_NUMBER_OF_BLOCKS] = 1;
    measurement_pkt_2[UT_MEASUREMENT_RECORD_LENGTH] = UT_RECORD_LENGTH;
    measurement_pkt_2[UT_MEASUREMENT_RECORD_LENGTH + 1] = 0x00;
    measurement_pkt_2[UT_MEASUREMENT_RECORD_LENGTH + 2] = 0x00;
    measurement_pkt_2[UT_MEASUREMENT_RECORD] = 1;
    measurement_pkt_2[UT_MEASUREMENT_RECORD + 1] = 1;
    measurement_pkt_2[UT_MEASUREMENT_RECORD + 2] = UT_RECORD_LENGTH;
    measurement_pkt_2[UT_MEASUREMENT_RECORD + 3] = 0x00;
    measurement_pkt_2[UT_MEASUREMENT_RECORD + 4] = 0x01;
    measurement_pkt_2[UT_MEASUREMENT_RECORD + 5] = 64;
    measurement_pkt_2[UT_MEASUREMENT_RECORD + 6] = 0x00;

    // For simplicity, just randomly fill in the measurement hash
    // Do the same for AFM in the ufm after this
    for (alt_u32 i = 0; i < 64; i++)
    {
    	measurement_pkt_2[UT_MEASUREMENT_RECORD + 7 + i] = 0x3a;
    }

    measurement_pkt_2[UT_MEASUREMENT_OPAQUE_LENGTH] = UT_OPAQUE_LENGTH;
    measurement_pkt_2[UT_MEASUREMENT_OPAQUE_LENGTH + 1] = 0x0;
    measurement_pkt_2[UT_MEASUREMENT_OPAQUE_LENGTH + 2] = 0xFE;
    measurement_pkt_2[UT_MEASUREMENT_OPAQUE_LENGTH + 3] = 0xCA;
    measurement_pkt_2[UT_MEASUREMENT_SIGNATURE] = 0;
    measurement_pkt_2[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    measurement_pkt_2[UT_MCTP_BYTE_COUNT_IDX] = 217;
    measurement_pkt_2[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    meas_pkt_ptr = (alt_u32*)&measurement_pkt_2[UT_MEASUREMENT_SIGNATURE];
    reset_buffer((alt_u8*)big_buffer_2_ptr, sizeof(LARGE_APPENDED_BUFFER));

    append_large_buffer(big_buffer_2_ptr, (alt_u8*)&get_measurement_pkt[7], 36);
    append_large_buffer(big_buffer_2_ptr, (alt_u8*)&measurement_pkt_2[7], (211 - 96));


    msg_to_hash = (alt_u8*)big_buffer_2_ptr->buffer;

    // Manually generate signature
    generate_ecdsa_signature((alt_u32*)sig_r, (alt_u32*)sig_s, (alt_u32*)msg_to_hash, big_buffer_2_ptr->buffer_size,
    	               (alt_u32*)test_pubkey_cx, (alt_u32*)priv_key, sha_mode);

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

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // SET 2: This test staged all the messages for PFR to retrieve for the second AFM in the BMC image

    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // If this is the last message, unset the final parameter of the functions
    ut_send_in_bmc_mctp_packet(m_memory, succ_cap_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt, 51, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_digest_pkt, 59, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    ut_send_in_bmc_mctp_packet(m_memory, mctp_cert_packet_with_cert_chain, 143, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Send in the packet
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt_challenge_auth, 239, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // Send in the packet
    ut_send_in_bmc_mctp_packet(m_memory, measurement_pkt_2, 218, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    reset_buffer((alt_u8*)big_buffer_2_ptr, sizeof(LARGE_APPENDED_BUFFER));

    measurement_pkt_2[UT_MEASUREMENT_RECORD] = 2;

    append_large_buffer(big_buffer_2_ptr, (alt_u8*)&get_measurement_pkt_2[7], 36);
    append_large_buffer(big_buffer_2_ptr, (alt_u8*)&measurement_pkt_2[7], (211 - 96));

    msg_to_hash = (alt_u8*)big_buffer_2_ptr->buffer;

    // Manually generate signature
    generate_ecdsa_signature((alt_u32*)sig_r, (alt_u32*)sig_s, (alt_u32*)msg_to_hash, big_buffer_2_ptr->buffer_size,
    	               (alt_u32*)test_pubkey_cx, (alt_u32*)priv_key, sha_mode);

    meas_pkt_ptr = (alt_u32*)&measurement_pkt_2[UT_MEASUREMENT_SIGNATURE];

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
    ut_send_in_bmc_mctp_packet(m_memory, measurement_pkt_2, 218, 0, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_MEDIUM, pfr_main());

    EXPECT_EQ(read_from_mailbox(MB_PROVISION_STATUS_2), (alt_u32)MB_UFM_PROV_ATTESTATION_ENABLED);

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for authentication
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Expect no spdm protocol error
    // TODO: debug this error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), (alt_u32) 0);
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), (alt_u32) 0);

    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), (alt_u32)PLATFORM_STATE_T0_BOOT_COMPLETE);

    // SET 1:

    // This is the expected message sent back to the hosts
    // The message is sent in byte by byte format
    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(expected_get_ver_msg[i], IORD(m_mctp_memory, 0));
    }

    // This is the expected message sent back to the hosts
    // The message is sent in byte by byte format
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
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(get_measurement_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(get_measurement_pkt_2[i], IORD(m_mctp_memory, 0));
    }

    // SET 2:
    // This is the expected message sent back to the hosts
    // The message is sent in byte by byte format
    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(expected_get_ver_msg[i], IORD(m_mctp_memory, 0));
    }

    // This is the expected message sent back to the hosts
    // The message is sent in byte by byte format
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
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(get_measurement_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(get_measurement_pkt_2[i], IORD(m_mctp_memory, 0));
    }
}

TEST_F(T0AttestationFlowTest, test_t0_flow_double_afm_with_attestation_status_set)
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
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN2_TWO_AFM_UUID1_UUID2_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN2_TWO_AFM_UUID1_UUID2_FILE_SIZE);
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

    /*alt_u8 meas_hash[SHA384_LENGTH] = {
    0x8d, 0x72, 0xb6, 0x20, 0xe1, 0x59, 0xb8, 0x52, 0xa3, 0x89, 0x31, 0x0e,
    0x03, 0x7c, 0x7e, 0x15, 0x93, 0x69, 0x87, 0xc4, 0xab, 0x82, 0x43, 0xfb,
    0xb3, 0xbb, 0x4b, 0xfd, 0x8b, 0x5a, 0xa7, 0x8d, 0x4b, 0xa2, 0xca, 0x36,
    0x3c, 0xe7, 0x29, 0x9c, 0x61, 0x0e, 0xb0, 0x2a, 0x75, 0x4e, 0x5a, 0x65
    };*/

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
         measurement_pkt[UT_MEASUREMENT_RECORD + 7 + i] = 0x3a;
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
    ut_send_in_bmc_mctp_packet(m_memory, measurement_pkt, 218, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // SET 2: This test staged all the messages for PFR to retrieve for the second AFM in the BMC image

    reset_buffer((alt_u8*)big_buffer_ptr, sizeof(LARGE_APPENDED_BUFFER));

    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_get_ver_msg[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_ver_mctp_pkt[7], 12);

    // If this is the last message, unset the final parameter of the functions
    ut_send_in_bmc_mctp_packet(m_memory, succ_cap_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&get_cap_mctp_pkt[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_cap_mctp_pkt[7], 12);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt, 51, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_nego_algo[7], sizeof(NEGOTIATE_ALGORITHMS));
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt[7], 44);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_digest_pkt, 59, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_digest_pkt[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_digest_pkt[7], 52);

    mctp_get_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0e;
    mctp_get_cert_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = CERT_REQUEST_SIZE;
    mctp_get_cert_pkt_ptr_2 = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr_2 = 0;

    mctp_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 206 + 8;
    mctp_cert_pkt[UT_MCTP_SOURCE_ADDR] = (0x02 << 1);
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 0xc8;
    mctp_get_pkt_ptr_2 = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr_2 = cert_chain_size - 0xc8;

    cert_chain_buffer_ptr = (alt_u8*)cert_res;

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

    // Have to manually append CHALLENGE/CHALLENGE_AUTH message here to generate the right hash and signature
    // To clear this before calling challenge function
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt_challenge[7], 36);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt_challenge_auth[7], (232 - 96));

    msg_to_hash = (alt_u8*)big_buffer_ptr->buffer;

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
        measurement_pkt[UT_MEASUREMENT_RECORD + 7 + i] = 0x3a;
    }

    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH] = UT_OPAQUE_LENGTH;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 1] = 0x0;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 2] = 0xFE;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 3] = 0xCA;
    measurement_pkt[UT_MEASUREMENT_SIGNATURE] = 0;
    measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 217;
    measurement_pkt[UT_MCTP_SOURCE_ADDR] = 0x02 << 1;

    meas_pkt_ptr = (alt_u32*)&measurement_pkt[UT_MEASUREMENT_SIGNATURE];

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


/////////////////////////
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

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for authentication
    //EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Expect no spdm protocol error
    //EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), (alt_u32) 0);
    //EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), (alt_u32) 0);

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
}


TEST_F(T0AttestationFlowTest, test_t0_flow_without_challenge_status_set)
{

    /*
     * Flow preparation
     */

    // Perform provisioning that does not enable CPLD to challenge other device
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Obtain the example message for succesful_version
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    alt_u8 succ_cap_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_CAPABILITY_MSG;
    succ_cap_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
    succ_cap_mctp_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    alt_u8 get_ver_mctp_pkt[11] = MCTP_SPDM_GET_VERSION_MSG;
    // Modify the SOM/EOM
    get_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;

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
    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // If this is the last message, unset the final parameter of the functions
    ut_send_in_bmc_mctp_packet(m_memory, succ_cap_mctp_pkt, 19, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    EXPECT_EQ(read_from_mailbox(MB_PROVISION_STATUS_2), (alt_u32)0x00);
    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for authentication
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Expect no spdm protocol error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), (alt_u32) MAJOR_ERROR_SPDM_PROTOCOL_ERROR);
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), (alt_u32) MINOR_ERROR_SPDM_CONNECTION);

    // Expect no request message to by CPLD as the status bit is not set
    // The message is sent in byte by byte format
    alt_u32 not_equal = 0;

    IORD(m_mctp_memory, 0);
    IORD(m_mctp_memory, 0);
    for (alt_u32 i = 0; i < 11; i++)
    {
        if (get_ver_mctp_pkt[i] != IORD(m_mctp_memory, 0))
        {
            not_equal = 1;
        }
    }
    EXPECT_TRUE(not_equal);

}

TEST_F(T0AttestationFlowTest, test_t0_flow_only_bios_boot)
{

    /*
     * Flow preparation
     */

    // Perform provisioning that enables CPLD to challenge other device
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);

    ut_prep_nios_gpi_signals();
    ut_send_acm_bios_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Obtain the example message for succesful_version
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = 0x02;

    alt_u8 succ_cap_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_CAPABILITY_MSG;
    succ_cap_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
    succ_cap_mctp_pkt[UT_MCTP_SOURCE_ADDR] = 0x02;

    alt_u8 get_ver_mctp_pkt[11] = MCTP_SPDM_GET_VERSION_MSG;
    // Modify the SOM/EOM
    get_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;

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
    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // If this is the last message, unset the final parameter of the functions
    ut_send_in_bmc_mctp_packet(m_memory, succ_cap_mctp_pkt, 19, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    EXPECT_EQ(read_from_mailbox(MB_PROVISION_STATUS_2), (alt_u32)0x01);

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for authentication
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Expect no spdm protocol error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), (alt_u32) 0);

    // Expect no request message to by CPLD as the status bit is not set
    // The message is sent in byte by byte format
    alt_u32 not_equal = 0;

    IORD(m_mctp_memory, 0);
    IORD(m_mctp_memory, 0);
    for (alt_u32 i = 0; i < 11; i++)
    {
        if (get_ver_mctp_pkt[i] != IORD(m_mctp_memory, 0))
        {
            not_equal = 1;
        }
    }
    EXPECT_TRUE(not_equal);
}

TEST_F(T0AttestationFlowTest, test_t0_flow_only_acm_boot)
{

    /*
     * Flow preparation
     */

    // Perform provisioning that enables CPLD to challenge other device
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);

    ut_prep_nios_gpi_signals();
    ut_send_acm_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Obtain the example message for succesful_version
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = 0x02;

    alt_u8 succ_cap_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_CAPABILITY_MSG;
    succ_cap_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
    succ_cap_mctp_pkt[UT_MCTP_SOURCE_ADDR] = 0x02;

    alt_u8 get_ver_mctp_pkt[11] = MCTP_SPDM_GET_VERSION_MSG;
    // Modify the SOM/EOM
    get_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;

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
    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // If this is the last message, unset the final parameter of the functions
    ut_send_in_bmc_mctp_packet(m_memory, succ_cap_mctp_pkt, 19, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    EXPECT_EQ(read_from_mailbox(MB_PROVISION_STATUS_2), (alt_u32)0x01);

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for authentication
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Expect no spdm protocol error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), (alt_u32) 0);

    // Expect no request message to by CPLD as the status bit is not set
    // The message is sent in byte by byte format
    alt_u32 not_equal = 0;

    IORD(m_mctp_memory, 0);
    IORD(m_mctp_memory, 0);
    for (alt_u32 i = 0; i < 11; i++)
    {
        if (get_ver_mctp_pkt[i] != IORD(m_mctp_memory, 0))
        {
            not_equal = 1;
        }
    }
    EXPECT_TRUE(not_equal);
}

TEST_F(T0AttestationFlowTest, test_t0_flow_only_bmc_boot)
{

    /*
     * Flow preparation
     */

    // Perform provisioning that enables CPLD to challenge other device
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);

    ut_prep_nios_gpi_signals();
    ut_send_bmc_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Obtain the example message for succesful_version
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = 0x02;

    alt_u8 succ_cap_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_CAPABILITY_MSG;
    succ_cap_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
    succ_cap_mctp_pkt[UT_MCTP_SOURCE_ADDR] = 0x02;

    alt_u8 get_ver_mctp_pkt[11] = MCTP_SPDM_GET_VERSION_MSG;
    // Modify the SOM/EOM
    get_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;

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
    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    // If this is the last message, unset the final parameter of the functions
    ut_send_in_bmc_mctp_packet(m_memory, succ_cap_mctp_pkt, 19, 0, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    EXPECT_EQ(read_from_mailbox(MB_PROVISION_STATUS_2), (alt_u32)0x01);
    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for authentication
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Expect no spdm protocol error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), (alt_u32) 0);


    // Expect no request message to by CPLD as the status bit is not set
    // The message is sent in byte by byte format
    alt_u32 not_equal = 0;

    IORD(m_mctp_memory, 0);
    IORD(m_mctp_memory, 0);
    for (alt_u32 i = 0; i < 11; i++)
    {
        if (get_ver_mctp_pkt[i] != IORD(m_mctp_memory, 0))
        {
            not_equal = 1;
        }
    }
    EXPECT_TRUE(not_equal);
}

TEST_F(T0AttestationFlowTest, test_t0_flow_double_afm_with_attestation_status_set_attest_before_and_after_update)
{
    SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
    // Reset system mocks and SPI flash
    sys->reset();
    sys->reset_spi_flash_mock();

    // Perform provisioning
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);

    // Reset Nios firmware
    ut_reset_nios_fw();

    /*
     * Test Preparation
     */
    // Load the entire image to flash
    // Always do this first for specific test case
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN2_TWO_AFM_UUID1_UUID2_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN2_TWO_AFM_UUID1_UUID2_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_3_TWO_AFM_FILE,
    		SIGNED_AFM_CAPSULE_V2P2_SVN_3_TWO_AFM_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN2_TWO_AFM_UUID1_UUID2_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN2_TWO_AFM_UUID1_UUID2_FILE, full_orig_image);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN3_TWO_AFM_UUID1_UUID2_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN3_TWO_AFM_UUID1_UUID2_FILE, full_new_image);

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

    /*alt_u8 meas_hash[SHA384_LENGTH] = {
    0x8d, 0x72, 0xb6, 0x20, 0xe1, 0x59, 0xb8, 0x52, 0xa3, 0x89, 0x31, 0x0e,
    0x03, 0x7c, 0x7e, 0x15, 0x93, 0x69, 0x87, 0xc4, 0xab, 0x82, 0x43, 0xfb,
    0xb3, 0xbb, 0x4b, 0xfd, 0x8b, 0x5a, 0xa7, 0x8d, 0x4b, 0xa2, 0xca, 0x36,
    0x3c, 0xe7, 0x29, 0x9c, 0x61, 0x0e, 0xb0, 0x2a, 0x75, 0x4e, 0x5a, 0x65
    };*/

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
         measurement_pkt[UT_MEASUREMENT_RECORD + 7 + i] = 0x3a;
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
    ut_send_in_bmc_mctp_packet(m_memory, measurement_pkt, 218, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // SET 2: This test staged all the messages for PFR to retrieve for the second AFM in the BMC image
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    reset_buffer((alt_u8*)big_buffer_ptr, sizeof(LARGE_APPENDED_BUFFER));

    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_get_ver_msg[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_ver_mctp_pkt[7], 12);

    // If this is the last message, unset the final parameter of the functions
    ut_send_in_bmc_mctp_packet(m_memory, succ_cap_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&get_cap_mctp_pkt[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_cap_mctp_pkt[7], 12);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt, 51, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_nego_algo[7], sizeof(NEGOTIATE_ALGORITHMS));
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt[7], 44);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_digest_pkt, 59, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_digest_pkt[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_digest_pkt[7], 52);

    mctp_get_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0e;
    mctp_get_cert_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = CERT_REQUEST_SIZE;
    mctp_get_cert_pkt_ptr_2 = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr_2 = 0;

    mctp_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 206 + 8;
    mctp_cert_pkt[UT_MCTP_SOURCE_ADDR] = (0x02 << 1);
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 0xc8;
    mctp_get_pkt_ptr_2 = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr_2 = cert_chain_size - 0xc8;

    cert_chain_buffer_ptr = (alt_u8*)cert_res;

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

    // Have to manually append CHALLENGE/CHALLENGE_AUTH message here to generate the right hash and signature
    // To clear this before calling challenge function
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt_challenge[7], 36);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt_challenge_auth[7], (232 - 96));

    msg_to_hash = (alt_u8*)big_buffer_ptr->buffer;

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
        measurement_pkt[UT_MEASUREMENT_RECORD + 7 + i] = 0x3a;
    }

    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH] = UT_OPAQUE_LENGTH;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 1] = 0x0;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 2] = 0xFE;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 3] = 0xCA;
    measurement_pkt[UT_MEASUREMENT_SIGNATURE] = 0;
    measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 217;
    measurement_pkt[UT_MCTP_SOURCE_ADDR] = 0x02 << 1;

    meas_pkt_ptr = (alt_u32*)&measurement_pkt[UT_MEASUREMENT_SIGNATURE];

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
    ut_send_in_bmc_mctp_packet(m_memory, measurement_pkt, 218, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    /*********************************************************
     *
     * Step One: Attest old image.
     *
     *********************************************************/
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

    /*
     * Verify results. Set 2
     */
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

    /*********************************************************
     *
     * Step two: Prepare New Image Attestation.
     *
     *********************************************************/
    SYSTEM_MOCK::get()->reset_code_block_counter();
    reset_buffer((alt_u8*)big_buffer_ptr, sizeof(LARGE_APPENDED_BUFFER));
    reset_buffer((alt_u8*)big_buffer_2_ptr, sizeof(LARGE_APPENDED_BUFFER));

    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_get_ver_msg[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_ver_mctp_pkt[7], 12);

    // If this is the last message, unset the final parameter of the functions
    ut_send_in_bmc_mctp_packet(m_memory, succ_cap_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&get_cap_mctp_pkt[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_cap_mctp_pkt[7], 12);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt, 51, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_nego_algo[7], sizeof(NEGOTIATE_ALGORITHMS));
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt[7], 44);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_digest_pkt, 59, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_digest_pkt[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_digest_pkt[7], 52);

    mctp_get_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0e;
    mctp_get_cert_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = CERT_REQUEST_SIZE;
    mctp_get_cert_pkt_ptr_2 = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr_2 = 0;

    mctp_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 206 + 8;
    mctp_cert_pkt[UT_MCTP_SOURCE_ADDR] = (0x02 << 1);
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 0xc8;
    mctp_get_pkt_ptr_2 = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr_2 = cert_chain_size - 0xc8;

    cert_chain_buffer_ptr = (alt_u8*)cert_res;

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

    // Have to manually append CHALLENGE/CHALLENGE_AUTH message here to generate the right hash and signature
    // To clear this before calling challenge function
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt_challenge[7], 36);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt_challenge_auth[7], (232 - 96));

    msg_to_hash = (alt_u8*)big_buffer_ptr->buffer;

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
        measurement_pkt[UT_MEASUREMENT_RECORD + 7 + i] = 0x3a;
    }

    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH] = UT_OPAQUE_LENGTH;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 1] = 0x0;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 2] = 0xFE;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 3] = 0xCA;
    measurement_pkt[UT_MEASUREMENT_SIGNATURE] = 0;
    measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 217;
    measurement_pkt[UT_MCTP_SOURCE_ADDR] = 0x02 << 1;

    meas_pkt_ptr = (alt_u32*)&measurement_pkt[UT_MEASUREMENT_SIGNATURE];

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

    reset_buffer((alt_u8*)big_buffer_2_ptr, sizeof(LARGE_APPENDED_BUFFER));

    measurement_pkt[UT_MEASUREMENT_RECORD] = 2;

    for (alt_u32 i = 0; i < 64; i++)
    {
        measurement_pkt[UT_MEASUREMENT_RECORD + 7 + i] = 0x6b;
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
    ut_send_in_bmc_mctp_packet(m_memory, measurement_pkt, 218, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // SET 2: This test staged all the messages for PFR to retrieve for the second AFM in the BMC image
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    reset_buffer((alt_u8*)big_buffer_ptr, sizeof(LARGE_APPENDED_BUFFER));

    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_get_ver_msg[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_ver_mctp_pkt[7], 12);

    // If this is the last message, unset the final parameter of the functions
    ut_send_in_bmc_mctp_packet(m_memory, succ_cap_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&get_cap_mctp_pkt[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_cap_mctp_pkt[7], 12);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt, 51, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_nego_algo[7], sizeof(NEGOTIATE_ALGORITHMS));
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt[7], 44);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_digest_pkt, 59, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_digest_pkt[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_digest_pkt[7], 52);

    mctp_get_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0e;
    mctp_get_cert_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = CERT_REQUEST_SIZE;
    mctp_get_cert_pkt_ptr_2 = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr_2 = 0;

    mctp_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 206 + 8;
    mctp_cert_pkt[UT_MCTP_SOURCE_ADDR] = (0x02 << 1);
    mctp_get_pkt_ptr = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_PORTION_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr = 0xc8;
    mctp_get_pkt_ptr_2 = (alt_u16*)&mctp_cert_pkt[UT_MCTP_SPDM_CERTIFICATE_REMAINDER_LENGTH_MASK - 1];
    *mctp_get_pkt_ptr_2 = cert_chain_size - 0xc8;

    cert_chain_buffer_ptr = (alt_u8*)cert_res;

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

    // Have to manually append CHALLENGE/CHALLENGE_AUTH message here to generate the right hash and signature
    // To clear this before calling challenge function
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt_challenge[7], 36);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt_challenge_auth[7], (232 - 96));

    msg_to_hash = (alt_u8*)big_buffer_ptr->buffer;

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
        measurement_pkt[UT_MEASUREMENT_RECORD + 7 + i] = 0x3a;
    }

    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH] = UT_OPAQUE_LENGTH;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 1] = 0x0;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 2] = 0xFE;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 3] = 0xCA;
    measurement_pkt[UT_MEASUREMENT_SIGNATURE] = 0;
    measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 217;
    measurement_pkt[UT_MCTP_SOURCE_ADDR] = 0x02 << 1;

    meas_pkt_ptr = (alt_u32*)&measurement_pkt[UT_MEASUREMENT_SIGNATURE];

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

    /*********************************************************
     *
     * Step three: Update new image.
     *
     *********************************************************/
    SYSTEM_MOCK::get()->reset_code_block_counter();

    // Send update intent
    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_MASK);

    // Run PFR Main. Always run with the timeout
    perform_t0_operations();

    /*
     * Verify result. Image updated
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));
    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Verify that active AFM is updated
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Verify that recovery AFM is updated
    alt_u32 recovery_afm_start = BMC_STAGING_REGION_RECOVERY_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 recovery_afm_end = recovery_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = recovery_afm_start >> 2; word_i < recovery_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Expect SVN Header to be updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM), (alt_u32) 3);

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(5));

    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(5));

    EXPECT_EQ(read_from_mailbox(MB_PROVISION_STATUS_2), (alt_u32)MB_UFM_PROV_ATTESTATION_ENABLED);

    /*
     * Verify results. Set 1
     */

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

    /*
     * Verify results. Set 2
     */
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

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}
