#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class AfmAuthenticationTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        // Reset system mocks and SPI flash
        sys->reset();
        sys->reset_spi_flash_mock();

        // Provision the system (e.g. root key hash)
        sys->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

        // Always use PCH SPI flash
        switch_spi_flash(SPI_FLASH_PCH);
    }

    virtual void TearDown() {}
};

TEST_F(AfmAuthenticationTest, test_get_afm_body_size)
{
    // Load signed BMC AFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_BMC_AFM_FILE, FULL_PFR_IMAGE_BMC_AFM_FILE_SIZE);
    alt_u32* afm_addr = get_spi_flash_ptr_with_offset(BMC_STAGING_REGION_ACTIVE_AFM_OFFSET + SIGNATURE_SIZE);

    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    AFM* afm_addr_ptr = (AFM*)afm_addr;

    // Check size of 1st device afm body
    alt_u32* afm_header = afm_addr_ptr->afm_addr_def;
    AFM_ADDR_DEF* afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    AFM_BODY* afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    alt_u32 afm_size = get_afm_body_size(afm_body);
    alt_u32 expected_afm_size = 792;
    EXPECT_EQ(afm_size, expected_afm_size);

    // Check size of 2nd device afm body
    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);
    expected_afm_size = 760;
    EXPECT_EQ(afm_size, expected_afm_size);

    // Check size of 3rd device afm body
    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);
    expected_afm_size = 824;
    EXPECT_EQ(afm_size, expected_afm_size);

}

