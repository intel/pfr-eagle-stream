#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class PFRSpdmFlowTest : public testing::Test
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

TEST_F(PFRSpdmFlowTest, test_spdm_micro_function_with_cpld_as_requester)
{
    /**
     *
     * Test preparation
     *
     */

    CRYPTO_MODE sha_mode = CRYPTO_384_MODE;
    alt_u32 sha_size = SHA384_LENGTH;

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

    // Obtain the example packet
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set second last param to 0
    // Set last param to zero meaning zero delay
    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 1, 0);
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

    spdm_context_ptr->timeout = 1;

    // Set uuid 2
    spdm_context_ptr->uuid = 2;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

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

    /**
     *
     * Test preparation
     *
     */
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
    /*succ_cap_mctp_pkt[UT_MCTP_SPDM_CAPABILITY_FLAGS_IDX] = SUPPORTS_DIGEST_CERT_RESPONSE_MSG_MASK |
                                                           SUPPORTS_CHALLENGE_AUTH_RESPONSE_MSG_MASK |
											               SUPPORTS_MEAS_RESPONSE_WITH_SIG_GEN_MSG_MASK;*/

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, succ_cap_mctp_pkt, 19, 1, 0);

    EXPECT_EQ(requester_cpld_get_capability(spdm_context_ptr, mctp_context_ptr), alt_u32(0));

    alt_u8 get_cap_mctp_pkt[11] = MCTP_SPDM_GET_CAPABILITY_MSG;
    get_cap_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    get_cap_mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(get_cap_mctp_pkt[i], IORD(m_mctp_memory, 0));
    }

    EXPECT_EQ(spdm_context_ptr->capability.ct_exponent, succ_cap_mctp_pkt[UT_MCTP_SPDM_CAPABILITY_CT_EXPONENT_IDX]);
    EXPECT_EQ(spdm_context_ptr->capability.flags, *cap_ptr);
    //EXPECT_EQ(spdm_context_ptr->capability.flags[3], succ_cap_mctp_pkt[UT_MCTP_SPDM_CAPABILITY_FLAGS_IDX]);

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

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt, 51, 1, 0);

    EXPECT_EQ(requester_cpld_negotiate_algorithm(spdm_context_ptr, mctp_context_ptr), alt_u32(0));

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

    alt_u8 mctp_get_digest_pkt[11] = MCTP_SPDM_GET_DIGEST_MSG;
    mctp_get_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    mctp_get_digest_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    alt_u8 mctp_digest_pkt[59] = MCTP_SPDM_DIGEST_MSG;
    mctp_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 58;
    mctp_digest_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);
    mctp_digest_pkt[UT_MCTP_SPDM_DIGEST_SLOT_MASK] = 0x0;

    for (alt_u8 i = 11; i < 59; i++)
    {
    	mctp_digest_pkt[i] = cert_chain_digest[i - 11];
    }

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_digest_pkt, 59, 1, 0);

    EXPECT_EQ(requester_cpld_get_digest(spdm_context_ptr, mctp_context_ptr), alt_u32(0));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 11; i++)
    {
        EXPECT_EQ(mctp_get_digest_pkt[i], IORD(m_mctp_memory, 0));
    }

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

    //EXPECT_EQ(requester_cpld_get_certificate(spdm_context_ptr, BMC), alt_u32(0));

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

    //EXPECT_EQ(requester_cpld_get_certificate(spdm_context_ptr, BMC), alt_u32(0));

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

    //EXPECT_EQ(requester_cpld_get_certificate(spdm_context_ptr, BMC), alt_u32(0));

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

    //EXPECT_EQ(requester_cpld_get_certificate(spdm_context_ptr, BMC), alt_u32(0));

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

    //EXPECT_EQ(requester_cpld_get_certificate(spdm_context_ptr, BMC), alt_u32(0));

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

    //EXPECT_EQ(requester_cpld_get_certificate(spdm_context_ptr, BMC), alt_u32(0));

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

    EXPECT_EQ(requester_cpld_get_certificate(spdm_context_ptr, mctp_context_ptr), alt_u32(0));

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

    alt_u8 mctp_pkt_challenge[43] = MCTP_SPDM_CHALLENGE_MSG;
    mctp_pkt_challenge[UT_CHALLENGE_SPDM_SLOT_NUMBER_IDX] = 0;
    mctp_pkt_challenge[UT_CHALLENGE_SPDM_MEAS_SUMMARY_HASH_TYPE_IDX] = 0xff;
    mctp_pkt_challenge[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_pkt_challenge[UT_MCTP_BYTE_COUNT_IDX] = 42;
    mctp_pkt_challenge[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    alt_u8 mctp_pkt_challenge_auth[239] = MCTP_SPDM_CHALLENGE_AUTH_MSG;
    mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_SLOT_NUMBER_IDX] = 0x0;
    mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_SLOT_MASK_IDX] = 0x01;

    alt_u16* mctp_pkt_challenge_auth_ptr = (alt_u16*)&mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_OPAQUE_LENGTH_IDX];
    *mctp_pkt_challenge_auth_ptr = 0x02;
    mctp_pkt_challenge_auth[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_pkt_challenge_auth[UT_MCTP_BYTE_COUNT_IDX] = 238;
    mctp_pkt_challenge_auth[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    alt_u32* ptr = (alt_u32*) cert_chain_digest;
    alt_u32* pkt_ptr = (alt_u32*)&mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_CERT_CHAIN_HASH_IDX];

    for (alt_u32 i = 0; i < SHA384_LENGTH/4; i++)
    {
        pkt_ptr[i] = ptr[i];
    }

    // Have to manually append CHALLENGE/CHALLENGE_AUTH message here to generate the right hash and signature
    // To clear this before calling challenge function

    // To clear this before calling challenge function
    append_large_buffer(&spdm_context_ptr->transcript.m1_m2,
                        (alt_u8*)&mctp_pkt_challenge[7], 36);
    append_large_buffer(&spdm_context_ptr->transcript.m1_m2,
                        (alt_u8*)&mctp_pkt_challenge_auth[7], 232 - 96);


    alt_u8 test_pubkey_cx[SHA384_LENGTH] = {0};
    alt_u8 test_pubkey_cy[SHA384_LENGTH] = {0};
    alt_u8 sig_r[SHA384_LENGTH] = {0};
    alt_u8 sig_s[SHA384_LENGTH] = {0};

    alt_u8* msg_to_hash = (alt_u8*)&spdm_context_ptr->transcript.m1_m2.buffer[0];

    // Manually generate the public key here
    generate_pubkey((alt_u32*)test_pubkey_cx, (alt_u32*)test_pubkey_cy, (alt_u32*)priv_key_1, sha_mode);

    // Manually generate signature
    generate_ecdsa_signature((alt_u32*)sig_r, (alt_u32*)sig_s, (alt_u32*)msg_to_hash, spdm_context_ptr->transcript.m1_m2.buffer_size,
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

    alt_u8* leaf_cert_key = (alt_u8*)&spdm_context_ptr->local_context.stored_chal_meas_signing_key[0];
    for (alt_u32 i = 0; i < SHA384_LENGTH; i++)
    {
        leaf_cert_key[i] = test_pubkey_cx[i];
    }

    for (alt_u32 i = 0; i < SHA384_LENGTH; i++)
    {
        leaf_cert_key[i + SHA384_LENGTH] = test_pubkey_cy[i];
    }

    // Clear buffer m1m2 and msg c as this will be appended by the function internally as well
    //reset_buffer((alt_u8*)&spdm_context_ptr->transcript.m1_m2, sizeof(LARGE_APPENDED_BUFFER));
    //reset_buffer((alt_u8*)&spdm_context_ptr->transcript.message_c, sizeof(SMALL_APPENDED_BUFFER));
    alt_u32 msg_size = spdm_context_ptr->transcript.m1_m2.buffer_size;
    alt_u8* m1_m2_buffer_u8 = (alt_u8*) spdm_context_ptr->transcript.m1_m2.buffer;
    reset_buffer(&m1_m2_buffer_u8[msg_size - (36 + 232 - 96)], (36 + 232 - 96));
    spdm_context_ptr->transcript.m1_m2.buffer_size -= (36 + 232 - 96);

    // Send in the packet
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt_challenge_auth, 239, 1, 0);

    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    EXPECT_EQ(requester_cpld_challenge(spdm_context_ptr, mctp_context_ptr), (alt_u32)SPDM_RESPONSE_SUCCESS);

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(mctp_pkt_challenge[i], IORD(m_mctp_memory, 0));
    }
