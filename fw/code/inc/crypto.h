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
 * @file crypto.h
 * @brief Utility functions to communicate with the crypto block.
 */

#ifndef EAGLESTREAM_INC_CRYPTO_H_
#define EAGLESTREAM_INC_CRYPTO_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "pfr_pointers.h"
#include "utils.h"
#include "spi_ctrl_utils.h"
#include "spi_common.h"
#include "gen_gpo_controls.h"
//
// AVMM Memory Map
// For any values that span multiple word addresses, the lowest address will always contain the lsbs of the value
//
// Word Address           | Description
//----------------------------------------------------------------------
// 0x00                   | 0: SHA start (WO) (sent to SHA block each time SHA data input register is written)
//                        | 1: SHA done current block (RO, cleared on go) (cleared each time SHA Input Valid is asserted)
//                        | 2: SHA last (RO, cleared on go) (sent to SHA block each time SHA data input register is written)
//                        | 7:3 : Reserved
//                        | 8 : ECDSA Start (WO)
//                        | 9 : ECDSA Instruction Valid (WO)
//                        | 11:10 : Reserved
//                        | 12 : ECDSA Dout Valid (RO, cleared on start)
//                        | 15:13 : Reserved
//                        | 20:16 : ECDSA Instruction (WO)
//                        | 23:21 : Reserved
//                        | 24 : entropy source health test error (set to 1 on an error, cleared by reset) (RO)
//                        | 25 : FIFO available, indicate FIFO is having at least one 32-bit entropy samples
//                        | 26 : FIFO full, indicate FIFO is full of entropy samples, 256 of 32-bit entropy samples
//                        | 27 : Reserved
//                        | 28 : Crypto Mode. This bit to select which crypto mode to use
//                        |    : 1'b0 (crypto 256), 1b'1 (crypto 384)
//                        |    : This field is valid when more than one crypto block is enabled, else it is reserved.
//                        | 29 : Reserved (WO)
//                        | 30 : Entropy Reset
//                        | 31 : Crypto Reset (WO)
//----------------------------------------------------------------------
// 0x10                   | SHA Data Input (WO)
//                        | Writing to this address causes the current value of start and last from the control register (address 0x00 above) to be sent to the SHA block
//----------------------------------------------------------------------
// 0x10-0x1b              | 384 bit SHA data output (RO)
//                        | Only addresses 0x00-0x07 (256 bits) are used when CRYPTO_LENGTH = 256
//----------------------------------------------------------------------
// 0x20-0x2b              | 384 bit ECDSA data input (WO)
//                        | Only addresses 0x20-0x28 (256 bits) are used when CRYPTO_LENGTH = 256
//----------------------------------------------------------------------
// 0x20-0x2b              | 384 bit ECDSA Cx output (RO)
//                        | Only addresses 0x20-0x28 (256 bits) are used when CRYPTO_LENGTH = 256
//                        | For signature verification this value will be 0 to indicate a valid signature
//                        | For signature verification this is the 'r' value
//                        | For public key generation this is the 'Qx' value
//----------------------------------------------------------------------
// 0x30-0x3b              | 384 bit ECDSA Cy output (RO)
//                        | Only addresses 0x30-0x38 (256 bits) are used when CRYPTO_LENGTH = 256
//                        | For signature verification this value can be ignored
//                        | For signature generation this is the 's' value
//                        | For public key generation THIS IS THE 'Qy' value
//----------------------------------------------------------------------
// 0x40-0x4b              | 384 or 256 bit Entropy sample output (RO)
//                        | Read access to FIFO which stores entropy samples
//                        | Only read this when FIFO available status bit is set

//Important addresses in crypto block memory space

// CSR address for Crypto Operation
#define CRYPTO_CSR_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_AVMM_BRIDGE_BASE, 0)
#define CRYPTO_DATA_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_AVMM_BRIDGE_BASE, 16)
#define CRYPTO_DATA_SHA_ADDR(x) __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_AVMM_BRIDGE_BASE, 16 + x)
#define CRYPTO_ECDSA_DATA_INPUT_ADDR(x) __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_AVMM_BRIDGE_BASE, 32 + x)
#define CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR(x) __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_AVMM_BRIDGE_BASE, 32 + x)
#define CRYPTO_ECDSA_DATA_CY_OUTPUT_ADDR(x) __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_AVMM_BRIDGE_BASE, 48 + x)
// CSR address for Entropy Source
#define CRYPTO_ENTROPY_SOURCE_OUTPUT_ADDR(x) __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_AVMM_BRIDGE_BASE, 64 + x)

// CSR address for DMA Operation
#define CRYPTO_DMA_CSR_ADDRESS __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_DMA_AVMM_BRIDGE_BASE, 0)
#define CRYPTO_DMA_CSR_START_ADDRESS __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_DMA_AVMM_BRIDGE_BASE, 1)
#define CRYPTO_DMA_CSR_DATA_TRANSFER_LENGTH __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_CRYPTO_DMA_AVMM_BRIDGE_BASE, 2)
#define CRYPTO_GPO_1_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_GPO_1_BASE, 0)

// The defined CSR interfaces from the table above
#define CRYPTO_CSR_SHA_START_MSK (0x01 << 0)
#define CRYPTO_CSR_SHA_DONE_CURRENT_BLOCK_MSK (0x01 << 1)
#define CRYPTO_CSR_SHA_DONE_CURRENT_BLOCK_OFST (1)
#define CRYPTO_CSR_SHA_LAST_MSK (0x01 << 2)
#define CRYPTO_CSR_ECDSA_START_MSK (0x01 << 8)
#define CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK (0x01 << 9)
#define CRYPTO_CSR_ECDSA_DOUT_VALID_MSK (0x01 << 12)
#define CRYPTO_CSR_ECDSA_DOUT_VALID_OFST (12)
#define CRYPTO_CSR_ENTROPY_SOURCE_HEALTH_TEST_ERROR_MSK (0x01 << 24)
#define CRYPTO_CSR_ENTROPY_SOURCE_HEALTH_TEST_ERROR_MSK_OFST (24)
#define CRYPTO_CSR_ENTROPY_SOURCE_FIFO_AVAILABILITY_OFST (25)
#define CRYPTO_CSR_ENTROPY_SOURCE_FIFO_FULL_MSK (0x01 << 26)
#define CRYPTO_CSR_ENTROPY_SOURCE_FIFO_FULL_OFST (26)
#define ENTROPY_CSR_RESET_MSK (0x01 << 30)
#define CRYPTO_CSR_CRYPTO_RESET_MSK (0x01 << 31)

// ECDSA Instructions.
// This is the new instruction to be passed onto crypto IP before attending to any data transaction
#define ECDSA_INSTR_WRITE_AX (0x01 << 16)
#define ECDSA_INSTR_WRITE_AY (0x02 << 16)
#define ECDSA_INSTR_WRITE_BX (0x03 << 16)
#define ECDSA_INSTR_WRITE_BY (0x04 << 16)
#define ECDSA_INSTR_WRITE_D_SIGN (0x04 << 16)
#define ECDSA_INSTR_WRITE_P (0x05 << 16)
#define ECDSA_INSTR_WRITE_A (0x06 << 16)
#define ECDSA_INSTR_WRITE_N (0x10 << 16)
#define ECDSA_INSTR_WRITE_R (0x11 << 16)
#define ECDSA_INSTR_WRITE_K (0x11 << 16)
#define ECDSA_INSTR_WRITE_D_KEYGEN (0x11 << 16)
#define ECDSA_INSTR_WRITE_E (0x12 << 16)
#define ECDSA_INSTR_WRITE_S (0x13 << 16)
#define ECDSA_INSTR_WRITE_LAMBDA (0x13 << 16)
#define ECDSA_INSTR_VALIDATE (0x0f << 16)
#define ECDSA_INSTR_SIGN (0x07 << 16)
#define ECDSA_INSTR_GEN_PUB_KEY (0x08 << 16)

// CSR interfaces for DMA
#define CRYPTO_DMA_CSR_GO_MASK (0x01 << 0)
#define CRYPTO_DMA_CSR_DONE_MASK (0x01 << 1)
#define CRYPTO_DMA_CSR_DONE_OFST (1)
#define CRYPTO_DMA_CSR_SOFT_RESET_MASK (0x01 << 31)

/**************************************************
 *
 * Constants for NIST P-256 curve
 *
 **************************************************/

// echo "6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296" | xxd -r -p | xxd -i
static const alt_u8 CRYPTO_EC_AX_256[SHA256_LENGTH] = {
    0x6b, 0x17, 0xd1, 0xf2, 0xe1, 0x2c, 0x42, 0x47, 0xf8, 0xbc, 0xe6, 0xe5, 0x63, 0xa4, 0x40, 0xf2,
    0x77, 0x03, 0x7d, 0x81, 0x2d, 0xeb, 0x33, 0xa0, 0xf4, 0xa1, 0x39, 0x45, 0xd8, 0x98, 0xc2, 0x96};

