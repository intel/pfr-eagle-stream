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
 * @file initialization.h
 * @brief Functions used to initialize the system.
 */

#ifndef EAGLESTREAM_INC_INITIALIZATION_H
#define EAGLESTREAM_INC_INITIALIZATION_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "crypto.h"
#include "gen_gpi_signals.h"
#include "gen_gpo_controls.h"
#include "mailbox_utils.h"
#include "rfnvram_utils.h"
#include "smbus_relay_utils.h"
#include "spi_ctrl_utils.h"
#include "ufm_rw_utils.h"
#include "ufm_utils.h"
#include "utils.h"
#include "watchdog_timers.h"

/**
 * @brief Check if it's ready for Nios to start.
 * When the common core deasserts reset for both the PCH and BMC,
 * Nios FW is ready to start its execution.
 *
 * @return 1 if ready; 0, otherwise.
 */
static alt_u32 check_ready_for_nios_start()
{
    return check_bit(U_GPI_1_ADDR, GPI_1_cc_RST_RSMRST_PLD_R_N) &&
            check_bit(U_GPI_1_ADDR, GPI_1_cc_RST_SRST_BMC_PLD_R_N);
}

/**
 * @brief Initialize host mailbox register file
 * 
 * CPLD RoT static identifier, version and SVN registers are initialized here.
 * UFM provision/lock status is initialized with the information in ufm_status field
 * of UFM_PFR_DATA in ufm. 
 * CPLD hash is calculated and saved in the mailbox CPLD RoT Hash register as well. 
 * 
 */
static void initialize_mailbox()
{
    // Write the static identifier to mailbox
    write_to_mailbox(MB_CPLD_STATIC_ID, MB_CPLD_STATIC_ID_VALUE);

    // Write CPLD RoT Version
    write_to_mailbox(MB_CPLD_RELEASE_VERSION, CPLD_ROT_RELEASE_VERSION);

    // Write CPLD RoT SVN
    write_to_mailbox(MB_CPLD_SVN, get_ufm_svn(UFM_SVN_POLICY_CPLD));

    // Write UFM provision/lock/PIT status
    if (is_ufm_provisioned())
    {
        mb_set_ufm_provision_status(MB_UFM_PROV_UFM_PROVISIONED_MASK);
    }
    if (is_ufm_locked())
    {
        mb_set_ufm_provision_status(MB_UFM_PROV_UFM_LOCKED_MASK);
    }
    if (check_ufm_status(UFM_STATUS_PIT_L1_ENABLE_BIT_MASK))
    {
        mb_set_ufm_provision_status(MB_UFM_PROV_UFM_PIT_L1_ENABLED_MASK);
    }
    if (check_ufm_status(UFM_STATUS_PIT_L2_PASSED_BIT_MASK))
    {
        mb_set_ufm_provision_status(MB_UFM_PROV_UFM_PIT_L2_PASSED_MASK);
    }
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    if (check_ufm_status(UFM_STATUS_ENABLE_CPLD_TO_CHALLENGE))
    {
        mb_set_ufm_provision_status_2(MB_UFM_PROV_ATTESTATION_ENABLED);
    }
#endif

    // Generate hash of CPLD active image
    alt_u32 cpld_hash[SHA384_LENGTH / 4];
    alt_u32 sha_size = SHA256_LENGTH;
    CRYPTO_MODE crypto_mode = CRYPTO_256_MODE;
    alt_u8* cpld_hash_u8_ptr = (alt_u8*) cpld_hash;
#ifdef USE_384_HASH
    sha_size = SHA384_LENGTH;
    crypto_mode = CRYPTO_384_MODE;
#endif
    calculate_and_save_sha(cpld_hash, UFM_CPLD_ACTIVE_IMAGE_OFFSET, get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET), UFM_CPLD_ACTIVE_IMAGE_LENGTH, crypto_mode, DISENGAGE_DMA);
    // Report CPLD hash to mailbox
    for (alt_u32 byte_i = 0; byte_i < sha_size; byte_i++)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_BASE, MB_CPLD_HASH + byte_i, cpld_hash_u8_ptr[byte_i]);
    }
}

/**
 * @brief Initialize the PFR system. 
 *
 * Initialize the PFR system as required. At this point the design has reset,
 * and the nios has started running. 
 * Initialization includes the followings:
 * - Mailbox register file
 * - SMBus relays (disabled by default)
 * - SPI filters (disabled by default)
 * - Configure SPI master and SPI flash devices
 * - Enable write to the UFM
 * 
 * @see initialize_mailbox()
 */
static void initialize_system()
{
    // Initialize mailbox register file
    initialize_mailbox();

    // EXTRST# is the BMC external reset. Initializing it to the inactive state.
    // We use EXTRST# during a BMC-only reset so that we do not have to reset the host.
    set_bit(U_GPO_1_ADDR, GPO_1_RST_PFR_EXTRST_N);

    // Write enable the UFM
    ufm_enable_write();

    // Initialize SMBus relays
    // The SMBus filters are disabled. 
    // All SMBus commands are disabled in the command enable memories.
    init_smbus_relays();

    // Release SPI Flashes from reset
    set_bit(U_GPO_1_ADDR, GPO_1_RST_SPI_PFR_BMC_BOOT_N);
    set_bit(U_GPO_1_ADDR, GPO_1_RST_SPI_PFR_PCH_N);

    // Configure SPI master and SPI flash devices
    configure_spi_master_csr();

    if (!is_ufm_provisioned())
    {
        // Disable SPI filter, when system is unprovisioned.
        // By default, these GPO bits are initialized to 0, which enables the filters.
        set_bit(U_GPO_1_ADDR, GPO_1_BMC_SPI_FILTER_DISABLE);
        set_bit(U_GPO_1_ADDR, GPO_1_PCH_SPI_FILTER_DISABLE);

        // Drive i_relay_all_address to 1 during unprovision.
        // i_relay_all_address is a new signal which replaces RELAY_ALL_ADDRESSES parameter
        set_bit(U_GPO_1_ADDR, GPO_1_RELAY1_ALL_ADDRESSES);
        set_bit(U_GPO_1_ADDR, GPO_1_RELAY2_ALL_ADDRESSES);
        set_bit(U_GPO_1_ADDR, GPO_1_RELAY3_ALL_ADDRESSES);
    }
    else
    {
        // Drive i_relay_all_address back to 0 during provisioning
		// Note: Allow all address for now as the actual operation(opcode) that harm the platform
		// will be continue blocking. This reduce the debug effort as platform setup may choose
		// device that is not part of default supported slave address
        set_bit(U_GPO_1_ADDR, GPO_1_RELAY1_ALL_ADDRESSES);
        set_bit(U_GPO_1_ADDR, GPO_1_RELAY2_ALL_ADDRESSES);
        set_bit(U_GPO_1_ADDR, GPO_1_RELAY3_ALL_ADDRESSES);
    }
}

#endif /* EAGLESTREAM_INC_INITIALIZATION_H */
