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

TEST_F(AttestationBasicTest, test_create_metadata_for_key_generation_256)
{
    alt_u32 cdix[SHA256_LENGTH/4] = {0};
    alt_u32 priv_key[SHA256_LENGTH/4] = {0};

    alt_u8 data[SHA256_LENGTH] = {
        0xf4, 0x19, 0x76, 0xe9, 0xeb, 0xd2, 0xab, 0x72, 0x8a, 0x78, 0xe1, 0x38,
        0x81, 0x17, 0xb7, 0xf6, 0x8d, 0xd8, 0xa3, 0x6c, 0xbf, 0xfa, 0xd0, 0x7d,
        0xde, 0x60, 0x74, 0xc1, 0x65, 0xfa, 0x42, 0x65};

    alt_u8 expected_priv_key[SHA256_LENGTH] = {
        0xa0, 0x78, 0xff, 0x47, 0x63, 0x97, 0x05, 0x11, 0x76, 0xd4, 0xc2, 0x63,
        0x65, 0x01, 0x0c, 0x25, 0x09, 0xbd, 0x43, 0x61, 0x81, 0x76, 0x91, 0x9d,
        0x8e, 0x41, 0x5b, 0x3a, 0x9e, 0x28, 0x8b, 0x40};

    // Create the priv key from data
    create_cdix_and_priv_keyx(cdix, priv_key, (alt_u32*)data, CRYPTO_256_MODE);

    alt_u32* expected_priv_key_ptr = (alt_u32*)expected_priv_key;

    // Check to see if the private key computed equals expected keys
    for (alt_u32 i = 0; i < (SHA256_LENGTH/4); i++)
    {
        ASSERT_EQ(expected_priv_key_ptr[i], priv_key[i]);
    }
}

TEST_F(AttestationBasicTest, test_basic_function_if_matcing_256)
{
    alt_u8 data_1[SHA256_LENGTH] = {
        0xf4, 0x19, 0x76, 0xe9, 0xeb, 0xd2, 0xab, 0x72, 0x8a, 0x78, 0xe1, 0x38,
        0x81, 0x17, 0xb7, 0xf6, 0x8d, 0xd8, 0xa3, 0x6c, 0xbf, 0xfa, 0xd0, 0x7d,
        0xde, 0x60, 0x74, 0xc1, 0x65, 0xfa, 0x42, 0x65};

    alt_u8 data_2[SHA256_LENGTH] = {
        0xf4, 0x19, 0x76, 0xe9, 0xeb, 0xd2, 0xab, 0x72, 0x8a, 0x78, 0xe1, 0x38,
        0x81, 0x17, 0xb7, 0xf6, 0x8d, 0xd8, 0xa3, 0x6c, 0xbf, 0xfa, 0xd0, 0x7d,
        0xde, 0x60, 0x74, 0xc1, 0x65, 0xfa, 0x42, 0x65};

    EXPECT_EQ(is_data_matching_stored_data((alt_u32*)data_1, (alt_u32*)data_2, SHA256_LENGTH >> 2), (alt_u32) 1);
}

TEST_F(AttestationBasicTest, test_basic_function_if_not_matching_256)
{
    alt_u8 data_1[SHA256_LENGTH] = {
        0xf4, 0x19, 0x76, 0xe9, 0xeb, 0xd2, 0xab, 0x72, 0x8a, 0x78, 0xe1, 0x38,
        0x81, 0x17, 0xb7, 0xf6, 0x8d, 0xd8, 0xa3, 0x6c, 0xbf, 0xfa, 0xd0, 0x7d,
        0xde, 0x60, 0x74, 0xc1, 0x65, 0xfa, 0x42, 0x65};

    alt_u8 data_2[SHA256_LENGTH] = {
        0xaa, 0x19, 0x76, 0xe9, 0xeb, 0xd2, 0xab, 0x72, 0x8a, 0x78, 0xe1, 0x38,
        0x81, 0x17, 0xb7, 0xf6, 0x8d, 0xd8, 0xa3, 0x6c, 0xbf, 0xfa, 0xd0, 0x7d,
        0xde, 0x60, 0x74, 0xc1, 0x65, 0xfa, 0x42, 0x65};

    EXPECT_EQ(is_data_matching_stored_data((alt_u32*)data_1, (alt_u32*)data_2, SHA256_LENGTH >> 2), (alt_u32) 0);
}

TEST_F(AttestationBasicTest, test_prep_attestation_256)
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
    for (alt_u32 i = 0; i < (SHA256_LENGTH/4); i++)
    {
        ASSERT_EQ(pub_key_0_ptr[i], 0xFFFFFFFF);
    }

    // Create the UDS
    generate_and_store_uds(cfm0_uds_ptr);

    // Use the uds to prepare for attestation
    prep_for_attestation(cfm0_uds_ptr, CRYPTO_384_MODE);

    // Public key 0 should now be stored
    for (alt_u32 i = 0; i < (SHA256_LENGTH/4); i++)
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

