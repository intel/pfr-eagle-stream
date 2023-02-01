#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class AuthenticationNegativeTest : public testing::Test
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
        switch_spi_flash(SPI_FLASH_BMC);
    }

    virtual void TearDown() {}
};


TEST_F(AuthenticationNegativeTest, test_authenticate_signed_can_cert_with_corrupted_payload)
{
    // Load the key cancellation certificate to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_PCH_PFM_KEY2, KEY_CAN_CERT_FILE_SIZE);

    alt_u8* m_flash_ptr = (alt_u8*) get_spi_flash_ptr();
    // Corrupt the key id in protected content of key cancellation certificate
    *(m_flash_ptr + SIGNATURE_SIZE) = 0xff;

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid(0));
}

/**
 * @brief authenticate a key cancellation certificate with invalid key ID (e.g. 255).
 */
TEST_F(AuthenticationNegativeTest, test_authenticate_signed_can_cert_with_bad_key_id)
{
    // Load the key cancellation certificate to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_PCH_PFM_KEY255, KEY_CAN_CERT_FILE_SIZE);
    EXPECT_FALSE(is_signature_valid(0));
}

/**
 * @brief authenticate a key cancellation certificate with invalid PC length.
 */
TEST_F(AuthenticationNegativeTest, test_authenticate_signed_can_cert_with_bad_pc_length)
{
    // Load the key cancellation certificate to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_BAD_PCH_LEGNTH, KEY_CAN_CERT_FILE_SIZE);
    EXPECT_FALSE(is_signature_valid(0));
}

/**
 * @brief authenticate a key cancellation certificate with incorrect reserved area.
 */
TEST_F(AuthenticationNegativeTest, test_authenticate_signed_can_cert_with_modified_reserved_area)
{
    // Load the key cancellation certificate to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_MODIFIED_RESERVED_AREA, KEY_CAN_CERT_FILE_SIZE);
    EXPECT_FALSE(is_signature_valid(0));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_binary_with_bad_block0_magic_number)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

	alt_u32* m_flash_ptr = get_spi_flash_ptr();
	// Corrupt the magic number of Block 0
	*m_flash_ptr = 0xdeadbeef;

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid(0));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_binary_with_bad_block1_magic_number)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

	alt_u32* m_flash_ptr = get_spi_flash_ptr();
	// Corrupt the magic number of Block 1
	*(m_flash_ptr + (BLOCK0_SIZE / 4))= 0xdeadbeef;

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid(0));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_binary_with_bad_block1_root_entry_magic_number)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

	alt_u32* m_flash_ptr = get_spi_flash_ptr();
	// Corrupt the magic number of Root entry in Block 1
	*(m_flash_ptr + ((BLOCK0_SIZE + BLOCK1_HEADER_SIZE)/ 4))= 0xdeadbeef;

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid(0));
}

