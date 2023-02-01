#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "ut_checks.h"


class CPLDUpdateFlowTest : public testing::Test
{
public:
    alt_u32* m_flash_x86_ptr = nullptr;

    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        sys->reset();
        sys->reset_spi_flash_mock();

        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

        // Reset Nios firmware
        ut_reset_nios_fw();
    }

    virtual void TearDown() {}
};

/*
 * This test runs the portion cpld update flow that happens
 * before switching to the recovery system.
 * when the update capsule is in the bmc flash
 */
TEST_F(CPLDUpdateFlowTest, test_trigger_cpld_update_from_bmc)
{
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, trigger_cpld_update(0));
    EXPECT_EQ(alt_u32(0x1), IORD(U_DUAL_CONFIG_BASE, 0));
    EXPECT_EQ(alt_u32(0x1), IORD(U_DUAL_CONFIG_BASE, 1));
}

/*
 * This test requests 12 multiple CPLD updates through BMC update intent.
 * Since there's no CPLD update capsule present in the flash, authentication should fail.
 */
TEST_F(CPLDUpdateFlowTest, test_multiple_failed_cpld_active_updates_from_bmc)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);

    // Run the flow
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_ALLOWED));
    // Check number of T-1 entries (1 in boot + max failed update attempts)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1 + MAX_FAILED_UPDATE_ATTEMPTS_ALLOWED));
}

/*
 * This test runs the portion cpld update flow that happens
 * before switching to the recovery system.
 * This test makes sure that we don't switch cfm when the authentication fails
 */
TEST_F(CPLDUpdateFlowTest, test_cpld_update_from_bmc_failed_authentication)
{
    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, act_on_bmc_update_intent());

    // Check that no CFM switch has occurred
    EXPECT_EQ(alt_u32(0), IORD(U_DUAL_CONFIG_BASE, 0));
    EXPECT_EQ(alt_u32(0), IORD(U_DUAL_CONFIG_BASE, 1));

    // Check the major/minor error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));
}

/*
 * This test runs the portion cpld update flow that happens
 * after switching to the recovery system.
 */
TEST_F(CPLDUpdateFlowTest, test_cpld_update_post_reconfig)
{
    switch_spi_flash(SPI_FLASH_BMC);

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region
            + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, perform_update_post_reconfig());

    EXPECT_EQ((alt_u32) 0x1, SYSTEM_MOCK::get()->get_mem_word((void*) U_DUAL_CONFIG_BASE));
    EXPECT_EQ((alt_u32) 0x3, SYSTEM_MOCK::get()->get_mem_word((void*) (U_DUAL_CONFIG_BASE  + 4)));

    alt_u32* bmc_cpld_update_capsule_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC)
    			                                        + get_ufm_pfr_data()->bmc_staging_region/4
    			                                        + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET/4;

    // Check to see if CFM1 got updated properly
    alt_u32 pc_size = ((KCH_BLOCK0*) bmc_cpld_update_capsule_ptr)->pc_length;
    alt_u32* cpld_update_bitstream = ((CPLD_UPDATE_PC*)(bmc_cpld_update_capsule_ptr + SIGNATURE_SIZE/4))->cpld_bitstream;
    alt_u32* cfm1_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);
    for (alt_u32 i = 0; i < pc_size/4; i++)
    {
        ASSERT_EQ(cfm1_ptr[i], cpld_update_bitstream[i]);
    }

    // Check UFM sector not touched.
    alt_u32* sector_ptr = get_ufm_ptr_with_offset(U_UFM_DATA_SECTOR2_START_ADDR);
    EXPECT_EQ((alt_u32) 0xffffffff, *sector_ptr);
}

/*
 * This test runs the portion cpld update flow that happens
 * after switching to the recovery system. When authentication fails
 */
TEST_F(CPLDUpdateFlowTest, test_cpld_update_post_reconfig_failed_auth)
{
    switch_spi_flash(SPI_FLASH_BMC);

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region
            + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    alt_u32* bmc_cpld_update_capsule_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC)
    			                                        + get_ufm_pfr_data()->bmc_staging_region/4
    			                                        + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET/4;
    // Corrupt the update capsule
    bmc_cpld_update_capsule_ptr[0] = 0xffffffff;

    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, perform_update_post_reconfig());

    EXPECT_EQ((alt_u32) 0x1, SYSTEM_MOCK::get()->get_mem_word((void*) U_DUAL_CONFIG_BASE));
    EXPECT_EQ((alt_u32) 0x3, SYSTEM_MOCK::get()->get_mem_word((void*) (U_DUAL_CONFIG_BASE  + 4)));

    // Check to see if CFM1 remains blank
    alt_u32 pc_size = ((KCH_BLOCK0*) bmc_cpld_update_capsule_ptr)->pc_length;
    alt_u32* cfm1_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);
    for (alt_u32 i = 1; i < pc_size/4; i++)
    {
        ASSERT_EQ((alt_u32) 0xFFFFFFFF, cfm1_ptr[i]);
    }

    // Check UFM sector not touched.
    alt_u32* sector_ptr = get_ufm_ptr_with_offset(U_UFM_DATA_SECTOR2_START_ADDR);
    EXPECT_EQ((alt_u32) 0xffffffff, *sector_ptr);
}

