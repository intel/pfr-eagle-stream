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
 * @file cert_flow.h
 * @brief Generate and verify cert chain.
 */

#ifndef EAGLESTREAM_INC_CERT_FLOW_H_
#define EAGLESTREAM_INC_CERT_FLOW_H_

#include "pfr_sys.h"

#include "cert_gen_options.h"
#include "cert_utils.h"
#include "crypto.h"

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

// Cert verification code

/**
 * @brief returns length of a certificate
 *
 * @NOTES: pfr_decode_der_goto_next_increment() returns 0 if valid DER encoded structure is found at buf, indicating no x509 is found at the pointer
 */
static alt_u32 pfr_get_certificate_size(alt_u8* buf) {
    return pfr_decode_der_goto_next_increment(buf);
}

/**
 * @brief Returns length of cert chain (number of certificates that form a cert chain)
 *
 * @param cert_chain_buffer : certificate chain
 * @param cert_chain_buffer_size : size of the certificate chain buffer
 *
 * @NOTES:
 *   - cert_chain_buffer_size must cover the certificate chain exactly, i.e. there are no extra bytes that are not part of the cert chain in cert_chain_buffer
 *   - If extra bytes are found, return 0 for failure to calculate certificate chain length
 */
static alt_u32 pfr_certificate_get_chain_length(alt_u8* cert_chain_buffer, alt_u32 cert_chain_buffer_size) {
    alt_u32 cert_chain_calculated_length = 0;
    alt_u16 cert_chain_calculated_size = 0;
    do {
        // Return failure for impossible cert chain size
        // This check prevents decoding invalid DER length
        if ( (cert_chain_buffer_size - cert_chain_calculated_size) < 8 ) {
            return 0;
        }
        alt_u16 next_cert_size = pfr_get_certificate_size(&cert_chain_buffer[cert_chain_calculated_size]);
        // Return failure if impossible cert size
        if ( !next_cert_size ) {
            break;
        }
        cert_chain_calculated_size += next_cert_size;
        // Return failure if calculated size exceeded buffer size, indicating invalid certificate DER encoded length
        if ( cert_chain_calculated_size > cert_chain_buffer_size ) {
            return 0;
        }
        cert_chain_calculated_length += 1;
    }
    while (cert_chain_calculated_size < cert_chain_buffer_size);
    return cert_chain_calculated_length;
}

/**
 * @brief Returns size of cert chain of known length (in bytes)
 *
 * @NOTES: returns 0 if fail to get cert chain size
 *
 * @param cert_chain_buffer : certificate chain
 * @param chain_length : number of certificates in the cert chain
 */
static alt_u32 pfr_certificate_get_chain_size(alt_u8* cert_chain_buffer, alt_u32 chain_length) {
    alt_u32 cert_chain_size = 0;
    for (alt_u8 layer = 0; layer < chain_length; layer++) {
    	alt_u32 next_cert_size = pfr_get_certificate_size(&cert_chain_buffer[cert_chain_size]);
    	if ( !next_cert_size )
    	{
    		return 0;
    	}
        cert_chain_size += next_cert_size;
    }
    return cert_chain_size;
}

/**
 * @brief DER verify - check major offsets of certificate fields to eliminate clearly invalid certificates before performing field-by-field verification
 */
static alt_u32 pfr_verify_der_certificate_check_offsets(alt_u8* der_cert_buffer, alt_u32 der_cert_buffer_size_limit, alt_u32* signed_content_offset, alt_u32* signature_algorithm_offset, alt_u32* certificate_signature_offset, alt_u32* der_decoded_cert_size) {
    if ( der_cert_buffer_size_limit <= 15 ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // enter primary sequence structure, get signed content SEQUENCE offset
    *signed_content_offset = pfr_decode_der_goto_value_increment(&der_cert_buffer[0]);
    if ( *signed_content_offset + SHORT_DER_SIZE >= der_cert_buffer_size_limit) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // get signature algorithm SEQUENCE offset
    *signature_algorithm_offset = *signed_content_offset + pfr_decode_der_goto_next_increment(&der_cert_buffer[*signed_content_offset]);
    if ( *signature_algorithm_offset + SHORT_DER_SIZE >= der_cert_buffer_size_limit) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // get certificate signature SEQUENCE offset
    *certificate_signature_offset = *signature_algorithm_offset + pfr_decode_der_goto_next_increment(&der_cert_buffer[*signature_algorithm_offset]);
    if ( *certificate_signature_offset + 11 > der_cert_buffer_size_limit) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // check der encoded size equals cert der buffer size
    *der_decoded_cert_size = pfr_get_certificate_size(der_cert_buffer);
    alt_u32 ptr_offset = *certificate_signature_offset + pfr_decode_der_goto_next_increment(&der_cert_buffer[*certificate_signature_offset]);
    if ( *der_decoded_cert_size == 0 || *der_decoded_cert_size != ptr_offset || *der_decoded_cert_size > der_cert_buffer_size_limit) {
        return CERTIFICATE_VERIFY_FAIL;
    }

    return CERTIFICATE_VERIFY_PASS;
}

/**
 * @brief DER verify - verify certificate signature algorithm
 */
static alt_u32 pfr_verify_der_certificate_signature_algorithm(alt_u8* der_cert_buffer, alt_u32 cert_buffer_ptr, alt_u32 cert_buffer_ptr_limit) {
    cert_buffer_ptr += pfr_decode_der_goto_value_increment(&der_cert_buffer[cert_buffer_ptr]);
    if ( cert_buffer_ptr >= cert_buffer_ptr_limit ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    alt_u8 asn1_oid_ecdsawithsha384[ASN1_OID_ECDSAWITHSHA384_LENGTH] = ASN1_OID_ECDSAWITHSHA384;
    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_verify_type_length_value(&der_cert_buffer[cert_buffer_ptr], ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_ECDSAWITHSHA384_LENGTH, asn1_oid_ecdsawithsha384) ) {
        // signature algorithm not ecdsaWith384
        return CERTIFICATE_VERIFY_FAIL;
    }

    return CERTIFICATE_VERIFY_PASS;
}

/**
 * @brief DER verify - verify certificate version
 */
static alt_u32 pfr_verify_der_certificate_content_version(alt_u8* der_cert_buffer, alt_u32 cert_buffer_ptr, alt_u32 cert_buffer_ptr_limit) {
    // x509 certificate must be version 3
    alt_u8 version_integer_der[3] = {ASN1_TYPE_TAG_INTEGER, CERT_VERSION_LENGTH, CERT_VERSION_VALUE_3};
    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_verify_type_length_value(&der_cert_buffer[cert_buffer_ptr], ASN1_TYPE_TAG_IDENTIFIER_OCTET_0, 3, version_integer_der) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    return CERTIFICATE_VERIFY_PASS;

}

/**
 * @brief DER verify - verify certificate signature algorithm identifier
 */
static alt_u32 pfr_verify_der_certificate_content_signature_algorithm_identifier(alt_u8* der_cert_buffer, alt_u32 cert_buffer_ptr, alt_u32 cert_buffer_ptr_limit) {
    // certificate signed content signature algorithm identifier should be indentical to certificate signature algorithm
    return pfr_verify_der_certificate_signature_algorithm(der_cert_buffer, cert_buffer_ptr, cert_buffer_ptr_limit);
}

/**
 * @brief DER verify - verify certificate validity period
 */
static alt_u32 pfr_verify_der_certificate_content_validity_period(alt_u8* der_cert_buffer, alt_u32 cert_buffer_ptr, alt_u32 cert_buffer_ptr_limit) {
    cert_buffer_ptr += SHORT_DER_SIZE;

    if ( cert_buffer_ptr >= cert_buffer_ptr_limit ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // get validity from (start) datetime
    SHORT_DER* cert_validity_from_header = (SHORT_DER*) &der_cert_buffer[cert_buffer_ptr];
    if ( cert_buffer_ptr >= cert_buffer_ptr_limit ) {
        return CERTIFICATE_VERIFY_FAIL;
    }

    alt_u8 cert_validity_from_datetime_type = 0;
    if ( cert_validity_from_header->type == ASN1_TYPE_TAG_UTCTIME ) {
        cert_validity_from_datetime_type = CERT_DER_DATETIME_UTCTIME;
    } else if ( cert_validity_from_header->type == ASN1_TYPE_TAG_GENERALIZEDTIME ) {
        cert_validity_from_datetime_type = CERT_DER_DATETIME_GENERALIZEDTIME;
    } else {
        return CERTIFICATE_VERIFY_FAIL;
    }

    CERT_DER_DATETIME cert_datetime_from;
    CERT_DER_DATETIME* cert_datetime_from_ptr = &cert_datetime_from;

    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_datetime_value(&der_cert_buffer[cert_buffer_ptr + SHORT_DER_SIZE], &cert_validity_from_header->length, cert_validity_from_datetime_type, cert_datetime_from_ptr) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_datetime_validity_check(cert_datetime_from_ptr) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }

    cert_buffer_ptr += pfr_decode_der_goto_next_increment(&der_cert_buffer[cert_buffer_ptr]);

    if ( cert_buffer_ptr >= cert_buffer_ptr_limit ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // get validity till (end) datetime

    SHORT_DER* cert_validity_till_header = (SHORT_DER*) &der_cert_buffer[cert_buffer_ptr];
    if ( cert_buffer_ptr >= cert_buffer_ptr_limit ) {
        return CERTIFICATE_VERIFY_FAIL;
    }

    alt_u8 cert_validity_till_datetime_type = 0;
    if ( cert_validity_till_header->type == ASN1_TYPE_TAG_UTCTIME ) {
        cert_validity_till_datetime_type = CERT_DER_DATETIME_UTCTIME;
    } else if ( cert_validity_till_header->type == ASN1_TYPE_TAG_GENERALIZEDTIME ) {
        cert_validity_till_datetime_type = CERT_DER_DATETIME_GENERALIZEDTIME;
    } else {
        return CERTIFICATE_VERIFY_FAIL;
    }

    CERT_DER_DATETIME cert_datetime_till;
    CERT_DER_DATETIME* cert_datetime_till_ptr = &cert_datetime_till;

    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_datetime_value(&der_cert_buffer[cert_buffer_ptr + SHORT_DER_SIZE], &cert_validity_till_header->length, cert_validity_till_datetime_type, cert_datetime_till_ptr) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_datetime_validity_check(cert_datetime_till_ptr) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }

    // compare if 'validity till' date is later than 'validity from' date
    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_datetime_compare(cert_datetime_from_ptr, cert_datetime_till_ptr) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }

    alt_u8 fw_valid_from_ascii[CERT_VALIDITY_UTCTIME_LENGTH] = CERT_VALIDITY_FROM; //alt_u8 fw_valid_till_ascii[CERT_VALIDITY_UTCTIME_LENGTH] = CERT_VALIDITY_TILL;
    alt_u8 utctime_length = CERT_VALIDITY_UTCTIME_LENGTH;
    CERT_DER_DATETIME fw_datetime_till;
    CERT_DER_DATETIME* fw_datetime_till_ptr = &fw_datetime_till;

    pfr_decode_der_datetime_value(fw_valid_from_ascii, &utctime_length, CERT_DER_DATETIME_UTCTIME, fw_datetime_till_ptr);

    // check "validity till" timestamp
    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_datetime_compare(fw_datetime_till_ptr, cert_datetime_till_ptr) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }

    return CERTIFICATE_VERIFY_PASS;
}

