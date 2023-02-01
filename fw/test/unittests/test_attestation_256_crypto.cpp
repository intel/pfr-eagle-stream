#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "ut_checks.h"

class AttestationFlowTest : public testing::Test
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
TEST_F(AttestationFlowTest, test_happy_path)
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

TEST_F(AttestationFlowTest, test_uds_after_cpld_active_update)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load the BMC/PCH image to SPI flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    // Load expected image for comparison purpose
    alt_u32* expected_cpld_recovery_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, expected_cpld_recovery_capsule);

    // Load the update capsule
    alt_u32 cpld_update_capsule_offset = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_B490_3_FILE,
            SIGNED_CAPSULE_CPLD_B490_3_FILE_SIZE,
            cpld_update_capsule_offset);

    alt_u32* cpld_update_capsule = get_spi_flash_ptr_with_offset(cpld_update_capsule_offset);
    CPLD_UPDATE_PC* staged_cpld_capsule_pc = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(cpld_update_capsule, SIGNATURE_SIZE);

    // Expected CPLD active image after the update
    alt_u32* expected_cpld_active_image = staged_cpld_capsule_pc->cpld_bitstream;

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    /*
     * Run Nios FW through PFR/Recovery Main.
     * Max10 is set up to boot with CFM0 (recovery_main). It will switch to CFM1 shortly after.
     * After receiving CPLD update intent, CPLD will switch from CFM1 to CFM0 to do update there.
     * Once update is complete in CFM0, CPLD will switch back to CFM1.
     */
    ut_run_main(CPLD_CFM0, true);

    // Track the UDS
    alt_u32 uds_storage[16] = {0};
    alt_u32* uds_ptr = get_ufm_ptr_with_offset(UFM_CPLD_UNTOUCHED_ROM_IMAGE_LENGTH);

    // Save the UDS for checking
    for (alt_u32 i = 0; i < 16; i++)
    {
        uds_storage[i] = uds_ptr[i];
    }

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ut_run_main(CPLD_CFM1, true);

    ut_run_main(CPLD_CFM0, true);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify results
     */
    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Active image should be updated
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }

    // Recovery image should be updated (i.e. match the update capsule)
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], expected_cpld_recovery_capsule[i]);
    }

    // Make sure CPLD staging region is writable in T0
    alt_u32 bmc_staging_region_for_cpld_capsule_addr = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + 4));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    // Make sure  CPLD recovery region is read-only in T0
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET));
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    // UDS after reconfig
    alt_u32* uds_ptr_after_reconfig = get_ufm_ptr_with_offset(UFM_CPLD_UNTOUCHED_ROM_IMAGE_LENGTH);

    // Check to see if CFM0 is untouched except the UDS section
    for (alt_u32 i = 0; i < (UFM_CPLD_UNTOUCHED_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm0_ufm_ptr[i], m_cfm0_image[i]);
    }

    // Check if UDS is altered
    for (alt_u32 i = 0; i < 16; i++)
    {
        ASSERT_EQ(uds_storage[i], uds_ptr_after_reconfig[i]);
    }

    /*
     * Clean up
     */
    delete[] expected_cpld_recovery_capsule;
}

