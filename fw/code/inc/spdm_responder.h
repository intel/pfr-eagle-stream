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
 * @file spdm_responder.h
 * @brief Process the MCTP packet through spdm protocol.
 */

#ifndef EAGLESTREAM_INC_SPDM_RESPONDER_H_
#define EAGLESTREAM_INC_SPDM_RESPONDER_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "spdm.h"
#include "spdm_error_handler.h"
#include "cert_gen_options.h"

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/*
 * @brief This function verifies SPDM 1.0 GET_VERSION and creates SPDM 1.0 SUCCESSFUL_VERSION message
 * before transporting them over the MCTP layer,
 *
 * If incoming message is received, this functions might perform CPLD as responder SPDM error handling
 * if CPLD is not in normal state. Creates the response to the requested message and cache both these messages,
 * saving them for later transaction and extract important information. Part of connection establishment with devices.
 *
 * @param context  : SPDM info
 * @param mctp_ctx : MCTP info
 * @param request size : incoming message size
 *
 * @return 0 if successful, otherwise return error code
 *
 */
static alt_u32 responder_cpld_respond_version(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx, alt_u32 request_size)
{
    GET_VERSION* get_ver_ptr = (GET_VERSION*)&mctp_ctx->processed_message.buffer;

    if (context->response_state != spdm_normal_state)
    {
        context->spdm_msg_state = SPDM_ALGORITHM_FLAG;
        return spdm_responder_error_handler(context, mctp_ctx, get_ver_ptr->header.request_response_code,
                                            (alt_u8*)&mctp_ctx->processed_message.buffer, request_size);
    }

    if (request_size != sizeof(GET_VERSION))
    {
        return SPDM_ERROR_PROTOCOL;
    }

    if ((get_ver_ptr->header.spdm_version != SPDM_VERSION) || (get_ver_ptr->header.request_response_code != REQUEST_VERSION))
    {
        return SPDM_ERROR_PROTOCOL;
    }

    // Append M1 message
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)get_ver_ptr, sizeof(GET_VERSION));

    alt_u8* processed_pkt = (alt_u8*)&mctp_ctx->processed_message.buffer;
    SUCCESSFUL_VERSION* ver_ptr = (SUCCESSFUL_VERSION*) processed_pkt;

    // Construct SUCCESSFUL_VERSION
    ver_ptr->header.spdm_version = SPDM_VERSION;
    ver_ptr->header.request_response_code = RESPONSE_VERSION;
    ver_ptr->header.param_1 = 0;
    ver_ptr->header.param_2 = 0;
    ver_ptr->version_num_entry_count = 1;
    ver_ptr->version_num_entry[0].major_minor_version = 0x10;
    //ver_ptr->version_num_entry[1].major_minor_version = 0x11;

    alt_u32 spdm_response_size = sizeof(SUCCESSFUL_VERSION) - sizeof(VERSION_NUMBER)*MAX_VERSION_NUMBER_ENTRY
                                 + (ver_ptr->version_num_entry_count)*sizeof(VERSION_NUMBER);

    mctp_ctx->cached_pkt_info &= ~TAG_OWNER;
    assemble_and_send_mctp_pkt(mctp_ctx, (alt_u8*)processed_pkt, spdm_response_size);

    if (mctp_ctx->error_type)
    {
        return SPDM_RESPONSE_ERROR_COMMUNICATION_FAIL;
    }

    // Append M1 message
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)processed_pkt, spdm_response_size);

    context->spdm_msg_state = SPDM_CAPABILITIES_FLAG;
    return SPDM_EXCHANGE_SUCCESS;
}

/*
 * @brief This function verifies SPDM 1.0 GET_CAPABILITIES and creates SUCCESSFUL_CAPABILITIES to the requester
 * before transporting them over the MCTP Layer.
 *
 * Part of the connection establishment with the devices attesting CPLD. If CPLD is not in normal state,
 * handles the SPDM error handler to the requester. Upon verification of the requested message, CPLD will
 * generate and share all its capabilities with the device before proceeding further to other SPDM messages.
 * ACache and append the requested and responded messages to previously cached messages.
 * Currently, CPLD does not support CACHE_CAP and all the SPDM transaction should start from VERSION.
 *
 * @param context : SPDM info
 * @param mctp_ctx : MCTP info
 * @param request_size : Requested message size
 *
 * @return 0 if successful otherwise return error code.
 *
 */
