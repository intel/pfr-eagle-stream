#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "testdata_info.h"

class CPLDRecoveryFlowTest : public testing::Test
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
 * This test runs the cpld recovery flow when the backup image is valid
 */
TEST_F(CPLDRecoveryFlowTest, test_cpld_recovery_from_valid_backup)
{
    switch_spi_flash(SPI_FLASH_BMC);

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE, SIGNED_CAPSULE_CPLD_FILE_SIZE,
            + BMC_CPLD_RECOVERY_IMAGE_OFFSET);

    alt_u32* bmc_cpld_update_backup_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC)
    	    	                        + BMC_CPLD_RECOVERY_IMAGE_OFFSET / 4;

    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, perform_cpld_recovery());

    // Check to see if we switched the configuration back to active
    EXPECT_EQ((alt_u32) 0x1, SYSTEM_MOCK::get()->get_mem_word((void*) U_DUAL_CONFIG_BASE));
    EXPECT_EQ((alt_u32) 0x3, SYSTEM_MOCK::get()->get_mem_word((void*) (U_DUAL_CONFIG_BASE  + 4)));

    // Check to see if CFM1 got updated properly
    alt_u32* cpld_update_bitstream = ((CPLD_UPDATE_PC*)(bmc_cpld_update_backup_ptr + SIGNATURE_SIZE/4))->cpld_bitstream;

    alt_u32* cfm1_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);

    for (alt_u32 i = 0; i < UFM_CPLD_ACTIVE_IMAGE_LENGTH/4; i++)
    {
        ASSERT_EQ(cfm1_ptr[i], cpld_update_bitstream[i]);
    }

    // Check UFM sector not touched.
    alt_u32* sector_ptr = get_ufm_ptr_with_offset(U_UFM_DATA_SECTOR2_START_ADDR);
    EXPECT_EQ((alt_u32) 0xffffffff, *sector_ptr);
}


/*
 * This test runs the cpld recovery flow when the backup image is valid starting from recovery main
 */
TEST_F(CPLDRecoveryFlowTest, test_cpld_recovery_from_valid_backup_from_recovery_main)
{
    switch_spi_flash(SPI_FLASH_BMC);
    ut_prep_nios_gpi_signals();
    SYSTEM_MOCK::get()->set_mem_word((void*) (U_DUAL_CONFIG_BASE + (4 << 2)), 0);

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE, SIGNED_CAPSULE_CPLD_FILE_SIZE,
            + BMC_CPLD_RECOVERY_IMAGE_OFFSET);

    alt_u32* bmc_cpld_update_backup_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC)
    	    	                        + BMC_CPLD_RECOVERY_IMAGE_OFFSET / 4;

    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, recovery_main());

    // Check to see if we switched the configuration back to active
    EXPECT_EQ((alt_u32) 0x1, SYSTEM_MOCK::get()->get_mem_word((void*) U_DUAL_CONFIG_BASE));
    EXPECT_EQ((alt_u32) 0x3, SYSTEM_MOCK::get()->get_mem_word((void*) (U_DUAL_CONFIG_BASE  + 4)));

    // Check to see if CFM1 got updated properly
    alt_u32* cpld_update_bitstream = ((CPLD_UPDATE_PC*)(bmc_cpld_update_backup_ptr + SIGNATURE_SIZE/4))->cpld_bitstream;

    alt_u32* cfm1_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);

    for (alt_u32 i = 0; i < UFM_CPLD_ACTIVE_IMAGE_LENGTH/4; i++)
    {
        ASSERT_EQ(cfm1_ptr[i], cpld_update_bitstream[i]);
    }

    // Check UFM sector not touched.
    alt_u32* sector_ptr = get_ufm_ptr_with_offset(U_UFM_DATA_SECTOR2_START_ADDR);
    EXPECT_EQ((alt_u32) 0xffffffff, *sector_ptr);
}


/*
 * This test runs the cpld recovery flow when the backup image is bad but the staging is good starting from recovery main
 */
TEST_F(CPLDRecoveryFlowTest, test_cpld_recovery_from_valid_staging_from_recovery_main)
{
    switch_spi_flash(SPI_FLASH_BMC);
    ut_prep_nios_gpi_signals();
    SYSTEM_MOCK::get()->set_mem_word((void*) (U_DUAL_CONFIG_BASE + (4 << 2)), 0);

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE, SIGNED_CAPSULE_CPLD_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    alt_u32* bmc_cpld_update_backup_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC)
    	    	                        + BMC_CPLD_RECOVERY_IMAGE_OFFSET / 4;


    alt_u32* bmc_cpld_update_staging_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC)
        	    	                        + (get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET) / 4;
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, recovery_main());

    // Check to see if we switched the configuration back to active
    EXPECT_EQ((alt_u32) 0x1, SYSTEM_MOCK::get()->get_mem_word((void*) U_DUAL_CONFIG_BASE));
    EXPECT_EQ((alt_u32) 0x3, SYSTEM_MOCK::get()->get_mem_word((void*) (U_DUAL_CONFIG_BASE  + 4)));

    // Check to see if CFM1 got updated properly
    alt_u32* cpld_update_bitstream = ((CPLD_UPDATE_PC*)(bmc_cpld_update_staging_ptr + SIGNATURE_SIZE/4))->cpld_bitstream;

    alt_u32* cfm1_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);
    for (alt_u32 i = 0; i < UFM_CPLD_ACTIVE_IMAGE_LENGTH/4; i++)
    {
        ASSERT_EQ(cfm1_ptr[i], cpld_update_bitstream[i]);
    }

    // Check to see if the recovery region got updated with the staging update capsule
    for (alt_u32 i = 0; i < EXPECTED_CPLD_UPDATE_CAPSULE_PC_LENGTH/4; i++)
    {
        ASSERT_EQ(bmc_cpld_update_backup_ptr[i], bmc_cpld_update_staging_ptr[i]);
    }

    // Check UFM sector not touched.
    alt_u32* sector_ptr = get_ufm_ptr_with_offset(U_UFM_DATA_SECTOR2_START_ADDR);
    EXPECT_EQ((alt_u32) 0xffffffff, *sector_ptr);
}


