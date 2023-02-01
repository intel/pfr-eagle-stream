#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "testdata_info.h"


class DecompressionFlowTest : public testing::Test
{
public:
    alt_u32* m_flash_x86_ptr = nullptr;

    // For simplicity, use PCH flash for all tests.
    SPI_FLASH_TYPE_ENUM m_spi_flash_in_use = SPI_FLASH_PCH;

    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        sys->reset();

        // Prepare SPI flash
        sys->reset_spi_flash(m_spi_flash_in_use);
        m_flash_x86_ptr = sys->get_x86_ptr_to_spi_flash(m_spi_flash_in_use);
        switch_spi_flash(m_spi_flash_in_use);

        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);
    }

    virtual void TearDown() {}
};

TEST_F(DecompressionFlowTest, test_copy_region_from_capsule_pch)
{
    // Load the signed capsule to recovery region
    alt_u32 recovery_offset = PCH_RECOVERY_REGION_ADDR;
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE, recovery_offset);
    alt_u32* signed_capsule = incr_alt_u32_ptr(m_flash_x86_ptr, recovery_offset);

    // Load the entire image locally
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_image);

    // Make sure that this SPI region was cleared.
    for (alt_u32 word_i = (PCH_SPI_REGION2_START_ADDR >> 2); word_i < (PCH_SPI_REGION2_END_ADDR >> 2); word_i++)
    {
        EXPECT_EQ(m_flash_x86_ptr[word_i], (alt_u32) 0xffffffff);
    }

    // Perform the decompression
    decompress_spi_region_from_capsule(PCH_SPI_REGION2_START_ADDR, PCH_SPI_REGION2_END_ADDR, signed_capsule, 0);

    // Compare against expected data
    for (alt_u32 word_i = (PCH_SPI_REGION2_START_ADDR >> 2); word_i < (PCH_SPI_REGION2_END_ADDR >> 2); word_i++)
    {
        EXPECT_EQ(full_image[word_i], m_flash_x86_ptr[word_i]);
    }

    delete[] full_image;
}

TEST_F(DecompressionFlowTest, test_authentication_after_decompressing_bmc_capsule)
{
    // Load the signed capsule to recovery region
    alt_u32 recovery_offset = BMC_RECOVERY_REGION_ADDR;
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, SIGNED_CAPSULE_BMC_FILE,
            SIGNED_CAPSULE_BMC_FILE_SIZE, recovery_offset);

    // Perform the decompression
    alt_u32* signed_capsule = get_spi_recovery_region_ptr(SPI_FLASH_BMC);
    decompress_capsule(signed_capsule, SPI_FLASH_BMC, DECOMPRESSION_STATIC_AND_DYNAMIC_REGIONS_MASK);

    // Authenticate the active region after decompression
    alt_u32 is_active_valid = is_active_region_valid(SPI_FLASH_BMC);
    EXPECT_TRUE(is_active_valid);
}

TEST_F(DecompressionFlowTest, test_authentication_after_decompressing_pch_capsule)
{
    // Load the signed capsule to recovery region
    alt_u32 recovery_offset = PCH_RECOVERY_REGION_ADDR;
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE, recovery_offset);

    // Perform the decompression
    alt_u32* signed_capsule = get_spi_recovery_region_ptr(SPI_FLASH_PCH);
    decompress_capsule(signed_capsule, SPI_FLASH_PCH, DECOMPRESSION_STATIC_AND_DYNAMIC_REGIONS_MASK);

    // Authenticate the active region after decompression
    EXPECT_TRUE(is_active_region_valid(SPI_FLASH_PCH));
}

#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)

TEST_F(DecompressionFlowTest, test_authentication_after_performing_active_recovery)
{
    // Switch to BMC flash
    switch_spi_flash(SPI_FLASH_BMC);

    // Load the signed capsule to recovery region
    alt_u32 recovery_offset = get_staging_region_offset(SPI_FLASH_BMC) + BMC_STAGING_REGION_AFM_RECOVERY_OFFSET;
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_FILE,
            SIGNED_AFM_CAPSULE_FILE_SIZE, recovery_offset);

    // Expect failure since no AFM image loaded
    EXPECT_FALSE(is_staging_active_afm_valid(SPI_FLASH_BMC));

    // Perform the afm active recovery
    alt_u32* signed_capsule = get_spi_recovery_afm_ptr(SPI_FLASH_BMC);
    perform_afm_active_recovery(signed_capsule);

    // Authenticate the active region after decompression
    EXPECT_TRUE(is_staging_active_afm_valid(SPI_FLASH_BMC));
}

#endif