/*
 * This test tests to see if the main code handles a cpld update intent from the bmc
 */
TEST_F(CPLDUpdateFlowTest, test_cpld_update_bmc_intent_cpld_active_update)
{
    switch_spi_flash(SPI_FLASH_BMC);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region
            + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);

    // Run the flow
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, EXPECT_ANY_THROW({ pfr_main(); }));

    //ASSERT_EQ((alt_u32) 0x1, SYSTEM_MOCK::get()->get_mem_word((void*) U_DUAL_CONFIG_BASE));
    EXPECT_EQ((alt_u32) 0x1, SYSTEM_MOCK::get()->get_mem_word((void*) (U_DUAL_CONFIG_BASE  + 4)));

    // Make sure that the UFM pointer has not been set
    alt_u32* update_status_ptr = get_ufm_ptr_with_offset(UFM_CPLD_UPDATE_STATUS_OFFSET);
    EXPECT_EQ((alt_u32) 0xFFFFFFFF, *update_status_ptr);

    // Check UFM sector not touched.
    alt_u32* sector_ptr = get_ufm_ptr_with_offset(U_UFM_DATA_SECTOR2_START_ADDR);
    EXPECT_EQ((alt_u32) 0xffffffff, *sector_ptr);
}

/*
 * This test tests to see if the main code handles a cpld update intent from the bmc
 */
TEST_F(CPLDUpdateFlowTest, test_cpld_update_bmc_intent_cpld_recovery_update)
{
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 cpld_update_capsule_offset = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE, SIGNED_CAPSULE_CPLD_FILE_SIZE, cpld_update_capsule_offset);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_RECOVERY_MASK);

    // Run the flow
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, EXPECT_ANY_THROW({ pfr_main(); }));

    EXPECT_EQ((alt_u32) 0x1, SYSTEM_MOCK::get()->get_mem_word((void*) (U_DUAL_CONFIG_BASE  + 4)));

    // Make sure that the UFM pointer has not been set
    alt_u32* update_status_ptr = get_ufm_ptr_with_offset(UFM_CPLD_UPDATE_STATUS_OFFSET);
    EXPECT_EQ((alt_u32) 0x0, *update_status_ptr);

    // Check UFM sector not touched.
    alt_u32* sector_ptr = get_ufm_ptr_with_offset(U_UFM_DATA_SECTOR2_START_ADDR);
    EXPECT_EQ((alt_u32) 0xffffffff, *sector_ptr);
}

/*
 * This test tests to see if the main code handles a cpld update intent form the bmc
 * when the update fails authentication
 */
TEST_F(CPLDUpdateFlowTest, test_cpld_update_bmc_intent_negative_test)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_MASK);
    // Run the flow
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    //ASSERT_EQ((alt_u32) 0x0, SYSTEM_MOCK::get()->get_mem_word((void*) U_DUAL_CONFIG_BASE));
    EXPECT_EQ((alt_u32) 0x0, SYSTEM_MOCK::get()->get_mem_word((void*) (U_DUAL_CONFIG_BASE  + 4)));
}

/**
 * @brief This test attempts to send CPLD update intent to PCH update intent register.
 * CPLD should not go into T-1 mode since this in-band CPLD update is not supported.
 */
TEST_F(CPLDUpdateFlowTest, test_cpld_update_pch_intent_negative_test)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_MASK);
    // Run the flow
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Expect Nios fw only entered T-1 mode once during boot up
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    // Expect no BMC-only reset happened
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));

    // Expect update failure
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_UPDATE_INTENT));

    // Expect no panic or recovery event
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
}

/*
 * This test tests the full cpld update flow starting from entry into recovery_main
 */