// echo "4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5" | xxd -r -p | xxd -i
static const alt_u8 CRYPTO_EC_AY_256[SHA256_LENGTH] = {
    0x4f, 0xe3, 0x42, 0xe2, 0xfe, 0x1a, 0x7f, 0x9b, 0x8e, 0xe7, 0xeb, 0x4a, 0x7c, 0x0f, 0x9e, 0x16,
    0x2b, 0xce, 0x33, 0x57, 0x6b, 0x31, 0x5e, 0xce, 0xcb, 0xb6, 0x40, 0x68, 0x37, 0xbf, 0x51, 0xf5};

// echo "ffffffff00000001000000000000000000000000ffffffffffffffffffffffff" | xxd -r -p | xxd -i
static const alt_u8 CRYPTO_EC_P_256[SHA256_LENGTH] = {
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

// echo "ffffffff00000001000000000000000000000000fffffffffffffffffffffffc" | xxd -r -p | xxd -i
static const alt_u8 CRYPTO_EC_A_256[SHA256_LENGTH] = {
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc};

// echo "ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551" | xxd -r -p | xxd -i
static const alt_u8 CRYPTO_EC_N_256[SHA256_LENGTH] = {
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xbc, 0xe6, 0xfa, 0xad, 0xa7, 0x17, 0x9e, 0x84, 0xf3, 0xb9, 0xca, 0xc2, 0xfc, 0x63, 0x25, 0x51};

static const alt_u8 CRYPTO_EC_DA_256[SHA256_LENGTH] = {
    0xc5, 0x1e, 0x47, 0x53, 0xaf, 0xde, 0xc1, 0xe6, 0xb6, 0xc6, 0xa5, 0xb9, 0x92, 0xf4, 0x3f, 0x8d,
    0xd0, 0xc7, 0xa8, 0x93, 0x30, 0x72, 0x70, 0x8b, 0x65, 0x22, 0x46, 0x8b, 0x2f, 0xfb, 0x06, 0xfd};

static const alt_u8 CRYPTO_PUB_CX_256[SHA256_LENGTH] = {
    0x94, 0x2c, 0x9f, 0x40, 0x8e, 0xad, 0x9d, 0x82, 0xd3, 0x4a, 0x1b, 0x9a, 0x6a, 0x82, 0x7e, 0xbe,
    0x3e, 0x2d, 0xdf, 0x78, 0x2b, 0x44, 0x8d, 0x23, 0xbe, 0x1b, 0x61, 0x43, 0x98, 0x8c, 0xce, 0xf4};

static const alt_u8 CRYPTO_PUB_CY_256[SHA256_LENGTH] = {
    0x8c, 0x9e, 0xaf, 0x6c, 0x0d, 0x14, 0xd9, 0x92, 0xfc, 0x63, 0xba, 0xd3, 0xe2, 0x49, 0x6b, 0xe2,
    0xee, 0xe6, 0x1c, 0xb5, 0xb9, 0x7f, 0x65, 0xf4, 0x28, 0xca, 0x94, 0xa5, 0xd0, 0xee, 0x19, 0xa1};

static alt_u8 HMAC_1_256[SHA256_LENGTH] = {0};

static alt_u8 HMAC_2_256[SHA256_LENGTH] = {0};

/**************************************************
 *
 * Constants for NIST P-384 curve
 *
 **************************************************/

static const alt_u8 CRYPTO_EC_AX_384[SHA384_LENGTH] = {
    0xaa, 0x87, 0xca, 0x22, 0xbe, 0x8b, 0x05, 0x37, 0x8e, 0xb1, 0xc7, 0x1e, 0xf3, 0x20, 0xad, 0x74,
    0x6e, 0x1d, 0x3b, 0x62, 0x8b, 0xa7, 0x9b, 0x98, 0x59, 0xf7, 0x41, 0xe0, 0x82, 0x54, 0x2a, 0x38,
    0x55, 0x02, 0xf2, 0x5d, 0xbf, 0x55, 0x29, 0x6c, 0x3a, 0x54, 0x5e, 0x38, 0x72, 0x76, 0x0a, 0xb7};

static const alt_u8 CRYPTO_EC_AY_384[SHA384_LENGTH] = {
    0x36, 0x17, 0xde, 0x4a, 0x96, 0x26, 0x2c, 0x6f, 0x5d, 0x9e, 0x98, 0xbf, 0x92, 0x92, 0xdc, 0x29,
    0xf8, 0xf4, 0x1d, 0xbd, 0x28, 0x9a, 0x14, 0x7c, 0xe9, 0xda, 0x31, 0x13, 0xb5, 0xf0, 0xb8, 0xc0,
    0x0a, 0x60, 0xb1, 0xce, 0x1d, 0x7e, 0x81, 0x9d, 0x7a, 0x43, 0x1d, 0x7c, 0x90, 0xea, 0x0e, 0x5f};

static const alt_u8 CRYPTO_EC_P_384[SHA384_LENGTH] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff};

static const alt_u8 CRYPTO_EC_A_384[SHA384_LENGTH] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xfc};

static const alt_u8 CRYPTO_EC_N_384[SHA384_LENGTH] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc7, 0x63, 0x4d, 0x81, 0xf4, 0x37, 0x2d, 0xdf,
    0x58, 0x1a, 0x0d, 0xb2, 0x48, 0xb0, 0xa7, 0x7a, 0xec, 0xec, 0x19, 0x6a, 0xcc, 0xc5, 0x29, 0x73};

static const alt_u8 CRYPTO_EC_DA_384[SHA384_LENGTH] = {
    0xcd, 0x80, 0x28, 0x3e, 0x15, 0x2c, 0x3f, 0x9f, 0x34, 0x68, 0x92, 0xbc, 0xbf, 0xce, 0x86, 0x1f,
    0x6d, 0x0a, 0x22, 0xae, 0x18, 0xdd, 0x2a, 0xa5, 0x93, 0x82, 0x0f, 0xd2, 0xbe, 0x43, 0x50, 0xa9,
    0x6b, 0xa3, 0x88, 0xa5, 0x1b, 0xfd, 0xb1, 0x4d, 0x42, 0x1e, 0x62, 0xd6, 0x11, 0xbc, 0x50, 0x70};

static const alt_u8 CRYPTO_PUB_CX_384[SHA384_LENGTH] = {
    0xe5, 0x42, 0x3d, 0x97, 0xff, 0xfb, 0x23, 0xa9, 0xa6, 0x2c, 0x98, 0x8a, 0x25, 0x90, 0x2f, 0x72,
    0xca, 0x19, 0xd1, 0x76, 0x60, 0x15, 0x80, 0x42, 0xd0, 0xf9, 0x5c, 0xf7, 0xcb, 0x05, 0x78, 0x86,
    0xf6, 0x70, 0x17, 0xb5, 0xab, 0x09, 0xe8, 0xc0, 0x96, 0xb7, 0xd3, 0x15, 0x2f, 0x52, 0x7e, 0x92};

static const alt_u8 CRYPTO_PUB_CY_384[SHA384_LENGTH] = {
    0xfd, 0x87, 0x43, 0x35, 0xd3, 0xfe, 0x31, 0xbb, 0x4e, 0xbe, 0xee, 0x38, 0xe2, 0x03, 0x64, 0x07,
    0x1e, 0xa1, 0x71, 0x65, 0xd9, 0xc1, 0x72, 0x54, 0xa5, 0x77, 0x81, 0x70, 0x4e, 0xbf, 0x03, 0xa4,
    0x3a, 0xdb, 0x7c, 0xdb, 0x18, 0x3f, 0xc3, 0x24, 0x06, 0x28, 0x97, 0x1a, 0x18, 0x78, 0xc3, 0xa2};

static alt_u8 HMAC_1_384[SHA384_LENGTH] = {0};

static alt_u8 HMAC_2_384[SHA384_LENGTH] = {0};

/*********************************************
 *
 *  Constants used in HMAC calculation
 *
 *********************************************/

// To save code space, the constant from step 4 and 7 have been pre-computed and are stored.
// The snippet of code below is used to generate those constant values.

// Step 1: Run the code below.
/****************************************************************************************************************
// The code to precompute hmac constants are found in ../testdata/hmac/hmac.c
******************************************************************************************************************/

// Step 2: Obtain the printed string

// Step 3: echo "<printed stirng>" | xxd -r -p | xxd -i

static const alt_u8 XOR_K0_AND_IPAD_384[CRYPTO_BLOCK_SIZE_FOR_384] = {
		  0x38, 0x44, 0x50, 0xc1, 0xb6, 0x10, 0x70, 0xca, 0xc5, 0x84, 0x9f, 0x40,
		  0x98, 0xdf, 0xe0, 0x0f, 0xb8, 0xff, 0x6a, 0x2c, 0x63, 0xab, 0x56, 0xbb,
		  0x0b, 0x80, 0xc9, 0x6d, 0xe7, 0x0e, 0xad, 0x41, 0xe5, 0x0a, 0x93, 0x89,
		  0x08, 0x25, 0xa5, 0x73, 0x5e, 0x8e, 0x79, 0x5d, 0xee, 0xc5, 0x43, 0x64,
		  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
		  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
		  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
		  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
		  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
		  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
		  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36};

