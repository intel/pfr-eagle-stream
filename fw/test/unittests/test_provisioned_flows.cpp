// Unit test for the PFR system flows

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "ut_checks.h"


class PFRProvisionedFlowsTest : public testing::Test
{
public:
    virtual void SetUp() {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        // Reset system mocks and SPI flash
        sys->reset();
        sys->reset_spi_flash_mock();

        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

        // Prepare the flashes
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);
        SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);

        // Reset Nios firmware
        ut_reset_nios_fw();
    }

    virtual void TearDown() {}
};

TEST_F(PFRProvisionedFlowsTest, test_provisioned_tmin1_to_t0)
{
    // Timed boot is only enabled in provisioned state.
    EXPECT_TRUE(is_ufm_provisioned());

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Insert the T0_TIMED_BOOT code block (break out of T0 loop when all timer has stopped)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_TIMED_BOOT);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_MEDIUM, pfr_main());

    // Check observed vs expected global_state
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_T0_BOOT_COMPLETE);

    // Expect no recovery or panic event
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));

    // SPI filters should be enabled
    EXPECT_FALSE(check_bit(U_GPO_1_ADDR, GPO_1_BMC_SPI_FILTER_DISABLE));
    EXPECT_FALSE(check_bit(U_GPO_1_ADDR, GPO_1_PCH_SPI_FILTER_DISABLE));

    // Check the SPI filter rules in Write Enable Memory
    ut_check_all_spi_filtering_rules();

    // SMBUS filters should be enabled
    EXPECT_FALSE(check_bit(U_GPO_1_ADDR, GPO_1_RELAY1_FILTER_DISABLE));
    EXPECT_FALSE(check_bit(U_GPO_1_ADDR, GPO_1_RELAY2_FILTER_DISABLE));
    EXPECT_FALSE(check_bit(U_GPO_1_ADDR, GPO_1_RELAY3_FILTER_DISABLE));

    // Expect all the i_relay_all_address to be unset after provisioning
    EXPECT_TRUE(check_bit(U_GPO_1_ADDR, GPO_1_RELAY1_ALL_ADDRESSES));
    EXPECT_TRUE(check_bit(U_GPO_1_ADDR, GPO_1_RELAY2_ALL_ADDRESSES));
    EXPECT_TRUE(check_bit(U_GPO_1_ADDR, GPO_1_RELAY3_ALL_ADDRESSES));
}

TEST_F(PFRProvisionedFlowsTest, test_panic_event_caused_by_acm_auth_failure_during_timed_boot)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Add some code blocks to test this flow path
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send authentication failure checkpoint message to BIOS checkpoint once
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data)
            {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* bmc_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BMC_CHECKPOINT;
            alt_u32* acm_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_ACM_CHECKPOINT;
            alt_u32* bios_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BIOS_CHECKPOINT;

            if (addr == bmc_ckpt_addr)
            {
                SYSTEM_MOCK::get()->set_mem_word(bmc_ckpt_addr, MB_CHKPT_COMPLETE, true);
            }
            else if (addr == acm_ckpt_addr)
            {
                // Send the AUTH_FAIL ACM checkpoint message once
                if ((read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                {
                    SYSTEM_MOCK::get()->set_mem_word(acm_ckpt_addr, MB_CHKPT_AUTH_FAIL, true);
                }
                else
                {
                    SYSTEM_MOCK::get()->set_mem_word(acm_ckpt_addr, MB_CHKPT_COMPLETE, true);
                }
            }
            else if (addr == bios_ckpt_addr)
            {
                if (wdt_boot_status & WDT_ACM_BOOT_DONE_MASK)
                {
                    // ACM has booted. Now IBB is supposed to be booting.
                    // Send COMPLETE to complete IBB and OBB boots
                    SYSTEM_MOCK::get()->set_mem_word(bios_ckpt_addr, MB_CHKPT_COMPLETE, true);

                    if ((read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                    {
                        // Send authentication failure in the first attempt
                        SYSTEM_MOCK::get()->set_mem_word(bios_ckpt_addr, MB_CHKPT_AUTH_FAIL, true);
                    }
                    else
                    {

                    }
                }
                else if (read_from_mailbox(MB_PANIC_EVENT_COUNT) > 0)
                {
                    // Wait until CPLD recover ACM after it reports authentication failure
                    // Sends BOOT_START to BIOS checkpoint, when ACM is booting.
                    // Nios is supposed to turn off ACM WDT when IBB starts to boot.
                    SYSTEM_MOCK::get()->set_mem_word(bios_ckpt_addr, MB_CHKPT_START, true);
                }
            }
        }
            });

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    // Check observed vs expected global_state
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_ACM_BIOS_AUTH_FAILED);
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_ACM_LAUNCH_FAIL);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
}