TEST_F(CPLDUpdateFlowTest, test_cpld_update_post_reconfig_in_recovery_main)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    SYSTEM_MOCK::get()->set_mem_word((void*) (U_DUAL_CONFIG_BASE + (4 << 2)), RECONFIG_REASON_POWER_UP_OR_SWITCH_TO_CFM0 << 13);

    switch_spi_flash(SPI_FLASH_BMC);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region
            + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);
    // recovery_main() looks for the breadcrumb to be 0 to determine if we are in recovery from a power cycle or a reconfig
    alt_u32* bread_crumb_ptr = get_ufm_ptr_with_offset(CFM1_BREAD_CRUMB);
    *bread_crumb_ptr = 0x0;

    // Load a recovery image to CFM0
    alt_u32 *cfm0_image = new alt_u32[CFM0_RECOVERY_IMAGE_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(CFM0_RECOVERY_IMAGE_FILE, cfm0_image);

    alt_u32* cfm0_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ROM_IMAGE_OFFSET);
    for (alt_u32 i = 0; i < (CFM0_RECOVERY_IMAGE_FILE_SIZE/4); i++)
    {
        cfm0_ptr[i] = cfm0_image[i];
    }

    /*
     * Execute flow
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_MEDIUM, recovery_main());

    EXPECT_EQ((alt_u32) 0x1, SYSTEM_MOCK::get()->get_mem_word((void*) U_DUAL_CONFIG_BASE));
    EXPECT_EQ((alt_u32) 0x3, SYSTEM_MOCK::get()->get_mem_word((void*) (U_DUAL_CONFIG_BASE  + 4)));

    alt_u32* bmc_cpld_update_capsule_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC)
    			                                        + get_ufm_pfr_data()->bmc_staging_region/4
    			                                        + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET/4;


    // Check to see if CFM1 got updated properly
    alt_u32* cpld_update_bitstream = ((CPLD_UPDATE_PC*)(bmc_cpld_update_capsule_ptr + SIGNATURE_SIZE/4))->cpld_bitstream;
    alt_u32* cfm1_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);
    for (alt_u32 i = 0; i < (UFM_CPLD_ACTIVE_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm1_ptr[i], cpld_update_bitstream[i]);
    }

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    // Check to see if CFM0 is untouched except the UDS section
    for (alt_u32 i = 0; i < (UFM_CPLD_UNTOUCHED_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm0_ptr[i], cfm0_image[i]);
    }
#else
    // Check to see if CFM0 is untouched
    for (alt_u32 i = 0; i < (UFM_CPLD_ROM_IMAGE_LENGTH/4); i++)
    {
        ASSERT_EQ(cfm0_ptr[i], cfm0_image[i]);
    }
#endif
    /*
     * Clean up
     */
    delete[] cfm0_image;
}

/*
 * This test tests the full cpld recovery update flow after switching back from CFM0
 */
TEST_F(CPLDUpdateFlowTest, test_perform_cpld_recovery_update)
{
    /*
     * Flow preparation
     */
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region
            + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    alt_u32* update_status_ptr = get_ufm_ptr_with_offset(UFM_CPLD_UPDATE_STATUS_OFFSET);
    *update_status_ptr = 0;

    /*
     * Execute flow
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, perform_cpld_recovery_update());

    alt_u32* bmc_cpld_update_capsule_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC)
    			                                        + get_ufm_pfr_data()->bmc_staging_region/4
    			                                        + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET/4;

    alt_u32* bmc_cpld_update_backup_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC)
    	    	                                        + BMC_CPLD_RECOVERY_IMAGE_OFFSET/4;

    // Check to make sure the update got backed up properly
    for (int i = 1; i <SIGNED_CAPSULE_CPLD_FILE_SIZE/4; i++)
    {
        ASSERT_EQ(bmc_cpld_update_capsule_ptr[i], bmc_cpld_update_backup_ptr[i]);
    }

    // Check UFM sector not touched.
    alt_u32* sector_ptr = get_ufm_ptr_with_offset(U_UFM_DATA_SECTOR2_START_ADDR);
    EXPECT_EQ((alt_u32) 0xffffffff, *sector_ptr);

    // Check to see if the SVN got updated
    EXPECT_EQ((alt_u32) 0, get_ufm_svn(UFM_SVN_POLICY_CPLD));
}

/*
 * This test tests the full cpld recovery update flow after switching back from CFM0 starting from pfr_main
 */
TEST_F(CPLDUpdateFlowTest, test_cpld_update_recovery_update_in_pfr_main)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    switch_spi_flash(SPI_FLASH_BMC);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region
            + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    alt_u32* update_status_ptr = get_ufm_ptr_with_offset(UFM_CPLD_UPDATE_STATUS_OFFSET);
    *update_status_ptr = 0;

    /*
     * Execute flow
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    alt_u32* bmc_cpld_update_capsule_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC)
    			                                        + get_ufm_pfr_data()->bmc_staging_region/4
    			                                        + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET/4;

    alt_u32* bmc_cpld_update_backup_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC)
    	    	                                        + BMC_CPLD_RECOVERY_IMAGE_OFFSET/4;

    // Check to make sure the update got backed up properly
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(bmc_cpld_update_capsule_ptr[i], bmc_cpld_update_backup_ptr[i]);
    }

    // Check UFM sector not touched.
    alt_u32* sector_ptr = get_ufm_ptr_with_offset(U_UFM_DATA_SECTOR2_START_ADDR);
    EXPECT_EQ((alt_u32) 0xffffffff, *sector_ptr);

    // Check to see if the SVN got updated
    EXPECT_EQ((alt_u32) 0, get_ufm_svn(UFM_SVN_POLICY_CPLD));
}

/**
 * @brief Test the full CPLD active update flow.
 * The CFM1 has B347.6 CPLD image, while the update capsule has B480.8 CPLD image. 
 * This test checks the change in CFM content.
 */
TEST_F(CPLDUpdateFlowTest, test_cpld_active_update)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load the BMC/PCH image to SPI flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

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
        ASSERT_EQ(cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }

    // Recovery image should be updated (i.e. match the update capsule)
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], expected_cpld_recovery_capsule[i]);
    }

    // Check UFM sector not touched.
    alt_u32* sector_ptr = get_ufm_ptr_with_offset(U_UFM_DATA_SECTOR2_START_ADDR);
    EXPECT_EQ((alt_u32) 0xffffffff, *sector_ptr);

    // Make sure CPLD staging region is writable in T0
    alt_u32 bmc_staging_region_for_cpld_capsule_addr = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + 4));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    // Make sure  CPLD recovery region is read-only in T0
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET));
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    /*
     * Clean up
     */
    delete[] cfm0_image;
    delete[] cfm1_image;
    delete[] expected_cpld_recovery_capsule;
}