static const alt_u8 XOR_K0_AND_OPAD_384[CRYPTO_BLOCK_SIZE_FOR_384] = {
		  0x52, 0x2e, 0x3a, 0xab, 0xdc, 0x7a, 0x1a, 0xa0, 0xaf, 0xee, 0xf5, 0x2a,
		  0xf2, 0xb5, 0x8a, 0x65, 0xd2, 0x95, 0x00, 0x46, 0x09, 0xc1, 0x3c, 0xd1,
		  0x61, 0xea, 0xa3, 0x07, 0x8d, 0x64, 0xc7, 0x2b, 0x8f, 0x60, 0xf9, 0xe3,
		  0x62, 0x4f, 0xcf, 0x19, 0x34, 0xe4, 0x13, 0x37, 0x84, 0xaf, 0x29, 0x0e,
		  0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
		  0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
		  0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
		  0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
		  0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
		  0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
		  0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c};

static const alt_u8 XOR_K0_AND_IPAD_256[CRYPTO_BLOCK_SIZE_FOR_256] = {
		  0xb8, 0xff, 0x6a, 0x2c, 0x63, 0xab, 0x56, 0xbb, 0x0b, 0x80, 0xc9, 0x6d,
		  0xe7, 0x0e, 0xad, 0x41, 0xe5, 0x0a, 0x93, 0x89, 0x08, 0x25, 0xa5, 0x73,
		  0x5e, 0x8e, 0x79, 0x5d, 0xee, 0xc5, 0x43, 0x64, 0x36, 0x36, 0x36, 0x36,
		  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
		  0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
		  0x36, 0x36, 0x36, 0x36};

static const alt_u8 XOR_K0_AND_OPAD_256[CRYPTO_BLOCK_SIZE_FOR_256] = {
		  0xd2, 0x95, 0x00, 0x46, 0x09, 0xc1, 0x3c, 0xd1, 0x61, 0xea, 0xa3, 0x07,
		  0x8d, 0x64, 0xc7, 0x2b, 0x8f, 0x60, 0xf9, 0xe3, 0x62, 0x4f, 0xcf, 0x19,
		  0x34, 0xe4, 0x13, 0x37, 0x84, 0xaf, 0x29, 0x0e, 0x5c, 0x5c, 0x5c, 0x5c,
		  0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
		  0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
		  0x5c, 0x5c, 0x5c, 0x5c};

// Max entropy retry for retrievement
#define CRYPTO_MAX_ENTROPY_DATA_HEALTH_FAILURE 3

static alt_u8 entropy_sample_output[256] = {0};

/**
 * @brief Reset entropy extractor, clear test health and fifo data
 *
 * @param crypto_256_or_384 : Crypto mode selection
 *
 * @return none
 */

static void reset_entropy(CRYPTO_MODE crypto_256_or_384)
{
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, ENTROPY_CSR_RESET_MSK | crypto_256_or_384);
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, (ENTROPY_CSR_RESET_MSK & ~ENTROPY_CSR_RESET_MSK) | crypto_256_or_384);
}

/**
 * @brief Reset crypto IP, clear all data in FIFO and reset state machine
 *
 * @param crypto_256_or_384 : Crypto mode selection
 *
 * @return none
 */

static void reset_crypto(CRYPTO_MODE crypto_256_or_384)
{
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_CRYPTO_RESET_MSK | crypto_256_or_384);
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, (CRYPTO_CSR_CRYPTO_RESET_MSK & 0x7fffffff) | crypto_256_or_384);
}

/**
 * @brief This function switches the mux to allow communication between Crypto and SPI or UFM through DMA (as the middle man).
 * The DMA will do padding and prepare the crypto block with the required configuration for hashing operation in general.
 *
 * This functions prepares the DMA as well as the Crypto before any data transaction begins.
 * The requirement is to setup the configuration by passing in important data such as the starting address of the data
 * located in the spi flashes or UFM and the data transfer length.
 * This function should only be used if there is any needed interaction between spi flashes or UFM and crypto block.
 * Otherwise, any crypto operation should be done by NIOS manually.
 *
 * @param start_addr_of_data : The address of the data located in the spi flash or ufm
 * @param data_size : The transfer length of the data (in bytes)
 * @param crypto_256_or_384 : Crypto mode selection
 *
 * @return none
 */

static void set_dma_interconnection(alt_u32 start_addr_of_data, alt_u32 data_size, CRYPTO_MODE crypto_256_or_384, DMA_MODE dma_mode)
{
    alt_u32 avmm_src_addr = (dma_mode == ENGAGE_DMA_SPI) ? (start_addr_of_data + U_SPI_FILTER_AVMM_BRIDGE_BASE) : (start_addr_of_data + U_UFM_DATA_BASE);
#ifdef USE_SYSTEM_MOCK
    avmm_src_addr = start_addr_of_data;
#endif
    reset_crypto(crypto_256_or_384);

    alt_u32 dma_256_or_384 = (crypto_256_or_384 == CRYPTO_384_MODE) ? 4 : 0;
    alt_u32 crypto_block_size = (crypto_256_or_384 == CRYPTO_384_MODE) ? CRYPTO_BLOCK_SIZE_FOR_384 : CRYPTO_BLOCK_SIZE_FOR_256;
    alt_u32 gpo_1_dma_sel = (dma_mode == ENGAGE_DMA_SPI) ? GPO_1_DMA_NIOS_MASTER_SEL : GPO_1_DMA_UFM_SEL;

    //Configure to select mux for spi to dma and dma to crypto
    set_bit(CRYPTO_GPO_1_ADDR, gpo_1_dma_sel);

    //Configure Crypto DMA CSR register to setup the start address
    IOWR_32DIRECT(CRYPTO_DMA_CSR_START_ADDRESS, 0, avmm_src_addr);

    //Configure Crypto DMA CSR register to setup the transfer length
    IOWR_32DIRECT(CRYPTO_DMA_CSR_DATA_TRANSFER_LENGTH, 0, data_size);

    //Set the Go bit alongside with the crypto mode (256 or 384)
    IOWR_32DIRECT(CRYPTO_DMA_CSR_ADDRESS, 0, CRYPTO_DMA_CSR_GO_MASK | dma_256_or_384);

    while (!check_bit(CRYPTO_DMA_CSR_ADDRESS, CRYPTO_DMA_CSR_DONE_OFST))
    {
        reset_hw_watchdog();
    }

    // This bit can be cleared once DMA is done.
    // This means CRYPTO_DMA_CSR_DONE_OFST is checked.
    clear_bit(CRYPTO_GPO_1_ADDR, gpo_1_dma_sel);

    while (data_size != 0)
    {
        data_size -= crypto_block_size;
        //Poll for sha_done
        while (!check_bit(CRYPTO_CSR_ADDR, CRYPTO_CSR_SHA_DONE_CURRENT_BLOCK_OFST))
        {
    	    reset_hw_watchdog();
        }
    }
}

/**
 * @brief This function is used to verify the signature associated with the ECDSA operation
 *
 * @param n_bytes: ECDSA cx output in bytes
 *
 * @return 0 if pass, else return 1 if fail
 */
static alt_u32 verify_cx (const alt_u32 n_bytes)
{
    PFR_ASSERT((n_bytes % 4) == 0)

    for (alt_u32 word_i = 0; word_i < (n_bytes / 4); word_i++)
    {
	    if (IORD(CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR(word_i), 0) != 0)
	    {
	        return 0;
	    }
    }

    return 1;
}

/**
 * @brief This function swap the bytes of the last message sent in after padding.
 * This is needed so that it is in the right endian addressing for the crypto IP.
 *
 * @param length_of_message : unpadded message length in bits
 *
 * @return data with bytes swapped
 */
static alt_u32 swap_bytes (alt_u32 length_of_message)
{
    return (((length_of_message >> 24) & 0xff) | ((length_of_message >> 8) & 0xff00) | ((length_of_message << 8) & 0xff0000) | ((length_of_message << 24) & 0xff000000));
}

/**
 * @brief This is the padding algorithm for sha-256 and sha-384 functionality.
 *
 * In general, the padding is done to make sure the original message length + padding to have the
 * correct bytes alignment. This function can pad any data size.
 *
 * For sha-256 mode, the padding to the back of the original message begins with a bit '1' then
 * followed by bit '0's and the last 64 bits is appended as the length of the original unpadded
 * message in "bits". This to make sure the entire message (including padding) for sha-256 is a
 * multiple of 64 bytes. If the unpadded message is already a multiple of 64 bytes, the padding
 * will then be a length of 64 bytes to make sure again the total message length is 64 bytes
 * aligned.
 *
 * For sha-384 mode, the padding also begins with a bit '1' then followed by bit '0's and the last
 * 128 bits is appended as the length of the unpadded message in "bits". This is to make sure the
 * entire message length (including padding) is a multiple of 128 bytes. If unpadded message is already
 * 128 bytes aligned, the padding will now be a length of 128 bytes as needed.
 *
 * NOTE: Given a message in which the remaining bytes to pad is small such that it cannot fit the
 * original message length, a rollover to the next sha block is required.
 *
 * @param dest_ptr : the address to be written to
 * @param crypto_256_or_384 : to select the mode either to use sha-256 or sha-384
 * @param data_size : the original unpadded message length to be passed in
 *
 * @return none
 *
 */
