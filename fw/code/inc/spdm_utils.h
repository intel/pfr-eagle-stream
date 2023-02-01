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
 * @file spdm_utils.h
 * @brief General SPDM utility functions.
 */

#ifndef EAGLESTREAM_INC_PFR_SPDM_UTILS_H
#define EAGLESTREAM_INC_PFR_SPDM_UTILS_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "spdm.h"

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/*
 * @brief Process and read a 3 byte value
 *
 * @param buf : data to read
 *
 * @return Processed 3 byte value in word
 *
 */

static alt_u32 alt_u24_read(alt_u8* buf)
{
    return (alt_u32)(buf[0] | buf[1] << 8 | buf[2] << 16);
}

/*
 * @brief Process and write a 3 byte value
 *
 * @param buf : data to write to
 * @param val : value written from
 *
 * @return Processed 3 byte value in word
 *
 */

static alt_u32 alt_u24_write(alt_u8* buf, alt_u32 val)
{
    for (alt_u8 i = 0; i < 3; i++)
    {
        buf[i] = (alt_u8)((val >> (i << 3)) & 0xff);
    }
    return val;
}

/**
 * @brief Append to buffer keeping small size message
 *
 * @param msg_transcript : Small-sized message
 * @param data : Data to append
 * @param msg-size : Message size
 *
 * @return none
 */
static void append_small_buffer(SMALL_APPENDED_BUFFER* msg_transcript, alt_u8* data, alt_u32 msg_size)
{
    alt_u8_memcpy((alt_u8*)((alt_u8*)&msg_transcript->buffer[0] + msg_transcript->buffer_size), data, msg_size);
    msg_transcript->buffer_size += msg_size;
}

/**
 * @brief Append to buffer keeping large size message
 *
 * @param msg_transcript : Large-sized message
 * @param data : Data to append
 * @param msg-size : Message size
 *
 * @return none
 */
static void append_large_buffer(LARGE_APPENDED_BUFFER* msg_transcript, alt_u8* data, alt_u32 msg_size)
{
    alt_u8_memcpy((alt_u8*)((alt_u8*)&msg_transcript->buffer[0] + msg_transcript->buffer_size), data, msg_size);
    msg_transcript->buffer_size += msg_size;
}

/**
 * @brief Shrink large buffer
 * NOTE: No checks here to save code space, hence careful usage is required
 *
 * @param msg_transcript : Large-sized message
 * @param msg-size : Message size to shrink
 *
 * @return none
 */
static void shrink_large_buffer(LARGE_APPENDED_BUFFER* msg_transcript, alt_u32 msg_size)
{
    PFR_ASSERT(!(msg_size > msg_transcript->buffer_size));
    msg_transcript->buffer_size -= msg_size;
}

#endif

#endif /* EAGLESTREAM_INC_PFR_SPDM_UTILS_H */