/*
    // Check the appended buffer of all the message thus far for transcript message A
    // This is used for future transaction of SPDM message
    alt_u8 msg_to_hash_2[200] = {0};
    alt_u8 hashed_msg[48] = {0};
    alt_u8 transcript_hashed_msg[48] = {0};

    //alt_u8* message_a_ptr = (alt_u8*)&spdm_context_ptr->transcript.message_a.buffer[0];
    alt_u8* message_a_ptr = (alt_u8*)&spdm_context_ptr->transcript.m1_m2.buffer[0];
    alt_u32 appended_size = 0;
    alt_u32 collected_size[6] = {
        sizeof(GET_VERSION),
	    sizeof(SUCCESSFUL_VERSION),
	    sizeof(GET_CAPABILITIES),
	    sizeof(SUCCESSFUL_CAPABILITIES),
	    sizeof(NEGOTIATE_ALGORITHMS),
	    (sizeof(SUCCESSFUL_ALGORITHMS) + 8)
    };

    alt_u8* collected_ptr[6] = {
        &expected_get_ver_msg[7],
	    &succ_ver_mctp_pkt[7],
	    &get_cap_mctp_pkt[7],
	    &succ_cap_mctp_pkt[7],
	    &expected_nego_algo[7],
	    &mctp_pkt[7]
    };
    for (alt_u32 i = 0; i < 6; i++)
    {
        alt_u8_memcpy(&msg_to_hash_2[appended_size], collected_ptr[i], collected_size[i]);
        appended_size += collected_size[i];

        if (i == 5)
        {
            calculate_and_save_sha((alt_u32*)hashed_msg, 0, (alt_u32*)msg_to_hash_2, appended_size, CRYPTO_384_MODE, DISENGAGE_DMA);
            calculate_and_save_sha((alt_u32*)transcript_hashed_msg, 0, (alt_u32*)message_a_ptr,
                                   appended_size, CRYPTO_384_MODE, DISENGAGE_DMA);
        }
    }

    for (alt_u32 i = 0; i < 48; i++)
    {
        EXPECT_EQ(hashed_msg[i], transcript_hashed_msg[i]);
    }
*/
}

