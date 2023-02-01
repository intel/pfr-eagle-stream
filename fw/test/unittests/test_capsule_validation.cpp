#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class CapsuleValidationTest : public testing::Test
{
public:
    alt_u32* m_flash_x86_ptr = nullptr;

    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        sys->reset();

        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

        // Reset Nios firmware
        ut_reset_nios_fw();

        // Load the entire image to flash
        sys->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
        sys->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);
    }

    virtual void TearDown() {}
};

TEST_F(CapsuleValidationTest, test_sanity)
{
    // Just need the first two signature blocks
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_ptr = get_spi_flash_ptr();
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_FILE, SIGNATURE_SIZE * 2);

    // Check the Block 0 Magic number
    EXPECT_EQ(*bmc_flash_ptr, 0xB6EAFD19);

    alt_u8* m_bmc_flash_x86_u8_ptr = (alt_u8*) bmc_flash_ptr;
    EXPECT_EQ(*m_bmc_flash_x86_u8_ptr, (alt_u8) 0x19);
    EXPECT_EQ(*(m_bmc_flash_x86_u8_ptr + 1), (alt_u8) 0xfd);
    EXPECT_EQ(*(m_bmc_flash_x86_u8_ptr + 2), (alt_u8) 0xea);
    EXPECT_EQ(*(m_bmc_flash_x86_u8_ptr + 3), (alt_u8) 0xb6);

    m_bmc_flash_x86_u8_ptr += 0x400;
    EXPECT_EQ(*m_bmc_flash_x86_u8_ptr, (alt_u8) 0x19);
    EXPECT_EQ(*(m_bmc_flash_x86_u8_ptr + 1), (alt_u8) 0xfd);
    EXPECT_EQ(*(m_bmc_flash_x86_u8_ptr + 2), (alt_u8) 0xea);
    EXPECT_EQ(*(m_bmc_flash_x86_u8_ptr + 3), (alt_u8) 0xb6);
}

TEST_F(CapsuleValidationTest, test_is_pbc_valid)
{
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 signed_capsule_addr = get_ufm_pfr_data()->bmc_staging_region;
    alt_u32* signed_capsule = get_spi_flash_ptr_with_offset(signed_capsule_addr);
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_BMC_FILE,
            SIGNED_CAPSULE_BMC_FILE_SIZE,
            signed_capsule_addr
    );

    PBC_HEADER* pbc = get_pbc_ptr_from_signed_capsule(signed_capsule);
    EXPECT_TRUE(is_pbc_valid(pbc));
}

TEST_F(CapsuleValidationTest, test_is_fw_capsule_valid_with_bmc_cap)
{
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 signed_capsule_addr = get_ufm_pfr_data()->bmc_staging_region;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_BMC_FILE,
            SIGNED_CAPSULE_BMC_FILE_SIZE,
            signed_capsule_addr
    );

    EXPECT_TRUE(is_signature_valid(signed_capsule_addr));
    EXPECT_TRUE(is_fw_capsule_content_valid(signed_capsule_addr, MB_UPDATE_INTENT_BMC_ACTIVE_MASK, 0));
}

TEST_F(CapsuleValidationTest, test_is_fw_capsule_valid_with_pch_cap)
{
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 signed_capsule_addr = get_ufm_pfr_data()->pch_staging_region;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE,
            signed_capsule_addr
    );

    EXPECT_TRUE(is_signature_valid(signed_capsule_addr));
    EXPECT_TRUE(is_fw_capsule_content_valid(signed_capsule_addr, MB_UPDATE_INTENT_PCH_ACTIVE_MASK, 0));
}

TEST_F(CapsuleValidationTest, test_authenticate_pch_fw_capsule_with_bad_tag_in_pbc_header)
{
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 signed_capsule_addr = get_ufm_pfr_data()->pch_staging_region;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_WITH_BAD_TAG_IN_PBC_HEADER_FILE,
            SIGNED_CAPSULE_PCH_WITH_BAD_TAG_IN_PBC_HEADER_FILE_SIZE,
            signed_capsule_addr
    );

    EXPECT_TRUE(is_signature_valid(signed_capsule_addr));
    EXPECT_FALSE(is_fw_capsule_content_valid(signed_capsule_addr, MB_UPDATE_INTENT_PCH_ACTIVE_MASK, 0));
}

TEST_F(CapsuleValidationTest, test_authenticate_pch_fw_capsule_with_bad_version_in_pbc_header)
{
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 signed_capsule_addr = get_ufm_pfr_data()->pch_staging_region;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_WITH_BAD_VERSION_IN_PBC_HEADER_FILE,
            SIGNED_CAPSULE_PCH_WITH_BAD_VERSION_IN_PBC_HEADER_FILE_SIZE,
            signed_capsule_addr
    );

    EXPECT_TRUE(is_signature_valid(signed_capsule_addr));
    EXPECT_FALSE(is_fw_capsule_content_valid(signed_capsule_addr, MB_UPDATE_INTENT_PCH_ACTIVE_MASK, 0));
}

TEST_F(CapsuleValidationTest, test_authenticate_pch_fw_capsule_with_bad_bitmap_nbit_in_pbc_header)
{
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 signed_capsule_addr = get_ufm_pfr_data()->pch_staging_region;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_WITH_BAD_BITMAP_NBIT_IN_PBC_HEADER_FILE,
            SIGNED_CAPSULE_PCH_WITH_BAD_BITMAP_NBIT_IN_PBC_HEADER_FILE_SIZE,
            signed_capsule_addr
    );

    EXPECT_TRUE(is_signature_valid(signed_capsule_addr));
    EXPECT_FALSE(is_fw_capsule_content_valid(signed_capsule_addr, MB_UPDATE_INTENT_PCH_ACTIVE_MASK, 0));
}

TEST_F(CapsuleValidationTest, test_authenticate_pch_fw_capsule_with_bad_page_size_in_pbc_header)
{
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 signed_capsule_addr = get_ufm_pfr_data()->pch_staging_region;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_WITH_BAD_PAGE_SIZE_IN_PBC_HEADER_FILE,
            SIGNED_CAPSULE_PCH_WITH_BAD_PAGE_SIZE_IN_PBC_HEADER_FILE_SIZE,
            signed_capsule_addr
    );

    EXPECT_TRUE(is_signature_valid(signed_capsule_addr));
    EXPECT_FALSE(is_fw_capsule_content_valid(signed_capsule_addr, MB_UPDATE_INTENT_PCH_ACTIVE_MASK, 0));
}

TEST_F(CapsuleValidationTest, test_authenticate_pch_fw_capsule_with_bad_pattern_in_pbc_header)
{
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 signed_capsule_addr = get_ufm_pfr_data()->pch_staging_region;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_WITH_BAD_PATT_IN_PBC_HEADER_FILE,
            SIGNED_CAPSULE_PCH_WITH_BAD_PATT_IN_PBC_HEADER_FILE_SIZE,
            signed_capsule_addr
    );

    EXPECT_TRUE(is_signature_valid(signed_capsule_addr));
    EXPECT_FALSE(is_fw_capsule_content_valid(signed_capsule_addr, MB_UPDATE_INTENT_PCH_ACTIVE_MASK, 0));
}
