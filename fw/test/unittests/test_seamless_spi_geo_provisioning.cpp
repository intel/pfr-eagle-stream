// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"


class UFMProvisioningSpiGeoTest : public testing::Test
{
public:
	UFM_PFR_DATA* m_ufm_data;

    virtual void SetUp() {
        SYSTEM_MOCK::get()->reset();
        m_ufm_data = (UFM_PFR_DATA*) get_ufm_pfr_data();
    }

    virtual void TearDown() {}
};

/*
 * @brief sanity test for ufm
 *
 */
TEST_F(UFMProvisioningSpiGeoTest, test_sanity_system_ufm_spi_geo)
{
    EXPECT_EQ(((alt_u32*) m_ufm_data)[0], (alt_u32) 0xFFFFFFFF);
    EXPECT_EQ(((alt_u32*) m_ufm_data)[8], (alt_u32) 0xFFFFFFFF);
    EXPECT_EQ(((alt_u32*) m_ufm_data)[12], (alt_u32) 0xFFFFFFFF);
    EXPECT_EQ(((alt_u32*) m_ufm_data)[20], (alt_u32) 0xFFFFFFFF);
}

/*
 * @brief test spi geometry provisioning
 */
TEST_F(UFMProvisioningSpiGeoTest, test_spi_geo_provisioning)
{
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_SPI_GEO_EXAMPLE_KEY_FILE);
    // UFM should not be locked
    EXPECT_FALSE(is_ufm_locked());
    // UFM should not be provisioned
    EXPECT_FALSE(is_ufm_provisioned());
    // SPI GEO should be provisioned
    EXPECT_TRUE(is_ufm_spi_geo_provisioned());

    // No root key hash is present
    EXPECT_EQ(m_ufm_data->root_key_hash_384[0], (alt_u32) 0xffffffff);
    EXPECT_EQ(m_ufm_data->root_key_hash_256[0], (alt_u32) 0xffffffff);
}

/*
 * @brief Check ufm status after spi geo provisioning
 */
TEST_F(UFMProvisioningSpiGeoTest, test_check_stat_after_spi_geo_provisioning)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);
    // Skip all T-1 operations
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_TMIN1_OPERATIONS);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check the UFM status.
     * Should not be provisioned or locked
     */
    alt_u32 status = read_from_mailbox(MB_PROVISION_STATUS);
    EXPECT_FALSE(MB_UFM_PROV_UFM_PROVISIONED_MASK & status);
    EXPECT_FALSE(MB_UFM_PROV_UFM_LOCKED_MASK & status);

    // Check if SPI GEO has been provisioned
    // SPI GEO should not be provisioned
    EXPECT_FALSE(is_ufm_spi_geo_provisioned());

    /*
     * Provision UFM SPI GEO data
     */
    // Perform provisioning of spi geo
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_SPI_GEO_EXAMPLE_KEY_FILE);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Check the UFM status (expect: unprovisioned and not locked)
    status = read_from_mailbox(MB_PROVISION_STATUS);
    EXPECT_FALSE(MB_UFM_PROV_UFM_PROVISIONED_MASK & status);
    EXPECT_FALSE(MB_UFM_PROV_UFM_LOCKED_MASK & status);

    /*
     * Check the UFM status
     * Should be unprovisioned, but not locked
     */
    status = read_from_mailbox(MB_PROVISION_STATUS);
    EXPECT_FALSE(MB_UFM_PROV_UFM_PROVISIONED_MASK & status);
    EXPECT_FALSE(MB_UFM_PROV_UFM_LOCKED_MASK & status);

    // Check if SPI GEO has been provisioned
    // SPI GEO should be provisioned
    EXPECT_TRUE(is_ufm_spi_geo_provisioned());

    /*
     * Provision UFM PFR data
     */
    // Perform provisioning
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    // Check the UFM status (expect: provisioned but not locked)
    status = read_from_mailbox(MB_PROVISION_STATUS);
    EXPECT_TRUE(MB_UFM_PROV_UFM_PROVISIONED_MASK & status);
    EXPECT_FALSE(MB_UFM_PROV_UFM_LOCKED_MASK & status);

    /*
     * Check the UFM status
     * Should be provisioned, but not locked
     */
    status = read_from_mailbox(MB_PROVISION_STATUS);
    EXPECT_TRUE(MB_UFM_PROV_UFM_PROVISIONED_MASK & status);
    EXPECT_FALSE(MB_UFM_PROV_UFM_LOCKED_MASK & status);

    // Check if SPI GEO has been provisioned
    // SPI GEO should not be provisioned
    EXPECT_FALSE(is_ufm_spi_geo_provisioned());
}

