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
 * @file cert.h
 * @brief Define X509 data structures and policies used in PFR certification.
 */

#ifndef EAGLESTREAM_INC_CERT_H_
#define EAGLESTREAM_INC_CERT_H_

#include "pfr_sys.h"

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

// Macros
#define DER_FORMAT_LENGTH_MSB_MSK 0b10000000

#define ASN_DER_MAX_BYTE_LENGTH   4

// ASN.1 Type values for x509 certificate DER encoding
#define ASN1_TYPE_TAG_BOOLEAN               0x01
#define ASN1_TYPE_TAG_INTEGER               0x02
#define ASN1_TYPE_TAG_BIT_STRING            0x03
#define ASN1_TYPE_TAG_OCTET_STRING          0x04
#define ASN1_TYPE_TAG_NULL                  0x05
#define ASN1_TYPE_TAG_OBJECT_IDENTIFIER     0x06
#define ASN1_TYPE_TAG_UTF8STRING            0x0C
#define ASN1_TYPE_TAG_PRINTABLESTRING       0x13
#define ASN1_TYPE_TAG_IA5STRING             0x16
#define ASN1_TYPE_TAG_UTCTIME               0x17
#define ASN1_TYPE_TAG_GENERALIZEDTIME       0x18
#define ASN1_TYPE_TAG_SEQUENCE              0x30
#define ASN1_TYPE_TAG_SET                   0x31
#define ASN1_TYPE_TAG_IDENTIFIER_OCTET_0    0xA0 // certificate signed content version field
#define ASN1_TYPE_TAG_IDENTIFIER_OCTET_3    0xA3 // certificate signed content extensions field


// ASN.1 Object Identifiers values
// - value defined in hexadecimal array; dot notation OID described in comments
#define ASN1_OID_ECDSAWITHSHA384 {0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x03} //OID: ecdsaWithSHA384 1.2.840.10045.4.3.3
#define ASN1_OID_ECDSAWITHSHA384_LENGTH   8 //length in bytes

#define ASN1_OID_ECPUBLICKEY {0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01}  //OID: 1.2.840.10045.2.1 ecPublicKey
#define ASN1_OID_ECPUBLICKEY_LENGTH   7 //length in bytes
#define ASN1_OID_SECP384R1 {0x2b, 0x81, 0x04, 0x00, 0x22}  //OID: 1.3.132.0.34 secp384r1
#define ASN1_OID_SECP384R1_LENGTH   5 //length in bytes

#define ASN1_OID_COUNTRY_NAME {0x55, 0x04, 0x06}  //OID: countryName 2.5.4.6
#define ASN1_OID_COUNTRY_NAME_LENGTH   3
#define ASN1_OID_STATE_OR_PROVINCE_NAME {0x55, 0x04, 0x08}  //OID: stateOrProvinceName 2.5.4.8
#define ASN1_OID_STATE_OR_PROVINCE_NAME_LENGTH   3
#define ASN1_OID_LOCALITY_NAME {0x55, 0x04, 0x07}  //OID: localityName 2.5.4.7
#define ASN1_OID_LOCALITY_NAME_LENGTH   3
#define ASN1_OID_ORGANIZATION_NAME {0x55, 0x04, 0x0a}  //OID: organizationName 2.5.4.10
#define ASN1_OID_ORGANIZATION_NAME_LENGTH   3
#define ASN1_OID_ORGANIZATIONAL_UNIT_NAME {0x55, 0x04, 0x0b}  //OID: organizationalUnitName 2.5.4.11
#define ASN1_OID_ORGANIZATIONAL_UNIT_NAME_LENGTH   3
#define ASN1_OID_COMMON_NAME {0x55, 0x04, 0x03} //OID: commonName 2.5.4.3
#define ASN1_OID_COMMON_NAME_LENGTH   3
#define ASN1_OID_EMAIL_ADDRESS {0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x01} //OID: emailAddress 1.2.840.113549.1.9.1
#define ASN1_OID_EMAIL_ADDRESS_LENGTH   9

