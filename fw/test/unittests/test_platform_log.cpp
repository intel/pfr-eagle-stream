#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class PlatformLogTest : public testing::Test
{
public:
    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
        ut_reset_nios_fw();
    }

    virtual void TearDown() {}
};

TEST_F(PlatformLogTest, test_log_wdt_recovery)
{
    log_wdt_recovery(LAST_RECOVERY_ACM_LAUNCH_FAIL, LAST_PANIC_ACM_BIOS_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_ACM_LAUNCH_FAIL));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(1));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_ACM_BIOS_WDT_EXPIRED));

    log_wdt_recovery(LAST_RECOVERY_ME_LAUNCH_FAIL, LAST_PANIC_ME_WDT_EXPIRED);
    EXPECT_EQ(read_from_mailbox(MB_RECOVERY_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_RECOVERY_REASON), alt_u32(LAST_RECOVERY_ME_LAUNCH_FAIL));
    EXPECT_EQ(read_from_mailbox(MB_PANIC_EVENT_COUNT), alt_u32(2));
    EXPECT_EQ(read_from_mailbox(MB_LAST_PANIC_REASON), alt_u32(LAST_PANIC_ME_WDT_EXPIRED));
}
