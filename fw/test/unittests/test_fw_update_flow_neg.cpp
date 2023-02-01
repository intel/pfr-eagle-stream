#include <iostream>

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "ut_checks.h"


class FWUpdateFlowNegativeTest : public testing::Test
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

/*
 * This test sends the update intent and runs pfr_main.
 * The active update flow is triggered but update process is interrupted with SPI tampering
 */
TEST_F(FWUpdateFlowNegativeTest, test_spi_tampering_during_active_update_pch)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_orig_image);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();
    // Block SPI access during PCH update
    ut_block_spi_access_during_active_update();

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
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Recovery should happen because update process is halted halfway
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_PCH_ACTIVE));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ACTIVE));

    // Check PCH FW version
    // Should be recover to old version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Check for recovered data in static regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Check that the dynamic regions are recovered.
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify recovered PFM
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_PCH));
    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test loads a BMC image with version 0.11 PFM to BMC flash.
 * In BMC staging region, there's an update capsule with version 0.9 PFM.
 *
 * This test sends a BMC active firmware update intent to Nios firmware.
 * The results are compared against a BMC image with version 0.9 PFM.
 * During update, the SPI is interrupted
 */
TEST_F(FWUpdateFlowNegativeTest, test_spi_tampering_during_active_update_bmc)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_orig_image);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V14_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();
    // Block access to BMC flash during update
    ut_block_spi_access_during_active_update();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_ACTIVE));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ACTIVE));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    // Should be the recovered version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // In these BMC images, there are only two read-only area
    // Check for recovered data in static regions
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Check that the dynamic regions are urecovered
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify recovered PFM
    alt_u32 active_pfm_start = get_ufm_pfr_data()->bmc_active_pfm;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_pfm_end = active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_BMC));
    for (alt_u32 word_i = active_pfm_start >> 2; word_i < active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the recovery
    ut_check_all_spi_filtering_rules();

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test sends the BMC recovery update intent and runs pfr_main.
 * The recovery update flow is triggered.
 */
TEST_F(FWUpdateFlowNegativeTest, test_spi_tampering_during_all_recovery_update_bmc_with_backup)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V14_FILE, full_new_image);

    alt_u32 *update_capsule = new alt_u32[SIGNED_CAPSULE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_BMC_V14_FILE, update_capsule);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();
    // Block SPI access during recovery update
    ut_block_spi_access_during_recovery_update();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_RECOVERY_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Verify updated data
     */

    // Nios should transition from T0 to T-1 exactly twice for recovery update flow
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_RECOVERY));

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_RECOVERY));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));

    // Check number of T-1 transitions (recovery update requires platform reset)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Both static and dynamic regions are updated
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Expect corrupted recovered region
    EXPECT_FALSE(is_signature_valid(get_recovery_region_offset(SPI_FLASH_BMC)));

    /*
     * Clean ups
     */
    delete[] full_new_image;
    delete[] update_capsule;
}

/*
 * This test sends the PCH recovery update intent and runs pfr_main.
 * The recovery update flow is triggered.
 */
TEST_F(FWUpdateFlowNegativeTest, test_spi_tampering_during_all_recovery_update_pch_with_backup)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_image);

    alt_u32 *update_capsule = new alt_u32[SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_PCH_V03P12_FILE, update_capsule);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();
    // Block SPI access during recovery update (not active update)
    ut_block_spi_access_during_recovery_update();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_MEDIUM, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly twice for recovery update flow
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_PCH_RECOVERY));

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_RECOVERY));

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

    // Check number of T-1 transitions (recovery update requires platform reset)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Check for updated data in static regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Check that the dynamic regions are untouched.
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Expect corrupted recovered region
    EXPECT_FALSE(is_signature_valid(get_recovery_region_offset(SPI_FLASH_PCH)));

    /*
     * Clean ups
     */
    delete[] full_new_image;
    delete[] update_capsule;
}

/*
 * This test sends the BMC recovery update intent and runs pfr_main.
 * The recovery update flow is triggered.
 */
