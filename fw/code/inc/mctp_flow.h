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
 * @file mctp_flow.h
 * @brief MCTP protocol through SMBus Host Mailbox.
 */

#ifndef EAGLESTREAM_INC_MCTP_FLOW_H_
#define EAGLESTREAM_INC_MCTP_FLOW_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "spdm_requester.h"
#include "spdm_responder.h"
#include "spdm.h"
#include "platform_log.h"
#include "transition.h"
#include "mctp_transport.h"

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/*
 * @brief This function publish any errors caught during SPDM message transaction.
 *
 * Major error code corresponds to the attributes of errors.
 * Minor error code corresponds to which message type linked to the Major Error Code.
 *
 * @param message : SPDM message type
 * @param error_state : Error attributes
 *
 * @return none.
 */
static void log_attestation_spdm_error(alt_u32 message, alt_u32 error_state)
{
    STATUS_MAJOR_ERROR_ENUM major_err = MAJOR_ERROR_ATTESTATION_MEAS_VAL_MISMATCH;
    STATUS_MINOR_ERROR_ATTESTATION_MSG_ENUM minor_err = MINOR_ERROR_SPDM_MEASUREMENT;

    // Error attributes
    if ((error_state & SPDM_RESPONSE_ERROR) || (error_state & SPDM_RESPONSE_ERROR_CHAL_AUTH))
    {
        major_err = MAJOR_ERROR_SPDM_PROTOCOL_ERROR;
    }
    else if (error_state & SPDM_RESPONSE_ERROR_TIMEOUT)
    {
        major_err = MAJOR_ERROR_ATTESTATION_CHALLENGE_TIMEOUT;
    }
    else if (error_state & SPDM_RESPONSE_ERROR_COMMUNICATION_FAIL)
    {
        major_err = MAJOR_ERROR_COMMUNICATION_FAIL;
    }

    // SPDM message type
    if (message & SPDM_CONNECTION_FLAG)
    {
        minor_err = MINOR_ERROR_SPDM_CONNECTION;
    }
    else if (message & SPDM_DIGEST_FLAG)
    {
        minor_err = MINOR_ERROR_SPDM_DIGEST;
    }
    else if (message & SPDM_CERTIFICATE_FLAG)
    {
        minor_err = MINOR_ERROR_SPDM_CERTIFICATE;
    }
    else if (message & SPDM_CHALLENGE_FLAG)
    {
        minor_err = MINOR_ERROR_SPDM_CHALLENGE;
    }
    // Log errors
    log_errors(major_err, minor_err);
}

/*
 * @brief This function checks AFM policy for if PFR needs to do
 * a reset or recovery to the device post attestation failure.
 *
 * @param spi_flash_type : SPI Flash type
 * @param error_state : Failure type
 * @param policy : AFM policy
 *
 * @return none.
 */
static void attestation_policy_handler(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 error_state, alt_u8 policy)
{
    // Error attributes
    // Check policy for a particular spdm message failure and if required, perform reset
    // else, do nothing and continue as error log has been published
    // If there is any issue on the SCL path for smbus communication, do nothing
    if ((!(error_state & SPDM_RESPONSE_ERROR_COMMUNICATION_FAIL)) &&
          (policy & AFM_POLICY_DEVICE_REQUIRES_LOCKDOWN_BIT_MASK))
    {
        perform_attestation_policy_reset(spi_flash_type, policy);
    }
}

/*
 * @brief This function checks if PFR is capable to perform cryptographic operations.
 * If TRNG functionality does not work, PFR deemed it as unsafe to create signature and keys.
 *
 * @param spdm_context : SPDM context
 *
 * @return none.
 */
static void set_pfr_spdm_responder_capability(SPDM_CONTEXT* spdm_context)
{
    // If TRNG hw failed health test, PFR cannot create keys and certificate
    // Configure cpld's capabilities
    if (*get_ufm_cpld_cert_ptr() == 0xFFFFFFFF)
    {
        spdm_context->capability.flags = SUPPORTS_MEAS_RESPONSE_WITHOUT_SIG_GEN_MSG_MASK;
    }
}