TEST_F(AfmAuthenticationTest, test_retrieve_afm_index_measurement)
{
    // Test 1
    // Begin the test by having an empty UFM on the AFM storage region
    alt_u32* ufm_afm_pointer = get_ufm_ptr_with_offset(UFM_AFM_1);
    EXPECT_EQ(*ufm_afm_pointer, (alt_u32)0xffffffff);

    // Now, we filled in the AFM with UUID. Assume UUID is 2
    // We filled in the first region in the UFM.
    *((alt_u16*)ufm_afm_pointer) = 2;

    const alt_u8 measurement_hash[SHA384_LENGTH] = {
    		  0xd3, 0xc4, 0x0a, 0x17, 0xe4, 0x00, 0x05, 0x15, 0xc0, 0x7e, 0x68, 0xc9,
    		  0xc2, 0xf9, 0xed, 0x12, 0x51, 0xbb, 0x5e, 0x6e, 0xa0, 0x64, 0x5c, 0xbb,
    		  0x10, 0x59, 0x3c, 0x71, 0xdf, 0xdb, 0x03, 0x58, 0x10, 0x2c, 0x82, 0xa5,
    		  0x9f, 0x21, 0x8b, 0xfb, 0x32, 0xf1, 0x0b, 0xb6, 0x3a, 0xf6, 0xc1, 0x6d
    };

    // For testing, store measurement hash into the ufm
    // Make sure to set the possible measurement number correctly
    ufm_afm_pointer = ((alt_u32*)ufm_afm_pointer) + 140;
    alt_u8* alt_u8_ufm_afm_pointer = ((alt_u8*) ufm_afm_pointer) + 1;
    *alt_u8_ufm_afm_pointer = 2;
    // Get the offset to the measurement hash
    alt_u8_ufm_afm_pointer = alt_u8_ufm_afm_pointer + 3;
    alt_u8* hash = (alt_u8*)measurement_hash;
    for (alt_u32 i = 0; i < SHA384_LENGTH; i++)
    {
        alt_u8_ufm_afm_pointer[i] = hash[i];
    }

    alt_u8_ufm_afm_pointer += SHA384_LENGTH;
    for (alt_u32 i = 0; i < SHA384_LENGTH; i++)
    {
    	alt_u8_ufm_afm_pointer[i] = hash[i];
    }

    alt_u8 poss_meas = 0;
    alt_u32 poss_meas_size = 0;
    // Expect to obtain the hash within the ufm
    alt_u32* stored_hash = retrieve_afm_index_measurement(2, 1, &poss_meas, &poss_meas_size);

    ufm_afm_pointer = get_ufm_ptr_with_offset(UFM_AFM_1) + 141;

    for (alt_u32 i = 0; i < SHA384_LENGTH/2; i++)
    {
        EXPECT_EQ(stored_hash[i], ufm_afm_pointer[i]);
    }


    // Test 2
    ufm_afm_pointer = get_ufm_ptr_with_offset(UFM_AFM_1);

    // Now, we filled in the AFM with UUID. Assume UUID is 2
    // We filled in the first region in the UFM.
    *((alt_u16*)ufm_afm_pointer) = 2;

    const alt_u8 measurement_hash_1[SHA384_LENGTH] = {
    		  0xd3, 0xc4, 0x0a, 0x17, 0xe4, 0x00, 0x05, 0x15, 0xc0, 0x7e, 0x68, 0xc9,
    		  0xc2, 0xf9, 0xed, 0x12, 0x51, 0xbb, 0x5e, 0x6e, 0xa0, 0x64, 0x5c, 0xbb,
    		  0x10, 0x59, 0x3c, 0x71, 0xdf, 0xdb, 0x03, 0x58, 0x10, 0x2c, 0x82, 0xa5,
    		  0x9f, 0x21, 0x8b, 0xfb, 0x32, 0xf1, 0x0b, 0xb6, 0x3a, 0xf6, 0xc1, 0x6d
    };

    const alt_u8 measurement_hash_2[SHA384_LENGTH] = {
    		  0xf5, 0xe5, 0xbe, 0xee, 0xf1, 0x18, 0xdf, 0xc8, 0x40, 0xb2, 0x5b, 0x15,
    		  0xf8, 0xee, 0x10, 0x5c, 0xb5, 0x44, 0xaa, 0x45, 0x16, 0x39, 0x8b, 0xe6,
    		  0x06, 0xba, 0xca, 0xeb, 0xcc, 0x3b, 0xb3, 0xd3, 0x2e, 0x4d, 0xa1, 0x5f,
    		  0x18, 0xe5, 0xa2, 0xee, 0x16, 0xc3, 0x3f, 0x38, 0xa4, 0xf7, 0xd9, 0x87
    };

    // For testing, store measurement hash into the ufm
    // Make sure to set the possible measurement number correctly
    ufm_afm_pointer = ((alt_u32*)ufm_afm_pointer) + 140;
    alt_u8_ufm_afm_pointer = ((alt_u8*) ufm_afm_pointer) + 1;
    *alt_u8_ufm_afm_pointer = 3;
    // Get the offset to the measurement hash
    alt_u8_ufm_afm_pointer = alt_u8_ufm_afm_pointer + 3;
    hash = (alt_u8*)measurement_hash_1;
    for (alt_u32 i = 0; i < SHA384_LENGTH; i++)
    {
    	alt_u8_ufm_afm_pointer[i] = hash[i];
    }

    alt_u8_ufm_afm_pointer += SHA384_LENGTH;
    hash = (alt_u8*)measurement_hash_2;
    for (alt_u32 i = 0; i < SHA384_LENGTH; i++)
    {
    	alt_u8_ufm_afm_pointer[i] = hash[i];
    }

    alt_u8_ufm_afm_pointer += SHA384_LENGTH;
    hash = (alt_u8*)measurement_hash_1;
    for (alt_u32 i = 0; i < SHA384_LENGTH; i++)
    {
    	alt_u8_ufm_afm_pointer[i] = hash[i];
    }

    // Expect to obtain the hash within the ufm
    stored_hash = retrieve_afm_index_measurement(2, 1, &poss_meas, &poss_meas_size);

    ufm_afm_pointer = get_ufm_ptr_with_offset(UFM_AFM_1) + 141;

    for (alt_u32 i = 0; i < (SHA384_LENGTH*3)/4; i++)
    {
        EXPECT_EQ(stored_hash[i], ufm_afm_pointer[i]);
    }

}

