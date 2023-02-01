#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "testdata_info.h"


class SeamlessUpdateFlowNegativeTest : public testing::Test
{
public:
    const alt_u32 m_bios_fv_regions_start_addr[PCH_NUM_BIOS_FV_REGIONS] = PCH_SPI_BIOS_FV_REGIONS_START_ADDR;
    const alt_u32 m_bios_fv_regions_end_addr[PCH_NUM_BIOS_FV_REGIONS] = PCH_SPI_BIOS_FV_REGIONS_END_ADDR;

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
 * @brief This test sends a Seamless update request with a Seamless capsule containing an unknown FV type.
 */
TEST_F(SeamlessUpdateFlowNegativeTest, test_seamless_update_with_unknown_fv_type)
{
    /*
      * Test Preparation
      */
     // Load a seamless capsule with an unknown FV type
     SYSTEM_MOCK::get()->load_to_flash(
             SPI_FLASH_PCH,
             SET2_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_FILE,
             SET2_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_FILE_SIZE,
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
     // Not using ut_send_in_update_intent_once_upon_boot_complete function here. By setup, it sends in
     //    update intent if the panic event count is 0 and in Seamless update, there's no panic event involved.
     write_to_mailbox(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

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
     EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_UNKNOWN_FV_TYPE));

     // Check # failed update attempts
     EXPECT_EQ(ut_get_num_failed_update_attempts(), alt_u32(1));

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
 * @brief This test sends a full pch update with invalid FV type.
 */
TEST_F(SeamlessUpdateFlowNegativeTest, test_sanity_full_pch_active_image_with_invalid_fv_type)
{
    /*
      * Test Preparation
      */

	 SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, PCH_SEAMLESS_FULL_IMAGE_BAD_UCODE1_FV_TYPE_50_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

     /*
      * Flow preparation
      */
     ut_prep_nios_gpi_signals();
     ut_send_block_complete_chkpt_msg();

     // Exit after 50 iterations in the T0 loop
     SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);
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
     EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_PCH_AUTH_FAILED));
     EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_ALL_REGIONS));

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
}

/**
 * @brief This test sends a full pch update with invalid FV type.
 */
TEST_F(SeamlessUpdateFlowNegativeTest, test_full_pch_active_ib_update_with_invalid_fv_type)
{
    /*
      * Test Preparation
      */

	 SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, PCH_SEAMLESS_FULL_IMAGE_GOOD_FV_TYPE_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);
     // Load a pch capsule with an invalid FV type
     SYSTEM_MOCK::get()->load_to_flash(
             SPI_FLASH_PCH,
			 SIGNED_CAPSULE_PCH_SEAMLESS_FULL_IMAGE_BAD_UCODE1_FV_TYPE_50_FILE,
			 SIGNED_CAPSULE_PCH_FILE_SIZE,
             get_ufm_pfr_data()->pch_staging_region
     );

     // Load the entire images locally for comparison purpose
     alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
     SYSTEM_MOCK::get()->init_x86_mem_from_file(PCH_SEAMLESS_FULL_IMAGE_GOOD_FV_TYPE_FILE, full_orig_image);

     /*
      * Flow preparation
      */
     ut_prep_nios_gpi_signals();
     ut_send_block_complete_chkpt_msg();

     // Exit after 50 iterations in the T0 loop
     SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

     // Send the active update intent
     ut_send_in_update_intent_once_upon_boot_complete(
             MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

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
     EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_UNKNOWN_FV_TYPE));

     // Check # failed update attempts
     EXPECT_EQ(ut_get_num_failed_update_attempts(), alt_u32(1));

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
 * @brief This test sends a full pch update with invalid FV type.
 */
