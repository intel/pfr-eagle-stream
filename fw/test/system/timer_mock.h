#ifndef INC_SYSTEM_TIMER_MOCK_H
#define INC_SYSTEM_TIMER_MOCK_H

// Standard headers
#include <memory>
#include <vector>
#include <array>
#include <chrono>
#include <thread>

// Mock headers
#include "alt_types_mock.h"
#include "memory_mock.h"

// BSP headers
#include "pfr_sys.h"
#include "timer_utils.h"

class TIMER_MOCK : public MEMORY_MOCK_IF
{
public:
    TIMER_MOCK();
    virtual ~TIMER_MOCK();

    alt_u32 get_mem_word(void* addr) override;
    void set_mem_word(void* addr, alt_u32 data) override;

    void reset() override;

    bool is_addr_in_range(void* addr) override;

private:
    alt_u32 m_timer_bank_timer1;
    alt_u32 m_timer_bank_timer2;
    alt_u32 m_timer_bank_timer3;
#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)
    alt_u32 m_timer_bank_timer4;
    std::chrono::time_point<std::chrono::steady_clock> m_clock_timer4;
    alt_u32 m_timer_bank_timer5;
    std::chrono::time_point<std::chrono::steady_clock> m_clock_timer5;
#endif

    std::chrono::time_point<std::chrono::steady_clock> m_clock_timer1;
    std::chrono::time_point<std::chrono::steady_clock> m_clock_timer2;
    std::chrono::time_point<std::chrono::steady_clock> m_clock_timer3;

    alt_u32 get_20ms_passed(std::chrono::time_point<std::chrono::steady_clock> clk);
    void update_timers();
};

#endif /* INC_SYSTEM_TIMER_MOCK_H */
