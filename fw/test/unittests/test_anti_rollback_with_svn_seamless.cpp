#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "testdata_info.h"


class PFRAntiRollbackWithSVNSeamlessTest : public testing::Test
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
 * @brief This test attempts to update BIOS FV with seamless capsule that has SVN higher
 * than what's on platform (SVN 0). This should be rejected. User can only upgrade SVN in
 * full IFWI update.
 */
TEST_F(PFRAntiRollbackWithSVNSeamlessTest, test_seamless_bios_update_with_new_svn)
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
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

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
TEST_F(PFRAntiRollbackWithSVNSeamlessTest, test_seamless_me_update_with_new_svn)
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
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

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
TEST_F(PFRAntiRollbackWithSVNSeamlessTest, test_seamless_ucode1_update_with_new_svn)
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
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

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
TEST_F(PFRAntiRollbackWithSVNSeamlessTest, test_seamless_ucode2_update_with_new_svn)
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
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

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
 * @brief This test attempts to update PCH Active Image with fw capsule that has SVN higher
 * than what's on platform (SVN 0). This should be rejected. User can only upgrade PFM SVN in
 * full recovery update.
 */
TEST_F(PFRAntiRollbackWithSVNSeamlessTest, test_pch_active_update_with_new_pfm_svn)
{
    /*
     * Test Preparation
     */
    // Load the Seamless FV capsule (with SVN 1)
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_FILE,
            ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_FILE_SIZE,
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

    // Seamless update should not require T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_PCH_UPDATE_INTENT));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));

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

    // Make sure the PCH flash has been updated to the new image
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

    // Make sure the new PFM/FVMs are loaded to PCH flash
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + SIGNED_PFM_MAX_SIZE;
    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        // The rest of Active region should be unchanged
        ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Check SVN policies
     * SVN policies should remain unchanged
     */
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_FVM(0), 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_FVM(1), 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_FVM(2), 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_FVM(3), 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
}

/**
 * @brief This test sends PCH recovery update to bump the SVNs for PFM and FVMs.
 *
 * ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_FILE capsule is used. Please refer to testdata_files_seamless*.h for
 * details regarding SVNs.
 *
 * This test first performs the recovery update to bump all SVNs.
 * Then it verify the resulting SVN policies inside UFM
 * Then it attempts a few updates with cancelled SVNs, which should be rejected.
 * Lastly, it attempt update with valid SVNs, which should be fine.
 */
TEST_F(PFRAntiRollbackWithSVNSeamlessTest, test_bump_all_svns_with_pch_recovery_update)
{
    /*
     * Test Preparation
     */
    // Load the Seamless FV capsule (with SVN 1)
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_FILE,
            ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region
    );

    // Load the entire images locally for comparison purpose
    alt_u32 *full_new_image = new alt_u32[ANTI_ROLLBACK_FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(ANTI_ROLLBACK_FULL_PFR_IMAGE_PCH_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send the Seamless update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

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
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_PCH_UPDATE_INTENT));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

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

    // Make sure the PCH flash has been updated to the new image
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        // Iterate through all words in this SPI region and make sure they match the expected data
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        // Iterate through all words in this SPI region and make sure they match the expected data
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Make sure the new PFM/FVMs are loaded to PCH flash
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + SIGNED_PFM_MAX_SIZE;
    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        // The rest of Active region should be unchanged
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    /*
     * Check SVN policies
     */
    // PFM has SVN 1
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_PCH, 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 1));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 2));
    // FV type 0 has SVN 33
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_FVM(0), 31));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_FVM(0), 32));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_FVM(0), 33));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_FVM(0), 34));
    // FV type 1 has SVN 1
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_FVM(1), 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_FVM(1), 1));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_FVM(1), 2));
    // FV type 2 has SVN 3
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_FVM(2), 2));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_FVM(2), 3));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_FVM(2), 4));
    // FV type 3 has SVN 5
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_FVM(3), 4));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_FVM(3), 5));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_FVM(3), 6));
    // Other SVN policy should be unaffected
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0));

    /*
     * Attempts to update with capsule that has out-dated SVNs.
     *
     * Attempt 1: ME Seamless Capsule with SVN 0
     * Attempt 2: PCH active firmware update with PCH firmware capsule that has SVN 0
     * Attempt 3: PCH recovery firmware update with PCH firmware capsule that has SVN 0
     */
    // Attempt 1: ME Seamless Capsule with SVN 0
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE,
            SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET
    );

    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));

    // Check # failed update attempts
    EXPECT_EQ(ut_get_num_failed_update_attempts(), alt_u32(1));

    // Attempt 2: PCH firmware capsule with SVN 0
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET
    );

    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    mb_update_intent_handler();

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));

    // Check # failed update attempts
    EXPECT_EQ(ut_get_num_failed_update_attempts(), alt_u32(2));

    // Attempt 3: PCH firmware capsule with SVN 0
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_PCH_FILE,
            SIGNED_CAPSULE_PCH_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET
    );

    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    mb_update_intent_handler();

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));

    // Check # failed update attempts
    EXPECT_EQ(ut_get_num_failed_update_attempts(), alt_u32(3));

    /*
     * Lastly, try a seamless update with matching SVN
     */
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_FILE,
            ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region
    );

    write_to_mailbox(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Expecting no error. (Error codes from previous failed update should have been cleared)
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Expecting no failed update attempts (Counts from previous update should have been reset)
    EXPECT_EQ(ut_get_num_failed_update_attempts(), alt_u32(0));

    /*
     * Clean ups
     */
    delete[] full_new_image;
}
