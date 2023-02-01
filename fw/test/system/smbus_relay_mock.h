
#ifndef SYSTEM_SMBUS_RELAY_MOCK_H
#define SYSTEM_SMBUS_RELAY_MOCK_H

// Standard headers
#include <memory>
#include <string>
#include <unordered_map>

// Mock headers
#include "alt_types_mock.h"
#include "memory_mock.h"
#include "unordered_map_memory_mock.h"

// PFR system
#include "pfr_sys.h"

class SMBUS_RELAY_MOCK
{
public:
    SMBUS_RELAY_MOCK();
    virtual ~SMBUS_RELAY_MOCK();

    void reset();

    alt_u32* get_cmd_enable_memory_for_smbus(alt_u32 bus_id);

private:
    // Memory area for SMBus relays
    alt_u32 *m_relay1_cmd_en_mem = nullptr;
    alt_u32 *m_relay2_cmd_en_mem = nullptr;
    alt_u32 *m_relay3_cmd_en_mem = nullptr;
};

#endif /* SYSTEM_SMBUS_RELAY_MOCK_H_ */
