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
 * @file afm.h
 * @brief Define AFM data structures, such as AFM measurements and policy.
 */

#ifndef EAGLESTREAM_INC_AFM_H_
#define EAGLESTREAM_INC_AFM_H_

#include "pfr_sys.h"

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

#define AFM_SPI_REGION_DEF_TYPE 3
// Lowest allowable unique ID
#define AFM_LOWEST_UNIQUE_ID 0
// Lowest allowable bus ID
#define AFM_LOWEST_BUS_ID 1

// A series of policy recorded in the AFM
#define AFM_POLICY_MISMATCHED_MEAS_LOCK_DOWN_SYSTEM_BIT_MASK      0b1
#define AFM_POLICY_CHAL_AUTH_LOCK_DOWN_SYSTEM_BIT_MASK            0b10
#define AFM_POLICY_PROTOCOL_OR_TIMEOUT_LOCK_DOWN_SYSTEM_BIT_MASK  0b100
#define AFM_POLICY_ATTEST_DEVICE_STAGE_BIT_MASK                   0b1000
// BMC/CPU hold in reset, otherwise recovery
#define AFM_POLICY_BMC_ONLY_RESET_BIT_MASK                        0b10000
#define AFM_POLICY_CPU_ONLY_RESET_BIT_MASK                        0b100000
// Combination of policies for lockdown
#define AFM_POLICY_DEVICE_REQUIRES_LOCKDOWN_BIT_MASK \
                 (AFM_POLICY_MISMATCHED_MEAS_LOCK_DOWN_SYSTEM_BIT_MASK | \
                  AFM_POLICY_CHAL_AUTH_LOCK_DOWN_SYSTEM_BIT_MASK | \
                  AFM_POLICY_PROTOCOL_OR_TIMEOUT_LOCK_DOWN_SYSTEM_BIT_MASK)

// Magic
#define AFM_CURVE_MAGIC_SECP256R1 0xC7B88C74
#define AFM_CURVE_MAGIC_SECP384R1 0x08F07B47
#define AFM_TAG 0x8883CE1D

#define AFM_HEADER_SIZE 28
#define AFM_MAX_MEAS_INDEX 255
// Including total number of measurements field
#define AFM_BODY_HEADER_SIZE 560
#define AFM_BODY_UFM_ALLOCATED_SIZE UFM_CPLD_PAGE_SIZE/4

/*!
 * Attestation firmware manifest data structure
 * A complete AFM data can contain many number of possible measurements
 */
typedef struct
{
    alt_u16 uuid; // specific to each device
    alt_u8 bus_id;
    alt_u8 device_addr;
    alt_u8 binding_spec;
    alt_u8 binding_spec_major_ver;
    alt_u8 binding_spec_minor_ver;
    alt_u8 policy;
    alt_u8 svn;
    alt_u8 _reserved;
    alt_u8 major_rev;
    alt_u8 minor_rev;
    alt_u32 curve_magic;
    alt_u16 platform_manu_str;
    alt_u16 platform_manu_id;
    alt_u32 _reserved2[5];
    alt_u32 pubkey_xy[128];
    alt_u32 pubkey_exponent;
    alt_u32 measurement_num;
    alt_u8 possible_meas_num;
    alt_u8 dmtf_spec_meas_val_type;
    alt_u16 dmtf_spec_meas_val_size;
    alt_u32 measurement[];
} AFM_BODY;

typedef struct
{
    alt_u8 possible_meas_num;
    alt_u8 dmtf_spec_meas_val_type;
    alt_u16 dmtf_spec_meas_val_size;
    alt_u32 measurement[];
} AFM_DMTF_SPEC_MEASUREMENT;

/*!
 * Attestation firmware manifest data structure (address definition)
 * A complete AFM address definition can contain many address to the AFM bodies
 */
typedef struct
{
    alt_u8 afm_def_type;
    alt_u8 device_addr;
    alt_u16 uuid;
    alt_u32 afm_length; // signature + afm body size
    alt_u32 afm_addr;
} AFM_ADDR_DEF;

/*!
 * Attestation firmware manifest data structure (header)
 * A complete header can contain many definitions of afm address definition
 */
typedef struct
{
    alt_u32 tag;
    alt_u8 svn;
    alt_u8 reserved;
    alt_u8 major_rev;
    alt_u8 minor_rev;
    alt_u32 oem_specific_data[4];
    alt_u32 length;
    alt_u32 afm_addr_def[];
} AFM;
#endif

#endif /* EAGLESTREAM_INC_AFM_H_ */