TEST_F(PFRSpdmFlowTest, test_spdm_micro_function_with_cpld_as_responder)
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
    // Set uuid 2
    spdm_context_ptr->uuid = 2;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    alt_u8* received_msg = (alt_u8*)&mctp_context_ptr->processed_message.buffer;

    // Obtain the example message for get_version
    alt_u8 get_ver_mctp_pkt[11] = MCTP_SPDM_GET_VERSION_MSG;
    // Modify the SOM/EOM
    get_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    // Modify the source address
    get_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    // Obtain the example packet
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    succ_ver_mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 14;
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX] = 0x01;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX + 1] = 0;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX + 2] = 0x10;

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, get_ver_mctp_pkt, 11, 0, 0);

    alt_u32 received_msg_size = receive_message(mctp_context_ptr, received_msg, spdm_context_ptr->timeout);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, mctp_context_ptr, received_msg_size), alt_u32(SPDM_EXCHANGE_SUCCESS));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(succ_ver_mctp_pkt[i], IORD(m_mctp_memory, 0));
    }

    alt_u8 get_cap_msg[11] = MCTP_SPDM_GET_CAPABILITY_MSG;
    get_cap_msg[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    get_cap_msg[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);
    get_cap_msg[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;

    alt_u8 succ_cap_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_CAPABILITY_MSG;
    succ_cap_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    succ_cap_mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 18;
    succ_cap_mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    alt_u32* cap_ptr = (alt_u32*)&succ_cap_mctp_pkt[15];
    *cap_ptr = spdm_context_ptr->capability.flags;
    succ_cap_mctp_pkt[12] = spdm_context_ptr->capability.ct_exponent;

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, get_cap_msg, 11, 0, 0);

    received_msg_size = receive_message(mctp_context_ptr, received_msg, spdm_context_ptr->timeout);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, mctp_context_ptr, received_msg_size), alt_u32(SPDM_EXCHANGE_SUCCESS));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 19; i++)
    {
        EXPECT_EQ(succ_cap_mctp_pkt[i], IORD(m_mctp_memory, 0));
    }

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

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt_nego_algo, 39, 0, 0);

    received_msg_size = receive_message(mctp_context_ptr, received_msg, spdm_context_ptr->timeout);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, mctp_context_ptr, received_msg_size), alt_u32(SPDM_EXCHANGE_SUCCESS));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(mctp_pkt[i], IORD(m_mctp_memory, 0));
    }

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

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_digest_pkt, 11, 0, 0);

    received_msg_size = receive_message(mctp_context_ptr, received_msg, spdm_context_ptr->timeout);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, mctp_context_ptr, received_msg_size), alt_u32(SPDM_EXCHANGE_SUCCESS));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 59; i++)
    {
        EXPECT_EQ(mctp_digest_pkt[i], IORD(m_mctp_memory, 0));
    }

