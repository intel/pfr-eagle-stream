#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "testdata_info.h"

class PFRAntiRollbackWithKeyIDSeamlessTest : public testing::Test
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
 * @brief This test cancels key ID 0 for signing seamless capsule and then
 * perform authentication on various signed payload.
 *
 * Note that the key cancellation policy is shared between keys used for signing PCH firmware capsule and
 * Seamless FV capsule. After cancelling key ID 0, keys with ID 0 for signing PCH firmware capsule and
 * Seamless FV capsule would be invalidated.
 */
TEST_F(PFRAntiRollbackWithKeyIDSeamlessTest, test_cancel_key_for_seamless_capsule_and_auth)
{
    switch_spi_flash(SPI_FLASH_PCH);

    /*
     * Cancel key ID 0 for signing Seamless capsule.
     * Verify authentication results before and after the cancellation.
     */
    // Load a Seamless FV capsule and a PCH firmware capsule
    alt_u32 signed_seamless_capsule_addr = get_ufm_pfr_data()->pch_staging_region;
    alt_u32 signed_pch_fw_capsule_addr = get_ufm_pfr_data()->pch_recovery_region;

    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE,
            PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE_SIZE,
            signed_seamless_capsule_addr);

    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE,
            signed_pch_fw_capsule_addr);

    // Authentication should pass in the beginning
    EXPECT_TRUE(is_signature_valid(signed_seamless_capsule_addr));
    EXPECT_TRUE(is_signature_valid(signed_pch_fw_capsule_addr));

    // Cancel key 0
    cancel_key(KCH_PC_PFR_PCH_SEAMLESS_CAPSULE, 0);

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid(signed_seamless_capsule_addr));
    EXPECT_FALSE(is_signature_valid(signed_pch_fw_capsule_addr));

    /*
     * Sanity checks on other PC types
     */

    // PCH signed PFM and FVM should not be invalidated, since they have separate Key Cancellation Policy
    alt_u32 signed_payload_addr = get_ufm_pfr_data()->pch_staging_region;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_PFM_PCH_FILE,
            SIGNED_PFM_PCH_FILE_SIZE,
            signed_payload_addr);
    // Authentication should pass
    EXPECT_TRUE(is_signature_valid(signed_payload_addr));

    // PCH signed PFM and FVM should not be invalidated, since they have separate Key Cancellation Policy
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            PCH_FW_UPDATE_SIGNED_FVM_PCH_UCODE1_FILE,
            PCH_FW_UPDATE_SIGNED_FVM_PCH_UCODE1_FILE_SIZE,
            signed_payload_addr);
    // Authentication should pass
    EXPECT_TRUE(is_signature_valid(signed_payload_addr));

    // CPLD capsule should not be invalidated, since they have separate Key Cancellation Policy
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE,
            signed_payload_addr);
    // Authentication should pass
    EXPECT_TRUE(is_signature_valid(signed_payload_addr));

    // BMC PFM should not be invalidated, since they have separate Key Cancellation Policy
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_PFM_BMC_FILE,
            SIGNED_PFM_BMC_FILE_SIZE,
            signed_payload_addr);
    // Authentication should pass
    EXPECT_TRUE(is_signature_valid(signed_payload_addr));

    // BMC firmware capsule should not be invalidated, since they have separate Key Cancellation Policy
    switch_spi_flash(SPI_FLASH_BMC);
    signed_payload_addr = get_ufm_pfr_data()->bmc_staging_region;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_BMC_FILE,
            SIGNED_CAPSULE_BMC_FILE_SIZE,
            signed_payload_addr);
    // Authentication should pass
    EXPECT_TRUE(is_signature_valid(signed_payload_addr));
}

/*
 * @brief This test cancels key ID 0 for signing PCH firmware capsule and then
 * perform authentication on various signed payload.
 *
 * Note that the key cancellation policy is shared between keys used for signing PCH firmware capsule and
 * Seamless FV capsule. After cancelling key ID 0, keys with ID 0 for signing PCH firmware capsule and
 * Seamless FV capsule would be invalidated.
 */
