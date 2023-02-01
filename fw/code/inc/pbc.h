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
 * @file pbc.h
 * @brief Define PFR compression structure format.
 */

#ifndef EAGLESTREAM_INC_PBC_H_
#define EAGLESTREAM_INC_PBC_H_

#include "pfr_sys.h"

// These values are specified in the HAS
#define PBC_EXPECTED_TAG 0x5F504243
#define PBC_EXPECTED_VERSION 0x2
#define PBC_EXPECTED_PAGE_SIZE 0x1000
#define PBC_EXPECTED_PATTERN_SIZE 0x0001
#define PBC_EXPECTED_PATTERN 0xFF

/**
 * Page Block Compression header structure
 *
 * The entire compression structure includes a header and two bit maps.
 * The compressed payload immediately follows the compression structure.
 */
typedef struct
{
    alt_u32 tag;
    alt_u32 version;
    alt_u32 page_size;
    alt_u32 pattern_size;
    alt_u32 pattern;
    alt_u32 bitmap_nbit;
    alt_u32 payload_len;
    alt_u32 _reserved[25];
} PBC_HEADER;

#endif /* EAGLESTREAM_INC_PBC_H_ */
