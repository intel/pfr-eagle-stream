#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the PFR headers
// Always include the BSP mock then pfr_sys.h first
#include "bsp_mock.h"
#include "pfr_sys.h"

// Other PFR headers
#include "ufm.h"
#include "ufm_utils.h"

class UFMUtilsTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
        SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);
    }
    virtual void TearDown() {}
};

TEST_F(UFMUtilsTest, test_svn_read)
{
    get_ufm_pfr_data()->svn_policy_cpld[0] = 0xfffffffc;
    get_ufm_pfr_data()->svn_policy_cpld[1] = 0xffffffff;

    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), (alt_u32) 2);
}

TEST_F(UFMUtilsTest, test_svn_write)
{
    get_ufm_pfr_data()->svn_policy_pch[0] = 0xfffffffc;
    get_ufm_pfr_data()->svn_policy_pch[1] = 0xffffffff;

    write_ufm_svn(UFM_SVN_POLICY_PCH, 3);

    EXPECT_EQ(get_ufm_pfr_data()->svn_policy_pch[0], (alt_u32) 0xfffffff8);
    EXPECT_EQ(get_ufm_pfr_data()->svn_policy_pch[1], (alt_u32) 0xffffffff);
}

TEST_F(UFMUtilsTest, test_svn_read_write)
{
    get_ufm_pfr_data()->svn_policy_bmc[0] = 0xffffffff;
    get_ufm_pfr_data()->svn_policy_bmc[1] = 0xffffffff;
    for (alt_u32 i = 0; i < 65; i++)
    {
        write_ufm_svn(UFM_SVN_POLICY_BMC, i);
        EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_BMC), (alt_u32) i);
    }
}

TEST_F(UFMUtilsTest, test_read_svn_policies)
{
    // Erase provisioning
    SYSTEM_MOCK::get()->erase_ufm_page(0);

    // Before provisioning UFM data, the SVN policies are the default values (i.e. 0)
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), (alt_u32) 0);
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_PCH), (alt_u32) 0);
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_BMC), (alt_u32) 0);

    // Provision UFM
    SYSTEM_MOCK::get()->provision_ufm_data(UFM_PFR_DATA_EXAMPLE_KEY_FILE);

    // Check expected SVN policies after provisioning
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), (alt_u32) 0);
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_PCH), (alt_u32) 0);
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_BMC), (alt_u32) 0);
}

TEST_F(UFMUtilsTest, test_max_svn_policies)
{
    get_ufm_pfr_data()->svn_policy_bmc[0] = 0;
    get_ufm_pfr_data()->svn_policy_bmc[1] = 0;

    // Check expected SVN policies after provisioning
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_BMC), (alt_u32) UFM_MAX_SVN);
}

TEST_F(UFMUtilsTest, test_is_svn_valid)
{
    write_ufm_svn(UFM_SVN_POLICY_PCH, 3);
    write_ufm_svn(UFM_SVN_POLICY_BMC, 9);

    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_PCH, 1));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 3));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 32));

    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 7));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 9));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 128));
}

TEST_F(UFMUtilsTest, test_svn_upgrade)
{
    // Checks before SVN upgrade
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 3));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 9));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 32));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 36));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, (alt_u32) UFM_MAX_SVN + 1));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0));

    // First upgrade to SVN 3 (i.e. all images signed with SVN < 3 are invalid)
    write_ufm_svn(UFM_SVN_POLICY_BMC, 3);

    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_BMC), (alt_u32) 3);
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_PCH), (alt_u32) 0);
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), (alt_u32) 0);

    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 3));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 9));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 32));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 36));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, (alt_u32) UFM_MAX_SVN + 1));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0));

    // Second upgrade to SVN 33 (i.e. all images signed with SVN < 33 are invalid)
    write_ufm_svn(UFM_SVN_POLICY_BMC, 33);

    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_BMC), (alt_u32) 33);
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_PCH), (alt_u32) 0);
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), (alt_u32) 0);

    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 0));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 3));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 9));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 32));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_BMC, 36));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, (alt_u32) UFM_MAX_SVN + 1));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0));

    // Third upgrade to SVN 60 (i.e. all images signed with SVN < 60 are invalid)
    write_ufm_svn(UFM_SVN_POLICY_BMC, 60);

    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_BMC), (alt_u32) 60);
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_PCH), (alt_u32) 0);
    EXPECT_EQ(get_ufm_svn(UFM_SVN_POLICY_CPLD), (alt_u32) 0);

    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 0));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 3));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 9));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 32));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, 36));
    EXPECT_FALSE(is_svn_valid(UFM_SVN_POLICY_BMC, (alt_u32) UFM_MAX_SVN + 1));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_PCH, 0));
    EXPECT_TRUE(is_svn_valid(UFM_SVN_POLICY_CPLD, 0));
}
