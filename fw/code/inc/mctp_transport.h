// (C) 2019 Intel Corporation. All rights reserved.
// Your use of Intel Corporation's design tools, logic functions and other
// software and tools, and its AMPP partner logic functions, and any output
// files from any of the foregoing (including device programming or simulation
// files), and any associated documentation or information are expressly subject
// to the terms and conditions of the Intel Program License Subscription
// Agreement, Intel FPGA IP License Agreement, or other applicable
// license agreement, including, without limitation, that your use is for the
// sole purpose of programming logic devices manufactured by Intel and sold by
// Intel or its authorized distributors.  Please refer to the applicable
// agreement for further details.

/**
 * @file mctp_utils.h
 * @brief Utility functions to handle MCTP message transaction.
 */

#ifndef EAGLESTREAM_INC_MCTP_TRANSPORT_H_
#define EAGLESTREAM_INC_MCTP_TRANSPORT_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"
#include "mctp_utils.h"

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/**
 * @brief This function to retrieve i2c messages
 *
 * @param mctp_ctx_ptr : mctp context
 * @param timeout : timeout
 *
 * @return 0 if non SPDM message, otherwise size of received SPDM message
 */

static alt_u32 receive_i2c_message(MCTP_CONTEXT* mctp_ctx_ptr, alt_u32 timeout)
{
    // Default to i2c/SMBus
    mctp_ctx_ptr->bus_type = 0;
    mctp_ctx_ptr->processed_message.buffer_size = receive_message(mctp_ctx_ptr, (alt_u8*)&mctp_ctx_ptr->processed_message.buffer, timeout);

#ifdef USE_SYSTEM_MOCK
    SYSTEM_MOCK::get()->stage_next_spdm_message();
#endif
    if (mctp_ctx_ptr->cached_msg_type == MCTP_MESSAGE_TYPE)
    {
        // Call read to decode spdm
        return mctp_ctx_ptr->processed_message.buffer_size;
    }
    return 0;
}

/**
 * @brief This function is used when CPLD is a requester.
 *
 * Send and receive the message (MCTP + SPDM) from host by reading from Mailbox Fifo
 *
 * @param mctp_ctx : mctp context
 * @param send_data : message to send
 * @param send_data_size : size
 * @param timeout : timeout
 *
 * @return 0 if error from MCTP layer or timeout, otherwise size of received message
 */
static alt_u32 send_and_receive_message(MCTP_CONTEXT* mctp_ctx, alt_u8* send_data, alt_u32 send_data_size,
                                        alt_u32 timeout)
{
    send_message(mctp_ctx, send_data, send_data_size);
    if (mctp_ctx->error_type)
    {
        return 0;
    }

    return receive_i2c_message(mctp_ctx, timeout);
}

#endif

#endif /* EAGLESTREAM_INC_MCTP_TRANSPORT_H_ */
