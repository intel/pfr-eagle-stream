#include <iostream>
#include <chrono>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/objects.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class AttestationBasicTest : public testing::Test
{
public:
    virtual void SetUp() {
        SYSTEM_MOCK::get()->reset();

        //SYSTEM_MOCK::get()->write_x86_mem_to_file("testdata/48_data.hex", td_crypto_data_48, 48);
}
    virtual void TearDown() {}
};

TEST_F(AttestationBasicTest, test_generate_uds)
{
    /*
     * Test Preparation
     */

    // Load ROM image to CFM0
    alt_u32* cfm0_image = new alt_u32[CFM0_RECOVERY_IMAGE_FILE_SIZE/4];
    alt_u32* cfm0_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ROM_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, cfm0_image);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, cfm0_ufm_ptr);

    // Load active image to CFM1
    alt_u32* cfm1_image = new alt_u32[CFM1_ACTIVE_IMAGE_FILE_SIZE/4];
    alt_u32* cfm1_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, cfm1_image);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, cfm1_ufm_ptr);

    // Get the location of the UDS
    alt_u32* cfm0_uds_ptr = get_ufm_ptr_with_offset(CFM0_UNIQUE_DEVICE_SECRET);

    // UDS should not be stored yet
    for (alt_u32 i = 0; i < UDS_WORD_SIZE; i++)
    {
        ASSERT_EQ(cfm0_uds_ptr[i], 0xFFFFFFFF);
    }

    // Create the UDS
    generate_and_store_uds(cfm0_uds_ptr);

    // Check UDS stored
    // UDS should not be stored yet
    for (alt_u32 i = 0; i < UDS_WORD_SIZE; i++)
    {
        ASSERT_NE(cfm0_uds_ptr[i], 0xFFFFFFFF);
    }

    // Check to see if CFM0 is untouched except the UDS section
    for (alt_u32 i = 0; i < (UFM_CPLD_UNTOUCHED_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm0_ufm_ptr[i], cfm0_image[i]);
    }

    // CPLD Active image should be untouched
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], cfm1_image[i]);
    }

    // Clean up
    delete[] cfm0_image;
    delete[] cfm1_image;
}

TEST_F(AttestationBasicTest, test_create_metadata_for_key_generation_384)
{
    alt_u32 cdix[SHA384_LENGTH/4] = {0};
    alt_u32 priv_key[SHA384_LENGTH/4] = {0};

    alt_u8 data[96] = {
        0x55, 0x05, 0xdf, 0xf8, 0xbd, 0xaa, 0x28, 0x42, 0x85, 0x82, 0x97, 0x88,
        0x58, 0x28, 0x58, 0x9b, 0x1c, 0xeb, 0x0d, 0xda, 0xe2, 0xf8, 0x4a, 0x3f,
        0x78, 0xcd, 0x7a, 0x7f, 0x72, 0x9f, 0x25, 0x64, 0x50, 0x9a, 0xdc, 0xdd,
        0xd1, 0xb4, 0x57, 0xa1, 0xb4, 0xa7, 0x6b, 0x8e, 0x4f, 0x1d, 0xe9, 0xbf,
        0x5e, 0x46, 0x77, 0xee, 0x95, 0x1b, 0x61, 0xeb, 0xc8, 0x23, 0x32, 0xbe,
        0x40, 0x1b, 0xa0, 0x36, 0xe5, 0x35, 0xd5, 0xe5, 0xa9, 0x4e, 0x06, 0x81,
        0x7f, 0x92, 0x96, 0xa6, 0x03, 0xa6, 0x40, 0x36, 0xb1, 0xe9, 0x01, 0xf5,
        0x51, 0xf4, 0x90, 0x18, 0xf7, 0x61, 0xbb, 0xd2, 0x2c, 0x37, 0x5d, 0xa2};

    alt_u8 expected_cdix[SHA384_LENGTH] = {
        0x3d, 0x6e, 0x4c, 0xc0, 0x1c, 0x78, 0x32, 0x77, 0x30, 0x63, 0xf0, 0xe3,
        0xd2, 0x39, 0x84, 0x26, 0x0d, 0xc6, 0xf6, 0x0b, 0x97, 0xd9, 0x5f, 0xbd,
        0x32, 0x8e, 0x57, 0xf1, 0x9c, 0x23, 0x2b, 0xd7, 0xfd, 0x62, 0x25, 0x31,
        0x52, 0xea, 0x3f, 0x6d, 0xea, 0xd7, 0x3d, 0x27, 0x9a, 0xea, 0x1b, 0x20};

    // Create the cdix from data
    create_cdix_and_priv_keyx(cdix, priv_key, (alt_u32*)data, CRYPTO_384_MODE);

    alt_u32* expected_cdix_ptr = (alt_u32*)expected_cdix;

    // Check to see if the cdix computed equals expected cdix
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
        ASSERT_EQ(expected_cdix_ptr[i], cdix[i]);
    }
}

