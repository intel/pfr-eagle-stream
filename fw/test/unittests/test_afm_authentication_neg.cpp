#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "testdata_info.h"
#include "ut_checks.h"

class AfmAuthenticationNegTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        // Reset system mocks and SPI flash
        sys->reset();
        sys->reset_spi_flash_mock();

        // Provision the system (e.g. root key hash)
        sys->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);

        // Reset Nios firmware
        ut_reset_nios_fw();

    }

    virtual void TearDown() {}
};

/**
 * @brief Perform AFM check on BMC flash image without any AFM.
 */
TEST_F(AfmAuthenticationNegTest, test_bmc_afm_authentication_with_no_afm)
{
    /*
     * Preparation
     */

    switch_spi_flash(SPI_FLASH_BMC);

    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Verify recovered data
     */

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_ALL_REGIONS));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Check if the SPI filtering rules are set up correctly, after the recovery
    // IMPORTANT NOTE: Do not call this after is_active_valid as the spi filter rules would have been applied already
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    // Should be empty
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(0));
}

/**
 * @brief Perform AFM check on corrupted active BMC flash image.
 */
TEST_F(AfmAuthenticationNegTest, test_bmc_afm_first_authentication_with_blocked_spi_access)
{
    /*
     * Preparation
     */

    switch_spi_flash(SPI_FLASH_BMC);

    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE, FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();
    // Block access of spi while doing first authentication for AFM
    ut_block_spi_read_access_once_during_afm_auth();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Verify recovered data
     */

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    // No recovery as Nios re-authenticate the AFM.
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Check if the SPI filtering rules are set up correctly, after the recovery
    // IMPORTANT NOTE: Do not call this after is_active_valid as the spi filter rules would have been applied already
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    // Should be empty
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));
}

/**
 * @brief Perform AFM check on corrupted active BMC flash image.
 */
TEST_F(AfmAuthenticationNegTest, test_bmc_afm_authentication_with_corrupted_active_region)
{
    /*
     * Preparation
     */

    switch_spi_flash(SPI_FLASH_BMC);

    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE, FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Corrupt afm active region
    alt_u32* afm_active_ptr = get_spi_active_afm_ptr(SPI_FLASH_BMC);
    *afm_active_ptr = 0xffffffff;

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Verify recovered data
     */

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_ACTIVE));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_ACTIVE));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Check if the SPI filtering rules are set up correctly, after the recovery
    // IMPORTANT NOTE: Do not call this after is_active_valid as the spi filter rules would have been applied already
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    // Should be empty
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));
}

/**
 * @brief Perform AFM check on corrupted active BMC flash image.
 */
TEST_F(AfmAuthenticationNegTest, test_bmc_afm_authentication_with_corrupted_afm_tag)
{
    /*
     * Preparation
     */

    switch_spi_flash(SPI_FLASH_BMC);

    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE, FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Corrupt afm active region
    alt_u32* afm_active_tag_ptr = get_spi_active_afm_ptr(SPI_FLASH_BMC) + 0x400/4;
    *afm_active_tag_ptr = 0xffffffff;

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Verify recovered data
     */

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_BMC_ACTIVE));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_ACTIVE));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Check if the SPI filtering rules are set up correctly, after the recovery
    // IMPORTANT NOTE: Do not call this after is_active_valid as the spi filter rules would have been applied already
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    // Should be empty
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(7));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(7));
}

/**
 * @brief Perform AFM check on corrupted recovery BMC flash image.
 */
TEST_F(AfmAuthenticationNegTest, test_bmc_afm_authentication_with_corrupted_recovery_region)
{
    /*
     * Preparation
     */

    switch_spi_flash(SPI_FLASH_BMC);

    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE, FULL_PFR_IMAGE_BMC_AFM_SVN_7_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    // flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit upon entry to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_ATTESTATION_AND_END_T0_OPERATIONS_AFTER_50_ITERS);

    // Corrupt afm recovery region
    alt_u32* afm_recovery_ptr = get_spi_recovery_afm_ptr(SPI_FLASH_BMC);
    *afm_recovery_ptr = 0xffffffff;

    /*
     * Run PFR Main. Always run with the timeout
     */
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Verify recovered data
     */

    // Check some mailbox output
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    // Expect no recovery
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AFM_AUTH_RECOVERY));

    // We should only enter T-1 once (because we ran Nios from beginning)
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));

    // Check if the SPI filtering rules are set up correctly, after the recovery
    // IMPORTANT NOTE: Do not call this after is_active_valid as the spi filter rules would have been applied already
    ut_check_all_spi_filtering_rules();

    // Check AFM SVN(s)
    // Should be empty
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(0)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(1)), alt_u32(0));
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_AFM_UUID(2)), alt_u32(0));
}
