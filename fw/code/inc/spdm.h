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
 * @file spdm.h
 * @brief Define SPDM mctp structures, such as request and responce packets
 */

#ifndef EAGLESTREAM_INC_SPDM_H_
#define EAGLESTREAM_INC_SPDM_H_

#include "pfr_sys.h"

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

// SPDM field information
#define SPDM_VERSION 0x10
#define SPDM_EXCHANGE_SUCCESS 0

// SPDM 1.0 Response Code Field
#define RESPONSE_DIGESTS             0x01
#define RESPONSE_CERTIFICATE         0x02
#define RESPONSE_CHALLENGE_AUTH      0x03
#define RESPONSE_VERSION             0x04
#define RESPONSE_MEASUREMENTS        0x60
#define RESPONSE_CAPABILITIES        0x61
#define RESPONSE_ALGORITHMS          0x63
#define RESPONSE_ERROR               0x7f

// SPDM 1.0 Request Code Field
#define REQUEST_DIGESTS              0x81
#define REQUEST_CERTIFICATE          0x82
#define REQUEST_CHALLENGE            0x83
#define REQUEST_VERSION              0x84
#define REQUEST_MEASUREMENTS         0xe0
#define REQUEST_CAPABILITIES         0xe1
#define REQUEST_ALGORITHMS           0xe3
#define REQUEST_RESPOND_IF_READY     0xff

// Get capability/successful capability requester/responder flag
// Only supported field will be entertained
#define SUPPORTS_CASHE_CAP_RESPONSE_MSG_MASK             0b1
#define SUPPORTS_DIGEST_CERT_RESPONSE_MSG_MASK           0b10
#define SUPPORTS_CHALLENGE_AUTH_RESPONSE_MSG_MASK        0b100
#define SUPPORTS_MEAS_RESPONSE_WITHOUT_SIG_GEN_MSG_MASK  0b1000
#define SUPPORTS_MEAS_RESPONSE_WITH_SIG_GEN_MSG_MASK     0b10000

// Successful capability responder flag
#define RESPONDER_CACHE_NEGOTIATED_STATE_ACROSS_RESET_MASK    0b1

// Pre-define the flexible member size due to compiler not being able to allocate memory for unknown struct
// Processing of the member is done in functions in spdm_responder.h
/*
#ifdef USE_SYSTEM_MOCK

#define MAX_VERSION_NUM_ENTRY 3
#define MAX_EXT_ASYM 2
#define MAX_EXT_HASH 2
#define MAX_EXT_ASYM_SEL 1
#define MAX_EXT_HASH_SEL 1

#else

// Set max number of version entry to be 2
// However, CPLD only support 1 version entry which is 1.0
#define MAX_VERSION_NUM_ENTRY 3
// Set max asym and hash to be 3 which are 256,384 and 512 (ecdsa)
// However, CPLD only support ECDSA 384 for attestation
#define MAX_EXT_ASYM 3
#define MAX_EXT_HASH 3
// Only support 1 type of algorithm, ecdsa 384.
#define MAX_EXT_ASYM_SEL 1
#define MAX_EXT_HASH_SEL 1

#endif
*/

// Variety of buffer size to hold cached messages or received messages
#define LARGE_SPDM_MESSAGE_MAX_BUFFER_SIZE  0xc00
//#define MEDIUM_SPDM_MESSAGE_MAX_BUFFER_SIZE 0x200
#define SMALL_SPDM_MESSAGE_MAX_BUFFER_SIZE  0x100

// Set max number of version entry to be 2
// However, CPLD only support 1 version entry which is 1.0
#define MAX_VERSION_NUMBER_ENTRY 2