static void sha_padding (alt_u32* dest_ptr, CRYPTO_MODE crypto_256_or_384, alt_u32 data_size, alt_u8* data)
{
    // Message length is no longer restricted to be multiple of 4
    // PFR_ASSERT((data_size % 4) == 0)

    // Allow quotient to be zero for the case where the data size is less than their crypto block size
    alt_u32 quotient = 0;
    alt_u32 processed_size = 4*(data_size/4);

    // This will be the default value when data is 4 byte aligned.
    alt_u32 first_padded_word = 0x80000000;
    // Max byte shift for a word for non 4-byte aligned data size
    alt_u32 msb_word_shift = 3;

    // If data is not 4-byte aligned, process the first word to be padded
    if (data_size % 4 != 0)
    {
        // Get the leftover bytes to align to multiple of sha block size
        alt_u32 leftover_bytes = data_size - processed_size;
        first_padded_word >>= leftover_bytes*8;

        // Copy over the existing leftover bytes of original message to first word to be padded
        while (leftover_bytes)
        {
            // Extract the leftover bytes after masking
            quotient = data[data_size - leftover_bytes] & 0xff;
            // Combine the extracted data
            first_padded_word |= (quotient << msb_word_shift*8);
            // Move on to the next leftover byte
            leftover_bytes--;
            msb_word_shift--;
        }
        quotient = 0;
    }

    // If the data size to be padded is equal/more than the crypto block size, do a division to obtain the quotient
    if (((crypto_256_or_384 == CRYPTO_384_MODE) && (processed_size >= CRYPTO_BLOCK_SIZE_FOR_384)) ||
    	((crypto_256_or_384 == CRYPTO_256_MODE) && (processed_size >= CRYPTO_BLOCK_SIZE_FOR_256)))
    {
        // This function rounds 'down' the quotient to the lowest integer
        quotient = (crypto_256_or_384 == CRYPTO_384_MODE) ? (processed_size / CRYPTO_BLOCK_SIZE_FOR_384) : (processed_size / CRYPTO_BLOCK_SIZE_FOR_256);
    }

    alt_u32 block_size = (crypto_256_or_384 == CRYPTO_384_MODE) ? CRYPTO_BLOCK_SIZE_FOR_384 : CRYPTO_BLOCK_SIZE_FOR_256;
    // Calculates the amount of zeros to be padded in words based on the calculated (rounded) quotient
    alt_u32 no_of_words_of_padded_zero = ((((quotient * block_size) + block_size) - processed_size) / 4) - 2;

    // Check the minimum leftover placeholder for padding
    // If leftover bytes to be padded are bigger than the placeholder, extend the padded
    // data to the next multiple of crypto block size
    alt_u32 min_placeholder_bytes = (crypto_256_or_384 == CRYPTO_384_MODE)
                        ? (CRYPTO_MINIMUM_LEFTOVER_BIT_384 / 8) : (CRYPTO_MINIMUM_LEFTOVER_BIT_256 / 8);

    // Default 'round down' integer multiplier for message length less than block size
    alt_u32 placeholder_bytes_check = 0;

    // Obtain the last sha block before padding
    alt_u32 last_sha_block_before_pad_size = data_size - (block_size * (data_size / block_size));

    // If data size is greater than block size, get the integer multiple of block size
    // else, simply subtract to get bytes needed to pad
    // Get to the nearest multiple of block size
    if (data_size > block_size)
    {
        placeholder_bytes_check = block_size - last_sha_block_before_pad_size;
    }
    else if (data_size < block_size)
    {
        placeholder_bytes_check = block_size - data_size;
    }

    alt_u32 first_sha_block_zero_padding = 0;
    // Assume the current block is not the first sha block
    alt_u32 toggle_control_bits = CRYPTO_CSR_SHA_LAST_MSK;

    if (((crypto_256_or_384 == CRYPTO_384_MODE) && (processed_size < CRYPTO_BLOCK_SIZE_FOR_384)) ||
        ((crypto_256_or_384 == CRYPTO_256_MODE) && (processed_size < CRYPTO_BLOCK_SIZE_FOR_256)))
    {
        toggle_control_bits = CRYPTO_CSR_SHA_START_MSK | CRYPTO_CSR_SHA_LAST_MSK;
    }

    // Minimum 128 bits plus 8 bits for sha384
    // Minimum 64 bits plus 8 bits for sha256
    alt_u32 ls4b_for_sha_block = (crypto_256_or_384 == CRYPTO_384_MODE) ?
    		             CRYPTO_LEAST_SIGNIFICANT_4_BYTES_384 : CRYPTO_LEAST_SIGNIFICANT_4_BYTES_256;

    last_sha_block_before_pad_size = 4*(last_sha_block_before_pad_size/4);

    if (last_sha_block_before_pad_size == ls4b_for_sha_block)
    {
        // Write sha start or/and sha last toggle bit at every least 32 bit of every sha block
        IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, toggle_control_bits | crypto_256_or_384);

        // If the first SHA block is sent in, untoggle SHA start control bit
        toggle_control_bits = CRYPTO_CSR_SHA_LAST_MSK;
    }

    // Padding begins with a '1' after the last byte of original message
    IOWR(dest_ptr, 0, swap_bytes(first_padded_word));

    // If data size is multiple of block size, skip.
    // If padded bytes is larger than the placeholder bytes to adds up to nearest block size, skip.
    if ((placeholder_bytes_check < min_placeholder_bytes) && ((data_size % block_size) != 0))
    {
        // Extend to next multiple of block size
        no_of_words_of_padded_zero = (block_size/4) - 1;

        // Check the zero's padding prior to end of a sha block
        first_sha_block_zero_padding = (4*((placeholder_bytes_check - 1)/4))/4;

        if (first_sha_block_zero_padding != 0)
        {
            for (alt_u32 i = 0; i < first_sha_block_zero_padding; i++)
            {
                if (i == (first_sha_block_zero_padding - 1))
                {
                    // Write sha start or/and sha last toggle bit at every least 32 bit of every sha block
                    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, toggle_control_bits | crypto_256_or_384);

                    // If the first SHA block is sent in, untoggle SHA start control bit
                    toggle_control_bits = CRYPTO_CSR_SHA_LAST_MSK;
                }
                // Pad with the calculated amount of zeros
                IOWR(dest_ptr, 0, 0);
            }
        }
        // Set SHA start and last bit to be 0 for every start of SHA block
        IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, (((CRYPTO_CSR_SHA_START_MSK | CRYPTO_CSR_SHA_LAST_MSK) & 0xfffffffa) | crypto_256_or_384));
    }

    // Pad with the calculated amount of zeros
    for (alt_u32 i = 0; i < no_of_words_of_padded_zero; i++)
    {
        IOWR(dest_ptr, 0, 0);
    }

    // Send sha start and sha last at the least significant 32 bit of padding if it is the first and last block.
    // Else, send sha last bit only.
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, toggle_control_bits | crypto_256_or_384);

    // Send the original message length as the last part to the padding requirement.
    IOWR(dest_ptr, 0, swap_bytes (data_size * 8));
}

//Incremental memcpy function but in reverse ordering just for the crypto implementation
/**
 * @brief This function copies data word by word to an incremental address, hence incremental.
 * This is a custom version of memcpy function just for cryptographic functionality. This is to store
 * data in little endian addressing.
 *
 * @param dest_ptr : address to be written to
 * @param scr_ptr : address to be read from
 * @param n_bytes : length of data read in bytes
 *
 * @return none
 */
static void alt_u32_memcpy_for_crypto(alt_u32* dest_ptr, const alt_u32* src_ptr, alt_u32 n_bytes)
{
    PFR_ASSERT((n_bytes % 4) == 0)

    alt_u32 sha_length_in_words = (n_bytes >> 2);

    for (alt_u32 i = 0; i < sha_length_in_words; i++)
    {
        //This function achieves the same functionality. However, IOWR is needed for intercept write to CRYPTO_MOCK
        //It is important to note that alt_u32_memcpy cannot use IOWR because this function is used for SPI_MOCK access and SYSTEM_MOCK cannot intercept write to SPI_MOCK
        //dest_ptr[i] = src_ptr[sha_length_in_words - i - 1];
        IOWR(&dest_ptr[i], 0, src_ptr[sha_length_in_words - i - 1]);
    }
}

// Non-incremental memcpy for crypto
/**
 * @brief This function copies data word by word to the same address, hence non-incremental.
 * This is a custom version of memcpy function just for cryptographic functionality.
 *
 * This is an updated version for phase 2 EGS. CRYPTO_CSR_SHA_START_MSK bit must be sent for
 * the first (and only) block of every transaction. PRIOR to the end of every block of an operation,
 * CRYPTO_CSR_SHA_LAST_MSK must be set to 1.
 *
 * @param dest_ptr : the address to be written to
 * @param scr_ptr : the address to be read from
 * @param n_bytes : the length in bytes of the data passed in
 * @param crypto_256_or_384 : to select either sha-256 or sha-384 mode
 * @param first_sha_block_indicator : this parameter is important to indicator whether a sha block is the very first of a transaction
 *
 * @return none
 */