//////////////////////////////////////////////////////
    LARGE_APPENDED_BUFFER big_cert_buffer;
    LARGE_APPENDED_BUFFER* big_buffer_ptr = &big_cert_buffer;
    reset_buffer((alt_u8*)big_buffer_ptr, sizeof(LARGE_APPENDED_BUFFER));

    alt_u8 mctp_cert_packet_with_cert_chain[500] = {0};

    alt_u8* pkt = (alt_u8*)&mctp_context_ptr->processed_message.buffer;
    alt_u8 mctp_get_cert_pkt[15] = MCTP_SPDM_GET_CERTIFICATE_MSG;
    mctp_get_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0e;
    mctp_get_cert_pkt[UT_MCTP_SOURCE_ADDR] = ((BMC_SLAVE_ADDRESS << 1) | 0b1);
    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;

    alt_u16* mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0;

    alt_u8_memcpy(pkt, (alt_u8*)&mctp_get_cert_pkt[7], 15 - 7);

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

    for (alt_u8 i = 0; i < 15; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = mctp_cert_pkt[i];
    }

    alt_u8* cert = (alt_u8*) get_ufm_cpld_cert_ptr();
    for (alt_u8 i = 15; i < 15 + 0xc8; i++)
    {
        mctp_cert_packet_with_cert_chain[i] = cert[i - 15];
    }

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 0, 0);

    received_msg_size = receive_message(mctp_context_ptr, received_msg, spdm_context_ptr->timeout);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, mctp_context_ptr, received_msg_size), alt_u32(SPDM_EXCHANGE_SUCCESS));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15 + 0xc8; i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[15], (alt_u32)0xc8);

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;

    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0xc8;

    alt_u8_memcpy(pkt, (alt_u8*)&mctp_get_cert_pkt[7], 15 - 7);

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

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 0, 0);

    received_msg_size = receive_message(mctp_context_ptr, received_msg, spdm_context_ptr->timeout);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, mctp_context_ptr, received_msg_size), alt_u32(SPDM_EXCHANGE_SUCCESS));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15 + 0xc8; i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[15], (alt_u32)0xc8);

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;

    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x190;

    alt_u8_memcpy(pkt, (alt_u8*)&mctp_get_cert_pkt[7], 15 - 7);

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

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 0, 0);

    received_msg_size = receive_message(mctp_context_ptr, received_msg, spdm_context_ptr->timeout);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, mctp_context_ptr, received_msg_size), alt_u32(SPDM_EXCHANGE_SUCCESS));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15 + 0xc8; i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[15], (alt_u32)0xc8);

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x258;

    alt_u8_memcpy(pkt, (alt_u8*)&mctp_get_cert_pkt[7], 15 - 7);

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

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 0, 0);

    received_msg_size = receive_message(mctp_context_ptr, received_msg, spdm_context_ptr->timeout);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, mctp_context_ptr, received_msg_size), alt_u32(SPDM_EXCHANGE_SUCCESS));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15 + 0xc8; i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[15], (alt_u32)0xc8);

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x320;

    alt_u8_memcpy(pkt, (alt_u8*)&mctp_get_cert_pkt[7], 15 - 7);

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

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 0, 0);

    received_msg_size = receive_message(mctp_context_ptr, received_msg, spdm_context_ptr->timeout);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, mctp_context_ptr, received_msg_size), alt_u32(SPDM_EXCHANGE_SUCCESS));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15 + 0xc8; i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[15], (alt_u32)0xc8);

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x3e8;

    alt_u8_memcpy(pkt, (alt_u8*)&mctp_get_cert_pkt[7], 15 - 7);

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

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 0, 0);

    received_msg_size = receive_message(mctp_context_ptr, received_msg, spdm_context_ptr->timeout);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, mctp_context_ptr, received_msg_size), alt_u32(SPDM_EXCHANGE_SUCCESS));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15 + 0xc8; i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[15], (alt_u32)0xc8);

    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x400;
    mctp_get_cert_pkt_ptr = (alt_u16*)&mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK - 1];
    *mctp_get_cert_pkt_ptr = 0x4b0;

    alt_u8_memcpy(pkt, (alt_u8*)&mctp_get_cert_pkt[7], 15 - 7);

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

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 0, 0);

    received_msg_size = receive_message(mctp_context_ptr, received_msg, spdm_context_ptr->timeout);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, mctp_context_ptr, received_msg_size), alt_u32(SPDM_EXCHANGE_SUCCESS));

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15 + (alt_u32)(*mctp_get_pkt_ptr); i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[15], (alt_u32)(*mctp_get_pkt_ptr));

    alt_u8* stored_cert_chain_ptr = (alt_u8*)&big_buffer_ptr->buffer[0];
    CERT_CHAIN_FORMAT* cert_chain = (CERT_CHAIN_FORMAT*) stored_cert_chain_ptr;

    alt_u16 cert_chain_length = cert_chain->length;
    alt_u32 sha_size = SHA384_LENGTH;
    cert_chain_length -= (alt_u16)(sha_size + sizeof(alt_u32));

    alt_u8* responder_cert = stored_cert_chain_ptr + sha_size + sizeof(alt_u32);

    alt_u32 key_buffer[100];
    alt_u32* leaf_cert_key = (alt_u32*)key_buffer;

    // Verify content of x509 certificate chain
    // Extract leaf cert key info and cache them for signature verification purposes.
    EXPECT_TRUE(pfr_verify_der_cert_chain(responder_cert, (alt_u32)cert_chain_length, leaf_cert_key, leaf_cert_key + (sha_size/4)));

    /*alt_u8 mctp_cert_packet_with_cert_chain[143] = {0};

    alt_u8 mctp_get_cert_pkt[15] = MCTP_SPDM_GET_CERTIFICATE_MSG;
    mctp_get_cert_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_cert_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0e;
    mctp_get_cert_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);
    mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_OFFSET_MASK] = 0;
    //mctp_get_cert_pkt[UT_MCTP_SPDM_GET_CERTIFICATE_LENGTH_MASK - 1] = (LARGE_SPDM_MESSAGE_MAX_BUFFER_SIZE/4) >> 8;
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
    }*/
