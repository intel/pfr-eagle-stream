// Unit test for the PFR system flows

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "testdata_info.h"

class PFRTMin1RoutinesTest : public testing::Test
{
public:

    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
        // Provision UFM data
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);
    }

    virtual void TearDown() {}
};

TEST_F(PFRTMin1RoutinesTest, test_mb_write_pch_and_bmc_pfm_info)
{
    // Prepare the flashes
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, FULL_PFR_IMAGE_BMC_FILE, FULL_PFR_IMAGE_BMC_FILE_SIZE);
    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_PCH, FULL_PFR_IMAGE_PCH_FILE, FULL_PFR_IMAGE_PCH_FILE_SIZE);

    switch_spi_flash(SPI_FLASH_BMC);
    mb_write_bmc_pfm_info();

    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_SVN), alt_u8(PCH_ACTIVE_PFM_SVN));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MAJOR_VER), alt_u8(PCH_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_ACTIVE_MINOR_VER), alt_u8(PCH_ACTIVE_PFM_MINOR_VER));

    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_SVN), alt_u8(BMC_ACTIVE_PFM_SVN));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MAJOR_VER), alt_u8(BMC_ACTIVE_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_ACTIVE_MINOR_VER), alt_u8(BMC_ACTIVE_PFM_MINOR_VER));

    switch_spi_flash(SPI_FLASH_PCH);
    mb_write_pch_pfm_info();

    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_SVN), alt_u8(PCH_RECOVERY_PFM_SVN));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MAJOR_VER), alt_u8(PCH_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_PCH_PFM_RECOVERY_MINOR_VER), alt_u8(PCH_RECOVERY_PFM_MINOR_VER));

    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_SVN), alt_u8(BMC_RECOVERY_PFM_SVN));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MAJOR_VER), alt_u8(BMC_RECOVERY_PFM_MAJOR_VER));
    EXPECT_EQ(read_from_mailbox(MB_BMC_PFM_RECOVERY_MINOR_VER), alt_u8(BMC_RECOVERY_PFM_MINOR_VER));
}