/**
 * Test the UFM command: 00h: Erase current (not-locked) provisioning spi geo
 * CPLD should allow erase as no locked
 */
TEST_F(UFMProvisioningSpiGeoTest, test_erase_ufm_spi_geo)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);
    // Skip all T-1 operations
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_TMIN1_OPERATIONS);

    // Perform provisioning of spi geo
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_SPI_GEO_EXAMPLE_KEY_FILE);

    /*
     * Send erase provisioning request
     */
    ut_send_in_ufm_command(MB_UFM_PROV_ERASE);

    /*
     * Execute the Nios firmware flow
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Wait until CPLD finished with the request
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // Make sure there's no error before moving on
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    // Ensure that all UFM pfr data is erased
    alt_u32* ufm_pfr_data_ptr = (alt_u32*) get_ufm_pfr_data();
    for (int i = 0; i < UFM_PFR_DATA_SIZE / 4; i++)
    {
        EXPECT_EQ(ufm_pfr_data_ptr[i], alt_u32(0xffffffff));
    }
}

/**
 * Attempt to lock UFM after provisioning spi geo only. This request should be denied.
 */
TEST_F(UFMProvisioningSpiGeoTest, test_lock_ufm_after_provisioning_spi_geo)
{
    /*
     * Provision spi geo ONLY
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    // Perform provisioning of spi geo
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_SPI_GEO_EXAMPLE_KEY_FILE);

    /*
     * Attempt to lock UFM
     */
    ut_send_in_ufm_command(MB_UFM_PROV_END);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check Results
     */
    // Nios should have processed this ufm command
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // There should be an error for locking UFM prior to completion of provisioning
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));
}

/**
 * Attempt to provision spi geo after ufm is locked
 */
TEST_F(UFMProvisioningSpiGeoTest, test_prov_spi_geo_after_ufm_is_locked)
{
    /*
     * Flow preparation
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);
    // Skip all T-1 operations
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::SKIP_TMIN1_OPERATIONS);

    // Perform provisioning
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

    /*
     * Send end provisioning (UFM Lock)
     */
    ut_send_in_ufm_command(MB_UFM_PROV_END);

    /*
     * Execute the Nios firmware flow
     */
    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Wait until CPLD finished with the request
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // Make sure there's no error before moving on
    EXPECT_FALSE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    // Check the UFM status. It should be locked.
    EXPECT_TRUE(MB_UFM_PROV_UFM_LOCKED_MASK & read_from_mailbox(MB_PROVISION_STATUS));


    /*
     * Now, send in spi geo
     */
    ut_send_in_ufm_command(MB_UFM_PROV_PCH_OFFSETS);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Wait until CPLD finished with the request
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // Expect there to be an error
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));

    /*
     * Now, send in spi geo
     */
    ut_send_in_ufm_command(MB_UFM_PROV_BMC_OFFSETS);

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check results
     */
    // Wait until CPLD finished with the request
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_DONE_MASK));
    // Expect there to be an error
    EXPECT_TRUE(ut_check_ufm_prov_status(MB_UFM_PROV_CMD_ERROR_MASK));
}

/**
 * Provision spi geometries
 */
