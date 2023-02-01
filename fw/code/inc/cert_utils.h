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
 * @file cert_utils.h
 * @brief Utilities common to certificate functions for PFR.
 */

#ifndef EAGLESTREAM_INC_CERT_UTILS_H_
#define EAGLESTREAM_INC_CERT_UTILS_H_

#include "pfr_sys.h"

#include "cert.h"
#include "spdm_utils.h"

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/**
 * @brief DER decode - Return long length value (>127d) from a DER type-length-value structure
 */
static alt_u32 pfr_get_long_val_length(alt_u8* long_src_buf, alt_u8 length_of_msg_length)
{
    // Get length of value
    alt_u32 val_length = 0;
    alt_u32 shift_bit = ASN_DER_MAX_BYTE_LENGTH * 8 - 8;

    for (alt_u8 j = 0, i = ASN_DER_MAX_BYTE_LENGTH - length_of_msg_length;
          i < ASN_DER_MAX_BYTE_LENGTH; j++, i++)
    {
        val_length += (long_src_buf[j] << (shift_bit - (i * 8)));
    }
    return val_length;
}

/**
 * @brief DER decode - Return byte increment to value of DER type-length-value structure
 *              *i.e. bytes to increment to point 1 layer deeper in DER structure
 * This function is used to go into DER sequences or sets to access its elements
 *
 * example structure:
 * T1 L1
 *   |-- TA LA VA
 * T2 L2
 *   |-- TB LB
 *         |-- TC LC VC
 *
 * example input: pointer to T1; output: increment to TA
 * example input: pointer to TA; output: increment to VA
 * example input: pointer to T2; output: increment to TB
 * example input: pointer to TB; output: increment to TC
 * example input: pointer to TC; output: increment to VC
 */
static alt_u32 pfr_decode_der_goto_value_increment(alt_u8* src_ptr)
{
    SHORT_DER* src_buf = (SHORT_DER*)src_ptr;
    if (src_buf->length & DER_FORMAT_LENGTH_MSB_MSK)
    {
        // Get length of message
        alt_u8 length_of_msg_length = (~DER_FORMAT_LENGTH_MSB_MSK & src_buf->length);
        return SHORT_DER_SIZE + length_of_msg_length;
    }
    return SHORT_DER_SIZE;
}

/**
 * @brief DER decode - Return byte increment to next DER type-length-value structure, skipping over the current DER type-length-value structure
 *              This function is used to traverse DER sequences or sets containing multiple elements
 *              *i.e. bytes to increment to point to the next element in the same layer
 *
 * @NOTES: returns 0 to indicate failure to increment due to improbable length values:
 *          - 0x00 means size is zero, which is impossible
 *          - 0xf~ means the size is at least a 240 digit long decimal value
 *
 * example structure:
 * T1 L1
 *   |-- TA LA VA
 *   |-- TB LB VA
 * T2 L2 V2
 *
 * example input 1: pointer to T1; output: increment to T2
 * example input 2: pointer to TA; output: increment to TB
 * example input 3: pointer to TB; output: increment to T2
 */
static alt_u32 pfr_decode_der_goto_next_increment(alt_u8* src_ptr)
{
    SHORT_DER* src_buf = (SHORT_DER*)src_ptr;

    // return 0 for obviously impossible size values
    if (src_buf->length == 0x00 || (src_buf->length & 0xf0) == 0xf0)
    {
    	return 0;
    }

    alt_u8 length_of_msg_length = 0;

    alt_u32 value_size;

    if (src_buf->length & DER_FORMAT_LENGTH_MSB_MSK)
    {
        // Get length of message
        length_of_msg_length = (~DER_FORMAT_LENGTH_MSB_MSK & src_buf->length);
        alt_u8* long_src_buf = (alt_u8*)(src_buf + 1);

        // Get 2 bytes length value
        value_size = pfr_get_long_val_length((alt_u8*)long_src_buf, length_of_msg_length);
    }
    else
    {
        // Get 1 byte length value
        value_size = src_buf->length;
    }

    // Return full DER encoded length
    alt_u32 increment_size = SHORT_DER_SIZE + length_of_msg_length + value_size;
    increment_size = (increment_size > MAX_SUPPORTED_CERT_SIZE) ? MAX_SUPPORTED_CERT_SIZE : increment_size;
    return increment_size;
}

/**
 * @brief DER verify - verify a DER type and length, comparing against expected values
 *
 * @param der_buffer : buffer pointing to a position in DER encoded certificate
 * @param expected_type : expected type
 * @param expected_length : expected length
 *
 * @NOTES:
 *   - This function should not be used if expected length >127
 */
