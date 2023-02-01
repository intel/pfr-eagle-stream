#ifndef INC_TESTDATA_FILES_X509_CERTIFICATE_SECP384_H
#define INC_TESTDATA_FILES_X509_CERTIFICATE_SECP384_H

#ifdef GTEST_ATTEST_384

#define X509_CERT_GENERATE_SELFSIGNED_TEST_OUTPUT_DER_FILE "testdata/x509_certificate/test_certificate_generation_verify_der_encoding_selfsigned_certificate.der"
#define X509_CERT_GENERATE_SELFSIGNED_TEST_OUTPUT_PEM_FILE "testdata/x509_certificate/test_certificate_generation_verify_der_encoding_selfsigned_certificate.pem"

#define X509_CERT_GENERATE_CHAIN_LAYER_0_TEST_OUTPUT_DER_FILE "testdata/x509_certificate/test_certificate_generation_verify_der_encoding_chain_certificate_layer_0.der"
#define X509_CERT_GENERATE_CHAIN_LAYER_0_TEST_OUTPUT_PEM_FILE "testdata/x509_certificate/test_certificate_generation_verify_der_encoding_chain_certificate_layer_0.pem"
#define X509_CERT_GENERATE_CHAIN_LAYER_1_TEST_OUTPUT_DER_FILE "testdata/x509_certificate/test_certificate_generation_verify_der_encoding_chain_certificate_layer_1.der"
#define X509_CERT_GENERATE_CHAIN_LAYER_1_TEST_OUTPUT_PEM_FILE "testdata/x509_certificate/test_certificate_generation_verify_der_encoding_chain_certificate_layer_1.pem"


