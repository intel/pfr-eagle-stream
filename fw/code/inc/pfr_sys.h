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
 * @file pfr_sys.h
 * @brief Common header for CPLD RoT Nios II code.
 */

#ifndef EAGLESTREAM_INC_PFR_SYS_H_
#define EAGLESTREAM_INC_PFR_SYS_H_

// Include all BSP headers. No other files should include the BSP headers so that
// we can mock them as required for unit testing

#include "io.h"
#include "system.h"
#include "alt_types.h"

#include "gen_smbus_relay_config.h"

// PFR system assert. Can be compiled out and handled separately for unit testing
#ifdef PFR_ENABLE_ASSERT
#define PFR_ASSERT(x) NOT_YET_IMPLEMENTED
#define PFR_INTERNAL_ERROR(msg) NOT_YET_IMPLEMENTED
#define PFR_INTERNAL_ERROR_VARG(format, ...) NOT_YET_IMPLEMENTED
#else
#ifndef PFR_NO_BSP_DEP
// Define the macro to do nothing
#define PFR_ASSERT(x)
#define PFR_INTERNAL_ERROR(msg)
#define PFR_INTERNAL_ERROR_VARG(format, ...)
#endif
#endif

// Inlining control macros. When in debug mode disable the forced inlining
#define PFR_ALT_DONT_INTLINE __attribute__((noinline))
#ifndef PFR_DEBUG_MODE
#undef PFR_ALT_INLINE
#undef PFR_ALT_ALWAYS_INLINE
#define PFR_ALT_INLINE ALT_INLINE
#define PFR_ALT_ALWAYS_INLINE ALT_ALWAYS_INLINE
#else
#define PFR_ALT_INLINE
#define PFR_ALT_ALWAYS_INLINE
#endif

/*******************************************************************
 * Address Calculation Macros
 *******************************************************************/
#define __IO_CALC_ADDRESS_NATIVE_ALT_U32(BASE, REGNUM) \
		((alt_u32*) (((alt_u8*) BASE) + ((REGNUM) * (SYSTEM_BUS_WIDTH / 8))))

// To create alt_u32*
#define U_GPO_1_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_GPO_1_BASE, 0)
#define U_GPI_1_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_GPI_1_BASE, 0)

/*******************************************************************
 * CPLD Design Info
 *******************************************************************/
#define CPLD_ROT_RELEASE_VERSION 2

/*******************************************************************
 * Watchdog Timer Settings
 * Set all timeout values to 25 minutes
 * HW Timer resolution is at 20ms
 *******************************************************************/
// Timeout value for BMC WDT
#define WD_BMC_TIMEOUT_VAL 0x124F8

// Timeout values for ME WDT
#define WD_ME_TIMEOUT_VAL 0x124F8

// Timeout values for ACM WDT
#define WD_ACM_TIMEOUT_VAL 0x124F8

// Timeout values for BIOS WDT
#define WD_IBB_TIMEOUT_VAL 0x124F8
#define WD_OBB_TIMEOUT_VAL 0x124F8

// When there's BTG (ACM) authentication failure, ACM will pass that information to ME firmware.
// After ME performs clean up, it shuts down the system. Then CPLD can perform the WDT recovery.
// This 2s timeout value is the time needs for ACM/ME communication and ME's clean up
#define WD_ACM_AUTH_FAILURE_WAIT_TIME_VAL 40

/********************************************************************
 *
 * Timer settings for SPDM service
 *
 ********************************************************************/
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

// To re-attest host, set timer to 10 minutes iteration
#define CPLD_REATTEST_HOST_TIME 0x7530

#ifdef PFR_SPDM_DEBUG

// Attest every minute to shorten test time on platform
#undef CPLD_REATTEST_HOST_TIME
#define CPLD_REATTEST_HOST_TIME 0xBB8

#endif

// Max RTT for non-crypto operation is is 40ms
#define CPLD_SPDM_RTT_NON_CRYPTO_MAX_TIMING_UNIT 2

// Min RTT for non-crypto operation is is 20ms
#define CPLD_SPDM_RTT_NON_CRYPTO_MIN_TIMING_UNIT 1

// Max RTT for crypto operation is 1.5s
#define CPLD_SPDM_RTT_CRYPTO_MAX_TIMING_UNIT 0x48

// Min RTT for crypto operation is 20ms
#define CPLD_SPDM_RTT_CRYPTO_MIN_TIMING_UNIT 1

#define CPLD_SPDM_ST1_TIMING_UNIT 5

#define CPLD_SPDM_T1_TIMING_UNIT (CPLD_SPDM_RTT_NON_CRYPTO_MAX_TIMING_UNIT + CPLD_SPDM_ST1_TIMING_UNIT)

