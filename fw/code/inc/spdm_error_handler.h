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
 * @file spdm_error_handler.h
 * @brief Error handling for SPDM flow.
 */

#ifndef EAGLESTREAM_INC_SPDM_ERROR_HANDLER_H_
#define EAGLESTREAM_INC_SPDM_ERROR_HANDLER_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "spdm.h"
#include "mctp_transport.h"
#include "spdm_utils.h"

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

// A series of SPDM error code used by CPLD
#define SPDM_RESPONSE_SUCCESS                  0
#define SPDM_RESPONSE_NOT_READY                0b1
#define SPDM_RESPONSE_ERROR                    0b10
#define SPDM_RESPONSE_ERROR_TIMEOUT            0b100
#define SPDM_RESPONSE_ERROR_MEAS_MISMATCH      0b1000
#define SPDM_RESPONSE_ERROR_INVALID_REQUEST    0b10000
#define SPDM_RESPONSE_ERROR_CHAL_AUTH          0b100000
// Used for communication fail with SMBus master that CPLD not being able to send out messages after SCL is held low
#define SPDM_RESPONSE_ERROR_COMMUNICATION_FAIL 0b1000000

// Combination of bit mask above
// This combination determines which error mask to retry
#define SPDM_RESPONSE_ERROR_REQUIRE_RETRY (SPDM_RESPONSE_NOT_READY | SPDM_RESPONSE_ERROR_TIMEOUT)

/*
static alt_u32 spdm_respond_invalid_request(MCTP_CONTEXT* mctp_ctx, ATTESTATION_HOST challenger)
{
    ERROR_RESPONSE_CODE invalid_request;
    ERROR_RESPONSE_CODE* invalid_request_ptr = &invalid_request;
    invalid_request_ptr->header.spdm_version = SPDM_VERSION;
    invalid_request_ptr->header.request_response_code = RESPONSE_ERROR;
    invalid_request_ptr->header.param_1 = SPDM_ERROR_CODE_INVALID_REQUEST;
    invalid_request_ptr->header.param_2 = 0;

    send_message(mctp_ctx, (alt_u8*)invalid_request_ptr, sizeof(ERROR_RESPONSE_CODE), challenger);
    return SPDM_RESPONSE_ERROR_INVALID_REQUEST;
}*/