static void alt_u32_memcpy_non_incr_for_crypto(alt_u32* dest_ptr, const alt_u32* src_ptr, const alt_u32 n_bytes, CRYPTO_MODE crypto_256_or_384, alt_u32* first_sha_block_indicator)
{
    // PFR_ASSERT((n_bytes % 4) == 0)
    // Copy over only the data size which is multiple of 4.
    alt_u32 processed_size = 4*(n_bytes/4);

    alt_u32 crypto_block_word_size = (crypto_256_or_384 == CRYPTO_384_MODE) ? (CRYPTO_BLOCK_SIZE_FOR_384/4) : (CRYPTO_BLOCK_SIZE_FOR_256/4);
    for (alt_u32 i = 0; i < (processed_size >> 2); i++)
    {
        // If this is the first ever sha block, set SHA START and SHA LAST bit to 1
        // prior to sending the least significant 32 bits ofr message
	    if ((i == (crypto_block_word_size - 1)) && (*first_sha_block_indicator == 1))
	    {
		    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, (CRYPTO_CSR_SHA_START_MSK | CRYPTO_CSR_SHA_LAST_MSK) | crypto_256_or_384);
		    *first_sha_block_indicator = 0;
	    }
	    else if (i == (crypto_block_word_size - 1))
	    {
		    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_SHA_LAST_MSK | crypto_256_or_384);
	    }
	    IOWR(dest_ptr, 0, src_ptr[i]);
    }
}

/**
 * @brief This function calculates the SHA hash of the given data.
 *
 * It sends all the inputs to the crypto block and the sha result can be retrieved through CSR interface.
 * Prior to any sha operation, the reset bit must be set to 1 and then set back to 0. This initiates the
 * operation.
 *
 * The function checks the length of the message to be hashed and then call the sha_padding function
 * to do the needed padding based on the length of unpadded data. CRYPTO_CSR_SHA_START_MSK bit and
 * CRYPTO_CSR_SHA_LAST_MSK bit is to be set to 0 at the beginning of transaction.
 *
 * @param data : data to be hashed
 * @param data_size : size of the data to be hashed in bytes
 * @param crypto_256_or_384 : to select between sha-256 and sha_384 mode
 *
 * @return none
 */
static void calculate_sha(const alt_u32* data, alt_u32 data_size, CRYPTO_MODE crypto_256_or_384)
{
    alt_u32 message_length = data_size;
    alt_u32 indicator = 1;
    alt_u32* first_sha_block = &indicator;
    alt_u32* data_local_ptr = (alt_u32*) data;
    alt_u32 block_size = (crypto_256_or_384 == CRYPTO_256_MODE) ? CRYPTO_BLOCK_SIZE_FOR_256 : CRYPTO_BLOCK_SIZE_FOR_384;

    while (data_size)
    {
        // Set SHA start and last bit to be 0
        IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, (((CRYPTO_CSR_SHA_START_MSK | CRYPTO_CSR_SHA_LAST_MSK) & 0xfffffffa) | crypto_256_or_384));
        if (((crypto_256_or_384 == CRYPTO_256_MODE) && (data_size >= CRYPTO_BLOCK_SIZE_FOR_256)) ||
        		((crypto_256_or_384 == CRYPTO_384_MODE) && (data_size >= CRYPTO_BLOCK_SIZE_FOR_384)))
        {
            // Copy payload from flash to CSR
            alt_u32_memcpy_non_incr_for_crypto(CRYPTO_DATA_ADDR, data_local_ptr, block_size, crypto_256_or_384, first_sha_block);
            data_local_ptr = incr_alt_u32_ptr(data_local_ptr, block_size);
            data_size -= block_size;
        }
        else if ((data_size > 0))
        {
            // Start padding. A padding can rollover to next multiple of sha block
            alt_u32_memcpy_non_incr_for_crypto(CRYPTO_DATA_ADDR, data_local_ptr, data_size, crypto_256_or_384, first_sha_block);
            sha_padding (CRYPTO_DATA_ADDR, crypto_256_or_384, message_length, (alt_u8*)data);
            data_size = 0;
        }
        // Wait for SHA_DONE (SHA only)
        while (!check_bit(CRYPTO_CSR_ADDR, CRYPTO_CSR_SHA_DONE_CURRENT_BLOCK_OFST))
        {
            reset_hw_watchdog();
        }
    }
    // Handles special cases where message size is already a multiple of sha block
    if ((((message_length % 128) == 0) && (crypto_256_or_384 == CRYPTO_384_MODE)) ||
        (((message_length % 64) == 0) && (crypto_256_or_384 == CRYPTO_256_MODE)))
    {
        IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, (((CRYPTO_CSR_SHA_START_MSK | CRYPTO_CSR_SHA_LAST_MSK) & 0xfffffffa) | crypto_256_or_384));
        sha_padding (CRYPTO_DATA_ADDR, crypto_256_or_384, message_length, (alt_u8*)data);

        while (!check_bit(CRYPTO_CSR_ADDR, CRYPTO_CSR_SHA_DONE_CURRENT_BLOCK_OFST))
        {
            reset_hw_watchdog();
        }
    }
}

/**
 * @brief This function calculates the SHA hash of the given data and
 * saves it at the destination address.
 *
 * @param dest_addr : address to be written to
 * @param addr : address of the data to be hashed
 * @param data : pointer to the data to be hashed
 * @param data_size : length of data to be hashed in bytes
 * @param crypto_256_or_384 : mode selection between sha-256 or sha-384
 * @param dma_mode : dma to ufm/ dma to spi
 *
 * @return none
 */
static void calculate_and_save_sha(alt_u32* dest_addr, const alt_u32 addr, const alt_u32* data, alt_u32 data_size, CRYPTO_MODE crypto_256_or_384, DMA_MODE dma_mode)
{
    alt_u32 sha_length = (crypto_256_or_384 == CRYPTO_384_MODE) ? SHA384_LENGTH : SHA256_LENGTH;
    if (dma_mode == DISENGAGE_DMA)
    {
        reset_crypto(crypto_256_or_384);
        calculate_sha(data, data_size, crypto_256_or_384);
    }
    else
    {
        set_dma_interconnection(addr, data_size, crypto_256_or_384, dma_mode);
    }

    for (alt_u32 word_i = 0; word_i < (sha_length / 4); word_i++)
    {
        dest_addr[word_i] = IORD(CRYPTO_DATA_SHA_ADDR((sha_length / 4) - 1 - word_i), 0);
    }
}

/**
 * @brief This function verifies the expected SHA256 hash for a given data.
 * It sends all the inputs to the crypto block.
 * Once the crypto block is done, compare the expected hash against the calculated hash.
 *
 * @param expected hash : the stored hash to be compared after sha operation is done
 * @param addr : the address of the data to be hashed
 * @param data : pointer to the data to be hashed
 * @param data_size : size of data in bytes
 * @param crypto_256_or_384 : sha-256 or sha-384 mode selection
 * @param dma_mode : dma ufm/ dma spi
 *
 * @return 1 if expected hash matches the calculated hash; 0, otherwise.
 */
static alt_u32 verify_sha(const alt_u32* expected_hash, const alt_u32 addr, const alt_u32* data, alt_u32 data_size, CRYPTO_MODE crypto_256_or_384, DMA_MODE dma_mode)
{
    alt_u32 sha_length = (crypto_256_or_384 == CRYPTO_384_MODE) ? SHA384_LENGTH : SHA256_LENGTH;
    if (dma_mode == DISENGAGE_DMA)
    {
        reset_crypto(crypto_256_or_384);
        calculate_sha(data, data_size, crypto_256_or_384);
    }
    else
    {
        set_dma_interconnection(addr, data_size, crypto_256_or_384, dma_mode);
    }

    // Go through CSR word offset 0x08-0x0f to check the expected hash against the calculated hash.
    for (alt_u32 word_i = 0; word_i < (sha_length / 4); word_i++)
    {
        if (IORD(CRYPTO_DATA_SHA_ADDR((sha_length / 4) - 1 - word_i), 0) != expected_hash[word_i])
        {
            return 0;
        }
    }
    return 1;
}

/*
 * @brief This function computes the hash of two messages and compare if they match.
 * This function do not check the data size of both messages.
 *
 * @param data_1 : message 1 to hash
 * @param data_1_size : message 1 size
 * @param data_2 : message 2 to hash
 * @param data_2_size : message 2 size
 *
 * @return 1 if hash match
 *
 */

static alt_u32 check_matched_hash(alt_u32* data_1, alt_u32 data_1_size, alt_u32* data_2, alt_u32 data_2_size, CRYPTO_MODE crypto_256_or_384)
{
    alt_u32 sha_length = (crypto_256_or_384 == CRYPTO_384_MODE) ? SHA384_LENGTH : SHA256_LENGTH;
    alt_u32 data_1_hash[SHA384_LENGTH];
    alt_u32 data_2_hash[SHA384_LENGTH];

    calculate_and_save_sha((alt_u32*)data_1_hash, 0, data_1, data_1_size, crypto_256_or_384, DISENGAGE_DMA);
    calculate_and_save_sha((alt_u32*)data_2_hash, 0, data_2, data_2_size, crypto_256_or_384, DISENGAGE_DMA);

    return is_data_matching_stored_data(data_1_hash, data_2_hash, sha_length);
}