TEST_F(AfmAuthenticationTest, test_retrieve_pub_key_from_afm)
{
    // Begin the test by having an empty UFM on the AFM storage region
    alt_u32* ufm_afm_pointer = get_ufm_ptr_with_offset(UFM_AFM_1);
    EXPECT_EQ(*ufm_afm_pointer, (alt_u32)0xffffffff);

    // Now, we filled in the AFM with UUID. Assume UUID is 2
    // We filled in the first region in the UFM.
    *((alt_u16*)ufm_afm_pointer) = 2;

    const alt_u8 test_pubkey_cx[SHA384_LENGTH] = {
    		  0xd3, 0xc4, 0x0a, 0x17, 0xe4, 0x00, 0x05, 0x15, 0xc0, 0x7e, 0x68, 0xc9,
    		  0xc2, 0xf9, 0xed, 0x12, 0x51, 0xbb, 0x5e, 0x6e, 0xa0, 0x64, 0x5c, 0xbb,
    		  0x10, 0x59, 0x3c, 0x71, 0xdf, 0xdb, 0x03, 0x58, 0x10, 0x2c, 0x82, 0xa5,
    		  0x9f, 0x21, 0x8b, 0xfb, 0x32, 0xf1, 0x0b, 0xb6, 0x3a, 0xf6, 0xc1, 0x6d
    };

    const alt_u8 test_pubkey_cy[SHA384_LENGTH] = {
    		  0xf5, 0xe5, 0xbe, 0xee, 0xf1, 0x18, 0xdf, 0xc8, 0x40, 0xb2, 0x5b, 0x15,
    		  0xf8, 0xee, 0x10, 0x5c, 0xb5, 0x44, 0xaa, 0x45, 0x16, 0x39, 0x8b, 0xe6,
    		  0x06, 0xba, 0xca, 0xeb, 0xcc, 0x3b, 0xb3, 0xd3, 0x2e, 0x4d, 0xa1, 0x5f,
    		  0x18, 0xe5, 0xa2, 0xee, 0x16, 0xc3, 0x3f, 0x38, 0xa4, 0xf7, 0xd9, 0x87
    };

    // For testing, store the public key into the ufm
    // For simplicity just tied this test to uuid of 2
    ufm_afm_pointer = ((alt_u32*)ufm_afm_pointer) + 10;
    alt_u32* key = (alt_u32*)test_pubkey_cx;
    for (alt_u32 i = 0; i < SHA384_LENGTH/4; i++)
    {
        ufm_afm_pointer[i] = key[i];
    }

    ufm_afm_pointer += SHA384_LENGTH/4;
    key = (alt_u32*)test_pubkey_cy;
    for (alt_u32 i = 0; i < SHA384_LENGTH/4; i++)
    {
        ufm_afm_pointer[i] = key[i];
    }

    // Expect to obtain the key within the ufm
    alt_u32* stored_key = retrieve_afm_pub_key(2);

    ufm_afm_pointer = get_ufm_ptr_with_offset(UFM_AFM_1) + 10;

    for (alt_u32 i = 0; i < SHA384_LENGTH/2; i++)
    {
        EXPECT_EQ(stored_key[i], ufm_afm_pointer[i]);
    }
}

TEST_F(AfmAuthenticationTest, test_get_empty_ufm_to_store_afm)
{
    // Begin the test by having an empty UFM on the AFM storage region
    alt_u32* ufm_afm_pointer = get_ufm_ptr_with_offset(UFM_AFM_1);
    EXPECT_EQ(*ufm_afm_pointer, (alt_u32)0xffffffff);

    // Expect the function to return the address offset to the first empty region
    alt_u32 empty_afm_region = get_ufm_empty_afm_offset();
    EXPECT_EQ(empty_afm_region, alt_u32(UFM_AFM_1));

    // Now, we filled in the AFM with UUID. Assume UUID is 2
    // We filled in the first region in the UFM.
    *((alt_u16*)ufm_afm_pointer) = 2;

    // It is expected the function to find the next empty location in UFM
    empty_afm_region = get_ufm_empty_afm_offset();
    EXPECT_EQ(empty_afm_region, alt_u32(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 2)));
}

TEST_F(AfmAuthenticationTest, test_get_stored_afm_in_ufm)
{
    // Begin test by writting the uuid into the designated location in UFM
    // Instead of writing to the first location, write to the second location
    alt_u32* ufm_afm_pointer = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 2));

    // Now, we filled in the AFM with UUID. Assume UUID is 2
    // We filled in the first region in the UFM.
    *((alt_u16*)ufm_afm_pointer) = 2;

    // It is expected the function to find the location of AFM within UFM with uuid 2
    alt_u32 empty_afm_region = get_ufm_afm_addr_offset(2);
    EXPECT_EQ(empty_afm_region, alt_u32(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 2)));

    // Experiment 1: Erase the uuid set previously
    ufm_erase_page(UFM_AFM_1);
    // Stroe uuid on different page of the UFM
    ufm_afm_pointer = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 2) + (UFM_CPLD_PAGE_SIZE >> 2));

    // Now, we filled in the AFM with UUID. Assume UUID is 3 this time
    *((alt_u16*)ufm_afm_pointer) = 3;

    // It is expected the function to find the location of AFM within UFM with uuid 3
    empty_afm_region = get_ufm_afm_addr_offset(3);
    EXPECT_EQ(empty_afm_region, alt_u32(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 2) + (UFM_CPLD_PAGE_SIZE >> 2)));

    EXPECT_EQ(get_ufm_afm_addr_offset(2), alt_u32(0));

    // Experiment 2: Erase uuid and leave the ufm empty
    ufm_erase_page(UFM_AFM_1);

    // It is expected the function to find the location of AFM within UFM with uuid 3
    empty_afm_region = get_ufm_afm_addr_offset(3);

    EXPECT_EQ(empty_afm_region, alt_u32(0));
}

