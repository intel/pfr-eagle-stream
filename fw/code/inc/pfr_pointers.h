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
 * @file pfr_pointers.h
 * @brief This header facilitates retrieval of various pointer types in this PFR system.
 */

#ifndef EAGLESTREAM_INC_PFR_POINTERS_H
#define EAGLESTREAM_INC_PFR_POINTERS_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "gen_gpo_controls.h"
#include "gen_smbus_relay_config.h"
#include "keychain.h"
#include "pbc.h"
#include "pfm.h"
#include "ufm.h"
#include "utils.h"


/**
 * @brief This function increments an alt_u32 pointer by a number of bytes.
 * Since alt_u32 pointer is word (4 bytes) aligned, nbytes must be multiple of 4.
 *
 * @param ptr pointer to be incremented
 * @param n_bytes number of bytes to increment
 * @return incremented pointer
 */
static PFR_ALT_INLINE alt_u32* PFR_ALT_ALWAYS_INLINE incr_alt_u32_ptr(alt_u32* ptr, alt_u32 n_bytes)
{
    PFR_ASSERT((n_bytes % 4) == 0);
    return ptr + (n_bytes >> 2);
}

/**
 * @brief Return a pointer to the start of the SPI flash memory space.
 *
 * @return a pointer to a SPI address
 */
static PFR_ALT_INLINE alt_u32* PFR_ALT_ALWAYS_INLINE get_spi_flash_ptr()
{
#ifdef USE_SYSTEM_MOCK
    // Use the pointer to the flash memory mock in unittests
    return SYSTEM_MOCK::get()->get_x86_ptr_to_spi_flash();
#endif
    return __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_SPI_FILTER_AVMM_BRIDGE_BASE, 0);
}

/**
 * @brief Return a pointer to an address in the SPI memory range, given an offset in the range.
 *
 * @param offset an offset within the SPI memory range
 * @return a pointer to a SPI address
 */
static PFR_ALT_INLINE alt_u32* PFR_ALT_ALWAYS_INLINE get_spi_flash_ptr_with_offset(alt_u32 offset)
{
    return incr_alt_u32_ptr(get_spi_flash_ptr(), offset);
}

/**
 * @brief Return a pointer to an address in the UFM memory range.
 *
 * @param offset an offset within the UFM memory range
 * @return a pointer to a UFM address
 */
static alt_u32* get_ufm_ptr_with_offset(alt_u32 offset)
{
#ifdef USE_SYSTEM_MOCK
    // Retrieve the mock ufm data from system_mock in unittest environment
    return incr_alt_u32_ptr(SYSTEM_MOCK::get()->get_ufm_data_ptr(), offset);
#endif
    return incr_alt_u32_ptr(U_UFM_DATA_ADDR, offset);
}

/**
 * @brief Return the pointer to starting address of the UFM PFR data.
 * By default, Nios firmware saves the persistent data in the first page of the UFM,
 * according to the UFM_PFR_DATA structure. This data includes the provisioned data
 * (root key hash, pit password, etc), SVN policy, CSK policy and others.
 *
 * Note that get_ufm_ptr_with_offset() function can be used in this function,
 * but it would cost a few hundred bytes of code more somehow.
 *
 * @return UFM_PFR_DATA* pointer to the UFM PFR data
 */
static PFR_ALT_INLINE UFM_PFR_DATA* PFR_ALT_ALWAYS_INLINE get_ufm_pfr_data()
{
    PFR_ASSERT(UFM_PFR_DATA_OFFSET == 0);
#ifdef USE_SYSTEM_MOCK
    // Retrieve the mock ufm data from system_mock in unittest environment
    return (UFM_PFR_DATA*) incr_alt_u32_ptr(SYSTEM_MOCK::get()->get_ufm_data_ptr(), UFM_PFR_DATA_OFFSET);
#endif
    return (UFM_PFR_DATA*) (U_UFM_DATA_ADDR + UFM_PFR_DATA_OFFSET / 4);
}

/**
 * @brief Return address of spi active region.
 *
 * @param spi_flash_type : BMC or PCH
 * @return active spi address
 */
static alt_u32 get_active_region_offset(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        return get_ufm_pfr_data()->bmc_active_pfm;
    }
    // spi_flash_type == SPI_FLASH_PCH
    return get_ufm_pfr_data()->pch_active_pfm;
}

/**
 * @brief Return address of spi recovery region.
 *
 * @param spi_flash_type : BMC or PCH
 * @return recovery spi address
 */
static alt_u32 get_recovery_region_offset(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        return get_ufm_pfr_data()->bmc_recovery_region;
    }
    // spi_flash_type == SPI_FLASH_PCH
    return get_ufm_pfr_data()->pch_recovery_region;
}

