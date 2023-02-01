#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "ut_checks.h"

class AttestationFlowNegTest : public testing::Test
{
public:

    // CFM0/CFM1 binary content used in this test suite
    alt_u32* m_cfm0_image;
    alt_u32* m_cfm1_image;

    // Pointer to start of CFM0/CFM1
    alt_u32* m_cfm0_ufm_ptr;
    alt_u32* m_cfm1_ufm_ptr;

    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        // Reset system mocks and SPI flash
        sys->reset();
        sys->reset_spi_flash_mock();

        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

        // Reset Nios firmware
        ut_reset_nios_fw();

        // Load the entire image to flash
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

        // Initialize CFM0
        m_cfm0_image = new alt_u32[UFM_CPLD_ROM_IMAGE_LENGTH/4];
        SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, m_cfm0_image);
        m_cfm0_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ROM_IMAGE_OFFSET);
        for (alt_u32 i = 0; i < (UFM_CPLD_ROM_IMAGE_LENGTH/4); i++)
        {
            m_cfm0_ufm_ptr[i] = m_cfm0_image[i];
        }

        // Initialize CFM1
        m_cfm1_image = new alt_u32[UFM_CPLD_ACTIVE_IMAGE_LENGTH/4];
        SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM1_ACTIVE_IMAGE_FILE, m_cfm1_image);
        m_cfm1_ufm_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);
        for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
        {
            m_cfm1_ufm_ptr[i] = m_cfm1_image[i];
        }
    }

    virtual void TearDown()
    {
        delete[] m_cfm0_image;
        delete[] m_cfm1_image;
    }
};

/*
 * @brief Execute the recovery flow in the manner that user would normally do.
 */
TEST_F(AttestationFlowNegTest, test_happy_path)
{
    /*
     * Flow preparation
     */
    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    /*
     * Run Nios FW through PFR/Recovery Main
     */
    ut_run_main(CPLD_CFM0, true);

    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify
     */

    // Check to see if CFM0 is untouched except the UDS section
    for (alt_u32 i = 0; i < (UFM_CPLD_UNTOUCHED_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm0_ufm_ptr[i], m_cfm0_image[i]);
    }

    // CPLD Active image should be untouched
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm1_ufm_ptr[i], m_cfm1_image[i]);
    }

    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
}

TEST_F(AttestationFlowNegTest, test_cfm_hash_change_after_corruption_384)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    ut_prep_nios_gpi_signals();

    SYSTEM_MOCK::get()->set_mem_word((void*) (U_DUAL_CONFIG_BASE + (4 << 2)), 0);

    // Load the BMC/PCH image to SPI flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    // Load expected image for comparison purpose
    alt_u32* expected_cpld_recovery_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, expected_cpld_recovery_capsule);

    // Track the cfm1 hash
    alt_u32 cfm1_hash_before_corruption[SHA384_LENGTH/4] = {0};
    alt_u32* cfm1_hash_before_corruption_ptr = get_ufm_ptr_with_offset(UFM_CPLD_CFM1_STORED_HASH);

    // Save the cfm1 hash for checking
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
    	cfm1_hash_before_corruption[i] = cfm1_hash_before_corruption_ptr[i];
    }

    // Ensure cfm1 has is not computed yet
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
    	ASSERT_EQ(cfm1_hash_before_corruption[i], 0xffffffff);
    }

    // Run recovery main
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, recovery_main());

    // Ensure cfm1 hash has been calculated
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
    	ASSERT_NE(cfm1_hash_before_corruption_ptr[i], 0xffffffff);
    }

    // Corrupt the CFM1 hash
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
    	cfm1_hash_before_corruption_ptr[i] = 0xdeadbeef;
    }

    // Run recovery again
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, recovery_main());

    // Cfm1 hash after reconfig
    alt_u32* cfm1_hash_after_reconfig = get_ufm_ptr_with_offset(UFM_CPLD_CFM1_STORED_HASH);

    // Ensure cfm1 hash has been recomputed after corruption
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
    	ASSERT_NE(cfm1_hash_after_reconfig[i], 0xdeadbeef);
    }

    /*
     * Clean up
     */
    delete[] expected_cpld_recovery_capsule;
}

