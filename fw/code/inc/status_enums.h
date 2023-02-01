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
 * @file status_enums.h
 * @brief Define encoding for various mailbox registers, such as platform state, last recovery reason and last panic reason.
 */

#ifndef EAGLESTREAM_INC_STATUS_ENUMS_H_
#define EAGLESTREAM_INC_STATUS_ENUMS_H_

/**
 * Define platform state
 */
typedef enum
{
    // CPLD Nios firmware T0 flow
    PLATFORM_STATE_CPLD_NIOS_WAITING_TO_START        = 0x01,
    PLATFORM_STATE_CPLD_NIOS_STARTED                 = 0x02,
    PLATFORM_STATE_ENTER_TMIN1                       = 0x03,
    PLATFORM_STATE_TMIN1_RESERVED1                   = 0x04,
    PLATFORM_STATE_TMIN1_RESERVED2                   = 0x05,
    PLATFORM_STATE_BMC_FLASH_AUTHENTICATION          = 0x06,
    PLATFORM_STATE_PCH_FLASH_AUTHENTICATION          = 0x07,
    PLATFORM_STATE_AUTHENTICATION_FAILED_LOCKDOWN    = 0x08,
    PLATFORM_STATE_ENTER_T0                          = 0x09,
    // Timed Boot Progress
    PLATFORM_STATE_T0_BMC_BOOTED                     = 0x0A,
    PLATFORM_STATE_T0_ME_BOOTED                      = 0x0B,
    PLATFORM_STATE_T0_ACM_BOOTED                     = 0x0C,
    PLATFORM_STATE_T0_BIOS_BOOTED                    = 0x0D,
    PLATFORM_STATE_T0_BOOT_COMPLETE                  = 0x0E,
    // Update event
    PLATFORM_STATE_PCH_FW_UPDATE                     = 0x10,
    PLATFORM_STATE_BMC_FW_UPDATE                     = 0x11,
    PLATFORM_STATE_CPLD_UPDATE                       = 0x12,
    PLATFORM_STATE_CPLD_UPDATE_IN_RECOVERY_MODE      = 0x13,
    PLATFORM_STATE_PCH_FV_SEAMLESS_UPDATE            = 0x14,
    PLATFORM_STATE_PCH_FV_SEAMLESS_UPDATE_DONE       = 0x15,
    // Recovery
    PLATFORM_STATE_TMIN1_FW_RECOVERY                 = 0x40,
    PLATFORM_STATE_TMIN1_FORCED_ACTIVE_FW_RECOVERY   = 0x41,
    PLATFORM_STATE_WDT_TIMEOUT_RECOVERY              = 0x42,
    PLATFORM_STATE_CPLD_RECOVERY_IN_RECOVERY_MODE    = 0x43,
    // PIT
    PLATFORM_STATE_PIT_L1_LOCKDOWN                   = 0x44,
    PLATFORM_STATE_PIT_L2_FW_SEALED                  = 0x45,
    PLATFORM_STATE_PIT_L2_PCH_HASH_MISMATCH_LOCKDOWN = 0x46,
    PLATFORM_STATE_PIT_L2_BMC_HASH_MISMATCH_LOCKDOWN = 0x47,
} STATUS_PLATFORM_STATE_ENUM;

/**
 * Define the value indicating last firmware recovery reason
 */
typedef enum
{
    LAST_RECOVERY_PCH_ACTIVE                = 0x1,
    LAST_RECOVERY_PCH_RECOVERY              = 0x2,
    LAST_RECOVERY_ME_LAUNCH_FAIL            = 0x3,
    LAST_RECOVERY_ACM_LAUNCH_FAIL           = 0x4,
    LAST_RECOVERY_IBB_LAUNCH_FAIL           = 0x5,
    LAST_RECOVERY_OBB_LAUNCH_FAIL           = 0x6,
    LAST_RECOVERY_BMC_ACTIVE                = 0x7,
    LAST_RECOVERY_BMC_RECOVERY              = 0x8,
    LAST_RECOVERY_BMC_LAUNCH_FAIL           = 0x9,
    LAST_RECOVERY_FORCED_ACTIVE_FW_RECOVERY = 0xA,
    LAST_RECOVERY_BMC_ATTESTATION_FAILED    = 0XB,
    LAST_RECOVERY_CPU_ATTESTATION_FAILED    = 0XC,
} STATUS_LAST_RECOVERY_ENUM;

/**
 * Define the value indicating last Panic reason
 */
typedef enum
{
    LAST_PANIC_DEFAULT                    = 0x00,
    LAST_PANIC_PCH_UPDATE_INTENT          = 0x01,
    LAST_PANIC_BMC_UPDATE_INTENT          = 0x02,
    LAST_PANIC_BMC_RESET_DETECTED         = 0x03,
    LAST_PANIC_BMC_WDT_EXPIRED            = 0x04,
    LAST_PANIC_ME_WDT_EXPIRED             = 0x05,
    LAST_PANIC_ACM_BIOS_WDT_EXPIRED       = 0x06,
    LAST_PANIC_RESERVED_1                 = 0x07,
    LAST_PANIC_RESERVED_2                 = 0x08,
    LAST_PANIC_ACM_BIOS_AUTH_FAILED       = 0x09,
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    LAST_PANIC_ATTESTATION_FAIURE         = 0x0A,
#endif
} STATUS_LAST_PANIC_ENUM;

