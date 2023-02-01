#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class DebugFlashTest : public testing::Test
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

        // Load the PCH PFR image to the SPI flash mock
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, PFR_DEBUG_FULL_PCH_IMAGE, FULL_PFR_IMAGE_PCH_FILE_SIZE);
        // Load the BMC PFR image to the SPI flash mock
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, PFR_DEBUG_FULL_BMC_IMAGE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    }

    virtual void TearDown() {}
};

TEST_F(DebugFlashTest, test_validate_custom_pch_full_image)
{
    switch_spi_flash(SPI_FLASH_PCH);

    // Verify signature of active section
    alt_u32 active_pfm_addr = get_active_region_offset(SPI_FLASH_PCH);
    if (is_signature_valid(active_pfm_addr) == false)
    {
        ut_debug_log_error(UT_PFM, UT_MANIFEST_SIG, UT_ACTIVE);
    }

    // Verify content of active section
    if (is_active_pfm_valid((PFM*) get_spi_flash_ptr_with_offset(active_pfm_addr + SIGNATURE_SIZE)) == false)
    {
        ut_debug_log_error(UT_PFM, 0, UT_ACTIVE);
    }

    // Verify the signature of the recovery capsule
    if (is_signature_valid(get_recovery_region_offset(SPI_FLASH_PCH)) == false)
    {
        ut_debug_log_error(UT_PFM, UT_CAPSULE_SIG, UT_RECOVERY);
    }

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    switch_spi_flash(SPI_FLASH_PCH);
    // Verify the content of the compressed capsule
    // Corrupt the first page in the first static region
    erase_spi_region(testdata_pch_static_regions_start_addr[0], PBC_EXPECTED_PAGE_SIZE);

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    switch_spi_flash(SPI_FLASH_PCH);

    // Verify the signature of the recovery capsule
    if (is_signature_valid(active_pfm_addr) == false)
    {
        ut_debug_log_error(UT_PFM, UT_MANIFEST_SIG, UT_RECOVERY);
    }

    // Verify content of recovery capsule
    if (is_active_pfm_valid((PFM*) get_spi_flash_ptr_with_offset(active_pfm_addr + SIGNATURE_SIZE)) == false)
    {
        ut_debug_log_error(UT_PFM, 0, UT_RECOVERY);
    }
}

TEST_F(DebugFlashTest, test_validate_custom_bmc_full_image)
{
    switch_spi_flash(SPI_FLASH_BMC);

    // Verify signature of active section
    alt_u32 active_pfm_addr = get_active_region_offset(SPI_FLASH_BMC);
    if (is_signature_valid(active_pfm_addr) == false)
    {
        ut_debug_log_error(UT_PFM, UT_MANIFEST_SIG, UT_ACTIVE);
    }

    // Verify content of active section
    if (is_active_pfm_valid((PFM*) get_spi_flash_ptr_with_offset(active_pfm_addr + SIGNATURE_SIZE)) == false)
    {
        ut_debug_log_error(UT_PFM, 0, UT_ACTIVE);
    }

    // Verify the signature of the recovery capsule
    if (is_signature_valid(get_recovery_region_offset(SPI_FLASH_BMC)) == false)
    {
        ut_debug_log_error(UT_PFM, UT_CAPSULE_SIG, UT_RECOVERY);
    }

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    switch_spi_flash(SPI_FLASH_BMC);

    // Verify the content of the compressed capsule
    // Corrupt the first page in the first static region
    erase_spi_region(testdata_bmc_spi_regions_start_addr[0], PBC_EXPECTED_PAGE_SIZE);

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Verify the signature of the recovery content
    if (is_signature_valid(active_pfm_addr) == false)
    {
        ut_debug_log_error(UT_PFM, UT_MANIFEST_SIG, UT_RECOVERY);
    }

    // Verify content of recovery capsule
    if (is_active_pfm_valid((PFM*) get_spi_flash_ptr_with_offset(active_pfm_addr + SIGNATURE_SIZE)) == false)
    {
        ut_debug_log_error(UT_PFM, 0, UT_RECOVERY);
    }
}

