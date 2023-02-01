#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class UtilsTest : public testing::Test
{
public:
    alt_u32* m_memory = nullptr;
    alt_u32* m_mailbox_mem = nullptr;

    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
        m_memory = NIOS_SCRATCHPAD_ADDR;
        m_mailbox_mem = U_MAILBOX_AVMM_BRIDGE_ADDR;
    }

    virtual void TearDown() {}
};

TEST_F(UtilsTest, test_basic)
{
    IOWR(m_memory, 0, 1);
    EXPECT_EQ(IORD(m_memory, 0), (alt_u32) 1);
    EXPECT_EQ(IORD(m_memory, 1), (alt_u32) 0);
}

TEST_F(UtilsTest, test_load_store)
{
    alt_u8 src_mem[4] = {0x01, 0x23, 0x45, 0x67};
    EXPECT_EQ(*((alt_u32*) src_mem), (alt_u32) 0x67452301);

    alt_u32 data = *((alt_u32*) src_mem);
    EXPECT_EQ(data, (alt_u32) 0x67452301);

    IOWR(m_memory, 0, data);
    EXPECT_EQ(IORD(m_memory, 0), (alt_u32) 0x67452301);
}

TEST_F(UtilsTest, test_set_bit)
{
    EXPECT_EQ(IORD(m_memory, 0), (alt_u32) 0);
    // Set the second bit
    set_bit(&m_memory[0], 2);
    EXPECT_EQ(IORD(m_memory, 0), (alt_u32) 4);
}

TEST_F(UtilsTest, test_clear_bit)
{
    IOWR(m_memory, 3, 7);
    EXPECT_EQ(IORD(m_memory, 3), (alt_u32) 7);

    // Clear the third bit
    clear_bit(&m_memory[3], 2);
    EXPECT_EQ(IORD(m_memory, 3), (alt_u32) 3);
}

TEST_F(UtilsTest, test_check_bit)
{
    IOWR(m_memory, 1, 7);
    EXPECT_EQ(IORD(m_memory, 1), (alt_u32) 7);

    // Verify the first 4 bits
    EXPECT_EQ(check_bit(&m_memory[1], 0), (alt_u32) 1);
    EXPECT_EQ(check_bit(&m_memory[1], 1), (alt_u32) 1);
    EXPECT_EQ(check_bit(&m_memory[1], 2), (alt_u32) 1);
    EXPECT_EQ(check_bit(&m_memory[1], 3), (alt_u32) 0);
}

TEST_F(UtilsTest, test_alt_u32_memcpy_non_incr_1word)
{
    // Set up source memory blocks
    alt_u8 src_mem[4] = {0x01, 0x23, 0x45, 0x67};
    EXPECT_EQ(*((alt_u32*) src_mem), (alt_u32) 0x67452301);

    // Perform memcpy
    alt_u32_memcpy_non_incr(m_memory, (alt_u32*) src_mem, 4);

    // Verify memcpy results
    EXPECT_EQ(IORD(m_memory, 0), (alt_u32) 0x67452301);
}