// Max exponent value for cryptography operation
// TODO: must calculate the size of message for crypto operation and define the exponent.
// If the message size is longer than expected, re-calculate the max ct exponent based on the initially chosen size.
#define CPLD_AS_RESPONDER_SPDM_CT_EXPONENT 20

#endif

/*******************************************************************
 * Platform Components
 *******************************************************************/
typedef enum
{
    SPI_FLASH_BMC = 1,
    SPI_FLASH_PCH = 2,
} SPI_FLASH_TYPE_ENUM;

#define BMC_SPI_FLASH_SIZE 0x8000000
#define PCH_SPI_FLASH_SIZE 0x4000000

typedef enum
{
    SPI_FLASH_MICRON = 0x0021ba20,
    SPI_FLASH_MACRONIX = 0x001a20c2,
} SPI_FLASH_ID_ENUM;

typedef enum
{
    // opcode = 0xEB, dummy cycles = 0xA
    SPI_FLASH_QUAD_READ_PROTOCOL_MICRON = 0x00000AEB,
    // opcode = 0xEB, dummy cycles = 0x6
    SPI_FLASH_QUAD_READ_PROTOCOL_MACRONIX = 0x000006EB,
} QUAD_IO_PROTOCOL;

typedef enum
{
    RESTORE_QE_BIT = 3,
    TRACK_QE_BIT = 2,
    SET_QE_BIT = 1,
    UNSET_QE_BIT = 0,
}MACRONIX_QUAD_PROTOCOL_ENUM;

typedef enum
{
    BMC  = 2,
    PCH  = 1,
    PCIE = 0,
}ATTESTATION_HOST;

/*******************************************************************
 * BMC flash offsets
 *******************************************************************/
// Offsets for different update capsule
// These offsets are relative to staging region
#define BMC_STAGING_REGION_BMC_UPDATE_CAPSULE_OFFSET 0
#define BMC_STAGING_REGION_PCH_UPDATE_CAPSULE_OFFSET 0x2000000
#define BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET 0x3000000
#define BMC_STAGING_REGION_AFM_ACTIVE_OFFSET 0x3400000
#define BMC_STAGING_REGION_AFM_RECOVERY_OFFSET 0x3420000

// This is a hard-coded value reserved for AFM
#define BMC_STAGING_REGION_ACTIVE_AFM_OFFSET 0x7E00000
#define BMC_STAGING_REGION_RECOVERY_AFM_OFFSET 0X7E20000

// Reserved area for CPLD recovery image
// This is a hard-coded value
#define BMC_CPLD_RECOVERY_IMAGE_OFFSET 0x7F00000
#define BMC_CPLD_RECOVERY_REGION_SIZE 0x100000
#define BMC_CPLD_RECOVERY_LOCATION_IN_WE_MEM (BMC_CPLD_RECOVERY_IMAGE_OFFSET >> 19)

/*******************************************************************
 * UFM data
 *******************************************************************/
#define U_UFM_DATA_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_UFM_DATA_BASE, 0)

// UFM PFR data (e.g. the provisioned data, svn and csk policies)
#define UFM_PFR_DATA_OFFSET U_UFM_DATA_SECTOR1_START_ADDR
//#define UFM_CPLD_UPDATE_STATUS_OFFSET U_UFM_DATA_SECTOR2_START_ADDR
#define UFM_CPLD_UPDATE_STATUS_OFFSET 0x2000

#define UFM_PFR_DATA_SECTOR_ID 0b001
#define UFM_CPLD_UPDATE_STATUS_SECTOR_ID 0b010

// CFM
typedef enum
{
    CPLD_CFM0 = 0,
    CPLD_CFM1 = 1,
} CPLD_CFM_TYPE_ENUM;

// The Recovery Manager CPLD image is stored in CFM0
// CFM0 locates in Sector 5 of the UFM
#define CMF0_SECTOR_ID 0b101
#define UFM_CPLD_ROM_IMAGE_OFFSET U_UFM_DATA_SECTOR5_START_ADDR
// CPLD image is the entire size of CFM0
#define UFM_CPLD_ROM_IMAGE_LENGTH (U_UFM_DATA_SECTOR5_END_ADDR - U_UFM_DATA_SECTOR5_START_ADDR + 1)

#define UFM_CPLD_UNTOUCHED_ROM_IMAGE_LENGTH (U_UFM_DATA_SECTOR5_END_ADDR - U_UFM_DATA_SECTOR5_START_ADDR + 1 - 64)

#define UFM_CPLD_PUBLIC_KEY_0 0x4000
// Total metadata size in bytes
#define UFM_CPLD_METADATA_256_SIZE 224
#define UFM_CPLD_METADATA_384_SIZE 336