TEST_F(FWUpdateFlowNegativeTest, test_spi_tampering_during_all_recovery_recovery_bmc_with_backup)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V14_FILE, full_new_image);

    alt_u32 *update_capsule = new alt_u32[SIGNED_CAPSULE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_BMC_V14_FILE, update_capsule);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();
    // Block SPI access during recovery update
    ut_block_spi_access_once_during_recovery_update();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_boot_complete(
            MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_RECOVERY_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Verify updated data
     */

    // Nios should transition from T0 to T-1 exactly twice for recovery update flow
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_RECOVERY));

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_RECOVERY));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(SIGNED_CAPSULE_BMC_V14_FILE_PFM_MINOR_VER));

    // Check number of T-1 transitions (recovery update requires platform reset)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Both static and dynamic regions are updated
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Expect good recovered region
    EXPECT_TRUE(is_signature_valid(get_recovery_region_offset(SPI_FLASH_BMC)));

    // Verify updated PFM
    alt_u32* capsule_pfm = incr_alt_u32_ptr(update_capsule, SIGNATURE_SIZE);
    alt_u32 capsule_pfm_size = get_signed_payload_size(capsule_pfm);
    alt_u32 start_i = get_ufm_pfr_data()->bmc_active_pfm  >> 2;
    alt_u32 end_i = (get_ufm_pfr_data()->bmc_active_pfm + capsule_pfm_size) >> 2;
    for (alt_u32 addr_i = 0; addr_i < (end_i - start_i); addr_i++)
    {
        ASSERT_EQ(capsule_pfm[addr_i], flash_x86_ptr[addr_i + start_i]);
    }

    // Verify updated data in recovery region
    alt_u32 capsule_size = get_signed_payload_size(update_capsule);
    start_i = get_recovery_region_offset(SPI_FLASH_BMC) >> 2;
    end_i = (get_recovery_region_offset(SPI_FLASH_BMC) + capsule_size) >> 2;
    for (alt_u32 addr_i = 0; addr_i < (end_i - start_i); addr_i++)
    {
        ASSERT_EQ(update_capsule[addr_i], flash_x86_ptr[addr_i + start_i]);
    }


    /*
     * Clean ups
     */
    delete[] full_new_image;
    delete[] update_capsule;
}

/*
 * This test sends the PCH recovery update intent and runs pfr_main.
 * The recovery update flow is triggered.
 */