TEST_F(PFRAntiRollbackWithKeyIDSeamlessTest, test_cancel_key_for_pch_fw_capsule_and_auth)
{
    switch_spi_flash(SPI_FLASH_PCH);

    /*
     * Cancel key ID 0 for signing PCH firmware capsule.
     * Verify authentication results before and after the cancellation.
     */
    // Load a Seamless FV capsule and a PCH firmware capsule
    alt_u32 signed_seamless_capsule_addr = get_ufm_pfr_data()->pch_staging_region;
    alt_u32 signed_pch_fw_capsule_addr = get_ufm_pfr_data()->pch_recovery_region;

    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE,
            PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE_SIZE,
            signed_seamless_capsule_addr);

    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_V03P12_FILE,
            SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE,
            signed_pch_fw_capsule_addr);

    // Authentication should pass in the beginning
    EXPECT_TRUE(is_signature_valid(signed_seamless_capsule_addr));
    EXPECT_TRUE(is_signature_valid(signed_pch_fw_capsule_addr));

    // Cancel key 0
    cancel_key(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 0);

    // Authentication should fail
    EXPECT_FALSE(is_signature_valid(signed_seamless_capsule_addr));
    EXPECT_FALSE(is_signature_valid(signed_pch_fw_capsule_addr));

    /*
     * Sanity checks on other PC types
     */

    // PCH signed PFM and FVM should not be invalidated, since they have separate Key Cancellation Policy
    alt_u32 signed_payload_addr = get_ufm_pfr_data()->pch_staging_region;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_PFM_PCH_FILE,
            SIGNED_PFM_PCH_FILE_SIZE,
            signed_payload_addr);
    // Authentication should pass
    EXPECT_TRUE(is_signature_valid(signed_payload_addr));

    // PCH signed PFM and FVM should not be invalidated, since they have separate Key Cancellation Policy
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            PCH_FW_UPDATE_SIGNED_FVM_PCH_UCODE1_FILE,
            PCH_FW_UPDATE_SIGNED_FVM_PCH_UCODE1_FILE_SIZE,
            signed_payload_addr);
    // Authentication should pass
    EXPECT_TRUE(is_signature_valid(signed_payload_addr));

    // CPLD capsule should not be invalidated, since they have separate Key Cancellation Policy
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_CPLD_FILE,
            SIGNED_CAPSULE_CPLD_FILE_SIZE,
            signed_payload_addr);
    // Authentication should pass
    EXPECT_TRUE(is_signature_valid(signed_payload_addr));

    // BMC PFM should not be invalidated, since they have separate Key Cancellation Policy
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_PFM_BMC_FILE,
            SIGNED_PFM_BMC_FILE_SIZE,
            signed_payload_addr);
    // Authentication should pass
    EXPECT_TRUE(is_signature_valid(signed_payload_addr));

    // BMC firmware capsule should not be invalidated, since they have separate Key Cancellation Policy
    switch_spi_flash(SPI_FLASH_BMC);
    signed_payload_addr = get_ufm_pfr_data()->bmc_staging_region;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_BMC_FILE,
            SIGNED_CAPSULE_BMC_FILE_SIZE,
            signed_payload_addr);
    // Authentication should pass
    EXPECT_TRUE(is_signature_valid(signed_payload_addr));
}

/**
 * @brief This test performs a key cancellation (PC type: PCH PFM + Key Can Cert),
 * then makes sure authentication fails for FVM signed with the cancelled key.
 */