///////////////////////////////////////////////////////////////////////
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

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    /*ut_send_in_bmc_mctp_packet(m_memory, get_ver_mctp_pkt, 11, 0, 0);

    alt_u32 received_msg_size = receive_message((MCTP_CONTEXT*)&spdm_context_ptr->mctp_context, received_msg, spdm_context_ptr->timeout, BMC);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, received_msg_size, BMC), alt_u32(0));*/

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    /*ut_send_in_bmc_mctp_packet(m_memory, get_cap_msg, 11, 0, 0);

    received_msg_size = receive_message((MCTP_CONTEXT*)&spdm_context_ptr->mctp_context, received_msg, spdm_context_ptr->timeout, BMC);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, received_msg_size, BMC), alt_u32(0));*/

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    /*ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt_nego_algo, 39, 0, 0);

    received_msg_size = receive_message((MCTP_CONTEXT*)&spdm_context_ptr->mctp_context, received_msg, spdm_context_ptr->timeout, BMC);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, received_msg_size, BMC), alt_u32(0));*/

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    /*ut_send_in_bmc_mctp_packet(m_memory, mctp_get_digest_pkt, 11, 0, 0);

    received_msg_size = receive_message((MCTP_CONTEXT*)&spdm_context_ptr->mctp_context, received_msg, spdm_context_ptr->timeout, BMC);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, received_msg_size, BMC), alt_u32(0));*/

    /*ut_send_in_bmc_mctp_packet(m_memory, mctp_get_cert_pkt, 15, 0, 0);

    received_msg_size = receive_message((MCTP_CONTEXT*)&spdm_context_ptr->mctp_context, received_msg, spdm_context_ptr->timeout, BMC);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, received_msg_size, BMC), alt_u32(0));*/

    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt_challenge, 43, 0, 0);

    received_msg_size = receive_message(mctp_context_ptr, received_msg, spdm_context_ptr->timeout);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, mctp_context_ptr, received_msg_size), alt_u32(0));

    ut_send_in_bmc_mctp_packet(m_memory, get_measurement_pkt, 43, 0, 0);

    received_msg_size = receive_message(mctp_context_ptr, received_msg, spdm_context_ptr->timeout);
    EXPECT_NE(received_msg_size, alt_u32(0));

    EXPECT_EQ(respond_message_handler(spdm_context_ptr, mctp_context_ptr, received_msg_size), alt_u32(0));

    /*EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(succ_ver_mctp_pkt[i], IORD(m_mctp_memory, 0));
    }*/

    /*EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 19; i++)
    {
        EXPECT_EQ(succ_cap_mctp_pkt[i], IORD(m_mctp_memory, 0));
    }*/

    /*EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 43; i++)
    {
        EXPECT_EQ(mctp_pkt[i], IORD(m_mctp_memory, 0));
    }*/

    /*EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 59; i++)
    {
        EXPECT_EQ(mctp_digest_pkt[i], IORD(m_mctp_memory, 0));
    }*/

    /*EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 143; i++)
    {
        EXPECT_EQ(mctp_cert_packet_with_cert_chain[i], IORD(m_mctp_memory, 0));
    }*/

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 239 - 96; i++)
    {
        EXPECT_EQ(mctp_pkt_challenge_auth[i], IORD(m_mctp_memory, 0));
    }

    alt_u8 sig_read[96] = {0};
    // Read out the signature
    for (alt_u32 i = 0; i < 96; i++)
    {
        sig_read[i] = IORD(m_mctp_memory, 0);
    }

    alt_u32* sig = (alt_u32*) sig_read;
    alt_u8* message = (alt_u8*) &spdm_context_ptr->transcript.m1_m2.buffer;

    alt_u32* cpld_public_key_1 = get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_1);

    // Verify the signature
    EXPECT_TRUE(verify_ecdsa_and_sha(cpld_public_key_1, cpld_public_key_1 + (48/4), sig, sig + (48/4), 0, (alt_u32*)message,
                         spdm_context_ptr->transcript.m1_m2.buffer_size, CRYPTO_384_MODE, DISENGAGE_DMA));

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
        sig_read[i] = IORD(m_mctp_memory, 0);
    }

    sig = (alt_u32*) sig_read;
    message = (alt_u8*) large_buf_ptr->buffer;

    // Verify the signature
    EXPECT_TRUE(verify_ecdsa_and_sha(cpld_public_key_1, cpld_public_key_1 + (48/4), sig, sig + (48/4), 0, (alt_u32*)message,
                             large_buf_ptr->buffer_size, CRYPTO_384_MODE, DISENGAGE_DMA));

    // Check the appended buffer of all the message thus far
    // This is used for future transaction of SPDM message
/*    alt_u8 msg_to_hash[100] = {0};
    alt_u8 hashed_msg[48] = {0};
    alt_u8 transcript_hashed_msg[48] = {0};

    alt_u8* message_a_ptr = (alt_u8*)&spdm_context_ptr->transcript.message_a.buffer[0];
    alt_u32 appended_size = 0;
    alt_u32 collected_size[6] = {
        sizeof(GET_VERSION),
	    sizeof(SUCCESSFUL_VERSION),
	    sizeof(GET_CAPABILITIES),
	    sizeof(SUCCESSFUL_CAPABILITIES),
	    sizeof(NEGOTIATE_ALGORITHMS),
	    sizeof(SUCCESSFUL_ALGORITHMS)
    };

    alt_u8* collected_ptr[6] = {
        &get_ver_mctp_pkt[7],
	    &succ_ver_mctp_pkt[7],
	    &get_cap_msg[7],
	    &succ_cap_mctp_pkt[7],
	    &mctp_pkt_nego_algo[7],
	    &mctp_pkt[7]
    };
    for (alt_u32 i = 0; i < 6; i++)
    {
        alt_u8_memcpy(&msg_to_hash[appended_size], collected_ptr[i], collected_size[i]);
        appended_size += collected_size[i];

        if (i == 5)
        {
            calculate_and_save_sha((alt_u32*)hashed_msg, 0, (alt_u32*)msg_to_hash, appended_size, CRYPTO_384_MODE, DISENGAGE_DMA);
            calculate_and_save_sha((alt_u32*)transcript_hashed_msg, 0, (alt_u32*)message_a_ptr,
                                   appended_size, CRYPTO_384_MODE, DISENGAGE_DMA);
        }
    }

    for (alt_u32 i = 0; i < 48; i++)
    {
        EXPECT_EQ(hashed_msg[i], transcript_hashed_msg[i]);
    }*/
}