TEST_F(FWUpdateFlowNegativeTest, test_spi_tampering_during_all_recovery_recovery_pch_with_backup)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_image);

    alt_u32 *update_capsule = new alt_u32[SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_CAPSULE_PCH_V03P12_FILE, update_capsule);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();
    // Block SPI access during recovery update
    ut_block_spi_access_once_during_recovery_update();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_MEDIUM, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly twice for recovery update flow
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_PCH_RECOVERY));

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_RECOVERY));

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

    // Check number of T-1 transitions (recovery update requires platform reset)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Check for updated data in static regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Check that the dynamic regions are untouched.
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Expect good recovered region
    //EXPECT_TRUE(is_signature_valid(get_staging_region_offset(SPI_FLASH_PCH)));

    // Verify updated PFM
    alt_u32* capsule_pfm = incr_alt_u32_ptr(update_capsule, SIGNATURE_SIZE);
    alt_u32 capsule_pfm_size = get_signed_payload_size(capsule_pfm);
    alt_u32 start_i = get_ufm_pfr_data()->pch_active_pfm  >> 2;
    alt_u32 end_i = (get_ufm_pfr_data()->pch_active_pfm + capsule_pfm_size) >> 2;
    for (alt_u32 addr_i = 0; addr_i < (end_i - start_i); addr_i++)
    {
        ASSERT_EQ(capsule_pfm[addr_i], flash_x86_ptr[addr_i + start_i]);
    }

    // Verify updated data in recovery region
    alt_u32 capsule_size = get_signed_payload_size(update_capsule);
    start_i = get_recovery_region_offset(SPI_FLASH_PCH) >> 2;
    end_i = (get_recovery_region_offset(SPI_FLASH_PCH) + capsule_size) >> 2;
    for (alt_u32 addr_i = 0; addr_i < (end_i - start_i); addr_i++)
    {
        ASSERT_EQ(update_capsule[addr_i], flash_x86_ptr[addr_i + start_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_new_image;
    delete[] update_capsule;
}

/**
 * @brief This test covers replay attack with no capsule in the SPI flash.
 * This is test case 1. For details about the update scenarios, refer to update_scenarios vector.
 */
TEST_F(FWUpdateFlowNegativeTest, test_replay_attack_with_bad_capsule_case_1)
{
    // Update scenarios
    std::vector<std::pair<MB_REGFILE_OFFSET_ENUM, MB_UPDATE_INTENT_PART1_MASK_ENUM>> update_scenarios = {
            // Nios would enter (full or partial) T-1 for these updates
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            // Nios would reject these update because max failed update attempts are reached
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
    };

    ut_common_routine_for_replay_attack_tests(update_scenarios);
}

/**
 * @brief This test covers replay attack with no capsule in the SPI flash.
 * This is test case 2. For details about the update scenarios, refer to update_scenarios vector.
 */
TEST_F(FWUpdateFlowNegativeTest, test_replay_attack_with_bad_capsule_case_2)
{
    // Update scenarios
    std::vector<std::pair<MB_REGFILE_OFFSET_ENUM, MB_UPDATE_INTENT_PART1_MASK_ENUM>> update_scenarios = {
            // Nios would enter (full or partial) T-1 for these updates
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            // Nios would reject these update because max failed update attempts are reached
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
    };

    ut_common_routine_for_replay_attack_tests(update_scenarios);
}

/**
 * @brief This test covers replay attack with no capsule in the SPI flash.
 * This is test case 3. For details about the update scenarios, refer to update_scenarios vector.
 */
TEST_F(FWUpdateFlowNegativeTest, test_replay_attack_with_bad_capsule_case_3)
{
    // Update scenarios
    std::vector<std::pair<MB_REGFILE_OFFSET_ENUM, MB_UPDATE_INTENT_PART1_MASK_ENUM>> update_scenarios = {
            // Nios would enter (full or partial) T-1 for these updates
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            // Nios would reject these update because max failed update attempts are reached
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
    };

    ut_common_routine_for_replay_attack_tests(update_scenarios);
}

/**
 * @brief This test covers replay attack with no capsule in the SPI flash.
 * This is test case 4. For details about the update scenarios, refer to update_scenarios vector.
 */
TEST_F(FWUpdateFlowNegativeTest, test_replay_attack_with_bad_capsule_case_4)
{
    // Update scenarios
    std::vector<std::pair<MB_REGFILE_OFFSET_ENUM, MB_UPDATE_INTENT_PART1_MASK_ENUM>> update_scenarios = {
            // Nios would enter (full or partial) T-1 for these updates
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
            // Nios would reject these update because max failed update attempts are reached
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
    };

    ut_common_routine_for_replay_attack_tests(update_scenarios);
}

/**
 * @brief This test covers replay attack with no capsule in the SPI flash.
 * This is test case 5. For details about the update scenarios, refer to update_scenarios vector.
 */
TEST_F(FWUpdateFlowNegativeTest, test_replay_attack_with_bad_capsule_case_5)
{
    // Update scenarios
    std::vector<std::pair<MB_REGFILE_OFFSET_ENUM, MB_UPDATE_INTENT_PART1_MASK_ENUM>> update_scenarios = {
            // Nios would enter (full or partial) T-1 for these updates
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_RECOVERY_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_RECOVERY_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_RECOVERY_MASK},
            // Nios would reject these update because max failed update attempts are reached
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
    };

    ut_common_routine_for_replay_attack_tests(update_scenarios);
}

/**
 * @brief This test covers replay attack with no capsule in the SPI flash.
 * This is test case 6. For details about the update scenarios, refer to update_scenarios vector.
 */
TEST_F(FWUpdateFlowNegativeTest, test_replay_attack_with_bad_capsule_case_6)
{
    // Update scenarios
    std::vector<std::pair<MB_REGFILE_OFFSET_ENUM, MB_UPDATE_INTENT_PART1_MASK_ENUM>> update_scenarios = {
            // Nios would enter (full or partial) T-1 for these updates
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_RECOVERY_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_RECOVERY_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_RECOVERY_MASK},
            // Nios would reject these update because max failed update attempts are reached
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_RECOVERY_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_RECOVERY_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK},
    };

    ut_common_routine_for_replay_attack_tests(update_scenarios);
}

/**
 * @brief BMC active update with an update capsule that has a invalid SVN.
 */
TEST_F(FWUpdateFlowNegativeTest, test_bmc_active_update_with_invalid_svn)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_FILE,
            SIGNED_CAPSULE_BMC_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    /*
     * Erase SPI regions and verify the erase for sanity
     */
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_bmc_static_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_bmc_static_regions_end_addr[region_i];

        // Erase this SPI region
        erase_spi_region(start_addr, end_addr - start_addr);

        // Verify that this SPI region is blank now
        for (alt_u32 word_i = start_addr >> 2; word_i < end_addr >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    // Set minimum SVN to higher than the SVN in the update capsule
    write_ufm_svn(UFM_SVN_POLICY_BMC, BMC_UPDATE_CAPSULE_PFM_SVN + 1);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 exactly once for authenticating this capsule
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    // Expect an error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));

    // Expect to see no data in static regions, since update didn't take place
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }
}

