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
 * @file rfnvram.h
 * @brief Define macros and structs that eases RFNVRAM IP access.
 */

#ifndef EAGLESTREAM_INC_RFNVRAM_H_
#define EAGLESTREAM_INC_RFNVRAM_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"


#define U_RFNVRAM_SMBUS_MASTER_ADDR __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_RFNVRAM_SMBUS_MASTER_BASE, 0)
#define RFNVRAM_RX_FIFO U_RFNVRAM_SMBUS_MASTER_ADDR + 1
#define RFNVRAM_TX_FIFO_BYTES_LEFT U_RFNVRAM_SMBUS_MASTER_ADDR + 6
#define RFNVRAM_RX_FIFO_BYTES_LEFT U_RFNVRAM_SMBUS_MASTER_ADDR + 7
#define RFNVRAM_SMBUS_ADDR 0xDC
#define RFNVRAM_INTERNAL_SIZE 1088
#define RFNVRAM_IDLE_MASK 0x100

// The RF ACCESS CONTROL register stored in offset 0x014:0x015 inside the RFID component
#define RFNVRAM_RF_ACCESS_CONTROL_OFFSET 0x0014
#define RFNVRAM_RF_ACCESS_CONTROL_MSB_OFFSET ((RFNVRAM_RF_ACCESS_CONTROL_OFFSET & 0xff00) >> 8)
#define RFNVRAM_RF_ACCESS_CONTROL_LSB_OFFSET (RFNVRAM_RF_ACCESS_CONTROL_OFFSET & 0xff)
#define RFNVRAM_RF_ACCESS_CONTROL_LENGTH 2
#define RFNVRAM_RF_ACCESS_CONTROL_DCI_RF_EN_MASK 0x4

// The PIT ID stored in the external RFNVRAM is written at offset 0x0160:0x0167 inside the RFID component.
#define RFNVRAM_PIT_ID_OFFSET 0x0160
#define RFNVRAM_PIT_ID_MSB_OFFSET ((RFNVRAM_PIT_ID_OFFSET & 0xff00) >> 8)
#define RFNVRAM_PIT_ID_LSB_OFFSET (RFNVRAM_PIT_ID_OFFSET & 0xff)
#define RFNVRAM_PIT_ID_LENGTH 8


#define RFNVRAM_CHIP_SMBUS_PORT    0b00
#define PCH_SMBUS_PORT             0b01
#define BMC_SMBUS_PORT             0b10
#define PCIE_SMBUS_PORT            0b11

#define RFNVRAM_CHIP_SMBUS_PORT_ADDR  0b0000
#define PCH_SMBUS_PORT_ADDR           0b1000
#define BMC_SMBUS_PORT_ADDR           0b0100
#define PCIE_SMBUS_PORT_ADDR          0b1100

#define RFVNRAM_MCTP_CONTROL_PEC_ENABLED    0b10
#define RFVNRAM_MCTP_CONTROL_SMBUS_MASTER_RESET    0b10000
#define RFNVRAM_MCTP_CONTROL U_RFNVRAM_SMBUS_MASTER_ADDR + 2
#define RFNVRAM_MCTP_STATUS U_RFNVRAM_SMBUS_MASTER_ADDR + 3
#define RFNVRAM_MCTP_SMBUS_SLAVE_PORT_ID U_RFNVRAM_SMBUS_MASTER_ADDR + 4


#endif /* EAGLESTREAM_INC_RFNVRAM_H_ */