TEST_F(PFRSpdmFlowTest, test_cpld_sends_request_to_host)
{
    /**
     *
     * Test preparation
     *
     */
    // Initialize MCTP context
    MCTP_CONTEXT mctp_ctx;
    MCTP_CONTEXT* mctp_ctx_ptr = (MCTP_CONTEXT*) &mctp_ctx;

    // Clear buffer
    reset_buffer((alt_u8*)mctp_ctx_ptr, sizeof(MCTP_CONTEXT));

    // Set Challenger host in order to point to correct location
    mctp_ctx_ptr->challenger_type = BMC;

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

    // Obtain the example packet
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_get_ver_msg[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_ver_mctp_pkt[7], 12);

    // Send in the packet to RXFIFO
    // Need to stage the message as more than one message exchange, hence set last param to 1
    ut_send_in_bmc_mctp_packet(m_memory, succ_ver_mctp_pkt, 19, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    alt_u8 get_cap_mctp_pkt[11] = MCTP_SPDM_GET_CAPABILITY_MSG;
    get_cap_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    get_cap_mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

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

    append_large_buffer(big_buffer_ptr, (alt_u8*)&get_cap_mctp_pkt[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_cap_mctp_pkt[7], 12);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
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

    append_large_buffer(big_buffer_ptr, (alt_u8*)&expected_nego_algo[7], sizeof(NEGOTIATE_ALGORITHMS));
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt[7], 44);

    // Send in the packet to RXFIFO
    // Need not stage the message as only one message exchange, hence set last param to 0
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt, 51, 1, 0);

    alt_u8 mctp_get_digest_pkt[11] = MCTP_SPDM_GET_DIGEST_MSG;
    mctp_get_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    mctp_get_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0a;
    mctp_get_digest_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    alt_u8 mctp_digest_pkt[59] = MCTP_SPDM_DIGEST_MSG;
    mctp_digest_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_digest_pkt[UT_MCTP_BYTE_COUNT_IDX] = 58;
    mctp_digest_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);
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

    alt_u16* mctp_pkt_challenge_auth_ptr = (alt_u16*)&mctp_pkt_challenge_auth[UT_CHALLENGE_AUTH_SPDM_OPAQUE_LENGTH_IDX];
    *mctp_pkt_challenge_auth_ptr = 0x02;
    mctp_pkt_challenge_auth[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_pkt_challenge_auth[UT_MCTP_BYTE_COUNT_IDX] = 238;
    mctp_pkt_challenge_auth[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

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
    // Set possible measurement size
    ((AFM_BODY*) ufm_afm_pointer)->dmtf_spec_meas_val_size = 64;
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

    ufm_afm_pointer += sha_size/4;
    ufm_afm_pointer += 104;
    ufm_afm_pointer += 1;
    *ufm_afm_pointer = 1;
    ufm_afm_pointer += 1;
    *((alt_u8*)ufm_afm_pointer) = 1;

    ufm_afm_pointer += 1;
    for (alt_u32 i = 0; i < 16; i++)
    {
        ufm_afm_pointer[i] = 0xbbbbbbbb;
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

    alt_u8 measurement_pkt[218] = MCTP_SPDM_MEASUREMENT_MSG;
    // Shall test for 1 block of measurement
    measurement_pkt[UT_MEASUREMENT_TOTAL_NUMBER_OF_BLOCKS] = 1;
    measurement_pkt[UT_MEASUREMENT_RECORD_LENGTH] = UT_RECORD_LENGTH;
    measurement_pkt[UT_MEASUREMENT_RECORD_LENGTH + 1] = 0x00;
    measurement_pkt[UT_MEASUREMENT_RECORD_LENGTH + 2] = 0x00;
    measurement_pkt[UT_MEASUREMENT_RECORD] = 1;
    measurement_pkt[UT_MEASUREMENT_RECORD + 1] = 1;
    measurement_pkt[UT_MEASUREMENT_RECORD + 2] = UT_RECORD_LENGTH;
    measurement_pkt[UT_MEASUREMENT_RECORD + 3] = 0;
    measurement_pkt[UT_MEASUREMENT_RECORD + 4] = 1;
    measurement_pkt[UT_MEASUREMENT_RECORD + 5] = 64;
    measurement_pkt[UT_MEASUREMENT_RECORD + 6] = 0;

    // For simplicity, just randomly fill in the measurement hash
    // Do the same for AFM in the ufm after this
    for (alt_u32 i = 0; i < 64; i++)
    {
        measurement_pkt[UT_MEASUREMENT_RECORD + i + 7] = 0xbb;
    }

    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH] = UT_OPAQUE_LENGTH;
    measurement_pkt[UT_MEASUREMENT_OPAQUE_LENGTH + 1] = 0x0;
    measurement_pkt[UT_MEASUREMENT_SIGNATURE] = 0;
    measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 217;
    measurement_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    pkt_ptr = (alt_u32*)&measurement_pkt[UT_MEASUREMENT_SIGNATURE];

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
        pkt_ptr[i] = ptr[i];
    }

    pkt_ptr += (SHA384_LENGTH >> 2);
    ptr = (alt_u32*) sig_s;
    for (alt_u32 i = 0; i < (sha_size >> 2); i++)
    {
        pkt_ptr[i] = ptr[i];
    }

    // Send in the packet
    ut_send_in_bmc_mctp_packet(m_memory, measurement_pkt, 218, 0, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    init_mctp_spdm_context(mctp_ctx_ptr, SMBUS, (alt_u16)2);
    cpld_sends_request_to_host(mctp_ctx_ptr, (alt_u16)2);

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
}

TEST_F(PFRSpdmFlowTest, test_cpld_checks_request_from_host)
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

    LARGE_APPENDED_BUFFER big_buffer;
    LARGE_APPENDED_BUFFER* big_buffer_ptr = &big_buffer;
    reset_buffer((alt_u8*)big_buffer_ptr, sizeof(LARGE_APPENDED_BUFFER));

    // Obtain the example message for get_version
    alt_u8 get_ver_mctp_pkt[11] = MCTP_SPDM_GET_VERSION_MSG;
    // Modify the SOM/EOM
    get_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    // Modify the source address
    get_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    // Obtain the example packet
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    succ_ver_mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 14;
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX] = 0x01;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX + 1] = 0;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX + 2] = 0x10;

    append_large_buffer(big_buffer_ptr, (alt_u8*)&get_ver_mctp_pkt[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_ver_mctp_pkt[7], 8);

    ut_send_in_bmc_mctp_packet(m_memory, get_ver_mctp_pkt, 11, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

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

    append_large_buffer(big_buffer_ptr, (alt_u8*)&get_cap_msg[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&succ_cap_mctp_pkt[7], 12);

    // Stage next message
    ut_send_in_bmc_mctp_packet(m_memory, get_cap_msg, 11, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

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

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt_nego_algo[7], sizeof(NEGOTIATE_ALGORITHMS));
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt[7], 36);

    // Last message, need not stage next message
    ut_send_in_bmc_mctp_packet(m_memory, mctp_pkt_nego_algo, 39, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

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

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_digest_pkt[7], 4);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_digest_pkt[7], 52);

    ut_send_in_bmc_mctp_packet(m_memory, mctp_get_digest_pkt, 11, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);
//////////////////////////////////////////////////////////////////

    LARGE_APPENDED_BUFFER big_cert_buffer;
    LARGE_APPENDED_BUFFER* big_cert_buffer_ptr = &big_cert_buffer;
    reset_buffer((alt_u8*)big_cert_buffer_ptr, sizeof(LARGE_APPENDED_BUFFER));

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

////////////////////////////////////////////////////////
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
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    ut_send_in_bmc_mctp_packet(m_memory, get_measurement_pkt, 43, 0, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

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

    append_large_buffer(big_cert_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[15], (alt_u32)0xc8);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_cert_pkt[7], 8);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[7], 15 + 0xc8 - 7);

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

    append_large_buffer(big_cert_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[15], (alt_u32)0xc8);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_cert_pkt[7], 8);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[7], 15 + 0xc8 - 7);

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

    append_large_buffer(big_cert_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[15], (alt_u32)0xc8);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_cert_pkt[7], 8);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[7], 15 + 0xc8 - 7);

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

    append_large_buffer(big_cert_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[15], (alt_u32)0xc8);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_cert_pkt[7], 8);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[7], 15 + 0xc8 - 7);

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

    append_large_buffer(big_cert_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[15], (alt_u32)0xc8);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_cert_pkt[7], 8);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[7], 15 + 0xc8 - 7);

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

    append_large_buffer(big_cert_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[15], (alt_u32)0xc8);

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_cert_pkt[7], 8);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[7], 15 + 0xc8 - 7);

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

    append_large_buffer(big_cert_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[15], (alt_u32)(*mctp_get_pkt_ptr));

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_get_cert_pkt[7], 8);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_cert_packet_with_cert_chain[7], 15 + (alt_u32)(*mctp_get_pkt_ptr) - 7);

    alt_u8* stored_cert_chain_ptr = (alt_u8*)&big_cert_buffer_ptr->buffer[0];
    CERT_CHAIN_FORMAT* cert_chain = (CERT_CHAIN_FORMAT*) stored_cert_chain_ptr;

    alt_u16 cert_chain_length = cert_chain->length;
    alt_u32 sha_size = SHA384_LENGTH;
    cert_chain_length -= (alt_u16)(sha_size + sizeof(alt_u32));

    alt_u8* responder_cert = stored_cert_chain_ptr + sha_size + sizeof(alt_u32);

    alt_u32 key_buffer[100];
    alt_u32* leaf_cert_key = (alt_u32*)key_buffer;

    // Verify content of x509 certificate chain
    // Extract leaf cert key info and cache them for signature verification purposes.
    EXPECT_TRUE(pfr_verify_der_cert_chain(responder_cert, (alt_u32)cert_chain_length, leaf_cert_key, leaf_cert_key + (sha_size/4)));