/*
 * This test runs the recovery flow when no backups are valid. We expect the system to enter an infinite loop waiting for the BMC to update it
 */
TEST_F(CPLDRecoveryFlowTest, test_cpld_recovery_no_valid_backups)
{
    switch_spi_flash(SPI_FLASH_BMC);
    ut_prep_nios_gpi_signals();
    SYSTEM_MOCK::get()->set_mem_word((void*) (U_DUAL_CONFIG_BASE + (4 << 2)), 0);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Expect that Nios firmware will stuck in the never_exit_loop.
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_FROM_ROM_IMAGE_TERMINAL_STATE);
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, EXPECT_ANY_THROW({ recovery_main(); }));
}


/*
 * This test runs the cpld recovery flow when the backup image is bad but the staging is good starting from recovery main
 */
TEST_F(CPLDRecoveryFlowTest, test_cpld_recovery_from_terminal_state)
{
    /*
     * Test Preparation
     */
    ut_prep_nios_gpi_signals();
    SYSTEM_MOCK::get()->set_mem_word((void*) (U_DUAL_CONFIG_BASE + (4 << 2)), 0);

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE, SIGNED_CAPSULE_CPLD_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_cpld_recovery_capsule_ptr = get_spi_flash_ptr_with_offset(BMC_CPLD_RECOVERY_IMAGE_OFFSET);
    alt_u32 bmc_cpld_update_capsule_addr = get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET;
    alt_u32* bmc_cpld_update_capsule = get_spi_flash_ptr_with_offset(bmc_cpld_update_capsule_addr);

    EXPECT_TRUE(is_signature_valid(bmc_cpld_update_capsule_addr));

    ut_send_in_update_intent_tmin1(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_RECOVERY_MASK);

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Throw after performing CFM switch
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_AFTER_CFM_SWITCH);

    // Run the code
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, EXPECT_ANY_THROW({ boot_bmc_and_wait_for_recovery_update(); }));
    ut_setup_for_recovery_main();
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, EXPECT_ANY_THROW({ recovery_main(); }));
    // After accepting the staged update capsule, switch to CFM0 is called (in boot_bmc_and_wait_for_recovery_update)
    ut_setup_for_recovery_main();
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, EXPECT_ANY_THROW({ recovery_main(); }));

    // Check to see if we switched the configuration back to active
    EXPECT_EQ((alt_u32) 0x1, SYSTEM_MOCK::get()->get_mem_word((void*) U_DUAL_CONFIG_BASE));
    EXPECT_EQ((alt_u32) 0x3, SYSTEM_MOCK::get()->get_mem_word((void*) (U_DUAL_CONFIG_BASE  + 4)));

    // Check to see if CFM1 got updated properly
    alt_u32* cpld_update_bitstream = ((CPLD_UPDATE_PC*)(bmc_cpld_update_capsule + SIGNATURE_SIZE/4))->cpld_bitstream;
    alt_u32* cfm1_ptr = get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET);
    for (alt_u32 i = 0; i < UFM_CPLD_ACTIVE_IMAGE_LENGTH/4; i++)
    {
        ASSERT_EQ(cfm1_ptr[i], cpld_update_bitstream[i]);
    }

    // Check to see if the recovery region got updated with the staging update capsule
    for (alt_u32 i = 0; i < EXPECTED_CPLD_UPDATE_CAPSULE_PC_LENGTH/4; i++)
    {
        ASSERT_EQ(bmc_cpld_recovery_capsule_ptr[i], bmc_cpld_update_capsule[i]);
    }

    // Check UFM sector not touched.
    alt_u32* sector_ptr = get_ufm_ptr_with_offset(U_UFM_DATA_SECTOR2_START_ADDR);
    EXPECT_EQ((alt_u32) 0xffffffff, *sector_ptr);
}

/*
 * This test runs the recovery flow when no backups are valid. We expect the system to enter an infinite loop waiting for the BMC to update it
 */
TEST_F(CPLDRecoveryFlowTest, test_cpld_recovery_with_valid_backups_but_spi_is_tampered)
{
	// Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    ut_prep_nios_gpi_signals();
    SYSTEM_MOCK::get()->set_mem_word((void*) (U_DUAL_CONFIG_BASE + (4 << 2)), 0);

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_CPLD_FILE, SIGNED_CAPSULE_CPLD_FILE_SIZE,
    		get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

    ut_block_spi_access_during_cpld_recovery();

    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    // Expect that Nios firmware will stuck in the never_exit_loop.
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_FROM_ROM_IMAGE_TERMINAL_STATE);
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, EXPECT_ANY_THROW({ recovery_main(); }));
}