TEST_F(PFRAntiRollbackWithKeyIDSeamlessTest, test_key_cancellation_flow_to_cancel_pch_pfm_key)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    /*
     * Send key cancellation certificate
     * This certificate would cancel CSK ID 2 that signs PFM and FVM
     */
    // Load the key cancellation certificate
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            KEY_CAN_CERT_PCH_PFM_KEY2,
            KEY_CAN_CERT_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET
    );

    // Send the update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    // No panic event as the Seamless update mechanism was used
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));

    // Expecting key ID 2 to be cancelled
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_SEAMLESS_CAPSULE, 2));

    // Have entered T-1 once
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Load an FVM that is signed with key 2
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 signed_payload_addr = get_ufm_pfr_data()->pch_staging_region;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_FVM_PCH_ME_WITH_CSK_ID2_FILE,
            SIGNED_FVM_PCH_ME_WITH_CSK_ID2_FILE_SIZE,
            signed_payload_addr);
    EXPECT_FALSE(is_signature_valid(signed_payload_addr));

    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_FVM_PCH_UCODE1_WITH_CSK_ID2_FILE,
            SIGNED_FVM_PCH_UCODE1_WITH_CSK_ID2_FILE_SIZE,
            signed_payload_addr);
    EXPECT_FALSE(is_signature_valid(signed_payload_addr));
}

/**
 * @brief This test performs a key cancellation (PC type: PCH FW capsule + Key Can Cert),
 * then attempts to update PCH firmware with capsules signed by the cancelled key.
 */
TEST_F(PFRAntiRollbackWithKeyIDSeamlessTest, test_key_cancellation_flow_to_cancel_pch_fw_cap_key)
{
    /*
     * Test Preparation
     */
    // Load the seamless capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_SEAMLESS_ME_WITH_CSK_ID10_FILE,
            SIGNED_CAPSULE_PCH_SEAMLESS_ME_WITH_CSK_ID10_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

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
     * Step 1: Attempt PCH FV Seamless update prior to key cancellation
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));

    // Have entered T-1 once
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    /*
     * Step 2: Send key cancellation certificate
     */
    // Load the key cancellation certificate
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            KEY_CAN_CERT_PCH_CAPSULE_KEY10,
            KEY_CAN_CERT_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

    // Send the update intent
    write_to_mailbox(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // Process update intent in T0
    mb_update_intent_handler();

    // Nios should transition from T0 to T-1 exactly once for key cancellation
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));

    // Expecting key ID 10 to be cancelled
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 10));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_SEAMLESS_CAPSULE, 10));

    // Have entered T-1 two times
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(1));

    /*
     * Step 3: Attempt PCH FV Seamless update (with capsule signed with cancelled CSK key) again.
     * It should be rejected this time.
     */
    // Load the Seamless capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_SEAMLESS_ME_WITH_CSK_ID10_FILE,
            SIGNED_CAPSULE_PCH_SEAMLESS_ME_WITH_CSK_ID10_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

    // Send the update intent
    write_to_mailbox(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

    // Process update intent in T0
    mb_update_intent_handler();

    // Expecting auth error in this update event
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    // Check number of failed update attempts
    EXPECT_EQ(num_failed_fw_or_cpld_update_attempts, alt_u32(1));

    // Have entered T-1 two times
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(1));

    /*
     * Step 4: Attempt full PCH Firmware update (with capsule signed with cancelled CSK key).
     * This request should be rejected
     */
    // Load the Seamless capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_SEAMLESS_WITH_CSK_ID10_FILE,
            SIGNED_CAPSULE_PCH_SEAMLESS_WITH_CSK_ID10_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

    // Send the update intent
    write_to_mailbox(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // Process update intent in T0
    mb_update_intent_handler();

    // Expecting auth error in this update event
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    // Check number of failed update attempts
    EXPECT_EQ(num_failed_fw_or_cpld_update_attempts, alt_u32(2));

    // Have entered T-1 three times
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(2));
}

/**
 * @brief This test performs a key cancellation (PC type: PCH Seamless capsule + Key Can Cert),
 * then attempts to update PCH firmware with capsules signed by the cancelled key.
 */