/**
 * @brief DER verify - verify certificate public key info
 */
static alt_u32 pfr_verify_der_certificate_content_public_key_info(alt_u8* der_cert_buffer, alt_u32 cert_buffer_ptr, alt_u32 cert_buffer_ptr_limit, alt_u32* cert_cx, alt_u32* cert_cy) {

    cert_buffer_ptr += pfr_decode_der_goto_value_increment(&der_cert_buffer[cert_buffer_ptr]);

    if ( cert_buffer_ptr >= cert_buffer_ptr_limit ) {
        return CERTIFICATE_VERIFY_FAIL;
    }

    alt_u32 ptr_offset = cert_buffer_ptr;
    ptr_offset += pfr_decode_der_goto_value_increment(&der_cert_buffer[ptr_offset]);
    if ( ptr_offset >= cert_buffer_ptr_limit ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // check subject's public key info -> ecpublickey
    alt_u8 ecpublickey_oid[ASN1_OID_ECPUBLICKEY_LENGTH] = ASN1_OID_ECPUBLICKEY;
    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_verify_type_length_value(&der_cert_buffer[ptr_offset], ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_ECPUBLICKEY_LENGTH, ecpublickey_oid) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }

    if ( ptr_offset >= cert_buffer_ptr_limit ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    ptr_offset += pfr_decode_der_goto_next_increment(&der_cert_buffer[ptr_offset]);
    // check subject's public key info -> secp384r1
    alt_u8 secp384r1_oid[ASN1_OID_SECP384R1_LENGTH] = ASN1_OID_SECP384R1;
    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_verify_type_length_value(&der_cert_buffer[ptr_offset], ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_SECP384R1_LENGTH, secp384r1_oid) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    cert_buffer_ptr += pfr_decode_der_goto_next_increment(&der_cert_buffer[cert_buffer_ptr]);

    if ( cert_buffer_ptr >= cert_buffer_ptr_limit ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // check subject's public key info -> public key BITSTRING
    if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_verify_type_length(&der_cert_buffer[cert_buffer_ptr], ASN1_TYPE_TAG_BIT_STRING, PUBLIC_KEY_INFO_KEY_LENGTH) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    cert_buffer_ptr += SHORT_DER_SIZE;
    // no need to check 1st and 2nd bytes. public key validity will be verified by signature verification
    cert_buffer_ptr += 2;

    if ( cert_buffer_ptr + 2*SHA384_LENGTH >= cert_buffer_ptr_limit ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // retrieve subject's public key info -> public key cx & cy
    alt_u8_memcpy((alt_u8*) cert_cx, &der_cert_buffer[cert_buffer_ptr], SHA384_LENGTH);
    alt_u8_memcpy((alt_u8*) cert_cy, &der_cert_buffer[cert_buffer_ptr + SHA384_LENGTH], SHA384_LENGTH);

    return CERTIFICATE_VERIFY_PASS;
}

/**
 * @brief DER verify - verify certificate basic constraints extension
 */
static alt_u32 pfr_verify_der_certificate_content_extensions_basic_constraints(alt_u8* der_cert_buffer, alt_u32 cert_buffer_ptr, alt_u32 cert_buffer_ptr_limit, CERT_EXTENSION_CONTEXT* cert_extension_context) {
    SHORT_DER* basic_constraint_struct = (SHORT_DER*) &der_cert_buffer[cert_buffer_ptr];
    if ( basic_constraint_struct->type != ASN1_TYPE_TAG_SEQUENCE ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    cert_buffer_ptr += SHORT_DER_SIZE;
    if ( cert_buffer_ptr_limit != cert_buffer_ptr + basic_constraint_struct->length ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    switch ( basic_constraint_struct->length ) {
        case 0:
            /* CA=false
             * > ASN1 DER encoded structure:
             *    - SEQUENCE (SIZE 0)
             * > hex: 30 00
             */
            return CERTIFICATE_VERIFY_PASS;
        case 3:
            /* CA=true, no path length constraint
             * > ASN1 DER encoded structure:
             *    - SEQUENCE (SIZE 3)
             *        |-- BOOLEAN (TRUE)
             * > hex: 30 03 01 01 ff
             */
            {
                alt_u8 ca_true_no_path_len[3] = {0x01, 0x01, 0xff};
                if ( !pfr_decode_der_verify_value(&der_cert_buffer[cert_buffer_ptr], ca_true_no_path_len, 3) ) {
                    return CERTIFICATE_VERIFY_FAIL;
                }
                cert_extension_context->cert_is_ca = 1;
                return CERTIFICATE_VERIFY_PASS;
            }
        case 6:
            /* CA=true with path length constraint
             * > ASN1 DER encoded structure:
             *    - SEQUENCE (SIZE 6)
             *        |-- BOOLEAN (TRUE)
             *        |-- INTEGER (<128)
             * > hex: 30 06 01 01 ff 02 01 XX
             *     *XX is the path length constraint value
             */
            {
                alt_u8 ca_true_with_path_len[5] = {0x01, 0x01, 0xff, 0x02, 0x01};
                if ( !pfr_decode_der_verify_value(&der_cert_buffer[cert_buffer_ptr], ca_true_with_path_len, 5) ) {
                    return CERTIFICATE_VERIFY_FAIL;
                }
                cert_buffer_ptr += 5;
                alt_u8 path_length_constraint = der_cert_buffer[cert_buffer_ptr];
                // Path length constraint >127 is not supported
                if ( path_length_constraint >= 0x80 ) {
                    return CERTIFICATE_VERIFY_FAIL;
                }
                cert_extension_context->cert_is_ca = 1;
                cert_extension_context->path_length_constraint_enabled = 1;
                cert_extension_context->path_length_constraint_value = path_length_constraint;
                return CERTIFICATE_VERIFY_PASS;
            }
        default:
            return CERTIFICATE_VERIFY_FAIL;
    }

}

/**
 * @brief DER verify - verify certificate key usage extension
 */
static alt_u32 pfr_verify_der_certificate_content_extensions_key_usage(alt_u8* der_cert_buffer, alt_u32 cert_buffer_ptr, alt_u32 cert_buffer_ptr_limit, CERT_EXTENSION_CONTEXT* cert_extension_context) {
    SHORT_DER* key_usage_struct = (SHORT_DER*) &der_cert_buffer[cert_buffer_ptr];
    if ( key_usage_struct->type != ASN1_TYPE_TAG_BIT_STRING ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    if ( cert_buffer_ptr_limit != cert_buffer_ptr + pfr_decode_der_goto_next_increment(&der_cert_buffer[cert_buffer_ptr]) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    if ( key_usage_struct->length < 2 ) {
        return CERTIFICATE_VERIFY_FAIL;
    }

    alt_u8 flag_bit_string_size = key_usage_struct->length - 1;
    cert_buffer_ptr += SHORT_DER_SIZE;


    alt_u16 cert_flags = 0;
    cert_buffer_ptr += 1;
    cert_flags |= der_cert_buffer[cert_buffer_ptr];
    if ( flag_bit_string_size == 1 ) {
        cert_flags <<= 1;
    } else {
        cert_flags <<= 8;
        cert_flags |= ( der_cert_buffer[cert_buffer_ptr + 1]);
        cert_flags >>= (16 - X509_KEY_USAGE_BIT_STRING_FULL_LENGTH);
    }

    // cannot have 0 flags set
    if ( cert_flags == 0 ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    if ( cert_flags & X509_KEY_USAGE_FLAG_DIGITAL_SIGNATURE ) {
        cert_extension_context->digital_signature_ku_flag_set = 1;
    }
    if ( cert_flags & X509_KEY_USAGE_FLAG_KEY_CERT_SIGN ) {
        cert_extension_context->key_cert_sign_ku_flag_set = 1;
    }
    return CERTIFICATE_VERIFY_PASS;
}

/**
 * @brief DER verify - verify certificate extended key usage extension
 */
static alt_u32 pfr_verify_der_certificate_content_extensions_extended_key_usage(alt_u8* der_cert_buffer, alt_u32 cert_buffer_ptr, alt_u32 cert_buffer_ptr_limit, CERT_EXTENSION_CONTEXT* cert_extension_context) {
    if ( der_cert_buffer[cert_buffer_ptr] != ASN1_TYPE_TAG_SEQUENCE ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    if ( cert_buffer_ptr_limit != cert_buffer_ptr + pfr_decode_der_goto_next_increment(&der_cert_buffer[cert_buffer_ptr]) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    cert_buffer_ptr += pfr_decode_der_goto_value_increment(&der_cert_buffer[cert_buffer_ptr]);

    alt_u8 server_auth_oid[ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH] = ASN1_OID_EXT_EXTENDED_KEY_USAGE_SERVER_AUTH;
    alt_u8 client_auth_oid[ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH] = ASN1_OID_EXT_EXTENDED_KEY_USAGE_CLIENT_AUTH;
    alt_u8 any_extended_key_usage_oid[ASN1_OID_EXT_EXTENDED_KEY_USAGE_ANY_EXTENDED_KEY_USAGE_LENGTH] = ASN1_OID_EXT_EXTENDED_KEY_USAGE_ANY_EXTENDED_KEY_USAGE;

    while ( cert_buffer_ptr < cert_buffer_ptr_limit ) {
        if ( cert_buffer_ptr + SHORT_DER_SIZE >= cert_buffer_ptr_limit ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
        SHORT_DER* extended_key_usage_struct = (SHORT_DER*) &der_cert_buffer[cert_buffer_ptr];
        if ( cert_buffer_ptr + SHORT_DER_SIZE + extended_key_usage_struct->length > cert_buffer_ptr_limit ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
        if ( extended_key_usage_struct->type != ASN1_TYPE_TAG_OBJECT_IDENTIFIER ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
        if ( extended_key_usage_struct->length == ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH ) {
            // determine extended key usage from OID
            alt_u8* extension_oid_value = &der_cert_buffer[cert_buffer_ptr + SHORT_DER_SIZE];
            if ( pfr_decode_der_verify_value(extension_oid_value, server_auth_oid, ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH) ) {
                cert_extension_context->server_auth_eku_defined = 1;
            } else if ( pfr_decode_der_verify_value(extension_oid_value, client_auth_oid, ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH) ) {
                cert_extension_context->client_auth_eku_defined = 1;
            }
        } else if ( extended_key_usage_struct->length == ASN1_OID_EXT_EXTENDED_KEY_USAGE_ANY_EXTENDED_KEY_USAGE_LENGTH ) {
            alt_u8* extension_oid_value = &der_cert_buffer[cert_buffer_ptr + SHORT_DER_SIZE];
            if ( pfr_decode_der_verify_value(extension_oid_value, any_extended_key_usage_oid, ASN1_OID_EXT_EXTENDED_KEY_USAGE_ANY_EXTENDED_KEY_USAGE_LENGTH) ) {
                cert_extension_context->any_extended_key_usage_eku_defined = 1;
            }
        }
        alt_u32 cert_buffer_ptr_incr = pfr_decode_der_goto_next_increment(&der_cert_buffer[cert_buffer_ptr]);
        if ( !cert_buffer_ptr_incr )
        {
        	return CERTIFICATE_VERIFY_FAIL;
        }
        cert_buffer_ptr += cert_buffer_ptr_incr;
    }
    if ( cert_buffer_ptr != cert_buffer_ptr_limit ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    return CERTIFICATE_VERIFY_PASS;
}

/**
 * @brief DER verify - verify certificate extensions
 */
static alt_u32 pfr_verify_der_certificate_content_extensions(alt_u8* der_cert_buffer, alt_u32 cert_buffer_ptr, alt_u32 cert_buffer_ptr_limit, CERT_TYPE cert_type, CERT_EXTENSION_CONTEXT* cert_extension_context) {

    if ( der_cert_buffer[cert_buffer_ptr] != ASN1_TYPE_TAG_IDENTIFIER_OCTET_3 ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    if ( cert_buffer_ptr_limit != cert_buffer_ptr + pfr_decode_der_goto_next_increment(&der_cert_buffer[cert_buffer_ptr]) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }

    cert_buffer_ptr += pfr_decode_der_goto_value_increment(&der_cert_buffer[cert_buffer_ptr]);
    if ( der_cert_buffer[cert_buffer_ptr] != ASN1_TYPE_TAG_SEQUENCE ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    if ( cert_buffer_ptr_limit != cert_buffer_ptr + pfr_decode_der_goto_next_increment(&der_cert_buffer[cert_buffer_ptr]) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }

    cert_buffer_ptr += pfr_decode_der_goto_value_increment(&der_cert_buffer[cert_buffer_ptr]);
    if ( cert_buffer_ptr > cert_buffer_ptr_limit ) {
        return CERTIFICATE_VERIFY_FAIL;
    }

    alt_u32 extensions_offset = cert_buffer_ptr;
    // get number of extensions in certificate
    alt_u32 extensions_length = 0;
    while ( cert_buffer_ptr < cert_buffer_ptr_limit ) {
        if ( der_cert_buffer[cert_buffer_ptr] != ASN1_TYPE_TAG_SEQUENCE ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
        extensions_length += 1;
        alt_u32 cert_buffer_ptr_incr = pfr_decode_der_goto_next_increment(&der_cert_buffer[cert_buffer_ptr]);
        if ( !cert_buffer_ptr_incr )
        {
        	return CERTIFICATE_VERIFY_FAIL;
        }
        cert_buffer_ptr += cert_buffer_ptr_incr;
        if ( cert_buffer_ptr == cert_buffer_ptr_limit ) {
            break;
        } else if ( cert_buffer_ptr > cert_buffer_ptr_limit ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
    }

    reset_buffer((alt_u8*)cert_extension_context, sizeof(CERT_EXTENSION_CONTEXT));

    // check each extension in certificate
    for (alt_u8 extension_itr = 0; extension_itr < extensions_length; extension_itr++) {

        alt_u32 next_extension_offset = extensions_offset + pfr_decode_der_goto_next_increment(&der_cert_buffer[extensions_offset]);

        if ( next_extension_offset > cert_buffer_ptr_limit ) {
            return CERTIFICATE_VERIFY_FAIL;
        } else if ( next_extension_offset == cert_buffer_ptr_limit ) {
            if ( extension_itr != extensions_length - 1 ) {
                return CERTIFICATE_VERIFY_FAIL;
            }
        }

        cert_buffer_ptr = extensions_offset + pfr_decode_der_goto_value_increment(&der_cert_buffer[extensions_offset]);

        if ( cert_buffer_ptr + SHORT_DER_SIZE + ASN1_OID_EXTENSIONS_UNIVERSAL_LENGTH >= next_extension_offset ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
        /*
        // expect extension OID
        if ( CERTIFICATE_VERIFY_FAIL == pfr_decode_der_verify_type_length(&der_cert_buffer[cert_buffer_ptr], ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_EXTENSIONS_UNIVERSAL_LENGTH) ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
        */

        alt_u8 extension_type = 0;
        alt_u8 basic_constraints_oid[ASN1_OID_EXTENSIONS_UNIVERSAL_LENGTH] = ASN1_OID_EXT_BASIC_CONSTRAINTS;
        alt_u8 key_usage_oid[ASN1_OID_EXTENSIONS_UNIVERSAL_LENGTH] = ASN1_OID_EXT_KEY_USAGE;
        alt_u8 extended_key_usage_oid[ASN1_OID_EXTENSIONS_UNIVERSAL_LENGTH] = ASN1_OID_EXT_EXTENDED_KEY_USAGE;
        alt_u32 extension_oid_offset = cert_buffer_ptr + SHORT_DER_SIZE;

        // determine extension type from encoded OID
        if ( pfr_decode_der_verify_value(&der_cert_buffer[extension_oid_offset], basic_constraints_oid, ASN1_OID_EXTENSIONS_UNIVERSAL_LENGTH) ) {
            if ( cert_extension_context->basic_constraints_defined ) {
                return CERTIFICATE_VERIFY_FAIL;
            }
            cert_extension_context->basic_constraints_defined = 1;
            extension_type = 1;
        } else if ( pfr_decode_der_verify_value(&der_cert_buffer[extension_oid_offset], key_usage_oid, ASN1_OID_EXTENSIONS_UNIVERSAL_LENGTH) ) {
            if ( cert_extension_context->key_usage_defined ) {
                return CERTIFICATE_VERIFY_FAIL;
            }
            cert_extension_context->key_usage_defined = 1;
            extension_type = 2;
        } else if ( pfr_decode_der_verify_value(&der_cert_buffer[extension_oid_offset], extended_key_usage_oid, ASN1_OID_EXTENSIONS_UNIVERSAL_LENGTH) ) {
            if ( cert_extension_context->extended_key_usage_defined ) {
                return CERTIFICATE_VERIFY_FAIL;
            }
            cert_extension_context->extended_key_usage_defined = 1;
            extension_type = 3;
        }

        cert_buffer_ptr += pfr_decode_der_goto_next_increment(&der_cert_buffer[cert_buffer_ptr]);
        if ( cert_buffer_ptr + ASN1_DER_BOOLEAN_STRUCT_SIZE > next_extension_offset ) {
            return CERTIFICATE_VERIFY_FAIL;
        }

        alt_u8 extension_has_critical_field = pfr_decode_der_verify_type_length(&der_cert_buffer[cert_buffer_ptr], ASN1_TYPE_TAG_BOOLEAN, ASN1_DER_BOOLEAN_LENGTH);
        alt_u8 extension_is_critical = 0;

        if ( !extension_type && extension_has_critical_field) {
            // if a unsupported extension is critical, then cert verification must fail
            extension_is_critical = der_cert_buffer[cert_buffer_ptr + SHORT_DER_SIZE];
            if ( extension_is_critical ) {
                return CERTIFICATE_VERIFY_FAIL;
            }
        } else if ( extension_type ) {
            if ( extension_has_critical_field ) {
                extension_is_critical = der_cert_buffer[cert_buffer_ptr + SHORT_DER_SIZE];
                cert_buffer_ptr += ASN1_DER_BOOLEAN_STRUCT_SIZE;
            }

            // 4 bytes is the smallest possible size of a supported extension
            if ( cert_buffer_ptr + 4 > next_extension_offset ) {
                return CERTIFICATE_VERIFY_FAIL;
            }
            // extension data must have the OCTET_STRING type
            if ( der_cert_buffer[cert_buffer_ptr] != ASN1_TYPE_TAG_OCTET_STRING ) {
                return CERTIFICATE_VERIFY_FAIL;
            }
            if ( next_extension_offset != cert_buffer_ptr + pfr_decode_der_goto_next_increment(&der_cert_buffer[cert_buffer_ptr]) ) {
                return CERTIFICATE_VERIFY_FAIL;
            }

            alt_u32 extension_verify_success = CERTIFICATE_VERIFY_FAIL;
            cert_buffer_ptr += pfr_decode_der_goto_value_increment(&der_cert_buffer[cert_buffer_ptr]);
            switch ( extension_type ) {
                case 1:
                    cert_extension_context->basic_constraints_is_critical = extension_has_critical_field;
                    extension_verify_success = pfr_verify_der_certificate_content_extensions_basic_constraints(der_cert_buffer, cert_buffer_ptr, next_extension_offset, cert_extension_context);
                    break;
                case 2:
                    cert_extension_context->key_usage_is_critical = extension_has_critical_field;
                    extension_verify_success = pfr_verify_der_certificate_content_extensions_key_usage(der_cert_buffer, cert_buffer_ptr, next_extension_offset, cert_extension_context);
                    break;
                case 3:
                    cert_extension_context->extended_key_usage_is_critical = extension_has_critical_field;
                    extension_verify_success = pfr_verify_der_certificate_content_extensions_extended_key_usage(der_cert_buffer, cert_buffer_ptr, next_extension_offset, cert_extension_context);
                    break;
            }

            if ( !extension_verify_success ) {
                return CERTIFICATE_VERIFY_FAIL;
            }

        }

        // point to next extension
        extensions_offset = next_extension_offset;
    }

    if ( cert_type == ROOT_CERT || cert_type == INTERMEDIATE_CERT ) {
        if ( cert_extension_context->basic_constraints_defined && !cert_extension_context->cert_is_ca ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
        if ( cert_extension_context->key_usage_defined && !cert_extension_context->key_cert_sign_ku_flag_set ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
        if ( cert_extension_context->extended_key_usage_defined && !(cert_extension_context->server_auth_eku_defined || cert_extension_context->client_auth_eku_defined) ) {
            if ( !cert_extension_context->any_extended_key_usage_eku_defined ) {
                return CERTIFICATE_VERIFY_FAIL;
            }
        }
    } else { // cert_type == LEAF_CERT
        if ( cert_extension_context->basic_constraints_defined && cert_extension_context->cert_is_ca ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
        if ( cert_extension_context->key_usage_defined && !cert_extension_context->digital_signature_ku_flag_set ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
        if ( cert_extension_context->key_cert_sign_ku_flag_set ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
        if ( cert_extension_context->extended_key_usage_defined && !(cert_extension_context->server_auth_eku_defined || cert_extension_context->client_auth_eku_defined) ) {
            if ( !cert_extension_context->any_extended_key_usage_eku_defined ) {
                return CERTIFICATE_VERIFY_FAIL;
            }
        }
    }

    return CERTIFICATE_VERIFY_PASS;
}

/**
 * @brief DER verify - verify certificate signature
 */
static alt_u32 pfr_verify_der_certificate_signature(alt_u8* der_cert_buffer, alt_u32 cert_buffer_ptr, alt_u32 cert_buffer_ptr_limit, alt_u32 signed_content_offset, alt_u32 signed_content_size, alt_u32* sign_cx, alt_u32* sign_cy, alt_u32* sig_r, alt_u32* sig_s) {

    SHORT_DER* signature_bitstring_short_der = (SHORT_DER*) &der_cert_buffer[cert_buffer_ptr];
    if (signature_bitstring_short_der->type != ASN1_TYPE_TAG_BIT_STRING) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // enter SEQUENCE containing sig_r and sig_s INTEGER values
    cert_buffer_ptr += + 5; // skip: SHORT_DER_SIZE (BTISTRING), 1st byte, and SHORT_DER_SIZE (SEQUENCE)
    if ( cert_buffer_ptr + SHORT_DER_SIZE >= cert_buffer_ptr_limit) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    SHORT_DER* sig_r_short_der = (SHORT_DER*) &der_cert_buffer[cert_buffer_ptr];
    if ( cert_buffer_ptr + SHORT_DER_SIZE + sig_r_short_der->length > cert_buffer_ptr_limit) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    //check first byte of INTEGER value (handle be 0x00)
    alt_u8* sig_r_first_byte = &der_cert_buffer[cert_buffer_ptr + 2];
    if ( sig_r_short_der->length > 49 ) {
        // sig_r length should never >= 50 for ecdsaWith384 (sha length = 48)
        return CERTIFICATE_VERIFY_FAIL;
    } else if ( sig_r_short_der->length == 49 ) {
        alt_u8_memcpy((alt_u8*) sig_r, &der_cert_buffer[cert_buffer_ptr + 3], SHA384_LENGTH);
    } else if ( *sig_r_first_byte != 0x00 && sig_r_short_der->length == 48 ) {
        alt_u8_memcpy((alt_u8*) sig_r, &der_cert_buffer[cert_buffer_ptr + 2], SHA384_LENGTH);
    } else {
        //handle sig_r with length < 48
        alt_u32 first_byte_skip = 0;
        alt_u32 clear_byte_size = SHA384_LENGTH - sig_r_short_der->length;
        if ( *sig_r_first_byte == 0x00 ) {
            first_byte_skip = 1;
        }
        reset_buffer((alt_u8*) sig_r, clear_byte_size);
        alt_u8* sig_r_u8 = (alt_u8*) sig_r;
        alt_u8_memcpy(&sig_r_u8[clear_byte_size], &der_cert_buffer[cert_buffer_ptr + 2 + first_byte_skip], sig_r_short_der->length);
    }
    // ptr increment to sig_s tlv
    cert_buffer_ptr += 2 + sig_r_short_der->length;

    if ( cert_buffer_ptr + SHORT_DER_SIZE >= cert_buffer_ptr_limit) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    SHORT_DER* sig_s_short_der = (SHORT_DER*) &der_cert_buffer[cert_buffer_ptr];
    if ( cert_buffer_ptr + SHORT_DER_SIZE + sig_s_short_der->length > cert_buffer_ptr_limit) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    //check first byte of INTEGER value (handle be 0x00)
    alt_u8* sig_s_first_byte = &der_cert_buffer[cert_buffer_ptr + 2];
    if ( sig_s_short_der->length > 49 ) {
        return CERTIFICATE_VERIFY_FAIL;
    } else if ( sig_s_short_der->length == 49 ) {
        alt_u8_memcpy((alt_u8*) sig_s, &der_cert_buffer[cert_buffer_ptr + 3], SHA384_LENGTH);
    } else if ( *sig_s_first_byte != 0x00 && sig_s_short_der->length == 48 ) {
        alt_u8_memcpy((alt_u8*) sig_s, &der_cert_buffer[cert_buffer_ptr + 2], SHA384_LENGTH);
    } else {
        //handle sig_s with length < 48
        alt_u32 first_byte_skip = 0;
        alt_u32 clear_byte_size = SHA384_LENGTH - sig_s_short_der->length;
        if ( *sig_s_first_byte == 0x00 ) {
            first_byte_skip = 1;
        }
        reset_buffer((alt_u8*) sig_s, clear_byte_size);
        alt_u8* sig_s_u8 = (alt_u8*) sig_s;
        alt_u8_memcpy(&sig_s_u8[clear_byte_size], &der_cert_buffer[cert_buffer_ptr + 2 + first_byte_skip], sig_s_short_der->length);
    }


    alt_u32 signed_content_buffer[round_up_to_multiple_of_4(signed_content_size)/4];
    alt_u8_memcpy((alt_u8*) signed_content_buffer, &der_cert_buffer[signed_content_offset], signed_content_size);

    // verify signature
    if ( 0 == verify_ecdsa_and_sha(sign_cx, sign_cy, sig_r, sig_s, 0, signed_content_buffer, signed_content_size, CRYPTO_384_MODE, DISENGAGE_DMA) ) {
        return CERTIFICATE_VERIFY_FAIL;
    }


    return CERTIFICATE_VERIFY_PASS;
}

/**
 * @brief DER verify - verify certificate from full der cert buffer; returns decoded cert size if cert verification is successful
 *
 * checks performed:
 *  - signature algorithm
 *     - is ecdsaWith384?
 *  - version
 *     - is version 3?
 *  - signature algorithm identifier
 *     - is ecdsaWith384?
 *  - validity period
 *     - own_validity_from <= cert_validity_till
 *     - cert_validity_from <= cert_validity_till
 *  - public key
 *     - is secp384r1?
 *  - extensions
 *     - basic constraints
 *     - key usage
 *     - extended key usage
 *  - verify signature
 *
 * Traversal to access signature:
 * > SEQUENCE
 *      |-- SEQUENCE (signed content/ to-be-signed certificate/ tbscertificate)
 *      |      |-- version < CHECK 2
 *      |      |-- serial number
 *      |      |-- signature algorithm identifier < CHECK 3
 *      |      |-- issuer name
 *      |      |-- validity period < CHECK 4
 *      |      |-- subject name
 *      |      |-- SEQUENCE (subject's public key info) < CHECK 5
 *      |             |-- SEQUENCE (public key type)
 *      |             |-- BITSTRING (public key)
 *      |      |-- extensions < CHECK 6
 *      |-- SEQUENCE (signature algorithm)
 *      |      |-- OID (ecdsaWithSHA384) < CHECK 1
 *      |-- BITSTRING (certificate signature) < CHECK 7
 *
 * @NOTES:
 *   - Checks offset against cert size frequently to prevent out-of-bounds on verifying cert with invalid 'length' values
 *   - Can handle datetime values with UTCTIME of GENERALIZEDTIME types encoded in the validity field
 *
 * @param der_cert_buffer : buffer pointing to head (byte 0) of DER encoded certificate
 * @param der_cert_buffer_size_limit : define cert buffer size limit; for preventing out-of-bound access
 * @param sign_cx : public key cx value for verifying cert signature
 * @param sign_cy : public key cy value for verifying cert signature
 * @param cert_cx : cert public key cx value (decoded from der_cert_buffer)
 * @param cert_cy : cert public key cy value (decoded from der_cert_buffer)
 * @param sig_r : cert signature sig r value
 * @param sig_s : cert signature sig s value
 * @param cert_type : define if certificate is a root cert, intermediate cert or leaf cert in a certificate chain; certificate field values will be verified differently
 * @param cert_extension_context : carry information decoded from the certificate's extension field
 *
 * @NOTES:
 *   - For self-signed certificates, assign sign_cx the same pointer as cert_cx; replicate this for sign_cy and cert_cy
 */
static alt_u32 pfr_verify_der_certificate(alt_u8* der_cert_buffer, alt_u32 der_cert_buffer_size_limit, alt_u32* sign_cx, alt_u32* sign_cy, alt_u32* cert_cx, alt_u32* cert_cy, alt_u32* sig_r, alt_u32* sig_s, CERT_TYPE cert_type, CERT_EXTENSION_CONTEXT* cert_extension_context) {

    alt_u32 signed_content_offset = 0;
    alt_u32 signature_algorithm_offset = 0;
    alt_u32 certificate_signature_offset = 0;
    alt_u32 ptr_offset = 0;
    alt_u32 der_decoded_cert_size = 0;
    alt_u32 certificate_verify_status = CERTIFICATE_VERIFY_FAIL;

    // CHECK 0: confirm offset of certificate components and check cert size
    certificate_verify_status = pfr_verify_der_certificate_check_offsets(der_cert_buffer, der_cert_buffer_size_limit, &signed_content_offset, &signature_algorithm_offset, &certificate_signature_offset, &der_decoded_cert_size);
    if ( CERTIFICATE_VERIFY_FAIL == certificate_verify_status ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // CHECK 1: Certificate signature algorithm
    certificate_verify_status = pfr_verify_der_certificate_signature_algorithm(der_cert_buffer, signature_algorithm_offset, certificate_signature_offset);
    if ( CERTIFICATE_VERIFY_FAIL == certificate_verify_status ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // ptr enter signed content sequence structure
    ptr_offset = signed_content_offset + pfr_decode_der_goto_value_increment(&der_cert_buffer[signed_content_offset]);
    if ( ptr_offset >= signature_algorithm_offset ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // CHECK 2: Certificate version
    certificate_verify_status = pfr_verify_der_certificate_content_version(der_cert_buffer, ptr_offset, signature_algorithm_offset);
    if ( CERTIFICATE_VERIFY_FAIL == certificate_verify_status ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // ptr increment through sequence structure (skip next 2 elements to reach validity period)
    for (alt_u8 i = 0; i < 2; i++) {
        ptr_offset += pfr_decode_der_goto_next_increment(&der_cert_buffer[ptr_offset]);
        if ( ptr_offset >= signature_algorithm_offset ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
    }
    // CHECK 3: Signature algorithm identifier in signed content
    certificate_verify_status = pfr_verify_der_certificate_content_signature_algorithm_identifier(der_cert_buffer, ptr_offset, signature_algorithm_offset);
    if ( CERTIFICATE_VERIFY_FAIL == certificate_verify_status ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // ptr increment to reach certificate validity period
    for (alt_u8 i = 0; i < 2; i++) {
        ptr_offset += pfr_decode_der_goto_next_increment( &der_cert_buffer[ptr_offset]);
        if ( ptr_offset >= signature_algorithm_offset ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
    }
    // CHECK 4: Validity period
    certificate_verify_status = pfr_verify_der_certificate_content_validity_period(der_cert_buffer, ptr_offset, signature_algorithm_offset);
    if ( CERTIFICATE_VERIFY_FAIL == certificate_verify_status ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // ptr increment through sequence structure (skip next 2 elements to reach subject's public key info)
    for (alt_u8 i = 0; i < 2; i++) {
        ptr_offset += pfr_decode_der_goto_next_increment(&der_cert_buffer[ptr_offset]);
        if ( ptr_offset >= signature_algorithm_offset ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
    }
    // CHECK 5: Subject's public key info
    certificate_verify_status = pfr_verify_der_certificate_content_public_key_info(der_cert_buffer, ptr_offset, signature_algorithm_offset, cert_cx, cert_cy);
    if ( CERTIFICATE_VERIFY_FAIL == certificate_verify_status ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // ptr increment to reach extensions
    ptr_offset += pfr_decode_der_goto_next_increment(&der_cert_buffer[ptr_offset]);
    if ( ptr_offset >= signature_algorithm_offset ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // CHECK 6: Extensions
    certificate_verify_status = pfr_verify_der_certificate_content_extensions(der_cert_buffer, ptr_offset, signature_algorithm_offset, cert_type, cert_extension_context);
    if ( CERTIFICATE_VERIFY_FAIL == certificate_verify_status ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    // CHECK 7: Certificate signature
    certificate_verify_status = pfr_verify_der_certificate_signature(der_cert_buffer, certificate_signature_offset, der_decoded_cert_size, signed_content_offset, signature_algorithm_offset - signed_content_offset, sign_cx, sign_cy, sig_r, sig_s);
    if ( CERTIFICATE_VERIFY_FAIL == certificate_verify_status ) {
        return CERTIFICATE_VERIFY_FAIL;
    }
    return der_decoded_cert_size;

}

/**
 * @brief DER verify - verify certificate chain
 *
 * @param cert_chain_buffer : buffer pointing to head (byte 0) of DER encoded certificate
 * @param der_cert_buffer_size_limit : define cert buffer size limit; for preventing out-of-bound access
 * @param leaf_cert_cx : returns subject's public key cx value in leaf cert
 * @param leaf_cert_cy : returns subject's public key cy value in leaf cert
 */
static alt_u8 pfr_verify_der_cert_chain(alt_u8* cert_chain_buffer, alt_u32 der_cert_buffer_size_limit, alt_u32* leaf_cert_cx, alt_u32* leaf_cert_cy) {

    // get certificate chain length
    alt_u32 cert_chain_length = 0;
    cert_chain_length = pfr_certificate_get_chain_length(cert_chain_buffer, der_cert_buffer_size_limit);
    if (cert_chain_length == 0) {
        return CERTIFICATE_VERIFY_FAIL;
    }

    alt_u32 sign_cx[SHA384_LENGTH/4];
    alt_u32 sign_cy[SHA384_LENGTH/4];
    alt_u32 sig_r[SHA384_LENGTH/4];
    alt_u32 sig_s[SHA384_LENGTH/4];
    CERT_EXTENSION_CONTEXT cert_extension_context;
    CERT_EXTENSION_CONTEXT* cert_extension_context_ptr = &cert_extension_context;

    // First cert in chain is self-signed
    alt_u32 cert_chain_ptr = 0;
    alt_u32 cert_size = 0;
    alt_u32 cert_verify_output = CERTIFICATE_VERIFY_FAIL;
    CERT_TYPE cert_type = ROOT_CERT;
    cert_chain_length -= 1;

    // Iterate through cert chain
    for ( alt_u32 cert_layer = 0; cert_layer <= cert_chain_length; cert_layer++ ) {
        cert_type = cert_layer == 0 ? ROOT_CERT : ( cert_layer == cert_chain_length ? LEAF_CERT : INTERMEDIATE_CERT );

        cert_size = pfr_get_certificate_size(&cert_chain_buffer[cert_chain_ptr]);
        if ( cert_size == 0 || cert_size >= MAX_SUPPORTED_CERT_SIZE ) {
            return CERTIFICATE_VERIFY_FAIL;
        }
        if ( cert_size > der_cert_buffer_size_limit - cert_chain_ptr ) {
            return CERTIFICATE_VERIFY_FAIL;
        }

        if ( cert_type == ROOT_CERT ) {
            cert_verify_output = pfr_verify_der_certificate(cert_chain_buffer, cert_size, sign_cx, sign_cy, sign_cx, sign_cy, sig_r, sig_s, ROOT_CERT, cert_extension_context_ptr);
        } else {
            cert_verify_output = pfr_verify_der_certificate(&cert_chain_buffer[cert_chain_ptr], cert_size, sign_cx, sign_cy, leaf_cert_cx, leaf_cert_cy, sig_r, sig_s, cert_type, cert_extension_context_ptr);
        }

        if ( cert_verify_output == CERTIFICATE_VERIFY_FAIL ) {
            return CERTIFICATE_VERIFY_FAIL;
        }

        // check for path length constraint violation
        if ( cert_extension_context_ptr->path_length_constraint_enabled ) {
            if( cert_extension_context_ptr->path_length_constraint_value + cert_layer < cert_chain_length - 1 ) {
                return CERTIFICATE_VERIFY_FAIL;
            }
        }

        if ( cert_type != ROOT_CERT ) {
            alt_u8_memcpy((alt_u8*) sign_cx, (alt_u8*) leaf_cert_cx, SHA384_LENGTH);
            alt_u8_memcpy((alt_u8*) sign_cy, (alt_u8*) leaf_cert_cy, SHA384_LENGTH);
        }

        cert_chain_ptr += cert_verify_output;
    }


    if (cert_chain_ptr != der_cert_buffer_size_limit) {
        return CERTIFICATE_VERIFY_FAIL;
    }

    return CERTIFICATE_VERIFY_PASS;

}



// Cert generation code

/**
 * @brief DER generate - version field
 */
static alt_u32 pfr_generate_der_certificate_content_version(alt_u8* buf, alt_u32 size) {
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_IDENTIFIER_OCTET_0, VERSION_OCTET_IDENTIFIER_LENGTH);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_INTEGER, CERT_VERSION_LENGTH);
    buf[size] = CERT_VERSION;
    size += 1;
    return size;
}

/**
 * @brief DER generate - serial number field
 */
static alt_u32 pfr_generate_der_certificate_content_serial_number(alt_u8* buf, alt_u32 size) {
    alt_u8 serial_number[CERT_SERIAL_NUMBER_LENGTH] = CERT_SERIAL_NUMBER;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_INTEGER, CERT_SERIAL_NUMBER_LENGTH, serial_number);
    return size;
}

/**
 * @brief DER generate - signature algorithm identifier field
 */
static alt_u32 pfr_generate_der_certificate_content_signature_algorithm_identifier(alt_u8* buf, alt_u32 size) {
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, SIGNATURE_ALGORITHM_IDENTIFIER_SEQUENCE_LENGTH);

    alt_u8 certificate_algorithm[ASN1_OID_ECDSAWITHSHA384_LENGTH] = ASN1_OID_ECDSAWITHSHA384;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_ECDSAWITHSHA384_LENGTH, certificate_algorithm);

    // no parameters need to be defined for signature_algorithm_identifier
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_NULL, SIGNATURE_ALGORITHM_IDENTIFIER_PARAMETER_LENGTH);

    return size;
}

/**
 * @brief DER generate - issuer name field
 */
static alt_u32 pfr_generate_der_certificate_content_issuer_name(alt_u8* buf, alt_u32 size) {

    size += pfr_encode_der_type_long_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_SEQUENCE_LENGTH);

    // country name
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SET, ISSUER_NAME_COUNTRY_NAME_SET_LENGTH);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_COUNTRY_NAME_SEQUENCE_LENGTH);
    alt_u8 country_oid[ASN1_OID_COUNTRY_NAME_LENGTH] = ASN1_OID_COUNTRY_NAME;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_COUNTRY_NAME_LENGTH, country_oid);
    alt_u8 country_name[ISSUER_NAME_COUNTRY_NAME_LENGTH] = ISSUER_NAME_COUNTRY_NAME_VALUE;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_PRINTABLESTRING, ISSUER_NAME_COUNTRY_NAME_LENGTH, country_name);

    // state or province name
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SET, ISSUER_NAME_STATE_OR_PROVINCE_NAME_SET_LENGTH);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_STATE_OR_PROVINCE_NAME_SEQUENCE_LENGTH);
    alt_u8 state_or_province_name_oid[ASN1_OID_STATE_OR_PROVINCE_NAME_LENGTH] = ASN1_OID_STATE_OR_PROVINCE_NAME;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_STATE_OR_PROVINCE_NAME_LENGTH, state_or_province_name_oid);
    alt_u8 state_or_province_name[ISSUER_NAME_STATE_OR_PROVINCE_NAME_LENGTH] = ISSUER_NAME_STATE_OR_PROVINCE_NAME_VALUE;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_UTF8STRING, ISSUER_NAME_STATE_OR_PROVINCE_NAME_LENGTH, state_or_province_name);

    // locality name
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SET, ISSUER_NAME_LOCALITY_NAME_SET_LENGTH);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_LOCALITY_NAME_SEQUENCE_LENGTH);
    alt_u8 locality_name_oid[ASN1_OID_LOCALITY_NAME_LENGTH] = ASN1_OID_LOCALITY_NAME;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_LOCALITY_NAME_LENGTH, locality_name_oid);
    alt_u8 locality_name[ISSUER_NAME_LOCALITY_NAME_LENGTH] = ISSUER_NAME_LOCALITY_NAME_VALUE;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_UTF8STRING, ISSUER_NAME_LOCALITY_NAME_LENGTH, locality_name);

    // organization name
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SET, ISSUER_NAME_ORGANIZATION_NAME_SET_LENGTH);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_ORGANIZATION_NAME_SEQUENCE_LENGTH);
    alt_u8 organization_name_oid[ASN1_OID_ORGANIZATION_NAME_LENGTH] = ASN1_OID_ORGANIZATION_NAME;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_ORGANIZATION_NAME_LENGTH, organization_name_oid);
    alt_u8 organization_name[ISSUER_NAME_ORGANIZATION_NAME_LENGTH] = ISSUER_NAME_ORGANIZATION_NAME_VALUE;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_UTF8STRING, ISSUER_NAME_ORGANIZATION_NAME_LENGTH, organization_name);

    // organizational unit name
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SET, ISSUER_NAME_ORGANIZATIONAL_UNIT_NAME_SET_LENGTH);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_ORGANIZATIONAL_UNIT_NAME_SEQUENCE_LENGTH);
    alt_u8 organizational_unit_name_oid[ASN1_OID_ORGANIZATIONAL_UNIT_NAME_LENGTH] = ASN1_OID_ORGANIZATIONAL_UNIT_NAME;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_ORGANIZATIONAL_UNIT_NAME_LENGTH, organizational_unit_name_oid);
    alt_u8 organizational_unit_name[ISSUER_NAME_ORGANIZATIONAL_UNIT_NAME_LENGTH] = ISSUER_NAME_ORGANIZATIONAL_UNIT_NAME_VALUE;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_UTF8STRING, ISSUER_NAME_ORGANIZATIONAL_UNIT_NAME_LENGTH, organizational_unit_name);

    // common name
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SET, ISSUER_NAME_COMMON_NAME_SET_LENGTH);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_COMMON_NAME_SEQUENCE_LENGTH);
    alt_u8 common_name_oid[ASN1_OID_COMMON_NAME_LENGTH] = ASN1_OID_COMMON_NAME;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_COMMON_NAME_LENGTH, common_name_oid);
    alt_u8 common_name[ISSUER_NAME_COMMON_NAME_LENGTH] = ISSUER_NAME_COMMON_NAME_VALUE_ROOT;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_UTF8STRING, ISSUER_NAME_COMMON_NAME_LENGTH, common_name);

    // email address
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SET, ISSUER_NAME_EMAIL_ADDRESS_SET_LENGTH);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_EMAIL_ADDRESS_SEQUENCE_LENGTH);
    alt_u8 email_address_oid[ASN1_OID_EMAIL_ADDRESS_LENGTH] = ASN1_OID_EMAIL_ADDRESS;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_EMAIL_ADDRESS_LENGTH, email_address_oid);
    alt_u8 email_address[ISSUER_NAME_EMAIL_ADDRESS_LENGTH] = ISSUER_NAME_EMAIL_ADDRESS_VALUE;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_IA5STRING, ISSUER_NAME_EMAIL_ADDRESS_LENGTH, email_address);

    return size;
}

/**
 * @brief DER generate - subject name field
 */
static alt_u32 pfr_generate_der_certificate_content_subject_name(alt_u8* buf, alt_u32 size) {

    size += pfr_encode_der_type_long_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_SEQUENCE_LENGTH);

    // country name
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SET, ISSUER_NAME_COUNTRY_NAME_SET_LENGTH);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_COUNTRY_NAME_SEQUENCE_LENGTH);
    alt_u8 country_oid[ASN1_OID_COUNTRY_NAME_LENGTH] = ASN1_OID_COUNTRY_NAME;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_COUNTRY_NAME_LENGTH, country_oid);
    alt_u8 country_name[ISSUER_NAME_COUNTRY_NAME_LENGTH] = ISSUER_NAME_COUNTRY_NAME_VALUE;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_PRINTABLESTRING, ISSUER_NAME_COUNTRY_NAME_LENGTH, country_name);

    // state or province name
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SET, ISSUER_NAME_STATE_OR_PROVINCE_NAME_SET_LENGTH);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_STATE_OR_PROVINCE_NAME_SEQUENCE_LENGTH);
    alt_u8 state_or_province_name_oid[ASN1_OID_STATE_OR_PROVINCE_NAME_LENGTH] = ASN1_OID_STATE_OR_PROVINCE_NAME;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_STATE_OR_PROVINCE_NAME_LENGTH, state_or_province_name_oid);
    alt_u8 state_or_province_name[ISSUER_NAME_STATE_OR_PROVINCE_NAME_LENGTH] = ISSUER_NAME_STATE_OR_PROVINCE_NAME_VALUE;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_UTF8STRING, ISSUER_NAME_STATE_OR_PROVINCE_NAME_LENGTH, state_or_province_name);

    // locality name
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SET, ISSUER_NAME_LOCALITY_NAME_SET_LENGTH);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_LOCALITY_NAME_SEQUENCE_LENGTH);
    alt_u8 locality_name_oid[ASN1_OID_LOCALITY_NAME_LENGTH] = ASN1_OID_LOCALITY_NAME;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_LOCALITY_NAME_LENGTH, locality_name_oid);
    alt_u8 locality_name[ISSUER_NAME_LOCALITY_NAME_LENGTH] = ISSUER_NAME_LOCALITY_NAME_VALUE;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_UTF8STRING, ISSUER_NAME_LOCALITY_NAME_LENGTH, locality_name);

    // organization name
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SET, ISSUER_NAME_ORGANIZATION_NAME_SET_LENGTH);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_ORGANIZATION_NAME_SEQUENCE_LENGTH);
    alt_u8 organization_name_oid[ASN1_OID_ORGANIZATION_NAME_LENGTH] = ASN1_OID_ORGANIZATION_NAME;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_ORGANIZATION_NAME_LENGTH, organization_name_oid);
    alt_u8 organization_name[ISSUER_NAME_ORGANIZATION_NAME_LENGTH] = ISSUER_NAME_ORGANIZATION_NAME_VALUE;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_UTF8STRING, ISSUER_NAME_ORGANIZATION_NAME_LENGTH, organization_name);

    // organizational unit name
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SET, ISSUER_NAME_ORGANIZATIONAL_UNIT_NAME_SET_LENGTH);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_ORGANIZATIONAL_UNIT_NAME_SEQUENCE_LENGTH);
    alt_u8 organizational_unit_name_oid[ASN1_OID_ORGANIZATIONAL_UNIT_NAME_LENGTH] = ASN1_OID_ORGANIZATIONAL_UNIT_NAME;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_ORGANIZATIONAL_UNIT_NAME_LENGTH, organizational_unit_name_oid);
    alt_u8 organizational_unit_name[ISSUER_NAME_ORGANIZATIONAL_UNIT_NAME_LENGTH] = ISSUER_NAME_ORGANIZATIONAL_UNIT_NAME_VALUE;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_UTF8STRING, ISSUER_NAME_ORGANIZATIONAL_UNIT_NAME_LENGTH, organizational_unit_name);

    // common name
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SET, ISSUER_NAME_COMMON_NAME_SET_LENGTH);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_COMMON_NAME_SEQUENCE_LENGTH);
    alt_u8 common_name_oid[ASN1_OID_COMMON_NAME_LENGTH] = ASN1_OID_COMMON_NAME;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_COMMON_NAME_LENGTH, common_name_oid);
    alt_u8 common_name[ISSUER_NAME_COMMON_NAME_LENGTH] = ISSUER_NAME_COMMON_NAME_VALUE_LEAF;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_UTF8STRING, ISSUER_NAME_COMMON_NAME_LENGTH, common_name);

    // email address
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SET, ISSUER_NAME_EMAIL_ADDRESS_SET_LENGTH);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, ISSUER_NAME_EMAIL_ADDRESS_SEQUENCE_LENGTH);
    alt_u8 email_address_oid[ASN1_OID_EMAIL_ADDRESS_LENGTH] = ASN1_OID_EMAIL_ADDRESS;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_EMAIL_ADDRESS_LENGTH, email_address_oid);
    alt_u8 email_address[ISSUER_NAME_EMAIL_ADDRESS_LENGTH] = ISSUER_NAME_EMAIL_ADDRESS_VALUE;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_IA5STRING, ISSUER_NAME_EMAIL_ADDRESS_LENGTH, email_address);

    return size;
}