/**
 * @brief PCH recovery update with an update capsule which is signed with a cancelled key
 */
TEST_F(FWUpdateFlowNegativeTest, test_pch_recovery_update_with_cancelled_key_in_capsule)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    /*
     * Erase SPI regions and verify the erase
     */
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_pch_static_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_pch_static_regions_end_addr[region_i] - start_addr;
        erase_spi_region(start_addr, end_addr);
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_pch_dynamic_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_pch_dynamic_regions_end_addr[region_i] - start_addr;
        erase_spi_region(start_addr, end_addr);
    }

    // For sanity purpose, verify the erase on some of SPI regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    // Cancel the key that was used to sign this update capsule.
    KCH_SIGNATURE* capsule_signature = (KCH_SIGNATURE*) get_spi_flash_ptr_with_offset(get_ufm_pfr_data()->pch_staging_region);
    cancel_key(KCH_PC_PFR_PCH_UPDATE_CAPSULE, capsule_signature->b1.csk_entry.key_id);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Expect to see no data in static and dynamic regions, since update didn't take place
     */
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }

    // Nios should transition from T0 to T-1 exactly once for authenticating this capsule
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    // Expect an error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));
}

/**
 * @brief Out-of-band PCH active update with an update capsule. The PFM has been signed with a cancelled key.
 */
TEST_F(FWUpdateFlowNegativeTest, test_oob_pch_active_update_with_cancelled_key_in_capsule_pfm)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    /*
     * Erase SPI regions and verify the erase
     */
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_pch_static_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_pch_static_regions_end_addr[region_i] - start_addr;
        erase_spi_region(start_addr, end_addr);
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_pch_dynamic_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_pch_dynamic_regions_end_addr[region_i] - start_addr;
        erase_spi_region(start_addr, end_addr);
    }

    // For sanity purpose, verify the erase on some of SPI regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 10 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // Cancel the key that was used to sign this update capsule.
    alt_u32* signed_capsule = get_spi_flash_ptr_with_offset(
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);
    KCH_SIGNATURE* pfm_signature = (KCH_SIGNATURE*) incr_alt_u32_ptr(signed_capsule, SIGNATURE_SIZE);
    cancel_key(KCH_PC_PFR_PCH_PFM, pfm_signature->b1.csk_entry.key_id);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Expect to see no data in static regions, since update didn't take place
     */
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }
    // Nios should transition from T0 to T-1 exactly once for authenticating this capsule
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    // Expect an error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));
}

/**
 * @brief This test sends CPLD update intent to PCH update intent register. In-band CPLD
 * update is not supported and Nios should posts an error.
 */
TEST_F(FWUpdateFlowNegativeTest, test_invalid_update_intent_from_pch_case1)
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

    // Send an invalid update intent
    ut_send_in_update_intent(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results.
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_UPDATE_INTENT));
}

/**
 * @brief This test sends "BMC Active + Update Dynamic" update intent to PCH update intent register. In-band BMC
 * update is not supported and Nios should posts an error.
 */
TEST_F(FWUpdateFlowNegativeTest, test_invalid_update_intent_from_pch_case2)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent(MB_PCH_UPDATE_INTENT_PART1,
            MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK | MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results.
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_UPDATE_INTENT));
}

/**
 * @brief This test sends UPDATE_DYNAMIC update intent to BMC update intent register.
 * Nios should not act on it, since it's only a qualifier bit.
 */
TEST_F(FWUpdateFlowNegativeTest, test_qualifier_bit_in_bmc_update_intent_case1)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results.
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // No addition T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
}

/**
 * @brief This test sends UPDATE_AT_RESET update intent to BMC update intent register.
 * Nios should not act on it, since it's only a qualifier bit.
 */
TEST_F(FWUpdateFlowNegativeTest, test_qualifier_bit_in_bmc_update_intent_case2)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_UPDATE_AT_RESET_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results.
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // No addition T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
}

/**
 * @brief This test sends UPDATE_DYNAMIC update intent to PCH update intent register.
 * Nios should not act on it, since it's only a qualifier bit.
 */
