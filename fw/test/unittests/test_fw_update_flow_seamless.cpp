#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "ut_checks.h"


class FWUpdateFlowSeamlessTest : public testing::Test
{
public:
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
    }

    virtual void TearDown() {}
};

/**
 * @brief This test sends PCH active firmware update, with a capsule that has some FVM(s) with higher SVN(s).
 * In the current implementation, CPLD allows user to load active PCH image that has FVMs with any valid
 * SVN(s). See are_fw_capsule_fvms_valid() function documentation for more detail.
 *
 * This test uses SET2_SIGNED_CAPSULE_PCH_FILE, which has Ucode1 FVM with SVN 1.
 */
TEST_F(FWUpdateFlowSeamlessTest, test_pch_active_update_with_new_svn_in_some_fvm)
{
    /*
     * Test Preparation
     */
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SET2_SIGNED_CAPSULE_PCH_FILE,
            SET2_SIGNED_CAPSULE_PCH_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET
    );

    // Load the entire images locally for comparison purpose
    alt_u32 *full_new_image = new alt_u32[SET2_FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SET2_FULL_PFR_IMAGE_PCH_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK | MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Verify results
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // OOB PCH update require a a full T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_BMC_UPDATE_INTENT));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(SET2_SIGNED_PFM_PCH_FILE_MAJOR_REV));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(SET2_SIGNED_PFM_PCH_FILE_MINOR_REV));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Check the entire active region
    for (alt_u32 word_i = 0; word_i < (PCH_SPI_FLASH_SIZE >> 2); word_i++)
    {
        // Skip staging region and recovery region
        if (word_i == (get_staging_region_offset(SPI_FLASH_PCH) >> 2))
        {
            word_i += (SPI_FLASH_PCH_STAGING_SIZE >> 2);
        }
        else if (word_i == (get_recovery_region_offset(SPI_FLASH_PCH) >> 2))
        {
            word_i += (SPI_FLASH_PCH_RECOVERY_SIZE >> 2);
        }

        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_new_image;
}

/**
 * @brief This test sends PCH active firmware update, with a capsule that has inconsistency in FV types
 * between some FVM address definition and FVM.
 *
 * It's expected that this issue will be pointed out by CPLD in some way.
 */
TEST_F(FWUpdateFlowSeamlessTest, test_pch_active_update_with_capsule_containing_inconsistent_fv_types)
{
    /*
     * Test Preparation
     */
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_WITH_NOT_MATCHING_FV_TYPES_FILE,
            SIGNED_CAPSULE_PCH_WITH_NOT_MATCHING_FV_TYPES_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region
    );

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_orig_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Verify results
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_PCH_UPDATE_INTENT));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_PCH_ACTIVE));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ACTIVE));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Check the entire active region
    for (alt_u32 word_i = 0; word_i < (PCH_SPI_FLASH_SIZE >> 2); word_i++)
    {
        // Skip staging region and recovery region
        if (word_i == (get_staging_region_offset(SPI_FLASH_PCH) >> 2))
        {
            word_i += (SPI_FLASH_PCH_STAGING_SIZE >> 2);
        }
        else if (word_i == (get_recovery_region_offset(SPI_FLASH_PCH) >> 2))
        {
            word_i += (SPI_FLASH_PCH_RECOVERY_SIZE >> 2);
        }

        ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_orig_image;
}

/**
 * @brief This test sends back-to-back update requests with seamless and non-seamless PCH IFWI images.
 *
 * This checks and makes sure CPLD allows user to switch between the two types of supported IFWI images.
 */