TEST_F(UFMProvisioningSpiGeoTest, test_provisioning_spi_geo_data)
{
    alt_u32* mb_ufm_prov_cmd_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_PROVISION_CMD;
    alt_u32* mb_ufm_cmd_trigger_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_UFM_CMD_TRIGGER;

    /************
     * PCH Offsets Provisioning Request
     ************/
    const alt_u8 w_pch_offsets[12] = {
            // start address of PCH SPI Active Region PFM
            0x00, 0x80, 0xFD, 0x03,
            // start address of PCH SPI Recovery Region
            0x00, 0x80, 0xFD, 0x02,
            // start address of PCH SPI Staging Region
            0x00, 0x80, 0xFD, 0x01,
    };
    for (int i = 0; i < 12; i++)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_WRITE_FIFO, w_pch_offsets[i]);
    }

    // In T0, mb_ufm_provisioning_handler is running in a while loop
    mb_ufm_provisioning_handler();

    // Execute the root key provisioning command
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_prov_cmd_addr, MB_UFM_PROV_PCH_OFFSETS, true);
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_cmd_trigger_addr, MB_UFM_CMD_EXECUTE_MASK, true);

    // In T0, mb_ufm_provisioning_handler is running in a while loop
    mb_ufm_provisioning_handler();

    // Wait until CPLD finished with the request
    ut_wait_for_ufm_prov_cmd_done();
    // Make sure there's no error before moving on
    EXPECT_EQ(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK, (alt_u32) 0);

    // In T0, mb_ufm_provisioning_handler is running in a while loop
    mb_ufm_provisioning_handler();

    /*
     * BMC Offsets Provisioning Request
     */
    const alt_u8 w_bmc_offsets[12] = {
            // start address of BMC SPI Active Region PFM
            0x00, 0x00, 0xbe, 0x10,
            // start address of BMC SPI Recovery Region
            0x00, 0x00, 0x40, 0x02,
            // start address of BMC SPI Staging Region
            0x00, 0x00, 0x00, 0x04,
    };
    for (int i = 0; i < 12; i++)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_WRITE_FIFO, w_bmc_offsets[i]);
    }

    // In T0, mb_ufm_provisioning_handler is running in a while loop
    mb_ufm_provisioning_handler();

    // Execute the root key provisioning command
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_prov_cmd_addr, MB_UFM_PROV_BMC_OFFSETS, true);
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_cmd_trigger_addr, MB_UFM_CMD_EXECUTE_MASK, true);

    // In T0, mb_ufm_provisioning_handler is running in a while loop
    mb_ufm_provisioning_handler();

    // Wait until CPLD finished with the request
    ut_wait_for_ufm_prov_cmd_done();
    // Make sure there's no error before moving on
    EXPECT_EQ(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK, (alt_u32) 0);

    // In T0, mb_ufm_provisioning_handler is running in a while loop
    mb_ufm_provisioning_handler();

    /************
     * Result checking
     ************/

    // Check that UFM is currently not provisioned and not locked
    EXPECT_FALSE(is_ufm_provisioned());
    EXPECT_FALSE(is_ufm_locked());
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_PROVISIONED_MASK);
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_LOCKED_MASK);

    // Check if SPI GEO has been provisioned
    // SPI GEO should be provisioned
    EXPECT_TRUE(is_ufm_spi_geo_provisioned());

    // In T0, mb_ufm_provisioning_handler is running in a while loop
    mb_ufm_provisioning_handler();

    // Check key hash in the UFM vs expected hash values
    UFM_PFR_DATA* system_ufm_data = (UFM_PFR_DATA*) SYSTEM_MOCK::get()->get_ufm_data_ptr();

    // Check PCH offsets
    alt_u8* offset = (alt_u8*) &system_ufm_data->pch_active_pfm;
    alt_u8* expected_offset = (alt_u8*) w_pch_offsets;
    for (int i = 0; i < 4; i++)
    {
        EXPECT_EQ(offset[i], *expected_offset);
        expected_offset++;
    }

    offset = (alt_u8*) &system_ufm_data->pch_recovery_region;
    for (int i = 0; i < 4; i++)
    {
        EXPECT_EQ(offset[i], *expected_offset);
        expected_offset++;
    }

    offset = (alt_u8*) &system_ufm_data->pch_staging_region;
    for (int i = 0; i < 4; i++)
    {
        EXPECT_EQ(offset[i], *expected_offset);
        expected_offset++;
    }

    // Check BMC offsets
    offset = (alt_u8*) &system_ufm_data->bmc_active_pfm;
    expected_offset = (alt_u8*) w_bmc_offsets;
    for (int i = 0; i < 4; i++)
    {
        EXPECT_EQ(offset[i], *expected_offset);
        expected_offset++;
    }

    offset = (alt_u8*) &system_ufm_data->bmc_recovery_region;
    for (int i = 0; i < 4; i++)
    {
        EXPECT_EQ(offset[i], *expected_offset);
        expected_offset++;
    }

    offset = (alt_u8*) &system_ufm_data->bmc_staging_region;
    for (int i = 0; i < 4; i++)
    {
        EXPECT_EQ(offset[i], *expected_offset);
        expected_offset++;
    }

    /************
     * Finally, test erase command and verify that UFM is no longer provisioned
     ************/
    // Execute the erase provisioning command
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_prov_cmd_addr, MB_UFM_PROV_ERASE, true);
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_cmd_trigger_addr, MB_UFM_CMD_EXECUTE_MASK, true);

    mb_ufm_provisioning_handler();

    // Wait until CPLD finished with the request
    ut_wait_for_ufm_prov_cmd_done();
    // Make sure there's no error before moving on
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK);

    // Check that UFM is currently not provisioned and not locked
    EXPECT_FALSE(is_ufm_provisioned());
    EXPECT_FALSE(is_ufm_locked());
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_PROVISIONED_MASK);
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_LOCKED_MASK);

    // Check if SPI GEO has been provisioned
    // SPI GEO should not be provisioned
    EXPECT_FALSE(is_ufm_spi_geo_provisioned());
}

