#include <iostream>

// Include the GTest headers
#include "gtest_headers.h"

// Include the SYSTEM MOCK and PFR headers
#include "ut_nios_wrapper.h"

class PFRResponderSpdmErrorHandlerTest : public testing::Test
{
public:
    alt_u32* m_memory = nullptr;
    alt_u32* m_mctp_memory = nullptr;

    virtual void SetUp()
    {
        SYSTEM_MOCK::get()->reset();
        m_memory = U_MAILBOX_AVMM_BRIDGE_ADDR;
        m_mctp_memory = U_RFNVRAM_SMBUS_MASTER_ADDR;
    }

    virtual void TearDown() {}
};


TEST_F(PFRResponderSpdmErrorHandlerTest, test_cpld_as_responder_reply_not_ready)
{
    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset everything
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));

    MCTP_CONTEXT mctp_context;
    MCTP_CONTEXT* mctp_context_ptr = &mctp_context;

    reset_buffer((alt_u8*)mctp_context_ptr, sizeof(MCTP_CONTEXT));

    init_spdm_context(spdm_context_ptr);

    // Expect first message from host to request for version
    spdm_context_ptr->spdm_msg_state = SPDM_VERSION_FLAG;

    // Set busy state, meaning CPLD is at not ready state
    spdm_context_ptr->response_state = spdm_not_ready_state;
    mctp_context_ptr->cached_addr = BMC_SLAVE_ADDRESS;
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;

    // Set Challenger host in order to point to correct location
    mctp_context_ptr->challenger_type = BMC;

    // Get the error response code
    alt_u8 mctp_error_pkt[15] = MCTP_SPDM_ERROR_RESPONSE_EXTENDED_ERROR_MSG;
    mctp_error_pkt[UT_MCTP_SPDM_ERROR_RESPONSE_ERROR_CODE] = SPDM_ERROR_CODE_RESPONSE_NOT_READY;
    mctp_error_pkt[UT_MCTP_SPDM_ERROR_RESPONSE_ERROR_DATA] = 0x00;
    mctp_error_pkt[UT_MCTP_BYTE_COUNT_IDX] = 0x0e;
    mctp_error_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    mctp_error_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    mctp_error_pkt[UT_MCTP_SPDM_ERROR_RESPONSE_EXTENDED_ERROR_RDT_EXPONENT] = 0x01;
    mctp_error_pkt[UT_MCTP_SPDM_ERROR_RESPONSE_EXTENDED_ERROR_REQUEST_CODE] = REQUEST_VERSION;
    mctp_error_pkt[UT_MCTP_SPDM_ERROR_RESPONSE_EXTENDED_ERROR_TOKEN] = 0x00;
    mctp_error_pkt[UT_MCTP_SPDM_ERROR_RESPONSE_EXTENDED_ERROR_RDTM] = 0x01;

    alt_u8* pkt = (alt_u8*)&mctp_context_ptr->processed_message.buffer;
    alt_u8 mctp_pkt[4] = SPDM_GET_VERSION_MSG;
    alt_u8_memcpy(pkt, (alt_u8*)&mctp_pkt[0], 4);

    alt_u8 succ_ver_mctp_pkt[19] = MCTP_SPDM_SUCCESSFUL_VERSION_MSG_THREE_VER_ENTRY;
    succ_ver_mctp_pkt[UT_MCTP_SOM_EOM_BYTE_IDX] = FIRST_AND_LAST_PKT;
    succ_ver_mctp_pkt[UT_MCTP_BYTE_COUNT_IDX] = 14;
    succ_ver_mctp_pkt[UT_MCTP_SOURCE_ADDR] = SRC_SLAVE_ADDR_WR;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX] = 0x01;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX + 1] = 0;
    //succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX + 3] = 0;
    succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX + 2] = 0x10;
    //succ_ver_mctp_pkt[UT_MCTP_SPDM_VERSION_ENTRY_COUNT_IDX + 4] = 0x11;

    responder_cpld_respond_version(spdm_context_ptr, mctp_context_ptr, 4);

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(mctp_error_pkt[i], IORD(m_mctp_memory, 0));
    }

    // Set back to normal state
    spdm_context_ptr->response_state = spdm_normal_state;

    responder_cpld_respond_version(spdm_context_ptr, mctp_context_ptr, 4);

    EXPECT_EQ((alt_u32)(BMC_SLAVE_ADDRESS << 1), IORD(m_mctp_memory, 0));
    EXPECT_EQ((alt_u32)MCTP_COMMAND_CODE, IORD(m_mctp_memory, 0));
    for (alt_u32 i = 0; i < 15; i++)
    {
        EXPECT_EQ(succ_ver_mctp_pkt[i], IORD(m_mctp_memory, 0));
    }
}
