#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "ut_checks.h"


class AFMUpdateFlowNegTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        // Reset system mocks and SPI flash
        sys->reset();
        sys->reset_spi_flash_mock();

        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);

        // Reset Nios firmware
        ut_reset_nios_fw();

        // Load the entire image to flash
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_AFM_SVN_2_FILE, FULL_PFR_IMAGE_BMC_AFM_SVN_2_FILE_SIZE);
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    }

    virtual void TearDown() {}
};

/*
 * This test loads a AFM image with SVN 2 to BMC flash.
 * In BMC staging region, there's an update capsule with SVN 4.
 *
 * This test sends a BMC AFM active update intent to Nios firmware.
 * The results are compared against a AFM image with version 0.9 PFM.
 */
TEST_F(AFMUpdateFlowNegTest, test_active_update_afm_invalid_svn)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_FILE,
    		SIGNED_AFM_CAPSULE_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_AFM_SVN_2_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_AFM_SVN_2_FILE, full_orig_image);

    // Image should not be updated as invalid SVN
    // Use this for comparison purposes
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_AFM_SVN_2_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_AFM_SVN_2_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(1));

    // Verify that AFM is not updated due to invalid SVN
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check AFM SVN(s)
    // Ensure that old SVN is not updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(2));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(2));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(2));

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test loads a AFM image with SVN 2 to BMC flash.
 * In BMC staging region, there's an update capsule with SVN 2.
 *
 * One of the AFM is unknown uuid
 */
TEST_F(AFMUpdateFlowNegTest, test_active_update_complete_afm_unknown_uuid)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE,
    		SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_orig_image);

    // Image should not be updated as invalid SVN
    // Use this for comparison purposes
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_UNKNOWN_AFM));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Verify that AFM is not updated due to invalid SVN
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    // Ensure that old SVN is not updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test loads a AFM image with SVN 2 to BMC flash.
 * In BMC staging region, there's an update capsule with SVN 2 SVN but different content.
 *
 * This test sends a BMC AFM active update intent to Nios firmware.
 * The results are compared against a AFM image with version 0.9 PFM.
 */
TEST_F(AFMUpdateFlowNegTest, DISABLED_test_active_update_complete_afm_tampered_spi)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    // Always do this first for specific test case
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN2_THREE_AFM_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN2_THREE_AFM_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V5P5_SVN_2_THREE_AFM_FILE,
    		SIGNED_AFM_CAPSULE_V5P5_SVN_2_THREE_AFM_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN2_THREE_AFM_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN2_THREE_AFM_FILE, full_orig_image);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V5P5_SVN2_THREE_AFM_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V5P5_SVN2_THREE_AFM_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();
    // Block spi access during afm active update
    ut_block_spi_access_during_active_update();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_ACTIVE));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_ACTIVE));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));
    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Verify that active AFM is recovered
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(9));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(10));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(11));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test loads a AFM image with SVN 2 to BMC flash.
 * In BMC staging region, there's an update capsule with SVN 2.
 *
 * One AFM is unknown uuid
 */
TEST_F(AFMUpdateFlowNegTest, DISABLED_test_active_update_single_afm_tampered_spi)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_4_ONE_AFM_UUID9_FILE,
    		SIGNED_AFM_CAPSULE_V2P2_SVN_4_ONE_AFM_UUID9_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_orig_image);

    // Image should not be updated as invalid SVN
    // Use this for comparison purposes
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();
    // Block spi access during afm active update
    ut_block_spi_access_during_active_update();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_ACTIVE));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_ACTIVE));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Verify that AFM is not updated due to invalid SVN
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    // Ensure that old SVN is not updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test loads a AFM image with SVN 2 to BMC flash.
 * In BMC staging region, there's an update capsule with SVN 2 but different content.
 *
 * Two of the AFM is of different version
 */
TEST_F(AFMUpdateFlowNegTest, DISABLED_test_spi_tampered_during_recovery_update_full_afm)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    // Always do this first for specific test case
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN2_THREE_AFM_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN2_THREE_AFM_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V6P6_SVN_2_THREE_AFM_FILE,
    		SIGNED_AFM_CAPSULE_V6P6_SVN_2_THREE_AFM_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN2_THREE_AFM_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN2_THREE_AFM_FILE, full_orig_image);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V6P6_SVN2_THREE_AFM_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V6P6_SVN2_THREE_AFM_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();
    ut_block_spi_access_once_during_afm_recovery_update();
    //ut_block_spi_access_during_recovery_update();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_RECOVERY));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_RECOVERY));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));
    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(6));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(6));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(6));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(6));

    // Verify that active AFM is updated
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Verify that recovery AFM is recovered
    alt_u32 recovery_afm_start = BMC_STAGING_REGION_RECOVERY_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 recovery_afm_end = recovery_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = recovery_afm_start >> 2; word_i < recovery_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(9));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(10));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(11));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test loads a AFM image with SVN 2 to BMC flash.
 * In BMC staging region, there's an update capsule with SVN 2.
 *
 * One AFM is unknown uuid
 */