TEST_F(PFRAntiRollbackWithKeyIDSeamlessTest, test_key_cancellation_flow_to_cancel_seamless_cap_key)
{
    /*
     * Test Preparation
     */
    // Load the seamless capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_WITH_CSK_ID10_FILE,
            SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_WITH_CSK_ID10_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET
    );

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
     * Step 1: Attempt PCH FV Seamless update prior to key cancellation
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));

    // Have entered T-1 once
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    /*
     * Step 2: Send key cancellation certificate
     */
    // Load the key cancellation certificate
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            KEY_CAN_CERT_PCH_FV_SEAMLESS_CAPSULE_KEY10,
            KEY_CAN_CERT_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

    // Send the update intent
    write_to_mailbox(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

    // Process update intent in T0
    mb_update_intent_handler();

    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    // No panic event as the Seamless update mechanism was used
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));

    // Expecting key ID 10 to be cancelled
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 10));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_SEAMLESS_CAPSULE, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_SEAMLESS_CAPSULE, 0));

    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    /*
     * Step 3: Attempt PCH FV Seamless update (with capsule signed with cancelled CSK key) again.
     * It should be rejected this time.
     */
    // Load the Seamless capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_CAPSULE_PCH_SEAMLESS_ME_WITH_CSK_ID10_FILE,
            SIGNED_CAPSULE_PCH_SEAMLESS_ME_WITH_CSK_ID10_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region);

    // Send the update intent
    write_to_mailbox(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

    // Process update intent in T0
    mb_update_intent_handler();

    // Expecting auth error in this update event
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));

    // Check number of failed update attempts
    EXPECT_EQ(num_failed_fw_or_cpld_update_attempts, alt_u32(1));

    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    /*
     * Step 4: Attempt full PCH Firmware update (with capsule signed with cancelled CSK key).
     * This request should be rejected
     */
    // Load the Seamless capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            SIGNED_CAPSULE_PCH_SEAMLESS_WITH_CSK_ID10_FILE,
            SIGNED_CAPSULE_PCH_SEAMLESS_WITH_CSK_ID10_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET
    );

    // Send the update intent
    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    // Process update intent in T0
    mb_update_intent_handler();

    // Expecting auth error in this update event
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_BMC_UPDATE_INTENT));

    // Check number of failed update attempts
    EXPECT_EQ(num_failed_fw_or_cpld_update_attempts, alt_u32(2));

    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));
}

/**
 * @brief This test performs an invalid key cancellation (PC type: PCH PFM + Key Can Cert),
 * then makes sure authentication fails for FVM signed with the cancelled key.
 */
TEST_F(PFRAntiRollbackWithKeyIDSeamlessTest, test_key_cancellation_flow_to_cancel_pch_pfm_key_while_being_used)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    /*
     * Send key cancellation certificate
     * This certificate would cancel CSK ID 0 that signs PFM and FVM
     */
    // Load the key cancellation certificate
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            KEY_CAN_CERT_PCH_PFM_KEY0,
            KEY_CAN_CERT_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET
    );

    // Send the update intent
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Expecting error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    // No panic event as the Seamless update mechanism was used
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));

    // Expecting key ID 0 to not be cancelled
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_SEAMLESS_CAPSULE, 2));

    // Have entered T-1 once
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Load the key cancellation certificate
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            KEY_CAN_CERT_PCH_PFM_KEY2,
            KEY_CAN_CERT_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET
    );

    // Send the update intent
    write_to_mailbox(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

    // Process update intent in T0
    mb_update_intent_part2_handler_for_bmc();

    // Expecting no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    // No panic event as the Seamless update mechanism was used
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));

    // Expecting key ID 0 to not be cancelled
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_SEAMLESS_CAPSULE, 2));

    // Have entered T-1 once
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(1));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_bmc_only_counter(), alt_u32(0));
    EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_pch_only_counter(), alt_u32(0));

    // Load an FVM that is signed with key 2
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32 signed_payload_addr = get_ufm_pfr_data()->pch_staging_region;
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_FVM_PCH_ME_WITH_CSK_ID2_FILE,
            SIGNED_FVM_PCH_ME_WITH_CSK_ID2_FILE_SIZE,
            signed_payload_addr);
    EXPECT_FALSE(is_signature_valid(signed_payload_addr));

    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            SIGNED_FVM_PCH_UCODE1_WITH_CSK_ID2_FILE,
            SIGNED_FVM_PCH_UCODE1_WITH_CSK_ID2_FILE_SIZE,
            signed_payload_addr);
    EXPECT_FALSE(is_signature_valid(signed_payload_addr));
}
