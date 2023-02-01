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
 * @file utils.h
 * @brief General utility functions.
 */

#ifndef EAGLESTREAM_INC_PFR_UTILS_H
#define EAGLESTREAM_INC_PFR_UTILS_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "dual_config_utils.h"
#include "status_enums.h"
#include "mailbox_enums.h"

/**
 * A never exiting loop
 */
static void never_exit_loop()
{
    // Put NOPs in the body of the loop to ensure that we can always break into the
    // loop using GDB. If we don't add NOPs the unconditional branch instruction back
    // onto itself will not support breaking into the code without HW breakpoints
    while (1)
    {
        asm("nop");
        asm("nop");
        asm("nop");
        reset_hw_watchdog();

#ifdef USE_SYSTEM_MOCK
        if (SYSTEM_MOCK::get()->should_exec_code_block(
                SYSTEM_MOCK::CODE_BLOCK_TYPES::THROW_FROM_NEVER_EXIT_LOOP))
        {
            PFR_ASSERT(0)
        }
#endif
    }
}

/**
 * @brief This function sets a specific bit of the word at the given address.
 * Note that since this function is always inlined, it's never called.
 *
 * @param addr the address to read from and write to
 * @param shift_bit number of bits to shift to the left
 * @return None
 */
static void set_bit(alt_u32* addr, alt_u32 shift_bit)
{
    // Example of the intended assembly code
    // 1000ac:	00900404 	movi	r2,16400
    // 1000b0:	10c00037 	ldwio	r3,0(r2)
    // 1000b4:	18c00054 	ori	    r3,r3,1
    // 1000b8:	10c00035 	stwio	r3,0(r2)
    alt_u32 val = IORD_32DIRECT(addr, 0);
    val |= (0b1 << shift_bit);
    IOWR_32DIRECT(addr, 0, val);
}

/**
 * @brief This function clears a specific bit of the word at the given address.
 * Note that since this function is always inlined, it's never called.
 *
 * @param addr the address to read from and write to
 * @param shift_bit number of bits to shift to the left
 * @return None
 */
static void clear_bit(alt_u32* addr, alt_u32 shift_bit)
{
    // Example of the intended assembly code
    // 1000ac:	00900404 	movi	r2,16400
    // 1000b0:	11000037 	ldwio	r4,0(r2)
    // 1000b4:	00ffff84 	movi	r3,-2
    // 1000b8:	20c6703a 	and     r3,r4,r3
    // 1000bc:	10c00035 	stwio	r3,0(r2)
    alt_u32 val = IORD_32DIRECT(addr, 0);
    val &= ~(0b1 << shift_bit);
    IOWR_32DIRECT(addr, 0, val);
}

/**
 * @brief This function checks the value of a specific bit of the word at the given address.
 * Note that since this function is always inlined, it's never called.
 *
 * @param addr the address to read from and write to
 * @param shift_bit number of bits to shift to the left
 * @return the value of the bit
 */
static alt_u32 check_bit(alt_u32* addr, alt_u32 shift_bit)
{
    alt_u32 val = IORD_32DIRECT(addr, 0);
    return ((val >> shift_bit) & 0b1);
}

/**
 * @brief This function implements a custom version of memcpy using alt_u32 read/writes.
 *
 * It copies the data word by word, since alt_u32 is the native size, to the same address.
 * Significantly smaller code than memcpy which operates on bytes. Also used for AVMM
 * slaves that do not support byte enable Note that since this function is always inlined, it's
 * never called.
 *
 * @param dest_ptr : pointer to the destination address to write to
 * @param src_ptr : the address to read from
 * @param n_bytes : number of bytes (must be multiples of 4) to write
 * @return None
 */
static void alt_u32_memcpy(alt_u32* dest_ptr, const alt_u32* src_ptr, alt_u32 n_bytes)
{
    PFR_ASSERT((n_bytes % 4) == 0);

    // Copy contents of src[] to dest[] using alt_u32
    // this means 4-bytes at a time
    for (alt_u32 i = 0; i < (n_bytes >> 2); i++)
    {
        dest_ptr[i] = src_ptr[i];
    }
}

/**
 * @brief This functions will log warnings related to memcpy_s to the mailbox.
 *
 * It records the current error codes and combine it with the error codes related to memcpy_s.
 * Then, it logs these combination to the mailbox without overwriting any major error codes.
 *
 * @param log : '1' for invalid pointer, '2' for oversized data and '3' for illegal destination size
 * @return None
 */
static void memcpy_s_logging(alt_u32 log)
{
    IOWR(U_MAILBOX_AVMM_BRIDGE_BASE, MB_MAJOR_ERROR_CODE, 0);
    IOWR(U_MAILBOX_AVMM_BRIDGE_BASE, MB_MINOR_ERROR_CODE, 0);

    IOWR(U_MAILBOX_AVMM_BRIDGE_BASE, MB_MAJOR_ERROR_CODE, MAJOR_ERROR_MEMCPY_S_FAILED);

    if (log == MEMCPY_S_INVALID_NULL_POINTER)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_BASE, MB_MINOR_ERROR_CODE, MINOR_ERROR_INVALID_POINTER);
	}
    else if (log == MEMCPY_S_EXCESSIVE_DATA_SIZE)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_BASE, MB_MINOR_ERROR_CODE,  MINOR_ERROR_EXCESSIVE_DATA_SIZE);
    }
    else if (log == MEMCPY_S_INVALID_MAX_DEST_SIZE)
    {
        IOWR(U_MAILBOX_AVMM_BRIDGE_BASE, MB_MINOR_ERROR_CODE, MINOR_ERROR_EXCESSIVE_DEST_SIZE);
    }
}

