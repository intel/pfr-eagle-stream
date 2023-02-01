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
 * @file spi_flash_state.h
 * @brief Track the state of SPI flash devices.
 */

#ifndef EAGLESTREAM_INC_SPI_FLASH_STATE_H_
#define EAGLESTREAM_INC_SPI_FLASH_STATE_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "spi_common.h"

/**
 * Define the state that BMC/PCH flash may be in
 */
typedef enum
{
    SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK            = 0b1,
    SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK         = 0b10,
    SPI_FLASH_STATE_HAS_PENDING_RECOVERY_FW_UPDATE_MASK  = 0b100,
    SPI_FLASH_STATE_REQUIRE_WDT_RECOVERY_MASK            = 0b1000,
    SPI_FLASH_STATE_READY_FOR_CPLD_RECOVERY_UPDATE_MASK  = 0b100000,
//#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    SPI_FLASH_STATE_READY_FOR_ATTESTATION_MASK           = 0b10000,
    SPI_FLASH_STATE_HAS_PENDING_RECOVERY_AFM_UPDATE_MASK = 0b1000000,
    SPI_FLASH_STATE_AFM_RECOVERY_FAILED_AUTH_MASK        = 0b10000000,
    SPI_FLASH_STATE_ATTESTATION_REQUIRE_RECOVERY_MASK    = 0b100000000,
    SPI_FLASH_STATE_ATTESTATION_LOCKDOWN_RESET_MASK      = 0b1000000000,

    // AFM state
    SPI_FLASH_STATE_CLEAR_AFM_AUTH_RESULT                = SPI_FLASH_STATE_AFM_RECOVERY_FAILED_AUTH_MASK | SPI_FLASH_STATE_READY_FOR_ATTESTATION_MASK,
//#endif
    SPI_FLASH_STATE_POST_SEAMLESS_UPDATE_MASK            = 0b10000000000,
    // Some combinations of the above
    SPI_FLASH_STATE_CLEAR_AUTH_RESULT                    = SPI_FLASH_STATE_RECOVERY_FAILED_AUTH_MASK | SPI_FLASH_STATE_ALL_REGIONS_FAILED_AUTH_MASK,
} SPI_FLASH_STATE_MASK_ENUM;

// Static variables to track the state of flash devices
static alt_u32 bmc_flash_state = 0;
static alt_u32 pch_flash_state = 0;

/******************************************************
 *
 * Helper functions to work with the static variables
 *
 ******************************************************/

/**************************
 * State settings
 **************************/

/**
 * @brief Clear spi flash state for authentication, update and recovery
 *
 * @param spi_flash_type : BMC or PCH
 * @param state : spi flash state
 *
 * @return none
 */
static void clear_spi_flash_state(SPI_FLASH_TYPE_ENUM spi_flash_type, SPI_FLASH_STATE_MASK_ENUM state)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        bmc_flash_state &= ~((alt_u32) state);
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        pch_flash_state &= ~((alt_u32) state);
    }
}

/**
 * @brief Check spi flash state for authentication, update and recovery
 *
 * @param spi_flash_type : BMC or PCH
 * @param state : spi flash state
 *
 * @return none
 */
static alt_u32 check_spi_flash_state(SPI_FLASH_TYPE_ENUM spi_flash_type, SPI_FLASH_STATE_MASK_ENUM state)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        return (bmc_flash_state & state) == state;
    }
    // spi_flash_type == SPI_FLASH_PCH
    return (pch_flash_state & state) == state;
}

/**
 * @brief Set spi flash state for authentication, update and recovery
 *
 * @param spi_flash_type : BMC or PCH
 * @param state : spi flash state
 *
 * @return none
 */
static void set_spi_flash_state(SPI_FLASH_TYPE_ENUM spi_flash_type, SPI_FLASH_STATE_MASK_ENUM state)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        bmc_flash_state |= state;
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        pch_flash_state |= state;
    }
}

#endif /* EAGLESTREAM_INC_SPI_FLASH_STATE_H_ */
