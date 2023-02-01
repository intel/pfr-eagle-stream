#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class PFRSpdmUtilsTest : public testing::Test
{
public:
    alt_u32* m_memory = nullptr;
    alt_u32* m_mctp_memory = nullptr;

    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
        m_memory = U_MAILBOX_AVMM_BRIDGE_ADDR;
        m_mctp_memory = U_RFNVRAM_SMBUS_MASTER_ADDR;
    }

    virtual void TearDown() {}
};

TEST_F(PFRSpdmUtilsTest, test_alt_u8_memcpy)
{
    alt_u8 test_data_1[100] = {0};

    alt_u8 test_data_2 = 0xde;
    alt_u8 test_data_3[10];

    for (alt_u32 i = 0 ; i < 10; i++)
    {
        test_data_3[i] = 0xaa;
    }

    alt_u8_memcpy((alt_u8*)test_data_1, (alt_u8*)test_data_3, 10);

    for (alt_u32 i = 0 ; i < 10; i++)
    {
        EXPECT_EQ(test_data_1[i], test_data_3[i]);
    }

    alt_u8_memcpy((alt_u8*)test_data_1, &test_data_2, 1);

    for (alt_u32 i = 1 ; i < 10; i++)
    {
    	EXPECT_EQ(test_data_1[i], test_data_3[i]);
    }

    EXPECT_EQ(test_data_1[0], test_data_2);
}

TEST_F(PFRSpdmUtilsTest, test_reset_buffer)
{
    alt_u8 test_data[100] = {0};

    for (alt_u32 i = 0; i < 50; i++)
    {
        test_data[i] = 0xaa;
    }

    for (alt_u32 i = 50; i < 100; i++)
    {
        test_data[i] = 0xbb;
    }

    reset_buffer((alt_u8*)test_data, 50);

    for (alt_u32 i = 0; i < 50; i++)
    {
    	EXPECT_EQ(test_data[i], 0);
    }

    for (alt_u32 i = 50; i < 100; i++)
    {
    	EXPECT_EQ(test_data[i], 0xbb);
    }

    reset_buffer((alt_u8*)test_data, 100);

    for (alt_u32 i = 0; i < 100; i++)
    {
    	EXPECT_EQ(test_data[i], 0);
    }
}

TEST_F(PFRSpdmUtilsTest, test_append_small_and_large_message)
{
    SMALL_APPENDED_BUFFER small_msg;
    SMALL_APPENDED_BUFFER* small_msg_ptr = &small_msg;

    LARGE_APPENDED_BUFFER large_msg;
    LARGE_APPENDED_BUFFER* large_msg_ptr = &large_msg;

    small_msg_ptr->buffer_size = 0;
    large_msg_ptr->buffer_size = 0;
    reset_buffer((alt_u8*)small_msg_ptr->buffer, SMALL_SPDM_MESSAGE_MAX_BUFFER_SIZE);

    reset_buffer((alt_u8*)large_msg_ptr->buffer, LARGE_SPDM_MESSAGE_MAX_BUFFER_SIZE);

    alt_u8 test_data1[128] = {0};
    alt_u8 test_data2[128] = {0};

    for (alt_u32 i = 0; i < 128; i++)
    {
    	test_data1[i] = 0xaa;
    }

    for (alt_u32 i = 0; i < 128; i++)
    {
    	test_data2[i] = 0xbb;
    }

    append_small_buffer(small_msg_ptr, (alt_u8*)test_data1, 128);
    append_small_buffer(small_msg_ptr, (alt_u8*)test_data2, 128);

    append_large_buffer(large_msg_ptr, (alt_u8*)test_data1, 128);
    append_large_buffer(large_msg_ptr, (alt_u8*)test_data2, 128);
    append_large_buffer(large_msg_ptr, (alt_u8*)test_data1, 128);
    append_large_buffer(large_msg_ptr, (alt_u8*)test_data2, 128);

    for (alt_u32 i = 0; i < 128/4; i++)
    {
        EXPECT_EQ(small_msg_ptr->buffer[i], 0xaaaaaaaa);
        EXPECT_EQ(large_msg_ptr->buffer[i], 0xaaaaaaaa);
    }

    for (alt_u32 i = 128/4; i < 256/4; i++)
    {
        EXPECT_EQ(small_msg_ptr->buffer[i], 0xbbbbbbbb);
        EXPECT_EQ(large_msg_ptr->buffer[i], 0xbbbbbbbb);
    }

    for (alt_u32 i = 256/4; i < 384/4; i++)
    {
        EXPECT_EQ(large_msg_ptr->buffer[i], 0xaaaaaaaa);
    }

    for (alt_u32 i = 384/4; i < 512/4; i++)
    {
        EXPECT_EQ(large_msg_ptr->buffer[i], 0xbbbbbbbb);
    }
}

TEST_F(PFRSpdmUtilsTest, test_shrink_large_message)
{
    LARGE_APPENDED_BUFFER large_msg;
    LARGE_APPENDED_BUFFER* large_msg_ptr = &large_msg;

    large_msg_ptr->buffer_size = 0;

    reset_buffer((alt_u8*)large_msg_ptr->buffer, LARGE_SPDM_MESSAGE_MAX_BUFFER_SIZE);

    alt_u8 test_data1[128] = {0};
    alt_u8 test_data2[128] = {0};

    for (alt_u32 i = 0; i < 128; i++)
    {
    	test_data1[i] = 0xaa;
    }

    for (alt_u32 i = 0; i < 128; i++)
    {
    	test_data2[i] = 0xbb;
    }

    append_large_buffer(large_msg_ptr, (alt_u8*)test_data1, 128);
    append_large_buffer(large_msg_ptr, (alt_u8*)test_data1, 128);

    for (alt_u32 i = 0; i < 256/4; i++)
    {
        EXPECT_EQ(large_msg_ptr->buffer[i], 0xaaaaaaaa);
    }

    shrink_large_buffer(large_msg_ptr, 128);
    shrink_large_buffer(large_msg_ptr, 128);
    append_large_buffer(large_msg_ptr, (alt_u8*)test_data2, 128);
    append_large_buffer(large_msg_ptr, (alt_u8*)test_data2, 128);

    for (alt_u32 i = 128/4; i < 256/4; i++)
    {
        EXPECT_EQ(large_msg_ptr->buffer[i], 0xbbbbbbbb);
    }
}
