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
 * @file platform_log.h
 * @brief Logging functions for various firmware stages/events (e.g. T0, recovery, T-1).
 */

#ifndef EAGLESTREAM_INC_PLATFORM_LOG_H_
#define EAGLESTREAM_INC_PLATFORM_LOG_H_

#include "pfr_sys.h"

#include "global_state.h"
#include "mailbox_utils.h"
#include "watchdog_timers.h"


// Tracking number of failed update attempts from BMC/PCH
static alt_u32 num_failed_fw_or_cpld_update_attempts = 0;

/**
 * @brief This function logs platform state to mailbox and
 * global state (which drives the 7-seg display content on platform).
 */
static void log_platform_state(const STATUS_PLATFORM_STATE_ENUM state)
{
    write_to_mailbox(MB_PLATFORM_STATE, (alt_u32) state);
    set_global_state(state);
}

/**
 * @brief This function logs major/minor error to mailbox
 */
static void log_errors(alt_u32 major_err, alt_u32 minor_err)
{
    write_to_mailbox(MB_MAJOR_ERROR_CODE, major_err);
    write_to_mailbox(MB_MINOR_ERROR_CODE, minor_err);
}

/**
 * @brief This function logs panic reason to mailbox
 */
static void log_panic(STATUS_LAST_PANIC_ENUM panic_reason)
{
    // Log the panic reason
    write_to_mailbox(MB_LAST_PANIC_REASON, (alt_u32) panic_reason);

    // Increment panic count
    write_to_mailbox(MB_PANIC_EVENT_COUNT, 1 + read_from_mailbox(MB_PANIC_EVENT_COUNT));
}

/**
 * @brief This function logs firmware update process to mailbox
 */
static void log_firmware_update(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        log_platform_state(PLATFORM_STATE_PCH_FW_UPDATE);
    }
    else
    {
        log_platform_state(PLATFORM_STATE_BMC_FW_UPDATE);
    }
}

/**
 * @brief This function logs recovery reason to mailbox
 */
static void log_recovery(STATUS_LAST_RECOVERY_ENUM last_recovery_reason)
{
    // Log the recovery reason
    write_to_mailbox(MB_LAST_RECOVERY_REASON, last_recovery_reason);

    // Increment recovery count
    write_to_mailbox(MB_RECOVERY_COUNT, read_from_mailbox(MB_RECOVERY_COUNT) + 1);
}

/**
 * @brief This function logs recovery reason to mailbox
 */
static void log_tmin1_recovery(STATUS_LAST_RECOVERY_ENUM last_recovery_reason)
{
    log_platform_state(PLATFORM_STATE_TMIN1_FW_RECOVERY);
    log_recovery(last_recovery_reason);
}

/**
 * @brief This function logs wdt recovery reason to mailbox
 */
static void log_wdt_recovery(STATUS_LAST_RECOVERY_ENUM last_recovery_reason, STATUS_LAST_PANIC_ENUM panic_reason)
{
    log_recovery(last_recovery_reason);
    log_panic(panic_reason);
}

/**
 * @brief This function logs recovery reason for active image to mailbox
 */
static void log_tmin1_recovery_on_active_image(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        log_tmin1_recovery(LAST_RECOVERY_BMC_ACTIVE);
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        log_tmin1_recovery(LAST_RECOVERY_PCH_ACTIVE);
    }
}

/**
 * @brief This function logs recovery reason for active recovery to mailbox
 */
static void log_tmin1_recovery_on_recovery_image(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        log_tmin1_recovery(LAST_RECOVERY_BMC_RECOVERY);
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        log_tmin1_recovery(LAST_RECOVERY_PCH_RECOVERY);
    }
}

/**
 * @brief log this failed update event and increment the counter that track failed update attempts.
 *
 * @param minor_err Minor error code to be posted in Mailbox Minor Error register
 * @param counter_incr The value to be added onto number of failed update attempts
 */
static void log_update_failure(alt_u32 minor_err, alt_u32 counter_incr)
{
    log_errors(MAJOR_ERROR_UPDATE_FAILED, minor_err);
    num_failed_fw_or_cpld_update_attempts += counter_incr;
}

/**
 * @brief Clear failed update attempts and major/minor error from previous update if any.
 */
static void clear_update_failure()
{
    if (read_from_mailbox(MB_MAJOR_ERROR_CODE) == MAJOR_ERROR_UPDATE_FAILED)
    {
        log_errors(0, 0);
    }
    num_failed_fw_or_cpld_update_attempts = 0;
}

/**
 * @brief log this failed auth to major and minor error
 *
 * @param spi_flash_type : BMC or PCH
 * @param minor_err Minor error code to be posted in Mailbox Minor Error register
 */
static void log_auth_failure(SPI_FLASH_TYPE_ENUM spi_flash_type, alt_u32 minor_err)
{
    if (spi_flash_type == SPI_FLASH_PCH)
    {
        log_errors(MAJOR_ERROR_PCH_AUTH_FAILED, minor_err);
    }
    else // spi_flash_type == SPI_FLASH_BMC
    {
        log_errors(MAJOR_ERROR_BMC_AUTH_FAILED, minor_err);
    }
}

/**
 * @brief Log boot complete status in T0 mode
 *
 * @param current_boot_state the status for the component that has just completed boot
 */
static void log_t0_timed_boot_complete_if_ready(const STATUS_PLATFORM_STATE_ENUM current_boot_state)
{
    if (is_timed_boot_done())
    {
        // If other components have finished booting, log timed boot complete status.
        log_platform_state(PLATFORM_STATE_T0_BOOT_COMPLETE);
    }
    else
    {
        // Otherwise, just log the this boot complete status
        log_platform_state(current_boot_state);
    }
}

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/**
 * @brief Clear failed attestation activity from previous challenge if any.
 */
static void clear_attestation_failure()
{
    alt_u32 attest_log = read_from_mailbox(MB_MAJOR_ERROR_CODE);
    if ((attest_log == MAJOR_ERROR_ATTESTATION_MEAS_VAL_MISMATCH) ||
        (attest_log == MAJOR_ERROR_ATTESTATION_CHALLENGE_TIMEOUT) ||
        (attest_log == MAJOR_ERROR_SPDM_PROTOCOL_ERROR))
    {
        log_errors(0, 0);
    }
}

#endif

#endif /* EAGLESTREAM_INC_PLATFORM_LOG_H_ */