// sample certificate files with different sig rs lengths, and respective signing public key cx cy values
// - these are all self-signed certificates
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48 "testdata/x509_certificate/selfsigned_certificate_sig_rs_length_r48_s48.der"
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_SIZE 540
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_CX {0x9f, 0x2c, 0x1f, 0x66, 0x42, 0x91, 0xb1, 0x3b, 0x2f, 0xc4, 0x94, 0x11, 0x0a, 0x25, 0xf1, 0xe5, 0xe7, 0x1c, 0xe7, 0x78, 0x5a, 0x98, 0x93, 0xe4, 0x99, 0xf7, 0x01, 0x7d, 0xb1, 0x6f, 0x4d, 0x1a, 0x97, 0x80, 0x8f, 0x42, 0x23, 0x08, 0x8d, 0x7f, 0x6c, 0x94, 0xc7, 0x1c, 0x9d, 0x71, 0x00, 0x51}
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_CY {0x76, 0xa5, 0x87, 0x0f, 0x6c, 0xe8, 0x8b, 0xa7, 0xd5, 0x3c, 0xfa, 0xd7, 0x16, 0x79, 0x39, 0x83, 0x95, 0x06, 0x1c, 0x8f, 0xee, 0x46, 0xef, 0x4f, 0xfc, 0x11, 0x07, 0x87, 0x06, 0x58, 0x1c, 0xe2, 0x80, 0x09, 0x9a, 0xeb, 0x3d, 0x7d, 0x42, 0xdc, 0x9a, 0xc6, 0xd9, 0xbb, 0x07, 0x57, 0xaf, 0xbb}
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_SIG_R {0x61, 0x78, 0xac, 0xc6, 0x35, 0x41, 0x07, 0xbc, 0x3d, 0x77, 0xbb, 0x77, 0xfd, 0xa8, 0xdf, 0xdc, 0xd7, 0x9a, 0xef, 0xc1, 0x55, 0x8e, 0x70, 0x34, 0x21, 0xbb, 0xc9, 0xb4, 0x73, 0xe6, 0x97, 0xa3, 0xdc, 0xa1, 0xbc, 0x3e, 0x08, 0x2b, 0x23, 0x45, 0x56, 0x7e, 0xf0, 0xc5, 0x73, 0xeb, 0x6e, 0xc4}
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_SIG_S {0x33, 0x00, 0xbc, 0x6b, 0x18, 0x97, 0xa6, 0x4c, 0x5f, 0x5e, 0x74, 0x4b, 0x8d, 0xa2, 0xca, 0x9c, 0x7a, 0x3e, 0x42, 0xa4, 0x09, 0x62, 0x27, 0xc3, 0x77, 0x1b, 0x13, 0x6b, 0x0c, 0x10, 0xfa, 0x9b, 0x62, 0x37, 0x4c, 0x65, 0x1c, 0x71, 0xae, 0x36, 0x70, 0xb1, 0xa3, 0xe9, 0x0a, 0x92, 0x91, 0xaf}
// offset of fields/ values in certificate sample ; corrupt targets for negative certificate verification test cases
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_CERT_SIGNATURE_ALGORITHM_OFFSET 425
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_VERSION_OFFSET 10
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_SIGNED_CONTENT_SIGNATURE_ALGORITHM_OFFSET 37
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_VALIDITY_PERIOD_FROM_OFFSET 120
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_VALIDITY_PERIOD_TILL_OFFSET 135
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_SUBJECT_PUBLIC_KEY_INFO_ECPUBLICKEY_OID_OFFSET 225
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_SUBJECT_PUBLIC_KEY_INFO_SECP384R1_OID_OFFSET 234
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_SUBJECT_PUBLIC_KEY_INFO_PUBLIC_KEY_OFFSET 241
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_SIGNATURE_OFFSET 435
// offset of 'length' values in certificate sample ; corrupt targets for negative certificate verification test cases, where 'length' values may point to address outside of cert buffer size bound
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_CERT_SEQUENCE_LENGTH_OFFSET 2 // 2 bytes; Original value = 0x0218
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_SIGNED_CONTENT_LENGTH_OFFSET 6 // 2 bytes; Original value = 0x019F
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_SIGNATURE_ALGORITHM_LENGTH_OFFSET 424 // 1 byte; Original value = 0x0A
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_CERTIFICATE_SIGNATURE_LENGTH_OFFSET 436 // 1 byte; Original value = 0x67
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_VALIDITY_PERIOD_LENGTH_OFFSET 119 // 1 byte; Original value = 0x1E
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_48_SUBJECT_PUBLIC_KEY_INFO_LENGTH_OFFSET 222 // 1 byte; Original value = 0x76


#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_49_49 "testdata/x509_certificate/selfsigned_certificate_sig_rs_length_r49_s49.der"
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_49_49_SIZE 542
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_49_49_CX {0xe2, 0x0e, 0xbf, 0x53, 0xe9, 0x40, 0x7e, 0x12, 0x00, 0xfd, 0x7b, 0xa8, 0xbb, 0xa3, 0x9c, 0x0d, 0xb1, 0xb1, 0xf4, 0xcb, 0x65, 0x47, 0x58, 0x6d, 0xbc, 0xab, 0x77, 0xdc, 0xe0, 0x26, 0x5c, 0xa2, 0x00, 0x8b, 0x2e, 0xd5, 0xea, 0x78, 0x5b, 0x12, 0xee, 0x7f, 0x11, 0x78, 0x33, 0x68, 0x60, 0x36}
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_49_49_CY {0x3d, 0xd6, 0x2f, 0x4d, 0x13, 0x4f, 0xe3, 0x98, 0xf5, 0xe2, 0xe8, 0x88, 0x37, 0xf9, 0x03, 0xae, 0xf7, 0x5e, 0x92, 0x40, 0xf3, 0x24, 0x5e, 0x1b, 0xb9, 0xd1, 0x9b, 0xf8, 0xff, 0x2c, 0x45, 0x8b, 0x7e, 0x21, 0x4e, 0xc2, 0x73, 0xb7, 0x72, 0xf6, 0xff, 0x22, 0x44, 0x91, 0x20, 0xc1, 0xfc, 0xce}
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_49_49_SIG_R {0x9c, 0xd5, 0xcf, 0x19, 0xe5, 0x69, 0x16, 0xb7, 0xab, 0x1b, 0x87, 0x77, 0x68, 0xa5, 0x91, 0x66, 0x07, 0x4b, 0x33, 0xf6, 0x09, 0x9b, 0x16, 0x47, 0x7d, 0x3d, 0xef, 0xb6, 0xff, 0x07, 0xff, 0x23, 0x67, 0x0a, 0x94, 0xa9, 0x35, 0x73, 0xd1, 0x56, 0x19, 0x87, 0x76, 0xe8, 0xe7, 0x98, 0x35, 0x18}
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_49_49_SIG_S {0x94, 0x11, 0xa2, 0xb2, 0xca, 0xc8, 0x57, 0x3b, 0x84, 0xc2, 0x5d, 0x06, 0xe7, 0x78, 0x8b, 0x89, 0x83, 0x33, 0xb4, 0xd1, 0xe5, 0x4a, 0xe4, 0xf3, 0x79, 0x9f, 0x0a, 0x24, 0x58, 0x88, 0x3f, 0x3b, 0xdd, 0xc5, 0xf3, 0x80, 0xc5, 0x55, 0xd1, 0x66, 0xe8, 0x53, 0x9d, 0x4f, 0x70, 0x16, 0x0b, 0x98}