/**
 * Define the value indicating major error code observed on the system
 * There are now three general error code for secure coding purposes which is associated with memcpy_s
 */
typedef enum
{
    MAJOR_ERROR_BMC_AUTH_FAILED          = 0x01,
    MAJOR_ERROR_PCH_AUTH_FAILED          = 0x02,
    /* BMC, PCH or CPLD update failed */
    MAJOR_ERROR_UPDATE_FAILED            = 0x03,
    MAJOR_ERROR_MEMCPY_S_FAILED          = 0x10,

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

    MAJOR_ERROR_ATTESTATION_MEAS_VAL_MISMATCH    = 0x05,
    MAJOR_ERROR_ATTESTATION_CHALLENGE_TIMEOUT    = 0x06,
    MAJOR_ERROR_SPDM_PROTOCOL_ERROR              = 0x07,
	/* Failure to send messages */
    MAJOR_ERROR_COMMUNICATION_FAIL               = 0x08,

#endif
} STATUS_MAJOR_ERROR_ENUM;

/**
 * Define the value indicating minor error code observed on the system.
 * This set of minor code is associated with authentication failure.
 * Hence, this is paired with the MAJOR_ERROR_BMC_AUTH_FAILED and MAJOR_ERROR_PCH_AUTH_FAILED.
 */
typedef enum
{
    MINOR_ERROR_AUTH_ACTIVE                 = 0x01,
    MINOR_ERROR_AUTH_RECOVERY               = 0x02,
    MINOR_ERROR_AUTH_ACTIVE_AND_RECOVERY    = 0x03,
    MINOR_ERROR_AUTH_ALL_REGIONS            = 0x04,
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    MINOR_ERROR_AFM_AUTH_ACTIVE                 = 0x05,
    MINOR_ERROR_AFM_AUTH_RECOVERY               = 0x06,
    MINOR_ERROR_AFM_AUTH_ACTIVE_AND_RECOVERY    = 0x07,
    MINOR_ERROR_AFM_AUTH_ALL_REGIONS            = 0x08,
#endif
} STATUS_MINOR_ERROR_AUTH_ENUM;

/**
 * Define the value indicating minor error code observed on the system.
 * This set of minor code is associated with bmc/pch/cpld update failure.
 * Hence, this is paired with the MAJOR_ERROR_UPDATE_FAILED.
 */
typedef enum
{
    MINOR_ERROR_INVALID_UPDATE_INTENT                    = 0x01,
    MINOR_ERROR_INVALID_SVN                              = 0x02,
    MINOR_ERROR_AUTHENTICATION_FAILED                    = 0x03,
    MINOR_ERROR_EXCEEDED_MAX_FAILED_ATTEMPTS             = 0x04,
    MINOR_ERROR_ACTIVE_FW_UPDATE_NOT_ALLOWED             = 0x05,
    MINOR_ERROR_RECOVERY_FW_UPDATE_AUTH_FAILED           = 0x06,
    MINOR_ERROR_AFM_UPDATE_NOT_ALLOWED                   = 0x07,
    MINOR_ERROR_UNKNOWN_AFM                              = 0x08,

#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED
    MINOR_ERROR_UNKNOWN_FV_TYPE                          = 0x10,
    MINOR_ERROR_AUTH_FAILED_AFTER_SEAMLESS_UPDATE        = 0x11,
#endif
} STATUS_MINOR_ERROR_FW_CPLD_UPDATE_ENUM;

/**
 *
 * Define the value indicating minor error code observed on the system.
 * This set of minor error code is associated with error caught by custom memcpy_s function.
 * Hence, this is paired with the MAJOR_ERROR_MEMCPY_S_FAILED.
 *
 */
typedef enum
{
    MINOR_ERROR_INVALID_POINTER          = 0x01,
    MINOR_ERROR_EXCESSIVE_DATA_SIZE      = 0x02,
    MINOR_ERROR_EXCESSIVE_DEST_SIZE      = 0x03,
} STATUS_MINOR_ERROR_MEMCPY_S_ENUM;

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
/**
 *
 * Define the value indicating minor error code observed on the system.
 * This set of minor error code is associated with error caught during attestation for individual message.
 * Hence, this is paired with the MAJOR_ERROR_ATTESTATION_MEAS_VAL_MISMATCH,
 * MAJOR_ERROR_ATTESTATION_CHALLENGE_TIMEOUT and MAJOR_ERROR_SPDM_PROTOCOL_ERROR  .
 *
 */
typedef enum
{
    MINOR_ERROR_SPDM_CONNECTION       = 0x01,
    MINOR_ERROR_SPDM_DIGEST           = 0x02,
    MINOR_ERROR_SPDM_CERTIFICATE      = 0x03,
    MINOR_ERROR_SPDM_CHALLENGE        = 0x04,
    MINOR_ERROR_SPDM_MEASUREMENT      = 0x05,
} STATUS_MINOR_ERROR_ATTESTATION_MSG_ENUM;
#endif

#endif /* EAGLESTREAM_INC_STATUS_ENUMS_H_ */
