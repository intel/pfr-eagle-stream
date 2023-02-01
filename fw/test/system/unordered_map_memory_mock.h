#ifndef INC_SYSTEM_UNORDERED_MAP_MEMORY_MOCK_H
#define INC_SYSTEM_UNORDERED_MAP_MEMORY_MOCK_H

// Standard headers
#include <memory>
#include <unordered_map>

// Mock headers
#include "alt_types_mock.h"
#include "memory_mock.h"

// BSP headers
#include "pfr_sys.h"

template <unsigned BASE, unsigned SPAN>
class UNORDERED_MAP_MEMORY_MOCK : public MEMORY_MOCK_IF
{
public:
    UNORDERED_MAP_MEMORY_MOCK() : m_base_addr(BASE), m_span(SPAN) {}
    virtual ~UNORDERED_MAP_MEMORY_MOCK() {}
    alt_u32 get_mem_word(void* addr) override
    {
        std::uintptr_t addr_int = reinterpret_cast<std::uintptr_t>(addr);
        auto it = m_memory.find(addr_int);
        if (it == m_memory.end())
        {
            // Initialize location on access
            m_memory[addr_int] = 0;
            return 0;
        }
        else
        {
            return it->second;
        }
    }
    void set_mem_word(void* addr, alt_u32 data) override
    {
        std::uintptr_t addr_int = reinterpret_cast<std::uintptr_t>(addr);
        m_memory[addr_int] = data;
    }
    void reset() override { m_memory.clear(); }
    bool is_addr_in_range(void* addr) override
    {
        return MEMORY_MOCK_IF::is_addr_in_range(
            addr, __IO_CALC_ADDRESS_NATIVE_ALT_U32(BASE, 0), SPAN);
    }

private:
    std::unordered_map<std::uintptr_t, alt_u32> m_memory;
    alt_u32 m_base_addr;
    alt_u32 m_span;
};

#endif /* INC_SYSTEM_UNORDERED_MAP_MEMORY_MOCK_H */