/*
 * @brief Initialize MCTP context for SPDM operation
 *
 * @param mctp_context_ptr : MCTP_SPDM context
 * @param phy_bind : physical binding for mctp
 * @param uuid : UUID
 *
 * @return none.
 */
static void init_mctp_spdm_context(MCTP_CONTEXT* mctp_context_ptr, MCTP_PHY_TRANS_BIND_ID phy_bind, alt_u16 uuid)
{
    // Initialise MCTP context
    mctp_context_ptr->msg_type = MCTP_MESSAGE_TYPE;
    // Get address of AFM
    alt_u32 afm_addr = get_ufm_afm_addr_offset(uuid);
    AFM_BODY* afm_body = (AFM_BODY*) get_ufm_ptr_with_offset(afm_addr);

    if (phy_bind == SMBUS)
    {
        mctp_context_ptr->bus_type = 0;
        mctp_context_ptr->cached_addr = afm_body->device_addr;
    }
}

/*
 * @brief This function initializes all CPLD's SPDM context for requester and responder mode.
 *
 * Initializes information such as timeout, retry times, message states, supported enumerated algorithms, etc..
 * Important to call this function prior to attesting with SPDM protocol. These context can also be changed
 * dynamically during runtime operation with the device being attested.
 *
 * @param spdm_context : SPDM info
 *
 * @return none.
 */
static void init_spdm_context(SPDM_CONTEXT* spdm_context)
{
#ifdef SPDM_DEBUG
    // Enable 300 milli seconds timeout to loosen margin for testing
    // Customize here for attestation timeout
    spdm_context->timeout = 0x0F;
#else
    // Set timeout to 20ms (smallest unit)
    spdm_context->timeout = 1;
#endif
    // Set retry times to twice
    spdm_context->retry_times = 2;
    // Always expect the first SPDM message to be VERSION
    spdm_context->spdm_msg_state = SPDM_VERSION_FLAG;
    // Clear error state
    spdm_context->error_state = 0;
    // Clear token
    spdm_context->token = 0;
    // CPLD's own opaque length
    spdm_context->local_context.cpld_opaque_length = 2;
    // Initialise CT exponent
    spdm_context->capability.ct_exponent = 1;
    spdm_context->response_state = spdm_normal_state;

    // Set the capability, algorithm supported from the CPLD
    spdm_context->capability.flags = SUPPORTS_DIGEST_CERT_RESPONSE_MSG_MASK |
                                     SUPPORTS_CHALLENGE_AUTH_RESPONSE_MSG_MASK |
                                     SUPPORTS_MEAS_RESPONSE_WITH_SIG_GEN_MSG_MASK;
    spdm_context->algorithm.measurement_hash_algo = MEAS_HASH_ALGO_TPM_ALG_SHA_256 |
                                                    MEAS_HASH_ALGO_TPM_ALG_SHA_384;
    spdm_context->algorithm.base_asym_algo = BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                                             BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;
    spdm_context->algorithm.base_hash_algo = BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384 |
                                             BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256;
}

/*
 * @brief This function identifies if there is any incoming SPDM requester message coming from all
 * available hosts.
 *
 * Reads the mailbox fifo if there is any message available and proceeds to call an SPDM routine
 * to process the request message. The SPDM routine will continue to wait for incoming messages after the
 * first SPDM message until timeout is captured or any errors recorded along the process.
 *
 * @param mctp_context_ptr : MCTP context
 *
 * @return 1 always just to know its finished.
 */