/**
 * @brief DER generate - validity period field
 */
static alt_u32 pfr_generate_der_certificate_content_validity_period(alt_u8* buf, alt_u32 size) {

    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, VALIDITY_PERIOD_SEQUENCE_LENGTH);

    // validity from date
    alt_u8 validity_from[CERT_VALIDITY_UTCTIME_LENGTH] = CERT_VALIDITY_FROM;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_UTCTIME, CERT_VALIDITY_UTCTIME_LENGTH, validity_from);

    // validity till date
    alt_u8 validity_till[CERT_VALIDITY_UTCTIME_LENGTH] = CERT_VALIDITY_TILL;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_UTCTIME, CERT_VALIDITY_UTCTIME_LENGTH, validity_till);

    return size;
}

/**
 * @brief DER generate - public key info field
 */
static alt_u32 pfr_generate_der_certificate_content_public_key_info(alt_u8* buf, alt_u32 size, alt_u32* subject_public_key) {

    size += pfr_encode_der_type_long_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, PUBLIC_KEY_INFO_SEQUENCE_LENGTH);

    // declare algorithm
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, PUBLIC_KEY_INFO_ALGORITHM_SEQUENCE_LENGTH);
    alt_u8 ecpublickey_oid[ASN1_OID_ECPUBLICKEY_LENGTH] = ASN1_OID_ECPUBLICKEY;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_ECPUBLICKEY_LENGTH, ecpublickey_oid);
    alt_u8 secp384r1_oid[ASN1_OID_SECP384R1_LENGTH] = ASN1_OID_SECP384R1;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_SECP384R1_LENGTH, secp384r1_oid);

    // write public key as BIT STRING type. Note that length is SHA384 key length +2 additional bytes
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_BIT_STRING, PUBLIC_KEY_INFO_KEY_LENGTH);
    buf[size] = PUBLIC_KEY_INFO_KEY_1ST_BYTE;
    buf[size + 1] = PUBLIC_KEY_INFO_KEY_2ND_BYTE;
    // copy public key raw value
    alt_u8_memcpy(&buf[size + 2] , (alt_u8*) subject_public_key, SHA384_LENGTH*2);
    size += PUBLIC_KEY_INFO_KEY_LENGTH;

    return size;

}