// Internal flag to track SPDM message states
#define SPDM_VERSION_FLAG        0b1
#define SPDM_CAPABILITIES_FLAG   0b10
#define SPDM_ALGORITHM_FLAG      0b100
#define SPDM_DIGEST_FLAG         0b1000
#define SPDM_CERTIFICATE_FLAG    0b10000
#define SPDM_CHALLENGE_FLAG      0b100000
#define SPDM_MEASUREMENT_FLAG    0b1000000
#define SPDM_ATTEST_FINISH_FLAG  0b10000000

// Some combination of the above
#define SPDM_CONNECTION_FLAG (SPDM_VERSION_FLAG | SPDM_CAPABILITIES_FLAG | SPDM_ALGORITHM_FLAG)
#define SPDM_AUTHENTICATION_FLAG (SPDM_DIGEST_FLAG | SPDM_CERTIFICATE_FLAG | SPDM_CHALLENGE_FLAG)

// Error State
#define SPDM_STATUS_SUCCESS    0
#define SPDM_STATUS_NO_CAPABILITIES    0x10

// Large buffer to hold cached messages
typedef struct
{
    alt_u32 buffer_size;
    alt_u32 buffer[LARGE_SPDM_MESSAGE_MAX_BUFFER_SIZE/4];
} LARGE_APPENDED_BUFFER;

// Small buffer for miscellaneous use
typedef struct
{
    alt_u32 buffer_size;
    alt_u32 buffer[SMALL_SPDM_MESSAGE_MAX_BUFFER_SIZE/4];
} SMALL_APPENDED_BUFFER;

// Groupings of M1/M2 and L1/L2 messages as well as received messages
// Message A,B and C are grouped under M1/M2
typedef struct
{
    LARGE_APPENDED_BUFFER m1_m2;
    LARGE_APPENDED_BUFFER l1_l2;
} SPDM_TRANSCRIPT;

// To save space, CPLD will use this structure to store both requester and responder
// capabilities as they are processed in mutual exclusive manner
typedef struct __attribute__((__packed__))
{
    alt_u8 ct_exponent;
    alt_u32 flags;
} SPDM_DEVICE_CAPABILITIES;

// Keeps track of the algorithm supported by responder
typedef struct __attribute__((__packed__))
{
    alt_u32 measurement_hash_algo;
    alt_u32 base_asym_algo;
    alt_u32 base_hash_algo;
} SPDM_DEVICE_ALGORITHM;

// SPDM response not ready data struct
// Used by responder (can be CPLD)
typedef struct __attribute__((__packed__))
{
    alt_u8 rdt_exponent;
    alt_u8 request_code;
    alt_u8 token;
    alt_u8 rdtm;
} SPDM_ERROR_RESPONSE_NOT_READY;

// CPLD's own response state
// For future implementation when CPLD is required to handle simultaneous transaction
typedef enum {
    spdm_normal_state,
    spdm_busy_state,
    spdm_not_ready_state,
    spdm_need_resync_state,
} SPDM_RESPONSE_STATE;

// Groupings of the capabilities and crypto algoritms supported by the responder
typedef struct __attribute__((__packed__))
{
    SPDM_DEVICE_CAPABILITIES capability;
    SPDM_DEVICE_ALGORITHM algorithm;
} SPDM_CONNECTION_INFO;

// CPLD only support ECDSA 384 which should result in lower cert size
// Allocate approximately 1800 bytes
#define MAX_CERT_CHAIN_SIZE 0x700
// SHA384
#define MAX_HASH_SIZE 48
// As defined per SPDM 1.0 specs
#define MAX_SLOT_COUNT 8
// Cert's chunk size
//#define CPLD_CERT_SIZE 0x200
#define CERT_REQUEST_SIZE 0xc8

