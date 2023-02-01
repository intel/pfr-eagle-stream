#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "testdata_info.h"
#include "ut_checks.h"


class AfmRecoveryFlowTest : public testing::Test
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
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE, FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE_SIZE);
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    }

    virtual void TearDown() {}
};

/**
 * @brief Corrupt static portion of the active firmware on BMC flash. This should trigger the FW recovery
 * mechanism in T-1 authentication step.
 */
TEST_F(AfmRecoveryFlowTest, test_bmc_fw_recovery_and_afm_validation)
{
    /*
     * Preparation
     */
	SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

    switch_spi_flash(SPI_FLASH_BMC);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE, full_image);

    // Corrupt the first page in the first static region
    erase_spi_region(testdata_bmc_static_regions_start_addr[0], PBC_EXPECTED_PAGE_SIZE);

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    //SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_MEDIUM, pfr_main());

    /*
     * Verify recovered data
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_ACTIVE));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ACTIVE));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Verify static regions
    for (alt_u32 region_i = 0; region_i < BMC_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        for (alt_u32 word_i = testdata_bmc_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_bmc_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(full_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Check if the SPI filtering rules are set up correctly, after the recovery
    // IMPORTANT NOTE: Do not call this after is_active_valid as the spi filter rules would have been applied already
    ut_check_all_spi_filtering_rules();

    // Get PFM offsets
    alt_u32 bmc_active_pfm_start = get_ufm_pfr_data()->bmc_active_pfm;
    alt_u32 bmc_active_pfm_end = bmc_active_pfm_start + get_signed_payload_size(get_spi_active_pfm_ptr(SPI_FLASH_BMC));

    // Verify PFM
    EXPECT_TRUE(is_active_region_valid(SPI_FLASH_BMC));
    for (alt_u32 word_i = bmc_active_pfm_start >> 2; word_i < bmc_active_pfm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_image[word_i], flash_x86_ptr[word_i]);
    }

    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();
    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_MEDIUM, pfr_main());

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));

    /*
     * Clean ups
     */
    delete[] full_image;
}

/**
 * @brief Corrupt the active afm firmware on BMC flash. This should trigger the FW recovery
 * mechanism in T-1 authentication step.
 */
TEST_F(AfmRecoveryFlowTest, test_afm_recovery_and_afm_active_corruption)
{
    /*
     * Preparation
     */

    switch_spi_flash(SPI_FLASH_BMC);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE, full_image);

    // Corrupt afm active region
    alt_u32* afm_active_ptr = get_spi_active_afm_ptr(SPI_FLASH_BMC);
    *afm_active_ptr = 0xffffffff;

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Verify recovered data
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_ACTIVE));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_ACTIVE));

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Check if the SPI filtering rules are set up correctly, after the recovery
    // IMPORTANT NOTE: Do not call this after is_active_valid as the spi filter rules would have been applied already
    ut_check_all_spi_filtering_rules();

    // Get AFM offsets
    alt_u32 bmc_active_afm_start = get_staging_region_offset(SPI_FLASH_BMC) + BMC_STAGING_REGION_AFM_ACTIVE_OFFSET;;
    alt_u32 bmc_active_afm_end = bmc_active_afm_start + get_signed_payload_size(get_spi_active_afm_ptr(SPI_FLASH_BMC));

    // Verify AFM
    EXPECT_TRUE(is_staging_active_afm_valid(SPI_FLASH_BMC));
    for (alt_u32 word_i = bmc_active_afm_start >> 2; word_i < bmc_active_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(full_image[word_i], flash_x86_ptr[word_i]);
    }

    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));

    /*
     * Clean ups
     */
    delete[] full_image;
}

/**
 * @brief Corrupt recovery afm firmware on BMC flash. This should trigger the FW recovery
 * mechanism in T-1 authentication step.
 */
TEST_F(AfmRecoveryFlowTest, test_afm_recovery_update_and_afm_recovery_corruption_case1)
{
    /*
     * Preparation
     */

    switch_spi_flash(SPI_FLASH_BMC);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE, full_image);

    // Load the update capsule to staging area
    // TODO: to load update capsule here so that this test can pass
    /*SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_FILE,
    		SIGNED_AFM_CAPSULE_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);*/

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_SVN_7_FILE,
    		SIGNED_AFM_CAPSULE_SVN_7_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Corrupt afm recovery region
    alt_u32* afm_recovery_ptr = get_spi_recovery_afm_ptr(SPI_FLASH_BMC);
    *afm_recovery_ptr = 0xffffffff;

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM), (alt_u32) 0);

    EXPECT_TRUE(is_signature_valid(get_staging_region_offset(SPI_FLASH_BMC)));
    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_MEDIUM, pfr_main());

    /*
     * Verify recovered data
     */
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* flash_x86_ptr = SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_RECOVERY));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_RECOVERY));


    // Expect SVN to be updated
    // TODO: Uncomment this with good capsule
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM), (alt_u32) 7);

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Check if the SPI filtering rules are set up correctly, after the recovery
    // IMPORTANT NOTE: Do not call this after is_active_valid as the spi filter rules would have been applied already
    ut_check_all_spi_filtering_rules();

    // Get AFM offsets
    alt_u32 bmc_recovery_afm_start = get_staging_region_offset(SPI_FLASH_BMC) + BMC_STAGING_REGION_AFM_RECOVERY_OFFSET;;
    alt_u32 bmc_recovery_afm_end = bmc_recovery_afm_start + get_signed_payload_size(get_spi_recovery_afm_ptr(SPI_FLASH_BMC));

    // Verify AFM
    EXPECT_TRUE(is_staging_recovery_afm_valid(SPI_FLASH_BMC));
    for (alt_u32 word_i = bmc_recovery_afm_start >> 2; word_i < bmc_recovery_afm_end >> 2; word_i++)
    {
        ASSERT_EQ(flash_x86_ptr[word_i - (BMC_STAGING_REGION_AFM_RECOVERY_OFFSET >> 2)], flash_x86_ptr[word_i]);
    }

    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);


    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));

    /*
     * Clean ups
     */
    delete[] full_image;
}

