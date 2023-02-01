#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class KeychainUtilsTest : public testing::Test
{
public:
    alt_u8 m_raw_data_x86[SIGNED_BINARY_BLOCKSIGN_FILE_SIZE] = {};

    virtual void SetUp()
    {
        SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
        sys->reset();

        // Load Blocksign tool signed payload to memory
        sys->init_x86_mem_from_file(SIGNED_BINARY_BLOCKSIGN_FILE, (alt_u32*) m_raw_data_x86);
    }

    virtual void TearDown() {}
};

TEST_F(KeychainUtilsTest, test_get_required_perm)
{
    EXPECT_EQ(get_required_perm(KCH_PC_PFR_CPLD_UPDATE_CAPSULE), (alt_u32) SIGN_CPLD_UPDATE_CAPSULE_MASK);
    EXPECT_EQ(get_required_perm(KCH_PC_PFR_PCH_PFM), (alt_u32) SIGN_PCH_PFM_MASK);
    EXPECT_EQ(get_required_perm(KCH_PC_PFR_PCH_UPDATE_CAPSULE), (alt_u32) SIGN_PCH_UPDATE_CAPSULE_MASK);
    EXPECT_EQ(get_required_perm(KCH_PC_PFR_BMC_PFM), (alt_u32) SIGN_BMC_PFM_MASK);
    EXPECT_EQ(get_required_perm(KCH_PC_PFR_BMC_UPDATE_CAPSULE), (alt_u32) SIGN_BMC_UPDATE_CAPSULE_MASK);
}

TEST_F(KeychainUtilsTest, test_get_signed_payload_size)
{
    EXPECT_EQ(get_signed_payload_size((alt_u32*) m_raw_data_x86), alt_u32(SIGNED_BINARY_BLOCKSIGN_FILE_SIZE));
}

TEST_F(KeychainUtilsTest, test_memcpy_payload)
{
	// Write to the beginning of PCH flash
	switch_spi_flash(SPI_FLASH_PCH);
	alt_u32* dest_ptr = get_spi_flash_ptr();

	alt_u32* signed_payload = (alt_u32*) m_raw_data_x86;
    memcpy_signed_payload(0, signed_payload);

    // Check result
    for (alt_u32 i = 0; i < (SIGNED_BINARY_BLOCKSIGN_FILE_SIZE / 4); i++)
    {
    	EXPECT_EQ(dest_ptr[i], signed_payload[i]);
    }
}

