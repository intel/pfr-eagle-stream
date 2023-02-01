#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"


class SPIFilterTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();

        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);
    }

    virtual void TearDown() {}
};

/**
 * @brief Test PCH SPI filtering with some custom SPI region definitions.
 *
 * SPI Region 1: 0x0000 - 0x17000 is RO
 * SPI Region 2: 0x17000 - 0x46000 is RW
 * SPI Region 3: 0x46000 - 0xC7000 is RO
 * SPI Region 4: 0xC7000 - 0x4000000 is RW
 */
TEST_F(SPIFilterTest, test_spi_filtering_case1)
{
    // Assuming these rules are from PCH flash
    switch_spi_flash(SPI_FLASH_PCH);

    // Apply rules in SPI Region definitions
    apply_spi_write_protection(0x0, 0x17000, 0);
    apply_spi_write_protection(0x17000, 0x46000, 1);
    apply_spi_write_protection(0x46000, 0xC7000, 0);
    apply_spi_write_protection(0xC7000, 0x4000000, 1);

    // SPI regions 1 & 2
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 0), alt_u32(0xFF800000));
    // SPI region 2
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 1), alt_u32(0xFFFFFFFF));
    // SPI region 2 & 3
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 2), alt_u32(0x0000003F));
    // SPI region 3
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 3), alt_u32(0x00000000));
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 4), alt_u32(0x00000000));
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 5), alt_u32(0x00000000));
    // SPI region 3 & 4
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 6), alt_u32(0xFFFFFF80));

    // SPI region 4
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 7), alt_u32(0xFFFFFFFF));
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 8), alt_u32(0xFFFFFFFF));
    // Expected maximum size of PCH flash is PCH_SPI_FLASH_SIZE
    alt_u32 last_word_pos_for_pch_flash_in_we_mem = PCH_SPI_FLASH_SIZE >> (14 + 5);
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, last_word_pos_for_pch_flash_in_we_mem - 2), alt_u32(0xFFFFFFFF));
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, last_word_pos_for_pch_flash_in_we_mem - 1), alt_u32(0xFFFFFFFF));

    // Ensure that BMC rules are untouched
    for (alt_u32 word_i = 0; word_i < U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_SPAN / 4; word_i++)
    {
        EXPECT_EQ(IORD(U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_BASE, word_i), alt_u32(0));
    }
}

/**
 * @brief Test PCH SPI filtering with some custom SPI region definitions, which are not in ascending order.
 *
 * SPI Region 1: 0xC7000 - 0x4000000 is RW
 * SPI Region 2: 0x0000 - 0x15000 is RO
 * SPI Region 3: 0x46000 - 0xC7000 is RO
 * SPI Region 4: 0x15000 - 0x46000 is RW
 */
TEST_F(SPIFilterTest, test_spi_filtering_case1_scrumbled_order)
{
    // Assuming these rules are from PCH flash
    switch_spi_flash(SPI_FLASH_PCH);

    // Apply rules in SPI Region definitions
    apply_spi_write_protection(0xC7000, 0x4000000, 1);
    apply_spi_write_protection(0x0, 0x15000, 0);
    apply_spi_write_protection(0x46000, 0xC7000, 0);
    apply_spi_write_protection(0x15000, 0x46000, 1);

    // SPI regions 1 & 2
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 0), alt_u32(0xFFE00000));
    // SPI region 2
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 1), alt_u32(0xFFFFFFFF));
    // SPI region 2 & 3
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 2), alt_u32(0x0000003F));
    // SPI region 3
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 3), alt_u32(0x00000000));
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 4), alt_u32(0x00000000));
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 5), alt_u32(0x00000000));
    // SPI region 3 & 4
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 6), alt_u32(0xFFFFFF80));

    // SPI region 4
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 7), alt_u32(0xFFFFFFFF));
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 8), alt_u32(0xFFFFFFFF));
    // Expected maximum size of PCH flash is PCH_SPI_FLASH_SIZE
    alt_u32 last_word_pos_for_pch_flash_in_we_mem = PCH_SPI_FLASH_SIZE >> (14 + 5);
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, last_word_pos_for_pch_flash_in_we_mem - 2), alt_u32(0xFFFFFFFF));
    EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, last_word_pos_for_pch_flash_in_we_mem - 1), alt_u32(0xFFFFFFFF));

    // Ensure that BMC rules are untouched
    for (alt_u32 word_i = 0; word_i < U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_SPAN / 4; word_i++)
    {
        EXPECT_EQ(IORD(U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_BASE, word_i), alt_u32(0));
    }
}

/**
 * @brief Test BMC SPI filtering with some custom SPI region definitions.
 *
 * SPI Region 1: 0x0000 - 0x4000 is RW
 * SPI Region 2: 0x4000 - 0x1E000 is RO
 * SPI Region 3: 0x1E000 - 0x8000000 is RW
 */
TEST_F(SPIFilterTest, test_spi_filtering_with_custom_pfm_case2)
{
    // Assuming these rules are from BMC flash
    switch_spi_flash(SPI_FLASH_BMC);

    // Define some custom SPI region definitions
    //   SPI Region 1: 0x0000 - 0x4000 is RW
    //   SPI Region 2: 0x4000 - 0x1E000 is RO
    //   SPI Region 3: 0x1E000 - 0x8000000 is RW

    // Apply rules in SPI Region definitions
    apply_spi_write_protection(0x0, 0x4000, 1);
    apply_spi_write_protection(0x4000, 0x1E000, 0);
    apply_spi_write_protection(0x1E000, 0x8000000, 1);

    // SPI regions 1/2/3
    EXPECT_EQ(IORD(U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_BASE, 0), alt_u32(0xC000000F));
    // SPI region 3
    EXPECT_EQ(IORD(U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_BASE, 1), alt_u32(0xFFFFFFFF));
    EXPECT_EQ(IORD(U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_BASE, 2), alt_u32(0xFFFFFFFF));
    // Expected maximum size of BMC flash is BMC_SPI_FLASH_SIZE
    alt_u32 last_word_pos_for_bmc_flash_in_we_mem = BMC_SPI_FLASH_SIZE >> (14 + 5);
    EXPECT_EQ(IORD(U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_BASE, last_word_pos_for_bmc_flash_in_we_mem - 2), alt_u32(0xFFFFFFFF));
    EXPECT_EQ(IORD(U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_BASE, last_word_pos_for_bmc_flash_in_we_mem - 1), alt_u32(0xFFFFFFFF));

    // Ensure that PCH rules are untouched
    for (alt_u32 word_i = 0; word_i < U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_SPAN / 4; word_i++)
    {
        EXPECT_EQ(IORD(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, word_i), alt_u32(0));
    }
}