TEST_F(AfmAuthenticationTest, test_authenticate_signed_afm_bmc)
{
    // Load signed BMC AFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_AFM_FILE, SIGNED_AFM_FILE_SIZE);

    alt_u32 matched_uuid = 0;
    // Get the ufm afm address with uuid 1
    EXPECT_TRUE(is_signed_afm_body_valid(0, 1, &matched_uuid));

    // Load signed BMC AFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_FULL_AFM_FILE, SIGNED_FULL_AFM_FILE_SIZE,
    		BMC_STAGING_REGION_AFM_ACTIVE_OFFSET + get_ufm_pfr_data()->bmc_staging_region);

    AFM* afm_ptr = (AFM*) get_spi_flash_ptr_with_offset(SIGNATURE_SIZE + BMC_STAGING_REGION_AFM_ACTIVE_OFFSET + get_ufm_pfr_data()->bmc_staging_region);

    alt_u32* afm_header = afm_ptr->afm_addr_def;
    AFM_ADDR_DEF* afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    AFM_BODY* afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    alt_u32 afm_size = get_afm_body_size(afm_body);

    alt_u32* afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1);
    alt_u32* afm_body_ptr = (alt_u32*)afm_body;
    EXPECT_TRUE(copy_afm_to_ufm(afm_ptr, matched_uuid));
    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 2));
    afm_body_ptr = (alt_u32*)afm_body;

    EXPECT_TRUE(copy_afm_to_ufm(afm_ptr, matched_uuid));
    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 1));
    afm_body_ptr = (alt_u32*)afm_body;

    EXPECT_TRUE(copy_afm_to_ufm(afm_ptr, matched_uuid));
    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }
}

TEST_F(AfmAuthenticationTest, test_validate_full_afm_in_bmc)
{
    // Load signed BMC AFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_BMC_AFM_FILE, FULL_PFR_IMAGE_BMC_AFM_FILE_SIZE);
    alt_u32* afm_addr = get_spi_flash_ptr_with_offset(BMC_STAGING_REGION_ACTIVE_AFM_OFFSET + SIGNATURE_SIZE);

    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    AFM* afm_addr_ptr = (AFM*)afm_addr;
    alt_u32* afm_header = afm_addr_ptr->afm_addr_def;
    AFM_ADDR_DEF* afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    AFM_BODY* afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    alt_u32 afm_size = get_afm_body_size(afm_body);

    alt_u32* afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1);
    alt_u32* afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 2));
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 1));
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }
}

TEST_F(AfmAuthenticationTest, test_validate_and_copy_same_afm_in_bmc)
{
    // Load signed BMC AFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_BMC_AFM_FILE, FULL_PFR_IMAGE_BMC_AFM_FILE_SIZE);
    alt_u32* afm_addr = get_spi_flash_ptr_with_offset(BMC_STAGING_REGION_ACTIVE_AFM_OFFSET + SIGNATURE_SIZE);

    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    AFM* afm_addr_ptr = (AFM*)afm_addr;
    alt_u32* afm_header = afm_addr_ptr->afm_addr_def;
    AFM_ADDR_DEF* afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    AFM_BODY* afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    alt_u32 afm_size = get_afm_body_size(afm_body);

    alt_u32* afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1);
    alt_u32* afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 2));
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 1));
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    // Expect that for the first time AFM is validated, UFM is not erased as they are empty
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(0));

    // Re-validate full AFM, simulating a power cycle
    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    // Expect that no erase cycle is done as AFM stored are exactly the same
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(0));

    // Re-validate full AFM, simulating a power cycle
    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    // Expect that no erase cycle is done as AFM stored are exactly the same
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(0));

    // Re-validate full AFM, simulating a power cycle
    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    // Expect that no erase cycle is done as AFM stored are exactly the same
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(0));
}