/**
 * @brief DER generate - 'extension is critical' label
 */
static alt_u32 pfr_generate_der_certificate_content_extensions_is_critical_field(alt_u8* buf, alt_u8 extension_is_critical) {
    if ( extension_is_critical ) {
        buf[0] = ASN1_TYPE_TAG_BOOLEAN;
        buf[1] = ASN1_DER_BOOLEAN_LENGTH;
        buf[2] = ASN1_DER_BOOLEAN_TRUE;
        return ASN1_DER_BOOLEAN_STRUCT_SIZE;
    }
    return 0;
}

/**
 * @brief DER generate - basic constraints extension field
 */
static alt_u32 pfr_generate_der_certificate_content_extensions_basic_constraints(alt_u8* buf, alt_u8 extension_is_critical, alt_u8 cert_is_ca, alt_u8 path_length_contraints_enabled, alt_u8 path_length_contraints_value) {
    alt_u32 size = 0;
    alt_u32 basic_constraint_sequence_length = 0;
    if ( !cert_is_ca ) {
        basic_constraint_sequence_length = 9;
    } else {
        if ( !path_length_contraints_enabled ) {
            basic_constraint_sequence_length = 12;
        } else {
            basic_constraint_sequence_length = 15;
        }
    }
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, basic_constraint_sequence_length + (extension_is_critical ? 3 : 0));
    alt_u8 basic_constraints_oid[ASN1_OID_EXTENSIONS_UNIVERSAL_LENGTH] = ASN1_OID_EXT_BASIC_CONSTRAINTS;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_EXTENSIONS_UNIVERSAL_LENGTH, basic_constraints_oid);
    size += pfr_generate_der_certificate_content_extensions_is_critical_field(&buf[size], extension_is_critical);
    if ( !cert_is_ca ) {
        alt_u8 basic_constraints_value[BASIC_CONSTRAINTS_NOT_CA_OCTET_STRING_LENGTH] = BASIC_CONSTRAINTS_NOT_CA_OCTET_STRING;
        size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OCTET_STRING, BASIC_CONSTRAINTS_NOT_CA_OCTET_STRING_LENGTH, basic_constraints_value);
    } else {
        if ( !path_length_contraints_enabled ) {
            alt_u8 basic_constraints_value[BASIC_CONSTRAINTS_IS_CA_OCTET_STRING_LENGTH] = BASIC_CONSTRAINTS_IS_CA_OCTET_STRING;
            size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OCTET_STRING, BASIC_CONSTRAINTS_IS_CA_OCTET_STRING_LENGTH, basic_constraints_value);
        } else {
            alt_u8 basic_constraints_value[BASIC_CONSTRAINTS_IS_CA_PATH_LENGTH_CONSTRAINT_OCTET_STRING_LENGTH] = BASIC_CONSTRAINTS_IS_CA_PATH_LENGTH_CONSTRAINT_OCTET_STRING;
            if ( path_length_contraints_value <= 127 ) {
                basic_constraints_value[BASIC_CONSTRAINTS_IS_CA_PATH_LENGTH_CONSTRAINT_OCTET_STRING_LENGTH-1] = path_length_contraints_value;
            }
            size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OCTET_STRING, BASIC_CONSTRAINTS_IS_CA_PATH_LENGTH_CONSTRAINT_OCTET_STRING_LENGTH, basic_constraints_value);
        }
    }
    return size;
}