static alt_u32 cpld_checks_request_from_host(MCTP_CONTEXT* mctp_context_ptr)
{
    // Initialise context
    // Move to separate function
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset context
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));
    // Initialise SPDM context
    init_spdm_context(spdm_context_ptr);
    // Here checks if CPLD is capable of performing any signing activities
    set_pfr_spdm_responder_capability(spdm_context_ptr);
    // TODO: Replace UUID
    spdm_context_ptr->uuid = 0x02;

    // Initialise MCTP context
    mctp_context_ptr->msg_type = mctp_context_ptr->cached_msg_type;

    // While CPLD is in responder mode, it may receive interleave messages (SPDM and non-SPDM),
    // thus, timer will be used to check for timeout at this level
    alt_u32 past_time = IORD(U_TIMER_BANK_TIMER4_ADDR, 0);

    // SPDM routine
    while (1)
    {
        reset_hw_watchdog();
        
        if (mctp_context_ptr->processed_message.buffer_size)
        {
            alt_u32 spdm_service_not_finished = respond_message_handler(spdm_context_ptr, mctp_context_ptr,
                                                    mctp_context_ptr->processed_message.buffer_size);
            // Wait for 1 second for the next SPDM request message, need to have a total longer waiting
            // time else due to the possibility of having mix messages, timeout may happen too frequently
            start_timer(U_TIMER_BANK_TIMER5_ADDR, 0x32);

            if (spdm_service_not_finished)
            {
                log_attestation_spdm_error(spdm_context_ptr->spdm_msg_state, spdm_service_not_finished);
                break;
            }

            if (spdm_context_ptr->spdm_msg_state & SPDM_ATTEST_FINISH_FLAG)
            {
                clear_attestation_failure();
                spdm_context_ptr->spdm_msg_state = 0;
                break;
            }
        }
        else if (is_timer_expired(U_TIMER_BANK_TIMER5_ADDR))
        {
            // Log any recorded error
            log_attestation_spdm_error(spdm_context_ptr->spdm_msg_state, SPDM_RESPONSE_ERROR_TIMEOUT);
            break;
        }
        // Get the size of the incoming SPDM request message
        mctp_context_ptr->processed_message.buffer_size = receive_i2c_message(mctp_context_ptr, spdm_context_ptr->timeout);
    }

    start_timer(U_TIMER_BANK_TIMER4_ADDR, past_time);
    IOWR(U_TIMER_BANK_TIMER5_ADDR, 0, 0);
    return 1;
}

/*
 * @brief This function initiates SPDM transaction with attest-able devices (responder) based on uuid stored.
 *
 * Initializes connection with the responder based on the UUID stored in the AFM(s). CPLD will call an
 * SPDM routine which sends the SPDM request message to the responder and wait for replies. Handles the
 * error log and also any timeout related issues. This functions ends if there are timeout or errors associated
 * with the SPDM messages.
 *
 * @param mctp_context_ptr : MCTP context
 * @param uuid : Unique identifier of devices
 *
 * @return 1 always just to know its finished.
 */
static alt_u32 cpld_sends_request_to_host(MCTP_CONTEXT* mctp_context_ptr, alt_u16 uuid)
{
    SPDM_CONTEXT spdm_context;
    SPDM_CONTEXT* spdm_context_ptr = &spdm_context;

    // Reset context
    reset_buffer((alt_u8*)spdm_context_ptr, sizeof(SPDM_CONTEXT));
    reset_buffer((alt_u8*)&mctp_context_ptr->processed_message, sizeof(MEDIUM_APPENDED_BUFFER));

    // Initialise SPDM context
    init_spdm_context(spdm_context_ptr);
    // Get UUID
    spdm_context_ptr->uuid = uuid;

    alt_u32 spdm_service_not_finished = 0;

    // SPDM routine
    while (1)
    {
        reset_hw_watchdog();
        spdm_service_not_finished = request_message_handler(spdm_context_ptr, mctp_context_ptr);

        if (spdm_service_not_finished)
        {
            // Log recorded errors
            log_attestation_spdm_error(spdm_context_ptr->spdm_msg_state, spdm_service_not_finished);
            break;
        }

        if (spdm_context_ptr->spdm_msg_state & SPDM_ATTEST_FINISH_FLAG)
        {
            clear_attestation_failure();
            spdm_context_ptr->spdm_msg_state = 0;
            break;
        }
    }
    return spdm_service_not_finished;
}

