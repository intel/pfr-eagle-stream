#ifndef INC_SYSTEM_MEMORY_MOCK_H
#define INC_SYSTEM_MEMORY_MOCK_H

// Standard headers
#include <memory>

// Mock headers
#include "alt_types_mock.h"

// All system mocks should implement this interface
class MEMORY_MOCK_IF
{
public:
    MEMORY_MOCK_IF() {}
    virtual ~MEMORY_MOCK_IF() {}

    virtual alt_u32 get_mem_word(void* addr) = 0;
    virtual void set_mem_word(void* addr, alt_u32 data) = 0;
    virtual void reset() = 0;

    bool is_addr_in_range(std::uintptr_t addr, std::uintptr_t start_addr, alt_u32 span)
    {
        return (addr >= (start_addr)) && (addr < (start_addr + span));
    }
    bool is_addr_in_range(void* addr, void* start_addr, alt_u32 span)
    {
        std::uintptr_t addr_int = reinterpret_cast<std::uintptr_t>(addr);
        std::uintptr_t start_addr_int = reinterpret_cast<std::uintptr_t>(start_addr);
        return is_addr_in_range(addr_int, start_addr_int, span);
    }
    virtual bool is_addr_in_range(void* addr) = 0;
};

#endif /* INC_SYSTEM_MEMORY_MOCK_H */
