#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class FlashValidationTest : public testing::Test
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

TEST_F(FlashValidationTest, test_validate_full_pch_image)
{
    // Load the PCH PFR image to the SPI flash mock
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // Verify the signature and content of the active section PFM
    EXPECT_TRUE(is_active_region_valid(SPI_FLASH_PCH));

    // Verify the signature of the recovery capsule
    EXPECT_TRUE(is_signature_valid(get_recovery_region_offset(SPI_FLASH_PCH)));
}

TEST_F(FlashValidationTest, test_validate_full_bmc_image)
{
    // Load the BMC PFR image to the SPI flash mock
    SYSTEM_MOCK::get()->load_to_flash(m_spi_flash_in_use, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);

    // Verify the signature and content of the active section PFM
    EXPECT_TRUE(is_active_region_valid(SPI_FLASH_BMC));

    // Verify the signature of the recovery capsule
    EXPECT_TRUE(is_signature_valid(get_recovery_region_offset(SPI_FLASH_BMC)));
}