TEST_F(AttestationFlowNegTest, test_pub_key_0_change_after_corruption_384)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    ut_prep_nios_gpi_signals();

    SYSTEM_MOCK::get()->set_mem_word((void*) (U_DUAL_CONFIG_BASE + (4 << 2)), 0);

    // Load the BMC/PCH image to SPI flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    // Load expected image for comparison purpose
    alt_u32* expected_cpld_recovery_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, expected_cpld_recovery_capsule);

    // Track the pubkey 0 cx and cy
    alt_u32 pubkey_0_before_corruption[SHA384_LENGTH/2] = {0};
    alt_u32* pubkey_0_before_corruption_ptr = get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_0);

    // Save them for checking
    for (alt_u32 i = 0; i < (SHA384_LENGTH/2); i++)
    {
    	pubkey_0_before_corruption[i] = pubkey_0_before_corruption_ptr[i];
    }

    // Ensure cx & cy is not computed yet
    for (alt_u32 i = 0; i < (SHA384_LENGTH/2); i++)
    {
    	ASSERT_EQ(pubkey_0_before_corruption[i], 0xffffffff);
    }

    // Run recovery main
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, recovery_main());

    // Ensure cx & cy has been calculated
    for (alt_u32 i = 0; i < (SHA384_LENGTH/2); i++)
    {
    	ASSERT_NE(pubkey_0_before_corruption_ptr[i], 0xffffffff);
    }

    // Corrupt the cx & cy value
    for (alt_u32 i = 0; i < (SHA384_LENGTH/2); i++)
    {
    	pubkey_0_before_corruption_ptr[i] = 0xdeadbeef;
    }

    // Run recovery again
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, recovery_main());

    // cx & cy after reconfig
    alt_u32* pubkey_0_after_reconfig = get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_0);

    // Ensure cfm1 hash has been recomputed after corruption
    for (alt_u32 i = 0; i < (SHA384_LENGTH/2); i++)
    {
    	ASSERT_NE(pubkey_0_after_reconfig[i], 0xdeadbeef);
    }

    /*
     * Clean up
     */
    delete[] expected_cpld_recovery_capsule;
}

TEST_F(AttestationFlowNegTest, test_priv_key_1_change_after_corruption_384)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    ut_prep_nios_gpi_signals();

    SYSTEM_MOCK::get()->set_mem_word((void*) (U_DUAL_CONFIG_BASE + (4 << 2)), 0);

    // Load the BMC/PCH image to SPI flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    // Load expected image for comparison purpose
    alt_u32* expected_cpld_recovery_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, expected_cpld_recovery_capsule);

    // Track the priv key 1
    alt_u32 privkey_1_before_corruption[SHA384_LENGTH/4] = {0};
    alt_u32* privkey_1_before_corruption_ptr = get_ufm_ptr_with_offset(UFM_CPLD_PRIVATE_KEY_1);

    // Save them for checking
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
    	privkey_1_before_corruption[i] = privkey_1_before_corruption_ptr[i];
    }

    // Ensure priv key 1 is not computed yet
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
    	ASSERT_EQ(privkey_1_before_corruption[i], 0xffffffff);
    }

    // Run recovery main
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, recovery_main());

    // Ensure priv key 1 has been calculated
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
    	ASSERT_NE(privkey_1_before_corruption_ptr[i], 0xffffffff);
    }

    // Corrupt the priv key 1 value
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
    	privkey_1_before_corruption_ptr[i] = 0xdeadbeef;
    }

    // Run recovery again
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, recovery_main());

    // Priv key 1 after reconfig
    alt_u32* privkey_1_after_reconfig = get_ufm_ptr_with_offset(UFM_CPLD_PRIVATE_KEY_1);

    // Ensure priv key 1 has been recomputed after corruption
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
    	ASSERT_NE(privkey_1_after_reconfig[i], 0xdeadbeef);
    }

    /*
     * Clean up
     */
    delete[] expected_cpld_recovery_capsule;
}

