#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class NiosFWInitializationTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
    }

    virtual void TearDown() {}
};

TEST_F(NiosFWInitializationTest, test_cpld_hash)
{
    /*
     * Run Nios FW to T-1 in unprovisioned state.
     */
    ut_prep_nios_gpi_signals();

    // Insert the T0_OPERATIONS code block (break out of T0 loop)
    SYSTEM_MOCK::get()->insert_code_block(SYSTEM_MOCK::CODE_BLOCK_TYPES::T0_OPERATIONS);

    // Load active CPLD image to CFM1
    SYSTEM_MOCK::get()->load_active_image_to_cfm1();

    // Run PFR Main. Always run with the timeout
    ASSERT_DURATION_LE(GTEST_TIMEOUT_SHORT, pfr_main());

    /*
     * Check if CPLD hash is reported correctly to the mailbox.
     */
    alt_u8 cfm1_hash[CFM1_ACTIVE_IMAGE_FILE_HASH_SIZE] = CFM1_ACTIVE_IMAGE_FILE_HASH;
#ifdef GTEST_USE_CRYPTO_384
    for (alt_u32 byte_i = 0; byte_i < SHA384_LENGTH; byte_i++)
    {
        EXPECT_EQ(IORD(U_MAILBOX_AVMM_BRIDGE_BASE, MB_CPLD_HASH + byte_i), cfm1_hash[byte_i]);
    }
#else
    for (alt_u32 byte_i = 0; byte_i < SHA256_LENGTH; byte_i++)
    {
        EXPECT_EQ(IORD(U_MAILBOX_AVMM_BRIDGE_BASE, MB_CPLD_HASH + byte_i), cfm1_hash[byte_i]);
    }
#endif
}