TEST_F(AFMUpdateFlowNegTest, test_active_update_single_afm_unknown_uuid)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_4_ONE_AFM_UUID9_FILE,
    		SIGNED_AFM_CAPSULE_V2P2_SVN_4_ONE_AFM_UUID9_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_orig_image);

    // Image should not be updated as invalid SVN
    // Use this for comparison purposes
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_UNKNOWN_AFM));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Verify that AFM is not updated due to invalid SVN
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    // Ensure that old SVN is not updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test loads a AFM image with SVN 2 to BMC flash.
 * In BMC staging region, there's an update capsule with SVN 2.
 *
 * Single recovery update is not allowed
 */
TEST_F(AFMUpdateFlowNegTest, test_recovery_update_single_afm)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_4_ONE_AFM_UUID1_FILE,
    		SIGNED_AFM_CAPSULE_V2P2_SVN_4_ONE_AFM_UUID1_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_orig_image);

    // Image should not be updated as invalid SVN
    // Use this for comparison purposes
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_UNKNOWN_AFM));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Verify that AFM is not updated due to invalid SVN
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    // Ensure that old SVN is not updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test loads a AFM image with SVN 7 to BMC flash.
 * In BMC staging region, there's an update capsule with SVN 9,10 and 11.
 *
 * This test sends a BMC AFM active update intent to Nios firmware.
 * The results are compared against a AFM image with version 0.9 PFM.
 */
TEST_F(AFMUpdateFlowNegTest, test_active_update_complete_afm_mismatch_svn)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_SVN_9_10_11_FILE,
    		SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_SVN_9_10_11_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_orig_image);

    // Image should not be updated as invalid SVN
    // Use this for comparison purposes
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_INVALID_SVN));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Verify that AFM is not updated due to invalid SVN
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    // Ensure that old SVN is not updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test loads a AFM image with SVN 7 to BMC flash.
 * In BMC staging region, there's an update capsule with SVN 9,10 and 11.
 *
 * This test sends a BMC AFM active update intent to Nios firmware.
 * The results are compared against a AFM image with version 0.9 PFM.
 */
TEST_F(AFMUpdateFlowNegTest, test_recovery_update_complete_afm_mismatch_uuid)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE,
    		SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_orig_image);

    // Image should not be updated as invalid SVN
    // Use this for comparison purposes
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_UNKNOWN_AFM));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Verify that AFM is not updated due to invalid SVN
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    // Ensure that old SVN is not updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/**
 * @brief This test covers replay attack with no capsule in the SPI flash.
 * For details about the update scenarios, refer to update_scenarios vector.
 */
TEST_F(AFMUpdateFlowNegTest, test_replay_attack_with_bad_afm_capsule_case1)
{
    // Update scenarios
    std::vector<std::pair<MB_REGFILE_OFFSET_ENUM, MB_UPDATE_INTENT_PART2_MASK_ENUM>> update_scenarios = {
            // Nios would enter (full or partial) T-1 for these updates
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            // Nios would reject these update because max failed update attempts are reached
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
    };

    ut_common_routine_for_replay_attack_tests_part_2(update_scenarios);
}

/**
 * @brief This test covers replay attack with no capsule in the SPI flash.
 * For details about the update scenarios, refer to update_scenarios vector.
 */
TEST_F(AFMUpdateFlowNegTest, test_replay_attack_with_bad_afm_capsule_case2)
{
    // Update scenarios
    std::vector<std::pair<MB_REGFILE_OFFSET_ENUM, MB_UPDATE_INTENT_PART2_MASK_ENUM>> update_scenarios = {
            // Nios would enter (full or partial) T-1 for these updates
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_MASK},
            // Nios would reject these update because max failed update attempts are reached
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK},
            {MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_MASK},
    };

    ut_common_routine_for_replay_attack_tests_part_2(update_scenarios);
}

/*
 * This test sends bmc active intent to update part 1 but have afm capsule
 */
TEST_F(AFMUpdateFlowNegTest, test_active_update_wrong_intent_1)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE,
    		SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_orig_image);

    // Image should not be updated as invalid SVN
    // Use this for comparison purposes
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

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
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Verify that AFM is not updated due to invalid SVN
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    // Ensure that old SVN is not updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test sends afm active intent to update part 1
 */
TEST_F(AFMUpdateFlowNegTest, test_active_update_wrong_intent_2)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE,
    		SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_orig_image);

    // Image should not be updated as invalid SVN
    // Use this for comparison purposes
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_AFM_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Verify that AFM is not updated due to invalid SVN
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    // Ensure that old SVN is not updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test sends bmc active intent to update part 2
 */
TEST_F(AFMUpdateFlowNegTest, test_active_update_wrong_intent_3)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE,
    		SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_orig_image);

    // Image should not be updated as invalid SVN
    // Use this for comparison purposes
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Verify that AFM is not updated due to invalid SVN
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    // Ensure that old SVN is not updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test sends pch_recovery to update part 2
 */