#define ASN1_OID_EXT_BASIC_CONSTRAINTS  {0x55, 0x1d, 0x13} //OID: basicConstraints 2.5.29.19
#define ASN1_OID_EXT_KEY_USAGE          {0x55, 0x1d, 0x0f} //OID: keyUsage         2.5.29.15
#define ASN1_OID_EXT_EXTENDED_KEY_USAGE {0x55, 0x1d, 0x25} //OID: extKeyUsage      2.5.29.37
#define ASN1_OID_EXTENSIONS_UNIVERSAL_LENGTH   3  //shared between basicConstraints, keyUsage and extKeyUsage OIDs

#define X509_BASIC_CONSTRAINTS_IS_CA   1
#define X509_BASIC_CONSTRAINTS_NOT_CA   0

#define X509_KEY_USAGE_FLAG_DIGITAL_SIGNATURE   0b100000000  // bit 0
#define X509_KEY_USAGE_FLAG_NON_REPUDIATION     0b010000000  // bit 1
#define X509_KEY_USAGE_FLAG_KEY_ENCIPHERMENT    0b001000000  // bit 2
#define X509_KEY_USAGE_FLAG_DATA_ENCIPHERMENT   0b000100000  // bit 3
#define X509_KEY_USAGE_FLAG_KEY_AGREEMENT       0b000010000  // bit 4
#define X509_KEY_USAGE_FLAG_KEY_CERT_SIGN       0b000001000  // bit 5
#define X509_KEY_USAGE_FLAG_CRL_SIGN            0b000000100  // bit 6
#define X509_KEY_USAGE_FLAG_ENCIPHER_ONLY       0b000000010  // bit 7
#define X509_KEY_USAGE_FLAG_DECIPHER_ONLY       0b000000001  // bit 8
#define X509_KEY_USAGE_BIT_STRING_FULL_LENGTH   9

#define ASN1_OID_EXT_EXTENDED_KEY_USAGE_ANY_EXTENDED_KEY_USAGE   {0x55, 0x1d, 0x25, 0x00} //OID: anyExtendedKeyUsage   2.5.29.37.0
#define ASN1_OID_EXT_EXTENDED_KEY_USAGE_ANY_EXTENDED_KEY_USAGE_LENGTH   4
#define ASN1_OID_EXT_EXTENDED_KEY_USAGE_SERVER_AUTH        {0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01} //OID: serverAuth      1.3.6.1.5.5.7.3.1
#define ASN1_OID_EXT_EXTENDED_KEY_USAGE_CLIENT_AUTH        {0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x02} //OID: clientAuth      1.3.6.1.5.5.7.3.2
#define ASN1_OID_EXT_EXTENDED_KEY_USAGE_CODE_SIGNING       {0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x03} //OID: codeSigning     1.3.6.1.5.5.7.3.3
#define ASN1_OID_EXT_EXTENDED_KEY_USAGE_EMAIL_PROTECTION   {0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x04} //OID: emailProtection 1.3.6.1.5.5.7.3.4
#define ASN1_OID_EXT_EXTENDED_KEY_USAGE_TIME_STAMPING      {0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x08} //OID: timeStamping    1.3.6.1.5.5.7.3.8
#define ASN1_OID_EXT_EXTENDED_KEY_USAGE_OCSP_SIGNING       {0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x09} //OID: ocspSigning     1.3.6.1.5.5.7.3.9
#define ASN1_OID_EXT_EXTENDED_KEY_USAGE_UNIVERSAL_LENGTH   8  //shared between extended key usage OIDs above
// flags for recognized extended key usage definitions
#define CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_ANY_EXTENDED_KEY_USAGE   0b1000000  // bit 0
#define CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_SERVER_AUTH              0b0100000  // bit 1
#define CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_CLIENT_AUTH              0b0010000  // bit 2
#define CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_CODE_SIGNING             0b0001000  // bit 3
#define CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_EMAIL_PROTECTION         0b0000100  // bit 4
#define CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_TIME_STAMPING            0b0000010  // bit 5
#define CERT_EXTENSIONS_EXTENDED_KEY_USAGE_DEFINE_OCSP_SIGNING             0b0000001  // bit 6