TEST_F(AfmAuthenticationTest, test_validate_and_copy_different_but_lesser_new_afm_in_bmc)
{
    // Load signed BMC AFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_BMC_AFM_FILE, FULL_PFR_IMAGE_BMC_AFM_FILE_SIZE);
    alt_u32* afm_addr = get_spi_flash_ptr_with_offset(BMC_STAGING_REGION_ACTIVE_AFM_OFFSET + SIGNATURE_SIZE);

    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    AFM* afm_addr_ptr = (AFM*)afm_addr;
    alt_u32* afm_header = afm_addr_ptr->afm_addr_def;
    AFM_ADDR_DEF* afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    AFM_BODY* afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    alt_u32 afm_size = get_afm_body_size(afm_body);

    alt_u32* afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1);
    alt_u32* afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 2));
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 1));
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    // Expect that for the first time AFM is validated, UFM is not erased as they are empty
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(0));

    // Load signed BMC AFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_BMC_V2P2_SVN2_TWO_AFM_UUID1_UUID2_FILE,
    		FULL_PFR_IMAGE_BMC_V2P2_SVN2_TWO_AFM_UUID1_UUID2_FILE_SIZE);

    // Re-validate full AFM, simulating a power cycle
    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    // Expect an erase cycle
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(1));

    // Validate the new AFM stored in hte UFM
    afm_addr_ptr = (AFM*)afm_addr;
    afm_header = afm_addr_ptr->afm_addr_def;
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1);
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 2));
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    // Re-validate full AFM, simulating a power cycle
    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    // Expect that no "extra" erase cycle is done as AFM stored are exactly the same
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(1));

    // Re-validate full AFM, simulating a power cycle
    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    // Expect that no "extra" erase cycle is done as AFM stored are exactly the same
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(1));
}

TEST_F(AfmAuthenticationTest, test_validate_and_copy_different_but_greater_new_afm_in_bmc)
{
    // Load signed BMC AFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_BMC_V7P7_SVN2_TWO_AFM_FILE,
    		FULL_PFR_IMAGE_BMC_V7P7_SVN2_TWO_AFM_FILE_SIZE);
    alt_u32* afm_addr = get_spi_flash_ptr_with_offset(BMC_STAGING_REGION_ACTIVE_AFM_OFFSET + SIGNATURE_SIZE);

    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    AFM* afm_addr_ptr = (AFM*)afm_addr;
    alt_u32* afm_header = afm_addr_ptr->afm_addr_def;
    AFM_ADDR_DEF* afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    AFM_BODY* afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    alt_u32 afm_size = get_afm_body_size(afm_body);

    alt_u32* afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1);
    alt_u32* afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 2));
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    // Expect that for the first time AFM is validated, UFM is not erased as they are empty
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(0));

    // Load signed BMC AFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_BMC_V7P7_SVN2_THREE_AFM_FILE,
    		FULL_PFR_IMAGE_BMC_V7P7_SVN2_THREE_AFM_FILE_SIZE);

    // Re-validate full AFM, simulating a power cycle
    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    // Expect no erase cycle as there is empty space for incoming new AFM(s) while retaining hte old one
    // as long as the old one completely matches the aFM(s) in the new image
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(0));

    // Validate the new AFM stored in hte UFM
    afm_addr_ptr = (AFM*)afm_addr;
    afm_header = afm_addr_ptr->afm_addr_def;
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1);
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 2));
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 1));
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    // Re-validate full AFM, simulating a power cycle
    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    // Expect that no "extra" erase cycle is done as AFM stored are exactly the same
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(0));

    // Re-validate full AFM, simulating a power cycle
    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    // Expect that no "extra" erase cycle is done as AFM stored are exactly the same
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(0));
}