// This test will depends on the platform supported because in phase 2, both 256-bit and 384-bit will be supported
// Therefore, we might not be able to use CURVE_SECP384R1_MAGIC and treat it as a bad root entry curve magic should the
// the signed_binary_blocksign uses secp384 signature magic. At this stage, it is alright
TEST_F(AuthenticationNegativeTest, test_authenticate_binary_with_bad_block1_root_entry_curve_magic_number)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

	alt_u32* m_flash_ptr = get_spi_flash_ptr();
	// Corrupt the curve magic number of Root entry in Block 1
	*(m_flash_ptr + ((BLOCK0_SIZE + BLOCK1_HEADER_SIZE + 4)/ 4))= 0xbeefdead;

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid(0));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_binary_with_bad_block1_csk_entry_magic_number)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

	alt_u32* m_flash_ptr = get_spi_flash_ptr();
	// Corrupt the magic number of Root entry in Block 1
	*(m_flash_ptr + ((BLOCK0_SIZE + BLOCK1_HEADER_SIZE + BLOCK1_ROOT_ENTRY_SIZE)/ 4))= 0xdeadbeef;

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid(0));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_binary_with_bad_block1_csk_entry_curve_magic_number)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

	alt_u32* m_flash_ptr = get_spi_flash_ptr();
	*(m_flash_ptr + ((BLOCK0_SIZE + BLOCK1_HEADER_SIZE + BLOCK1_ROOT_ENTRY_SIZE + 4)/ 4))= 0xbeefbeef;

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid(0));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_block1_with_modified_block0_after_signing)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

    // Modify an unimportant field in Block0
    KCH_SIGNATURE* sig = (KCH_SIGNATURE*) get_spi_flash_ptr();
    KCH_BLOCK0* b0 = &sig->b0;
    KCH_BLOCK1* b1 = &sig->b1;
    b0->reserved = 0x1;

    EXPECT_FALSE(is_block1_valid(b0, b1, 0, 0));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_binary_with_0_as_pc_length)
{
    // Load a payload, which is signed by Blocksign tool, to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_BINARY_BLOCKSIGN_FILE, SIGNED_BINARY_BLOCKSIGN_FILE_SIZE);

    // Corrupt the signature
    KCH_SIGNATURE* sig = (KCH_SIGNATURE*) get_spi_flash_ptr();
    KCH_BLOCK0* b0 = &sig->b0;
    KCH_BLOCK1* b1 = &sig->b1;
    b0->pc_length = 0;

    EXPECT_FALSE(is_block0_valid(b0, b1, 0));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_fake_cpld_capsule_with_enormous_pc_length)
{
    // Load a CPLD capsule to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE, SIGNED_CAPSULE_CPLD_FILE_SIZE);

    // Corrupt the PC length
    KCH_SIGNATURE* sig = (KCH_SIGNATURE*) get_spi_flash_ptr();
    KCH_BLOCK0* b0 = &sig->b0;
    KCH_BLOCK1* b1 = &sig->b1;

    b0->pc_length = 0xFFFFFFFF;

    EXPECT_FALSE(is_block0_valid(b0, b1, 0));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_fake_pch_pfm_with_enormous_pc_length)
{
    // Load a PCH PFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_PFM_PCH_FILE, SIGNED_PFM_PCH_FILE_SIZE);

    // Corrupt the PC length
    KCH_SIGNATURE* sig = (KCH_SIGNATURE*) get_spi_flash_ptr();
    KCH_BLOCK0* b0 = &sig->b0;
    KCH_BLOCK1* b1 = &sig->b1;
    b0->pc_length = 0xFF000000;

    EXPECT_FALSE(is_block0_valid(b0, b1, 0));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_fake_pch_capsule_with_enormous_pc_length)
{
    // Load a PCH capsule to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_FILE, SIGNED_CAPSULE_PCH_FILE_SIZE);

    // Corrupt the PC length
    KCH_SIGNATURE* sig = (KCH_SIGNATURE*) get_spi_flash_ptr();
    KCH_BLOCK0* b0 = &sig->b0;
    KCH_BLOCK1* b1 = &sig->b1;

    b0->pc_length = 0xFFFFFFFE;

    EXPECT_FALSE(is_block0_valid(b0, b1, 0));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_fake_bmc_pfm_with_enormous_pc_length)
{
    // Load a BMC PFM to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_PFM_BMC_FILE, SIGNED_PFM_BMC_FILE_SIZE);

    // Corrupt the PC length
    KCH_SIGNATURE* sig = (KCH_SIGNATURE*) get_spi_flash_ptr();
    KCH_BLOCK0* b0 = &sig->b0;
    KCH_BLOCK1* b1 = &sig->b1;

    b0->pc_length = 0xAA000000;

    EXPECT_FALSE(is_block0_valid(b0, b1, 0));
}

TEST_F(AuthenticationNegativeTest, test_authenticate_fake_bmc_capsule_with_enormous_pc_length)
{
    // Load a BMC capsule to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_FILE, SIGNED_CAPSULE_BMC_FILE_SIZE);

    // Corrupt the PC length
    KCH_SIGNATURE* sig = (KCH_SIGNATURE*) get_spi_flash_ptr();
    KCH_BLOCK0* b0 = &sig->b0;
    KCH_BLOCK1* b1 = &sig->b1;
    b0->pc_length = 0x08000000;

    EXPECT_FALSE(is_block0_valid(b0, b1, 0));
}

/**
 * @brief Authenticate a PCH firmware update capsule that has 10 as the PC Type value. That
 * is invalid. Expecting to see an authentication failure.
 */
// THIS HAS CHANGE
TEST_F(AuthenticationNegativeTest, test_authenticate_pch_fw_capsule_with_invalid_pc_type)
{
    // Load a PCH capsule to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
			SIGNED_CAPSULE_PCH_WITH_PC_TYPE_10_FILE,
			SIGNED_CAPSULE_PCH_WITH_PC_TYPE_10_FILE_SIZE);

    KCH_SIGNATURE* sig = (KCH_SIGNATURE*) get_spi_flash_ptr();
    KCH_BLOCK0* b0 = &sig->b0;
    KCH_BLOCK1* b1 = &sig->b1;

    EXPECT_EQ(get_kch_pc_type(b0), alt_u32(10));

    // PC Type 10 is invalid
    EXPECT_EQ(get_kch_pc_type(b0), alt_u32(10));
    EXPECT_FALSE(is_block0_valid(b0, b1, 0));
    EXPECT_FALSE(is_signature_valid(0));
}

/**
 * @brief Authenticate a PCH firmware update capsule for which the CSK Key has been cancelled.
 */
TEST_F(AuthenticationNegativeTest, test_authenticate_pch_fw_capsule_with_cancelled_key)
{
    // Load a PCH capsule to SPI flash memory
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_WITH_CSK_ID10_FILE, SIGNED_CAPSULE_PCH_WITH_CSK_ID10_FILE_SIZE);

    // Authentication should pass before key cancellation
    EXPECT_TRUE(is_signature_valid(0));

    // Cancel CSK Key #10 for signing PCH update capsule
    cancel_key(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 10);

    // Authentication should fail after key cancellation
    EXPECT_FALSE(is_signature_valid(0));
}
