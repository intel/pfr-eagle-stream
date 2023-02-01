// Include standard headers
#include <iostream>
using namespace std;

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"


// Basic sanity tests

// Check to make sure the test definitions of the standard types is as expected
TEST(SanityTest, test_type_size)
{
    EXPECT_EQ(sizeof(alt_u8), static_cast<unsigned int>(1));
    EXPECT_EQ(sizeof(alt_u16), static_cast<unsigned int>(2));
    EXPECT_EQ(sizeof(alt_u32), static_cast<unsigned int>(4));
    EXPECT_EQ(sizeof(alt_u64), static_cast<unsigned int>(8));
}

TEST(SanityTest, test_io_rw)
{
    SYSTEM_MOCK::get()->reset();
    alt_u32* memory = NIOS_SCRATCHPAD_ADDR;

    alt_u8 src_mem[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    alt_u32* src_mem_u32 = (alt_u32*) src_mem;

    // Write with both IOWR and IOWR_32DIRECT
    // Note that the offset input to IO*_32DIRECT is different to IO*.
    IOWR(memory, 0, *src_mem_u32);
    EXPECT_EQ(IORD(memory, 0), (alt_u32) 0x67452301);
    EXPECT_EQ(IORD_32DIRECT(memory, 0), (alt_u32) 0x67452301);
    IOWR(memory, 0, alt_u32(0));

    IOWR_32DIRECT(memory, 0, *(src_mem_u32 + 1));
    EXPECT_EQ(IORD_32DIRECT(memory, 0), (alt_u32) 0xEFCDAB89);
    EXPECT_EQ(IORD(memory, 0), (alt_u32) 0xEFCDAB89);
    IOWR_32DIRECT(memory, 0, alt_u32(0));

    IOWR(memory, 1, *src_mem_u32);
    EXPECT_EQ(IORD(memory, 1), (alt_u32) 0x67452301);
    EXPECT_EQ(IORD_32DIRECT(memory, 4), (alt_u32) 0x67452301);
    EXPECT_EQ(IORD(memory, 0), alt_u32(0));
    IOWR(memory, 1, alt_u32(0));

    IOWR_32DIRECT(memory, 4, *(src_mem_u32 + 1));
    EXPECT_EQ(IORD_32DIRECT(memory, 4), (alt_u32) 0xEFCDAB89);
    EXPECT_EQ(IORD(memory, 1), (alt_u32) 0xEFCDAB89);
    EXPECT_EQ(IORD_32DIRECT(memory, 0), alt_u32(0));
    IOWR_32DIRECT(memory, 4, alt_u32(0));
}