TEST_F(PFRProvisionedFlowsTest, test_panic_event_caused_by_ibb_auth_failure_during_timed_boot)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Add some code blocks to test this flow path
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Send authentication failure checkpoint message to BIOS checkpoint once
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data)
            {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* bmc_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BMC_CHECKPOINT;
            alt_u32* acm_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_ACM_CHECKPOINT;
            alt_u32* bios_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BIOS_CHECKPOINT;

            if (addr == bmc_ckpt_addr)
            {
                SYSTEM_MOCK::get()->set_mem_word(bmc_ckpt_addr, MB_CHKPT_COMPLETE, true);
            }
            else if (addr == acm_ckpt_addr)
            {
                // Nios is supposed to ignore the BOOT_START/BOOT_DONE checkpoint messages from ACM.
                // Sending this shouldn't hurt.
                SYSTEM_MOCK::get()->set_mem_word(acm_ckpt_addr, MB_CHKPT_COMPLETE, true);
            }
            else if (addr == bios_ckpt_addr)
            {
                if (wdt_boot_status & WDT_ACM_BOOT_DONE_MASK)
                {
                    // ACM has booted. Now IBB is supposed to be booting.
                    if ((read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                    {
                        // Send authentication failure in the first attempt
                        SYSTEM_MOCK::get()->set_mem_word(bios_ckpt_addr, MB_CHKPT_AUTH_FAIL, true);
                    }
                    else
                    {
                        // Send COMPLETE to complete IBB and OBB boots
                        SYSTEM_MOCK::get()->set_mem_word(bios_ckpt_addr, MB_CHKPT_COMPLETE, true);
                    }
                }
                else
                {
                    // Sends BOOT_START to BIOS checkpoint, when ACM is booting.
                    // Nios is supposed to turn off ACM WDT when IBB starts to boot.
                    SYSTEM_MOCK::get()->set_mem_word(bios_ckpt_addr, MB_CHKPT_START, true);
                }
            }
        }
            });

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_LONG, pfr_main());

    // Check observed vs expected global_state
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), LAST_PANIC_ACM_BIOS_AUTH_FAILED);
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), LAST_RECOVERY_IBB_LAUNCH_FAIL);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
}

/*
 * Check the T0 behaviour after provisioning.
 * During testing on platform, I noticed that the watchdog timer immediately timed out after provisioning.
 * That bug was fixed since. This test to make sure that does not happen again.
 */
TEST_F(PFRProvisionedFlowsTest, test_t0_behavior_after_provisioning)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Erase provisioned data
    SYSTEM_MOCK::get()->erase_ufm_page(0);
    EXPECT_FALSE(is_ufm_provisioned());

    // Ends pfr_main after getting to T0
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Check expected global state
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_ENTER_T0);

    /*
     * Perform provisioning.
     */
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

    /*
     * Continue the T0 loop
     */
    // Skip after 12 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Run T0 loop. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, perform_t0_operations());

    // Platform state should remain unchanged
    EXPECT_EQ(ut_get_global_state(), (alt_u32) PLATFORM_STATE_ENTER_T0);
    EXPECT_EQ(read_from_mailbox(MB_PLATFORM_STATE), (alt_u32) PLATFORM_STATE_ENTER_T0);

    // Expect no recovery or panic event
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(0));
}

