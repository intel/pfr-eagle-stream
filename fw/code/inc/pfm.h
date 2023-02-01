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
 * @file pfm.h
 * @brief Define PFM data structures, such as PFM SPI region definition and SMBus rule definition.
 */

#ifndef EAGLESTREAM_INC_PFM_H_
#define EAGLESTREAM_INC_PFM_H_

#include "pfr_sys.h"

#define SMBUS_RULE_DEF_TYPE 2
// Command whitelist contains 256 bits. 256 / 8 = 32 bytes.
#define SMBUS_NUM_BYTE_IN_WHITELIST 32

#define SPI_REGION_DEF_TYPE 1
#define SPI_REGION_DEF_MIN_SIZE 16
#define SPI_REGION_DEF_WITH_256_HASH_SIZE (SPI_REGION_DEF_MIN_SIZE + SHA256_LENGTH)
#define SPI_REGION_DEF_WITH_384_HASH_SIZE (SPI_REGION_DEF_MIN_SIZE + SHA384_LENGTH)
#define SPI_REGION_PROTECT_MASK_READ_ALLOWED 0b1
#define SPI_REGION_PROTECT_MASK_WRITE_ALLOWED 0b10
#define SPI_REGION_PROTECT_MASK_RECOVER_ON_FIRST_RECOVERY 0b100
#define SPI_REGION_PROTECT_MASK_RECOVER_ON_SECOND_RECOVERY 0b1000
#define SPI_REGION_PROTECT_MASK_RECOVER_ON_THIRD_RECOVERY 0b10000
#define SPI_REGION_PROTECT_MASK_RECOVER_BITS 0b11100

#define PFM_MAGIC 0x02b3ce1d
#define PFM_HASH_ALGO_SHA256_MASK 0b1
#define PFM_HASH_ALGO_SHA384_MASK 0b10
#define PFM_PADDING 0xFF
// PFM size - PFM body size
#define PFM_HEADER_SIZE 0x20
#define MAX_FVM_TYPE_NUMBER	9

#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED

#define FVM_ADDR_DEF_TYPE 3

#define PFM_CAPABILITY_DEF_TYPE 4
#define PFM_CAPABILITY_REVISION 0x1
#define PFM_CAPABILITY_POST_SEAMLESS_UPDATE_REBOOT_REQ_MASK 0b1

#define FVM_MAGIC 0xA8E7C2D4
// FVM size - FVM body size
#define FVM_HEADER_SIZE 0x20
//#define FVM_MAX_SUPPORTED_FV_TYPE 20

#endif /* PLATFORM_SEAMLESS_FEATURES_ENABLED */

typedef enum
{
    CRYPTO_CURVE_256 = 0,
    CRYPTO_CURVE_384 = 1,
} CRYPTO_CURVE_ENUM;

/*!
 * Platform firmware manifest SMBus rule definition
 */
typedef struct
{
    alt_u8 def_type;
    // Cannot use type alt_u32 for this reserved region since it crosses the word boundary.
    alt_u8 _reserved;
    alt_u8 _reserved2;
    alt_u8 _reserved3;
    alt_u8 _reserved4;
    alt_u8 bus_id;
    alt_u8 rule_id;
    alt_u8 device_addr;
    alt_u8 cmd_whitelist[SMBUS_NUM_BYTE_IN_WHITELIST];
} PFM_SMBUS_RULE_DEF;

/*!
 * Platform firmware manifest SPI region definition
 */
typedef struct
{
    alt_u8 def_type;
    alt_u8 protection_mask;
    alt_u16 hash_algorithm;
    alt_u32 _reserved;
    alt_u32 start_addr;
    alt_u32 end_addr;
    alt_u32 region_hash_start;
} PFM_SPI_REGION_DEF;

#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED

/*!
 * Platform firmware manifest FVM address definition.
 *
 * This structure is mapped directly to a binary. Use packing to prevent
 * compiler from adding padding to this struct.
 */
typedef struct __attribute__((__packed__))
{
    alt_u8 def_type;
    alt_u16 fv_type;
    alt_u8 _reserved;
    alt_u32 _reserved2;
    alt_u32 fvm_addr;
} PFM_FVM_ADDR_DEF;

/*!
 * Platform firmware manifest Capability structure
 */
typedef struct
{
    alt_u8 def_type;
    alt_u16 _reserved;
    alt_u8 revision;
    alt_u16 size;
    // Seamless Package Version - Start
    alt_u8 major_version;
    alt_u8 minor_version;
    alt_u8 release_version;
    alt_u8 hotfix;
    // Seamless Package Version - End
    alt_u32 seamless_layout_id;
    alt_u32 seamless_postupdate_action;
    alt_u32 _reserved1[6];
    alt_u16 _reserved2;
    alt_u32 seamless_fw_des[5];
} PFM_CAPABILITY_DEF;

#endif /* PLATFORM_SEAMLESS_FEATURES_ENABLED */

/*!
 * Platform firmware manifest data structure (header)
 * A complete PFM data can contain many SPI region and SMBus rule definitions.
 */
typedef struct
{
    alt_u32 tag;
    alt_u8 svn;
    alt_u8 bkc_version;
    alt_u8 major_rev;
    alt_u8 minor_rev;
    alt_u32 _reserved;
    alt_u32 oem_specific_data[4];
    alt_u32 length;
    alt_u32 pfm_body[];
} PFM;

#ifdef PLATFORM_SEAMLESS_FEATURES_ENABLED

/*!
 * Seamless Firmware Volume Manifest data structure
 */
typedef struct
{
    alt_u32 tag;
    alt_u8 svn;
    alt_u8 _reserved;
    alt_u8 major_rev;
    alt_u8 minor_rev;
    alt_u16 _reserved2;
    alt_u16 fv_type;
    alt_u32 oem_specific_data[4];
    alt_u32 length;
    alt_u32 fvm_body[];
} FVM;

#endif /* PLATFORM_SEAMLESS_FEATURES_ENABLED */

#endif /* EAGLESTREAM_INC_PFM_H_ */