/*
 * @brief Handle SPDM error response code from responder or timeout.
 *
 * This function handles SPDM response error code:
 * 1) SPDM_ERROR_CODE_BUSY
 * - Nios handles this error code by retrying the request up to 'n' times set in SPDM context.
 *
 * 2) SPDM_ERROR_CODE_REQUEST_RESYNCH
 * - Nios handles this error code by resetting the message state back to VERSION if the responder needs
 *   to re-sync with CPLD.
 *
 * and handles any response message not received by CPLD within a given time.
 *
 * TODO: To support more error response code when there are more use cases
 *
 * @param context : SPDM info
 * @param error_code : SPDM error code
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 spdm_requester_simple_error(SPDM_CONTEXT* context, alt_u8 error_code)
{
    if (error_code == SPDM_ERROR_CODE_BUSY)
    {
        // If responder replied with SPDM BUSY code, CPLD will retry up to 'n' times set in context
        shrink_large_buffer(&context->transcript.m1_m2, context->previous_cached_size);
        context->transcript.l1_l2.buffer_size = 0;
        return SPDM_RESPONSE_NOT_READY;
    }
    else if (error_code == SPDM_ERROR_CODE_REQUEST_RESYNCH)
    {
        // Reset the message transaction back to GET_VERSION
        context->spdm_msg_state &= 0;
    }
    else
    {
        // If response not received within a given time
        shrink_large_buffer(&context->transcript.m1_m2, context->previous_cached_size);
        context->transcript.l1_l2.buffer_size = 0;
        return SPDM_RESPONSE_ERROR_TIMEOUT;
    }

    return SPDM_RESPONSE_ERROR;
}

/*
 * @brief Handle SPDM error response code SPDM_ERROR_RESPONSE_NOT_READY.
 * Generate SPDM_ERROR_RESPONSE_IF_READY code.
 *
 * This function handles SPDM response error code:
 * 1) SPDM_ERROR_RESPONSE_NOT_READY
 * - Nios handles this error code by generating a REQUEST_RESPOND_IF_READY message and
 *   wait for the next response from the responder. The next response message is expected to be the
 *   current state of message. For example, if CHALLENGE is not ready, CPLD will wait for WTMax
 *   calculated from rdt exponent and rdtm for that CHALLENGE message.
 *
 * and handles any response message not received by CPLD within a given time.
 *
 *
 * @param context : SPDM info
 * @param response : Responded message
 * @param expected_response_code : Expected message code
 * @param expected_response_size : Ecpected message size
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 spdm_requester_response_if_ready(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx, alt_u8 expected_response_code,
                                                alt_u32 expected_response_size)
{
    alt_u32 status = 0;
    // Generate respond if ready error code
    ERROR_RESPONSE_CODE respond_if_ready;
    ERROR_RESPONSE_CODE* respond_if_ready_ptr = &respond_if_ready;
    respond_if_ready_ptr->header.spdm_version = SPDM_VERSION;
    respond_if_ready_ptr->header.request_response_code = REQUEST_RESPOND_IF_READY;
    respond_if_ready_ptr->header.param_1 = context->error_data.request_code;
    respond_if_ready_ptr->header.param_2 = context->error_data.token;

    // Wait for WTMax by getting the rdt exponent and rdtm
    context->timeout = compute_spdm_wt_max_time_unit(context->error_data.rdt_exponent, context->error_data.rdtm);

    // Send message and wait for a respond at a specified time
    status = send_and_receive_message(mctp_ctx, (alt_u8*)respond_if_ready_ptr, sizeof(ERROR_RESPONSE_CODE), context->timeout);

    alt_u8* response = (alt_u8*)&mctp_ctx->processed_message.buffer;
    // Handles timeout or message not received
    if (mctp_ctx->error_type)
    {
        return SPDM_RESPONSE_ERROR_COMMUNICATION_FAIL;
    }
    else if (!status)
    {
        // Wait for WTMax, else timeout.
        return SPDM_RESPONSE_ERROR_TIMEOUT;
    }
    else if (status > expected_response_size)
    {
        // Check max size of responded message
        return SPDM_RESPONSE_ERROR;
    }
    if (((SPDM_HEADER*)response)->request_response_code != expected_response_code)
    {
        // If the message state is mismatched
        return SPDM_RESPONSE_ERROR;
    }
    return SPDM_RESPONSE_SUCCESS;
}

/*
 * @brief Handle SPDM error response code SPDM_ERROR_RESPONSE_NOT_READY.
 *
 * This function handles SPDM response error code:
 * 1) SPDM_ERROR_RESPONSE_NOT_READY
 * - Nios handles this error code by caching all the error data related to the particular message state.
 *   For example, the rdtm, error code etc...
 *
 * This function then calls spdm_requester_response_if_ready(...), to create a respond packet to repsonder.
 *
 *
 * @param context : SPDM info
 * @param response : Responded message
 * @param expected_response_code : Expected message code
 * @param expected_response_size : Ecpected message size
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 spdm_requester_response_not_ready(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx, alt_u8 expected_response_code,
                                                 alt_u32 expected_response_size)
{
    SPDM_ERROR_RESPONSE_NOT_READY* ext_error_data;
    alt_u8* response = (alt_u8*)&mctp_ctx->processed_message.buffer;
    ext_error_data = (SPDM_ERROR_RESPONSE_NOT_READY*)(((ERROR_RESPONSE_CODE*)response) + 1);

    // Cache error data for later
    context->error_data.rdt_exponent = ext_error_data->rdt_exponent;
    context->error_data.rdtm = ext_error_data->rdtm;
    context->error_data.request_code = ext_error_data->request_code;
    context->error_data.token = ext_error_data->token;

    // Process the message and then return to SPDM handling functions
    return spdm_requester_response_if_ready(context, mctp_ctx, expected_response_code, expected_response_size);
}

/*
 * @brief Generate an extended error code when CPLD is a responder.
 *
 * NOTE: This function is not yet utilized today and is for future development as more use cases are
 * created depending on the devices and their environment.
 *
 * This function is to handle any SPDM transaction when CPLD (as a responder) is not able to serve the
 * requester at that given time. For example, CPLD is busy handling a device A and device B request CPLD
 * to respond at the same time. CPLD do not implement ISR (for code space saving) therefore, the requirement
 * is to only handle a requester one at a time.
 *
 * For example, CPLD generates SPDM_ERROR_RESPONSE_NOT_READY with this function.
 *
 * @param context : SPDM info
 * @param error_code : Message code
 * @param error_data : Message data
 * @param extended_error_data : Extended message data
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 spdm_responder_generate_extended_error(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx, alt_u8 error_code, alt_u8 error_data,
                                                      alt_u8* extended_error_data)
{
    alt_u8 response_buf[SMALL_SPDM_MESSAGE_MAX_BUFFER_SIZE] = {0};
    // Creates the error response message
    ERROR_RESPONSE_CODE* spdm_response = (ERROR_RESPONSE_CODE*) &response_buf;
    spdm_response->header.spdm_version = SPDM_VERSION;
    spdm_response->header.request_response_code = RESPONSE_ERROR;
    spdm_response->header.param_1 = error_code;
    spdm_response->header.param_2 = error_data;

    alt_u32 respond_size = sizeof(ERROR_RESPONSE_CODE) + sizeof(SPDM_ERROR_RESPONSE_NOT_READY);

    alt_u8_memcpy((alt_u8*)(spdm_response + 1), extended_error_data, sizeof(SPDM_ERROR_RESPONSE_NOT_READY));

    mctp_ctx->cached_pkt_info &= ~TAG_OWNER;
    assemble_and_send_mctp_pkt(mctp_ctx, (alt_u8*)spdm_response, respond_size);

    if (mctp_ctx->error_type)
    {
        return SPDM_RESPONSE_ERROR_COMMUNICATION_FAIL;
    }
    return SPDM_RESPONSE_SUCCESS;
}

/*
 * @brief Generate an error code when CPLD is a responder.
 *
 * NOTE: This function is not yet utilized today and is for future development as more use cases are
 * created depending on the devices and their environment.
 *
 * This function is to handle any SPDM transaction when CPLD (as a responder) is not able to serve the
 * requester at that given time. For example, CPLD is busy handling a device A and device B request CPLD
 * to respond at the same time. CPLD do not implement ISR (for code space saving) therefore, the requirement
 * is to only handle a requester one at a time.
 *
 *For example, CPLD generates SPDM_ERROR_CODE_BUSY with this function.
 *
 * @param context : SPDM info
 * @param error_code : Message code
 * @param error_data : Message data
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 spdm_responder_generate_error(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx, alt_u8 error_code, alt_u8 error_data)
{
    ERROR_RESPONSE_CODE response_buf;
    ERROR_RESPONSE_CODE* spdm_response = (ERROR_RESPONSE_CODE*) &response_buf;

    // Creates the error response code (not extended)
    spdm_response->header.spdm_version = SPDM_VERSION;
    spdm_response->header.request_response_code = RESPONSE_ERROR;
    spdm_response->header.param_1 = error_code;
    spdm_response->header.param_2 = error_data;

    alt_u32 respond_size = sizeof(ERROR_RESPONSE_CODE);

    send_message(mctp_ctx, (alt_u8*)spdm_response, respond_size);

    if (mctp_ctx->error_type)
    {
        return SPDM_RESPONSE_ERROR_COMMUNICATION_FAIL;
    }
    return SPDM_RESPONSE_SUCCESS;
}

/*
 * @brief Error handler function when CPLD is a responder.
 *
 * NOTE: This function is not yet utilized today and is for future development as more use cases are
 * created depending on the devices and their environment.
 *
 * This function is to handle any SPDM transaction when CPLD (as a responder) is not able to serve the
 * requester at that given time. For example, CPLD is busy handling a device A and device B request CPLD
 * to respond at the same time. CPLD do not implement ISR (for code space saving) therefore, the requirement
 * is to only handle a requester one at a time.
 *
 * For example, CPLD generates SPDM_ERROR_CODE_BUSY with this function.
 *
 * @param context      : SPDM info
 * @param mctp_ctx     : MCTP info
 * @param error_code   : Message code
 * @param error_data   : Message data
 * @param request_size : size
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 spdm_responder_error_handler(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx, alt_u8 request_code, alt_u8* request,
                                            alt_u32 request_size)
{
    if (context->response_state == spdm_not_ready_state)
    {
        // Handles spdm extended error code
        // Generate error code SPDM_ERROR_RESPONSE_NOT_READY for a particular message
        context->cached_spdm_request_size = request_size;
        alt_u8_memcpy((alt_u8*)&context->cached_spdm_request[0], request, request_size);

        // Simply hardcode rdtm and rdt exponent for now
        // TODO: rdtm and rdt exponent can be set in context
        context->error_data.rdt_exponent = 1;
        context->error_data.rdtm = 1;
        context->error_data.request_code = request_code;
        context->error_data.token = context->token++;

        // Creates and send extended error code
        return spdm_responder_generate_extended_error(context, mctp_ctx, SPDM_ERROR_CODE_RESPONSE_NOT_READY, 0,
                                                      (alt_u8*)&context->error_data);
    }
    else if (context->response_state == spdm_busy_state)
    {
        // Generate error code SPDM_ERROR_CODE_BUSY for a particular message
        // Expects retries from Requester
        return spdm_responder_generate_error(context, mctp_ctx, SPDM_ERROR_CODE_BUSY, 0);
    }
    context->response_state = spdm_normal_state;
    return SPDM_RESPONSE_SUCCESS;
}

/*
 * @brief Error handler function when CPLD is a requester.
 *
 * This function is to handle any SPDM transaction when responder could not reply to CPLD (as a requester)
 * at a given time. Handles simple error such as timeout, busy response and response not ready message.
 *
 * For example, this function is used when no message is responded at a given time or there is an
 * SPDM response error code message.
 *
 * @param context : SPDM info
 * @param response : Responded message
 * @param expected_response_code : Expected message code
 * @param expected_response_size : Expected size
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 spdm_requester_error_handler(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx, alt_u8 expected_response_code,
                                            alt_u32 expected_response_size)
{
    alt_u8* response = (alt_u8*)&mctp_ctx->processed_message.buffer;
    if (mctp_ctx->error_type)
    {
        return SPDM_RESPONSE_ERROR_COMMUNICATION_FAIL;
    }
    else if (((SPDM_HEADER*)response)->param_1 != SPDM_ERROR_CODE_RESPONSE_NOT_READY)
    {
        // Handles timeout error, busy response etc....
        return spdm_requester_simple_error(context, ((SPDM_HEADER*)response)->param_1);
    }
    // Handles respond not ready message
    return spdm_requester_response_not_ready(context, mctp_ctx, expected_response_code, expected_response_size);
}

#endif

#endif /* EAGLESTREAM_INC_SPDM_ERROR_HANDLER_H_ */