static alt_u32 responder_cpld_respond_capabilities(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx, alt_u32 request_size)
{
    GET_CAPABILITIES* get_capabilities_ptr = (GET_CAPABILITIES*)&mctp_ctx->processed_message.buffer;

    if (context->response_state != spdm_normal_state)
    {
        context->spdm_msg_state = SPDM_ALGORITHM_FLAG;
        return spdm_responder_error_handler(context, mctp_ctx, get_capabilities_ptr->header.request_response_code,
                                            (alt_u8*)&mctp_ctx->processed_message.buffer, request_size);
    }

    if (request_size != sizeof(GET_CAPABILITIES))
    {
        return SPDM_ERROR_PROTOCOL;
    }

    if ((get_capabilities_ptr->header.spdm_version != SPDM_VERSION) || (get_capabilities_ptr->header.request_response_code != REQUEST_CAPABILITIES))
    {
        return SPDM_ERROR_PROTOCOL;
    }

    // Append M1 message
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)get_capabilities_ptr, sizeof(GET_CAPABILITIES));

    alt_u8* processed_pkt = (alt_u8*)&mctp_ctx->processed_message.buffer;
    SUCCESSFUL_CAPABILITIES* cap_ptr = (SUCCESSFUL_CAPABILITIES*) processed_pkt;

    cap_ptr->header.spdm_version = SPDM_VERSION;
    cap_ptr->header.request_response_code = RESPONSE_CAPABILITIES;
    cap_ptr->header.param_1 = 0;
    cap_ptr->header.param_2 = 0;
    cap_ptr->reserved_1 = 0;
    cap_ptr->ct_exponent = context->capability.ct_exponent;
    cap_ptr->reserved_2[0] = 0;
    cap_ptr->reserved_2[1] = 0;
    cap_ptr->flags = context->capability.flags;

    alt_u32 spdm_response_size = sizeof(SUCCESSFUL_CAPABILITIES);

    mctp_ctx->cached_pkt_info &= ~TAG_OWNER;
    assemble_and_send_mctp_pkt(mctp_ctx, (alt_u8*)processed_pkt, spdm_response_size);

    if (mctp_ctx->error_type)
    {
        return SPDM_RESPONSE_ERROR_COMMUNICATION_FAIL;
    }

    // Append M1 message
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)processed_pkt, spdm_response_size);

    context->spdm_msg_state = SPDM_ALGORITHM_FLAG;
    return SPDM_EXCHANGE_SUCCESS;
}

/*
 * @brief This function verifies SPDM 1.0 NEGOTIATE_ALGORITHM and create SUCCESSFUL_ALGORITHMS to the requester
 * before transporting them over MCTP layer.
 *
 * Part of the connection establishment with other devices. If CPLD is not in the normal state, handles the
 * error as part of the SPDM message. Upon verification of the incoming SPDM messages, CPLD will state its
 * supported cryptography algorithm. Currently CPLD supports SHA384 AND ECDSA 384 for keys generation, signature
 * generation/verification and certification. Cache these messages with previously cached messages
 * for usage in next transaction.
 *
 * @param context : SPDM info
 * @param mctp_ctx : MCTP info
 * @param request_size : Requested message size
 *
 * @return 0 if successful otherwise return error code.
 *
 */

static alt_u32 responder_cpld_respond_algorithm(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx, alt_u32 request_size)
{
    NEGOTIATE_ALGORITHMS* negotiate_algorithm_ptr = (NEGOTIATE_ALGORITHMS*)&mctp_ctx->processed_message.buffer;

    if (context->response_state != spdm_normal_state)
    {
        context->spdm_msg_state = SPDM_ALGORITHM_FLAG;
        return spdm_responder_error_handler(context, mctp_ctx, negotiate_algorithm_ptr->header.request_response_code,
                                            (alt_u8*)&mctp_ctx->processed_message.buffer, request_size);
    }

    // Check minimum and maximum size as defined with asym and hash count
    if (request_size < sizeof(NEGOTIATE_ALGORITHMS) +
                       sizeof(alt_u32)*negotiate_algorithm_ptr->ext_asym_count +
                       sizeof(alt_u32)*negotiate_algorithm_ptr->ext_hash_count)
    {
        return SPDM_ERROR_PROTOCOL;
    }

    if (request_size > sizeof(NEGOTIATE_ALGORITHMS) +
                       sizeof(alt_u32)*negotiate_algorithm_ptr->ext_asym_count +
                       sizeof(alt_u32)*negotiate_algorithm_ptr->ext_hash_count)
    {
        return SPDM_ERROR_PROTOCOL;
    }

    if ((negotiate_algorithm_ptr->header.spdm_version != SPDM_VERSION) || (negotiate_algorithm_ptr->header.request_response_code != REQUEST_ALGORITHMS))
    {
        return SPDM_ERROR_PROTOCOL;
    }

    // Get request message size
    request_size = sizeof(NEGOTIATE_ALGORITHMS) +
                   sizeof(alt_u32)*negotiate_algorithm_ptr->ext_asym_count +
                   sizeof(alt_u32)*negotiate_algorithm_ptr->ext_hash_count;

    // Append M1 message
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)negotiate_algorithm_ptr, request_size);

    alt_u8* processed_pkt = (alt_u8*)&mctp_ctx->processed_message.buffer;
    SUCCESSFUL_ALGORITHMS* algo_ptr = (SUCCESSFUL_ALGORITHMS*) processed_pkt;

    algo_ptr->header.spdm_version = SPDM_VERSION;
    algo_ptr->header.request_response_code = RESPONSE_ALGORITHMS;
    algo_ptr->header.param_1 = 0;
    algo_ptr->header.param_2 = 0;
    algo_ptr->length = sizeof(SUCCESSFUL_ALGORITHMS);
    algo_ptr->measurement_spec = SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF;
    // CPLD will use EC-384 algorithm throughout SPDM exchange
    algo_ptr->measurement_hash_algo = context->algorithm.measurement_hash_algo &
                                      MEAS_HASH_ALGO_TPM_ALG_SHA_384;
    algo_ptr->base_asym_sel = context->algorithm.base_asym_algo &
                              BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;
    algo_ptr->base_hash_sel = context->algorithm.base_hash_algo &
                              BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384;

    alt_u32 spdm_response_size = sizeof(SUCCESSFUL_ALGORITHMS);
    mctp_ctx->cached_pkt_info &= ~TAG_OWNER;
    assemble_and_send_mctp_pkt(mctp_ctx, (alt_u8*)processed_pkt, spdm_response_size);

    if (mctp_ctx->error_type)
    {
        return SPDM_RESPONSE_ERROR_COMMUNICATION_FAIL;
    }

    // Append M1 message
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)processed_pkt, spdm_response_size);

    if (context->capability.flags == SUPPORTS_MEAS_RESPONSE_WITHOUT_SIG_GEN_MSG_MASK)
    {
        // CPLD can no longer supports DIGEST, CERTIFICATE and CHAL_AUTH
        context->spdm_msg_state = SPDM_MEASUREMENT_FLAG;
    }
    else
    {
        context->spdm_msg_state = SPDM_DIGEST_FLAG;
    }
    return SPDM_EXCHANGE_SUCCESS;
}