// CPLD's local context for SPDM messages important payload
// Stores certificate chain, digest, public keys
// Also stores certificate parameters such as length for internal usage
typedef struct
{
    alt_u32 stored_cert_chain[MAX_CERT_CHAIN_SIZE/4];
    alt_u32 stored_digest_buf[(MAX_HASH_SIZE*MAX_SLOT_COUNT)/4];
    alt_u32 stored_chal_meas_signing_key[PFR_ROOT_KEY_DATA_SIZE_FOR_384/4];
    alt_u8 stored_measurement_summary_hash[SHA384_LENGTH];
    alt_u32 stored_cert_chain_size;
    alt_u16 cached_remainder_length;
    alt_u16 cpld_opaque_length;
    alt_u8 slot_num;
    alt_u8 cert_flag;
} SPDM_LOCAL_CONTEXT;


// SPDM context which groups all of the above data structures
// CPLD refers to these context along the way until the end of SPDM transaction
// Used in both requester and responder modes
typedef struct
{
    SPDM_TRANSCRIPT	transcript;
    SPDM_DEVICE_CAPABILITIES capability;
    SPDM_DEVICE_ALGORITHM algorithm;
    SPDM_CONNECTION_INFO connection_info;
    SPDM_ERROR_RESPONSE_NOT_READY error_data;
    SPDM_RESPONSE_STATE response_state;
    SPDM_LOCAL_CONTEXT local_context;
    //MCTP_CONTEXT mctp_context;
    alt_u32 error_state;
    alt_u32 spdm_msg_state;
    alt_u32 timeout;
    alt_u32 cached_spdm_request_size;
    alt_u32 previous_cached_size;
    alt_u16 uuid;
    alt_u8 cached_spdm_request[SMALL_SPDM_MESSAGE_MAX_BUFFER_SIZE];
    alt_u8 retry_times;
    alt_u8 token;
} SPDM_CONTEXT;
// Measurements
//
#define MEASUREMENT_SPECIFICATION 0

/****************
 *
 * SPDM 1.0
 *
 ***************/

/*!
 * SPDM Common Header
 *
 */
typedef struct __attribute__((__packed__))
{
    alt_u8 spdm_version;
    alt_u8 request_response_code;
    alt_u8 param_1;
    alt_u8 param_2;
} SPDM_HEADER;

/*!
 * Get version data structure (header)
 * Used to discover endpoint version protocol
 */
typedef struct __attribute__((__packed__))
{
    SPDM_HEADER header;
} GET_VERSION;

// Used in conjunction to SUCCESSFUL_VERSION message
typedef struct __attribute__((__packed__))
{
    alt_u8 alpha_update_ver_number;
    alt_u8 major_minor_version;
} VERSION_NUMBER;

/*!
 * Successful version data structure (header)
 * Used to respond regarding endpoint protocol version
 */
typedef struct __attribute__((__packed__))
{
    SPDM_HEADER header;
    alt_u8 reserved;
    alt_u8 version_num_entry_count;
    VERSION_NUMBER version_num_entry[MAX_VERSION_NUMBER_ENTRY];
} SUCCESSFUL_VERSION;

/*!
 * Get capabilities data structure (header)
 * Used to discover endpoint protocol capabilities
 */
typedef struct __attribute__((__packed__))
{
    SPDM_HEADER header;
} GET_CAPABILITIES;

#define SUCC_CAP_CT_EXPONENT 10

/*!
 * Successful capabilities data structure (header)
 * Used to respond regarding endpoint protocol capabilities
 */
typedef struct __attribute__((__packed__))
{
    SPDM_HEADER header;
    alt_u8 reserved_1;
    alt_u8 ct_exponent;
    alt_u8 reserved_2[2];
    alt_u32 flags;
} SUCCESSFUL_CAPABILITIES;

// SPDM ASYM and HASH bit mask
#define BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256  0b10000
#define BASE_ASYM_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384  0b10000000

#define BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P256  0b1
#define BASE_HASH_ALGO_TPM_ALG_ECDSA_ECC_NIST_P384  0b10

// Only one type of algorithm is supported
#define EXT_ASYM_COUNT  1
#define EXT_HASH_COUNT  1

#define MEAS_HASH_ALGO_TPM_ALG_SHA_256  0b10
#define MEAS_HASH_ALGO_TPM_ALG_SHA_384  0b100

