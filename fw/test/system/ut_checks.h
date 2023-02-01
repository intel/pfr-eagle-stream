#ifndef UNITTEST_SYSTEM_UT_CHECKS_H_
#define UNITTEST_SYSTEM_UT_CHECKS_H_

#include "ut_common.h"


/***************************************
 *
 * Checkers for SPI filtering rules configuration
 *
 ***************************************/
/**
 * @brief Return 1 if the given page in the given SPI flash is writable, according to the
 * Write Enable memory inside its SPI filter.
 *
 * @see apply_spi_write_protection
 */
static alt_u32 ut_is_page_writable(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 addr)
{
    switch_spi_flash(spi_flash_type);
    alt_u32* we_mem = get_spi_we_mem_ptr();

    // 12 bits to get to 4kB chunks, 5 more bits because we have 32 (=2^5) bits in each word in the memory
    alt_u32 word_pos = addr >> (12 + 5);
    // Grab the 5 address bits that tell us the starting bit position within the 32 bit word
    alt_u32 bit_pos = (addr >> 12) & 0x0000001f;

    // If bit == 1 in WE memory, it means that this page is writable.
    return check_bit(we_mem + word_pos, bit_pos);
}

static void ut_check_spi_filtering_rules_for_regions(
        SPI_FLASH_TYPE_ENUM spi_flash_type,
        const alt_u32* spi_regions_start_addr,
        const alt_u32* spi_regions_end_addr,
        alt_u32 num_regions,
        alt_u32 are_writable)
{
    for (alt_u32 region_i = 0; region_i < num_regions; region_i++)
    {
        // Check the Write Protection setting on 3 pages per SPI region
        alt_u32 check_addr1 = spi_regions_start_addr[region_i];
        alt_u32 check_addr2 = spi_regions_end_addr[region_i] - 4;
        alt_u32 check_addr3 = check_addr1 + ((spi_regions_end_addr[region_i] - check_addr1) / 2);

        if (ut_is_page_writable(spi_flash_type, check_addr1) != are_writable)
        {
            std::cerr << "Error: The SPI filtering rule for address " << check_addr1 << " in region " << \
                    spi_regions_start_addr[region_i] << " - " << spi_regions_end_addr[region_i] << " is wrong. " << std::endl;
            EXPECT_TRUE(0);
        }

        if (ut_is_page_writable(spi_flash_type, check_addr2) != are_writable)
        {
            std::cerr << "Error: The SPI filtering rule for address " << check_addr2 << " in region " << \
                    spi_regions_start_addr[region_i] << " - " << spi_regions_end_addr[region_i] << " is wrong. " << std::endl;
            EXPECT_TRUE(0);
        }

        if (ut_is_page_writable(spi_flash_type, check_addr3) != are_writable)
        {
            std::cerr << "Error: The SPI filtering rule for address " << check_addr3 << " in region " << \
                    spi_regions_start_addr[region_i] << " - " << spi_regions_end_addr[region_i] << " is wrong. " << std::endl;
            EXPECT_TRUE(0);
        }
    }
}

