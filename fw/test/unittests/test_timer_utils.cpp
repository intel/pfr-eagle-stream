// Include the GTest headers
#include "gtest_headers.h"

#include <math.h>
#include <chrono>
#include <thread>
#include <stdlib.h>

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class PFRTimerUtilsTest : public testing::Test
{
public:
    virtual void SetUp() { SYSTEM_MOCK::get()->reset(); }

    virtual void TearDown() {}
};

TEST_F(PFRTimerUtilsTest, test_sanity)
{
    EXPECT_EQ(U_TIMER_BANK_TIMER_ACTIVE_MASK, 1 << U_TIMER_BANK_TIMER_ACTIVE_BIT);
}

TEST_F(PFRTimerUtilsTest, test_check_timer_expired)
{
    // Countdown from 20ms
    start_timer(U_TIMER_BANK_TIMER1_ADDR, 1);
    EXPECT_FALSE(is_timer_expired(U_TIMER_BANK_TIMER1_ADDR));

    // Sleep for 500ms. Timer should be expired.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_TRUE(is_timer_expired(U_TIMER_BANK_TIMER1_ADDR));
}

TEST_F(PFRTimerUtilsTest, test_pause_timer)
{
    // Countdown from 40ms
    start_timer(U_TIMER_BANK_TIMER2_ADDR, 2);
    pause_timer(U_TIMER_BANK_TIMER2_ADDR);

    // Sleep for 100 ms.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Since we paused the timer, it should not have expired
    EXPECT_FALSE(is_timer_expired(U_TIMER_BANK_TIMER2_ADDR));

    resume_timer(U_TIMER_BANK_TIMER2_ADDR);

    // Sleep for 500 ms.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // Since we resumed the timer, it should be expired by now
    EXPECT_TRUE(is_timer_expired(U_TIMER_BANK_TIMER2_ADDR));
}

TEST_F(PFRTimerUtilsTest, DISABLED_test_sleep_20ms)
{
    auto clock_start = std::chrono::steady_clock::now();

    // Sleep for 20ms.
    sleep_20ms(1);

    auto clock_end = std::chrono::steady_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(clock_end - clock_start);

    // Expect the delay is within 1 second (giving some tolerance)
    EXPECT_LE((alt_u32) duration_us.count(), alt_u32(1000000));
}

TEST_F(PFRTimerUtilsTest, test_restart_timer)
{
    start_timer(U_TIMER_BANK_TIMER3_ADDR, 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_TRUE(is_timer_expired(U_TIMER_BANK_TIMER3_ADDR));

    // Restart the timer
    start_timer(U_TIMER_BANK_TIMER3_ADDR, 100);
    // Timer should be active
    EXPECT_FALSE(is_timer_expired(U_TIMER_BANK_TIMER3_ADDR));
    EXPECT_GT(IORD(U_TIMER_BANK_TIMER3_ADDR, 0) & U_TIMER_BANK_TIMER_VALUE_MASK, (alt_u32) 0);
    EXPECT_TRUE(IORD(U_TIMER_BANK_TIMER3_ADDR, 0) & U_TIMER_BANK_TIMER_ACTIVE_MASK);
    EXPECT_TRUE(check_bit(U_TIMER_BANK_TIMER3_ADDR, U_TIMER_BANK_TIMER_ACTIVE_BIT));
}

#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)
TEST_F(PFRTimerUtilsTest, test_check_timer_4_expired)
{
    // Countdown from 20ms
    start_timer(U_TIMER_BANK_TIMER4_ADDR, 1);
    EXPECT_FALSE(is_timer_expired(U_TIMER_BANK_TIMER4_ADDR));

    // Sleep for 500ms. Timer should be expired.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_TRUE(is_timer_expired(U_TIMER_BANK_TIMER4_ADDR));
}

TEST_F(PFRTimerUtilsTest, test_compute_spdm_t2_time_unit_test_1)
{
    // Compute T2 time unit

    // Expect to return the correspondant unit
    // The Hw timer resolution is set to 20ms
    EXPECT_EQ(compute_spdm_t2_time_unit(20), (alt_u32)124);
}

TEST_F(PFRTimerUtilsTest, test_compute_spdm_t2_time_unit_test_2)
{
    // Compute T2 time unit

    // Expect to return the correspondant unit
    // The Hw timer resolution is set to 20ms
    EXPECT_EQ(compute_spdm_t2_time_unit(17), (alt_u32)78);
}
#endif