TEST_F(UtilsTest, test_alt_u32_memcpy_non_incr)
{
    // Set up source m_memory blocks
    alt_u8 src_mem[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    // Perform memcpy
    alt_u32* dest_ptr = &m_memory[10];
    alt_u32_memcpy_non_incr(dest_ptr, (alt_u32*) src_mem, 8);

    // Verify memcpy results
    EXPECT_EQ(IORD(m_memory, 9), (alt_u32) 0);
    EXPECT_EQ(IORD(m_memory, 10), (alt_u32) 0xEFCDAB89);
    EXPECT_EQ(IORD(m_memory, 11), (alt_u32) 0);
}

TEST_F(UtilsTest, test_alt_u32_memcpy)
{
    // Set up source m_memory blocks
    alt_u8 src_mem[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    // Perform memcpy
    alt_u32 dest_ptr[10] = {0};
    alt_u32_memcpy(dest_ptr, (alt_u32*) src_mem, 8);

    // Verify memcpy results
    EXPECT_EQ(dest_ptr[0], (alt_u32) 0x67452301);
    EXPECT_EQ(dest_ptr[1], (alt_u32) 0xEFCDAB89);
    EXPECT_EQ(dest_ptr[2], (alt_u32) 0);
}

TEST(SanityTest, test_alt_u32_memcpy_non_incr_bad_num_words)
{
    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    EXPECT_ANY_THROW({
        alt_u32_memcpy_non_incr(NIOS_SCRATCHPAD_ADDR, NIOS_SCRATCHPAD_ADDR, 3);
    });
}

TEST_F(UtilsTest, testalt_u32_memcpy_bad_num_words)
{
    // Set asserts to throw as opposed to abort
    SYSTEM_MOCK::get()->set_assert_to_throw();
    EXPECT_ANY_THROW(
        { alt_u32_memcpy(NIOS_SCRATCHPAD_ADDR, NIOS_SCRATCHPAD_ADDR, 3); });
}

TEST_F(UtilsTest, test_alt_u32_memcpy_s_oversized_data)
{
    // Set up source m_memory blocks
    alt_u8 src_mem[12] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67};

    // Initialise the array ot all zeros
    alt_u32 dest_ptr[2] = {0};
    // As the array is 32 bits, the number of bytes is 4
    // The destination size is smaller than the data size
    // Perform memcpy_s where the data size is bigger than the destination size
    alt_u32_memcpy_s(dest_ptr, 8, (alt_u32*) src_mem, 12);

    // Verify memcpy results
    // Expect the content to not be overwritten because of illegal data size
    EXPECT_EQ(dest_ptr[0], (alt_u32) 0);
    EXPECT_EQ(dest_ptr[1], (alt_u32) 0);
}

TEST_F(UtilsTest, test_alt_u32_memcpy_s_bad_src_ptr_logging)
{
    // Invalid source pointer
    alt_u32* src_mem = nullptr;

    // Perform memcpy
    alt_u32 dest_ptr[3] = {0};
    alt_u32_memcpy_s(dest_ptr, 12, src_mem, 8);

    // Error/unsafe ptr log should be reported
    EXPECT_EQ(IORD(m_mailbox_mem, MB_MAJOR_ERROR_CODE), MAJOR_ERROR_MEMCPY_S_FAILED);
    EXPECT_EQ(IORD(m_mailbox_mem, MB_MINOR_ERROR_CODE), MINOR_ERROR_INVALID_POINTER);
}
TEST_F(UtilsTest, test_alt_u32_memcpy_s_bad_dest_ptr_logging)
{
    // Set up source m_memory blocks
    alt_u8 src_mem[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    // Invalid dest pointer
    // Perform memcpy
    alt_u32* dest_ptr = nullptr;
    alt_u32_memcpy_s(dest_ptr, 32, (alt_u32*) src_mem, 8);

    // Error/unsafe ptr log should be reported
    EXPECT_EQ(IORD(m_mailbox_mem, MB_MAJOR_ERROR_CODE), MAJOR_ERROR_MEMCPY_S_FAILED);
    EXPECT_EQ(IORD(m_mailbox_mem, MB_MINOR_ERROR_CODE), MINOR_ERROR_INVALID_POINTER);
}

TEST_F(UtilsTest, test_alt_u32_memcpy_s_oversized_data_logging)
{
    // Set up source m_memory blocks
    alt_u8 src_mem[12] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67};

    // Perform memcpy
    alt_u8 dest_ptr[4] = {0};
    dest_ptr[0] = 0xde;
    dest_ptr[1] = 0xde;
    // As the array is 32 bits, the number of bytes is 4
    // The destination size is smaller than the data size
    alt_u32_memcpy_s((alt_u32*) dest_ptr, 4, (alt_u32*) src_mem, 12);

    // Error/unsafe ptr log should be reported
    EXPECT_EQ(IORD(m_mailbox_mem, MB_MAJOR_ERROR_CODE), MAJOR_ERROR_MEMCPY_S_FAILED);
    EXPECT_EQ(IORD(m_mailbox_mem, MB_MINOR_ERROR_CODE), MINOR_ERROR_EXCESSIVE_DATA_SIZE);

    // Verify memcpy results
    // Expect data to not be overwritten
    EXPECT_EQ(dest_ptr[0], 0xde);
    EXPECT_EQ(dest_ptr[1], 0xde);
}

TEST_F(UtilsTest, test_alt_u32_memcpy_s_big_destination_size)
{
    // Set up source m_memory blocks
    alt_u8 src_mem[12] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45, 0x67};

    // Perform memcpy
    alt_u8 dest_ptr[4] = {0};
    dest_ptr[0] = 0xde;
    dest_ptr[1] = 0xde;
    // As the array is 32 bits, the number of bytes is 4
    // The destination size is smaller than the data size
    alt_u32_memcpy_s((alt_u32*) dest_ptr, (PCH_SPI_FLASH_SIZE + PCH_SPI_FLASH_SIZE), (alt_u32*) src_mem, 12);

    // Error/unsafe ptr log should be reported
    EXPECT_EQ(IORD(m_mailbox_mem, MB_MAJOR_ERROR_CODE), MAJOR_ERROR_MEMCPY_S_FAILED);
    EXPECT_EQ(IORD(m_mailbox_mem, MB_MINOR_ERROR_CODE), MINOR_ERROR_EXCESSIVE_DEST_SIZE);

    // Verify memcpy results
    // Expect data to not be overwritten
    EXPECT_EQ(dest_ptr[0], 0xde);
    EXPECT_EQ(dest_ptr[1], 0xde);
}

#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)

TEST_F(UtilsTest, test_utility_compute_power_1)
{
    // Compute 2 to power of 15
    // SPDM protocol uses base 2 and is in us
    EXPECT_EQ(compute_pow(2,15), (alt_u32)32768);
}

TEST_F(UtilsTest, test_utility_compute_power_2)
{
    // Basic test with different base.
    // Computing 4 to power of 7
    EXPECT_EQ(compute_pow(4,7), (alt_u32)16384);
}

TEST_F(UtilsTest, test_utility_round_up_to_multiple_of_4_1)
{
    // Round up 97 to nearest multiple of 4
    EXPECT_EQ(round_up_to_multiple_of_4(97), (alt_u32)100);
}

TEST_F(UtilsTest, test_utility_round_up_to_multiple_of_4_2)
{
	// Round up 31 to nearest multiple of 4
    EXPECT_EQ(round_up_to_multiple_of_4(31), (alt_u32)32);
}

#endif