TEST_F(AttestationFlowNegTest, test_cdi1_change_after_corruption_384)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    ut_prep_nios_gpi_signals();

    SYSTEM_MOCK::get()->set_mem_word((void*) (U_DUAL_CONFIG_BASE + (4 << 2)), 0);

    // Load the BMC/PCH image to SPI flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    // Load expected image for comparison purpose
    alt_u32* expected_cpld_recovery_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, expected_cpld_recovery_capsule);

    // Track the cdi 1
    alt_u32 cdi1_before_corruption[SHA384_LENGTH/4] = {0};
    alt_u32* cdi1_before_corruptionn_ptr = get_ufm_ptr_with_offset(UFM_CPLD_CDI1);

    // Save them for checking
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
    	cdi1_before_corruption[i] = cdi1_before_corruptionn_ptr[i];
    }

    // Ensure cdi 1 is not computed yet
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
    	ASSERT_EQ(cdi1_before_corruption[i], 0xffffffff);
    }

    // Run recovery main
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, recovery_main());

    // Ensure cdi 1 has been calculated
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
    	ASSERT_NE(cdi1_before_corruptionn_ptr[i], 0xffffffff);
    }

    // Corrupt the cdi 1 value
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
    	cdi1_before_corruptionn_ptr[i] = 0xdeadbeef;
    }

    // Run recovery again
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, recovery_main());

    // cdi 1 after reconfig
    alt_u32* cdi1_after_reconfig = get_ufm_ptr_with_offset(UFM_CPLD_CDI1);

    // Ensure cdi 1 has been recomputed after corruption
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
    	ASSERT_NE(cdi1_after_reconfig[i], 0xdeadbeef);
    }

    /*
     * Clean up
     */
    delete[] expected_cpld_recovery_capsule;
}

TEST_F(AttestationFlowNegTest, test_pub_key_1_change_after_corruption_384)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    ut_prep_nios_gpi_signals();

    SYSTEM_MOCK::get()->set_mem_word((void*) (U_DUAL_CONFIG_BASE + (4 << 2)), 0);

    // Load the BMC/PCH image to SPI flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    // Load expected image for comparison purpose
    alt_u32* expected_cpld_recovery_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, expected_cpld_recovery_capsule);

    // Track the pubkey 1 cx and cy
    alt_u32 pubkey_1_before_corruption[SHA384_LENGTH/2] = {0};
    alt_u32* pubkey_1_before_corruption_ptr = get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_1);

    // Save them for checking
    for (alt_u32 i = 0; i < (SHA384_LENGTH/2); i++)
    {
    	pubkey_1_before_corruption[i] = pubkey_1_before_corruption_ptr[i];
    }

    // Ensure cx & cy is not computed yet
    for (alt_u32 i = 0; i < (SHA384_LENGTH/2); i++)
    {
    	ASSERT_EQ(pubkey_1_before_corruption[i], 0xffffffff);
    }

    // Run recovery main
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, recovery_main());

    // Ensure cx & cy has been calculated
    for (alt_u32 i = 0; i < (SHA384_LENGTH/2); i++)
    {
    	ASSERT_NE(pubkey_1_before_corruption_ptr[i], 0xffffffff);
    }

    // Corrupt the cx & cy value
    for (alt_u32 i = 0; i < (SHA384_LENGTH/2); i++)
    {
    	pubkey_1_before_corruption_ptr[i] = 0xdeadbeef;
    }

    // Run recovery again
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, recovery_main());

    // cx & cy after reconfig
    alt_u32* pubkey_1_after_reconfig = get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_1);

    // Ensure cx & cy has been recomputed after corruption
    for (alt_u32 i = 0; i < (SHA384_LENGTH/2); i++)
    {
    	ASSERT_NE(pubkey_1_after_reconfig[i], 0xdeadbeef);
    }

    /*
     * Clean up
     */
    delete[] expected_cpld_recovery_capsule;
}


TEST_F(AttestationFlowNegTest, test_prep_attestation_384_alter_priv_key)
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

    // Alter the key
    ut_alter_key();

    // After altering the key, the private key will be set to higher value than EC N

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

TEST_F(AttestationFlowNegTest, test_generate_uds_with_entropy_error)
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

    // Force health to fail within the crypto mock
    SYSTEM_MOCK::get()->force_mock_entropy_error();

    // Create the UDS
    EXPECT_EQ(generate_and_store_uds(cfm0_uds_ptr), alt_u32(0));

    // Force health to fail within the crypto mock
    SYSTEM_MOCK::get()->force_mock_entropy_error();

    // Use the uds to prepare for attestation
    EXPECT_EQ(prep_for_attestation(cfm0_uds_ptr, CRYPTO_384_MODE), alt_u32(0));

    // Public key 0 should not be stored
    for (alt_u32 i = 0; i < (SHA384_LENGTH/4); i++)
    {
        ASSERT_EQ(pub_key_0_ptr[i], 0xFFFFFFFF);
    }

    // Check to see if CFM0 is untouched including the UDS section
    for (alt_u32 i = 0; i < (UFM_CPLD_ROM_IMAGE_LENGTH/4); i++)
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