/**
 * @brief Corrupt recovery afm firmware on BMC flash. This should trigger the FW recovery
 * mechanism in T-1 authentication step.
 */
TEST_F(AfmRecoveryFlowTest, test_afm_recovery_update_and_afm_recovery_corruption_case2)
{
    /*
     * Preparation
     */

    switch_spi_flash(SPI_FLASH_BMC);

    // Load the entire image locally for comparison purpose
    alt_u32 *full_image = new alt_u32[FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE, full_image);

    // Load the update capsule to staging area
    // TODO: to load update capsule here so that this test can pass
    /*SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_FILE,
    		SIGNED_AFM_CAPSULE_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);*/

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_SVN_2_FILE,
    		SIGNED_AFM_CAPSULE_SVN_2_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Corrupt afm recovery region
    alt_u32* afm_recovery_ptr = get_spi_recovery_afm_ptr(SPI_FLASH_BMC);
    *afm_recovery_ptr = 0xffffffff;

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM), (alt_u32) 0);

    EXPECT_TRUE(is_signature_valid(get_staging_region_offset(SPI_FLASH_BMC)));
    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_MEDIUM, pfr_main());

    /*
     * Verify recovered data
     */
    switch_spi_flash(SPI_FLASH_BMC);

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_RECOVERY));


    // Expect SVN to not be updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM), (alt_u32) 0);

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(0));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Check if the SPI filtering rules are set up correctly, after the recovery
    // IMPORTANT NOTE: Do not call this after is_active_valid as the spi filter rules would have been applied already
    ut_check_all_spi_filtering_rules();


    // Verify AFM
    // Expect false as no recovery is happeneing
    EXPECT_FALSE(is_staging_recovery_afm_valid(SPI_FLASH_BMC));

    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);


    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_RECOVERY));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

    // Check AFM SVN(s)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM), (alt_u32) 0);
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(0));

    /*
     * Clean ups
     */
    delete[] full_image;
}

/**
 * @brief Corrupt recovery afm firmware and staging on BMC flash. This should trigger the FW recovery
 * mechanism in T-1 authentication step.
 */
TEST_F(AfmRecoveryFlowTest, test_afm_recovery_and_staging_corruption)
{
    /*
     * Preparation
     */

    switch_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule to staging area
    // TODO: to load update capsule here so that this test can pass
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_FILE,
    		SIGNED_AFM_CAPSULE_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Corrupt afm recovery region
    alt_u32* afm_recovery_ptr = get_spi_recovery_afm_ptr(SPI_FLASH_BMC);
    *afm_recovery_ptr = 0xffffffff;

    // Corrupt staging region
    alt_u32* afm_staging_ptr = get_spi_staging_region_ptr(SPI_FLASH_BMC);
    *afm_staging_ptr = 0xffffffff;

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_RECOVERY));

    // Check AFM SVN(s)
    // Should not be updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(0));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Check if the SPI filtering rules are set up correctly, after the recovery
    // IMPORTANT NOTE: Do not call this after is_active_valid as the spi filter rules would have been applied already
    ut_check_all_spi_filtering_rules();
}

/**
 * @brief Corrupt all afm region on BMC flash. This should trigger the FW recovery
 * mechanism in T-1 authentication step.
 */
TEST_F(AfmRecoveryFlowTest, test_afm_recovery_and_staging_and_active_corruption)
{
    /*
     * Preparation
     */

    switch_spi_flash(SPI_FLASH_BMC);

    // Load the update capsule to staging area
    // TODO: to load update capsule here so that this test can pass
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_AFM_CAPSULE_FILE,
    		SIGNED_AFM_CAPSULE_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

    // Corrupt afm recovery region
    alt_u32* afm_recovery_ptr = get_spi_recovery_afm_ptr(SPI_FLASH_BMC);
    *afm_recovery_ptr = 0xffffffff;

    // Corrupt staging region
    alt_u32* afm_staging_ptr = get_spi_staging_region_ptr(SPI_FLASH_BMC);
    *afm_staging_ptr = 0xffffffff;

    // Corrupt active region
    alt_u32* afm_active_ptr = get_spi_active_afm_ptr(SPI_FLASH_BMC);
    *afm_active_ptr = 0xffffffff;

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_ALL_REGIONS));

    // Check AFM SVN(s)
    // Should not be updated
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(0));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Check if the SPI filtering rules are set up correctly, after the recovery
    // IMPORTANT NOTE: Do not call this after is_active_valid as the spi filter rules would have been applied already
    ut_check_all_spi_filtering_rules();
}