/*
 * @brief This function verifies SPDM 1.0 GET_DIGEST and create DIGEST to the requester
 * before transporting them over MCTP layer.
 *
 * In this function, CPLD will be calculating its own certificate chain hash. Certificate chain consists of
 * cert chain header, root cert and leaf cert. Verifies the GET_DIGEST on protocol, size etc...
 *
 * @param context  : SPDM info
 * @param mctp_ctx : MCTP info
 * @param request_size : Requested message size
 *
 * @return 0 if successful otherwise return error code.
 *
 */
static alt_u32 responder_cpld_respond_digest(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx, alt_u32 request_size)
{
    GET_DIGESTS* get_digest_ptr = (GET_DIGESTS*)&mctp_ctx->processed_message.buffer;

    if (context->response_state != spdm_normal_state)
    {
        context->spdm_msg_state = SPDM_DIGEST_FLAG;
        return spdm_responder_error_handler(context, mctp_ctx, get_digest_ptr->header.request_response_code,
                                            (alt_u8*)&mctp_ctx->processed_message.buffer, request_size);
    }

    if (request_size != sizeof(GET_DIGESTS))
    {
        return SPDM_ERROR_PROTOCOL;
    }

    if ((get_digest_ptr->header.spdm_version != SPDM_VERSION) || (get_digest_ptr->header.request_response_code != REQUEST_DIGESTS))
    {
        return SPDM_ERROR_PROTOCOL;
    }

    // Append to M1
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)get_digest_ptr, request_size);

    // CPLD only have 1 slot for digest and at first index
    alt_u8 slot_mask = 1;

    alt_u8* processed_pkt = (alt_u8*)&mctp_ctx->processed_message.buffer;
    SUCCESSFUL_DIGESTS* digest_ptr = (SUCCESSFUL_DIGESTS*) processed_pkt;

    digest_ptr->header.spdm_version = SPDM_VERSION;
    digest_ptr->header.request_response_code = RESPONSE_DIGESTS;
    digest_ptr->header.param_1 = 0;
    digest_ptr->header.param_2 = slot_mask;

    alt_u32 sha_size = (context->algorithm.base_hash_algo & BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
                        ? SHA384_LENGTH : SHA256_LENGTH;

    // Cpld will only have 1 digest of its certificate
    alt_u32 spdm_response_size = sizeof(SUCCESSFUL_DIGESTS) + sha_size;

    alt_u32 digest_hash[SHA384_LENGTH/4] = {0};
    alt_u8* hash_ptr = (alt_u8*)(digest_ptr + 1);

    alt_u8_memcpy((alt_u8*)&digest_hash[0], hash_ptr, sha_size);

    CRYPTO_MODE sha_mode = (context->algorithm.base_hash_algo & BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
                            ? CRYPTO_384_MODE : CRYPTO_256_MODE;

    //alt_u32 cert_length = FULL_CERT_LENGTH;
    alt_u8* cptr = (alt_u8*)get_ufm_cpld_cert_ptr();
    CERT_CHAIN_FORMAT* cert_chain = (CERT_CHAIN_FORMAT*)cptr;

    alt_u32 cert_length = cert_chain->length;

    // Fixed size of certificate chain for now
    calculate_and_save_sha((alt_u32*)digest_hash, 0, get_ufm_cpld_cert_ptr(), cert_length, sha_mode, DISENGAGE_DMA);

    alt_u8_memcpy(hash_ptr, (alt_u8*)&digest_hash[0], sha_size);

    mctp_ctx->cached_pkt_info &= ~TAG_OWNER;
    assemble_and_send_mctp_pkt(mctp_ctx, (alt_u8*)processed_pkt, spdm_response_size);

    if (mctp_ctx->error_type)
    {
        return SPDM_RESPONSE_ERROR_COMMUNICATION_FAIL;
    }

    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)processed_pkt, spdm_response_size);

    context->spdm_msg_state = SPDM_CERTIFICATE_FLAG;

    return SPDM_EXCHANGE_SUCCESS;
}

/*
 * @brief This function verifies SPDM 1.0 GET_CERTIFICATE and create CERTIFICATE to the requester
 * before transporting them over MCTP layer.
 *
 * In this function, CPLD generates its own certificate chain during CFM0 boot up.
 * Certificate chain consists of cert chain header, root cert and leaf cert. CPLD sends the cert chain
 * to the responder with flexible length depending on the requester message. If the requested length is more
 * than what CPLD can send, CPLD will send max chunk size (done to save code space) and might have more iterations
 * of CERTIFICATE exchange between CPLD and devices.
 *
 * Verify the requested length and offset of the cert chain.
 *
 * @param context  : SPDM info
 * @param mctp_ctx : MCTP info
 * @param request_size : Requested message size
 *
 * @return 0 if successful otherwise return error code.
 *
 */