static void ut_check_all_spi_filtering_rules()
{
    // PCH SPI regions
    ut_check_spi_filtering_rules_for_regions(
            SPI_FLASH_PCH, testdata_pch_static_regions_start_addr, testdata_pch_static_regions_end_addr,
            PCH_NUM_STATIC_SPI_REGIONS, 0);
    ut_check_spi_filtering_rules_for_regions(
            SPI_FLASH_PCH, testdata_pch_dynamic_regions_start_addr, testdata_pch_dynamic_regions_end_addr,
            PCH_NUM_DYNAMIC_SPI_REGIONS, 1);

    // PCH PFR SPI regions
    const alt_u32 pch_read_only_pfr_regions_start_addr[2] = {
            get_active_region_offset(SPI_FLASH_PCH),
            get_recovery_region_offset(SPI_FLASH_PCH)
    };
    const alt_u32 pch_read_only_pfr_regions_end_addr[2] = {
            get_active_region_offset(SPI_FLASH_PCH) + SIGNED_PFM_MAX_SIZE,
            get_recovery_region_offset(SPI_FLASH_PCH) + SPI_FLASH_PCH_RECOVERY_SIZE
    };

    const alt_u32 pch_writable_pfr_regions_start_addr[1] = {
            get_staging_region_offset(SPI_FLASH_PCH)
    };
    const alt_u32 pch_writable_pfr_regions_end_addr[1] = {
            get_staging_region_offset(SPI_FLASH_PCH) + SPI_FLASH_PCH_STAGING_SIZE
    };

    ut_check_spi_filtering_rules_for_regions(
            SPI_FLASH_PCH, pch_read_only_pfr_regions_start_addr, pch_read_only_pfr_regions_end_addr, 2, 0);

    ut_check_spi_filtering_rules_for_regions(
            SPI_FLASH_PCH, pch_writable_pfr_regions_start_addr, pch_writable_pfr_regions_end_addr, 1, 1);

    // BMC SPI regions
    ut_check_spi_filtering_rules_for_regions(
            SPI_FLASH_BMC, testdata_bmc_static_regions_start_addr, testdata_bmc_static_regions_end_addr,
            BMC_NUM_STATIC_SPI_REGIONS, 0);
    ut_check_spi_filtering_rules_for_regions(
            SPI_FLASH_BMC, testdata_bmc_dynamic_regions_start_addr, testdata_bmc_dynamic_regions_end_addr,
            BMC_NUM_DYNAMIC_SPI_REGIONS, 1);

    // BMC PFR SPI regions
    const alt_u32 bmc_read_only_pfr_regions_start_addr[2] = {
            get_active_region_offset(SPI_FLASH_BMC),
            get_recovery_region_offset(SPI_FLASH_BMC)
    };
    const alt_u32 bmc_read_only_pfr_regions_end_addr[2] = {
            get_active_region_offset(SPI_FLASH_BMC) + SIGNED_PFM_MAX_SIZE,
            get_recovery_region_offset(SPI_FLASH_BMC) + SPI_FLASH_BMC_RECOVERY_SIZE
    };

    const alt_u32 bmc_writable_pfr_regions_start_addr[1] = {
            get_staging_region_offset(SPI_FLASH_BMC)
    };
    const alt_u32 bmc_writable_pfr_regions_end_addr[1] = {
            get_staging_region_offset(SPI_FLASH_BMC) + SPI_FLASH_BMC_STAGING_SIZE
    };

    ut_check_spi_filtering_rules_for_regions(
            SPI_FLASH_BMC, bmc_read_only_pfr_regions_start_addr, bmc_read_only_pfr_regions_end_addr, 2, 0);
    ut_check_spi_filtering_rules_for_regions(
            SPI_FLASH_BMC, bmc_writable_pfr_regions_start_addr, bmc_writable_pfr_regions_end_addr, 1, 1);
}

/***************************************
 *
 * Common routine/checks for replay attack unittests
 *
 ***************************************/
