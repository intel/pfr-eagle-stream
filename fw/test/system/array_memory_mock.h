#ifndef INC_SYSTEM_ARRAY_MEMORY_MOCK_H
#define INC_SYSTEM_ARRAY_MEMORY_MOCK_H

// Standard headers
#include <memory>
#include <vector>

// Mock headers
#include "alt_types_mock.h"
#include "memory_mock.h"

// BSP headers
#include "pfr_sys.h"

template <unsigned BASE, unsigned SPAN>
class ARRAY_MEMORY_MOCK : public MEMORY_MOCK_IF
{
public:
    ARRAY_MEMORY_MOCK() : m_base_addr(BASE), m_span(SPAN) { m_memory.resize(SPAN / 4); }
    virtual ~ARRAY_MEMORY_MOCK() {}
    alt_u32 get_mem_word(void* addr) override
    {
        std::uintptr_t addr_int = reinterpret_cast<std::uintptr_t>(addr);
        return m_memory[(addr_int - BASE) / 4];
    }
    void set_mem_word(void* addr, alt_u32 data) override
    {
        std::uintptr_t addr_int = reinterpret_cast<std::uintptr_t>(addr);
        m_memory[(addr_int - BASE) / 4] = data;
    }
    void reset() override
    {
        for (auto& elem : m_memory)
        {
            elem = 0;
        }
    }
    bool is_addr_in_range(void* addr) override
    {
        return MEMORY_MOCK_IF::is_addr_in_range(
            addr, __IO_CALC_ADDRESS_NATIVE_ALT_U32(BASE, 0), SPAN);
    }

private:
    std::vector<alt_u32> m_memory;
    alt_u32 m_base_addr;
    alt_u32 m_span;
};

#endif /* INC_SYSTEM_ARRAY_MEMORY_MOCK_H */