TEST_F(AfmAuthenticationTest, test_validate_and_copy_different_new_afm_with_similar_content_but_less_one_afm_in_bmc)
{
    // Load signed BMC AFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_BMC_V7P7_SVN2_THREE_AFM_FILE,
    		FULL_PFR_IMAGE_BMC_V7P7_SVN2_THREE_AFM_FILE_SIZE);
    alt_u32* afm_addr = get_spi_flash_ptr_with_offset(BMC_STAGING_REGION_ACTIVE_AFM_OFFSET + SIGNATURE_SIZE);

    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    AFM* afm_addr_ptr = (AFM*)afm_addr;
    alt_u32* afm_header = afm_addr_ptr->afm_addr_def;
    AFM_ADDR_DEF* afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    AFM_BODY* afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    alt_u32 afm_size = get_afm_body_size(afm_body);

    alt_u32* afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1);
    alt_u32* afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 2));
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 1));
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    // Expect that for the first time AFM is validated, UFM is not erased as they are empty
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(0));

    // Load signed BMC AFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_BMC_V7P7_SVN2_TWO_AFM_FILE,
    		FULL_PFR_IMAGE_BMC_V7P7_SVN2_TWO_AFM_FILE_SIZE);

    // Re-validate full AFM, simulating a power cycle
    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    // Expect an erase cycle as the new AFM(s) number has changed eventhough other AFM matches
    // This is becasue PFR must keep up to date with the current active image and not store old AFM.
    // If old AFM is desired, the new active image must contain that old AFM.
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(1));

    // Validate the new AFM stored in hte UFM
    afm_addr_ptr = (AFM*)afm_addr;
    afm_header = afm_addr_ptr->afm_addr_def;
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1);
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 2));
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    // Re-validate full AFM, simulating a power cycle
    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    // Expect that no "extra" erase cycle is done as AFM stored are exactly the same
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(1));

    // Re-validate full AFM, simulating a power cycle
    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    // Expect that no "extra" erase cycle is done as AFM stored are exactly the same
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(1));
}

TEST_F(AfmAuthenticationTest, test_validate_and_copy_different_new_afm_with_similar_content_but_less_two_afm_in_bmc)
{
    // Load signed BMC AFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_BMC_V7P7_SVN2_THREE_AFM_FILE,
    		FULL_PFR_IMAGE_BMC_V7P7_SVN2_THREE_AFM_FILE_SIZE);
    alt_u32* afm_addr = get_spi_flash_ptr_with_offset(BMC_STAGING_REGION_ACTIVE_AFM_OFFSET + SIGNATURE_SIZE);

    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    AFM* afm_addr_ptr = (AFM*)afm_addr;
    alt_u32* afm_header = afm_addr_ptr->afm_addr_def;
    AFM_ADDR_DEF* afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    AFM_BODY* afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    alt_u32 afm_size = get_afm_body_size(afm_body);

    alt_u32* afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1);
    alt_u32* afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 2));
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    afm_header = incr_alt_u32_ptr(afm_header, sizeof(AFM_ADDR_DEF));
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1 + (UFM_CPLD_PAGE_SIZE >> 1));
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    // Expect that for the first time AFM is validated, UFM is not erased as they are empty
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(0));

    // Load signed BMC AFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE,
    		FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE_SIZE);

    // Re-validate full AFM, simulating a power cycle
    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    // Expect an erase cycle as the new AFM(s) number has changed eventhough other AFM matches
    // This is becasue PFR must keep up to date with the current active image and not store old AFM.
    // If old AFM is desired, the new active image must contain that old AFM.
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(1));

    // Validate the new AFM stored in hte UFM
    afm_addr_ptr = (AFM*)afm_addr;
    afm_header = afm_addr_ptr->afm_addr_def;
    afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    afm_body = (AFM_BODY*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr + SIGNATURE_SIZE);

    afm_size = get_afm_body_size(afm_body);

    afm_ufm_ptr = get_ufm_ptr_with_offset(UFM_AFM_1);
    afm_body_ptr = (alt_u32*)afm_body;

    // It is expeced that FW stores the AFM into the empty UFM
    for (alt_u32 i = 0; i < afm_size/4; i++)
    {
        EXPECT_EQ(afm_ufm_ptr[i], afm_body_ptr[i]);
    }

    // Re-validate full AFM, simulating a power cycle
    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    // Expect that no "extra" erase cycle is done as AFM stored are exactly the same
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(1));

    // Re-validate full AFM, simulating a power cycle
    EXPECT_TRUE(is_active_afm_valid((AFM*) afm_addr, 1));

    // Expect that no "extra" erase cycle is done as AFM stored are exactly the same
    EXPECT_EQ(SYSTEM_MOCK::get()->get_ufm_erase_page_counter(), alt_u32(1));
}