static void ut_common_routine_for_replay_attack_tests(
        std::vector<std::pair<MB_REGFILE_OFFSET_ENUM, MB_UPDATE_INTENT_PART1_MASK_ENUM>> update_scenarios)
{
    // Flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Run PFR Main to reach to T0 first. Always run with the timeout
    ASSERT_DURATION_LE(10, pfr_main());

    // Iterate through all update scenarios and apply each update
    for (alt_u32 i = 0; i < update_scenarios.size(); i++)
    {
        // Write the update intent to CPLD mailbox
        MB_REGFILE_OFFSET_ENUM update_intent_reg = update_scenarios[i].first;
        MB_UPDATE_INTENT_PART1_MASK_ENUM update_intent = update_scenarios[i].second;
        write_to_mailbox(update_intent_reg, update_intent);

        // Execute the update flow
        mb_update_intent_handler();

        // Verify result
        if (i < MAX_FAILED_UPDATE_ATTEMPTS_ALLOWED)
        {
            // This update was supposed to be allowed

            if (i == (MAX_FAILED_UPDATE_ATTEMPTS_ALLOWED - 1))
            {
                // Nios should not be blocking update yet
                EXPECT_TRUE(ut_nios_is_blocking_update());
            }
            else
            {
                EXPECT_FALSE(ut_nios_is_blocking_update());
            }

            // There should be one T-1 entry for this update
            EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(i + 1));

            // This update should fail due to authentication error, since the update capsule is absent.
            EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
            EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));
        }
        else
        {
            // Nios should be blocking update from now on
            EXPECT_TRUE(ut_nios_is_blocking_update());

            // T-1 entries should be capped
            EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_ALLOWED));

            // This update should be rejected since Nios has reached max failed update attempts
            EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
            EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
        }

        if (testing::Test::HasNonfatalFailure())
        {
            // Found some error in this unittest. Provide some information regarding this update.
            std::cerr << "[Error] Gtest found non-fatal failure. " << std::endl;
            std::cerr << "[Error] In main test loop (iteration " << i << "). Wrote " << update_intent << " to Mailbox register " << update_intent_reg << ". " << std::endl;
        }
    }
}

static void ut_common_routine_for_replay_attack_tests_part_2(
        std::vector<std::pair<MB_REGFILE_OFFSET_ENUM, MB_UPDATE_INTENT_PART2_MASK_ENUM>> update_scenarios)
{
    // Flow preparation
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();

    // Exit after 50 iterations in the T0 loop
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
    // Skip authentication in T-1
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_FLASH_AUTHENTICATION);

    // Run PFR Main to reach to T0 first. Always run with the timeout
    ASSERT_DURATION_LE(10, pfr_main());

    // Iterate through all update scenarios and apply each update
    for (alt_u32 i = 0; i < update_scenarios.size(); i++)
    {
        // Write the update intent to CPLD mailbox
        MB_REGFILE_OFFSET_ENUM update_intent_reg = update_scenarios[i].first;
        MB_UPDATE_INTENT_PART2_MASK_ENUM update_intent = update_scenarios[i].second;
        write_to_mailbox(update_intent_reg, update_intent);

        // Execute the update flow
        mb_update_intent_handler();

        // Verify result
        if (i < MAX_FAILED_UPDATE_ATTEMPTS_ALLOWED)
        {
            // This update was supposed to be allowed

            if (i == (MAX_FAILED_UPDATE_ATTEMPTS_ALLOWED - 1))
            {
                // Nios should not be blocking update yet
                EXPECT_TRUE(ut_nios_is_blocking_update());
            }
            else
            {
                EXPECT_FALSE(ut_nios_is_blocking_update());
            }

            // There should be one T-1 entry for this update
            EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(i + 1));

            // This update should fail due to authentication error, since the update capsule is absent.
            EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
            EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));
        }
        else
        {
            // Nios should be blocking update from now on
            EXPECT_TRUE(ut_nios_is_blocking_update());

            // T-1 entries should be capped
            EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(MAX_FAILED_UPDATE_ATTEMPTS_ALLOWED));

            // This update should be rejected since Nios has reached max failed update attempts
            EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
            EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS));
        }

        if (testing::Test::HasNonfatalFailure())
        {
            // Found some error in this unittest. Provide some information regarding this update.
            std::cerr << "[Error] Gtest found non-fatal failure. " << std::endl;
            std::cerr << "[Error] In main test loop (iteration " << i << "). Wrote " << update_intent << " to Mailbox register " << update_intent_reg << ". " << std::endl;
        }
    }
}

#endif /* UNITTEST_SYSTEM_UT_CHECKS_H_ */