/**
 * @brief Test the full CPLD recovery update flow.
 * The CFM1 has B347.6 CPLD image, while the update capsule has B480.8 CPLD image.
 * This test checks the change in CFM content.
 */
TEST_F(CPLDUpdateFlowTest, test_cpld_recovery_update)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Load the BMC/PCH image to SPI flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

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

    // Load active image to CPLD Recovery Region
    alt_u32* cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_FILE, cpld_recovery_capsule_ptr);

    // Load expected image for comparison purpose
    alt_u32* expected_cpld_recovery_capsule = new alt_u32[SIGNED_CAPSULE_CPLD_B490_3_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_CPLD_B490_3_FILE, expected_cpld_recovery_capsule);

    CPLD_UPDATE_PC* expected_cpld_recovery_capsule_pc = (CPLD_UPDATE_PC*) incr_alt_u32_ptr(expected_cpld_recovery_capsule, SIGNATURE_SIZE);
    alt_u32* expected_cpld_active_image = expected_cpld_recovery_capsule_pc->cpld_bitstream;

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_CPLD_B490_3_FILE,
            SIGNED_CAPSULE_CPLD_B490_3_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    /*
     * Flow preparation
     */
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

    // Send a update intent that has both CPLD active and recovery update.
    // CPLD should perform CPLD recovery update, in this case.
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_MASK);
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
        ASSERT_EQ(cfm1_ufm_ptr[i], expected_cpld_active_image[i]);
    }

    // Recovery image should be updated (i.e. match the update capsule)
    for (int i = 1; i < (SIGNED_CAPSULE_CPLD_FILE_SIZE/4); i++)
    {
        ASSERT_EQ(cpld_recovery_capsule_ptr[i], expected_cpld_recovery_capsule[i]);
    }

    // Check UFM sector not touched.
    alt_u32* sector_ptr = get_ufm_ptr_with_offset(U_UFM_DATA_SECTOR2_START_ADDR);
    EXPECT_EQ((alt_u32) 0xffffffff, *sector_ptr);

    // Make sure CPLD staging region is writable in T0 after Nios has completed the update
    alt_u32 bmc_staging_region_for_cpld_capsule_addr = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + 4));
    EXPECT_TRUE(ut_is_page_writable(SPI_FLASH_BMC, bmc_staging_region_for_cpld_capsule_addr + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    // Make sure  CPLD recovery region is read-only in T0
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET));
    EXPECT_FALSE(ut_is_page_writable(SPI_FLASH_BMC, BMC_CPLD_RECOVERY_IMAGE_OFFSET + MAX_CPLD_UPDATE_CAPSULE_SIZE - 4));

    /*
     * Clean up
     */
    delete[] cfm0_image;
    delete[] cfm1_image;
    delete[] expected_cpld_recovery_capsule;
}