#define UFM_AFM_1 0x6000
#define UFM_AFM_MEASUREMENTS UFM_AFM_1 + 0x100

#define UFM_CPLD_PAGE_SIZE U_UFM_DATA_BYTES_PER_PAGE

#ifdef ENABLE_ATTESTATION_WITH_ECDSA_256

#define UFM_CPLD_PUB_KEY_SIZE 64
// Collection of metadata under a page
// Allow 64 bytes of data for public key -256
#define UFM_CPLD_CFM1_STORED_HASH UFM_CPLD_PUBLIC_KEY_0 + 64
// Allow 32 bytes of data for cfm1 hash
#define UFM_CPLD_CDI1 UFM_CPLD_CFM1_STORED_HASH + 32
// Allow 32 bytes of data for CDI1
#define UFM_CPLD_PRIVATE_KEY_1 UFM_CPLD_CDI1 + 32
// Allow 32 bytes of data for private key 1
#define UFM_CPLD_PUBLIC_KEY_1 UFM_CPLD_PRIVATE_KEY_1 + 32

#endif

#ifdef ENABLE_ATTESTATION_WITH_ECDSA_384

#define UFM_CPLD_PUB_KEY_SIZE 96
// Collection of metadata under a page
// Allow 96 bytes of data for public key -384
#define UFM_CPLD_CFM1_STORED_HASH UFM_CPLD_PUBLIC_KEY_0 + 96
// Allow 48 bytes of data for cfm1 hash
#define UFM_CPLD_CDI1 UFM_CPLD_CFM1_STORED_HASH + 48
// Allow 48 bytes of data for CDI1
#define UFM_CPLD_PRIVATE_KEY_1 UFM_CPLD_CDI1 + 48
// Allow 48 bytes of data for private key 1
#define UFM_CPLD_PUBLIC_KEY_1 UFM_CPLD_PRIVATE_KEY_1 + 48

#endif

#define UFM_CPLD_STORED_CERTIFICATE 0x5000

#define MAX_NO_OF_AFM_SUPPORTED 4

// The active CPLD image is stored in CFM1
// CFM1 locates in Sector 3 and 4 of the UFM
#define CMF1_TOP_HALF_SECTOR_ID 0b011
#define CMF1_BOTTOM_HALF_SECTOR_ID 0b100
#define UFM_CPLD_ACTIVE_IMAGE_OFFSET U_UFM_DATA_SECTOR3_START_ADDR
// CPLD image is the entire size of CFM1
#define UFM_CPLD_ACTIVE_IMAGE_LENGTH (U_UFM_DATA_SECTOR4_END_ADDR - U_UFM_DATA_SECTOR3_START_ADDR + 1)

// CFM0 UDS storage location
#define CFM0_UNIQUE_DEVICE_SECRET (U_UFM_DATA_SECTOR5_END_ADDR + 1 - 48 - 8)

// CFM1 breadcrumb
// Nios sets it to 0 if Nios is in the recovery due to a CPLD update, 0xFFFFFFFF otherwise
#define CFM1_BREAD_CRUMB (U_UFM_DATA_SECTOR5_START_ADDR - 4)

// CFM1 breadcrumb
// Nios sets it to 0 once UDS has been stored
#define CFM0_BREAD_CRUMB (U_UFM_DATA_SECTOR5_END_ADDR + 1 - 8)

/*******************************************************************
 * CPLD Update Setting
 *******************************************************************/
// External agent that is responsible for sending CPLD update intent
#define CPLD_UPDATE_AGENT SPI_FLASH_BMC

// Max size for a CPLD update capsule
// This size is selected because the offsets of CPLD images in BMC are 1MB apart.
#define MAX_CPLD_UPDATE_CAPSULE_SIZE 0x100000

// Image 1 & Image 2 has the same pre-defined size for all the Max10 devices.
// For example, on 10M16 device, both Image 1 & 2 are 264 KB (66 pages * 32 Kb/page)
// RPD file size is also the same as Image 1/2 size.
// Protected content of CPLD update capsule contains:
//    SVN (4 bytes) + RPD (with correct bit/byte ordering) + Paddings (124 bytes)
#define EXPECTED_CPLD_UPDATE_CAPSULE_PC_LENGTH (128 + UFM_CPLD_ACTIVE_IMAGE_LENGTH)

/*******************************************************************
 * Firmware Update Setting
 *******************************************************************/

// Max size for a BMC FW update capsule
// In BMC flash, there're 32MB allocated for BMC update capsule
#define MAX_BMC_FW_UPDATE_CAPSULE_SIZE 0x2000000
// Max size for a PCH FW update capsule
// In BMC flash, there're 16MB allocated for PCH update capsule
#define MAX_PCH_FW_UPDATE_CAPSULE_SIZE 0x1000000