TEST_F(AttestationFlowTest, test_cfm_hash_change_after_cpld_active_update_256)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load the BMC/PCH image to SPI flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    // Load expected image for comparison purpose
    alt_u32* expected_cpld_recovery_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, expected_cpld_recovery_capsule);

    // Load the update capsule
    alt_u32 cpld_update_capsule_offset = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_B490_3_FILE,
            SIGNED_CAPSULE_CPLD_B490_3_FILE_SIZE,
            cpld_update_capsule_offset);

    alt_u32* cpld_update_capsule = get_spi_flash_ptr_with_offset(cpld_update_capsule_offset);
    CPLD_UPDATE_PC* staged_cpld_capsule_pc = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(cpld_update_capsule, SIGNATURE_SIZE);

    // Expected CPLD active image after the update
    alt_u32* expected_cpld_active_image = staged_cpld_capsule_pc->cpld_bitstream;

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    /*
     * Run Nios FW through PFR/Recovery Main.
     * Max10 is set up to boot with CFM0 (recovery_main). It will switch to CFM1 shortly after.
     * After receiving CPLD update intent, CPLD will switch from CFM1 to CFM0 to do update there.
     * Once update is complete in CFM0, CPLD will switch back to CFM1.
     */
    ut_run_main(CPLD_CFM0, true);

    // Track the cfm1 hash
    alt_u32 cfm1_hash_before_update[SHA256_LENGTH/4] = {0};
    alt_u32* cfm1_hash_before_update_ptr = get_ufm_ptr_with_offset(UFM_CPLD_CFM1_STORED_HASH);

    // Save the cfm1 hash for checking
    for (alt_u32 i = 0; i < (SHA256_LENGTH/4); i++)
    {
    	cfm1_hash_before_update[i] = cfm1_hash_before_update_ptr[i];
    }

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ut_run_main(CPLD_CFM1, true);

    ut_run_main(CPLD_CFM0, true);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify results
     */
    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Active image should be updated
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }

    // Recovery image should be updated (i.e. match the update capsule)
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], expected_cpld_recovery_capsule[i]);
    }

    // Make sure CPLD staging region is writable in T0
    alt_u32 bmc_staging_region_for_cpld_capsule_addr = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + 4));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    // Make sure  CPLD recovery region is read-only in T0
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET));
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    // Cfm1 hash after reconfig
    alt_u32* cfm1_hash_after_reconfig = get_ufm_ptr_with_offset(UFM_CPLD_CFM1_STORED_HASH);

    // Check to see if CFM0 is untouched except the UDS section
    for (alt_u32 i = 0; i < (UFM_CPLD_UNTOUCHED_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm0_ufm_ptr[i], m_cfm0_image[i]);
    }

    // Check if cfm1 hash has changed
    for (alt_u32 i = 0; i < (SHA256_LENGTH/4); i++)
    {
        ASSERT_NE(cfm1_hash_before_update[i], cfm1_hash_after_reconfig[i]);
    }

    /*
     * Clean up
     */
    delete[] expected_cpld_recovery_capsule;
}

TEST_F(AttestationFlowTest, test_pub_key_0_change_after_cpld_active_update_256)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load the BMC/PCH image to SPI flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    // Load expected image for comparison purpose
    alt_u32* expected_cpld_recovery_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, expected_cpld_recovery_capsule);

    // Load the update capsule
    alt_u32 cpld_update_capsule_offset = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_B490_3_FILE,
            SIGNED_CAPSULE_CPLD_B490_3_FILE_SIZE,
            cpld_update_capsule_offset);

    alt_u32* cpld_update_capsule = get_spi_flash_ptr_with_offset(cpld_update_capsule_offset);
    CPLD_UPDATE_PC* staged_cpld_capsule_pc = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(cpld_update_capsule, SIGNATURE_SIZE);

    // Expected CPLD active image after the update
    alt_u32* expected_cpld_active_image = staged_cpld_capsule_pc->cpld_bitstream;

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    /*
     * Run Nios FW through PFR/Recovery Main.
     * Max10 is set up to boot with CFM0 (recovery_main). It will switch to CFM1 shortly after.
     * After receiving CPLD update intent, CPLD will switch from CFM1 to CFM0 to do update there.
     * Once update is complete in CFM0, CPLD will switch back to CFM1.
     */
    ut_run_main(CPLD_CFM0, true);

    // Track the public key 0
    alt_u32 pubkey_0_before_update[SHA256_LENGTH/2] = {0};
    alt_u32* pubkey_0_before_update_ptr = get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_0);

    // Save the pub key 0 for checking
    for (alt_u32 i = 0; i < (SHA256_LENGTH/2); i++)
    {
    	pubkey_0_before_update[i] = pubkey_0_before_update_ptr[i];
    }

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ut_run_main(CPLD_CFM1, true);

    ut_run_main(CPLD_CFM0, true);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify results
     */
    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Active image should be updated
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }

    // Recovery image should be updated (i.e. match the update capsule)
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], expected_cpld_recovery_capsule[i]);
    }

    // Make sure CPLD staging region is writable in T0
    alt_u32 bmc_staging_region_for_cpld_capsule_addr = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + 4));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    // Make sure  CPLD recovery region is read-only in T0
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET));
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    // Pub key 0 after reconfig
    alt_u32* pubkey_0_after_reconfig = get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_0);

    // Check to see if CFM0 is untouched except the UDS section
    for (alt_u32 i = 0; i < (UFM_CPLD_UNTOUCHED_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm0_ufm_ptr[i], m_cfm0_image[i]);
    }

    // Check if pub key 0 has changed.
    // Note: We expect no change here as the UDS is safely kept
    for (alt_u32 i = 0; i < (SHA256_LENGTH/2); i++)
    {
        ASSERT_EQ(pubkey_0_before_update[i], pubkey_0_after_reconfig[i]);
    }

    /*
     * Clean up
     */
    delete[] expected_cpld_recovery_capsule;
}