TEST_F(FWUpdateFlowSeamlessTest, test_back_to_back_updates_with_seamless_and_non_seamless_pch_images)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_new_seamless_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_seamless_image);

    alt_u32 *full_new_non_seamless_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_NON_SEAMLESS_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_NON_SEAMLESS_FILE, full_new_non_seamless_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK | MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK);

    /*********************************************************
     *
     * Step one: Issue static + dynamic active firmware update with seamless IFWI image.
     *
     *********************************************************/
    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region
    );

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Verify results
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check the entire active region
    for (alt_u32 word_i = 0; word_i < (PCH_SPI_FLASH_SIZE >> 2); word_i++)
    {
        // Skip staging region and recovery region
        if (word_i == (get_staging_region_offset(SPI_FLASH_PCH) >> 2))
        {
            word_i += (SPI_FLASH_PCH_STAGING_SIZE >> 2);
        }
        else if (word_i == (get_recovery_region_offset(SPI_FLASH_PCH) >> 2))
        {
            word_i += (SPI_FLASH_PCH_RECOVERY_SIZE >> 2);
        }

        ASSERT_EQ(full_new_seamless_image[word_i], flash_x86_ptr[word_i]);
    }

    /*********************************************************
     *
     * Step two: Issue static + dynamic active firmware update with non-seamless IFWI image.
     *
     *********************************************************/
    SYSTEM_MOCK::get()->reset_code_block_counter();

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_V03P12_NON_SEAMLESS_FILE,
            SIGNED_CAPSULE_PCH_V03P12_NON_SEAMLESS_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region
    );

    // Send update intent
    write_to_mailbox(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK | MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK);

    // Run PFR Main. Always run with the timeout
    perform_t0_operations();

    // Verify results
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(2));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check the entire active region
    for (alt_u32 word_i = 0; word_i < (PCH_SPI_FLASH_SIZE >> 2); word_i++)
    {
        // Skip staging region and recovery region
        if (word_i == (get_staging_region_offset(SPI_FLASH_PCH) >> 2))
        {
            word_i += (SPI_FLASH_PCH_STAGING_SIZE >> 2);
        }
        else if (word_i == (get_recovery_region_offset(SPI_FLASH_PCH) >> 2))
        {
            word_i += (SPI_FLASH_PCH_RECOVERY_SIZE >> 2);
        }

        ASSERT_EQ(full_new_non_seamless_image[word_i], flash_x86_ptr[word_i]);
    }

    /*********************************************************
     *
     * Step three: Issue recovery firmware update with seamless IFWI image.
     *
     *********************************************************/
    SYSTEM_MOCK::get()->reset_code_block_counter();

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region
    );

    // Send update intent
    write_to_mailbox(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    // Run PFR Main. Always run with the timeout
    perform_t0_operations();

    // Verify results
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(2));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(4));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check the entire active region
    for (alt_u32 word_i = 0; word_i < (PCH_SPI_FLASH_SIZE >> 2); word_i++)
    {
        // Skip staging region and recovery region
        if (word_i == (get_staging_region_offset(SPI_FLASH_PCH) >> 2))
        {
            word_i += (SPI_FLASH_PCH_STAGING_SIZE >> 2);
        }

        ASSERT_EQ(full_new_seamless_image[word_i], flash_x86_ptr[word_i]);
    }

    /*********************************************************
     *
     * Step four: Issue recovery firmware update with non-seamless IFWI image.
     *
     *********************************************************/
    SYSTEM_MOCK::get()->reset_code_block_counter();

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_V03P12_NON_SEAMLESS_FILE,
            SIGNED_CAPSULE_PCH_V03P12_NON_SEAMLESS_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region
    );

    // Send update intent
    write_to_mailbox(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    // Run PFR Main. Always run with the timeout
    perform_t0_operations();

    // Verify results
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(2));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(5));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(6));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check the entire active region
    for (alt_u32 word_i = 0; word_i < (PCH_SPI_FLASH_SIZE >> 2); word_i++)
    {
        // Skip staging region and recovery region
        if (word_i == (get_staging_region_offset(SPI_FLASH_PCH) >> 2))
        {
            word_i += (SPI_FLASH_PCH_STAGING_SIZE >> 2);
        }

        ASSERT_EQ(full_new_non_seamless_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(SIGNED_CAPSULE_PCH_V03P12_FILE_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    /*
     * Clean ups
     */
    delete[] full_new_seamless_image;
    delete[] full_new_non_seamless_image;
}