#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_49 "testdata/x509_certificate/selfsigned_certificate_sig_rs_length_r48_s49.der"
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_49_SIZE 541
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_49_CX {0xef, 0x96, 0x54, 0x4a, 0xf0, 0x60, 0x6a, 0x91, 0xd1, 0xad, 0xc0, 0x26, 0x88, 0xbf, 0xe4, 0x6a, 0xe0, 0xbd, 0xf3, 0x63, 0xc7, 0xa1, 0xdb, 0xe9, 0x8e, 0xe5, 0xb0, 0x68, 0x91, 0x8e, 0x37, 0xfb, 0x44, 0xf5, 0x9e, 0xf9, 0x5a, 0x3a, 0x69, 0xe1, 0x1a, 0xbe, 0x57, 0xfb, 0x1b, 0xeb, 0x6b, 0x49}
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_49_CY {0xb0, 0x0f, 0xe3, 0xaa, 0x18, 0x12, 0x52, 0xca, 0x82, 0x51, 0x14, 0xae, 0x1b, 0xa3, 0xe4, 0x48, 0x89, 0xc1, 0x85, 0x4f, 0x53, 0xf6, 0x59, 0xfe, 0xce, 0xb8, 0xf9, 0x6c, 0xf2, 0x57, 0x84, 0xe3, 0x73, 0x1b, 0xc6, 0xa6, 0xae, 0x88, 0x7d, 0x0b, 0xdd, 0x9d, 0xcb, 0xba, 0x9a, 0xe9, 0xde, 0x6f}
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_49_SIG_R {0x01, 0x0a, 0x40, 0xea, 0x49, 0x8d, 0x00, 0x56, 0x78, 0xbe, 0xe0, 0xa3, 0x5a, 0x82, 0x35, 0x06, 0x6a, 0xef, 0x71, 0x18, 0xd4, 0xf3, 0x9e, 0x58, 0x44, 0xf6, 0xd7, 0xcb, 0x83, 0xde, 0x12, 0x95, 0xed, 0x56, 0x29, 0x0a, 0x0a, 0xf0, 0xe3, 0xd4, 0x96, 0x98, 0xf5, 0xf4, 0x50, 0x3c, 0x25, 0x9d}
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_49_SIG_S {0xcb, 0xc2, 0x51, 0x9c, 0x19, 0xdd, 0x6e, 0x92, 0x55, 0xee, 0x9e, 0xbe, 0x8c, 0xdc, 0xe2, 0xe5, 0x1f, 0x36, 0xc9, 0xb5, 0x91, 0x35, 0x57, 0x6e, 0x0f, 0x9c, 0x4a, 0xa2, 0xd0, 0xe2, 0x4a, 0xe8, 0xeb, 0x7a, 0xf6, 0x9d, 0x70, 0x27, 0xf1, 0x0c, 0x12, 0xdf, 0x3f, 0x0f, 0x2d, 0x2a, 0xb4, 0x7a}

