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
 * @file spi_common.h
 * @brief SPI-related macros and enums.
 */

#ifndef EAGLESTREAM_INC_SPI_COMMON_H
#define EAGLESTREAM_INC_SPI_COMMON_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#define SPI_CONTROL_1_CSR_BASE_ADDR \
        __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_SPI_FILTER_CSR_AVMM_BRIDGE_0_BASE, 0)

/**
 * CSR interface of SPI control block
 */
typedef enum
{
    SPI_CONTROL_1_CSR_CONTROL_OFST                      = 0,
    SPI_CONTROL_1_CSR_BAUD_RATE_OFST                    = 1,
    SPI_CONTROL_1_CSR_CS_DELAY_OFST                     = 2,
    SPI_CONTROL_1_CSR_READ_CAPTURING_OFST               = 3,
    SPI_CONTROL_1_CSR_CS_OPERATING_PROTOCOLS_OFST       = 4,
    SPI_CONTROL_1_CSR_CS_READ_INSTRUCTION_OFST          = 5,
    SPI_CONTROL_1_CSR_CS_WRITE_INSTRUCTION_OFST         = 6,
    SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_SETTING_OFST     = 7,
    SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_CONTROL_OFST     = 8,
    SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_ADDRESS_OFST     = 9,
    SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_WRITE_DATA0_OFST = 10,
    SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_WRITE_DATA1_OFST = 11,
    SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST  = 12,
    SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA1_OFST  = 13,
} SPI_CONTROL_1_CSR_OFFSET_ENUM;

/**
 * SPI commands that this PFR system uses
 */
typedef enum
{
    // Write enable
    // This needs to be sent prior to each write/program/erase command
    SPI_CMD_WRITE_ENABLE = 0x06,
    // Enter 4-byte address mode
    SPI_CMD_ENTER_4B_ADDR_MODE = 0xB7,
    // Exit 4-byte address mode
    SPI_CMD_EXIT_4B_ADDR_MODE = 0xE9,
    // 4KB sector erase
    SPI_CMD_4KB_SECTOR_ERASE = 0x20,
    // 64KB sector erase
    SPI_CMD_64KB_SECTOR_ERASE = 0xD8,
    // Read status reg
    SPI_CMD_READ_STATUS_REG = 0x05,
    // Read configuration reg
    SPI_CMD_READ_CONFIGURATION_REG = 0x15,
    // Write status/configuration reg
    SPI_CMD_WRITE_STATUS_AND_CONFIGURATION_REG = 0x01,
    // Read flash device chip ID
    SPI_CMD_READ_ID = 0x9F,
} SPI_COMMAND_ENUM;

/**
 * SPI status register bit mask
 */
typedef enum
{
    SPI_STATUS_WIP_BIT_MASK  = 0b1,
    SPI_STATUS_WEL_BIT_MASK  = 0b10,
    SPI_STATUS_BP0_BIT_MASK  = 0b100,
    SPI_STATUS_BP1_BIT_MASK  = 0b1000,
    SPI_STATUS_BP2_BIT_MASK  = 0b10000,
    SPI_STATUS_BP3_BIT_MASK  = 0b100000,
    SPI_STATUS_QE_BIT_MASK   = 0b1000000,
    SPI_STATUS_SRWD_BIT_MASK = 0b10000000,
} SPI_STATUS_REG_BIT_MASK;

#endif /* EAGLESTREAM_INC_SPI_COMMON_H */