TEST_F(AttestationFlowTest, test_priv_key_1_change_after_cpld_active_update_256)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load the BMC/PCH image to SPI flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    // Load expected image for comparison purpose
    alt_u32* expected_cpld_recovery_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, expected_cpld_recovery_capsule);

    // Load the update capsule
    alt_u32 cpld_update_capsule_offset = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_B490_3_FILE,
            SIGNED_CAPSULE_CPLD_B490_3_FILE_SIZE,
            cpld_update_capsule_offset);

    alt_u32* cpld_update_capsule = get_spi_flash_ptr_with_offset(cpld_update_capsule_offset);
    CPLD_UPDATE_PC* staged_cpld_capsule_pc = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(cpld_update_capsule, SIGNATURE_SIZE);

    // Expected CPLD active image after the update
    alt_u32* expected_cpld_active_image = staged_cpld_capsule_pc->cpld_bitstream;

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    /*
     * Run Nios FW through PFR/Recovery Main.
     * Max10 is set up to boot with CFM0 (recovery_main). It will switch to CFM1 shortly after.
     * After receiving CPLD update intent, CPLD will switch from CFM1 to CFM0 to do update there.
     * Once update is complete in CFM0, CPLD will switch back to CFM1.
     */
    ut_run_main(CPLD_CFM0, true);

    // Track the private key 1
    alt_u32 privkey_1_before_update[SHA256_LENGTH/4] = {0};
    alt_u32* privkey_1_before_update_ptr = get_ufm_ptr_with_offset(UFM_CPLD_PRIVATE_KEY_1);

    // Save the pub key 0 for checking
    for (alt_u32 i = 0; i < (SHA256_LENGTH/4); i++)
    {
    	privkey_1_before_update[i] = privkey_1_before_update_ptr[i];
    }

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ut_run_main(CPLD_CFM1, true);

    ut_run_main(CPLD_CFM0, true);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify results
     */
    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Active image should be updated
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }

    // Recovery image should be updated (i.e. match the update capsule)
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], expected_cpld_recovery_capsule[i]);
    }

    // Make sure CPLD staging region is writable in T0
    alt_u32 bmc_staging_region_for_cpld_capsule_addr = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + 4));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    // Make sure  CPLD recovery region is read-only in T0
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET));
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    // Pub key 0 after reconfig
    alt_u32* privkey_1_after_reconfig = get_ufm_ptr_with_offset(UFM_CPLD_PRIVATE_KEY_1);

    // Check to see if CFM0 is untouched except the UDS section
    for (alt_u32 i = 0; i < (UFM_CPLD_UNTOUCHED_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm0_ufm_ptr[i], m_cfm0_image[i]);
    }

    // Check if cfm1 hash has changed
    for (alt_u32 i = 0; i < (SHA256_LENGTH/4); i++)
    {
        ASSERT_NE(privkey_1_before_update[i], privkey_1_after_reconfig[i]);
    }

    /*
     * Clean up
     */
    delete[] expected_cpld_recovery_capsule;
}