#define EXT_ASYM_SEL_COUNT  1
#define EXT_HASH_SEL_COUNT  1

#define SPDM_ALGO_MEASUREMENT_SPECIFICATION_DMTF 0b1

// Extended algorithm as defined in SPDM specs
typedef struct __attribute__((__packed__))
{
    alt_u8 registry_id;
    alt_u8 reserved;
    alt_u8 algorithm_id[2];
} SPDM_EXTENDED_ALGORITHM;

/*!
 * Negotiate algorithms data structure (header)
 * Used to discover endpoint algorithm available
 */
typedef struct __attribute__((__packed__))
{
    SPDM_HEADER header;
    alt_u16 length;
    alt_u8 measurement_spec;
    alt_u8 reserved_1;
    alt_u32 base_asym_algo;
    alt_u32 base_hash_algo;
    alt_u8 reserved_2[12];
    alt_u8 ext_asym_count;
    alt_u8 ext_hash_count;
    alt_u8 reserved_3[2];
} NEGOTIATE_ALGORITHMS;

/*!
 * Successful algorithms data structure (header)
 * Used to respond endpoint algorithm availability
 * The responder shall respond showing no more than once chosen algorithm per method
 */
typedef struct __attribute__((__packed__))
{
    SPDM_HEADER header;
    alt_u16 length;
    alt_u8 measurement_spec;
    alt_u8 reserved_1;
    alt_u32 measurement_hash_algo;
    alt_u32 base_asym_sel;
    alt_u32 base_hash_sel;
    alt_u8 reserved_2[12];
    alt_u8 ext_asym_sel_count;
    alt_u8 ext_hash_sel_count;
    alt_u8 reserved_3[2]; // split
} SUCCESSFUL_ALGORITHMS;

/*!
 * Get digests data structure (header)
 * Used to retrieve certificate chain digests
 */
typedef struct
{
    SPDM_HEADER header;
} GET_DIGESTS;

/*!
 * Successful digests data structure (header)
 * Used to respond with certificate chain digests
 */
typedef struct __attribute__((__packed__))
{
    SPDM_HEADER header;
    //alt_u8 digest[MAX_HASH_SIZE*MAX_SLOT_COUNT];
} SUCCESSFUL_DIGESTS;

// Certficate chain header
typedef struct __attribute__((__packed__))
{
    alt_u16 length;
    alt_u16 reserved;
    alt_u32 root_hash[SHA384_LENGTH/4];
    // cert chain
} CERT_CHAIN_FORMAT;

/*!
 * Get certificate data structure (header)
 * Used to retrieve certificate chain
 */
typedef struct __attribute__((__packed__))
{
    SPDM_HEADER header;
    alt_u16 offset;
    alt_u16 length;
} GET_CERTIFICATE;

/*!
 * Successful certificate data structure (header)
 * Used to retrieve certificate chain
 */
typedef struct __attribute__((__packed__))
{
    SPDM_HEADER header;
    alt_u16 portion_length;
    alt_u16 remainder_length;
    //alt_u32 cert_chain[];
} SUCCESSFUL_CERTIFICATE;

// Fixed nonce size
#define SPDM_NONCE_SIZE 32

/*!
 * Challenge data structure (header)
 * Used to retrieve challenge chain
 */
typedef struct __attribute__((__packed__))
{
    SPDM_HEADER header;
    alt_u8 nonce[SPDM_NONCE_SIZE];
} CHALLENGE;

#define MAX_SPDM_OPAQUE_DATA_SIZE 1024
// Measurement hash type defined in spdm specs
#define MEAS_HASH_TYPE_ALL_MEAS_HASH 0xff
#define MEAS_HASH_TYPE_NO_MEAS_HASH 0x0

/*!
 * Successful challenge data structure (header)
 * Used to retrieve challenge chain
 */