TEST_F(AttestationBasicTest, test_basic_function_if_matcing_384)
{
    alt_u8 data_1[SHA384_LENGTH] = {
        0xf4, 0x19, 0x76, 0xe9, 0xeb, 0xd2, 0xab, 0x72, 0x8a, 0x78, 0xe1, 0x38,
        0x81, 0x17, 0xb7, 0xf6, 0x8d, 0xd8, 0xa3, 0x6c, 0xbf, 0xfa, 0xd0, 0x7d,
        0xde, 0x60, 0x74, 0xc1, 0x65, 0xfa, 0x42, 0x65, 0x49, 0x29, 0x95, 0x1b,
        0xde, 0x60, 0x74, 0xc1, 0x65, 0xfa, 0x42, 0x65, 0x49, 0x29, 0x95, 0x1b};

    alt_u8 data_2[SHA384_LENGTH] = {
        0xf4, 0x19, 0x76, 0xe9, 0xeb, 0xd2, 0xab, 0x72, 0x8a, 0x78, 0xe1, 0x38,
        0x81, 0x17, 0xb7, 0xf6, 0x8d, 0xd8, 0xa3, 0x6c, 0xbf, 0xfa, 0xd0, 0x7d,
        0xde, 0x60, 0x74, 0xc1, 0x65, 0xfa, 0x42, 0x65, 0x49, 0x29, 0x95, 0x1b,
        0xde, 0x60, 0x74, 0xc1, 0x65, 0xfa, 0x42, 0x65, 0x49, 0x29, 0x95, 0x1b};

    EXPECT_EQ(is_data_matching_stored_data((alt_u32*)data_1, (alt_u32*)data_2, SHA384_LENGTH >> 2), (alt_u32) 1);
}

TEST_F(AttestationBasicTest, test_basic_function_if_not_matching_384)
{
    alt_u8 data_1[SHA384_LENGTH] = {
        0xf4, 0x19, 0x76, 0xe9, 0xeb, 0xd2, 0xab, 0x72, 0x8a, 0x78, 0xe1, 0x38,
        0x81, 0x17, 0xb7, 0xf6, 0x8d, 0xd8, 0xa3, 0x6c, 0xbf, 0xfa, 0xd0, 0x7d,
        0xde, 0x60, 0x74, 0xc1, 0x65, 0xfa, 0x42, 0x65, 0x49, 0x29, 0x95, 0x1b,
        0xde, 0x60, 0x74, 0xc1, 0x65, 0xfa, 0x42, 0x65, 0x49, 0x29, 0x95, 0x1b};

    alt_u8 data_2[SHA384_LENGTH] = {
        0xf1, 0x19, 0x76, 0xe9, 0xeb, 0xd2, 0xab, 0x72, 0x8a, 0x78, 0xe1, 0x38,
        0x81, 0x17, 0xb7, 0xf6, 0x8d, 0xd8, 0xa3, 0x6c, 0xbf, 0xfa, 0xd0, 0x7d,
        0xde, 0x60, 0x74, 0xc1, 0x65, 0xfa, 0x42, 0x65, 0x49, 0x29, 0x95, 0x1b,
        0xde, 0x60, 0x74, 0xc1, 0x65, 0xfa, 0x42, 0x65, 0x49, 0x29, 0x95, 0x1b};

    EXPECT_EQ(is_data_matching_stored_data((alt_u32*)data_1, (alt_u32*)data_2, SHA384_LENGTH >> 2), (alt_u32) 0);
}

TEST_F(AttestationBasicTest, test_prep_attestation_384)
{
    /*
     * Test Preparation
     */

    // Load ROM image to CFM0
    alt_u32* cfm0_image = new alt_u32[CFM0_RECOVERY_IMAGE_FILE_SIZE/4];
    alt_u32* cfm0_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ROM_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, cfm0_image);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, cfm0_ufm_ptr);

    // Load active image to CFM1
    alt_u32* cfm1_image = new alt_u32[CFM1_ACTIVE_IMAGE_FILE_SIZE/4];
    alt_u32* cfm1_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);

    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, cfm1_image);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, cfm1_ufm_ptr);

    // Get the location of the UDS
    alt_u32* cfm0_uds_ptr = get_ufm_ptr_with_offset(CFM0_UNIQUE_DEVICE_SECRET);

    // Get the location of public key 0
    alt_u32* pub_key_0_ptr = get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_0);

    // UDS should not be stored yet
    for (alt_u32 i = 0; i < UDS_WORD_SIZE; i++)
    {
        ASSERT_EQ(cfm0_uds_ptr[i], 0xFFFFFFFF);
    }

    // Pub key 0 should not be stored yet
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
        ASSERT_EQ(pub_key_0_ptr[i], 0xFFFFFFFF);
    }

    // Create the UDS
    generate_and_store_uds(cfm0_uds_ptr);

    // Use the uds to prepare for attestation
    prep_for_attestation(cfm0_uds_ptr, CRYPTO_384_MODE);

    // Public key 0 should now be stored
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
        ASSERT_NE(pub_key_0_ptr[i], 0xFFFFFFFF);
    }

    // Check to see if CFM0 is untouched except the UDS section
    for (alt_u32 i = 0; i < (UFM_CPLD_UNTOUCHED_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm0_ufm_ptr[i], cfm0_image[i]);
    }

    // CPLD Active image should be untouched
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ufm_ptr[i], cfm1_image[i]);
    }

    // Clean up
    delete[] cfm0_image;
    delete[] cfm1_image;
}