/**
 * @brief DER generate - key usage extension field
 */
static alt_u32 pfr_generate_der_certificate_content_extensions_key_usage(alt_u8* buf, alt_u8 extension_is_critical, alt_u16 key_usage) {
    alt_u32 size = 0;
    alt_u16 key_usage_bit_string = key_usage << 7;
    alt_u32 key_usage_bit_string_length = (((key_usage) & X509_KEY_USAGE_FLAG_DECIPHER_ONLY) ? 3 : 2);
    alt_u32 key_usage_octet_string_length = 2 + key_usage_bit_string_length;
    alt_u32 key_usage_sequence_length = 7 + ((extension_is_critical) ? 3 : 0) + key_usage_octet_string_length;

    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, key_usage_sequence_length);
    alt_u8 key_usage_oid[ASN1_OID_EXTENSIONS_UNIVERSAL_LENGTH] = ASN1_OID_EXT_KEY_USAGE;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_EXTENSIONS_UNIVERSAL_LENGTH, key_usage_oid);
    size += pfr_generate_der_certificate_content_extensions_is_critical_field(&buf[size], extension_is_critical);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_OCTET_STRING, key_usage_octet_string_length);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_BIT_STRING, key_usage_bit_string_length);
    alt_u8 key_usage_unused_bits = 7;
    alt_u16 unused_bits_mask = 0x00ff;
    while ( !(key_usage_bit_string & unused_bits_mask) ) {
        key_usage_unused_bits += 1;
        if (unused_bits_mask >= 0x8000) {
            break;
        }
        unused_bits_mask <<= 1;
        unused_bits_mask += 1;
    }
    buf[size] = key_usage_unused_bits % 8;
    buf[size + 1] = key_usage_bit_string >> 8;
    if ( key_usage_bit_string_length == 3 ) {
        buf[size + 2] = key_usage_bit_string & 0xff;
    }
    size += key_usage_bit_string_length;
    return size;
}