#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_49_48 "testdata/x509_certificate/selfsigned_certificate_sig_rs_length_r49_s48.der"
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_49_48_SIZE 541
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_49_48_CX {0x96, 0x46, 0x74, 0xc6, 0x17, 0x64, 0x47, 0x37, 0xf4, 0xaf, 0xbd, 0xbb, 0xf7, 0x95, 0xf6, 0xe3, 0xaf, 0x25, 0xc6, 0x62, 0xa0, 0x70, 0x7c, 0x4b, 0x62, 0x4e, 0xcd, 0xc0, 0x36, 0x37, 0x90, 0x8c, 0x1b, 0x28, 0x3c, 0xc7, 0x1a, 0x9f, 0xc6, 0xff, 0x83, 0xa9, 0xfb, 0x7a, 0xfd, 0xe2, 0xd3, 0xca}
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_49_48_CY {0x33, 0x8d, 0x84, 0xb2, 0x07, 0xbc, 0x9f, 0x8c, 0x07, 0xe6, 0x92, 0xbb, 0xda, 0xe1, 0xb7, 0x1b, 0xc8, 0x8c, 0xd2, 0xef, 0x61, 0x81, 0x4f, 0x49, 0xe7, 0x64, 0xab, 0xa5, 0x1a, 0xfb, 0xe0, 0xdd, 0xc7, 0x78, 0xd1, 0xf8, 0x06, 0x36, 0xbd, 0x47, 0x05, 0xda, 0x01, 0xe9, 0xf6, 0x07, 0x78, 0x36}
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_49_48_SIG_R {0xf4, 0xea, 0x2f, 0x64, 0x89, 0x01, 0x0f, 0x19, 0x3b, 0xe6, 0x53, 0x92, 0x27, 0xad, 0x92, 0xd8, 0x42, 0x18, 0x7c, 0x26, 0x68, 0xe8, 0x97, 0x63, 0xdf, 0xce, 0x1f, 0xf8, 0x30, 0xd9, 0xa8, 0x13, 0x1a, 0x39, 0x8f, 0x68, 0xb1, 0x4f, 0x48, 0xc9, 0x98, 0xeb, 0x9e, 0x78, 0x05, 0x09, 0xed, 0x67}
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_49_48_SIG_S {0x0e, 0xcb, 0x81, 0x7a, 0x74, 0xf0, 0x67, 0xae, 0x41, 0x9b, 0x5c, 0xc7, 0xed, 0xfc, 0xc0, 0xc0, 0xd7, 0x64, 0x8d, 0x40, 0xee, 0xfa, 0xab, 0x62, 0xb6, 0x3b, 0x3c, 0x2b, 0xb2, 0x98, 0xbc, 0xe3, 0x06, 0x0a, 0x4c, 0xfc, 0xf9, 0x53, 0xaf, 0x84, 0x10, 0xe4, 0xb3, 0xf4, 0xd3, 0xe0, 0x2f, 0x77}

