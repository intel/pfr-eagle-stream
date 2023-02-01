#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "testdata_info.h"


class SpiGeoSeamlessUpdateFlowNegativeTest : public testing::Test
{
public:
    const alt_u32 m_bios_fv_regions_start_addr[PCH_NUM_BIOS_FV_REGIONS] = PCH_SPI_BIOS_FV_REGIONS_START_ADDR;
    const alt_u32 m_bios_fv_regions_end_addr[PCH_NUM_BIOS_FV_REGIONS] = PCH_SPI_BIOS_FV_REGIONS_END_ADDR;

    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        // Reset system mocks and SPI flash
        sys->reset();
        sys->reset_spi_flash_mock();

        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_SPI_GEO_EXAMPLE_KEY_FILE);

        // Reset Nios firmware
        ut_reset_nios_fw();

        // Load the entire image to flash
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);
    }

    virtual void TearDown() {}
};

/**
 * @brief This test sends an unprovisioned Seamless update request with a Seamless capsule containing
 * an unknown FV type.
 */
TEST_F(SpiGeoSeamlessUpdateFlowNegativeTest, DISABLED_test_spi_geo_seamless_update_with_unknown_fv_type)
{
    /*
      * Test Preparation
      */
     // Load a seamless capsule with an unknown FV type
     SYSTEM_MOCK::get()->load_to_flash(
             SPI_FLASH_PCH,
             SET2_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_FILE,
             SET2_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_FILE_SIZE,
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

     // Send the Seamless update intent
     // Not using ut_send_in_update_intent_once_upon_boot_complete function here. By setup, it sends in
     //    update intent if the panic event count is 0 and in Seamless update, there's no panic event involved.
     write_to_mailbox(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

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

     // Seamless update should not require T-1 entry
     EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
     EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
     EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

     EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
     EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
     EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
     EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
     EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
     EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_UNKNOWN_FV_TYPE));

     // Check # failed update attempts
     EXPECT_EQ(ut_get_num_failed_update_attempts(), alt_u32(1));

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

     // Make sure no data has been updated
     for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
     {
         // Iterate through all words in this SPI region and make sure they match the expected data
         for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                 word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
         {
             ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
         }
     }
     for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
     {
         // Iterate through all words in this SPI region and make sure they match the expected data
         for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                 word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
         {
             ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
         }
     }

     // Verify that the active PFR region has not changed
     // PFM should not change. Only one FVM will be updated.
     alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
     alt_u32 pch_active_pfm_end = pch_active_pfm_start + SIGNED_PFM_MAX_SIZE;
     for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
     {
         // The rest of Active region should be unchanged
         ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
     }

     /*
      * Clean ups
      */
     delete[] full_orig_image;
}

/**
 * @brief This test attempts to update BIOS FV with seamless capsule that has SVN higher
 * than what's on platform (SVN 0). This should be rejected. User can only upgrade SVN in
 * full IFWI update.
 */
TEST_F(SpiGeoSeamlessUpdateFlowNegativeTest, DISABLED_test_spi_geo_seamless_bios_update_with_new_svn)
{
    /*
     * Test Preparation
     */
    // Load the BIOS Seamless FV capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE,
            ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE_SIZE,
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

    // Send the Seamless update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

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

    // Seamless update should not require T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));

    // Check PCH FW version (should not change in seamless update flow)
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Make sure no data has been updated
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        // Iterate through all words in this SPI region and make sure they match the expected data
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        // Iterate through all words in this SPI region and make sure they match the expected data
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify that the active PFR region has not changed
    // PFM should not change. Only one FVM will be updated.
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + SIGNED_PFM_MAX_SIZE;
    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        // The rest of Active region should be unchanged
        ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_orig_image;
}

/**
 * @brief This test attempts to update ME FV with seamless capsule that has SVN higher
 * than what's on platform (SVN 0). This should be rejected. User can only upgrade SVN in
 * full IFWI update.
 */
TEST_F(SpiGeoSeamlessUpdateFlowNegativeTest, DISABLED_test_spi_geo_seamless_me_update_with_new_svn)
{
    /*
     * Test Preparation
     */
    // Load the ME Seamless FV capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE,
            ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET
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

    // Send the Seamless update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

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

    // Seamless update should not require T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));

    // Check PCH FW version (should not change in seamless update flow)
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Make sure no data has been updated
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        // Iterate through all words in this SPI region and make sure they match the expected data
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        // Iterate through all words in this SPI region and make sure they match the expected data
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify that the active PFR region has not changed
    // PFM should not change. Only one FVM will be updated.
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + SIGNED_PFM_MAX_SIZE;
    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        // The rest of Active region should be unchanged
        ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_orig_image;
}

/**
 * @brief This test attempts to update UCODE1 FV with seamless capsule that has SVN higher
 * than what's on platform (SVN 0). This should be rejected. User can only upgrade SVN in
 * full IFWI update.
 */
TEST_F(SpiGeoSeamlessUpdateFlowNegativeTest, DISABLED_test_spi_geo_seamless_ucode1_update_with_new_svn)
{
    /*
     * Test Preparation
     */
    // Load the UCODE1 Seamless FV capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_FILE,
            ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_FILE_SIZE,
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

    // Send the Seamless update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

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

    // Seamless update should not require T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));

    // Check PCH FW version (should not change in seamless update flow)
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Make sure no data has been updated
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        // Iterate through all words in this SPI region and make sure they match the expected data
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        // Iterate through all words in this SPI region and make sure they match the expected data
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify that the active PFR region has not changed
    // PFM should not change. Only one FVM will be updated.
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + SIGNED_PFM_MAX_SIZE;
    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        // The rest of Active region should be unchanged
        ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_orig_image;
}

/**
 * @brief This test attempts to update UCODE2 FV with seamless capsule that has SVN higher
 * than what's on platform (SVN 0). This should be rejected. User can only upgrade SVN in
 * full IFWI update.
 */
TEST_F(SpiGeoSeamlessUpdateFlowNegativeTest, DISABLED_test_spi_geo_seamless_ucode2_update_with_new_svn)
{
    /*
     * Test Preparation
     */
    // Load the UCODE2 Seamless FV capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_FILE,
            ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET
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

    // Send the Seamless update intent
    ut_send_in_update_intent_once_upon_entry_to_t0(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

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

    // Seamless update should not require T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));

    // Check PCH FW version (should not change in seamless update flow)
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // Make sure no data has been updated
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        // Iterate through all words in this SPI region and make sure they match the expected data
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        // Iterate through all words in this SPI region and make sure they match the expected data
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify that the active PFR region has not changed
    // PFM should not change. Only one FVM will be updated.
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + SIGNED_PFM_MAX_SIZE;
    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        // The rest of Active region should be unchanged
        ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Clean ups
     */
    delete[] full_orig_image;
}