/**
 * @brief DER generate - extended key usage extension field
 */
static alt_u32 pfr_generate_der_certificate_content_extensions_extended_key_usage(alt_u8* buf, alt_u8 extension_is_critical, alt_u16 extended_key_usage) {
    alt_u32 size = 0;

    alt_u32 extended_key_usage_definitions_sequence_length = ((extended_key_usage & CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_SERVER_AUTH) ? 2 + ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH : 0) + ((extended_key_usage & CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_CLIENT_AUTH) ? 2 + ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH : 0) + ((extended_key_usage & CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_CODE_SIGNING) ? 2 + ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH : 0) + ((extended_key_usage & CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_EMAIL_PROTECTION) ? 2 + ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH : 0) + ((extended_key_usage & CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_TIME_STAMPING) ? 2 + ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH : 0) + ((extended_key_usage & CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_OCSP_SIGNING) ? 2+ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH : 0) + ((extended_key_usage & CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_ANY_EXTENDED_KEY_USAGE) ? 2 + ASN1_OID_EXT_EXTENDED_KEY_USAGE_ANY_EXTENDED_KEY_USAGE_LENGTH : 0);
    alt_u32 extended_key_usage_octet_string_length = 2 + extended_key_usage_definitions_sequence_length;
    alt_u32 extended_key_usage_sequence_length = 7 + ((extension_is_critical) ? 3 : 0) + extended_key_usage_octet_string_length;

    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, extended_key_usage_sequence_length);
    alt_u8 extended_key_usage_oid[ASN1_OID_EXTENSIONS_UNIVERSAL_LENGTH] = ASN1_OID_EXT_EXTENDED_KEY_USAGE;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_EXTENSIONS_UNIVERSAL_LENGTH, extended_key_usage_oid);
    size += pfr_generate_der_certificate_content_extensions_is_critical_field(&buf[size], extension_is_critical);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_OCTET_STRING, extended_key_usage_octet_string_length);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, extended_key_usage_definitions_sequence_length);
    if ( extended_key_usage & CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_ANY_EXTENDED_KEY_USAGE ) {
        alt_u8 extended_key_usage_definition_oid[ASN1_OID_EXT_EXTENDED_KEY_USAGE_ANY_EXTENDED_KEY_USAGE_LENGTH] = ASN1_OID_EXT_EXTENDED_KEY_USAGE_ANY_EXTENDED_KEY_USAGE;
        size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_EXT_EXTENDED_KEY_USAGE_ANY_EXTENDED_KEY_USAGE_LENGTH, extended_key_usage_definition_oid);
    }
    if ( extended_key_usage & CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_SERVER_AUTH ) {
        alt_u8 extended_key_usage_definition_oid[ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH] = ASN1_OID_EXT_EXTENDED_KEY_USAGE_SERVER_AUTH;
        size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH, extended_key_usage_definition_oid);
    }
    if ( extended_key_usage & CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_CLIENT_AUTH ) {
        alt_u8 extended_key_usage_definition_oid[ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH] = ASN1_OID_EXT_EXTENDED_KEY_USAGE_CLIENT_AUTH;
        size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH, extended_key_usage_definition_oid);
    }
    if ( extended_key_usage & CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_CODE_SIGNING ) {
        alt_u8 extended_key_usage_definition_oid[ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH] = ASN1_OID_EXT_EXTENDED_KEY_USAGE_CODE_SIGNING;
        size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH, extended_key_usage_definition_oid);
    }
    if ( extended_key_usage & CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_EMAIL_PROTECTION ) {
        alt_u8 extended_key_usage_definition_oid[ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH] = ASN1_OID_EXT_EXTENDED_KEY_USAGE_EMAIL_PROTECTION;
        size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH, extended_key_usage_definition_oid);
    }
    if ( extended_key_usage & CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_TIME_STAMPING ) {
        alt_u8 extended_key_usage_definition_oid[ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH] = ASN1_OID_EXT_EXTENDED_KEY_USAGE_TIME_STAMPING;
        size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH, extended_key_usage_definition_oid);
    }
    if ( extended_key_usage & CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_OCSP_SIGNING ) {
        alt_u8 extended_key_usage_definition_oid[ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH] = ASN1_OID_EXT_EXTENDED_KEY_USAGE_OCSP_SIGNING;
        size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH, extended_key_usage_definition_oid);
    }

    return size;
}

