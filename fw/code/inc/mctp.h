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
 * @file mctp.h
 * @brief MCTP protocol through SMBus Host Mailbox.
 */

#ifndef EAGLESTREAM_INC_MCTP_H_
#define EAGLESTREAM_INC_MCTP_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"



#define MCTP_COMMAND_CODE 0x0f
// SPDM type
#define MCTP_MESSAGE_TYPE 0x05
// Define max byte count of SPDM message without MCTP header
#define MAX_BYTE_COUNT 249
#define MCTP_HEADER_AFTER_BYTE_COUNT_SIZE 6
#define MAX_MCTP_I2C_BUFFER_SIZE 1024
#define MAX_MCTP_I2C_PKT_IDX 4

// General MCTP usage
#define MEDIUM_MESSAGE_MAX_BUFFER_SIZE 0x200

#define MCTP_PKT_ERROR_BIT 0b10000
#define MCTP_PKT_AVAIL_BIT 1

#define VALID_PKT        1
#define INVALID_PKT      0

// Byte 1
#define DEST_SLAVE_ADDR_WR 0
// Byte 4 for BMC or PCIE/PCH smbus mailbox address
#define SRC_SLAVE_ADDR_WR   0x38 << 1 | 1
#define SRC_SLAVE_ADDR_PCIE 0x70 << 1 | 1
// Byte 5
#define MCTP_HDR_VER 0b0001
#define MCTP_RESERVED 0b0 << 4

// Byte 8
#define INTERMEDIARY_PKT   0
#define LAST_PKT           0b01000000  //EOM
#define FIRST_PKT          0b10000000  //SOM
#define FIRST_AND_LAST_PKT (FIRST_PKT | LAST_PKT)  // SOM and EOM
#define PKT_SEQ_NO_MSK 0b110000
#define MSG_TAG 0b000

//Byte 9
#define MSG_IC 0b1 << 7

// Medium buffer to hold received messages
typedef struct
{
    alt_u32 buffer_size;
    alt_u32 buffer[MEDIUM_MESSAGE_MAX_BUFFER_SIZE/4];
} MEDIUM_APPENDED_BUFFER;

// MCTP Physical Transport Binding Identifier
typedef enum
{
    SMBUS = 0X01
} MCTP_PHY_TRANS_BIND_ID;

// MCTP packet common structure
// This field is filtered by RTL
typedef struct __attribute__((__packed__))
{
    alt_u8 dest_addr;
    alt_u8 command_code;
    alt_u8 byte_count;
} MCTP_HEADER_FILTERED;

// Common MCTP transport header for i2c
typedef struct __attribute__((__packed__))
{
    alt_u8 hdr_version;
    alt_u8 dest_eid;
    alt_u8 src_eid;
    alt_u8 pkt_info;
    alt_u8 msg_type;
} MCTP_COMMON_HEADER;

// As part of CPLD RoT design, this header will be received by FW
typedef struct __attribute__((__packed__))
{
    MCTP_HEADER_FILTERED field;
    alt_u8 source_addr;
    MCTP_COMMON_HEADER transport_header;
} MCTP_HEADER;

// MCTP protocol TO
#define TAG_OWNER 0b1000

// Groupings of information from the MCTP layer
typedef struct
{
    // Use cached information to save info
    alt_u8 cached_pkt_info;
    alt_u8 cached_dest_eid;
    alt_u8 cached_msg_type;
    // Use for i2c slave address
    alt_u8 cached_addr;
    alt_u8 cached_cpld_eid;
    // Use non-cached to initiate
    alt_u8 msg_type;
    alt_u8 bus_type;
    // Type of challenger: BMC, PCH and PCIE
    alt_u8 challenger_type;
    // Error state
    alt_u8 error_type;
    // Use buffer to store received msg
    MEDIUM_APPENDED_BUFFER processed_message;
} MCTP_CONTEXT;


#endif /* EAGLESTREAM_INC_MCTP_H_ */