/**
 * @brief This function implements a custom version of memcpy_s using alt_u32 read/writes.
 *
 * This function checks for unsafe arguments being passed into the function and inform users
 * for easier debugging. If the destination and source pointer is null, this functions will log
 * the invalid pointer warning to the mailbox as error codes on one of the reserved register area.
 * This is because null pointer can cause undefined behavior.
 * If the destination size to be written to is smaller than the data size, oversized data warning will be
 * logged to the mailbox. This is to prevent overlapping of data onto other regions.
 * For these cases, the function will not proceed to do copy operation.
 *
 * @param dest_ptr : pointer to the destination address to write to
 * @param dest_size : maximum size of the destination area
 * @param src_ptr : the address to read from
 * @param n_bytes : number of bytes (must be multiples of 4) to write
 * @return None
 */
static void alt_u32_memcpy_s(alt_u32* dest_ptr, const alt_u32 dest_size, const alt_u32* src_ptr, alt_u32 n_bytes)
{
    PFR_ASSERT((n_bytes % 4) == 0);

    if ((!dest_ptr) || (!src_ptr))
    {
        memcpy_s_logging(MEMCPY_S_INVALID_NULL_POINTER);
    }
    else if (dest_size < n_bytes)
    {
        memcpy_s_logging(MEMCPY_S_EXCESSIVE_DATA_SIZE);
    }
    else if (dest_size > PCH_SPI_FLASH_SIZE)
    {
        memcpy_s_logging(MEMCPY_S_INVALID_MAX_DEST_SIZE);
    }
    else
    {
        for (alt_u32 i = 0; i < (n_bytes >> 2); i++)
        {
            dest_ptr[i] = src_ptr[i];
        }
    }
}

/**
 * @brief This function implements a custom version of memcpy which doesn't increment destination
 * address.
 *
 * It copies the data word by word, since alt_u32 is the native size, to the same address.
 * Significantly smaller code than memcpy which operates on bytes. Also used for AVMM slaves that do
 * not support byte enable..
 *
 * @param dest_ptr : pointer to the destination address to write to
 * @param src_ptr : the address to read from
 * @param n_bytes : number of bytes (must be multiples of 4) to write
 * @return None
 */
static void alt_u32_memcpy_non_incr(alt_u32* dest_ptr, const alt_u32* src_ptr, const alt_u32 n_bytes)
{
    PFR_ASSERT((n_bytes % 4) == 0);

    // Copy contents of src[] to dest using alt_u32
    // this means 4-bytes at a time. dest does not increment.
    for (alt_u32 i = 0; i < (n_bytes >> 2); i++)
    {
        // Equivalent code: dest_ptr[0] = src_ptr[i];
        // and this is the smallest code:
        //        1000a0:   18c5883a    add r2,r3,r3
        //        1000a4:   1085883a    add r2,r2,r2
        //        1000a8:   2085883a    add r2,r4,r2
        //        1000ac:   10800037    ldwio   r2,0(r2)
        //        1000b0:   30800035    stwio   r2,0(r6)

        // Equivalent code: dest_ptr[0] = src_ptr[i];
        // Use IOWR so that we can intercept and mock the writes
        IOWR(dest_ptr, 0, src_ptr[i]);
    }
}

/*
 * @brief Check and compare the stored data against incoming data.
 *
 * @param data : Address of data input
 * @param stored_data : Address of stored data
 * @param data_size : size of referred entity
 *
 * @return 1 if matched; 0 otherwise
 */
static alt_u32 is_data_matching_stored_data(alt_u32* data, alt_u32* stored_data, alt_u32 data_size)
{
    for (alt_u32 i = 0; i < (data_size / 4); i++)
    {
        if (data[i] != stored_data[i])
        {
        	return 0;
        }
    }
    return 1;
}

/**
 * @brief Copy byte by byte to specifically handle end-to-end message
 *
 * @param dest_ptr : Destination address
 * @param src_ptr : Source address
 * @param n_bytes : Data size
 *
 * @return none
 *
 */
static void alt_u8_memcpy(alt_u8* dest_ptr, alt_u8* src_ptr, alt_u32 n_bytes)
{
    for (alt_u32 i = 0; i < n_bytes; i++)
    {
        dest_ptr[i] = src_ptr[i];
    }
}

/**
 * @brief Clear buffer to zero according to specified size
 *
 * @param buffer : Buffer to be cleared
 * @param size : Buffer size to clear
 *
 * @return none
 */
static void reset_buffer(alt_u8* buffer, alt_u32 size)
{
    while (size-- != 0)
    {
        *(buffer++)	= 0;
    }
}


#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/**
 * @brief Compute the power of a value
 *
 * @param base : Base value
 * @param exp : Exponent
 *
 * @return Result
 *
 */
static alt_u32 compute_pow(alt_u32 base, alt_u32 exp)
{
    alt_u32 result = 1;
    do
    {
        result *= base;
    }
    while (--exp != 0);
    return result;
}

/**
 * @brief Rounds up input value to nearest multiple of 4
 * This function is used for 4-byte alignment padding calculation
 *
 * @param value : value to round up
 *
 * @return Result
 *
 */
static alt_u32 round_up_to_multiple_of_4(alt_u32 value)
{
    alt_u32 result = value;
    while (result % 4 != 0) {
    	result++;
    }
    return result;
}

#endif

#endif /* EAGLESTREAM_INC_PFR_UTILS_H */