/**
 * @brief DER generate -  root certificate extensions field
 */
static alt_u32 pfr_generate_der_certificate_content_extensions_root(alt_u8* buf, alt_u32 size) {
    size += pfr_encode_der_type_long_length(buf, size, ASN1_TYPE_TAG_IDENTIFIER_OCTET_3, EXTENSIONS_ROOT_LENGTH);
    size += pfr_encode_der_type_long_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, EXTENSIONS_ROOT_SEQUENCE_LENGTH);

#if EXTENSIONS_ROOT_CERT_BASIC_CONSTRAINTS_IS_ENABLED
    size += pfr_generate_der_certificate_content_extensions_basic_constraints(&buf[size], EXTENSIONS_ROOT_CERT_BASIC_CONSTRAINTS_IS_CRITICAL, EXTENSIONS_ROOT_CERT_BASIC_CONSTRAINTS_IS_CA, EXTENSIONS_ROOT_CERT_BASIC_CONSTRAINTS_PATH_LENGTH_CONSTRAINTS_IS_ENABLED, EXTENSIONS_ROOT_CERT_BASIC_CONSTRAINTS_PATH_LENGTH_CONSTRAINTS_VALUE);
#endif

#if EXTENSIONS_ROOT_CERT_KEY_USAGE_IS_ENABLED
    size += pfr_generate_der_certificate_content_extensions_key_usage(&buf[size], EXTENSIONS_ROOT_CERT_KEY_USAGE_IS_CRITICAL, EXTENSIONS_ROOT_CERT_KEY_USAGE_FLAGS);
#endif

#if EXTENSIONS_ROOT_CERT_EXTENDED_KEY_USAGE_IS_ENABLED
    size += pfr_generate_der_certificate_content_extensions_extended_key_usage(&buf[size], EXTENSIONS_ROOT_CERT_EXTENDED_KEY_USAGE_IS_CRITICAL, EXTENSIONS_ROOT_CERT_EXTENDED_KEY_USAGE_DEFINITIONS);
#endif

    return size;
}

/**
 * @brief DER generate -  leaf certificate extensions field
 */
static alt_u32 pfr_generate_der_certificate_content_extensions_leaf(alt_u8* buf, alt_u32 size) {
    size += pfr_encode_der_type_long_length(buf, size, ASN1_TYPE_TAG_IDENTIFIER_OCTET_3, EXTENSIONS_LEAF_LENGTH);
    size += pfr_encode_der_type_long_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, EXTENSIONS_LEAF_SEQUENCE_LENGTH);

#if EXTENSIONS_LEAF_CERT_BASIC_CONSTRAINTS_IS_ENABLED
    size += pfr_generate_der_certificate_content_extensions_basic_constraints(&buf[size], EXTENSIONS_LEAF_CERT_BASIC_CONSTRAINTS_IS_CRITICAL, EXTENSIONS_LEAF_CERT_BASIC_CONSTRAINTS_IS_CA, EXTENSIONS_LEAF_CERT_BASIC_CONSTRAINTS_PATH_LENGTH_CONSTRAINTS_IS_ENABLED, EXTENSIONS_LEAF_CERT_BASIC_CONSTRAINTS_PATH_LENGTH_CONSTRAINTS_VALUE);
#endif

#if EXTENSIONS_LEAF_CERT_KEY_USAGE_IS_ENABLED
    size += pfr_generate_der_certificate_content_extensions_key_usage(&buf[size], EXTENSIONS_LEAF_CERT_KEY_USAGE_IS_CRITICAL, EXTENSIONS_LEAF_CERT_KEY_USAGE_FLAGS);
#endif

#if EXTENSIONS_LEAF_CERT_EXTENDED_KEY_USAGE_IS_ENABLED
    size += pfr_generate_der_certificate_content_extensions_extended_key_usage(&buf[size], EXTENSIONS_LEAF_CERT_EXTENDED_KEY_USAGE_IS_CRITICAL, EXTENSIONS_LEAF_CERT_EXTENDED_KEY_USAGE_DEFINITIONS);
#endif

    return size;
}

/**
 * @brief DER generate -  signature algorithm field
 */
static alt_u32 pfr_generate_der_certificate_signature_algorithm(alt_u8* buf, alt_u32 size) {

    alt_u8 certificate_algorithm[ASN1_OID_ECDSAWITHSHA384_LENGTH] = ASN1_OID_ECDSAWITHSHA384;
    size += pfr_encode_der_type_length_value(buf, size, ASN1_TYPE_TAG_OBJECT_IDENTIFIER, ASN1_OID_ECDSAWITHSHA384_LENGTH, certificate_algorithm);
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_NULL, SIGNATURE_ALGORITHM_PARAMETERS_LENGTH);

    return size;
}

/**
 * @brief DER generate -  signature field
 */