static alt_u32 responder_cpld_respond_certificate(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx, alt_u32 request_size)
{
    GET_CERTIFICATE* get_cert_ptr = (GET_CERTIFICATE*)&mctp_ctx->processed_message.buffer;

    if (context->response_state != spdm_normal_state)
    {
        context->spdm_msg_state = SPDM_CERTIFICATE_FLAG;
        return spdm_responder_error_handler(context, mctp_ctx, get_cert_ptr->header.request_response_code,
                                            (alt_u8*)&mctp_ctx->processed_message.buffer, request_size);
    }

    if (request_size != sizeof(GET_CERTIFICATE))
    {
        return SPDM_ERROR_PROTOCOL;
    }

    if ((get_cert_ptr->header.spdm_version != SPDM_VERSION) || (get_cert_ptr->header.request_response_code != REQUEST_CERTIFICATE))
    {
        return SPDM_ERROR_PROTOCOL;
    }

    alt_u8 slot_number = get_cert_ptr->header.param_1;
    // CPLD only have one slot which is slot 0

    // This checks can be removed if CPLD have more than 1 slot
    if (slot_number > 0)
    {
        return SPDM_ERROR_PROTOCOL;
    }

    alt_u16 offset = get_cert_ptr->offset;
    alt_u16 length = get_cert_ptr->length;

    //alt_u16 cpld_cert_size = FULL_CERT_LENGTH;
    //alt_u16 returned_cert_size = FULL_CERT_LENGTH;

    // Hardcode this to 200 bytes
    // CPLD will return 200 bytes of cert chunk MAX at a time
    alt_u16 returned_cert_size = 0xc8;
    alt_u8* cptr = (alt_u8*)get_ufm_cpld_cert_ptr();
    CERT_CHAIN_FORMAT* cert_chain = (CERT_CHAIN_FORMAT*)cptr;

    alt_u16 cpld_cert_size = cert_chain->length;

    // Check offset
    if (offset >= cpld_cert_size)
    {
        return SPDM_ERROR_PROTOCOL;
    }

    /*if (length > cpld_cert_size)
    {
        length = cpld_cert_size;
    }*/

    // Check length
    if (length > returned_cert_size)
    {
        length = returned_cert_size;
    }

    // As we have checked the max size for offset and length, no overflow is possible
    if ((offset + length) > cpld_cert_size)
    {
        length = cpld_cert_size - offset;

        if (length > returned_cert_size)
        {
            length = returned_cert_size;
        }
    }

    // Calculate the remainder cert chain length
    context->local_context.cached_remainder_length = cpld_cert_size - (offset + length);

    alt_u32 spdm_response_size = sizeof(SUCCESSFUL_CERTIFICATE) + length;

    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)get_cert_ptr, request_size);

    alt_u8* processed_pkt = (alt_u8*)&mctp_ctx->processed_message.buffer;
    SUCCESSFUL_CERTIFICATE* cert_ptr = (SUCCESSFUL_CERTIFICATE*) processed_pkt;

    cert_ptr->header.spdm_version = SPDM_VERSION;
    cert_ptr->header.request_response_code = RESPONSE_CERTIFICATE;
    cert_ptr->header.param_1 = slot_number;
    cert_ptr->header.param_2 = 0;
    cert_ptr->portion_length = length;
    cert_ptr->remainder_length = context->local_context.cached_remainder_length;

    alt_u8* stored_cert_chain = (alt_u8*) get_ufm_cpld_cert_ptr();
    alt_u8* dest_cert = (alt_u8*)(cert_ptr + 1);
    alt_u8_memcpy(dest_cert, (stored_cert_chain + offset), length);

    mctp_ctx->cached_pkt_info &= ~TAG_OWNER;
    assemble_and_send_mctp_pkt(mctp_ctx, (alt_u8*)processed_pkt, spdm_response_size);

    if (mctp_ctx->error_type)
    {
        return SPDM_RESPONSE_ERROR_COMMUNICATION_FAIL;
    }

    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)processed_pkt, spdm_response_size);

    context->spdm_msg_state = SPDM_CHALLENGE_FLAG;

    if (context->local_context.cached_remainder_length != 0)
    {
        context->spdm_msg_state = SPDM_CERTIFICATE_FLAG;
    }

    return SPDM_EXCHANGE_SUCCESS;
}

/*
 * @brief This function verifies SPDM 1.0 CHALLENGE and create CHALLENGE_AUTH to the requester
 * before transporting them over MCTP layer.
 *
 * CPLD verifies the CHALLENGE message from the requester here before responding with CHAL_AUTH.
 * CPLD saves all the messages previously to M1 and sign the message with layer 1 private key.
 * CPLD expects that the requester to verify the signature to ensure M1 = M2. It is expected that
 * requester is able to extract the key information from CPLD's leaf certificate provided in the past
 * message.
 *
 * @param context : SPDM info
 * @param mctp_ctx : MCTP info
 * @param request_size : Requested message size
 *
 * @return 0 if successful otherwise return error code.
 */