TEST_F(AttestationFlowTest, test_cdi_1_change_after_cpld_active_update_256)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load the BMC/PCH image to SPI flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    // Load expected image for comparison purpose
    alt_u32* expected_cpld_recovery_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, expected_cpld_recovery_capsule);

    // Load the update capsule
    alt_u32 cpld_update_capsule_offset = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_B490_3_FILE,
            SIGNED_CAPSULE_CPLD_B490_3_FILE_SIZE,
            cpld_update_capsule_offset);

    alt_u32* cpld_update_capsule = get_spi_flash_ptr_with_offset(cpld_update_capsule_offset);
    CPLD_UPDATE_PC* staged_cpld_capsule_pc = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(cpld_update_capsule, SIGNATURE_SIZE);

    // Expected CPLD active image after the update
    alt_u32* expected_cpld_active_image = staged_cpld_capsule_pc->cpld_bitstream;

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    /*
     * Run Nios FW through PFR/Recovery Main.
     * Max10 is set up to boot with CFM0 (recovery_main). It will switch to CFM1 shortly after.
     * After receiving CPLD update intent, CPLD will switch from CFM1 to CFM0 to do update there.
     * Once update is complete in CFM0, CPLD will switch back to CFM1.
     */
    ut_run_main(CPLD_CFM0, true);

    // Track the cdi 1
    alt_u32 cdi_1_before_update[SHA256_LENGTH/4] = {0};
    alt_u32* cdi_1_before_update_ptr = get_ufm_ptr_with_offset(UFM_CPLD_CDI1);

    // Save the cdi 1 for checking
    for (alt_u32 i = 0; i < (SHA256_LENGTH/4); i++)
    {
    	cdi_1_before_update[i] = cdi_1_before_update_ptr[i];
    }

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ut_run_main(CPLD_CFM1, true);

    ut_run_main(CPLD_CFM0, true);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify results
     */
    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Active image should be updated
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }

    // Recovery image should be updated (i.e. match the update capsule)
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], expected_cpld_recovery_capsule[i]);
    }

    // Make sure CPLD staging region is writable in T0
    alt_u32 bmc_staging_region_for_cpld_capsule_addr = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + 4));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    // Make sure  CPLD recovery region is read-only in T0
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET));
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    // cdi 1 after reconfig
    alt_u32* cdi_1_after_reconfig = get_ufm_ptr_with_offset(UFM_CPLD_CDI1);

    // Check to see if CFM0 is untouched except the UDS section
    for (alt_u32 i = 0; i < (UFM_CPLD_UNTOUCHED_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm0_ufm_ptr[i], m_cfm0_image[i]);
    }

    // Check if cfm1 hash has changed
    for (alt_u32 i = 0; i < (SHA256_LENGTH/4); i++)
    {
        ASSERT_NE(cdi_1_before_update[i], cdi_1_after_reconfig[i]);
    }

    /*
     * Clean up
     */
    delete[] expected_cpld_recovery_capsule;
}

TEST_F(AttestationFlowTest, test_pubkey_1_change_after_cpld_active_update_256)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load the BMC/PCH image to SPI flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    // Load expected image for comparison purpose
    alt_u32* expected_cpld_recovery_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, expected_cpld_recovery_capsule);

    // Load the update capsule
    alt_u32 cpld_update_capsule_offset = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_B490_3_FILE,
            SIGNED_CAPSULE_CPLD_B490_3_FILE_SIZE,
            cpld_update_capsule_offset);

    alt_u32* cpld_update_capsule = get_spi_flash_ptr_with_offset(cpld_update_capsule_offset);
    CPLD_UPDATE_PC* staged_cpld_capsule_pc = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(cpld_update_capsule, SIGNATURE_SIZE);

    // Expected CPLD active image after the update
    alt_u32* expected_cpld_active_image = staged_cpld_capsule_pc->cpld_bitstream;

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    /*
     * Run Nios FW through PFR/Recovery Main.
     * Max10 is set up to boot with CFM0 (recovery_main). It will switch to CFM1 shortly after.
     * After receiving CPLD update intent, CPLD will switch from CFM1 to CFM0 to do update there.
     * Once update is complete in CFM0, CPLD will switch back to CFM1.
     */
    ut_run_main(CPLD_CFM0, true);

    // Track the pubkey 1
    alt_u32 pubkey_1_before_update[SHA256_LENGTH/2] = {0};
    alt_u32* pubkey_1_before_update_ptr = get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_1);

    // Save the pubkey 1 for checking
    for (alt_u32 i = 0; i < (SHA256_LENGTH/2); i++)
    {
    	pubkey_1_before_update[i] = pubkey_1_before_update_ptr[i];
    }

    // Send the active update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ut_run_main(CPLD_CFM1, true);

    ut_run_main(CPLD_CFM0, true);
    ut_run_main(CPLD_CFM1, false);

    /*
     * Verify results
     */
    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Active image should be updated
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }

    // Recovery image should be updated (i.e. match the update capsule)
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], expected_cpld_recovery_capsule[i]);
    }

    // Make sure CPLD staging region is writable in T0
    alt_u32 bmc_staging_region_for_cpld_capsule_addr = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + 4));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    // Make sure  CPLD recovery region is read-only in T0
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET));
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    // pubkey 1 after reconfig
    alt_u32* pubkey_1_after_reconfig = get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_1);

    // Check to see if CFM0 is untouched except the UDS section
    for (alt_u32 i = 0; i < (UFM_CPLD_UNTOUCHED_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(m_cfm0_ufm_ptr[i], m_cfm0_image[i]);
    }

    // Check if pubkey 1 has changed
    for (alt_u32 i = 0; i < (SHA256_LENGTH/2); i++)
    {
        ASSERT_NE(pubkey_1_before_update[i], pubkey_1_after_reconfig[i]);
    }

    /*
     * Clean up
     */
    delete[] expected_cpld_recovery_capsule;
}