TEST_F(FWUpdateFlowNegativeTest, test_qualifier_bit_in_pch_update_intent_case1)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_UPDATE_DYNAMIC_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results.
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // No addition T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
}

/**
 * @brief This test sends UPDATE_AT_RESET update intent to PCH update intent register.
 * Nios should not act on it, since it's only a qualifier bit.
 */
TEST_F(FWUpdateFlowNegativeTest, test_qualifier_bit_in_pch_update_intent_case2)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_UPDATE_AT_RESET_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     * Since there's no update capsule in staging region, update would fail.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results.
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // No addition T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
}

/**
 * @brief PCH active update with an update capsule which has bad page size (16KB instead of 4KB) in
 * compression structure header.
 */
TEST_F(FWUpdateFlowNegativeTest, test_pch_active_update_with_capsule_that_has_bad_page_size_in_pbc_header)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the fw update capsule to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_WITH_BAD_PAGE_SIZE_IN_PBC_HEADER_FILE,
            SIGNED_CAPSULE_PCH_WITH_BAD_PAGE_SIZE_IN_PBC_HEADER_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    /*
     * Erase SPI regions and verify the erase
     */
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_pch_static_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_pch_static_regions_end_addr[region_i] - start_addr;
        erase_spi_region(start_addr, end_addr);
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        alt_u32 start_addr = testdata_pch_dynamic_regions_start_addr[region_i];
        alt_u32 end_addr = testdata_pch_dynamic_regions_end_addr[region_i] - start_addr;
        erase_spi_region(start_addr, end_addr);
    }

    // For sanity purpose, verify the erase on some of SPI regions
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Nios should transition from T0 to T-1 exactly once for authenticating this capsule
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));

    // Expect an error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

    /*
     * Expect to see no data in static and dynamic regions, since update didn't take place
     */
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(alt_u32(0xffffffff), flash_x86_ptr[word_i]);
        }
    }
}

/**
 * @brief When PCH recovery image is corrupted, in-band PCH active firmware update should be banned.
 */
TEST_F(FWUpdateFlowNegativeTest, test_banned_inband_pch_active_update_when_recovery_image_is_corrupted)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Corrupt PCH flash
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 signed_recovery_capsule_addr = get_recovery_region_offset(SPI_FLASH_PCH);
    alt_u32* signed_recovery_capsule = get_spi_flash_ptr_with_offset(signed_recovery_capsule_addr);
    *signed_recovery_capsule = 0;

    EXPECT_FALSE(is_signature_valid(signed_recovery_capsule_addr));

    // Send active firwmare update
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check result
     */
    // Expect no recovery because staging region is empty
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));

    // Expect no panic event because active firmware update should be banned.
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));

    // Expect an error message about the banned active update
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_ACTIVE_FW_UPDATE_NOT_ALLOWED));
}

/**
 * @brief When BMC recovery image is corrupted, out-of-band BMC active firmware update should be banned.
 */
TEST_F(FWUpdateFlowNegativeTest, test_banned_oob_bmc_active_update_when_recovery_image_is_corrupted)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Corrupt BMC flash
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 signed_recovery_capsule_addr = get_recovery_region_offset(SPI_FLASH_BMC);
    alt_u32* signed_recovery_capsule = get_spi_flash_ptr_with_offset(signed_recovery_capsule_addr);
    *signed_recovery_capsule = 0;

    EXPECT_FALSE(is_signature_valid(signed_recovery_capsule_addr));

    // Send active firwmare update
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check result
     */
    // Expect no recovery because staging region is empty
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));

    // Expect no panic event because active firmware update should be banned.
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));

    // Expect an error message about the banned active update
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_ACTIVE_FW_UPDATE_NOT_ALLOWED));
}

/**
 * @brief When PCH recovery image is corrupted, out-of-band PCH active firmware update should be banned.
 */
TEST_F(FWUpdateFlowNegativeTest, test_banned_oob_pch_active_update_when_recovery_image_is_corrupted)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Corrupt PCH flash
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 signed_recovery_capsule_addr = get_recovery_region_offset(SPI_FLASH_PCH);
    alt_u32* signed_recovery_capsule = get_spi_flash_ptr_with_offset(signed_recovery_capsule_addr);
    signed_recovery_capsule[3] = 0xDEADBEEF;

    EXPECT_FALSE(is_signature_valid(signed_recovery_capsule_addr));

    // Send active firwmare update
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check result
     */
    // Expect no recovery because staging region is empty
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));

    // Expect no panic event because active firmware update should be banned.
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));

    // Expect an error message about the banned active update
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_ACTIVE_FW_UPDATE_NOT_ALLOWED));
}

