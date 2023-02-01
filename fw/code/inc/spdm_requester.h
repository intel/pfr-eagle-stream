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
 * @file spdm_requester.h
 * @brief Process the MCTP packet through spdm protocol.
 */

#ifndef EAGLESTREAM_INC_SPDM_REQUESTER_H_
#define EAGLESTREAM_INC_SPDM_REQUESTER_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "spdm.h"
#include "spdm_error_handler.h"
#include "cert_flow.h"

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/*
 * @brief Request supported SPDM version from the responder before proceeding.
 *
 * This function construct and send GET_VERSION to responder. It also verifies the responded VERSION
 * before caching the data into M1 message. Part of connection establishment with devices.
 *
 * @param context : SPDM info
 * @param mctp_ctx: MCTP info
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 requester_cpld_get_version(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx)
{
    GET_VERSION* get_ver_ptr = (GET_VERSION*)&mctp_ctx->processed_message.buffer;

    // Construct GET_VERSION
    get_ver_ptr->header.spdm_version = SPDM_VERSION;
    get_ver_ptr->header.request_response_code = REQUEST_VERSION;
    get_ver_ptr->header.param_1 = 0;
    get_ver_ptr->header.param_2 = 0;

    // Append to M1
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)get_ver_ptr, sizeof(GET_VERSION));
    context->previous_cached_size = sizeof(GET_VERSION);

    alt_u8* processed_pkt = (alt_u8*)&mctp_ctx->processed_message.buffer;

    // Send and wait for responder VERSION
    alt_u32 spdm_response_size = send_and_receive_message(mctp_ctx, (alt_u8*)get_ver_ptr, sizeof(GET_VERSION),
                                                  context->timeout);

    // Handles message not received or timeout
    if (!spdm_response_size)
    {
        return spdm_requester_error_handler(context, mctp_ctx, RESPONSE_VERSION, sizeof(SUCCESSFUL_VERSION) + 2);
    }

    SUCCESSFUL_VERSION* ver_ptr = (SUCCESSFUL_VERSION*) processed_pkt;

    // Handles SPDM error response code
    if (ver_ptr->header.request_response_code == RESPONSE_ERROR)
    {
        return spdm_requester_error_handler(context, mctp_ctx, RESPONSE_VERSION, sizeof(SUCCESSFUL_VERSION) + 2);
    }

    ver_ptr = (SUCCESSFUL_VERSION*) processed_pkt;

    // Check responded message size
    if (spdm_response_size < sizeof(SUCCESSFUL_VERSION) -
                             sizeof(VERSION_NUMBER)*MAX_VERSION_NUMBER_ENTRY +
                             (ver_ptr->version_num_entry_count)*sizeof(VERSION_NUMBER))
    {
        return SPDM_RESPONSE_ERROR;
    }

    spdm_response_size = sizeof(SUCCESSFUL_VERSION) - sizeof(VERSION_NUMBER)*MAX_VERSION_NUMBER_ENTRY
                         + (ver_ptr->version_num_entry_count)*sizeof(VERSION_NUMBER);

    if ((ver_ptr->header.spdm_version != SPDM_VERSION) || (ver_ptr->header.request_response_code != RESPONSE_VERSION))
    {
        return SPDM_RESPONSE_ERROR;
    }

    // Append to M1
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)ver_ptr, spdm_response_size);

    // Set next SPDM state
    context->spdm_msg_state = SPDM_CAPABILITIES_FLAG;
    return SPDM_RESPONSE_SUCCESS;
}

/*
 * @brief Request supported SPDM capability from the responder before proceeding.
 *
 * This function construct and send GET_CAPABILITY to responder. It also verifies the responded CAPABILITY
 * before caching the data into M1 message. Saves the capabilities of responder for digest, certificate
 * and measurements. Part of connection establishment with devices.
 *
 * NOTE: CPLD do not support CACHE_CAP and MEAS_FRESH_CAP
 *
 * @param context : SPDM info
 * @param mctp_ctx: MCTP info
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 requester_cpld_get_capability(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx)
{
    GET_CAPABILITIES* get_capabilities_ptr = (GET_CAPABILITIES*)&mctp_ctx->processed_message.buffer;

    // Construct GET_CAPABILITIES
    get_capabilities_ptr->header.spdm_version = SPDM_VERSION;
    get_capabilities_ptr->header.request_response_code = REQUEST_CAPABILITIES;
    get_capabilities_ptr->header.param_1 = 0;
    get_capabilities_ptr->header.param_2 = 0;

    // Append to M1
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)get_capabilities_ptr, sizeof(GET_CAPABILITIES));
    context->previous_cached_size = sizeof(GET_CAPABILITIES);

    alt_u8* processed_pkt = (alt_u8*)&mctp_ctx->processed_message.buffer;

    // Send and wait for responder CAPABILITIES
    alt_u32 spdm_response_size = send_and_receive_message(mctp_ctx, (alt_u8*)get_capabilities_ptr,
                                                          sizeof(GET_CAPABILITIES), context->timeout);
    // Handles message not received or timeout
    if (!spdm_response_size)
    {
        return spdm_requester_error_handler(context, mctp_ctx, RESPONSE_CAPABILITIES, sizeof(SUCCESSFUL_CAPABILITIES));
    }

    SUCCESSFUL_CAPABILITIES* capabilities_ptr = (SUCCESSFUL_CAPABILITIES*) processed_pkt;

    // Handles SPDM error response code
    if (capabilities_ptr->header.request_response_code == RESPONSE_ERROR)
    {
        return spdm_requester_error_handler(context, mctp_ctx, RESPONSE_CAPABILITIES, sizeof(SUCCESSFUL_CAPABILITIES));
    }

    capabilities_ptr = (SUCCESSFUL_CAPABILITIES*) processed_pkt;

    // Check responded message size
    if (spdm_response_size > sizeof(SUCCESSFUL_CAPABILITIES))
    {
        return SPDM_RESPONSE_ERROR;
    }
    if ((capabilities_ptr->header.spdm_version != SPDM_VERSION) || (capabilities_ptr->header.request_response_code != RESPONSE_CAPABILITIES))
    {
        return SPDM_RESPONSE_ERROR;
    }

    // Save responder's capabilities
    context->capability.ct_exponent = capabilities_ptr->ct_exponent;
    context->capability.flags = capabilities_ptr->flags;

    spdm_response_size = sizeof(SUCCESSFUL_CAPABILITIES);

    // Append to M1
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)capabilities_ptr, spdm_response_size);

    // Set next SPDM state
    context->spdm_msg_state = SPDM_ALGORITHM_FLAG;
    return SPDM_RESPONSE_SUCCESS;
}


/*
 * @brief Request supported crypto algorithm from the responder before proceeding.
 *
 * This function construct and send NEGOTIATE_ALGORITHM to responder. It also verifies the responded ALGORITHM
 * before caching the data into M1 message. Saves the supported algorithm (ec256 or ec384 ONLY). As a requester, CPLD
 * expect only a type of algorithm to be chosen by responder, for example, only EC384 bit mask is set and not
 * both EC256 and EC384. CPLD only support EC384 OR EC256 and none others enumerated algorithm.
 * Part of connection establishment with devices.
 *
 * @param context : SPDM info
 * @param mctp_ctx: MCTP info
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 requester_cpld_negotiate_algorithm(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx)
{
    NEGOTIATE_ALGORITHMS* negotiate_algorithm_ptr = (NEGOTIATE_ALGORITHMS*)&mctp_ctx->processed_message.buffer;
    reset_buffer((alt_u8*)negotiate_algorithm_ptr, sizeof(NEGOTIATE_ALGORITHMS));

    // Construct NEGOTIATE_ALGORITHM
    negotiate_algorithm_ptr->header.spdm_version = SPDM_VERSION;
    negotiate_algorithm_ptr->header.request_response_code = REQUEST_ALGORITHMS;
    negotiate_algorithm_ptr->header.param_1 = 0;
    negotiate_algorithm_ptr->header.param_2 = 0;
    negotiate_algorithm_ptr->length = sizeof(NEGOTIATE_ALGORITHMS);
    negotiate_algorithm_ptr->measurement_spec = SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF;
    negotiate_algorithm_ptr->base_asym_algo = context->algorithm.base_asym_algo;
    negotiate_algorithm_ptr->base_hash_algo = context->algorithm.base_hash_algo;

    // No extended asym and hash count, hence the last two parameters can be empty
    negotiate_algorithm_ptr->ext_asym_count = 0;
    negotiate_algorithm_ptr->ext_hash_count = 0;

    // Append to M1
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)negotiate_algorithm_ptr, sizeof(NEGOTIATE_ALGORITHMS));
    context->previous_cached_size = sizeof(NEGOTIATE_ALGORITHMS);

    alt_u8* processed_pkt = (alt_u8*)&mctp_ctx->processed_message.buffer;

    // Send and wait for responder ALGORITHM
    alt_u32 spdm_response_size = send_and_receive_message(mctp_ctx, (alt_u8*)negotiate_algorithm_ptr,
                                                   sizeof(NEGOTIATE_ALGORITHMS), context->timeout);
    // Handles message not received or timeout
    if (!spdm_response_size)
    {
        return spdm_requester_error_handler(context, mctp_ctx, RESPONSE_ALGORITHMS,
                                            sizeof(SUCCESSFUL_ALGORITHMS) + sizeof(SPDM_EXTENDED_ALGORITHM));
    }

    SUCCESSFUL_ALGORITHMS* algo_ptr = (SUCCESSFUL_ALGORITHMS*) processed_pkt;
    // Handles SPDM error response code
    if (algo_ptr->header.request_response_code == RESPONSE_ERROR)
    {
        return spdm_requester_error_handler(context, mctp_ctx, RESPONSE_ALGORITHMS,
                                            sizeof(SUCCESSFUL_ALGORITHMS) + sizeof(SPDM_EXTENDED_ALGORITHM));
    }

    algo_ptr = (SUCCESSFUL_ALGORITHMS*) processed_pkt;
    if ((algo_ptr->header.spdm_version != SPDM_VERSION) || (algo_ptr->header.request_response_code != RESPONSE_ALGORITHMS))
    {
        return SPDM_RESPONSE_ERROR;
    }

    // Check responded message size
    if (spdm_response_size < sizeof(SUCCESSFUL_ALGORITHMS) +
                             sizeof(alt_u32)*algo_ptr->ext_asym_sel_count +
                             sizeof(alt_u32)*algo_ptr->ext_hash_sel_count)
    {
        return SPDM_RESPONSE_ERROR;
    }

    // These checks covered:
    // 1) Check meas hash algo, base asym sel and base hash sel enumeration algorithm
    // 2) Check either only EC384 or EC256 is selected
    // 3) Check if both EC384 and EC256 enumeration are selected
    if (algo_ptr->measurement_hash_algo != MEAS_HASH_ALGO_TPM_ALG_SHA_384)
    {
        if (algo_ptr->measurement_hash_algo != MEAS_HASH_ALGO_TPM_ALG_SHA_256)
        {
            return SPDM_RESPONSE_ERROR;
        }
    }

    if (algo_ptr->base_asym_sel != BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
    {
        if (algo_ptr->base_asym_sel != BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256)
        {
            return SPDM_RESPONSE_ERROR;
        }
    }

    if (algo_ptr->base_hash_sel != BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
    {
        if (algo_ptr->base_hash_sel != BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256)
        {
            return SPDM_RESPONSE_ERROR;
        }
    }

    // Save the SPDM-enumerated algorithm if they are accepted
    context->connection_info.algorithm.measurement_hash_algo = algo_ptr->measurement_hash_algo;
    context->connection_info.algorithm.base_asym_algo = algo_ptr->base_asym_sel;
    context->connection_info.algorithm.base_hash_algo = algo_ptr->base_hash_sel;

    spdm_response_size = sizeof(SUCCESSFUL_ALGORITHMS) +
                         sizeof(alt_u32)*algo_ptr->ext_asym_sel_count +
                         sizeof(alt_u32)*algo_ptr->ext_hash_sel_count;

    // Append to M1
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)algo_ptr, spdm_response_size);

    // Set next SPDM state
    context->spdm_msg_state = SPDM_DIGEST_FLAG;
    return SPDM_RESPONSE_SUCCESS;
}

/*
 * @brief Verify certificate chain digest and store received cert chain digest.
 *
 * This function verifies the digest of the cert chain by calculating the hash against stored hash obtained
 * from previous message. If no digest is stored, CPLD proceeds to storing the cert chain digest into memory.
 * As stated in the specification of SPDM, it is acceptable to proceed to CERTIFICATE if digest is mismatched.
 *
 * @param context : SPDM info
 * @param digest : Cert chain hash
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 requester_cpld_verify_digest(SPDM_CONTEXT* context, alt_u32* digest)
{
    if (context->local_context.cert_flag)
    {
        // Cert chain digest has been stored
        // Verify the cert digest
        alt_u32* cert_chain_ptr = (alt_u32*)&context->local_context.stored_cert_chain[0];
        CRYPTO_MODE sha_mode = (context->connection_info.algorithm.base_hash_algo & BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
                            ? CRYPTO_384_MODE : CRYPTO_256_MODE;

        return verify_sha(digest, 0, cert_chain_ptr, context->local_context.stored_cert_chain_size, sha_mode, DISENGAGE_DMA);
    }
    else
    {
        // Cert chain digest has not been stored
        // Store them in memory and proceed to next message
        context->local_context.cert_flag = 1;
        alt_u32* stored_digest_buf_ptr = (alt_u32*)&context->local_context.stored_digest_buf[0];
        alt_u32 sha_size = (context->connection_info.algorithm.base_hash_algo & BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
                            ? SHA384_LENGTH : SHA256_LENGTH;

        alt_u32_memcpy(stored_digest_buf_ptr, digest, sha_size);
    }
    return 1;
}

/*
 * @brief Request cert chain digest from the responder and verify them before proceeding.
 *
 * This function construct and send GET_DIGEST to responder. It also verifies the responded DIGEST
 * before caching the data into M1 message. CPLD also saves the cert chain digest for use in next
 * message. As specified in the DMTF specs, CPLD can proceed if the digest is mismatched at this stage.
 * CPLD only support EC384 OR EC256 and none others enumerated algorithm.
 *
 * @param context : SPDM info
 * @param mctp_ctx: MCTP info
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 requester_cpld_get_digest(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx)
{
    GET_DIGESTS* get_digest_ptr = (GET_DIGESTS*)&mctp_ctx->processed_message.buffer;
    reset_buffer((alt_u8*)get_digest_ptr, sizeof(GET_DIGESTS));

    // Check if responder supports DIGEST, else exit
    if ((context->capability.flags & SUPPORTS_DIGEST_CERT_RESPONSE_MSG_MASK) == 0)
    {
        // Set next SPDM state
        context->spdm_msg_state = SPDM_CERTIFICATE_FLAG;
        return SPDM_RESPONSE_SUCCESS;
    }

    // Construct GET_DIGEST
    context->error_state = SPDM_STATUS_NO_CAPABILITIES;
    get_digest_ptr->header.spdm_version = SPDM_VERSION;
    get_digest_ptr->header.request_response_code = REQUEST_DIGESTS;
    get_digest_ptr->header.param_1 = 0;
    get_digest_ptr->header.param_2 = 0;

    // Append to M1
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)get_digest_ptr, sizeof(GET_DIGESTS));
    context->previous_cached_size = sizeof(GET_DIGESTS);

    alt_u8* processed_pkt = (alt_u8*)&mctp_ctx->processed_message.buffer;

    // Send and wait for responder DIGEST
    alt_u32 spdm_response_size = send_and_receive_message(mctp_ctx, (alt_u8*)get_digest_ptr,
                                                   sizeof(GET_DIGESTS), context->timeout);

    // Handles message not received or timeout
    if (!spdm_response_size)
    {
        return spdm_requester_error_handler(context, mctp_ctx, RESPONSE_DIGESTS,
                                            sizeof(SUCCESSFUL_DIGESTS) + MAX_HASH_SIZE*MAX_SLOT_COUNT);
    }

    SUCCESSFUL_DIGESTS* digest_ptr = (SUCCESSFUL_DIGESTS*) processed_pkt;
    // Handles SPDM error response code
    if (digest_ptr->header.request_response_code == RESPONSE_ERROR)
    {
        return spdm_requester_error_handler(context, mctp_ctx, RESPONSE_DIGESTS,
                                            sizeof(SUCCESSFUL_DIGESTS) + MAX_HASH_SIZE*MAX_SLOT_COUNT);
    }

    if ((digest_ptr->header.spdm_version != SPDM_VERSION) || (digest_ptr->header.request_response_code != RESPONSE_DIGESTS))
    {
        return SPDM_RESPONSE_ERROR;
    }
    // Check responded message size
    // Already check slot count hereas well, so to save code space no longer need to check further
    if (spdm_response_size > (sizeof(SUCCESSFUL_DIGESTS) + MAX_HASH_SIZE*MAX_SLOT_COUNT))
    {
        return SPDM_RESPONSE_ERROR;
    }

    // Check sha algo
    alt_u32 digest_size = SHA384_LENGTH;
    if (context->connection_info.algorithm.base_hash_algo == BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256)
    {
        digest_size = SHA256_LENGTH;
    }

    // Digest count should not be null at this point
    alt_u32 digest_count = (spdm_response_size - sizeof(SUCCESSFUL_DIGESTS))/digest_size;
    if (!digest_count)
    {
        return SPDM_RESPONSE_ERROR;
    }

    alt_u32 digest_hash[SHA384_LENGTH/4] = {0};
    alt_u8* hash_ptr = (alt_u8*)(digest_ptr + 1);

    alt_u8_memcpy((alt_u8*)&digest_hash[0], hash_ptr, digest_size);

    alt_u32* digest_hash_ptr = digest_hash;
    // Verify the digest or store them
    if (!requester_cpld_verify_digest(context, digest_hash_ptr))
    {
        return SPDM_RESPONSE_ERROR;
    }

    // Append to M1
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)digest_ptr, spdm_response_size);

    // Set next SPDM state
    context->spdm_msg_state = SPDM_CERTIFICATE_FLAG;
    return SPDM_RESPONSE_SUCCESS;
}


/*
 * @brief Verify content of x509 certificate chain and their digest.
 *
 * Certificate chain may consist of root certificate + intermediate certificate + leaf certificate
 * This function verifies the digest of the cert chain by calculating the hash against stored hash obtained
 * from previous message. Then it proceeds to verify the content of the certificate such as checking the
 * date validity, the signature and the structure of the certificate. If certificate chain contains more
 * than one x509 certificate, CPLD will verify all of them. Leaf cert keys info are also extracted from
 * leaf certificate.
 *
 * Cert chain is received in DER format as specified in DMTF specs.
 *
 * @param context : SPDM info
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 requester_cpld_verify_cert_chain(SPDM_CONTEXT* context)
{
    alt_u32* stored_digest_buf_ptr = (alt_u32*)&context->local_context.stored_digest_buf[0];
    // Verify cert chain digest
    if (!requester_cpld_verify_digest(context, stored_digest_buf_ptr))
    {
        return SPDM_RESPONSE_ERROR;
    }

    // Traverse through the cert chain header
    alt_u8* stored_cert_chain_ptr = (alt_u8*)&context->local_context.stored_cert_chain[0];
    CERT_CHAIN_FORMAT* cert_chain = (CERT_CHAIN_FORMAT*) stored_cert_chain_ptr;

    alt_u16 cert_chain_length = cert_chain->length;
    alt_u32 sha_size = (context->connection_info.algorithm.base_hash_algo & BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
                        ? SHA384_LENGTH : SHA256_LENGTH;
    cert_chain_length -= (alt_u16)(sha_size + sizeof(alt_u32));

    alt_u8* responder_cert = stored_cert_chain_ptr + sha_size + sizeof(alt_u32);
    alt_u32* leaf_cert_key = (alt_u32*)&context->local_context.stored_chal_meas_signing_key[0];

    // Verify content of x509 certificate chain
    // Extract leaf cert key info and cache them for signature verification purposes.
    if (!pfr_verify_der_cert_chain(responder_cert, (alt_u32)cert_chain_length, leaf_cert_key, leaf_cert_key + (sha_size/4)))
    {
        return SPDM_RESPONSE_ERROR;
    }

    return SPDM_RESPONSE_SUCCESS;
}

/*
 * @brief Request x509 certificate chain from the responder and verify them before proceeding.
 *
 * This function construct and send GET_CERTIFICATE to responder. It also verifies the responded CERTIFICATE
 * before caching the data into M1 message. CPLD will request certificate chain in smaller chunk size
 * to save coding space. By doing that, CPLD will repeatedly send GET_CERTIFICATE to the responder
 * and repeatedly verify all those messages (verify the GET_CERTIFICATE header format etc...)
 *
 * All the received cert chunks will be appended and stored in M1 and also their content verified.
 * Digest and x509 cert content are verified before pub keys info in the leaf cert are extracted and stored.
 * Cache all these message chunks into M1 including the each message's header.
 *
 * CPLD only support EC384 OR EC256 and none others enumerated algorithm.
 *
 * @param context : SPDM info
 * @param mctp_ctx: MCTP info
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 requester_cpld_get_certificate(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx)
{
    GET_CERTIFICATE get_certificate;
    GET_CERTIFICATE* get_certificate_ptr = &get_certificate;
    reset_buffer((alt_u8*)get_certificate_ptr, sizeof(GET_CERTIFICATE));
    context->previous_cached_size = 0;

    alt_u8* processed_pkt = (alt_u8*)&mctp_ctx->processed_message.buffer;

    alt_u32 spdm_response_size = 0;

    // Check if responder supports CERTIFICATE, else exit
    if ((context->capability.flags & SUPPORTS_DIGEST_CERT_RESPONSE_MSG_MASK) == 0)
    {
        // Set next SPDM state
        context->spdm_msg_state = SPDM_CHALLENGE_FLAG;
        return SPDM_RESPONSE_SUCCESS;
    }

    SUCCESSFUL_CERTIFICATE* successful_cert_ptr = (SUCCESSFUL_CERTIFICATE*) processed_pkt;
    // Set slot number here
    // Just going to request for slot 0 which contains one certificate chain
    context->local_context.slot_num = 0;
    context->local_context.stored_cert_chain_size = 0;

    // CPLD will request in smaller chunk size to save code space
    alt_u16 length = CERT_REQUEST_SIZE;
    // Stored cert chain for comparison
    alt_u8* stored_cert_chain_ptr = (alt_u8*)&context->local_context.stored_cert_chain[0];

    // Begin the loop to obtain the entire certificate chain
    do
    {
        // This wont change but still need to done in this loop for storing
        // Construct GET_CERTIFICATE
        get_certificate_ptr->header.spdm_version = SPDM_VERSION;
        get_certificate_ptr->header.request_response_code = REQUEST_CERTIFICATE;
        get_certificate_ptr->header.param_2 = 0;

        get_certificate_ptr->header.param_1 = context->local_context.slot_num;
        get_certificate_ptr->offset = (alt_u16) context->local_context.stored_cert_chain_size;
        get_certificate_ptr->length = length;

        // Send and wait for responder CERTIFICATE
        spdm_response_size = send_and_receive_message(mctp_ctx, (alt_u8*)get_certificate_ptr,
                                                       sizeof(GET_CERTIFICATE), context->timeout);

        // Handles message not received or timeout
        if (!spdm_response_size)
        {
            return spdm_requester_error_handler(context, mctp_ctx, RESPONSE_CERTIFICATE,
                                                sizeof(SUCCESSFUL_CERTIFICATE) + MAX_CERT_CHAIN_SIZE);
        }

        // Handles SPDM error response code
        if (successful_cert_ptr->header.request_response_code == RESPONSE_ERROR)
        {
            return spdm_requester_error_handler(context, mctp_ctx, RESPONSE_CERTIFICATE,
                                                sizeof(SUCCESSFUL_CERTIFICATE) + MAX_CERT_CHAIN_SIZE);
        }

        successful_cert_ptr = (SUCCESSFUL_CERTIFICATE*) processed_pkt;
        if ((successful_cert_ptr->header.spdm_version != SPDM_VERSION) || (successful_cert_ptr->header.request_response_code != RESPONSE_CERTIFICATE))
        {
            return SPDM_RESPONSE_ERROR;
        }
        // Check if the requested certificate is longer than the requested length.
        // This will also cover the case if the the certificate is longer than the max certificate size
        if (spdm_response_size > (sizeof(SUCCESSFUL_CERTIFICATE) + get_certificate_ptr->length))
        {
            return SPDM_RESPONSE_ERROR;
        }

        // Check that the given length is not greater than requested length
        if (successful_cert_ptr->portion_length > get_certificate_ptr->length)
        {
            return SPDM_RESPONSE_ERROR;
        }

        // Check that correct slot number is returned
        if (successful_cert_ptr->header.param_1 != context->local_context.slot_num)
        {
            return SPDM_RESPONSE_ERROR;
        }

        // Check responded message size
        if (spdm_response_size < (sizeof(SUCCESSFUL_CERTIFICATE) + successful_cert_ptr->portion_length))
        {
            return SPDM_RESPONSE_ERROR;
        }

        spdm_response_size = sizeof(SUCCESSFUL_CERTIFICATE) + successful_cert_ptr->portion_length;

        // Append to M1
        append_large_buffer(&context->transcript.m1_m2, (alt_u8*)get_certificate_ptr, sizeof(GET_CERTIFICATE));
        append_large_buffer(&context->transcript.m1_m2, (alt_u8*)successful_cert_ptr, spdm_response_size);

        context->previous_cached_size += (sizeof(GET_CERTIFICATE) + spdm_response_size);

        // Store in byte rather than word as the cert chain returned might not be word aligned
        alt_u8_memcpy(stored_cert_chain_ptr, (alt_u8*)(successful_cert_ptr + 1), successful_cert_ptr->portion_length);
        // Save the cached cert chain length so far
        context->local_context.stored_cert_chain_size += successful_cert_ptr->portion_length;

        stored_cert_chain_ptr += successful_cert_ptr->portion_length;

        // Keep tracks of the remainder length
        length = (successful_cert_ptr->remainder_length >= CERT_REQUEST_SIZE)
                  ? CERT_REQUEST_SIZE : successful_cert_ptr->remainder_length;

    } while (successful_cert_ptr->remainder_length != 0);

    // Verify the digest and content of the cert chain
    if (requester_cpld_verify_cert_chain(context))
    {
        return SPDM_RESPONSE_ERROR;
    }

    // Set next SPDM state
    context->spdm_msg_state = SPDM_CHALLENGE_FLAG;
    return SPDM_RESPONSE_SUCCESS;
}

/*
 * @brief Request CHALLENGE message from the responder and verify them before proceeding.
 *
 * This function construct and send GET_CHALLENGE to responder. It also verifies the responded CHALLENGE_AUTH
 * before caching the data into M1 message. CPLD will construct nonce data from TRNG entropy for this
 * message. The M1 message from the responder shall be signed from the public keys extracted from the
 * leaf certificate of the responder. M1 message are all the cached message done from the responder side.
 * M2 message are all the appended message from the requester size in this case is the CPLD.
 * Responder shall sign their M1 message without including the signature field of CHALLENGE_AUTH message.
 *
 * CPLD verifies the signature with the extracted leaf keys and will be able to catch if M1 != M2
 * CPLD also perform misc checks like content of the CHALLENGE header, message length etc..
 *
 * CPLD only support EC384 OR EC256 and none others enumerated algorithm.
 *
 * @param context : SPDM info
 * @param mctp_ctx: MCTP info
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 requester_cpld_challenge(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx)
{
    CHALLENGE* challenge_ptr = (CHALLENGE*)&mctp_ctx->processed_message.buffer;
    reset_buffer((alt_u8*)challenge_ptr, sizeof(CHALLENGE));

    // Check if responder supports GET_CHALLENGE, else exit
    if ((context->capability.flags & SUPPORTS_CHALLENGE_AUTH_RESPONSE_MSG_MASK) == 0)
    {
        // Set next SPDM message state
        context->spdm_msg_state = SPDM_MEASUREMENT_FLAG;
        return SPDM_RESPONSE_SUCCESS;
    }

    // Set slot number here
    // Just going to request for slot 0 which contains one certificate chain
    context->local_context.slot_num = 0;

    // Construct CHALLENGE
    challenge_ptr->header.spdm_version = SPDM_VERSION;
    challenge_ptr->header.request_response_code = REQUEST_CHALLENGE;
    challenge_ptr->header.param_1 = context->local_context.slot_num;
    // request for all measurement hash
    challenge_ptr->header.param_2 = MEAS_HASH_TYPE_ALL_MEAS_HASH;
    // Check if responder can provide measurements
    if ((context->capability.flags &
        (SUPPORTS_MEAS_RESPONSE_WITHOUT_SIG_GEN_MSG_MASK | SUPPORTS_MEAS_RESPONSE_WITH_SIG_GEN_MSG_MASK)) == 0)
    {
        challenge_ptr->header.param_2 = MEAS_HASH_TYPE_NO_MEAS_HASH;
    }

    // Extract random value using RNG entropy
    alt_u8* nonce_ptr = (alt_u8*)challenge_ptr->nonce;
    alt_u32 nonce_entropy[SPDM_NONCE_SIZE/4] = {0};

#ifdef USE_SYSTEM_MOCK
    // To test and make sure the provided data to verify signature is consistent, need to have a fixed nonce value
    for (alt_u32 i = 0; i < SPDM_NONCE_SIZE/4; i++)
    {
        nonce_entropy[i] = 0x11111111;
    }
#else
    extract_entropy_data((alt_u32*)nonce_entropy, SPDM_NONCE_SIZE/4);
#endif

    alt_u8_memcpy(nonce_ptr, (alt_u8*)&nonce_entropy[0], SPDM_NONCE_SIZE);

    // Append to M2
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)challenge_ptr, sizeof(CHALLENGE));
    context->previous_cached_size = sizeof(CHALLENGE);

    alt_u8* processed_pkt = (alt_u8*)&mctp_ctx->processed_message.buffer;

    // Send and wait for responder CHALLENGE_AUTH
	// Challenge takes longest time to be processed
	context->timeout = 0x1E;
    alt_u32 spdm_response_size = send_and_receive_message(mctp_ctx, (alt_u8*)challenge_ptr,
                                                   sizeof(CHALLENGE), context->timeout);

    // Handles message not received or timeout
    if (!spdm_response_size)
    {
        return spdm_requester_error_handler(context, mctp_ctx, RESPONSE_CHALLENGE_AUTH,
                                            sizeof(CHALLENGE_AUTH));
    }

    CHALLENGE_AUTH* challenge_auth_ptr = (CHALLENGE_AUTH*) processed_pkt;

    // Handles SPDM error response code
    if (challenge_auth_ptr->header.request_response_code == RESPONSE_ERROR)
    {
        return spdm_requester_error_handler(context, mctp_ctx, RESPONSE_CHALLENGE_AUTH,
                                            sizeof(CHALLENGE_AUTH));
    }

    challenge_auth_ptr = (CHALLENGE_AUTH*) processed_pkt;
    if ((challenge_auth_ptr->header.spdm_version != SPDM_VERSION) || (challenge_auth_ptr->header.request_response_code != RESPONSE_CHALLENGE_AUTH))
    {
        return SPDM_RESPONSE_ERROR;
    }
    // Check max size
    if (spdm_response_size > sizeof(CHALLENGE_AUTH))
    {
        return SPDM_RESPONSE_ERROR;
    }

    if (spdm_response_size < sizeof(SPDM_HEADER))
    {
        return SPDM_RESPONSE_ERROR;
    }

    // Ensure slot num is consistent for the cert chain
    if (challenge_auth_ptr->header.param_1 != context->local_context.slot_num)
    {
        return SPDM_RESPONSE_ERROR;
    }

    // Ensure responder set the slot mask bit correctly
    if (challenge_auth_ptr->header.param_2 != (1 << context->local_context.slot_num))
    {
        return SPDM_RESPONSE_ERROR;
    }

    alt_u8 sha_size = (context->connection_info.algorithm.base_hash_algo & BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
                        ? SHA384_LENGTH : SHA256_LENGTH;
    alt_u8 sig_size = (context->connection_info.algorithm.base_asym_algo & BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
                        ? PFR_ROOT_KEY_DATA_SIZE_FOR_384 : PFR_ROOT_KEY_DATA_SIZE_FOR_256;

    CRYPTO_MODE sha_mode = (context->connection_info.algorithm.base_hash_algo & BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
                            ? CRYPTO_384_MODE : CRYPTO_256_MODE;

    alt_u8* ptr = processed_pkt + sizeof(SPDM_HEADER);
    alt_u32 challenge_hash[SHA384_LENGTH/4] = {0};

    alt_u8_memcpy((alt_u8*)&challenge_hash[0], ptr, sha_size);

    alt_u32* digest_ptr = challenge_hash;
    // Check the cert chain hash given and the one stored to ensure cert chain hash match what was given
    if (!requester_cpld_verify_digest(context, digest_ptr))
    {
        return SPDM_RESPONSE_ERROR;
    }

    ptr += sha_size;
    ptr += SPDM_NONCE_SIZE;
    alt_u8* meas_summary = (alt_u8*) ptr;
    ptr += sha_size;

    alt_u16 opaque_length = 0;
    alt_u8_memcpy((alt_u8*)&opaque_length, ptr, sizeof(alt_u16));
    ptr += sizeof(alt_u16);

    // Check responded message size
    if (spdm_response_size < sizeof(SPDM_HEADER) +
                             sha_size +
							 SPDM_NONCE_SIZE +
							 sha_size +
                             sizeof(alt_u16) +
                             opaque_length +
                             sig_size)
    {
        return SPDM_RESPONSE_ERROR;
    }
    spdm_response_size = sizeof(SPDM_HEADER) +
                         sha_size +
			             SPDM_NONCE_SIZE +
			             sha_size +
                         sizeof(alt_u16) +
                         opaque_length +
                         sig_size;

    ptr += opaque_length;

    alt_u32 chal_sig[PFR_ROOT_KEY_DATA_SIZE_FOR_384/4] = {0};

    alt_u8_memcpy((alt_u8*)&chal_sig[0], ptr, sig_size);
    alt_u32* sig_ptr = (alt_u32*) chal_sig;

    // Append to M2
    append_large_buffer(&context->transcript.m1_m2, (alt_u8*)challenge_auth_ptr, spdm_response_size - sig_size);

    alt_u32* msg_to_hash = (alt_u32*)&context->transcript.m1_m2.buffer[0];

    //alt_u32* pub_key = retrieve_afm_pub_key(context->uuid);

    // Obtained the stored leaf cert keys extracted from the verified slot cert chain
    alt_u32* pub_key = (alt_u32*)&context->local_context.stored_chal_meas_signing_key[0];


    // Verify the signature of M1 message
    if (!verify_ecdsa_and_sha(pub_key, pub_key + (sha_size/4), sig_ptr, sig_ptr + (sha_size/4),
                              0, msg_to_hash, context->transcript.m1_m2.buffer_size, sha_mode, DISENGAGE_DMA))
    {
        return SPDM_RESPONSE_ERROR_CHAL_AUTH;
    }

    // Reset buffer M1M2 once done
    reset_buffer((alt_u8*)&context->transcript.m1_m2, sizeof(LARGE_APPENDED_BUFFER));

    // Check if responder support having measurement, if yes save those measurement summary hash
    if (!((context->capability.flags &
        (SUPPORTS_MEAS_RESPONSE_WITHOUT_SIG_GEN_MSG_MASK | SUPPORTS_MEAS_RESPONSE_WITH_SIG_GEN_MSG_MASK)) == 0))
    {
        alt_u8* meas_summary_hash_ptr = (alt_u8*) &context->local_context.stored_measurement_summary_hash[0];
        alt_u8_memcpy(meas_summary_hash_ptr, meas_summary, sha_size);
    }

    // Set next SPDM message state
    context->spdm_msg_state = SPDM_MEASUREMENT_FLAG;
    return SPDM_RESPONSE_SUCCESS;
}

/*
 * @brief Request MEASUREMENT message from the responder and verify them before proceeding.
 *
 * This function construct and send GET_MEASUREMENT to responder. It also verifies the responded MEASUREMENT
 * before caching the data into L2 message. CPLD will construct nonce data from TRNG entropy for this
 * message. The L1 message from the responder shall be signed from the public keys extracted from the
 * leaf certificate of the responder. L1 message are all the cached measurement message done from the
 * responder side. L2 message are all the appended message from the requester size in this case is the CPLD.
 * Responder shall sign their L1 message without including the signature field of MEASUREMENT message.
 *
 * CPLD verifies the signature with the extracted leaf keys and will be able to catch if L1 != L2
 * CPLD also perform misc checks like content of the MEASUREMENT header, message length etc..
 *
 * CPLD only support EC384 OR EC256 and none others enumerated algorithm.
 *
 * NOTE: CPLD will not request for a number of measurements from the responder as CPLD already have this
 * information in the AFM. This saves some code space as well. CPLD will follow a strict requirement and
 * matches the measurements given the AFM to the measurements obtained from the MEASUREMENT message.
 *
 * @param context    : SPDM info
 * @param mctp_ctx   : MCTP info
 * @param meas_index : Index measurement in the AFM
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 requester_cpld_get_measurement(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx, alt_u8 meas_index)
{
    GET_MEASUREMENT get_measurement;
    GET_MEASUREMENT* get_measurement_ptr = &get_measurement;
    reset_buffer((alt_u8*)get_measurement_ptr, sizeof(GET_MEASUREMENT));

    // Default request attributes, set accordingly later
    alt_u8 request_attribute = 0;
    alt_u32 spdm_request_size = sizeof(GET_MEASUREMENT);
    alt_u32 meas_data_length = 0;

    // Set as according to supported algorithm
    alt_u8 sig_size = (context->connection_info.algorithm.base_asym_algo & BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
                        ? PFR_ROOT_KEY_DATA_SIZE_FOR_384 : PFR_ROOT_KEY_DATA_SIZE_FOR_256;

    CRYPTO_MODE sha_mode = (context->connection_info.algorithm.base_hash_algo & BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384)
                            ? CRYPTO_384_MODE : CRYPTO_256_MODE;

    // Check if responder supports MEASUREMENT, else exit
    if ((context->capability.flags &
        (SUPPORTS_MEAS_RESPONSE_WITH_SIG_GEN_MSG_MASK | SUPPORTS_MEAS_RESPONSE_WITHOUT_SIG_GEN_MSG_MASK)) == 0)
    {
        return SPDM_RESPONSE_SUCCESS;
    }

    // Check if EC signing is supported by the responder
    if (context->capability.flags & SUPPORTS_MEAS_RESPONSE_WITH_SIG_GEN_MSG_MASK)
    {
        request_attribute = SPDM_GET_MEASUREMENT_REQUEST_GENERATE_SIG;
    }

    // Construct GET_MEASUREMENT
    get_measurement_ptr->header.spdm_version = SPDM_VERSION;
    get_measurement_ptr->header.request_response_code = REQUEST_MEASUREMENTS;
    get_measurement_ptr->header.param_1 = request_attribute;
    // From the measurement index in the AFM, CPLD will request the measurements
    get_measurement_ptr->header.param_2 = meas_index;

    // Extract random value using RNG entropy
    alt_u8* nonce_ptr = (alt_u8*)get_measurement_ptr->nonce;
    alt_u32 nonce_data[SPDM_NONCE_SIZE/4] = {0};

    // Responder is incapable of generating signature
    if (!request_attribute)
    {
        sig_size = 0;
        spdm_request_size = sizeof(SPDM_HEADER);
    }
    else
    {
#ifdef USE_SYSTEM_MOCK
        // To test and make sure the provided data to verify signature is consistent, need to have a fixed nonce value
        for (alt_u32 i = 0; i < SPDM_NONCE_SIZE/4; i++)
        {
            nonce_data[i] = 0x11111111;
        }
#else
        // Create nonce data
        extract_entropy_data((alt_u32*)nonce_data, SPDM_NONCE_SIZE/4);
#endif
    }

    alt_u8_memcpy(nonce_ptr, (alt_u8*)&nonce_data[0], SPDM_NONCE_SIZE);

    // Append L2 message
    append_large_buffer(&context->transcript.l1_l2, (alt_u8*)get_measurement_ptr, spdm_request_size);

    alt_u8* processed_pkt = (alt_u8*)&mctp_ctx->processed_message.buffer;

    // Send and wait for responder MEASUREMENT
    alt_u32 spdm_response_size = send_and_receive_message(mctp_ctx, (alt_u8*)get_measurement_ptr,
                                                  spdm_request_size, context->timeout);

    // Handles message not received or timeout
    if (!spdm_response_size)
    {
        return spdm_requester_error_handler(context, mctp_ctx, RESPONSE_MEASUREMENTS,
                                            sizeof(MEASUREMENT));
    }

    MEASUREMENT* measurement_ptr = (MEASUREMENT*) processed_pkt;
    // Handles SPDM error response code
    if (measurement_ptr->header.request_response_code == RESPONSE_ERROR)
    {
        return spdm_requester_error_handler(context, mctp_ctx, RESPONSE_MEASUREMENTS,
                                            sizeof(MEASUREMENT));
    }

    measurement_ptr = (MEASUREMENT*) processed_pkt;
    if ((measurement_ptr->header.spdm_version != SPDM_VERSION) || (measurement_ptr->header.request_response_code != RESPONSE_MEASUREMENTS))
    {
        return SPDM_RESPONSE_ERROR;
    }

    // Check incoming message size
    if (spdm_response_size < sizeof(SPDM_HEADER))
    {
        return SPDM_RESPONSE_ERROR;
    }

    if (spdm_response_size > sizeof(MEASUREMENT))
    {
        return SPDM_RESPONSE_ERROR;
    }

    // At this stage, measurements have been saved into the AFM
    // CPLD request measurements based on the index given in the AFM
    if ((get_measurement_ptr->header.param_2 == SPDM_GET_ALL_MEASUREMENT) &&
        (measurement_ptr->num_of_blocks == 0))
    {
        // GET_MEASUREMENT param 2 can be changed to request all measurements
        return SPDM_RESPONSE_ERROR;
    }
    else if ((get_measurement_ptr->header.param_2 == 0) &&
             (measurement_ptr->num_of_blocks != 0))
    {
        // GET_MEASUREMENT param 2 can be changed to 0 if CPLD is required to request number of measurements
        return SPDM_RESPONSE_ERROR;
    }
    else if (measurement_ptr->num_of_blocks != 1)
    {
        // CPLD is using this path as information is available in AFM pre-SPDM process
        return SPDM_RESPONSE_ERROR;
    }

    // Handle unaligned bytes and endian-ness
    meas_data_length = alt_u24_read(measurement_ptr->meas_record_length);

    if (get_measurement_ptr->header.param_2 == 0)
    {
        // Should be empty as param2 is to request nmber of measurements
        if (meas_data_length != 0)
        {
            return SPDM_RESPONSE_ERROR;
        }
    }

    // Check incoming message size
    // Responder is capable of generating signature, hence signature verification is required
    if (request_attribute == SPDM_GET_MEASUREMENT_REQUEST_GENERATE_SIG)
    {
        if (spdm_response_size < sizeof(SPDM_HEADER) +
                                 sizeof(alt_u32) +
								 meas_data_length +
								 SPDM_NONCE_SIZE +
                                 sizeof(alt_u16))
        {
            return SPDM_RESPONSE_ERROR;
        }
        alt_u8* ptr = measurement_ptr->meas_record + meas_data_length;
        ptr += SPDM_NONCE_SIZE;
        alt_u16 opaque_length = 0;
        alt_u8_memcpy((alt_u8*)&opaque_length, ptr, sizeof(alt_u16));
        ptr += sizeof(alt_u16);

        if (spdm_response_size < sizeof(SPDM_HEADER) +
                                 sizeof(alt_u32) +
                                 meas_data_length +
                                 SPDM_NONCE_SIZE +
                                 sizeof(alt_u16) +
                                 opaque_length +
                                 sig_size)
        {
            return SPDM_RESPONSE_ERROR;
        }

        spdm_response_size = sizeof(SPDM_HEADER) +
                             sizeof(alt_u32) +
                             meas_data_length +
                             SPDM_NONCE_SIZE +
                             sizeof(alt_u16) +
                             opaque_length +
                             sig_size;

        ptr += opaque_length;
        alt_u32 meas_sig[PFR_ROOT_KEY_DATA_SIZE_FOR_384/4] = {0};

        alt_u8_memcpy((alt_u8*)&meas_sig[0], ptr, sig_size);
        alt_u32* signature = (alt_u32*) meas_sig;

        // Append to L2
        append_large_buffer(&context->transcript.l1_l2, (alt_u8*)measurement_ptr, spdm_response_size - sig_size);

        alt_u32* msg_to_hash = (alt_u32*)&context->transcript.l1_l2.buffer[0];
        //alt_u32* pub_key = retrieve_afm_pub_key(context->uuid);

        alt_u32* pub_key = (alt_u32*)&context->local_context.stored_chal_meas_signing_key[0];

        // Verify the signature of the message
        // For SPDM implementation, it is important use FW padding as DMA can only pad 4-byte aligned data
        // Manual padding from firmware is flexible in terms of message size
        if (!verify_ecdsa_and_sha(pub_key, pub_key + (sig_size/8), signature, signature + (sig_size/8),
                                  0, msg_to_hash, context->transcript.l1_l2.buffer_size, sha_mode, DISENGAGE_DMA))
        {
            return SPDM_RESPONSE_ERROR;
        }

        // Reset buffer L1L2 once done
        reset_buffer((alt_u8*)&context->transcript.l1_l2, sizeof(LARGE_APPENDED_BUFFER));
    }
    else
    {
        // Responder is incapable of generating signature for this MEASUREMENT message
    	// CPLD shall check and set the response message size accordingly
        if (get_measurement_ptr->header.param_2 == 0)
        {
            // Requester is not expecting any measurements here
            spdm_response_size = sizeof(SPDM_HEADER) + sizeof(alt_u32);
        }
        else
        {
            // Measurements are expected, check and set response size
            spdm_response_size = sizeof(SPDM_HEADER) + sizeof(alt_u32) + meas_data_length;
        }

        // Append to L2
        append_large_buffer(&context->transcript.l1_l2,
                            (alt_u8*)measurement_ptr, spdm_response_size - sig_size);
    }

    alt_u8 block_num = 0;
    if (get_measurement_ptr->header.param_2 == 0)
    {
        // Save the number of measurements responder have
        block_num = measurement_ptr->header.param_1;
    }
    else
    {
        // At this stage, CPLD already verified that the number of blocks returned by responder is correct
        alt_u8* measurement_block_ptr = (alt_u8*)&measurement_ptr->meas_record;
        block_num = measurement_ptr->num_of_blocks;
        reset_buffer((alt_u8*)&context->transcript.m1_m2, sizeof(LARGE_APPENDED_BUFFER));

        // Saves the measurement blocks of the responder
        for (alt_u8 i = 0; i < block_num ; i++)
        {
            //alt_u32 size = SHA512_LENGTH;
            SPDM_MEASUREMENT_BLOCK_DMTF_HEADER* size_ptr = (SPDM_MEASUREMENT_BLOCK_DMTF_HEADER*)measurement_block_ptr;
            alt_u32 size = (alt_u16)(size_ptr->meas_dmtf.dmtf_spec_meas_value_size);

            measurement_block_ptr += sizeof(SPDM_MEASUREMENT_BLOCK_DMTF_HEADER);
            // M1M2 buffer is now free, use them to save the measurement block
            // This save total space usage
            append_large_buffer(&context->transcript.m1_m2, measurement_block_ptr, size);
            measurement_block_ptr += size;
        }

        alt_u8 idx_possible_meas = 0;
        alt_u32 idx_possible_meas_size = 0;
        alt_u32* cached_meas_hash = (alt_u32*)&context->transcript.m1_m2.buffer[0];

        // Retrieve the stored measurement in the AFM based on the index and the UUID
        alt_u32* stored_meas = retrieve_afm_index_measurement(context->uuid, meas_index, &idx_possible_meas, &idx_possible_meas_size);
        alt_u32 is_stored_meas_ver_matching = 0;

        // There might be multiple possible measurements returned
        // For example, responder can save 'n' versions of measurements into the AFM
        // During the exchange of the MEASUREMENT message, responder can choose to return a particular version
        // of the measurement for the index. CPLD check that at least one of the version of the measurement
        // matches. For example, responder can choose to save 3 firmware hash of different versions in AFM while
        // returning the measurement of all 3 versions or any of them.
        while (idx_possible_meas)
        {
            // Check if measurements matched
            is_stored_meas_ver_matching = check_matched_hash(cached_meas_hash, idx_possible_meas_size,
                                                             stored_meas, idx_possible_meas_size, sha_mode);
            if (is_stored_meas_ver_matching)
            {
                break;
            }
            idx_possible_meas--;
            // measurements are padded to 4-byte aligned sizes
            stored_meas += (round_up_to_multiple_of_4(idx_possible_meas_size) >> 2);
        }

        // None of the measurements match the stored measurements
        if (!is_stored_meas_ver_matching)
        {
            return SPDM_RESPONSE_ERROR_MEAS_MISMATCH;
        }
    }

    return SPDM_RESPONSE_SUCCESS;
}

/*
 * @brief Request MEASUREMENT message handler.
 *
 * This function checks for the total number of measurements (in terms of index) stored
 * in the AFM for a particular device. For each of the index, CPLD will request for the MEASUREMENT message
 * from the responder.
 *
 * For one device, CPLD can send multiple GET_MEASUREMENT messages based on the measurements stored in the AFM.
 * It is expected for responder to send all indices measurements provided in the AFM.
 *
 * CPLD only support EC384 OR EC256 and none others enumerated algorithm.
 *
 * @param context : SPDM info
 * @param mctp_ctx: MCTP info
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 requester_cpld_get_measurement_handler(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx)
{
    //context->timeout = 1;
    alt_u32 total_idx = get_total_number_of_measurements(context->uuid) + 1;
    // starts with index 1 of measurement
    alt_u8 current_meas_idx = 1;
    // TODO: Might want to set max measurement index
    /*if (meas_idx > 10)
    {
        return SPDM_RESPONSE_ERROR;
    }*/
    alt_u32 status = SPDM_RESPONSE_ERROR;

    // Request for the measurements of all indices
    while (current_meas_idx < total_idx)
    {
        status = requester_cpld_get_measurement(context, mctp_ctx, current_meas_idx);
        current_meas_idx++;
        if (status)
        {
            break;
        }
    }

    // Finish attestation
    context->spdm_msg_state = SPDM_ATTEST_FINISH_FLAG;
    return status;
}