/**
 * @brief Quickly scan through various UFM commands and check for CPLD handling routine
 * For example, provision root key hash after spi geo has been provisioned
 */
TEST_F(UFMProvisioningSpiGeoTest, test_various_ufm_cmds_after_spi_geo)
{
    /*
     * Lock ufm without provisioning data first. This should be rejected
     */
    IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_CMD, MB_UFM_PROV_END);
    IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_CMD_TRIGGER, MB_UFM_CMD_EXECUTE_MASK);
    mb_ufm_provisioning_handler();
    EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK);

    // Check if SPI GEO has been provisioned
    // SPI GEO should not be provisioned
    EXPECT_FALSE(is_ufm_spi_geo_provisioned());

    EXPECT_FALSE(is_ufm_provisioned());
    EXPECT_FALSE(is_ufm_locked());
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_PROVISIONED_MASK);
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_LOCKED_MASK);

    /*
     * Test various commands after the UFM is provisioned
     * Unmapped value (e.g. 0xFF) is not tested here. In provisioned state,
     * Nios currently ignores them and doesn't issue error for them.
     */
    alt_u32 num_test_cmds_1 = 11;
    alt_u32 ufm_cmd_list_1[11] = {
            MB_UFM_PROV_ERASE,
            MB_UFM_PROV_PIT_ID,
            MB_UFM_PROV_PCH_OFFSETS,
            MB_UFM_PROV_BMC_OFFSETS,
            MB_UFM_PROV_END,
            MB_UFM_PROV_RD_ROOT_KEY,
            MB_UFM_PROV_RD_PCH_OFFSETS,
            MB_UFM_PROV_RD_BMC_OFFSETS,
            MB_UFM_PROV_RECONFIG_CPLD,
            MB_UFM_PROV_ENABLE_PIT_L1,
            MB_UFM_PROV_ENABLE_PIT_L2,
    };

    // Check whether the command would be rejected (1 means rejected by Nios code)
    //   Provisioning of PCH offsets/BMC offsets is rejected because they have been provisioned
    //   UFM cannot be locked because UFM is not provisioned
    //   Enable PIT L1 is rejected because PIT ID has not been provisioned
    //   Enable PIT L2 is rejected because PIT L1 has not been enabled
    alt_u32 result_list_1[11] = {0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1};

    for (alt_u32 cmd_i = 0; cmd_i < num_test_cmds_1; cmd_i++)
    {
        // Re-provision data
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_SPI_GEO_EXAMPLE_KEY_FILE);

        // Send in the UFM command
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_CMD, ufm_cmd_list_1[cmd_i]);
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_CMD_TRIGGER, MB_UFM_CMD_EXECUTE_MASK);

        // Run Nios code
        mb_ufm_provisioning_handler();

        // Check UFM status
        EXPECT_EQ(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK,
                (result_list_1[cmd_i] << 2));
        EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_BUSY_MASK);
        EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_DONE_MASK);
    }

    // Re-provision data
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_SPI_GEO_EXAMPLE_KEY_FILE);

    // Check if SPI GEO has been provisioned
    // SPI GEO should be provisioned
    EXPECT_TRUE(is_ufm_spi_geo_provisioned());

    /************
     * Root Key Hash Provisioning Request
     ************/
    const alt_u8 w_test_key_hash[48] = {
            0xdc, 0xa0, 0xb4, 0xed, 0x14, 0x12, 0xea, 0xe6, 0xf8, 0x5d, 0x02, 0xae,
            0x6e, 0xf3, 0xd3, 0x50, 0xb3, 0xd0, 0xe5, 0xba, 0x6f, 0x20, 0x80, 0x08,
            0x5c, 0xf4, 0xb7, 0xb1, 0xdc, 0xd5, 0xba, 0x17, 0xde, 0xad, 0xbe, 0xef,
            0xdc, 0xa0, 0xb4, 0xed, 0x14, 0x12, 0xea, 0xe6, 0xf8, 0x5d, 0x02, 0xae
    };
    for (int i = 0; i < 48; i++) { IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_WRITE_FIFO, w_test_key_hash[i]); }

    alt_u32* mb_ufm_prov_cmd_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_PROVISION_CMD;
    alt_u32* mb_ufm_cmd_trigger_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_UFM_CMD_TRIGGER;

    // Execute the root key provisioning command
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_prov_cmd_addr, MB_UFM_PROV_ROOT_KEY, true);
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_cmd_trigger_addr, MB_UFM_CMD_EXECUTE_MASK, true);

    mb_ufm_provisioning_handler();

    // Wait until CPLD finished with the request
    ut_wait_for_ufm_prov_cmd_done();
    // Make sure there's no error before moving on
    EXPECT_EQ(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK, (alt_u32) 0);

    mb_ufm_provisioning_handler();

    // Check if SPI GEO has been provisioned
    // SPI GEO should not be provisioned
    EXPECT_FALSE(is_ufm_spi_geo_provisioned());

    EXPECT_TRUE(is_ufm_provisioned());
    EXPECT_FALSE(is_ufm_locked());
    EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_PROVISIONED_MASK);
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_LOCKED_MASK);

    /*
     * Lock UFM
     */
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);
    IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_CMD, MB_UFM_PROV_END);
    IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_CMD_TRIGGER, MB_UFM_CMD_EXECUTE_MASK);
    mb_ufm_provisioning_handler();
    EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK);

    EXPECT_TRUE(is_ufm_provisioned());
    EXPECT_TRUE(is_ufm_locked());
    EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_PROVISIONED_MASK);
    EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_LOCKED_MASK);

    // Check if SPI GEO has been provisioned
    // SPI GEO should not be provisioned
    EXPECT_FALSE(is_ufm_spi_geo_provisioned());

    /*
     * Test various commands after the UFM is locked
     */
    alt_u32 num_test_cmds_2 = 15;
    alt_u32 ufm_cmd_list_2[15] = {
            MB_UFM_PROV_ERASE,
            MB_UFM_PROV_ROOT_KEY,
            MB_UFM_PROV_PIT_ID,
            MB_UFM_PROV_PCH_OFFSETS,
            MB_UFM_PROV_BMC_OFFSETS,
            MB_UFM_PROV_END,
            MB_UFM_PROV_RD_ROOT_KEY,
            MB_UFM_PROV_RD_PCH_OFFSETS,
            MB_UFM_PROV_RD_BMC_OFFSETS,
            MB_UFM_PROV_RECONFIG_CPLD,
            MB_UFM_PROV_ENABLE_PIT_L1,
            MB_UFM_PROV_ENABLE_PIT_L2,
            // Unmapped value
            0x9,
            0xF,
            0xFF,
    };
    // Check whether the command would be rejected (1 means rejected by Nios code)
    alt_u32 result_list_2[15] = {1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1};

    for (alt_u32 cmd_i = 0; cmd_i < num_test_cmds_2; cmd_i++)
    {
        // Send in the UFM command
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_CMD, ufm_cmd_list_2[cmd_i]);
        IOWR(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_UFM_CMD_TRIGGER, MB_UFM_CMD_EXECUTE_MASK);

        // Run Nios code
        mb_ufm_provisioning_handler();

        // Check UFM status
        EXPECT_EQ(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_ERROR_MASK,
                (result_list_2[cmd_i] << 2));
        EXPECT_FALSE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_BUSY_MASK);
        EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_DONE_MASK);

        // Check provisioned/locked status
        EXPECT_TRUE(is_ufm_provisioned());
        EXPECT_TRUE(is_ufm_locked());
        EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_PROVISIONED_MASK);
        EXPECT_TRUE(IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_UFM_LOCKED_MASK);

        // Check if SPI GEO has been provisioned
        // SPI GEO should not be provisioned
        EXPECT_FALSE(is_ufm_spi_geo_provisioned());
    }
}