typedef struct __attribute__((__packed__))
{
    SPDM_HEADER header;
    alt_u8 cert_chain_hash[48]; // Set to sha384 size
    alt_u8 nonce[SPDM_NONCE_SIZE];
    alt_u8 measurement_summary_hash[48];
    alt_u16 opaque_length;
    alt_u8 opaque_data[MAX_SPDM_OPAQUE_DATA_SIZE];
    alt_u8 signature[96];
} CHALLENGE_AUTH;

/*!
 * Successful challenge data structure (header)
 * Used by CPLD when it is a responder
 */
typedef struct __attribute__((__packed__))
{
    SPDM_HEADER header;
} CHALLENGE_AUTH_RESPONDER;

// Measurement SPDM parameters
#define SPDM_GET_MEASUREMENT_REQUEST_GENERATE_SIG 0b1
#define SPDM_MEASUREMENT_SPECIFICATION_DMTF 0b1
#define SPDM_DMTF_SPEC_MEAS_VAL_TYPE_MUTABLE_FIRMWARE 1
#define SPDM_GET_ALL_MEASUREMENT 0xff
#define SPDM_MAX_MEASUREMENT_BLOCK_COUNT 8

// Define size of measurements
typedef struct __attribute__((__packed__))
{
    alt_u8 index;
    alt_u8 measurement_spec;
    alt_u16 measurement_size;
    //alt_u8 measurement[measurement_size];
} SPDM_MEASUREMENT_BLOCK_HEADER;

// Define type of specific measurements
typedef struct __attribute__((__packed__))
{
    alt_u8 	dmtf_spec_meas_value_type;
    alt_u16 dmtf_spec_meas_value_size;
    // alt_u8 dmtf_spec_meas_value[dmtf_spec_meas_value_size];
} SPDM_MEASUREMENT_DMTF_HEADER;

// Groupings of the above
typedef struct __attribute__((__packed__))
{
    SPDM_MEASUREMENT_BLOCK_HEADER meas_block;
    SPDM_MEASUREMENT_DMTF_HEADER meas_dmtf;
    //alt_u8 hash[hash_size];
} SPDM_MEASUREMENT_BLOCK_DMTF_HEADER;

/*!
 * Get measurement data structure (header)
 * Used to retrieve measurements
 */
typedef struct __attribute__((__packed__))
{
    SPDM_HEADER header;
    alt_u8 nonce[32];
} GET_MEASUREMENT;

/*!
 * Measurement data structure (header)
 */
typedef struct __attribute__((__packed__))
{
    SPDM_HEADER header;
    alt_u8 num_of_blocks;
    alt_u8 meas_record_length[3];
    alt_u8 meas_record[(sizeof(SPDM_MEASUREMENT_BLOCK_DMTF_HEADER) + SHA512_LENGTH)*SPDM_MAX_MEASUREMENT_BLOCK_COUNT];
    alt_u8 nonce[SPDM_NONCE_SIZE];
    alt_u16 opaque_length;
    alt_u8 opaque_data[MAX_SPDM_OPAQUE_DATA_SIZE];
    alt_u8 signature[96];
} MEASUREMENT;

//#define SPDM_ERROR_TIMEOUT    1
// A series of errors
#define SPDM_ERROR_PROTOCOL   2

// SPDM error code
#define SPDM_ERROR_CODE_INVALID_REQUEST         0x01
#define SPDM_ERROR_CODE_BUSY                    0x03
#define SPDM_ERROR_CODE_RESPONSE_NOT_READY      0x42
#define SPDM_ERROR_CODE_REQUEST_RESYNCH         0x43

/*!
 * Error response code
 */
typedef struct __attribute__((__packed__))
{
    SPDM_HEADER header;
    //alt_u8 extended_err_data[];
} ERROR_RESPONSE_CODE;

#endif

#endif /* EAGLESTREAM_INC_SPDM_H_ */