/**
 * @brief Return address of spi staging region.
 *
 * @param spi_flash_type : BMC or PCH
 * @return staging spi address
 */
static alt_u32 get_staging_region_offset(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        return get_ufm_pfr_data()->bmc_staging_region;
    }
    // spi_flash_type == SPI_FLASH_PCH
    return get_ufm_pfr_data()->pch_staging_region;
}

/**
 * @brief Return a pointer to pfm address in spi.
 *
 * @param spi_flash_type : BMC or PCH
 * @return a pointer to active PFM address
 */
static alt_u32* get_spi_active_pfm_ptr(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    return get_spi_flash_ptr_with_offset(get_active_region_offset(spi_flash_type));
}

/**
 * @brief Return a pointer to pfm address in spi.
 *
 * @param spi_flash_type : BMC or PCH
 * @return a pointer to recovery PFM address
 */
static alt_u32* get_spi_recovery_region_ptr(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    return get_spi_flash_ptr_with_offset(get_recovery_region_offset(spi_flash_type));
}

/**
 * @brief Return a pointer to pfm address in spi.
 *
 * @param spi_flash_type : BMC or PCH
 * @return a pointer to staging PFM address
 */
static alt_u32* get_spi_staging_region_ptr(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    return get_spi_flash_ptr_with_offset(get_staging_region_offset(spi_flash_type));
}

/**
 * @brief Return a pointer to afm address in spi.
 *
 * @param spi_flash_type : BMC or PCH
 * @return a pointer to active AFM address
 */
static alt_u32* get_spi_active_afm_ptr(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    return get_spi_flash_ptr_with_offset(get_staging_region_offset(spi_flash_type) + BMC_STAGING_REGION_AFM_ACTIVE_OFFSET);
}

/**
 * @brief Return a pointer to afm address in spi.
 *
 * @param spi_flash_type : BMC or PCH
 * @return a pointer to recovery AFM address
 */
static alt_u32* get_spi_recovery_afm_ptr(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    return get_spi_flash_ptr_with_offset(get_staging_region_offset(spi_flash_type) + BMC_STAGING_REGION_AFM_RECOVERY_OFFSET);
}

/**
 * @brief Return a pointer to cert address in ufm memory range.
 *
 * @return a pointer to cert in UFM address
 */
static alt_u32* get_ufm_cpld_cert_ptr()
{
    return get_ufm_ptr_with_offset(UFM_CPLD_STORED_CERTIFICATE);
}

/**
 * @brief Return a pointer to afm address in spi.
 *
 * @param spi_flash_type : BMC or PCH
 * @return a pointer to capsule AFM address
 */
static alt_u32* get_capsule_afm_ptr(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    return get_spi_flash_ptr_with_offset(get_staging_region_offset(spi_flash_type) + SIGNATURE_SIZE + SIGNATURE_SIZE);
}

/**
 * @brief Return a pointer to afm address in spi.
 *
 * @param spi_flash_type : BMC or PCH
 * @return a pointer to capsule first AFM address
 */
static alt_u32* get_capsule_first_afm_ptr(alt_u32 capsule_afm_addr)
{
    alt_u32* capsule_afm_sig = get_spi_flash_ptr_with_offset(capsule_afm_addr + SIGNATURE_SIZE);
    // Get the signature of first AFM
    return get_spi_flash_ptr_with_offset(capsule_afm_addr + SIGNATURE_SIZE + SIGNATURE_SIZE +
                           ((KCH_BLOCK0*) capsule_afm_sig)->pc_length);
}

/**
 * @brief Given the bus ID, return the pointer to the SMBus relay AvMM base address in the system.
 *
 * @param bus_id SMBus ID
 *
 * @return the pointer to the SMBus relay AvMM base address
 */
static alt_u32* get_relay_base_ptr(alt_u32 bus_id)
{
    PFR_ASSERT(NUM_RELAYS == 3);

#ifdef USE_SYSTEM_MOCK
    // Use the pointer to the SMBus relay mock in unittests
    return SYSTEM_MOCK::get()->smbus_relay_mock_ptr->get_cmd_enable_memory_for_smbus(bus_id);
#endif

    if (bus_id == 1)
    {
        return __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_RELAY1_AVMM_BRIDGE_BASE, 0);
    }
    else if (bus_id == 2)
    {
        return __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_RELAY2_AVMM_BRIDGE_BASE, 0);
    }
    else if (bus_id == 3)
    {
        return __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_RELAY3_AVMM_BRIDGE_BASE, 0);
    }
    return 0;
}

#endif /* EAGLESTREAM_INC_PFR_POINTERS_H */