static alt_u32 responder_cpld_respond_challenge_auth(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx, alt_u32 request_size)
{
    CHALLENGE* get_chal_ptr = (CHALLENGE*)&mctp_ctx->processed_message.buffer;

    if (context->response_state != spdm_normal_state)
    {
        context->spdm_msg_state = SPDM_CHALLENGE_FLAG;
        return spdm_responder_error_handler(context, mctp_ctx, get_chal_ptr->header.request_response_code,
                                            (alt_u8*)&mctp_ctx->processed_message.buffer, request_size);
    }

    if (request_size != sizeof(CHALLENGE))
    {
        return SPDM_ERROR_PROTOCOL;
    }

    // check protocol
    if ((get_chal_ptr->header.spdm_version != SPDM_VERSION) || (get_chal_ptr->header.request_response_code != REQUEST_CHALLENGE))
    {
        return SPDM_ERROR_PROTOCOL;
    }

    alt_u8 requested_meas_summary_hash_type = get_chal_ptr->header.param_2;
    alt_u8 slot_num = get_chal_ptr->header.param_1;

    // CPLD has only 1 slot which is slot 0
    // Can remove this check next time if CPLD has more than 1 slot
    if (slot_num > 0)
    {
        return SPDM_ERROR_PROTOCOL;
    }

    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)get_chal_ptr, request_size);

    alt_u8 sig_size = (context->algorithm.base_asym_algo & BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
                       ? PFR_ROOT_KEY_DATA_SIZE_FOR_384 : PFR_ROOT_KEY_DATA_SIZE_FOR_256;

    CRYPTO_MODE sha_mode = (context->algorithm.base_hash_algo & BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
                            ? CRYPTO_384_MODE : CRYPTO_256_MODE;

    // Set the whole message size according to all the parameters
    alt_u32 spdm_response_size = sizeof(CHALLENGE_AUTH_RESPONDER) +
                                 (sig_size >> 1) +
                                 SPDM_NONCE_SIZE +
                                 (sig_size >> 1) +
                                 sizeof(alt_u16) +
                                 context->local_context.cpld_opaque_length +
                                 sig_size;

    alt_u8* processed_pkt = (alt_u8*)&mctp_ctx->processed_message.buffer;
    CHALLENGE_AUTH_RESPONDER* chal_auth_ptr = (CHALLENGE_AUTH_RESPONDER*) processed_pkt;

    chal_auth_ptr->header.spdm_version = SPDM_VERSION;
    chal_auth_ptr->header.request_response_code = RESPONSE_CHALLENGE_AUTH;
    chal_auth_ptr->header.param_1 = slot_num;
    chal_auth_ptr->header.param_2 = (1 << slot_num);

    alt_u8* ptr = (alt_u8*)(chal_auth_ptr + 1);
#ifdef USE_SYSTEM_MOCK
    // To test and make sure the provided data to verify signature is consistent, need to have a fixed nonce value
    for (alt_u8 i = 0; i < (sig_size >> 1); i++)
    {
        ptr[i] = 0x11;
    }
#else
    alt_u8* cptr = (alt_u8*)get_ufm_cpld_cert_ptr();
    CERT_CHAIN_FORMAT* cert_chain = (CERT_CHAIN_FORMAT*)cptr;

    alt_u32 cert_length = cert_chain->length;

    alt_u32 cert_chain_hash[PFR_ROOT_KEY_DATA_SIZE_FOR_384/8] = {0};

    calculate_and_save_sha((alt_u32*)cert_chain_hash, 0, get_ufm_cpld_cert_ptr(), cert_length, sha_mode, DISENGAGE_DMA);

    alt_u8_memcpy(ptr, (alt_u8*)&cert_chain_hash[0], sig_size/2);

#endif
    ptr += (sig_size >> 1);

#ifdef USE_SYSTEM_MOCK
    // To test and make sure the provided data to verify signature is consistent, need to have a fixed nonce value
    for (alt_u32 i = 0; i < SPDM_NONCE_SIZE; i++)
    {
        ptr[i] = 0x11;
    }
#else
    alt_u32 entropy_chal[SPDM_NONCE_SIZE >> 2] = {0};
    extract_entropy_data((alt_u32*)entropy_chal, (SPDM_NONCE_SIZE >> 2));

    alt_u8_memcpy(ptr, (alt_u8*)&entropy_chal[0], SPDM_NONCE_SIZE);
#endif

    ptr += SPDM_NONCE_SIZE;

    // Cpld only have 1 measurement hash which is her own firmware version
    if (requested_meas_summary_hash_type)
    {
#ifdef USE_SYSTEM_MOCK
        // To test and make sure the provided data to verify signature is consistent, need to have a fixed nonce value
        for (alt_u8 i = 0; i < (sig_size >> 1); i++)
        {
            ptr[i] = 0x11;
        }
#else
        // Use DMA for padding here as the data size is huge
        alt_u32* cfm1_hash = get_ufm_ptr_with_offset(UFM_CPLD_CFM1_STORED_HASH);

        alt_u8_memcpy(ptr, (alt_u8*)&cfm1_hash[0], (sig_size >> 1));
#endif
        ptr += (sig_size >> 1);
    }

    alt_u8_memcpy(ptr, (alt_u8*)&context->local_context.cpld_opaque_length, sizeof(alt_u16));
    ptr += sizeof(alt_u16);

    // Set a unique opaque data for CPLD
    alt_u16 opaque_data = 0xcafe;
    alt_u8_memcpy(ptr, (alt_u8*)&opaque_data, sizeof(alt_u16));
    ptr += context->local_context.cpld_opaque_length;

    append_large_buffer(&context->transcript.m1_m2, processed_pkt, spdm_response_size - sig_size);

    alt_u32 stored_sig[PFR_ROOT_KEY_DATA_SIZE_FOR_384/4] = {0};
    alt_u32* sig = (alt_u32*)stored_sig;
    alt_u32* ufm_attest_pubkey_1 = get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_1);
    alt_u32* ufm_attest_privkey_1 = get_ufm_ptr_with_offset(UFM_CPLD_PRIVATE_KEY_1);

    alt_u32* message = (alt_u32*)&context->transcript.m1_m2.buffer;
    // Generate the responder's signature
    generate_ecdsa_signature(sig, sig + (sig_size >> 3), (const alt_u32*)message,
                             context->transcript.m1_m2.buffer_size, ufm_attest_pubkey_1, ufm_attest_privkey_1, sha_mode);

    alt_u8_memcpy(ptr, (alt_u8*)&stored_sig[0], sig_size);

    mctp_ctx->cached_pkt_info &= ~TAG_OWNER;
    assemble_and_send_mctp_pkt(mctp_ctx, (alt_u8*)processed_pkt, spdm_response_size);

    if (mctp_ctx->error_type)
    {
        return SPDM_RESPONSE_ERROR_COMMUNICATION_FAIL;
    }

    context->spdm_msg_state = SPDM_MEASUREMENT_FLAG;

    return SPDM_EXCHANGE_SUCCESS;
}