TEST_F(DebugFlashTest, test_validate_custom_bmc_full_image_with_afm)
{
    // Perform provisioning
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);

    // Load the BMC PFR image to the SPI flash mock
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, PFR_DEBUG_FULL_BMC_WITH_AFM_IMAGE, FULL_PFR_IMAGE_BMC_AFM_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);

    // Verify signature of active section
    alt_u32 active_afm_addr = get_staging_region_offset(SPI_FLASH_BMC) + BMC_STAGING_REGION_AFM_ACTIVE_OFFSET;

    if (is_signature_valid(active_afm_addr) == false)
    {
        ut_debug_log_error(UT_AFM, UT_MANIFEST_SIG, UT_ACTIVE);
    }

    // Verify content of active section
    if (is_active_afm_valid((AFM*)get_spi_flash_ptr_with_offset(active_afm_addr + SIGNATURE_SIZE), 1) == false)
    {
        ut_debug_log_error(UT_AFM, 0, UT_ACTIVE);
    }

    // Verify the signature of recovery
    alt_u32 recovery_afm_addr = get_staging_region_offset(SPI_FLASH_BMC) + BMC_STAGING_REGION_AFM_RECOVERY_OFFSET;

    // Verify the signature of the recovery capsule
    if (is_signature_valid(recovery_afm_addr) == false)
    {
        ut_debug_log_error(UT_AFM, UT_CAPSULE_SIG, UT_RECOVERY);
    }

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    switch_spi_flash(SPI_FLASH_BMC);

    // Verify the content of the compressed capsule
    // Corrupt the first page of AFM image
    *get_spi_flash_ptr_with_offset(active_afm_addr) = 0xdeadcafe;

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Verify the signature of the recovery content
    if (is_signature_valid(active_afm_addr) == false)
    {
        ut_debug_log_error(UT_AFM, UT_MANIFEST_SIG, UT_RECOVERY);
    }

    // Verify content of recovery capsule
    if (is_active_afm_valid((AFM*)get_spi_flash_ptr_with_offset(active_afm_addr + SIGNATURE_SIZE), 1) == false)
    {
        ut_debug_log_error(UT_AFM, 0, UT_RECOVERY);
    }
}

TEST_F(DebugFlashTest, test_validate_custom_pch_full_image_with_seamless_update)
{
    /*
     * Test Preparation
     */
    // Load the PCH PFR image to the SPI flash mock
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, PFR_DEBUG_SEAMLESS_FULL_IMAGE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // Load the Seamless FV capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            PFR_DEBUG_SEAMLESS_UPDATE_CAPSULE,
            PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET
    );

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(PFR_DEBUG_SEAMLESS_FULL_IMAGE, full_orig_image);

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

    // Seamless update should not require T-1 entry
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Check BMC FW version
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u32(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u32(BMC_ACTIVE_PFM_MINOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u32(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u32(BMC_RECOVERY_PFM_MINOR_VER));

    // The original update capsule should be invalidated after successful seamless update
    switch_spi_flash(SPI_FLASH_BMC);
    EXPECT_FALSE(is_signature_valid(get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET));

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
}

TEST_F(DebugFlashTest, test_validate_custom_bmc_full_image_with_afm_update_case1)
{

	switch_spi_flash(SPI_FLASH_BMC);
    // Load the BMC PFR image to the SPI flash mock
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, PFR_DEBUG_FULL_BMC_WITH_AFM_IMAGE_2, PFR_DEBUG_FULL_BMC_WITH_AFM_IMAGE_SIZE_2);

    // Perform provisioning
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);


    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);


    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());


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

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(4));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(4));

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, PFR_DEBUG_BMC_UPDATE_CAPSULE,
    		PFR_DEBUG_BMC_UPDATE_CAPSULE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Send the active update intent
    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    mb_update_intent_handler();


    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));


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

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(4));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(4));

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, PFR_DEBUG_FULL_BMC_WITH_AFM_IMAGE_2, FULL_PFR_IMAGE_BMC_AFM_FILE_SIZE);

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, PFR_DEBUG_AFM_ACTIVE_UPDATE_CAPSULE,
    		PFR_DEBUG_AFM_ACTIVE_UPDATE_CAPSULE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Send the recovery update intent
    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK);

    mb_update_intent_handler();


    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));


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
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_ACTIVE_MINOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MAJOR_VER), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_BMC_AFM_RECOVERY_MINOR_VER), alt_u32(2));

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(4));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(4));


    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, PFR_DEBUG_AFM_ACTIVE_UPDATE_CAPSULE,
    		PFR_DEBUG_AFM_ACTIVE_UPDATE_CAPSULE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Send the recovery update intent
    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_RECOVERY_MASK);

    mb_update_intent_handler();


    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));


    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(3));
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

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(4));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(4));

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, PFR_DEBUG_BMC_UPDATE_CAPSULE,
    		PFR_DEBUG_BMC_UPDATE_CAPSULE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Send the active update intent
    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_BMC_ACTIVE_MASK);

    mb_update_intent_handler();


    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(2));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(3));


    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(4));
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

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(4));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(4));

    // Load the update capsule
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, PFR_DEBUG_AFM_ACTIVE_UPDATE_CAPSULE,
    		PFR_DEBUG_AFM_ACTIVE_UPDATE_CAPSULE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Send the active update intent
    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK);

    mb_update_intent_handler();


    // Nios should transition from T0 to T-1 exactly once for active update flow
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(2));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(4));


    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(5));
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

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(4));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(4));
}