/*
 * @brief CPLD's SPDM Request message handler.
 *
 * This function tracks the state of the SPDM message CPLD is having with the responder.
 * This function also handle some SPDM response error protocol
 *
 * Based on the retry times set in the context, CPLD will retry with the current message state if
 * ERROR_RESPONSE_BUSY is the responder's message for that particular state.
 *
 * @param context : SPDM info
 * @param mctp_ctx: MCTP info
 *
 * @return 0 if successful, otherwise return error code
 */
static alt_u32 request_message_handler(SPDM_CONTEXT* context, MCTP_CONTEXT* mctp_ctx)
{
    // Number of retry times are set to 2 times for now and can be changed accordingly
    alt_u8 retry = context->retry_times;
    alt_u32 status = 0;
    do
    {
        // This will track the SPDM message state CPLD is having with the responder
        if (context->spdm_msg_state & SPDM_VERSION_FLAG)
        {
            status = requester_cpld_get_version(context, mctp_ctx);
        }
        else if (context->spdm_msg_state & SPDM_CAPABILITIES_FLAG)
        {
            status = requester_cpld_get_capability(context, mctp_ctx);
        }
        else if (context->spdm_msg_state & SPDM_ALGORITHM_FLAG)
        {
            status = requester_cpld_negotiate_algorithm(context, mctp_ctx);
        }
        else if (context->spdm_msg_state & SPDM_DIGEST_FLAG)
        {
            status = requester_cpld_get_digest(context, mctp_ctx);
        }
        else if (context->spdm_msg_state & SPDM_CERTIFICATE_FLAG)
        {
            // Need not stage the message here as staged internally
            status = requester_cpld_get_certificate(context, mctp_ctx);
        }
        else if (context->spdm_msg_state & SPDM_CHALLENGE_FLAG)
        {
            status = requester_cpld_challenge(context, mctp_ctx);
        }
        else if (context->spdm_msg_state & SPDM_MEASUREMENT_FLAG)
        {
            status = requester_cpld_get_measurement_handler(context, mctp_ctx);
        }

        if (!(status & SPDM_RESPONSE_ERROR_REQUIRE_RETRY))
        {
            /*if (status & SPDM_RESPONSE_ERROR_INVALID_REQUEST)
            {
                return spdm_respond_invalid_request((MCTP_CONTEXT*)&context->mctp_context, challenger);
            }*/
            return status;
        }

    } while (retry-- != 0);

    return status;
}

#endif

#endif /* EAGLESTREAM_INC_SPDM_REQUESTER_H_ */