/**
 * @brief During the recovery update, ACM/ME/BIOS may not be able to boot after applying update on
 * PCH active firmware. It's expected that the recovery update should be abandoned and a watchdog recovery
 * should be applied on PCH active firmware. This case performs the recovery update through in-band path.
 */
TEST_F(FWUpdateFlowNegativeTest, test_boot_failure_during_ib_pch_recovery_update)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* pch_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the PCH fw update capsule to BMC staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_orig_pch_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_orig_pch_image);
    alt_u32 *full_new_pch_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_pch_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Exit after 1 iteration of the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);
    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Run flow.
     * Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_MEDIUM, pfr_main());

    /*
     * Verify the active update
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    // Both static and dynamic regions are updated
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_pch_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_pch_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }

    /*
     * Run flow.
     * After entering T0, Nios was halted. Clear the ACM/BIOS timer count down value to trigger a watchdog recovery.
     */
    // Clear the count-down value and keep the HW tiemr in active mode.
    IOWR(WDT_ACM_BIOS_TIMER_ADDR, 0, U_TIMER_BANK_TIMER_ACTIVE_MASK);

    // Run the T0 loop again
    perform_t0_operations();

    /*
     * Check platform states and PCH/BMC flash content
     */
    // There should have been a watchdog recovery
    alt_u32 mb_platform_status = read_from_mailbox(MB_PLATFORM_STATE);
    EXPECT_TRUE((mb_platform_status == PLATFORM_STATE_ENTER_T0) || (mb_platform_status == PLATFORM_STATE_T0_ME_BOOTED));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_ACM_BIOS_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_ACM_LAUNCH_FAIL);
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check number of T-1 transitions (recovery update requires platform reset)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Both PCH and BMC should be out-of-reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // Get the staging region offset
    alt_u32 pch_staging_start = get_staging_region_offset(SPI_FLASH_PCH);
    alt_u32 pch_staging_end = pch_staging_start + SPI_FLASH_PCH_STAGING_SIZE;
    alt_u32 pch_staging_start_word_i = pch_staging_start >> 2;
    alt_u32 pch_staging_end_word_i = pch_staging_end >> 2;

    // PCH flash should contain the original image
    for (alt_u32 word_i = 0; word_i < (FULL_PFR_IMAGE_PCH_FILE_SIZE >> 2); word_i++)
    {
        if ((pch_staging_start_word_i <= word_i) && (word_i < pch_staging_end_word_i))
        {
            // Skip the staging region
            continue;
        }
        ASSERT_EQ(full_orig_pch_image[word_i], pch_flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_orig_pch_image;
    delete[] full_new_pch_image;
}

/**
 * @brief During the recovery update, ACM/ME/BIOS may not be able to boot after applying update on
 * PCH active firmware. It's expected that the recovery update should be abandoned and a watchdog recovery
 * should be applied on PCH active firmware. This case performs the recovery update through out-of-band path.
 */
TEST_F(FWUpdateFlowNegativeTest, test_boot_failure_during_oob_pch_recovery_update)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* pch_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_PCH);

    // Load the PCH fw update capsule to BMC staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_orig_pch_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_orig_pch_image);

    alt_u32 *full_new_pch_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_pch_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Exit after 1 iteration of the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);
    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Run flow.
     * Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_MEDIUM, pfr_main());

    /*
     * Verify the active update
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);

    // Both static and dynamic regions are updated
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_pch_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_pch_image[word_i], pch_flash_x86_ptr[word_i]);
        }
    }

    /*
     * Run flow.
     * After entering T0, Nios was halted. Clear the ACM/BIOS timer count down value to trigger a watchdog recovery.
     */
    // Clear the count-down value and keep the HW tiemr in active mode.
    IOWR(WDT_ACM_BIOS_TIMER_ADDR, 0, U_TIMER_BANK_TIMER_ACTIVE_MASK);

    // Run the T0 loop again
    perform_t0_operations();

    /*
     * Check platform states and PCH/BMC flash content
     */
    // There should have been a watchdog recovery
    alt_u32 mb_platform_status = read_from_mailbox(MB_PLATFORM_STATE);
    EXPECT_TRUE((mb_platform_status == PLATFORM_STATE_ENTER_T0) || (mb_platform_status == PLATFORM_STATE_T0_ME_BOOTED));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_ACM_BIOS_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_ACM_LAUNCH_FAIL);
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check number of T-1 transitions (recovery update requires platform reset)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Both PCH and BMC should be out-of-reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // Get the staging region offset
    alt_u32 pch_staging_start = get_staging_region_offset(SPI_FLASH_PCH);
    alt_u32 pch_staging_end = pch_staging_start + SPI_FLASH_PCH_STAGING_SIZE;
    alt_u32 pch_staging_start_word_i = pch_staging_start >> 2;
    alt_u32 pch_staging_end_word_i = pch_staging_end >> 2;

    // PCH flash should contain the original image
    for (alt_u32 word_i = 0; word_i < (FULL_PFR_IMAGE_PCH_FILE_SIZE >> 2); word_i++)
    {
        if ((pch_staging_start_word_i <= word_i) && (word_i < pch_staging_end_word_i))
        {
            // Skip the staging region
            continue;
        }
        if (full_orig_pch_image[word_i] != pch_flash_x86_ptr[word_i])
        {
            std::cout<< "Word ind: "  << word_i<<std::endl;
        }
        ASSERT_EQ(full_orig_pch_image[word_i], pch_flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_orig_pch_image;
    delete[] full_new_pch_image;
}

/**
 * @brief During the BMC recovery update, BMC may not be able to boot after applying update on
 * its active firmware. It's expected that the recovery update should be abandoned and a watchdog recovery
 * should be applied on BMC active firmware.
 */
TEST_F(FWUpdateFlowNegativeTest, test_boot_failure_during_bmc_recovery_update)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* bmc_flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the PCH fw update capsule to BMC staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_CAPSULE_BMC_V14_FILE,
            SIGNED_CAPSULE_BMC_V14_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_orig_bmc_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_orig_bmc_image);

    alt_u32 *full_new_bmc_image = new alt_u32[FULL_PFR_IMAGE_BMC_V14_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V14_FILE, full_new_bmc_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Exit after 1 iteration of the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);
    // Send the recovery update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_RECOVERY_MASK);

    /*
     * Run flow.
     * Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_MEDIUM, pfr_main());

    /*
     * Verify the active update
     */
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);

    // Both static and dynamic regions are updated
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_bmc_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < BMC_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_bmc_image[word_i], bmc_flash_x86_ptr[word_i]);
        }
    }

    /*
     * Run flow.
     * After entering T0, Nios was halted. Clear the BMC timer count down value to trigger a watchdog recovery.
     */
    // Clear the count-down value and keep the HW tiemr in active mode.
    IOWR(WDT_BMC_TIMER_ADDR, 0, U_TIMER_BANK_TIMER_ACTIVE_MASK);

    // Run the T0 loop again
    perform_t0_operations();

    /*
     * Check platform states and PCH/BMC flash content
     */
    // There should have been a watchdog recovery
    alt_u32 mb_platform_status = read_from_mailbox(MB_PLATFORM_STATE);
    EXPECT_TRUE((mb_platform_status == PLATFORM_STATE_ENTER_T0) || (mb_platform_status == PLATFORM_STATE_T0_ME_BOOTED));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_BMC_LAUNCH_FAIL);
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check number of T-1 transitions (recovery update requires platform reset)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    // Both PCH and BMC should be out-of-reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());
    EXPECT_TRUE(ut_is_pch_out_of_reset());

    // Get the staging region offset
    alt_u32 bmc_staging_start = get_staging_region_offset(SPI_FLASH_BMC);
    alt_u32 bmc_staging_end = bmc_staging_start + SPI_FLASH_BMC_STAGING_SIZE;
    alt_u32 bmc_staging_start_word_i = bmc_staging_start >> 2;
    alt_u32 bmc_staging_end_word_i = bmc_staging_end >> 2;

    // BMC flash should contain the original image
    for (alt_u32 word_i = 0; word_i < (FULL_PFR_IMAGE_BMC_FILE_SIZE >> 2); word_i++)
    {
        if ((bmc_staging_start_word_i <= word_i) && (word_i < bmc_staging_end_word_i))
        {
            // Skip the staging region
            continue;
        }
        ASSERT_EQ(full_orig_bmc_image[word_i], bmc_flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_orig_bmc_image;
    delete[] full_new_bmc_image;
}