static alt_u8 pfr_decode_der_verify_type_length(alt_u8* der_buffer, alt_u8 expected_type, alt_u8 expected_length) {
    if ( expected_length > 0x80 ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    SHORT_DER* short_der = (SHORT_DER*) der_buffer;
    if (short_der->type != expected_type) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    if (short_der->length != expected_length) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    return CERTIFICATE_VERIFY_PASS;
}

/**
 * @brief DER verify - verify a DER value, comparing against expected value (byte array)
 *
 * @param der_buffer : buffer pointing to a position in DER encoded certificate
 * @param expected_value : expected value
 * @param compare_length : size of value to check
 */
static alt_u8 pfr_decode_der_verify_value(alt_u8* der_buffer, alt_u8* expected_value, alt_u8 compare_length) {
    for (alt_u8 byte_i = 0; byte_i < compare_length; byte_i++) {
        if (der_buffer[byte_i] != expected_value[byte_i]) {
            return CERTIFICATE_VERIFY_FAIL;
        }
    }
    return CERTIFICATE_VERIFY_PASS;
}

/**
 * @brief DER verify - verify a DER type-length-value, comparing against expected values
 *
 * @param der_buffer : buffer pointing to a position in DER encoded certificate
 * @param expected_type : expected type
 * @param expected_length : expected length
 * @param expected_value : expected value
 */
static alt_u8 pfr_decode_der_verify_type_length_value(alt_u8* der_buffer, alt_u8 expected_type, alt_u8 expected_length, alt_u8* expected_value) {
    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_verify_type_length(&der_buffer[0], expected_type, expected_length) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    return pfr_decode_der_verify_value(&der_buffer[SHORT_DER_SIZE], expected_value, expected_length);
}

/**
 * @brief DER verify - decode time ASCII value to alt_u8 numerical values
 */
static alt_u8 pfr_decode_der_datetime_numeric_value(alt_u8* time_buffer) {
    alt_u8 num_value_0 = time_buffer[0] - 0x30;
    alt_u8 num_value_1 = time_buffer[1] - 0x30;
    return (10 * num_value_0) + num_value_1;
}

/**
 * @brief DER verify - check if datetime->year value is a leap year
 *
 * return: 1 if leap year, 0 if not leap year
 */
static alt_u8 pfr_decode_der_datetime_leap_year_check(CERT_DER_DATETIME* datetime_buffer) {
    alt_u16 year_0 = datetime_buffer->year_0;
    alt_u16 year_1 = datetime_buffer->year_1;
    alt_u16 year = ( year_0 * 100 ) + year_1;
    if (year % 400 == 0) {
        return 1;
    }
    else if (year % 100 == 0) {
        return 0;
    }
    else if (year % 4 == 0) {
        return 1;
    }
    return 0;
}

/**
 * @brief DER verify - check date time validity
 */
static alt_u8 pfr_decode_der_datetime_validity_check(CERT_DER_DATETIME* datetime_buffer) {
    if ( 59 < datetime_buffer->second ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    if ( 59 < datetime_buffer->minute ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    if ( 23 < datetime_buffer->hour ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    if ( datetime_buffer->month < 1 || 12 < datetime_buffer->month ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    if ( datetime_buffer->day < 1 ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    alt_u8 max_day = 31;
    if ( datetime_buffer->month == 4 ||
         datetime_buffer->month == 6 ||
         datetime_buffer->month == 9 ||
         datetime_buffer->month == 11 ) {
        max_day = 30;
    } else if ( datetime_buffer->month == 2 ) {
        if ( 1 == pfr_decode_der_datetime_leap_year_check(datetime_buffer) ) {
            max_day = 29;
        } else {
            max_day = 28;
        }
    }

    if ( max_day < datetime_buffer->day ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    return CERTIFICATE_VERIFY_PASS;
}

/**
 * @brief DER verify - decode UTCTIME or GENERALIZEDTIME value
 */
static alt_u8 pfr_decode_der_datetime_value(alt_u8* datetime_buffer, alt_u8* datetime_der_length, alt_u8 datetime_type, CERT_DER_DATETIME* datetime_struct) {

    if ( datetime_type == CERT_DER_DATETIME_UTCTIME &&
         *datetime_der_length != CERT_VALIDITY_UTCTIME_LENGTH ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    alt_u8 datetime_numeric_values_length = 12;
    alt_u8 datetime_z_char_pos = datetime_numeric_values_length;
    alt_u8 skip_year_0 = 1; // UTCTIME omits first 2 digits of the year value
    if ( datetime_type == CERT_DER_DATETIME_GENERALIZEDTIME ) {
        datetime_numeric_values_length = 14;
        datetime_z_char_pos = *datetime_der_length - 1;
        skip_year_0 = 0;
    }
    // check ASCII chars are numeric values 0 - 9
    for ( alt_u8 i = 0; i < datetime_numeric_values_length ; i++ ) {
        if ( datetime_buffer[i] < 0x30 || 0x39 < datetime_buffer[i] ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
    }
    // check for 'Z' char
    if ( datetime_buffer[datetime_z_char_pos] != 'Z' ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    alt_u8* datetime_u8 = (alt_u8*) datetime_struct;
    for ( alt_u8 i = 0; i < CERT_DER_DATETIME_SIZE - skip_year_0; i++ ) {
        datetime_u8[i + skip_year_0] = pfr_decode_der_datetime_numeric_value( &datetime_buffer[i * 2] );
    }
    if ( skip_year_0 == 1 ) {
        datetime_struct->year_0 = 20;
        if ( datetime_struct->year_1 >= 50 ) {
            datetime_struct->year_0 = 19;
        }
    }

    return CERTIFICATE_VERIFY_PASS;
}

/**
 * @brief DER verify - compare two datetime (CERT_DER_DATETIME struct) values
 *
 * returns: datetime_buffer_1 <= datetime_buffer_2
 *
 * @param datetime_buffer_1 : supposedly earlier time value
 * @param datetime_buffer_2 : supposedly later time value
 */
static alt_u8 pfr_decode_der_datetime_compare(CERT_DER_DATETIME* datetime_buffer_1, CERT_DER_DATETIME* datetime_buffer_2) {
    alt_u8* datetime_1_u8 = (alt_u8*) datetime_buffer_1;
    alt_u8* datetime_2_u8 = (alt_u8*) datetime_buffer_2;
    for (alt_u8 byte_i = 0; byte_i < CERT_DER_DATETIME_SIZE; byte_i++) {
        if (datetime_1_u8[byte_i] < datetime_2_u8[byte_i]) {
            return CERTIFICATE_VERIFY_PASS;
        }
        if (datetime_1_u8[byte_i] > datetime_2_u8[byte_i]) {
            return CERTIFICATE_VERIFY_FAIL;
        }
    }
    return CERTIFICATE_VERIFY_PASS;
}

/**
 * @brief DER encode type and length for DER type-length-value structure
 */
static alt_u32 pfr_encode_der_type_length(alt_u8* buf, alt_u32 ptr, alt_u8 type, alt_u8 length)
{
    SHORT_DER* short_der = (SHORT_DER*) &buf[ptr];
    short_der->type = type;
    short_der->length = length;

    return SHORT_DER_SIZE;
}

/**
 * @brief DER encode DER type-length-value structure
 */
static alt_u32 pfr_encode_der_type_length_value(alt_u8* buf, alt_u32 ptr, alt_u8 type, alt_u32 length, alt_u8* value)
{
    //write type and length
    pfr_encode_der_type_length(buf, ptr, type, length);
    //copy the value
    alt_u8_memcpy(&buf[ptr + SHORT_DER_SIZE] , value, length);
    return SHORT_DER_SIZE + length;
}

/**
 * @brief DER encode type and length for DER type-length-value structure, with length values >127
 */
static alt_u32 pfr_encode_der_type_long_length(alt_u8* buf, alt_u32 ptr, alt_u8 type, alt_u16 length)
{
    if ( length < DER_FORMAT_LENGTH_MSB_MSK) { // for length <= 127d
        //generate short length definition
        return pfr_encode_der_type_length(buf, ptr, type, length);
    }
    else if( length >= DER_FORMAT_LENGTH_MSB_MSK && length <= 0xff) { // for 127d < length <= 255d
        //generate long definite length definition
        pfr_encode_der_type_length(buf, ptr, type, 0b10000001);
        buf[ptr + 2] = (alt_u8) length;
        return 3;
    }
    else { // for length 255d < length <= 65535d
        //generate long definite length definition
        pfr_encode_der_type_length(buf, ptr, type, 0b10000010);
        // copy alt_u16 length value into alt_u8 buffer
        buf[ptr + 2] = (length >> 8) & 0xff;
        buf[ptr + 3] = length & 0xff;
        return 4;
    }
}

#endif

#endif /* EAGLESTREAM_INC_CERT_UTILS_H_ */