/*
 * @brief This function initiates SPDM transaction with attest-able devices (responder) based on uuid stored.
 *
 * Check for the number of devices to be attested and attest them one by one. At the moment only PCIE path
 * can be attested.
 *
 * @param mctp_context_ptr : MCTP context
 * @param phy_bind : mctp pshysical binding
 *
 * @return none.
 */
static void cpld_challenge_all_devices(MCTP_CONTEXT* mctp_ctx_ptr, MCTP_PHY_TRANS_BIND_ID phy_bind)
{
    alt_u32* afm_addr = get_ufm_ptr_with_offset(UFM_AFM_1);
    alt_u32 number_of_devices = MAX_NO_OF_AFM_SUPPORTED - get_ufm_empty_afm_number();
    alt_u32 incr_size = (UFM_CPLD_PAGE_SIZE >> 2);

    while (number_of_devices)
    {
        AFM_BODY* afm_body = (AFM_BODY*) afm_addr;
        alt_u16 stored_uuid = afm_body->uuid;

        if (afm_body->binding_spec == phy_bind)
        {
            // Attest corresponding device
            init_mctp_spdm_context(mctp_ctx_ptr, phy_bind, stored_uuid);
            alt_u32 err_state = cpld_sends_request_to_host(mctp_ctx_ptr, stored_uuid);

            if (err_state)
            {
                // Perform policy if there is any attestation error
                attestation_policy_handler(SPI_FLASH_BMC, err_state, afm_body->policy);
            }
        }
        afm_addr = incr_alt_u32_ptr(afm_addr, incr_size);
        number_of_devices--;
    }
}

/*
 * @brief This function checks for any incoming SPDM messages from mailbox fifo.
 *
 * On some platform, SCL was held low by slaves indefinitely. (only seen on some platform)
 * Clearing the messages in the FIFO actually solves the problem of SCL being held.
 * This way of receiving message will ignore the message if it is not MCTP_MESSAGE_TYPE.
 *
 * @param mctp_context_ptr : MCTP context
 *
 * @return none.
 */
static alt_u32 check_incoming_message_i2c(MCTP_CONTEXT* mctp_ctx_ptr)
{
    if (read_from_mailbox(MB_MCTP_PCIE_PACKET_AVAIL_AND_RX_PACKET_ERROR) & MCTP_PKT_AVAIL_BIT)
    {
#ifdef SPDM_DEBUG
    	// Set challenger type for any incoming
        mctp_ctx_ptr->challenger_type = PCIE;

        if (receive_i2c_message(mctp_ctx_ptr, 1))
        {
            // This is SPDM message
            cpld_checks_request_from_host(mctp_ctx_ptr);
        }
#endif
    }
    else if (read_from_mailbox(MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR) & MCTP_PKT_AVAIL_BIT)
    {
    	// Set challenger type for any incoming
        mctp_ctx_ptr->challenger_type = BMC;

        if (receive_i2c_message(mctp_ctx_ptr, 1))
        {
            // This is SPDM message
            cpld_checks_request_from_host(mctp_ctx_ptr);
        }
    }
    else if (read_from_mailbox(MB_MCTP_PCH_PACKET_AVAIL_AND_RX_PACKET_ERROR) & MCTP_PKT_AVAIL_BIT)
    {
    	// Set challenger type for any incoming
        mctp_ctx_ptr->challenger_type = PCH;

        if (receive_i2c_message(mctp_ctx_ptr, 1))
        {
            // This is SPDM message
            cpld_checks_request_from_host(mctp_ctx_ptr);
        }
    }
    return 0;
}

#endif

/*
 * @brief This function initializes MCTP context by clearing buffer.
 *
 * @param mctp_ctx_ptr: MCTP context
 *
 * @return none.
 */
static void init_mctp_context(MCTP_CONTEXT* mctp_ctx_ptr)
{
    // Clear buffer
    reset_buffer((alt_u8*)mctp_ctx_ptr, sizeof(MCTP_CONTEXT));
}

#endif /* EAGLESTREAM_INC_MCTP_FLOW_H_ */
