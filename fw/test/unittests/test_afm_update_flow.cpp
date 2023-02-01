#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "ut_checks.h"


class AFMUpdateFlowTest : public testing::Test
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
 * In BMC staging region, there's an update capsule with SVN 2 SVN but different content.
 *
 * This test sends a BMC AFM active update intent to Nios firmware.
 * The results are compared against a AFM image with version 0.9 PFM.
 */
TEST_F(AFMUpdateFlowTest, test_active_update_complete_afm)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_SVN_2_FILE,
    		SIGNED_AFM_CAPSULE_SVN_2_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_AFM_SVN_2_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_AFM_SVN_2_FILE, full_orig_image);

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
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

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

    // Verify that AFM is updated
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
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(2));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(2));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(2));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
}

/*
 * This test loads a AFM image with SVN 2 to BMC flash.
 * In BMC staging region, there's an recovery update capsule with SVN 7.
 *
 * This test sends a BMC AFM recovery update intent to Nios firmware.
 * The results are compared against a AFM image with version 0.9 PFM.
 */
TEST_F(AFMUpdateFlowTest, test_recovery_update_complete_afms_valid_svn)
{
    /*
     * Test Preparation
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_SVN_7_FILE,
    		SIGNED_AFM_CAPSULE_SVN_7_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_AFM_SVN_2_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_AFM_SVN_2_FILE, full_orig_image);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE, full_new_image);

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
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
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
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(1));

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
    for (alt_u32 word_i = recovery_afm_start >> 2; word_i < recovery_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
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
 * This test loads a AFM image with SVN 4 to BMC flash.
 * In BMC staging region, there's an update capsule with SVN 4 but different content.
 *
 */
TEST_F(AFMUpdateFlowTest, test_active_update_single_afm)
{
    /*
     * Test Preparation
     */
    // Load the entire image to flash
    // Always do this first for specific test case
    //SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_AFM_FILE, FULL_PFR_IMAGE_BMC_AFM_FILE_SIZE);

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_AFM_SVN_2_FILE, FULL_PFR_IMAGE_BMC_AFM_FILE_SIZE);
    switch_spi_flash(SPI_FLASH_BMC);
    //alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule
    //SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V2P2_SVN_4_ONE_AFM_FILE,
    		//SIGNED_AFM_CAPSULE_V2P2_SVN_4_ONE_AFM_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_SVN_2_FILE,
    		SIGNED_AFM_CAPSULE_V2P2_SVN_4_ONE_AFM_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);


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
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

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
    // Verify that AFM is updated
    AFM* afm_ptr = (AFM*) get_spi_flash_ptr_with_offset(SIGNATURE_SIZE + BMC_STAGING_REGION_AFM_ACTIVE_OFFSET + get_ufm_pfr_data()->bmc_staging_region);

    alt_u32* afm_header = afm_ptr->afm_addr_def;
    AFM_ADDR_DEF* afm_addr_def = (AFM_ADDR_DEF*) afm_header;

    alt_u32* afm_body = (alt_u32*) get_spi_flash_ptr_with_offset(afm_addr_def->afm_addr);

    alt_u32* capsule_afm_body = get_capsule_first_afm_ptr(get_staging_region_offset(SPI_FLASH_BMC));

    for (alt_u32 word_i = 0; word_i < (0x1000 >> 2); word_i++)
    {
        ASSERT_EQ(afm_body[word_i], capsule_afm_body[word_i]);
    }

    // Check if the SPI filtering rules are set up correctly, after the update
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(2));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(2));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(2));
}

/*
 * This test loads a AFM image with SVN 2 to BMC flash.
 * In BMC staging region, there's an update capsule with SVN 2 but different content.
 *
 * One of the AFM is of different version
 */
TEST_F(AFMUpdateFlowTest, test_active_update_full_afm_case_1)
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
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));
    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(5));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(5));
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
 * In BMC staging region, there's an update capsule with SVN 2 but different content.
 *
 * Two of the AFM is of different version
 */
TEST_F(AFMUpdateFlowTest, test_active_update_full_afm_case_2)
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
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));
    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(6));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(6));
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
 * In BMC staging region, there's an update capsule with SVN 2 but different content.
 *
 * Three of the AFM is of different version
 */
TEST_F(AFMUpdateFlowTest, test_active_update_full_afm_case_3)
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
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_V7P7_SVN_2_THREE_AFM_FILE,
    		SIGNED_AFM_CAPSULE_V7P7_SVN_2_THREE_AFM_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_BMC_V2P2_SVN2_THREE_AFM_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V2P2_SVN2_THREE_AFM_FILE, full_orig_image);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_BMC_V7P7_SVN2_THREE_AFM_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_V7P7_SVN2_THREE_AFM_FILE, full_new_image);

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
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check PCH FW version
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u32(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u32(PCH_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u32(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u32(PCH_RECOVERY_PFM_MINOR_VER));
    // Check BMC AFM version
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(7));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(7));
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
 * In BMC staging region, there's an update capsule with SVN 2 but different content.
 *
 * Two of the AFM is of different version
 */
TEST_F(AFMUpdateFlowTest, test_recovery_update_full_afm_case_1)
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

    // Verify that recovery AFM is updated
    alt_u32 recovery_afm_start = BMC_STAGING_REGION_RECOVERY_AFM_OFFSET;
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32 recovery_afm_end = recovery_afm_start + (SPI_FLASH_PAGE_SIZE_OF_64KB << 1);
    for (alt_u32 word_i = recovery_afm_start >> 2; word_i < recovery_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_new_image[word_i], flash_x86_ptr[word_i]);
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