TEST_F(SeamlessUpdateFlowNegativeTest, test_full_pch_recovery_ib_update_with_invalid_fv_type)
{
    /*
      * Test Preparation
      */

	 SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, PCH_SEAMLESS_FULL_IMAGE_GOOD_FV_TYPE_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);
     // Load a pch capsule with an invalid FV type
     SYSTEM_MOCK::get()->load_to_flash(
             SPI_FLASH_PCH,
			 SIGNED_CAPSULE_PCH_SEAMLESS_FULL_IMAGE_BAD_UCODE1_FV_TYPE_50_FILE,
			 SIGNED_CAPSULE_PCH_FILE_SIZE,
             get_ufm_pfr_data()->pch_staging_region
     );

     // Load the entire images locally for comparison purpose
     alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
     SYSTEM_MOCK::get()->init_x86_mem_from_file(PCH_SEAMLESS_FULL_IMAGE_GOOD_FV_TYPE_FILE, full_orig_image);

     /*
      * Flow preparation
      */
     ut_prep_nios_gpi_signals();
     ut_send_block_complete_chkpt_msg();

     // Exit after 50 iterations in the T0 loop
     SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

     // Send the active update intent
     ut_send_in_update_intent_once_upon_boot_complete(
             MB_PCH_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

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
     EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

     EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
     EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_PCH_UPDATE_INTENT));
     EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
     EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
     EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
     EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_UNKNOWN_FV_TYPE));

     // Check # failed update attempts
     EXPECT_EQ(ut_get_num_failed_update_attempts(), alt_u32(1));

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
 * @brief This test sends a full pch update with invalid FV type.
 */
TEST_F(SeamlessUpdateFlowNegativeTest, test_full_pch_recovery_oob_update_with_invalid_fv_type)
{
    /*
      * Test Preparation
      */

	 SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, PCH_SEAMLESS_FULL_IMAGE_GOOD_FV_TYPE_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);
     // Load a pch capsule with an invalid FV type
     SYSTEM_MOCK::get()->load_to_flash(
             SPI_FLASH_BMC,
			 SIGNED_CAPSULE_PCH_SEAMLESS_FULL_IMAGE_BAD_UCODE1_FV_TYPE_50_FILE,
			 SIGNED_CAPSULE_PCH_FILE_SIZE,
             get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET
     );

     // Load the entire images locally for comparison purpose
     alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
     SYSTEM_MOCK::get()->init_x86_mem_from_file(PCH_SEAMLESS_FULL_IMAGE_GOOD_FV_TYPE_FILE, full_orig_image);

     /*
      * Flow preparation
      */
     ut_prep_nios_gpi_signals();
     ut_send_block_complete_chkpt_msg();

     // Exit after 50 iterations in the T0 loop
     SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

     // Send the active update intent
     ut_send_in_update_intent_once_upon_boot_complete(
             MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_RECOVERY_MASK);

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
     EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

     EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
     EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_BMC_UPDATE_INTENT));
     EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
     EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
     EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
     EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_UNKNOWN_FV_TYPE));

     // Check # failed update attempts
     EXPECT_EQ(ut_get_num_failed_update_attempts(), alt_u32(1));

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
 * @brief This test sends a full pch update with invalid FV type.
 */
TEST_F(SeamlessUpdateFlowNegativeTest, test_full_pch_active_oob_update_with_invalid_fv_type)
{
    /*
      * Test Preparation
      */

	 SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, PCH_SEAMLESS_FULL_IMAGE_GOOD_FV_TYPE_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);
     // Load a pch capsule with an invalid FV type
     SYSTEM_MOCK::get()->load_to_flash(
             SPI_FLASH_BMC,
			 SIGNED_CAPSULE_PCH_SEAMLESS_FULL_IMAGE_BAD_UCODE1_FV_TYPE_50_FILE,
			 SIGNED_CAPSULE_PCH_FILE_SIZE,
             get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET
     );

     // Load the entire images locally for comparison purpose
     alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
     SYSTEM_MOCK::get()->init_x86_mem_from_file(PCH_SEAMLESS_FULL_IMAGE_GOOD_FV_TYPE_FILE, full_orig_image);

     /*
      * Flow preparation
      */
     ut_prep_nios_gpi_signals();
     ut_send_block_complete_chkpt_msg();

     // Exit after 50 iterations in the T0 loop
     SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS_END_AFTER_50_ITERS);

     // Send the active update intent
     ut_send_in_update_intent_once_upon_boot_complete(
             MB_BMC_UPDATE_INTENT_PART1, MB_UPDATE_INTENT_PCH_ACTIVE_MASK);

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
     EXPECT_EQ(SYSTEM_MOCK::get()->get_t_minus_1_counter(), alt_u32(2));

     EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
     EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_BMC_UPDATE_INTENT));
     EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(0));
     EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(0));
     EXPECT_EQ(read_from_mailbox(MB_MAJOR_ERROR_CODE), alt_u32(MAJOR_ERROR_UPDATE_FAILED));
     EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_UNKNOWN_FV_TYPE));

     // Check # failed update attempts
     EXPECT_EQ(ut_get_num_failed_update_attempts(), alt_u32(1));

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
 * @brief This test sends a Seamless update request with a Seamless capsule containing an unknown FV type.
 */
