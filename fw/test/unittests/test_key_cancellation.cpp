#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"
#include "testdata_info.h"

class KeyCancellationUtilityTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
        // Perform provisioning
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);
#ifdef GTEST_ATTEST_384
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_ATTEST_EXAMPLE_KEY_FILE);
#endif
    }

    virtual void TearDown() {}
};

TEST_F(KeyCancellationUtilityTest, test_cancel_key)
{
    UFM_PFR_DATA* ufm = get_ufm_pfr_data();

    cancel_key(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 3);
    cancel_key(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 0);
    EXPECT_EQ(ufm->csk_cancel_cpld_update_cap[0], alt_u32(0b01101111111111111111111111111111));

    cancel_key(KCH_PC_PFR_PCH_PFM, 127);
    cancel_key(KCH_PC_PFR_PCH_PFM, 120);
    cancel_key(KCH_PC_PFR_PCH_PFM, 118);
    EXPECT_EQ(ufm->csk_cancel_pch_pfm[3], alt_u32(0b11111111111111111111110101111110));

    cancel_key(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 8);
    EXPECT_EQ(ufm->csk_cancel_pch_update_cap[0], alt_u32(0b11111111011111111111111111111111));

    cancel_key(KCH_PC_PFR_BMC_PFM, 32);
    cancel_key(KCH_PC_PFR_BMC_PFM, 33);
    EXPECT_EQ(ufm->csk_cancel_bmc_pfm[1], alt_u32(0b00111111111111111111111111111111));

    cancel_key(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 66);
    EXPECT_EQ(ufm->csk_cancel_bmc_update_cap[2], alt_u32(0b11011111111111111111111111111111));

#ifdef GTEST_ATTEST_384
    cancel_key(KCH_PC_PFR_AFM, 127);
    cancel_key(KCH_PC_PFR_AFM, 120);
    cancel_key(KCH_PC_PFR_AFM, 118);
    EXPECT_EQ(ufm->csk_cancel_bmc_afm[3], alt_u32(0b11111111111111111111110101111110));
#endif
}

TEST_F(KeyCancellationUtilityTest, test_is_key_cancelled)
{
    cancel_key(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 3);
    cancel_key(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 0);

    cancel_key(KCH_PC_PFR_PCH_PFM, 127);
    cancel_key(KCH_PC_PFR_PCH_PFM, 120);
    cancel_key(KCH_PC_PFR_PCH_PFM, 118);

    cancel_key(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 8);

    cancel_key(KCH_PC_PFR_BMC_PFM, 32);
    cancel_key(KCH_PC_PFR_BMC_PFM, 64);

    cancel_key(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 66);
    cancel_key(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 0);

    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 3));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 2));

    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 127));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 118));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 126));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 2));

    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 8));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 3));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 0));

    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 32));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 64));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 33));

    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 66));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 2));

#ifdef GTEST_ATTEST_384
    cancel_key(KCH_PC_PFR_AFM, 127);
    cancel_key(KCH_PC_PFR_AFM, 120);
    cancel_key(KCH_PC_PFR_AFM, 118);

    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_AFM, 127));
    EXPECT_FALSE(is_csk_key_valid(KCH_PC_PFR_AFM, 118));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_AFM, 126));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_AFM, 2));
#endif
}

TEST_F(KeyCancellationUtilityTest, test_is_key_cancelled_with_example_ufm)
{
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 3));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_CPLD_UPDATE_CAPSULE, 87));

    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 127));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 118));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 126));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_PFM, 2));

    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 8));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 3));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 127));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_PCH_UPDATE_CAPSULE, 99));

    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 32));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 64));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 33));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_PFM, 127));

    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 66));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 0));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 1));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 2));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 100));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_BMC_UPDATE_CAPSULE, 127));

#ifdef GTEST_ATTEST_384
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_AFM, 127));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_AFM, 118));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_AFM, 126));
    EXPECT_TRUE(is_csk_key_valid(KCH_PC_PFR_AFM, 2));
#endif
}