#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_47 "testdata/x509_certificate/selfsigned_certificate_sig_rs_length_r48_s47.der"
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_47_SIZE 539
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_47_CX {0x3a, 0x77, 0xef, 0xfe, 0xd8, 0x7b, 0x95, 0xb1, 0x4b, 0x3d, 0xf0, 0xfa, 0x2c, 0x96, 0xb5, 0xae, 0x00, 0x73, 0x70, 0x9c, 0xce, 0x07, 0x45, 0xd7, 0x65, 0x43, 0xf3, 0xe3, 0x16, 0x25, 0x96, 0xff, 0x2b, 0x9d, 0xe4, 0xf2, 0x64, 0x8d, 0x07, 0x1f, 0xec, 0xf0, 0x6c, 0xae, 0x8d, 0x4f, 0x3a, 0x8d}
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_47_CY {0xd7, 0xcf, 0xef, 0xc4, 0xf3, 0x31, 0x47, 0xf7, 0x47, 0x64, 0xb9, 0xf9, 0x24, 0x11, 0x11, 0x60, 0xb4, 0x9f, 0x2a, 0x4b, 0x4a, 0x84, 0x53, 0xbc, 0x4a, 0x18, 0x87, 0x8c, 0x5e, 0xb1, 0xc4, 0xca, 0x72, 0x97, 0x41, 0x8e, 0xf0, 0xc1, 0x49, 0xf4, 0xb5, 0xfa, 0x57, 0xe4, 0xb4, 0x36, 0xff, 0x1e}
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_47_SIG_R {0x4b, 0x8a, 0x5d, 0x4f, 0x0c, 0x6c, 0x2d, 0x94, 0x08, 0x0a, 0x2c, 0x6a, 0xa7, 0x8d, 0x12, 0x4a, 0x87, 0xcd, 0xd1, 0x4c, 0x1d, 0x47, 0xf3, 0x69, 0x77, 0x8e, 0xed, 0xf6, 0xd1, 0x9f, 0xe0, 0x08, 0xd5, 0x8d, 0xfd, 0x4d, 0x19, 0x83, 0x72, 0xa8, 0xe7, 0x9a, 0x63, 0x47, 0x85, 0x94, 0x90, 0xae}
#define X509_SAMPLE_CERT_DER_FILE_SIG_RS_48_47_SIG_S {0x00, 0x03, 0x87, 0x61, 0xb3, 0xf1, 0x1c, 0x69, 0xbd, 0x03, 0x1f, 0xf2, 0xfc, 0x58, 0xae, 0x91, 0xf4, 0x59, 0x7c, 0xc6, 0x9b, 0xb4, 0xc4, 0x4e, 0xc4, 0x87, 0x7f, 0x7b, 0xd5, 0x3a, 0xeb, 0x04, 0x56, 0x61, 0xbb, 0xc5, 0x80, 0xd0, 0x11, 0x54, 0x18, 0x54, 0x73, 0xa3, 0x88, 0xe9, 0xbe, 0x92}


// sample certificate files with duplicate instances of select extensions; used for negative test case/ duplicate extension detection
// - these are all self-signed certificates
#define X509_SAMPLE_CERT_DER_FILE_DUPLICATE_EXTENSIONS_BASIC_CONSTRAINTS "testdata/x509_certificate/selfsigned_certificate_duplicate_extensions_test_basic_constraints.der"
#define X509_SAMPLE_CERT_DER_FILE_DUPLICATE_EXTENSIONS_BASIC_CONSTRAINTS_SIZE 576
#define X509_SAMPLE_CERT_DER_FILE_DUPLICATE_EXTENSIONS_KEY_USAGE "testdata/x509_certificate/selfsigned_certificate_duplicate_extensions_test_key_usage.der"
#define X509_SAMPLE_CERT_DER_FILE_DUPLICATE_EXTENSIONS_KEY_USAGE_SIZE 575
#define X509_SAMPLE_CERT_DER_FILE_DUPLICATE_EXTENSIONS_EXTENDED_KEY_USAGE "testdata/x509_certificate/selfsigned_certificate_duplicate_extensions_test_extended_key_usage.der"
#define X509_SAMPLE_CERT_DER_FILE_DUPLICATE_EXTENSIONS_EXTENDED_KEY_USAGE_SIZE 605