TEST_F(SeamlessUpdateFlowNegativeTest, test_spi_tampering_during_oob_seamless_update)
{
    /*
     * Test Preparation
     */
    // Load the Seamless FV capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_BMC,
            PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE,
            PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE_SIZE,
            get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET
    );

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_orig_image);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_image);

    alt_u32 *new_signed_fvm = new alt_u32[PCH_FW_UPDATE_SIGNED_FVM_PCH_BIOS_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(PCH_FW_UPDATE_SIGNED_FVM_PCH_BIOS_FILE, new_signed_fvm);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();
    // Block spi access durign seamless update
    ut_block_spi_access_during_seamless_update();

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
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

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

    // Make sure only data in BIOS FV regions are changed
    for (alt_u32 region_i = 0; region_i < PCH_NUM_STATIC_SPI_REGIONS; region_i++)
    {
        // If this region is a BIOS FV region, compare against full_new_image
        alt_u32* gold_image = full_orig_image;
        /*for (alt_u32 addr_i = 0; addr_i < PCH_NUM_BIOS_FV_REGIONS; addr_i++)
        {
            if (testdata_pch_static_regions_start_addr[region_i] == m_bios_fv_regions_start_addr[addr_i])
            {
                gold_image = full_new_image;
            }
        }*/

        // Iterate through all words in this SPI region and make sure they match the expected data
        for (alt_u32 word_i = testdata_pch_static_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_static_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(gold_image[word_i], flash_x86_ptr[word_i]);
        }
    }
    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        // By default, the dynamic SPI regions are not updated in Seamless update.
        alt_u32* gold_image = full_orig_image;

        // Iterate through all words in this SPI region and make sure they match the expected data
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(gold_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify the active PFR region
    // PFM should not change. Only one FVM will be updated.
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + SIGNED_PFM_MAX_SIZE;

    // Info on updated FVM
    alt_u16 target_fv_type = ((FVM*) incr_alt_u32_ptr(new_signed_fvm, SIGNATURE_SIZE))->fv_type;
    alt_u32 target_fvm_start_addr = get_active_fvm_addr(target_fv_type);
    alt_u32 target_fvm_end_addr = target_fvm_start_addr + (4096 - PCH_FW_UPDATE_SIGNED_FVM_PCH_BIOS_FILE_SIZE % 4096);

    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        if (((target_fvm_start_addr >> 2) <= word_i) && (word_i < (target_fvm_end_addr >> 2)))
        {
            // Check against the expected new FVM, if the address falls within this FVM in active PFR region
            /*alt_u32 new_signed_fvm_word_i = word_i - (target_fvm_start_addr >> 2);
            if (new_signed_fvm_word_i < (PCH_FW_UPDATE_SIGNED_FVM_PCH_BIOS_FILE_SIZE >> 2))
            {
                ASSERT_EQ(new_signed_fvm[new_signed_fvm_word_i], flash_x86_ptr[word_i]);
            }
            else
            {
                // Data would be blank (0xFFFFFFFF) for the remaining of the page that this FVM occupies
                ASSERT_EQ(alt_u32(0xFFFFFFFF), flash_x86_ptr[word_i]);
            }*/
        }
        else
        {
            // The rest of Active region should be unchanged
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // The original update capsule should be valid because a seamless update is not performed
    switch_spi_flash(SPI_FLASH_BMC);
    EXPECT_TRUE(is_signature_valid(get_ufm_pfr_data()->bmc_staging_region + BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
    delete[] new_signed_fvm;
}

/**
 * @brief This test sends a Seamless update request with a Seamless capsule containing an unknown FV type.
 */
TEST_F(SeamlessUpdateFlowNegativeTest, test_spi_tampering_during_ib_seamless_update)
{
    /*
     * Test Preparation
     */
    // Load the Seamless FV capsule
    SYSTEM_MOCK::get()->load_to_flash(
            SPI_FLASH_PCH,
            PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE,
            PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE_SIZE,
            get_ufm_pfr_data()->pch_staging_region
    );

    // Load the entire images locally for comparison purpose
    alt_u32 *full_orig_image = new alt_u32[FULL_PFR_IMAGE_PCH_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_FILE, full_orig_image);

    alt_u32 *full_new_image = new alt_u32[FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(FULL_PFR_IMAGE_PCH_V03P12_FILE, full_new_image);

    alt_u32 *new_signed_fvm = new alt_u32[PCH_FW_UPDATE_SIGNED_FVM_PCH_BIOS_FILE_SIZE/4];
    SYSTEM_MOCK::get()->init_x86_mem_from_file(PCH_FW_UPDATE_SIGNED_FVM_PCH_BIOS_FILE, new_signed_fvm);

    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();
    ut_send_block_complete_chkpt_msg();
    // Block spi access durign seamless update
    ut_block_spi_access_during_seamless_update();

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
    EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTH_FAILED_AFTER_SEAMLESS_UPDATE));

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

    for (alt_u32 region_i = 0; region_i < PCH_NUM_DYNAMIC_SPI_REGIONS; region_i++)
    {
        // By default, the dynamic SPI regions are not updated in Seamless update.
        alt_u32* gold_image = full_orig_image;

        // Iterate through all words in this SPI region and make sure they match the expected data
        for (alt_u32 word_i = testdata_pch_dynamic_regions_start_addr[region_i] >> 2;
                word_i < testdata_pch_dynamic_regions_end_addr[region_i] >> 2; word_i++)
        {
            ASSERT_EQ(gold_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // Verify the active PFR region
    // PFM should not change. Only one FVM will be updated.
    alt_u32 pch_active_pfm_start = get_ufm_pfr_data()->pch_active_pfm;
    alt_u32 pch_active_pfm_end = pch_active_pfm_start + SIGNED_PFM_MAX_SIZE;

    // Info on updated FVM
    alt_u16 target_fv_type = ((FVM*) incr_alt_u32_ptr(new_signed_fvm, SIGNATURE_SIZE))->fv_type;
    alt_u32 target_fvm_start_addr = get_active_fvm_addr(target_fv_type);
    alt_u32 target_fvm_end_addr = target_fvm_start_addr + (4096 - PCH_FW_UPDATE_SIGNED_FVM_PCH_BIOS_FILE_SIZE % 4096);

    for (alt_u32 word_i = pch_active_pfm_start >> 2; word_i < pch_active_pfm_end >> 2; word_i++)
    {
        if (((target_fvm_start_addr >> 2) <= word_i) && (word_i < (target_fvm_end_addr >> 2)))
        {
            // Check against the expected new FVM, if the address falls within this FVM in active PFR region
            /*alt_u32 new_signed_fvm_word_i = word_i - (target_fvm_start_addr >> 2);
            if (new_signed_fvm_word_i < (PCH_FW_UPDATE_SIGNED_FVM_PCH_BIOS_FILE_SIZE >> 2))
            {
                ASSERT_EQ(new_signed_fvm[new_signed_fvm_word_i], flash_x86_ptr[word_i]);
            }
            else
            {
                // Data would be blank (0xFFFFFFFF) for the remaining of the page that this FVM occupies
                ASSERT_EQ(alt_u32(0xFFFFFFFF), flash_x86_ptr[word_i]);
            }*/
        }
        else
        {
            // The rest of Active region should be unchanged
            ASSERT_EQ(full_orig_image[word_i], flash_x86_ptr[word_i]);
        }
    }

    // The original update capsule should be invalidated after seamless update operation is done
    switch_spi_flash(SPI_FLASH_PCH);
    EXPECT_FALSE(is_signature_valid(get_ufm_pfr_data()->pch_staging_region));

    /*
     * Clean ups
     */
    delete[] full_orig_image;
    delete[] full_new_image;
    delete[] new_signed_fvm;
}

/**
 * @brief This test sends a Seamless update request with a Seamless capsule containing an unknown FV type.
 */
TEST_F(SeamlessUpdateFlowNegativeTest, test_bios_seamless_update_with_incorrect_addr)
{
    /*
      * Test Preparation
      */
     // Load a seamless capsule with an unknown FV type
     SYSTEM_MOCK::get()->load_to_flash(
             SPI_FLASH_PCH,
			 PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_WRONG_SPI_ADDR_FILE,
			 PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_WRONG_SPI_ADDR_FILE_SIZE,
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
     // Not using ut_send_in_update_intent_once_upon_boot_complete function here. By setup, it sends in
     //    update intent if the panic event count is 0 and in Seamless update, there's no panic event involved.
     write_to_mailbox(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

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
     EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

     // Check # failed update attempts
     EXPECT_EQ(ut_get_num_failed_update_attempts(), alt_u32(1));

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
 * @brief This test sends a Seamless update request with a Seamless capsule containing an unknown FV type.
 */
TEST_F(SeamlessUpdateFlowNegativeTest, test_me_seamless_update_with_incorrect_start_addr)
{
    /*
      * Test Preparation
      */
     // Load a seamless capsule with an unknown FV type
     SYSTEM_MOCK::get()->load_to_flash(
             SPI_FLASH_PCH,
			 PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_ME_WRONG_SPI_START_ADDR_FILE,
			 PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_ME_WRONG_SPI_START_ADDR_FILE_SIZE,
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
     // Not using ut_send_in_update_intent_once_upon_boot_complete function here. By setup, it sends in
     //    update intent if the panic event count is 0 and in Seamless update, there's no panic event involved.
     write_to_mailbox(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

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
     EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

     // Check # failed update attempts
     EXPECT_EQ(ut_get_num_failed_update_attempts(), alt_u32(1));

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
 * @brief This test sends a Seamless update request with a Seamless capsule containing an unknown FV type.
 */
TEST_F(SeamlessUpdateFlowNegativeTest, test_ucode1_seamless_update_with_incorrect_end_addr)
{
    /*
      * Test Preparation
      */
     // Load a seamless capsule with an unknown FV type
     SYSTEM_MOCK::get()->load_to_flash(
             SPI_FLASH_PCH,
			 PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_WRONG_SPI_END_ADDR_FILE,
			 PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_WRONG_SPI_END_ADDR_FILE_SIZE,
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
     // Not using ut_send_in_update_intent_once_upon_boot_complete function here. By setup, it sends in
     //    update intent if the panic event count is 0 and in Seamless update, there's no panic event involved.
     write_to_mailbox(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

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
     EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

     // Check # failed update attempts
     EXPECT_EQ(ut_get_num_failed_update_attempts(), alt_u32(1));

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
 * @brief This test sends a Seamless update request with a Seamless capsule containing an unknown FV type.
 */
TEST_F(SeamlessUpdateFlowNegativeTest, test_ucode2_seamless_update_with_incorrect_addr)
{
    /*
      * Test Preparation
      */
     // Load a seamless capsule with an unknown FV type
     SYSTEM_MOCK::get()->load_to_flash(
             SPI_FLASH_PCH,
			 PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_WRONG_SPI_ADDR_FILE,
			 PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_WRONG_SPI_ADDR_FILE_SIZE,
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
     // Not using ut_send_in_update_intent_once_upon_boot_complete function here. By setup, it sends in
     //    update intent if the panic event count is 0 and in Seamless update, there's no panic event involved.
     write_to_mailbox(MB_PCH_UPDATE_INTENT_PART2, MB_UPDATE_INTENT_PCH_FV_SEAMLESS_MASK);

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
     EXPECT_EQ(read_from_mailbox(MB_MINOR_ERROR_CODE), alt_u32(MINOR_ERROR_AUTHENTICATION_FAILED));

     // Check # failed update attempts
     EXPECT_EQ(ut_get_num_failed_update_attempts(), alt_u32(1));

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