//////////////////////////////////////////////
    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 239 - 96; i++)
    {
        EXPECT_EQ(mctp_pkt_challenge_auth[i], IORD(m_mctp_memory, 0));
    }

    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt_challenge[7], 36);
    append_large_buffer(big_buffer_ptr, (alt_u8*)&mctp_pkt_challenge_auth[7], (232 - 96));

    alt_u8 sig_read[96] = {0};
    // Read out the signature
    for (alt_u32 i = 0; i < 96; i++)
    {
        sig_read[i] = IORD(m_mctp_memory, 0);
    }

    alt_u32* sig = (alt_u32*) sig_read;
    alt_u8* message = (alt_u8*) &big_buffer.buffer;

    alt_u32* cpld_public_key_1 = get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_1);

    // Verify the signature
    EXPECT_TRUE(verify_ecdsa_and_sha(cpld_public_key_1, cpld_public_key_1 + (48/4), sig, sig + (48/4), 0, (alt_u32*)message,
                         big_buffer.buffer_size, CRYPTO_384_MODE, DISENGAGE_DMA));

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
        sig_read[i] = IORD(m_mctp_memory, 0);
    }

    sig = (alt_u32*) sig_read;
    message = (alt_u8*) large_buf_ptr->buffer;

    // Verify the signature
    EXPECT_TRUE(verify_ecdsa_and_sha(cpld_public_key_1, cpld_public_key_1 + (48/4), sig, sig + (48/4), 0, (alt_u32*)message,
                             large_buf_ptr->buffer_size, CRYPTO_384_MODE, DISENGAGE_DMA));
}