// sample certificate chains with erroneous values in the extension fields
// - All these certificate chains consist of 1 root cert and 1 leaf cert
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_ROOT_CERT_NOT_CA "testdata/x509_certificate/sample_cert_chain_invalid_extension_root_cert_not_ca.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_ROOT_CERT_NOT_CA_SIZE 1217
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_ROOT_CERT_NO_KEY_USAGE_SET "testdata/x509_certificate/sample_cert_chain_invalid_extension_root_cert_no_key_usage_set.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_ROOT_CERT_NO_KEY_USAGE_SET_SIZE 1221
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_ROOT_CERT_MISSING_KEY_CERT_SIGN_KEY_USAGE "testdata/x509_certificate/sample_cert_chain_invalid_extension_root_cert_missing_key_cert_sign_key_usage.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_ROOT_CERT_MISSING_KEY_CERT_SIGN_KEY_USAGE_SIZE 1217
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_ROOT_CERT_MISSING_SERVERAUTH_CLIENTAUTH_EXTENDED_KEY_USAGE "testdata/x509_certificate/sample_cert_chain_invalid_extension_root_cert_missing_serverauth_clientauth_extended_key_usage.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_ROOT_CERT_MISSING_SERVERAUTH_CLIENTAUTH_EXTENDED_KEY_USAGE_SIZE 1209
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_LEAF_CERT_IS_CA "testdata/x509_certificate/sample_cert_chain_invalid_extension_leaf_cert_is_ca.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_LEAF_CERT_IS_CA_SIZE 1223
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_LEAF_CERT_MISSING_DIGITAL_SIGNATURE_KEY_USAGE "testdata/x509_certificate/sample_cert_chain_invalid_extension_leaf_cert_missing_digital_signature_key_usage.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_LEAF_CERT_MISSING_DIGITAL_SIGNATURE_KEY_USAGE_SIZE 1219
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_LEAF_CERT_CONTAINS_KEY_CERT_SIGN_KEY_USAGE "testdata/x509_certificate/sample_cert_chain_invalid_extension_leaf_cert_contains_key_cert_sign_key_usage.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_LEAF_CERT_CONTAINS_KEY_CERT_SIGN_KEY_USAGE_SIZE 1218
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_LEAF_CERT_MISSING_SERVERAUTH_CLIENTAUTH_EXTENDED_KEY_USAGE "testdata/x509_certificate/sample_cert_chain_invalid_extension_leaf_cert_missing_serverauth_clientauth_extended_key_usage.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_INVALID_EXTENSION_LEAF_CERT_MISSING_SERVERAUTH_CLIENTAUTH_EXTENDED_KEY_USAGE_SIZE 1200


// sample certificate chains with various combinations of path length constraint values in the basic constraints extension
// - All these certificate chains consist of 1 root cert, 2 intermediate certs and 1 leaf cert
// Path length constraint values for following cert chain: 2    (root) -> 1    (intermediate_1) -> 0    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_0 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_0.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_0_SIZE 2430
// Path length constraint values for following cert chain: 3    (root) -> 2    (intermediate_1) -> 1    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_1 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_1.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_1_SIZE 2431
// Path length constraint values for following cert chain: 3    (root) -> 2    (intermediate_1) -> 4    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_2 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_2.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_2_SIZE 2430
// Path length constraint values for following cert chain: 3    (root) -> 4    (intermediate_1) -> 5    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_3 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_3.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_3_SIZE 2429
// Path length constraint values for following cert chain: none (root) -> none (intermediate_1) -> none (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_4 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_4.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_4_SIZE 2422
// Path length constraint values for following cert chain: none (root) -> 1    (intermediate_1) -> 0    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_5 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_5.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_5_SIZE 2425
// Path length constraint values for following cert chain: none (root) -> 1    (intermediate_1) -> none (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_6 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_6.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_6_SIZE 2422
// Path length constraint values for following cert chain: 0    (root) -> 1    (intermediate_1) -> 0    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_7 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_7.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_7_SIZE 2429
// Path length constraint values for following cert chain: 2    (root) -> 0    (intermediate_1) -> 0    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_8 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_8.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_8_SIZE 2431
// Path length constraint values for following cert chain: none (root) -> 0    (intermediate_1) -> 0    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_9 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_9.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_9_SIZE 2429


