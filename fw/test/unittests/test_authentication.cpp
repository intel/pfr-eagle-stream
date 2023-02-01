#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class AuthenticationTest : public testing::Test
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


TEST_F(AuthenticationTest, test_authenticate_signed_pfm_bmc)
{
    // Load signed BMC PFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_PFM_BMC_FILE, SIGNED_PFM_BMC_FILE_SIZE);
    EXPECT_TRUE(is_signature_valid(0));
}

TEST_F(AuthenticationTest, test_authenticate_signed_pfm_pch)
{
    // Load signed PCH PFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_PFM_PCH_FILE, SIGNED_PFM_PCH_FILE_SIZE);
    EXPECT_TRUE(is_signature_valid(0));
}

TEST_F(AuthenticationTest, test_authenticate_blocksign_signed_binary)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);
    EXPECT_TRUE(is_signature_valid(0));
}

TEST_F(AuthenticationTest, test_authenticate_cpld_update_signed_binary)
{
    // Load signed CPLD capsule to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_CPLD_FILE, SIGNED_CAPSULE_CPLD_FILE_SIZE);
    EXPECT_TRUE(is_signature_valid(0));
}

TEST_F(AuthenticationTest, test_authenticate_signed_key_cancellation_certificate)
{
    // Load signed CPLD capsule to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, KEY_CAN_CERT_PCH_PFM_KEY2, KEY_CAN_CERT_FILE_SIZE);
    EXPECT_TRUE(is_signature_valid(0));
}