/**
 * @brief This function generally can be used to compare between two different values stored in their respective arrays.
 * For our crypto operation, this function is used to compare the calculated hmac with the constant ECDSA parameter N.
 *
 * The ECDSA parameter N would most likely be fixed for our operation and the hmac value varies for each computation.
 * This function checks all elements of both arrays, comparing them in the order of MSB to LSB.
 * For hmac to be valid and to comply with the requirements set by the NIST, it's value must not be zero
 * and must be smaller than 'N' (between 1 to N-1).
 *
 * @param hmac : The computed hmac
 * @param ec_n_ptr : Pointer to an array storing the value of ECDSA parameter N
 * @param crypto_256_or_384 : Crypto mode selection between SHA256 or SHA384
 *
 * @return 1 if the hmac value is smaller than ECDSA parameter 'N' value.
 * Otherwise return 0 if hmac value is zero or bigger than 'N'.
 */
static alt_u32 check_hmac_validity (alt_u32* hmac, const alt_u32* ec_n_ptr, CRYPTO_MODE crypto_256_or_384)
{
    alt_u32 prev_indicator = 0;
    alt_u32 word = (crypto_256_or_384 == CRYPTO_384_MODE) ? (SHA384_LENGTH/4) : (SHA256_LENGTH/4);
    alt_u32 max_word = word;
    alt_u32 zero_counter = word;
    alt_u32 equal_counter = word;

    while (word != 0)
    {
        alt_u32 leftover_word = max_word - word;

        if (hmac[leftover_word] < ec_n_ptr[leftover_word])
        {
            prev_indicator = 1;
        }
        else if (hmac[leftover_word] > ec_n_ptr[leftover_word])
        {
            if (!prev_indicator)
            {
                return 0;
            }
        }
        else if (hmac[leftover_word] == ec_n_ptr[leftover_word])
        {
            equal_counter--;
            if (equal_counter == 0)
            {
                return 0;
            }
        }

        if (hmac[leftover_word] == 0)
        {
            zero_counter--;
            if (zero_counter == 0)
            {
                return 0;
            }
        }
        word--;
    }
    return 1;
}

/**
 * @brief This function extracts the random data from the true random generator of entropy circuit and save it.
 *
 * Prior to any operation, the function resets the entropy circuit and wait for the entropy source FIFO
 * to be filled up before extracting those data, then saves them somewhere.
 *
 * @param entropy_sample : The destination address in which the entropy data is saved.
 * @param crypto_256_or_384 : Crypto mode selection
 *
 * @return 0 if failed; 1 otherwise
 */
static alt_u32 extract_entropy_data (alt_u32* entropy_sample, alt_u32 sample_size)
{
    // Perform a reset to the entropy block
    // When reading entropy reading, the crypto mode is not important
    alt_u8 max_entropy_failure = 0;
    reset_entropy(CRYPTO_384_MODE);

    // Wait for the fifo to be filled up with the data
    while (1)
    {
        reset_hw_watchdog();
        if (check_bit(CRYPTO_CSR_ADDR, CRYPTO_CSR_ENTROPY_SOURCE_FIFO_FULL_OFST))
        {
            // If the entropy data passes health test, safe to use the data
            if (!check_bit(CRYPTO_CSR_ADDR, CRYPTO_CSR_ENTROPY_SOURCE_HEALTH_TEST_ERROR_MSK_OFST))
            {
                break;
            }
            else
            {
                // Reset entropy and wait for re-accumulation of entropy data
                // If max acceptable health failure has occurred, entropy data is rejected
                if (max_entropy_failure == CRYPTO_MAX_ENTROPY_DATA_HEALTH_FAILURE)
                {
                    return 0;
                }
                max_entropy_failure++;
                reset_entropy(CRYPTO_384_MODE);
            }
        }
    }

    // Read off the data from the entropy source and save it.
    for (alt_u32 word_i = 0; word_i < sample_size; word_i++)
    {
    	entropy_sample[word_i] = IORD(CRYPTO_ENTROPY_SOURCE_OUTPUT_ADDR(0), 0);
    }
    return 1;
}

/**
 * @brief This function computes the Keyed-Hash Message Authentication Code (HMAC) using the extracted entropy sample data.
 * Generally, the function takes the output of a cryptographic random number generator as input to key derivation.
 * The cryptographic application used is deterministic, among many other secured application types.
 *
 * This function implements the HMAC algorithm based on the specification as published by the NIST which is a recommendation
 * for any application using approved hash algorithms.
 * The following operation is performed to compute a MAC:
 *
 *       ************************************************************************
 *       * MAC(text) = HMAC(K, text) = H((K0 ^ opad) || H((K0 ^ ipad) || text)) *
 *       ************************************************************************
 *
 *       Where:
 *       H : An approved hash function
 *       ipad : Inner pad consisting of byte x'36' repeated B times
 *       opad : Onner pad consisting of byte x'5c' repeated B times
 *       B : Block size in bytes
 *       K : Secret key shared between originator and recipient
 *       K0 : Key K after forming B byte length
 *       text : Data which HMAC is calculated, in our case is the entropy sample data
 *
 * Step 1: If length of K = B, set K0 = K.
 * Step 2: If length of K > B, hash K and append the result with 'zeros' to form B-byte string.
 * Step 3: If length of K is < B, append 'zeros' to form B-byte string.
 * Step 4: Exclusive-Or K0 with ipad to form B-byte string.
 * Step 5: Append string of 'text' to string resulted from Step 4.
 * Step 6: Hash the string obtained from Step 5 to obtained a shorter length string.
 * Step 7: Exclusive-Or the result from step 6 with opad to form B-byte string.
 * Step 8: Append string from step 6 to step 7.
 * Step 9: Hash the string obtained from step 8.
 *
 * @param hmac_ptr : The destination address to save the computed HMAC
 * @param entropy_sample_output_ptr : The extracted entropy sample data
 * @param k0_xor_ipad : Constant from step 4
 * @param k0_xor_opad : Constant from step 7
 * @param crypto_256_or_384 : crypto mode selection
 *
 * @return none
 *
 */
static void compute_hmac_algorithm (alt_u32* hmac_ptr, alt_u32* entropy_sample_output_ptr, alt_u32* k0_xor_ipad, alt_u32* k0_xor_opad, CRYPTO_MODE crypto_256_or_384)
{
    // Initialization
    alt_u32 k0_xor_ipad_text_array[PFR_K0_XOR_IPAD_MAX_384_SIZE] = {0};
    alt_u32 k0_xor_opad_ipad_text_array[PFR_K0_XOR_OPAD_IPAD_MAX_384_SIZE] = {0};

    // Assign pointers to the array
    alt_u32* k0_xor_ipad_text_array_ptr = k0_xor_ipad_text_array;
    alt_u32* k0_xor_opad_ipad_text_array_ptr = k0_xor_opad_ipad_text_array;

    alt_u32 crypto_block_size_in_words = (crypto_256_or_384 == CRYPTO_384_MODE) ? (CRYPTO_BLOCK_SIZE_FOR_384/4) : (CRYPTO_BLOCK_SIZE_FOR_256/4);
    alt_u32 entropy_extraction_in_words = (crypto_256_or_384 == CRYPTO_384_MODE) ? PFR_ENTROPY_EXTRACTION_384_SIZE : PFR_ENTROPY_EXTRACTION_256_SIZE;
    alt_u32 k0_xor_ipad_byte_size = (crypto_256_or_384 == CRYPTO_384_MODE) ? (PFR_K0_XOR_IPAD_MAX_384_SIZE*4) : (PFR_K0_XOR_IPAD_MAX_256_SIZE*4);
    alt_u32 k0_xor_opad_ipad_byte_size = (crypto_256_or_384 == CRYPTO_384_MODE) ? (PFR_K0_XOR_OPAD_IPAD_MAX_384_SIZE*4) : (PFR_K0_XOR_OPAD_IPAD_MAX_256_SIZE*4);
    alt_u32 sha_word_length = (crypto_256_or_384 == CRYPTO_384_MODE) ? (SHA384_LENGTH/4) : (SHA256_LENGTH/4);

    // Extract the precomputed constant for hmac calculation.
    for (alt_u32 i = 0; i < crypto_block_size_in_words; i++)
    {
        k0_xor_ipad_text_array_ptr[i] = k0_xor_ipad[i];
    }

    alt_u32 j = 0;

    // Extract the entropy data.
    for (alt_u32 i = crypto_block_size_in_words; i < crypto_block_size_in_words + entropy_extraction_in_words; i++)
    {
        k0_xor_ipad_text_array_ptr[i] = entropy_sample_output_ptr[j];
        j++;
    }

    j = 0;

    // Compute the first hash and save it to an array.
    calculate_and_save_sha(hmac_ptr, 0, k0_xor_ipad_text_array_ptr, k0_xor_ipad_byte_size, crypto_256_or_384, DISENGAGE_DMA);

    // Extract and store the second pre-computed constant for hmac calculation.
    for (alt_u32 i = 0; i < crypto_block_size_in_words; i++)
    {
        k0_xor_opad_ipad_text_array_ptr[i] = k0_xor_opad[i];
    }

    // Append with previous hash.
    for (alt_u32 i = crypto_block_size_in_words; i < crypto_block_size_in_words + sha_word_length; i++)
    {
        k0_xor_opad_ipad_text_array_ptr[i] = hmac_ptr[j];
        j++;
    }

    j = 0;

    // Compute the new hash and save it.
    // This will be regarded as the HMAC value and will then be checked for its validity.
    calculate_and_save_sha(hmac_ptr, 0, k0_xor_opad_ipad_text_array_ptr, k0_xor_opad_ipad_byte_size, crypto_256_or_384, DISENGAGE_DMA);
}