// sample certificate files with duplicate (multiple occurrence of the same) extensions; used for negative test case/ duplicate extension detection
// - these certificate chains consist of 4 certs
// Path length constraint values for following cert chain: 2    (root) -> 1    (intermediate_1) -> 0    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_0 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_0.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_0_SIZE 2430
// Path length constraint values for following cert chain: 3    (root) -> 2    (intermediate_1) -> 1    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_1 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_1.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_1_SIZE 2431
// Path length constraint values for following cert chain: 3    (root) -> 2    (intermediate_1) -> 4    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_2 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_2.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_2_SIZE 2430
// Path length constraint values for following cert chain: 3    (root) -> 4    (intermediate_1) -> 5    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_3 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_3.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_3_SIZE 2429
// Path length constraint values for following cert chain: none (root) -> none (intermediate_1) -> none (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_4 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_4.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_4_SIZE 2422
// Path length constraint values for following cert chain: none (root) -> 1    (intermediate_1) -> 0    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_5 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_5.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_5_SIZE 2425
// Path length constraint values for following cert chain: none (root) -> 1    (intermediate_1) -> none (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_6 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_6.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_6_SIZE 2422
// Path length constraint values for following cert chain: 0    (root) -> 1    (intermediate_1) -> 0    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_7 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_7.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_7_SIZE 2429
// Path length constraint values for following cert chain: 2    (root) -> 0    (intermediate_1) -> 0    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_8 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_8.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_8_SIZE 2431
// Path length constraint values for following cert chain: none (root) -> 0    (intermediate_1) -> 0    (intermediate_2)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_9 "testdata/x509_certificate/sample_cert_chain_pathlenconstraint_test_9.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_PATH_LENGTH_CONSTRAINT_TEST_9_SIZE 2429

// sample certificate chain (consist of 3 certificates)
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_CERTIFICATE_CHAIN_0 "testdata/x509_certificate/sample_test_certificate_chain_0.der"
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_CERTIFICATE_CHAIN_0_SIZE 1492
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_CERTIFICATE_CHAIN_0_LENGTH 3
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_CERTIFICATE_CHAIN_0_LEAF_CX {0x6c, 0x22, 0x41, 0xdf, 0xb7, 0xe4, 0xd6, 0x8d, 0x53, 0x72, 0x4e, 0x4a, 0x1b, 0x99, 0x82, 0xe6, 0x56, 0xd2, 0x2d, 0x97, 0x4b, 0x98, 0x40, 0xa9, 0x99, 0xd6, 0x0d, 0xd8, 0xe9, 0xa6, 0xfc, 0x74, 0xb9, 0xce, 0x89, 0x48, 0xa7, 0xb5, 0x09, 0xb6, 0x24, 0x49, 0xd6, 0x23, 0xb3, 0x5f, 0x3a, 0xf0}
#define X509_SAMPLE_CERT_CHAIN_DER_FILE_CERTIFICATE_CHAIN_0_LEAF_CY {0x99, 0xb0, 0xca, 0x63, 0x7d, 0x24, 0xfe, 0xe9, 0x12, 0x19, 0x0f, 0xc2, 0x73, 0x1c, 0xe3, 0x76, 0x91, 0xec, 0x57, 0x6c, 0xcd, 0x7b, 0xab, 0x32, 0xfd, 0x6d, 0x6e, 0x92, 0x7d, 0x37, 0x60, 0x01, 0xdb, 0x13, 0x92, 0x3b, 0x77, 0xf7, 0x12, 0x97, 0x1d, 0x5e, 0xe3, 0xb9, 0x15, 0x83, 0xaf, 0x89}

#endif

#endif /* INC_TESTDATA_FILES_X509_CERTIFICATE_SECP384_H */