/*
 * @brief This test corrupts the BMC SPI flash and attempt to boot the system.
 * CPLD should not release BMC or PCH from reset, when BMC flash is corrupted.
 */
TEST_F(PFRProvisionedFlowsTest, test_t0_behavior_when_bmc_flash_is_corrupted)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Corrupt BMC flash
    switch_spi_flash(SPI_FLASH_BMC);
    alt_u32* signed_active_pfm = get_spi_active_pfm_ptr(SPI_FLASH_BMC);
    *signed_active_pfm = 0;
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_BMC);
    *signed_recovery_capsule = 0;

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_MEDIUM, pfr_main());

    // Expect no recovery or panic event
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_BMC_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ALL_REGIONS));

    // SPI filters should be enabled regardless authentication result
    EXPECT_FALSE(check_bit(U_GPO_1_ADDR, GPO_1_PCH_SPI_FILTER_DISABLE));
    EXPECT_FALSE(check_bit(U_GPO_1_ADDR, GPO_1_BMC_SPI_FILTER_DISABLE));

    // Both BMC and PCH should be in reset
    EXPECT_FALSE(ut_is_bmc_out_of_reset());
    EXPECT_FALSE(ut_is_pch_out_of_reset());

    // No watchdog timer should be active
    EXPECT_FALSE(check_bit(WDT_BMC_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    EXPECT_FALSE(check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    EXPECT_FALSE(check_bit(WDT_ME_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
}

TEST_F(PFRProvisionedFlowsTest, test_boot_bmc_only_when_pch_flash_is_corrupted)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop upon entry)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

    // Corrupt PCH flash
    switch_spi_flash(SPI_FLASH_PCH);
    alt_u32* signed_active_pfm = get_spi_active_pfm_ptr(SPI_FLASH_PCH);
    *signed_active_pfm = 0;
    alt_u32* signed_recovery_capsule = get_spi_recovery_region_ptr(SPI_FLASH_PCH);
    *signed_recovery_capsule = 0;

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Expect no recovery or panic event
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
    EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ALL_REGIONS));

    // SPI filters should be enabled regardless authentication result
    EXPECT_FALSE(check_bit(U_GPO_1_ADDR, GPO_1_PCH_SPI_FILTER_DISABLE));
    EXPECT_FALSE(check_bit(U_GPO_1_ADDR, GPO_1_BMC_SPI_FILTER_DISABLE));

    // SMBUS filters should be enabled
    // TODO: Re-enable these checks once the expected behaviour is confirmed
    //    EXPECT_FALSE(check_bit(U_GPO_1_ADDR, GPO_1_RELAY1_FILTER_DISABLE));
    //    EXPECT_FALSE(check_bit(U_GPO_1_ADDR, GPO_1_RELAY2_FILTER_DISABLE));
    //    EXPECT_FALSE(check_bit(U_GPO_1_ADDR, GPO_1_RELAY3_FILTER_DISABLE));

    // BMC should be out of reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());

    // PCH should still be in reset
    EXPECT_FALSE(ut_is_pch_out_of_reset());

    // Both BMC and PCH timers should be inactive
    EXPECT_FALSE(check_bit(WDT_BMC_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    EXPECT_FALSE(check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));

    /*
     * Some event (e.g. BMC active update) triggered BMC only reset
     * BMC and PCH WDTs should all still be inactive. PCH should still be in reset.
     */
    perform_bmc_only_reset();

    // BMC should be out of reset
    EXPECT_TRUE(ut_is_bmc_out_of_reset());

    // PCH should still be in reset
    EXPECT_FALSE(ut_is_pch_out_of_reset());

    // Both BMC and PCH timers should be inactive
    EXPECT_FALSE(check_bit(WDT_BMC_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
    EXPECT_FALSE(check_bit(WDT_ACM_BIOS_TIMER_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
}