/**
 * @brief This function generates public key using the pre-determined private key as input.
 * Prior to generating the public key, the hmac value has to be computed first using the extracted entropy data as input.
 * The requirement for the hmac value to be valid prior to generating is :
 *     1)  HMAC < EC_N
 *     2)  HMAC != 0
 *     3)  HMAC != EC_N
 * Else, new entropy sample is extracted and HMAC is recomputed.
 * When HMAC is valid, this function generates the public key storing them in the given array.
 *
 * @param pubkey_cx : Q(x) public key
 * @param publey_cy : Q(y) public key
 * @param priv_key : private key
 * @param crypto_256_or_384 : crypto mode
 *
 * @return 0 if no public key generated; 1 otherwise
 */
static alt_u32 generate_pubkey (alt_u32* pubkey_cx, alt_u32* pubkey_cy, const alt_u32* priv_key, CRYPTO_MODE crypto_256_or_384)
{
    // Initialising pointers and arrays
    alt_u32* hmac_ptr = (alt_u32*)HMAC_1_384;
    alt_u32* xor_k0_and_ipad_ptr = (alt_u32*)XOR_K0_AND_IPAD_384;
    alt_u32* xor_k0_and_opad_ptr = (alt_u32*)XOR_K0_AND_OPAD_384;
    const alt_u32* ec_ax_ptr = (const alt_u32*)CRYPTO_EC_AX_384;
    const alt_u32* ec_ay_ptr = (const alt_u32*)CRYPTO_EC_AY_384;
    const alt_u32* ec_p_ptr = (const alt_u32*)CRYPTO_EC_P_384;
    const alt_u32* ec_a_ptr = (const alt_u32*)CRYPTO_EC_A_384;
    const alt_u32* ec_n_ptr = (const alt_u32*)CRYPTO_EC_N_384;

    if (crypto_256_or_384 == CRYPTO_256_MODE)
    {
        hmac_ptr = (alt_u32*)HMAC_1_256;
        xor_k0_and_ipad_ptr = (alt_u32*)XOR_K0_AND_IPAD_256;
        xor_k0_and_opad_ptr = (alt_u32*)XOR_K0_AND_OPAD_256;
        ec_ax_ptr = (const alt_u32*)CRYPTO_EC_AX_256;
        ec_ay_ptr = (const alt_u32*)CRYPTO_EC_AY_256;
        ec_p_ptr = (const alt_u32*)CRYPTO_EC_P_256;
        ec_a_ptr = (const alt_u32*)CRYPTO_EC_A_256;
        ec_n_ptr = (const alt_u32*)CRYPTO_EC_N_256;
    }

    alt_u32 length = (crypto_256_or_384 == CRYPTO_384_MODE) ? SHA384_LENGTH : SHA256_LENGTH;
    alt_u32 entropy_sample_size = (crypto_256_or_384 == CRYPTO_384_MODE) ? SHA384_HMAC_ENTROPY : SHA256_HMAC_ENTROPY;

    // Start by extracting entropy data which saves the data to an array.
    if (!extract_entropy_data((alt_u32*)entropy_sample_output, entropy_sample_size))
    {
        return 0;
    }

    // Compute the hmac algorithm using the stored entropy data.
    compute_hmac_algorithm(hmac_ptr, (alt_u32*)entropy_sample_output, xor_k0_and_ipad_ptr, xor_k0_and_opad_ptr, crypto_256_or_384);

    // Check if the computed hmac is valid. Please refer to the comment above for the conditions.
    // If not valid, extract new entropy data and recompute hmac.
    // Repeat until valid.
    while (!check_hmac_validity(hmac_ptr, ec_n_ptr, crypto_256_or_384))
    {
        reset_hw_watchdog();
        if (!extract_entropy_data((alt_u32*)entropy_sample_output, entropy_sample_size))
        {
            return 0;
        }
        compute_hmac_algorithm(hmac_ptr, (alt_u32*)entropy_sample_output, xor_k0_and_ipad_ptr, xor_k0_and_opad_ptr, crypto_256_or_384);
    }

    // Perform a crypto reset before any data transaction.
    reset_crypto(crypto_256_or_384);

    //Step 1: Set ECDSA start
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_START_MSK | crypto_256_or_384);

    //Step 2: Write AX from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_AX | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), ec_ax_ptr, length);

    //Step 3: Write AY from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_AY | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), ec_ay_ptr, length);

    //Step 4: Write P from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_P | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), ec_p_ptr, length);

    //Step 5: Write A from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_A | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), ec_a_ptr, length);

    //Step 6: Write N from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_N | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), ec_n_ptr, length);

    //Step 7: Write private key dA to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_D_KEYGEN | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), priv_key, length);

    //Step 8: Write random number LAMBDA to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_LAMBDA | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), hmac_ptr, length);

    //Step 9: Write generate public key instruction to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_GEN_PUB_KEY | crypto_256_or_384);

    //Step 10: Polling ECDSA valid bit
    while (!check_bit(CRYPTO_CSR_ADDR, CRYPTO_CSR_ECDSA_DOUT_VALID_OFST))
    {
	    reset_hw_watchdog();
    }

    for (alt_u32 word_i = 0; word_i < (length / 4); word_i++)
    {
        pubkey_cx[word_i] = IORD(CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR((length / 4) -1 - word_i), 0);
        pubkey_cy[word_i] = IORD(CRYPTO_ECDSA_DATA_CY_OUTPUT_ADDR((length / 4) -1 - word_i), 0);
    }

    return 1;
}

/**
 * @brief This function verifies the expected SHA256 hash and also expected SHA384 hash
 * and EC signature (for prime256 and secp384) for a given data. It sends all the inputs
 * and NIST P-256 curve constants or NIST 384 curve constants to the crypto block for calculation.
 *
 * The function calculate_sha is called initially to obtain the hash output of the data
 * and this hash data will be transfered to the ecdsa block to be used for signature
 * verification.
 *
 * Prior to sending in the ec constant, it is a requirement to send in the opcode for that
 * particular constants. Then the constant is copied word by word through the custom version
 * incremental memcpy.
 *
 * @param cx : Q(x) public key
 * @param cy : Q(y) public key
 * @param sig_r : signature r
 * @param sig_s : signature s
 * @param addr : address of the data to be hashed
 * @param data : data to be hashed
 * @param data_size : size of data to be hashed in bytes
 * @param crypto_256_or_384 : sha-256 or sha-384 mode selection
 * @param dma_mode : dma ufm or dma spi
 *
 * @return 1 if EC and SHA are good; 0, otherwise.
 */
static alt_u32 verify_ecdsa_and_sha (const alt_u32* cx, const alt_u32* cy,
        const alt_u32* sig_r, const alt_u32* sig_s, const alt_u32 addr, const alt_u32* data, alt_u32 data_size, CRYPTO_MODE crypto_256_or_384, DMA_MODE dma_mode)
{
    // Initialising pointers and arrays
    const alt_u32* ec_ax_ptr = (const alt_u32*)CRYPTO_EC_AX_384;
    const alt_u32* ec_ay_ptr = (const alt_u32*)CRYPTO_EC_AY_384;
    const alt_u32* ec_p_ptr = (const alt_u32*)CRYPTO_EC_P_384;
    const alt_u32* ec_a_ptr = (const alt_u32*)CRYPTO_EC_A_384;
    const alt_u32* ec_n_ptr = (const alt_u32*)CRYPTO_EC_N_384;

    if (dma_mode == DISENGAGE_DMA)
    {
        reset_crypto(crypto_256_or_384);
        calculate_sha (data, data_size, crypto_256_or_384);
    }
    else
    {
        set_dma_interconnection(addr, data_size, crypto_256_or_384, dma_mode);
    }

    if (crypto_256_or_384 == CRYPTO_256_MODE)
    {
        ec_ax_ptr = (const alt_u32*)CRYPTO_EC_AX_256;
        ec_ay_ptr = (const alt_u32*)CRYPTO_EC_AY_256;
        ec_p_ptr = (const alt_u32*)CRYPTO_EC_P_256;
        ec_a_ptr = (const alt_u32*)CRYPTO_EC_A_256;
        ec_n_ptr = (const alt_u32*)CRYPTO_EC_N_256;
    }

    alt_u32 length = (crypto_256_or_384 == CRYPTO_384_MODE) ? SHA384_LENGTH : SHA256_LENGTH;

    //Step 1: Set ECDSA start
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_START_MSK | crypto_256_or_384);

    //Step 2: Write AX from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_AX | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), ec_ax_ptr, length);

    //Step 3: Write AY from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_AY | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), ec_ay_ptr, length);

    //Step 4: Write BX from flash to CSR (also known as cx)
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_BX | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), cx, length);

    //Step 5: Write BY from flash to CSR (also known as cy)
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_BY | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), cy, length);

    //Step 6: Write P from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_P | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), ec_p_ptr, length);

    //Step 7: Write A from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_A | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), ec_a_ptr, length);

    //Step 8: Write N from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_N | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), ec_n_ptr, length);

    //Step 9: Write R from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_R | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), sig_r, length);

    //Step 10: Write S from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_S | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), sig_s, length);

    //Step 11: Write E from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_E | crypto_256_or_384);
    for (alt_u32 word_i = 0; word_i < (length / 4); word_i++)
    {
	    IOWR(CRYPTO_ECDSA_DATA_INPUT_ADDR(word_i), 0, IORD(CRYPTO_DATA_SHA_ADDR(word_i), 0));
    }

    //Step 12: Write ECDSA validate
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_VALIDATE | crypto_256_or_384);

    //Step 13: Polling ECDSA valid bit
    while (!check_bit(CRYPTO_CSR_ADDR, CRYPTO_CSR_ECDSA_DOUT_VALID_OFST))
    {
	    reset_hw_watchdog();
    }

    return verify_cx (length);
}