/*
 * @brief This function verifies SPDM 1.0 GET_MEASUREMENT and create MEASUREMENT to the requester
 * before transporting them over MCTP layer.
 *
 * CPLD verifies the GET_MEASUREMENT message from the requester here before responding with MEASUREMENTS.
 * CPLD only have 1 measurement which is active firmware hash. Therefore, CPLD shall only respond with a
 * single MEASUREMENT message. Requester can initially enquire how many measurements CPLD will be responding.
 * CPLD also supports signing the messages plus all the measurement(S). Requester can verify this signature
 * to ensure L1 = L2. These keys are expected to be obtained from CPLD's leaf certificate.
 *
 * @param context : SPDM info
 * @param mctp_ctx : MCTP info
 * @param request_size : Requested message size
 *
 * @return 0 if successful otherwise return error code.
 */
static alt_u32 responder_cpld_respond_measurement(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx, alt_u32 request_size)
{
    GET_MEASUREMENT* get_meas_ptr = (GET_MEASUREMENT*)&mctp_ctx->processed_message.buffer;

    if (context->response_state != spdm_normal_state)
    {
        context->spdm_msg_state = SPDM_MEASUREMENT_FLAG;
        return spdm_responder_error_handler(context, mctp_ctx, get_meas_ptr->header.request_response_code,
                                            (alt_u8*)&mctp_ctx->processed_message.buffer, request_size);
    }

    alt_u8 request_attribute = get_meas_ptr->header.param_1;
    alt_u8 meas_operation = get_meas_ptr->header.param_2;

    // If CPLD cannot support signature generation but is still requested, error out
    if ((context->capability.flags == SUPPORTS_MEAS_RESPONSE_WITHOUT_SIG_GEN_MSG_MASK) &&
        (request_attribute == SPDM_GET_MEASUREMENT_REQUEST_GENERATE_SIG))
    {
        return SPDM_ERROR_PROTOCOL;
    }

    // Check if signature is required for measurements
    if (request_attribute == SPDM_GET_MEASUREMENT_REQUEST_GENERATE_SIG)
    {
        if (request_size != sizeof(GET_MEASUREMENT))
        {
            return SPDM_ERROR_PROTOCOL;
        }
    }
    else
    {
        if (request_size != sizeof(SPDM_HEADER))
        {
            return SPDM_ERROR_PROTOCOL;
        }
    }

    if ((get_meas_ptr->header.spdm_version != SPDM_VERSION) || (get_meas_ptr->header.request_response_code != REQUEST_MEASUREMENTS))
    {
        return SPDM_ERROR_PROTOCOL;
    }

    append_large_buffer(&context->transcript.l1_l2, (alt_u8*)get_meas_ptr, request_size);

    alt_u8* processed_pkt = (alt_u8*)&mctp_ctx->processed_message.buffer;
    MEASUREMENT* meas_ptr = (MEASUREMENT*) processed_pkt;
    alt_u8* ptr = processed_pkt;
    alt_u8* sig = processed_pkt;

    CRYPTO_MODE sha_mode = (context->algorithm.base_hash_algo & BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
                            ? CRYPTO_384_MODE : CRYPTO_256_MODE;

    alt_u32 sig_size = (context->algorithm.base_asym_algo & BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
                       ? PFR_ROOT_KEY_DATA_SIZE_FOR_384 : PFR_ROOT_KEY_DATA_SIZE_FOR_256;

    alt_u32 sha_size = (context->algorithm.base_hash_algo & BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
                            ? SHA384_LENGTH : SHA256_LENGTH;

    // Signature is not required
    if ((request_attribute & SPDM_GET_MEASUREMENT_REQUEST_GENERATE_SIG) == 0)
    {
        sig_size = 0;
    }

    alt_u32 spdm_response_size = sizeof(SPDM_HEADER) + sizeof(alt_u32);

    alt_u32* ufm_attest_pubkey_1 = get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_1);
    alt_u32* ufm_attest_privkey_1 = get_ufm_ptr_with_offset(UFM_CPLD_PRIVATE_KEY_1);

    meas_ptr->header.spdm_version = SPDM_VERSION;
    meas_ptr->header.request_response_code = RESPONSE_MEASUREMENTS;
    meas_ptr->header.param_2 = 0;

    if (!meas_operation)
    {
        // CPLD only has 1 measurement which is her own firmware hash.
        meas_ptr->header.param_1 = 1;
        meas_ptr->num_of_blocks = 0;

        alt_u24_write(meas_ptr->meas_record_length, 0);

        // For case where CPLD cannot generate signature, this bit has been checked
        if (request_attribute & SPDM_GET_MEASUREMENT_REQUEST_GENERATE_SIG)
        {
            ptr += spdm_response_size;
#ifdef USE_SYSTEM_MOCK
            for (alt_u8 i = 0; i < SPDM_NONCE_SIZE; i++)
            {
                ptr[i] = 0x11;
            }
#else
            alt_u32 meas_entropy[SPDM_NONCE_SIZE/4] = {0};
            extract_entropy_data((alt_u32*)meas_entropy, (SPDM_NONCE_SIZE >> 2));

            alt_u8_memcpy(ptr, (alt_u8*)&meas_entropy[0], SPDM_NONCE_SIZE);
#endif
            ptr += SPDM_NONCE_SIZE;

            alt_u8_memcpy(ptr, (alt_u8*)&context->local_context.cpld_opaque_length, sizeof(alt_u16));

            ptr += sizeof(alt_u16);
            alt_u16 opaque_data = 0xcafe;
            alt_u8_memcpy(ptr, (alt_u8*)&opaque_data, sizeof(alt_u16));

            spdm_response_size = spdm_response_size +
                                 SPDM_NONCE_SIZE +
                                 sizeof(alt_u16) +
                                 context->local_context.cpld_opaque_length;

            append_large_buffer(&context->transcript.l1_l2, (alt_u8*)meas_ptr, spdm_response_size);

            alt_u32 meas_calc_sig[PFR_ROOT_KEY_DATA_SIZE_FOR_384/4] = {0};
            alt_u32* meas_calc_sig_ptr = meas_calc_sig;
            sig += spdm_response_size;

            alt_u32* message = (alt_u32*)&context->transcript.l1_l2.buffer;
            // Generate the responder's signature
            generate_ecdsa_signature(meas_calc_sig_ptr, meas_calc_sig_ptr + (sig_size >> 3), (const alt_u32*)message,
                                     context->transcript.l1_l2.buffer_size, ufm_attest_pubkey_1, ufm_attest_privkey_1, sha_mode);
            alt_u8_memcpy(sig, (alt_u8*)&meas_calc_sig[0], sig_size);
        }
        else
        {
            ptr += spdm_response_size;
            ptr[0] = 0;
            ptr[1] = 0;

            /*spdm_response_size = spdm_response_size +
                                 sizeof(alt_u16) +
                                 context->local_context.cpld_opaque_length;*/

            spdm_response_size = spdm_response_size +
                                 sizeof(alt_u16);

            append_large_buffer(&context->transcript.l1_l2, (alt_u8*)meas_ptr, spdm_response_size);
        }

        mctp_ctx->cached_pkt_info &= ~TAG_OWNER;
        assemble_and_send_mctp_pkt(mctp_ctx, (alt_u8*)processed_pkt, spdm_response_size + sig_size);

        if (mctp_ctx->error_type)
        {
            return SPDM_RESPONSE_ERROR_COMMUNICATION_FAIL;
        }

        return SPDM_EXCHANGE_SUCCESS;
    }
    else if (meas_operation == SPDM_GET_ALL_MEASUREMENT || meas_operation == 1)
    {
        // CPLD only has 1 measurement which is her own firmware hash.
        meas_ptr->num_of_blocks = 1;
        meas_ptr->header.param_1 = 0;
        alt_u32 meas_record_size = sizeof(SPDM_MEASUREMENT_BLOCK_DMTF_HEADER) + sha_size;
        alt_u24_write(meas_ptr->meas_record_length, meas_record_size);

        ptr += spdm_response_size;
        SPDM_MEASUREMENT_BLOCK_DMTF_HEADER meas_block_container;
        SPDM_MEASUREMENT_BLOCK_DMTF_HEADER* meas_block_ptr = (SPDM_MEASUREMENT_BLOCK_DMTF_HEADER*) &meas_block_container;

        // Measurement block format
        meas_block_ptr->meas_block.index = 1;
        meas_block_ptr->meas_block.measurement_spec = SPDM_MEASUREMENT_SPECIFICATION_DMTF;
        meas_block_ptr->meas_block.measurement_size = (alt_u16)(sizeof(SPDM_MEASUREMENT_DMTF_HEADER) + sha_size);

        // DMTF specification format
        meas_block_ptr->meas_dmtf.dmtf_spec_meas_value_type = SPDM_DMTF_SPEC_MEAS_VAL_TYPE_MUTABLE_FIRMWARE;
        meas_block_ptr->meas_dmtf.dmtf_spec_meas_value_size = (alt_u16)(sha_size);

        alt_u8_memcpy(ptr, (alt_u8*)meas_block_ptr, sizeof(SPDM_MEASUREMENT_BLOCK_DMTF_HEADER));
        ptr += sizeof(SPDM_MEASUREMENT_BLOCK_DMTF_HEADER);

        alt_u32* cfm1_hash = get_ufm_ptr_with_offset(UFM_CPLD_CFM1_STORED_HASH);
        alt_u8_memcpy(ptr, (alt_u8*)cfm1_hash, sha_size);
        // Use DMA for padding here as the data size is huge

        ptr += meas_block_ptr->meas_dmtf.dmtf_spec_meas_value_size;

        // For case where CPLD cannot generate signature, this bit has been checked
        if (request_attribute & SPDM_GET_MEASUREMENT_REQUEST_GENERATE_SIG)
        {
#ifdef USE_SYSTEM_MOCK
            for (alt_u8 i = 0; i < SPDM_NONCE_SIZE; i++)
            {
                ptr[i] = 0x11;
            }
#else
            alt_u32 meas_entropy[SPDM_NONCE_SIZE >> 2] = {0};
            extract_entropy_data((alt_u32*)meas_entropy, (SPDM_NONCE_SIZE >> 2));
            alt_u8_memcpy(ptr, (alt_u8*)&meas_entropy[0], SPDM_NONCE_SIZE);
#endif
            ptr += SPDM_NONCE_SIZE;
        }

        alt_u8_memcpy(ptr, (alt_u8*)&context->local_context.cpld_opaque_length, sizeof(alt_u16));

        ptr += sizeof(alt_u16);
        alt_u16 opaque_data = 0xcafe;
        alt_u8_memcpy(ptr, (alt_u8*)&opaque_data, sizeof(alt_u16));

        spdm_response_size = spdm_response_size +
                             sizeof(SPDM_MEASUREMENT_BLOCK_DMTF_HEADER) +
                             meas_block_ptr->meas_dmtf.dmtf_spec_meas_value_size +
                             sizeof(alt_u16) +
                             context->local_context.cpld_opaque_length;

        if (request_attribute & SPDM_GET_MEASUREMENT_REQUEST_GENERATE_SIG)
        {
            spdm_response_size = spdm_response_size	+ SPDM_NONCE_SIZE;
            sig += spdm_response_size;

            append_large_buffer(&context->transcript.l1_l2, (alt_u8*)meas_ptr, spdm_response_size);

            alt_u32* message = (alt_u32*)&context->transcript.l1_l2.buffer;
            alt_u32 res_sig[PFR_ROOT_KEY_DATA_SIZE_FOR_384/4] = {0};
            alt_u32* res_sig_ptr = res_sig;
            // Generate the responder's signature
            generate_ecdsa_signature(res_sig_ptr, res_sig_ptr + (sig_size >> 3), (const alt_u32*)message,
                                     context->transcript.l1_l2.buffer_size, ufm_attest_pubkey_1, ufm_attest_privkey_1, sha_mode);
            alt_u8_memcpy(sig, (alt_u8*)&res_sig[0], sig_size);
        }
        else
        {
            append_large_buffer(&context->transcript.l1_l2, (alt_u8*)meas_ptr, spdm_response_size);
        }
    }
    else
    {
        // CPLD only has 1 measurement which is her own firmware version
        return SPDM_ERROR_PROTOCOL;
    }

    mctp_ctx->cached_pkt_info &= ~TAG_OWNER;
    assemble_and_send_mctp_pkt(mctp_ctx, (alt_u8*)processed_pkt, spdm_response_size + sig_size);

    if (mctp_ctx->error_type)
    {
        return SPDM_RESPONSE_ERROR_COMMUNICATION_FAIL;
    }

    context->spdm_msg_state = SPDM_ATTEST_FINISH_FLAG;

    return SPDM_EXCHANGE_SUCCESS;
}

/*
 * @brief CPLD's SPDM Response message handler.
 *
 * This function tracks the state of the SPDM message CPLD is having with the requester.
 * This function also handle some SPDM response error protocol
 *
 *
 * @param context : SPDM info
 * @param mctp_ctx : MCTP info
 * @param request_size : Incoming message size
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 respond_message_handler(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx, alt_u32 request_size)
{
    alt_u32 status = 0;
    if (context->spdm_msg_state & SPDM_VERSION_FLAG)
    {
        status = responder_cpld_respond_version(context, mctp_ctx, request_size);
    }
    else if (context->spdm_msg_state & SPDM_CAPABILITIES_FLAG)
    {
        status = responder_cpld_respond_capabilities(context, mctp_ctx, request_size);
    }
    else if (context->spdm_msg_state & SPDM_ALGORITHM_FLAG)
    {
        status = responder_cpld_respond_algorithm(context, mctp_ctx, request_size);
    }
    else if (context->spdm_msg_state & SPDM_DIGEST_FLAG)
    {
        status = responder_cpld_respond_digest(context, mctp_ctx, request_size);
    }
    else if (context->spdm_msg_state & SPDM_CERTIFICATE_FLAG)
    {
        status = responder_cpld_respond_certificate(context, mctp_ctx, request_size);
    }
    else if (context->spdm_msg_state & SPDM_CHALLENGE_FLAG)
    {
        status = responder_cpld_respond_challenge_auth(context, mctp_ctx, request_size);
    }
    else if (context->spdm_msg_state & SPDM_MEASUREMENT_FLAG)
    {
        status = responder_cpld_respond_measurement(context, mctp_ctx, request_size);
    }

    return status;
}

#endif

#endif /* EAGLESTREAM_INC_SPDM_RESPONDER_H_ */