TEST_F(AFMUpdateFlowNegTest, test_active_update_wrong_intent_4)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE,
    		SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_orig_image);

    // Image should not be updated as invalid SVN
    // Use this for comparison purposes
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Send the active update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_UNKNOWN_AFM));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Verify that AFM is not updated due to invalid SVN
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    // Ensure that old SVN is not updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test AFM active update when there is no AFM stored in the BMC
 */
TEST_F(AFMUpdateFlowNegTest, test_active_afm_update_without_afm_in_flash)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_orig_image);

    // Image should not be updated as invalid SVN
    // Use this for comparison purposes
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_ALL_REGIONS));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(0xff));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(0xff));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(0xff));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(0xff));

    // Check AFM SVN(s)
    // Ensure that old SVN is not updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(0));

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE,
    		SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Send the active update intent
    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK);

    mb_update_intent_handler();

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_UPDATE_NOT_ALLOWED));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(0xff));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(0xff));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(0xff));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(0xff));


    // Verify that AFM is not updated due to invalid SVN
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check AFM SVN(s)
    // Ensure that old SVN is not updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(0));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test AFM recovery update when there is no AFM stored in the BMC
 */
TEST_F(AFMUpdateFlowNegTest, test_recovery_afm_update_without_afm_in_flash)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN4_THREE_AFM_SVN7_FILE, full_orig_image);

    // Image should not be updated as invalid SVN
    // Use this for comparison purposes
    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_ALL_REGIONS));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(0xff));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(0xff));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(0xff));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(0xff));

    // Check AFM SVN(s)
    // Ensure that old SVN is not updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(0));

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE,
    		SIGNED_AFM_CAPSULE_V2P2_SVN_4_THREE_AFM_UUID8_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Send the active update intent
    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_MASK);

    mb_update_intent_handler();

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_UPDATE_NOT_ALLOWED));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));

    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(0xff));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(0xff));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(0xff));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(0xff));


    // Verify that AFM is not updated due to invalid SVN
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check AFM SVN(s)
    // Ensure that old SVN is not updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(0));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test perform recovery update when recovery AFM is corrupted.
 */
TEST_F(AFMUpdateFlowNegTest, test_afm_recovery_update_when_recovery_corrupted_case1)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    // Always do this first for specific test case
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE, full_orig_image);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE, full_new_image);

    alt_u32 *recovery_new_image = new alt_u32[SIGNED_AFM_CAPSULE_V2P2_SVN_2_ONE_AFM_UUID_9_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(SIGNED_AFM_CAPSULE_V2P2_SVN_2_ONE_AFM_UUID_9_FILE, recovery_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Corrupt afm recovery region
    alt_u32* afm_recovery_ptr = get_spi_recovery_afm_ptr(SPI_FLASH_BMC);
    *afm_recovery_ptr = 0xffffffff;

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_RECOVERY));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));
    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_2_ONE_AFM_UUID_9_FILE,
    	SIGNED_AFM_CAPSULE_V2P2_SVN_2_ONE_AFM_UUID_9_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_MASK);

    mb_update_intent_handler();

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));
    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Verify that active AFM is updated
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    alt_u32 j = 0;
    // Verify that recovery AFM is updated
    alt_u32 recovery_afm_start = BMC_STAGING_REGION_RECOVERY_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 recovery_afm_end = recovery_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = recovery_afm_start >> 2; word_i < recovery_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(recovery_new_image[j], flash_x86_ptr[word_i]);
    	j++;
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(4));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
    delete[] recovery_new_image;
}

/*
 * This test perform recovery update when recovery AFM is corrupted and mismatch afm active/capsule
 */
TEST_F(AFMUpdateFlowNegTest, test_afm_recovery_update_when_recovery_corrupted_case2)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    // Always do this first for specific test case
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE, FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE, full_orig_image);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN2_ONE_AFM_UUID9_FILE, full_new_image);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Corrupt afm recovery region
    alt_u32* afm_recovery_ptr = get_spi_recovery_afm_ptr(SPI_FLASH_BMC);
    *afm_recovery_ptr = 0xffffffff;

    /*
     * Execute the flow. Active update flow will be triggered.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_RECOVERY));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));
    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_4_ONE_AFM_FILE,
    	SIGNED_AFM_CAPSULE_V2P2_SVN_4_ONE_AFM_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_MASK);

    mb_update_intent_handler();

    /*
     * Verify results
     */
    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_RECOVERY));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));
    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Verify that active AFM is updated
    alt_u32 active_afm_start = BMC_STAGING_REGION_ACTIVE_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 active_afm_end = active_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = active_afm_start >> 2; word_i < active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Verify that recovery AFM is updated
    alt_u32 recovery_afm_start = BMC_STAGING_REGION_RECOVERY_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 recovery_afm_end = recovery_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = (recovery_afm_start >> 2) + 1; word_i < recovery_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(0));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}
