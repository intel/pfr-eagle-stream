
#ifndef SYSTEM_DUAL_CONFIG_MOCK_H
#define SYSTEM_DUAL_CONFIG_MOCK_H

// Standard headers
#include <memory>
#include <string>
#include <unordered_map>
#include <queue>

// Mock headers
#include "alt_types_mock.h"
#include "memory_mock.h"
#include "unordered_map_memory_mock.h"

// PFR system
#include "pfr_sys.h"
#include "dual_config_utils.h"

class DUAL_CONFIG_MOCK : public MEMORY_MOCK_IF
{
public:
    DUAL_CONFIG_MOCK();
    virtual ~DUAL_CONFIG_MOCK();

    void reset() override;

    alt_u32 get_mem_word(void* addr) override;
    void set_mem_word(void* addr, alt_u32 data) override;

    bool is_addr_in_range(void* addr) override;

private:
    // Memory area for dual config IP
    UNORDERED_MAP_MEMORY_MOCK<U_DUAL_CONFIG_BASE, U_DUAL_CONFIG_SPAN> m_dual_config;
};

#endif /* SYSTEM_DUAL_CONFIG_MOCK_H */