typedef enum
{
    ROOT_CERT  = 0,
    INTERMEDIATE_CERT  = 1,
    LEAF_CERT = 2,
}CERT_TYPE;

// x509 certificate chain verification context for tracking relevant values
typedef struct
{
    alt_u8 basic_constraints_defined;
    alt_u8 basic_constraints_is_critical;
    alt_u8 key_usage_defined;
    alt_u8 key_usage_is_critical;
    alt_u8 extended_key_usage_defined;
    alt_u8 extended_key_usage_is_critical;
    // basic constraint
    alt_u8 cert_is_ca;
    alt_u8 path_length_constraint_enabled;
    alt_u8 path_length_constraint_value;
    // key usage
    alt_u8 key_cert_sign_ku_flag_set;
    alt_u8 digital_signature_ku_flag_set;
    // extended key usage
    alt_u8 server_auth_eku_defined;
    alt_u8 client_auth_eku_defined;
    alt_u8 any_extended_key_usage_eku_defined;
} CERT_EXTENSION_CONTEXT;


// x509 Certificate value constants
#define CERT_VERSION_LENGTH   1 // Always 1 byte
#define CERT_VERSION_VALUE_3   0x02
#define CERT_VALIDITY_UTCTIME_LENGTH   13 //UTCTIME format: YYMMDDhhmmssZ
#define PUBLIC_KEY_INFO_KEY_LENGTH   98 // 2 + 2*SHA384_LENGTH
// for ASN1 DER encoding, first value of BIT STRING type must declare number of unused bytes, in this case = 0
#define PUBLIC_KEY_INFO_KEY_1ST_BYTE   0x00
// for storing elliptic-curve cryptography value, 2nd byte is used to declare compression type.
#define PUBLIC_KEY_INFO_KEY_2ND_BYTE   0x04 // 0x04 = uncompressed, i.e. value is stored as is.
#define CERT_SIGNATURE_STRUCTURE_LENGTH   105 // 105 // 1 + 2 + ( 2*(2 + 1 + SHA384_LENGTH) )

#define MAX_SUPPORTED_CERT_SIZE   MAX_CERT_CHAIN_SIZE // overflow guard for certificate decoding process

// ASN.1 DER boolean type values
#define ASN1_DER_BOOLEAN_TRUE    0xff
#define ASN1_DER_BOOLEAN_FALSE   0x00
#define ASN1_DER_BOOLEAN_LENGTH   0x01
#define ASN1_DER_BOOLEAN_STRUCT_SIZE   3

/*!
 * Standard DER data structure (header)
 * A complete DER data structure can contain any size of value
 */
typedef struct __attribute__((__packed__))
{
    alt_u8 type;
    alt_u8 length;
    //alt_u8 value[];
} SHORT_DER;
#define SHORT_DER_SIZE   2

#define CERTIFICATE_VERIFY_PASS   1
#define CERTIFICATE_VERIFY_FAIL   0

/*!
 * x509 certificate datetime data (for storing UTCTIME or GENERALIZEDTIME data)
 */
typedef struct __attribute__((__packed__))
{
    alt_u8 year_0; //first 2 digits of year, i.e. '20' in 2049
    alt_u8 year_1; //latter 2 digits of year, i.e. '49' in 2049
    alt_u8 month;
    alt_u8 day;
    alt_u8 hour;
    alt_u8 minute;
    alt_u8 second;
} CERT_DER_DATETIME;
#define CERT_DER_DATETIME_SIZE   7

#define CERT_DER_DATETIME_UTCTIME           0
#define CERT_DER_DATETIME_GENERALIZEDTIME   1

#endif

#endif /* EAGLESTREAM_INC_CERT_H_ */