TEST_F(PFRSpdmFlowTest, test_cpld_checks_request_from_host_but_with_entropy_error)
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
    // Force health to fail within the crypto mock
    SYSTEM_MOCK::get()->force_mock_entropy_error();
    ut_run_main(CPLD_CFM0, true);
    ut_run_main(CPLD_CFM1, false);

    // Obtain the example message for get_version
    alt_u8 get_ver_mctp_pkt[11] = MCTP_SPDM_GET_VERSION_MSG;
    // Modify the SOM/EOM
    get_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    // Modify the source address
    get_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = (BMC_SLAVE_ADDRESS << 1);

    // Obtain the example packet
    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    succ_ver_mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 14;
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX] = 0x01;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX + 1] = 0;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX + 2] = 0x10;

    ut_send_in_bmc_mctp_packet(m_memory, get_ver_mctp_pkt, 11, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

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
    *cap_ptr = SUPPORTS_MEAS_RESPONSE_WITHOUT_SIG_GEN_MSG_MASK;

    // Stage next message
    ut_send_in_bmc_mctp_packet(m_memory, get_cap_msg, 11, 1, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

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
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

    alt_u8 get_measurement_pkt[11] = MCTP_SPDM_GET_MEASUREMENT_NO_SIG_MSG;
    get_measurement_pkt[UT_GET_MEASUREMENT_REQUEST_ATTRIBUTE] = 0;
    get_measurement_pkt[UT_GET_MEASUREMENT_OPERATION_MEASUREMENT_NUMBER] = 0x01;
    get_measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT | TAG_OWNER;
    get_measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 10;
    get_measurement_pkt[UT_MCTP_SOURCE_ADDR] = 0x02 << 1;

    alt_u8 measurement_pkt[90] = MCTP_SPDM_MEASUREMENT_NO_SIG_MSG;
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

    for (alt_u32 i = 0; i < 12; i++)
    {
        meas_pkt[i] = cfm1_hash[i];
    }

    measurement_pkt[UT_MEASUREMENT_NO_SIG_OPAQUE_LENGTH - 16] = UT_OPAQUE_LENGTH;
    measurement_pkt[UT_MEASUREMENT_NO_SIG_OPAQUE_LENGTH + 1 - 16] = 0x0;
    measurement_pkt[UT_MEASUREMENT_NO_SIG_OPAQUE_LENGTH + 2 - 16] = 0xfe;
    measurement_pkt[UT_MEASUREMENT_NO_SIG_OPAQUE_LENGTH + 3 - 16] = 0xca;
    measurement_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    measurement_pkt[UT_MCTP_BYTE_COUNT_IDX] = 73;
    measurement_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;

    ut_send_in_bmc_mctp_packet(m_memory, get_measurement_pkt, 43 - 32, 0, 0);
    EXPECT_EQ(read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR), (alt_u32)0x01);

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

    // Expect no spdm protocol error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), (alt_u32) 0);
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), (alt_u32) 0);

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
    for (alt_u32 i = 0; i < 202 - 96 - 32; i++)
    {
        EXPECT_EQ(measurement_pkt[i], IORD(m_mctp_memory, 0));
    }
}