static alt_u32 pfr_generate_der_certificate_signature(alt_u8* buf, alt_u32 size, alt_u32* cert_signature_gen_pubkey, alt_u32* cert_signature_gen_privkey, alt_u32 cert_layer) {

    alt_u32 signature_size = 0;

    // skip writing header. Need to obtain signature size first
    alt_u32 signature_header_ptr = size;
    size += SHORT_DER_SIZE;

    // First value of BIT STRING type declares number of unused bytes, in this case = 0
    buf[size] = 0x00;
    size += 1;

    // For ecdsa-with-384, the signature of the signed content is stored instead of a encrypted hash.
    // The signature integers {r, s} are stored as separate INTEGER values in a sequence.
    // Skip since need to obtain signature size first
    alt_u32 signature_rs_sequence_ptr = size;
    size += SHORT_DER_SIZE;

    // generate signature and write into certificate DER encoding buffer
    alt_u32 sig_r[SHA384_LENGTH/4];
    alt_u32 sig_s[SHA384_LENGTH/4];
    alt_u8* sig_r_u8 = (alt_u8*) sig_r;
    alt_u8* sig_s_u8 = (alt_u8*) sig_s;

    alt_u32 cert_signed_content_length = CERT_SIGNED_CONTENT_LENGTH_ROOT;
    alt_u32 cert_signed_content_header_length = CERT_SIGNED_CONTENT_HEADER_LENGTH_ROOT;
    if ( cert_layer == 1 ) {
        cert_signed_content_length = CERT_SIGNED_CONTENT_LENGTH_LEAF;
        cert_signed_content_header_length = CERT_SIGNED_CONTENT_HEADER_LENGTH_LEAF;
    }

    alt_u32 signed_content_buffer[round_up_to_multiple_of_4(cert_signed_content_length)/4];
    alt_u8_memcpy((alt_u8*) signed_content_buffer, &buf[cert_signed_content_header_length], cert_signed_content_length);

    if (!generate_ecdsa_signature ( sig_r,  sig_s,
            (const alt_u32*) signed_content_buffer, cert_signed_content_length,
            cert_signature_gen_pubkey, cert_signature_gen_privkey, CRYPTO_384_MODE))
    {
        return 0;
    }

    // check for number of clear bytes (0x00) in sig_r most significant bytes; Very rare, but possible occurence
    alt_u32 sig_r_clear_bytes = 0;
    for (alt_u32 byte_i = 0; byte_i < SHA384_LENGTH; byte_i++) {
        if ( sig_r_u8[byte_i] != 0x00 ) {
            sig_r_clear_bytes = byte_i;
            break;
        }
    }
    // check if sig_r needs to be encoded as a negative DER INTEGER; if negative, need to add a clear byte (0x00) in front
    alt_u32 sig_r_is_negative = 0;
    if ( sig_r_u8[sig_r_clear_bytes] & 0b10000000 ) {
        sig_r_is_negative = 1;
    }
    alt_u32 sig_r_value_length = SHA384_LENGTH - sig_r_clear_bytes + sig_r_is_negative;
    // write sig_r INTEGER header
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_INTEGER, sig_r_value_length);
    // write 0x00 for first byte if sig_r is intepreted as negative integer (MSBit == 1)
    if ( sig_r_is_negative == 1 ) {
        buf[size] = 0x00;
    }
    // write sig_r into cert buffer
    alt_u8_memcpy(&buf[size + sig_r_is_negative], &sig_r_u8[sig_r_clear_bytes], sig_r_value_length - sig_r_is_negative);
    size += sig_r_value_length;

    // check for number of clear bytes (0x00) in sig_s most significant bytes; Very rare, but possible occurence
    alt_u32 sig_s_clear_bytes = 0;
    for (alt_u32 byte_i = 0; byte_i < SHA384_LENGTH; byte_i++) {
        if ( sig_s_u8[byte_i] != 0x00 ) {
            sig_s_clear_bytes = byte_i;
            break;
        }
    }
    // check if sig_s needs to be encoded as a negative DER INTEGER; if negative, need to add a clear byte (0x00) in front
    alt_u32 sig_s_is_negative = 0;
    if ( sig_s_u8[sig_s_clear_bytes] & 0b10000000 ) {
        sig_s_is_negative = 1;
    }
    alt_u32 sig_s_value_length = SHA384_LENGTH - sig_s_clear_bytes + sig_s_is_negative;
    // write sig_s INTEGER header
    size += pfr_encode_der_type_length(buf, size, ASN1_TYPE_TAG_INTEGER, sig_s_value_length);
    // write 0x00 for first byte if sig_s is intepreted as negative integer (MSBit == 1)
    if ( sig_s_is_negative == 1 ) {
        buf[size] = 0x00;
    }
    // write sig_s into cert buffer
    alt_u8_memcpy(&buf[size + sig_s_is_negative], &sig_s_u8[sig_s_clear_bytes], sig_s_value_length - sig_s_is_negative);

    // write header length values skipped, now that exact signature length is obtained
    pfr_encode_der_type_length(buf, signature_header_ptr, ASN1_TYPE_TAG_BIT_STRING, 7 + sig_r_value_length + sig_s_value_length);
    pfr_encode_der_type_length(buf, signature_rs_sequence_ptr, ASN1_TYPE_TAG_SEQUENCE, 4 + sig_r_value_length + sig_s_value_length);
    signature_size = 9 + sig_r_value_length + sig_s_value_length;

    return signature_size;
}

/**
 * @brief DER generate - CPLD root certificate
 *
 * @param buf : x509 certificate will be generated here
 * @param cert_subject_pubkey : public key to be stored in the certificate (under subject's public key info field)
 * @param cert_signature_gen_pubkey : public key used for generating certificate signature
 * @param cert_signature_gen_privkey : private key used for generating certificate signature
 */
static alt_u32 pfr_generate_der_certificate_root(alt_u8* buf, alt_u32* cert_subject_pubkey, alt_u32* cert_signature_gen_pubkey, alt_u32* cert_signature_gen_privkey) {

    /** DER sequence structure for 3 elems:
     * - certificate signed content
     * - certificate signature algorithm
     * - encrypted hash, which in this case is the signed contents ecdsa384 signature
     */
    alt_u32 size = 0;
    size += pfr_encode_der_type_long_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, CERT_ALL_CONTENTS_SEQUENCE_LENGTH_ROOT);

    // generate certificate signed content
    size += pfr_encode_der_type_long_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, CERT_SIGNED_CONTENT_SEQUENCE_LENGTH_ROOT);
    size = pfr_generate_der_certificate_content_version(buf, size);
    size = pfr_generate_der_certificate_content_serial_number(buf, size);
    size = pfr_generate_der_certificate_content_signature_algorithm_identifier(buf, size);
    size = pfr_generate_der_certificate_content_issuer_name(buf, size);
    size = pfr_generate_der_certificate_content_validity_period(buf, size);
    size = pfr_generate_der_certificate_content_issuer_name(buf, size);
    size = pfr_generate_der_certificate_content_public_key_info(buf, size, cert_subject_pubkey);
    size = pfr_generate_der_certificate_content_extensions_root(buf, size);

    // generate certificate signature algorithm
    size += pfr_encode_der_type_long_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, CERT_SIGNATURE_ALGORITHM_SEQUENCE_LENGTH);
    size = pfr_generate_der_certificate_signature_algorithm(buf, size);

    // generate certificate encrypted hash: in this case is the ecdsa384 signature of the certificate signed content
    alt_u32 cert_signature_size = pfr_generate_der_certificate_signature(buf, size, cert_signature_gen_pubkey, cert_signature_gen_privkey, 0);

    if (!cert_signature_size)
    {
        return 0;
    }
    size += cert_signature_size;

    // update full cert der length now that signature size is obtained
    pfr_encode_der_type_long_length(buf, 0, ASN1_TYPE_TAG_SEQUENCE, CERT_SIGNED_CONTENT_LENGTH_ROOT + CERT_SIGNATURE_ALGORITHM_LENGTH + cert_signature_size);

    return size;
}

/**
 * @brief DER generate - CPLD leaf certificate
 *
 * @param buf : x509 certificate will be generated here
 * @param cert_subject_pubkey : public key to be stored in the certificate (under subject's public key info field)
 * @param cert_signature_gen_pubkey : public key used for generating certificate signature
 * @param cert_signature_gen_privkey : private key used for generating certificate signature
 */
static alt_u32 pfr_generate_der_certificate_leaf(alt_u8* buf, alt_u32* cert_subject_pubkey, alt_u32* cert_signature_gen_pubkey, alt_u32* cert_signature_gen_privkey) {

    /** DER sequence structure for 3 elems:
     * - certificate signed content
     * - certificate signature algorithm
     * - encrypted hash, which in this case is the signed contents ecdsa384 signature
     */
    alt_u32 size = 0;
    size += pfr_encode_der_type_long_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, CERT_ALL_CONTENTS_SEQUENCE_LENGTH_LEAF);

    // generate certificate signed content
    size += pfr_encode_der_type_long_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, CERT_SIGNED_CONTENT_SEQUENCE_LENGTH_LEAF);
    size = pfr_generate_der_certificate_content_version(buf, size);
    size = pfr_generate_der_certificate_content_serial_number(buf, size);
    size = pfr_generate_der_certificate_content_signature_algorithm_identifier(buf, size);
    size = pfr_generate_der_certificate_content_issuer_name(buf, size);
    size = pfr_generate_der_certificate_content_validity_period(buf, size);
    size = pfr_generate_der_certificate_content_subject_name(buf, size);
    size = pfr_generate_der_certificate_content_public_key_info(buf, size, cert_subject_pubkey);
    size = pfr_generate_der_certificate_content_extensions_leaf(buf, size);

    // generate certificate signature algorithm
    size += pfr_encode_der_type_long_length(buf, size, ASN1_TYPE_TAG_SEQUENCE, CERT_SIGNATURE_ALGORITHM_SEQUENCE_LENGTH);
    size = pfr_generate_der_certificate_signature_algorithm(buf, size);

    // generate certificate encrypted hash: in this case is the ecdsa384 signature of the certificate signed content
    alt_u32 cert_signature_size = pfr_generate_der_certificate_signature(buf, size, cert_signature_gen_pubkey, cert_signature_gen_privkey, 1);

    if (!cert_signature_size)
    {
        return 0;
    }
    size += cert_signature_size;

    // update full cert der length now that signature size is obtained
    pfr_encode_der_type_long_length(buf, 0, ASN1_TYPE_TAG_SEQUENCE, CERT_SIGNED_CONTENT_LENGTH_LEAF + CERT_SIGNATURE_ALGORITHM_LENGTH + cert_signature_size);

    return size;
}

/**
 * @brief Generates certificate chain (layer 0 root cert and layer 1 leaf cert)
 */
static alt_u32 pfr_generate_certificate_chain(alt_u8* buf, alt_u32* privkey_0, alt_u32* pubkey_0, alt_u32* pubkey_1) {
    alt_u32 root_cert_chain_size = pfr_generate_der_certificate_root(buf, pubkey_0, pubkey_0, privkey_0);
    alt_u32 leaf_cert_chain_size =  pfr_generate_der_certificate_leaf(&buf[root_cert_chain_size], pubkey_1, pubkey_0, privkey_0);

    if ((root_cert_chain_size == 0) || (leaf_cert_chain_size == 0))
    {
        return 0;
    }
    return (root_cert_chain_size + leaf_cert_chain_size);
}

#endif

#endif /* EAGLESTREAM_INC_CERT_FLOW_H_ */