/**
 * @brief This function can be used to generate either ecdsa-256 or ecdsa-384 signature from
 * the message digest and key components. Just like public key generation, the HMAC is computed
 * using certified entropy source data and checked for its validity. Two HMAC values will be
 * used for K and LAMBDA accordingly.The randomness of the HMAC is important for the generation of the signature.
 *
 * This function will validate both HMAC values before injecting them into the crypto block.
 * The signature generated should be different every time this function is called due to the
 * randomness of HMAC.
 *
 * Prior to sending in the ec constant, it is a requirement to send in the opcode for that
 * particular constants. Then the constant is copied word by word through the custom version
 * incremental memcpy.
 *
 * @param sig_r : Signature R
 * @param sig_s : Signature S
 * @param data : Message
 * @param data_size : Message length
 * @param cx : Q(x) public key
 * @param cy : Q(y) public key
 * @param priv_key : Private key
 * @param crypto_256_or_384 : Crypto mode
 *
 * @return 0 if failed; 1 otherwise
 */
static alt_u32 generate_ecdsa_signature (alt_u32* sig_r, alt_u32* sig_s, const alt_u32* data, alt_u32 data_size, const alt_u32* cx, const alt_u32* priv_key, CRYPTO_MODE crypto_256_or_384)
{
    // Initialising pointers and arrays
    alt_u32* hmac_ptr = (alt_u32*)HMAC_1_384;
    alt_u32* hmac_2_ptr = (alt_u32*)HMAC_2_384;
    alt_u32* xor_k0_and_ipad_ptr = (alt_u32*)XOR_K0_AND_IPAD_384;
    alt_u32* xor_k0_and_opad_ptr = (alt_u32*)XOR_K0_AND_OPAD_384;
    const alt_u32* ec_ax_ptr = (const alt_u32*)CRYPTO_EC_AX_384;
    const alt_u32* ec_ay_ptr = (const alt_u32*)CRYPTO_EC_AY_384;
    const alt_u32* ec_p_ptr = (const alt_u32*)CRYPTO_EC_P_384;
    const alt_u32* ec_a_ptr = (const alt_u32*)CRYPTO_EC_A_384;
    const alt_u32* ec_n_ptr = (const alt_u32*)CRYPTO_EC_N_384;

    if (crypto_256_or_384 == CRYPTO_256_MODE)
    {
        hmac_ptr = (alt_u32*)HMAC_1_256;
        hmac_2_ptr = (alt_u32*)HMAC_2_256;
        xor_k0_and_ipad_ptr = (alt_u32*)XOR_K0_AND_IPAD_256;
        xor_k0_and_opad_ptr = (alt_u32*)XOR_K0_AND_OPAD_256;
        ec_ax_ptr = (const alt_u32*)CRYPTO_EC_AX_256;
        ec_ay_ptr = (const alt_u32*)CRYPTO_EC_AY_256;
        ec_p_ptr = (const alt_u32*)CRYPTO_EC_P_256;
        ec_a_ptr = (const alt_u32*)CRYPTO_EC_A_256;
        ec_n_ptr = (const alt_u32*)CRYPTO_EC_N_256;
    }

    alt_u32 length = (crypto_256_or_384 == CRYPTO_384_MODE) ? SHA384_LENGTH : SHA256_LENGTH;
    alt_u32 entropy_sample_size = (crypto_256_or_384 == CRYPTO_384_MODE) ? SHA384_HMAC_ENTROPY : SHA256_HMAC_ENTROPY;

    // This is done for K first
    // Start by extracting entropy data which saves the data to an array.
    if (!extract_entropy_data((alt_u32*)entropy_sample_output, entropy_sample_size))
    {
        return 0;
    }

    // Compute the hmac algorithm using the stored entropy data.
    compute_hmac_algorithm(hmac_ptr, (alt_u32*)entropy_sample_output, xor_k0_and_ipad_ptr, xor_k0_and_opad_ptr, crypto_256_or_384);

    // Check if the computed hmac is valid. Please refer to the comment above for the conditions.
    // If not valid, extract new entropy data and recompute hmac.
    // Repeat until valid.
    while (!check_hmac_validity(hmac_ptr, ec_n_ptr, crypto_256_or_384))
    {
        reset_hw_watchdog();
        if (!extract_entropy_data((alt_u32*)entropy_sample_output, entropy_sample_size))
        {
            return 0;
        }
        compute_hmac_algorithm(hmac_ptr, (alt_u32*)entropy_sample_output, xor_k0_and_ipad_ptr, xor_k0_and_opad_ptr, crypto_256_or_384);
    }

    // Repeat the above steps for LAMBDA
    // Start by extracting entropy data which saves the data to an array.
    if (!extract_entropy_data((alt_u32*)entropy_sample_output, entropy_sample_size))
    {
        return 0;
    }

    // Compute the hmac algorithm using the stored entropy data.
    compute_hmac_algorithm(hmac_2_ptr, (alt_u32*)entropy_sample_output, xor_k0_and_ipad_ptr, xor_k0_and_opad_ptr, crypto_256_or_384);

    // Check if the computed hmac is valid. Please refer to the comment above for the conditions.
    // If not valid, extract new entropy data and recompute hmac.
    // Repeat until valid.
    while (!check_hmac_validity(hmac_2_ptr, ec_n_ptr, crypto_256_or_384))
    {
        reset_hw_watchdog();
        if (!extract_entropy_data((alt_u32*)entropy_sample_output, entropy_sample_size))
        {
            return 0;
        }
        compute_hmac_algorithm(hmac_2_ptr, (alt_u32*)entropy_sample_output, xor_k0_and_ipad_ptr, xor_k0_and_opad_ptr, crypto_256_or_384);
    }

    // Perform a crypto reset before any data transaction.
    reset_crypto(crypto_256_or_384);

    // Calculate the message hash
    calculate_sha (data, data_size, crypto_256_or_384);

    //Step 1: Set ECDSA start
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_START_MSK | crypto_256_or_384);

    //Step 2: Write AX from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_AX | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), ec_ax_ptr, length);

    //Step 3: Write AY from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_AY | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), ec_ay_ptr, length);

    //Step 4: Write BX from flash to CSR (also known as cx)
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_BX | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), cx, length);

    //Step 5: Write private key dA to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_D_SIGN | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), priv_key, length);

    //Step 6: Write P from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_P | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), ec_p_ptr, length);

    //Step 7: Write A from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_A | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), ec_a_ptr, length);

    //Step 8: Write N from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_N | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), ec_n_ptr, length);

    //Step 9: Write random number K to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_K | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), hmac_ptr, length);

    //Step 10: Write random number LAMBDA to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_LAMBDA | crypto_256_or_384);
    alt_u32_memcpy_for_crypto(CRYPTO_ECDSA_DATA_INPUT_ADDR(0), hmac_2_ptr, length);

    //Step 11: Write E from flash to CSR
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_WRITE_E | crypto_256_or_384);

    for (alt_u32 word_i = 0; word_i < (length / 4); word_i++)
    {
	    IOWR(CRYPTO_ECDSA_DATA_INPUT_ADDR(word_i), 0, IORD(CRYPTO_DATA_SHA_ADDR(word_i), 0));
    }

    //Step 12: Write ECDSA sign
    IOWR_32DIRECT(CRYPTO_CSR_ADDR, 0, CRYPTO_CSR_ECDSA_INSTRUCTION_VALID_MSK | ECDSA_INSTR_SIGN | crypto_256_or_384);

    //Step 13: Polling ECDSA valid bit
    while (!check_bit(CRYPTO_CSR_ADDR, CRYPTO_CSR_ECDSA_DOUT_VALID_OFST))
    {
	    reset_hw_watchdog();
    }

    // Save the signature into arrays
    for (alt_u32 word_i = 0; word_i < (length / 4); word_i++)
    {
        sig_r[word_i] = IORD(CRYPTO_ECDSA_DATA_CX_OUTPUT_ADDR((length / 4) -1 - word_i), 0);
        sig_s[word_i] = IORD(CRYPTO_ECDSA_DATA_CY_OUTPUT_ADDR((length / 4) -1 - word_i), 0);
    }

    return 1;
}
#endif /* EAGLESTREAM_INC_CRYPTO_H_ */
