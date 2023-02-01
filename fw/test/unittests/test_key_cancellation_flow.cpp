#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "testdata_info.h"


class KeyCancellationFlowTest : public testing::Test
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
 * @brief Execute in-band key cancellation with pc type PCH_PFM and id 2.
 * Send key cancellation certificate to PCH flash and use PCH active update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key2_for_pch_pfm_through_ib_pch_active_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, KEY_CAN_CERT_PCH_PFM_KEY2,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

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
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 2 for signing PCH PFM should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 2));
}

/**
 * @brief Execute in-band key cancellation with pc type BMC_UPDATE_CAPSULE and id 1.
 * Send key cancellation certificate to PCH flash and use PCH active update intent to
 * trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key1_for_bmc_capsule_through_ib_pch_active_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, KEY_CAN_CERT_BMC_CAPSULE_KEY1,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

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
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 1 for signing BMC update capsule should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 1));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 1));
}

/**
 * @brief Execute in-band key cancellation with pc type CPLD_UPDATE_CAPSULE and id 10.
 * Send key cancellation certificate to PCH flash and use PCH active update intent to
 * trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key10_for_cpld_capsule_through_ib_pch_active_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, KEY_CAN_CERT_CPLD_CAPSULE_KEY10,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

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
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 10 for signing CPLD update capsule should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 10));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 10));
}

/**
 * @brief Execute oob-band key cancellation with pc type PCH_UPDATE_CAPSULE and id 2.
 * Send key cancellation certificate to BMC flash and use PCH recovery update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key2_for_pch_capsule_through_oob_pch_recovery_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_PCH_CAPSULE_KEY2,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

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
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 2 for signing PCH update capsule should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 2));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 2));
}

/**
 * @brief Execute oob-band key cancellation with pc type BMC_PFM and id 10.
 * Send key cancellation certificate to BMC flash and use BMC active update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key10_for_bmc_pfm_through_oob_bmc_active_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_BMC_PFM_KEY10,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

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

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 10 for signing BMC PFM should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 10));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 10));
}

/**
 * @brief Execute oob-band key cancellation with pc type CPLD_UPDATE_CAPSULE and id 10.
 * Send key cancellation certificate to BMC flash and use BMC active update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key10_for_cpld_capsule_through_oob_bmc_active_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_CPLD_CAPSULE_KEY10,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

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

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 10 for signing CPLD update capsule should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 10));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 10));
}

/**
 * @brief Execute oob-band key cancellation with pc type CPLD_UPDATE_CAPSULE and id 10.
 * Send key cancellation certificate to BMC flash and use CPLD active update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key10_for_cpld_capsule_through_oob_cpld_active_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_CPLD_CAPSULE_KEY10,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

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
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_ACTIVE_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 10 for signing CPLD update capsule should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 10));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 10));
}

/**
 * @brief Execute oob-band key cancellation with pc type CPLD_UPDATE_CAPSULE and id 10.
 * Send key cancellation certificate to BMC flash and use CPLD recover update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key10_for_cpld_capsule_through_oob_cpld_recovery_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_CPLD_CAPSULE_KEY10,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET);

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
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_CPLD_RECOVERY_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 10 for signing CPLD update capsule should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 10));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 10));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 10));
}

#ifdef GTEST_USE_CRYPTO_384
/**
 * @brief Execute oob-band key cancellation with pc type PCH_UPDATE_CAPSULE and id 2.
 * Send key cancellation certificate to BMC flash and use PCH recovery update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key2_with_secp256_for_pch_capsule_secp384_through_oob_pch_recovery_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_PCH_CAPSULE_KEY2_SECP256,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

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
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(3));

    // Key ID 2 for signing PCH update capsule should not be cancelled as they key can cert is wrong.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 2));
}
#else
/**
 * @brief Execute oob-band key cancellation with pc type PCH_UPDATE_CAPSULE and id 2.
 * Send key cancellation certificate to BMC flash and use PCH recovery update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key2_with_secp384_for_pch_capsule_secp256_through_oob_pch_recovery_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_PCH_CAPSULE_KEY2_SECP384,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET);

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
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(3));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(3));

    // Key ID 2 for signing PCH update capsule should not be cancelled as they are the wrong key cancellation cert.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 2));
}
#endif

#ifdef GTEST_ATTEST_384

/**
 * @brief Execute in-band key cancellation with pc type AFM and id 3.
 * Send key cancellation certificate to PCH flash and use PCH active update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key3_for_afm_through_ib_pch_active_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    // Perform provisioning
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_AFM_FILE, FULL_PFR_IMAGE_BMC_AFM_FILE_SIZE);

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, KEY_CAN_CERT_AFM_KEY3,
    		KEY_CAN_CERT_AFM_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

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
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 3 for signing AFM should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_AFM, 3));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 3));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 3));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 3));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 3));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 3));
}

/**
 * @brief Execute in-band key cancellation with pc type AFM and id 6.
 * Send key cancellation certificate to PCH flash and use PCH active update intent to
 * trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key6_for_afm_capsule_through_ib_pch_active_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    // Perform provisioning
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_AFM_FILE, FULL_PFR_IMAGE_BMC_AFM_FILE_SIZE);

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, KEY_CAN_CERT_AFM_KEY6,
    		KEY_CAN_CERT_AFM_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

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
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 6 for signing AFM update capsule should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_AFM, 6));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 6));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 6));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 6));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 6));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 6));
}

/**
 * @brief Execute oob-band key cancellation with pc type AFM and id 7.
 * Send key cancellation certificate to BMC flash and use BMC active update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key7_for_afm_through_oob_bmc_active_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    // Perform provisioning
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_AFM_FILE, FULL_PFR_IMAGE_BMC_AFM_FILE_SIZE);

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_AFM_KEY7,
    		KEY_CAN_CERT_AFM_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

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

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 7 for signing AFM should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_AFM, 7));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 7));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 7));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 7));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 7));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 7));
}

/**
 * @brief Execute oob-band key cancellation with pc type AFM and id 7.
 * Send key cancellation certificate to BMC flash and use BMC active update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key7_for_afm_through_oob_afm_active_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    // Perform provisioning
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);
    // Load the entire image to flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_AFM_FILE, FULL_PFR_IMAGE_BMC_AFM_FILE_SIZE);

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_AFM_KEY7,
    		KEY_CAN_CERT_AFM_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

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
    ut_send_in_update_intent_once_upon_boot_complete(MB_BMC_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_AFM_ACTIVE_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    // Expect no error
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // Key ID 7 for signing AFM should be cancelled now.
    // Policies for other PC type should not be affected
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_AFM, 7));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 7));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 7));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 7));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 7));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 7));
}

#endif


/**
 * @brief Execute oob-band key cancellation with pc type BMC_PFM and id 1.
 * Send key cancellation certificate to BMC flash and use BMC active update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key1_for_bmc_pfm_through_oob_bmc_active_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, KEY_CAN_CERT_BMC_PFM_KEY1,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->bmc_staging_region);

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

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_BMC_UPDATE_INTENT);

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

    // Key ID 1 for signing BMC PFM should not be cancelled as it is being used.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 1));
}

/**
 * @brief Execute in-band key cancellation with pc type PCH_PFM and id 0.
 * Send key cancellation certificate to PCH flash and use PCH active update intent
 * to trigger the cancellation.
 */
TEST_F(KeyCancellationFlowTest, test_cancel_key0_for_pch_pfm_through_ib_pch_active_fw_update)
{
    /*
     * Test Preparation
     */
    // Load the key cancellation certificate to staging region in flash
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, KEY_CAN_CERT_PCH_PFM_KEY0,
            KEY_CAN_CERT_FILE_SIZE, get_ufm_pfr_data()->pch_staging_region);

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
    ut_send_in_update_intent_once_upon_boot_complete(MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

    /*
     * Execute the flow.
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Nios should transition from T0 to T-1 once to process the key cancellation certificate
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_PCH_UPDATE_INTENT);

    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

    // Key ID 2 for signing PCH PFM should not be cancelled as it is being used.
    // Policies for other PC type should not be affected
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 0));
}