// PFM/Recovery/Staging region size
// Maximum size for signed PFM is 64KB.
// In SPI maps, 64KB is reserved for signed PFM on PCH side and 128KB is reserved on BMC side.
#define SIGNED_PFM_MAX_SIZE 0x10000

#define SPI_FLASH_BMC_RECOVERY_SIZE 0x2000000
#define SPI_FLASH_PCH_RECOVERY_SIZE 0x1000000

#define SPI_FLASH_BMC_STAGING_SIZE  0x3600000 - BMC_CPLD_RECOVERY_REGION_SIZE
#define SPI_FLASH_PCH_STAGING_SIZE  0x1000000

/*******************************************************************
 * Firmware/CPLD Update Setting
 *******************************************************************/
// Max number of failed attempts for firmware/CPLD update
#define MAX_FAILED_UPDATE_ATTEMPTS_ALLOWED 10

/*******************************************************************
 * Memcpy_s macro
 ******************************************************************/
#define MEMCPY_S_INVALID_NULL_POINTER 1
#define MEMCPY_S_EXCESSIVE_DATA_SIZE 2
#define MEMCPY_S_INVALID_MAX_DEST_SIZE 3

/*******************************************************************
 * Crypto
 *******************************************************************/
#define SHA256_LENGTH 32
#define SHA384_LENGTH 48
#define SHA512_LENGTH 64

#define SHA256_HMAC_ENTROPY 48
#define SHA384_HMAC_ENTROPY 64

typedef enum
{
    CRYPTO_256_MODE = 0x00 << 28,
    CRYPTO_384_MODE = 0x01 << 28,
} CRYPTO_MODE;

// Crypto block size of sha in bytes
#define CRYPTO_BLOCK_SIZE_FOR_256 64 //0x40
#define CRYPTO_BLOCK_SIZE_FOR_384 128 //0x80

// Minimum leftover size to to pad first sha block in bits
#define CRYPTO_MINIMUM_LEFTOVER_BIT_256 (CRYPTO_BLOCK_SIZE_FOR_256 + 8)
#define CRYPTO_MINIMUM_LEFTOVER_BIT_384 (CRYPTO_BLOCK_SIZE_FOR_384 + 8)

// Least significant word of a sha block
#define CRYPTO_LEAST_SIGNIFICANT_4_BYTES_384 124
#define CRYPTO_LEAST_SIGNIFICANT_4_BYTES_256 60

#define PFR_CRYPTO_LENGTH SHA256_LENGTH

typedef enum
{
    ENGAGE_DMA_UFM = 0x02,
    ENGAGE_DMA_SPI = 0x01,
    DISENGAGE_DMA  = 0x00,
}DMA_MODE;

// PFR Root Key Hash
// Nios computes the root key hash with:
// Root public X (little-endian byte order) + Root Public key Y (little-endian byte order)

// Nios can copy 4MB data from SPI flash to crypto block without petting the HW timer.
#define PFR_CRYPTO_SAFE_COPY_DATA_SIZE 0x400000

#define PFR_ROOT_KEY_DATA_SIZE_FOR_256 64
#define PFR_ROOT_KEY_DATA_SIZE_FOR_384 96

// Size in words
#define PFR_K0_XOR_IPAD_MAX_384_SIZE 96
#define PFR_K0_XOR_OPAD_IPAD_MAX_384_SIZE 44
// Size in words
#define PFR_K0_XOR_IPAD_MAX_256_SIZE 64
#define PFR_K0_XOR_OPAD_IPAD_MAX_256_SIZE 24

#define PFR_ENTROPY_EXTRACTION_384_SIZE 64
#define PFR_ENTROPY_EXTRACTION_256_SIZE 48

// Size of UDS in words
// Choosing 48 bytes of data will ensure compatibility for both sha-256 and sha-384 padding algorithm
#define UDS_WORD_SIZE 12

#define PFR_AFM_OFFSET 0

#define BMC_SLAVE_ADDRESS    2
#define PCH_SLAVE_ADDRESS    4
#define PCIE_SLAVE_ADDRESS   6
/*******************************************************************
 * SMBus Relay Settings
 *******************************************************************/
#define PFR_SYS_NUM_SMBUS_RELAYS 3

// Assume the command enable memory has the same size for all three relays
// This is done to save some code space.
#define SMBUS_RELAY_CMD_EN_MEM_NUM_WORDS (U_RELAY1_AVMM_BRIDGE_SPAN / 4)

// SMBus relays mapping for device address/bus ID/rule ID
static const alt_u8 smbus_device_addr[NUM_RELAYS][MAX_I2C_ADDRESSES_PER_RELAY] = RELAYS_I2C_ADDRESSES;

#endif /* EAGLESTREAM_INC_PFR_SYS_H_ */
