// Include the GTest headers
#include "gtest_headers.h"

// Include the PFR headers
// Always include the BSP mock then pfr_sys.h first
#include "bsp_mock.h"
#include "pfr_sys.h"

TEST(SystemMockTest, get_returns_inst)
{
    EXPECT_NE(nullptr, SYSTEM_MOCK::get());
}

TEST(SystemMockTest, reset_cleans_bmc_mem)
{
    SYSTEM_MOCK::get()->reset();

    EXPECT_EQ(alt_u32(0), IORD(NIOS_SCRATCHPAD_ADDR, 0));
    IOWR(NIOS_SCRATCHPAD_ADDR, 0, 0xDEADBEEF);
    EXPECT_EQ(alt_u32(0xDEADBEEF), IORD(NIOS_SCRATCHPAD_ADDR, 0));

    SYSTEM_MOCK::get()->reset();

    EXPECT_EQ(alt_u32(0), IORD(NIOS_SCRATCHPAD_ADDR, 0));
}

TEST(SystemMockTest, test_init_from_dat)
{
    SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
    sys->reset();

    sys->init_nios_mem_from_file(SIGNED_BINARY_BLOCKSIGN_FILE, NIOS_SCRATCHPAD_ADDR);
    alt_u32 read_word = IORD(NIOS_SCRATCHPAD_ADDR, 0);
    EXPECT_EQ(alt_u32(0x19fdeab6), read_word);

    alt_u8 memory[1152] = {0};
    sys->init_x86_mem_from_file(SIGNED_BINARY_BLOCKSIGN_FILE, (alt_u32*) memory);

    EXPECT_EQ(memory[0], alt_u8(0x19));
    EXPECT_EQ(memory[1], alt_u8(0xfd));
    EXPECT_EQ(memory[2], alt_u8(0xea));
    EXPECT_EQ(memory[3], alt_u8(0xb6));
#ifdef GTEST_USE_CRYPTO_384
    EXPECT_EQ(memory[1148], alt_u8(0));
    EXPECT_EQ(memory[1149], alt_u8(0));
    EXPECT_EQ(memory[1150], alt_u8(0));
    EXPECT_EQ(memory[1151], alt_u8(0));
#else
    EXPECT_EQ(memory[1148], alt_u8(0x54));
    EXPECT_EQ(memory[1149], alt_u8(0x0e));
    EXPECT_EQ(memory[1150], alt_u8(0x20));
    EXPECT_EQ(memory[1151], alt_u8(0x3b));
#endif
}

TEST(SystemMockTest, test_load_to_spi_flash)
{
    SYSTEM_MOCK* sys = SYSTEM_MOCK::get();
    sys->reset();
    sys->reset_spi_flash(SPI_FLASH_BMC);

    SYSTEM_MOCK::get()->load_to_flash(SPI_FLASH_BMC, SIGNED_PFM_BMC_FILE, SIGNED_PFM_BMC_FILE_SIZE, 4);
    alt_u8* spi_flash_x86_ptr = (alt_u8*) sys->get_x86_ptr_to_spi_flash(SPI_FLASH_BMC);

    EXPECT_EQ(spi_flash_x86_ptr[0], alt_u8(0xff));
    EXPECT_EQ(spi_flash_x86_ptr[1], alt_u8(0xff));
    EXPECT_EQ(spi_flash_x86_ptr[2], alt_u8(0xff));
    EXPECT_EQ(spi_flash_x86_ptr[3], alt_u8(0xff));

    EXPECT_EQ(spi_flash_x86_ptr[4], alt_u8(0x19));
    EXPECT_EQ(spi_flash_x86_ptr[5], alt_u8(0xfd));
    EXPECT_EQ(spi_flash_x86_ptr[6], alt_u8(0xea));
    EXPECT_EQ(spi_flash_x86_ptr[7], alt_u8(0xb6));
}
