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
 * @file attestation.h
 * @brief Attestation management in the UFM
 */

#ifndef EAGLESTREAM_INC_ATTESTATION_H_
#define EAGLESTREAM_INC_ATTESTATION_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "crypto.h"
#include "ufm_rw_utils.h"
#include "cert_flow.h"

/*
 * @brief Extract TRNG data from entropy block as UDS
 *
 * @param uds : The destination address to save to
 *
 * @return 0 if failed; 1 otherwise
 */
static alt_u32 generate_and_store_uds(alt_u32* uds)
{
    // Extracting Unique Device Secret from entropy source
    if (!extract_entropy_data(uds, UDS_WORD_SIZE))
    {
        return 0;
    }
    return 1;
}

/*
 * @brief This is a collection of functions served to create the metadata CDI'x', private key 'x' and validate
 * private key 'x' where 'x' is variable(either layer 0 or 1)
 *
 * This function is used to create metadata of layer 0 and key components whenever platform is booted up.
 * It is also used to regenerate layer 1 metadata and key components should CPLD CFM1 image changes which could be due
 * to update/recovery/cancellation/altered
 *
 * @param cdix : Destination address to save cdix
 * @param priv_keyx : Destination address to save priv key x
 * @param data : Input data address from which metadata and key components are calculated
 * @param crypto_256_or_384 : -384/-256
 *
 * @return none
 */
static void create_cdix_and_priv_keyx(alt_u32* cdix, alt_u32* priv_keyx, alt_u32* data, CRYPTO_MODE crypto_256_or_384)
{
    // Size of hash
    alt_u32 sha_length = (crypto_256_or_384 == CRYPTO_384_MODE) ? SHA384_LENGTH : SHA256_LENGTH;

    // Generate and save the CDIx but never to the UFM
    calculate_and_save_sha(cdix, 0, (const alt_u32*)data, sha_length*2, crypto_256_or_384, DISENGAGE_DMA);

    // Create and save the private key but never to UFM (only on runtime)
    calculate_and_save_sha(priv_keyx, 0, (const alt_u32*)cdix, sha_length, crypto_256_or_384, DISENGAGE_DMA);

#ifdef USE_SYSTEM_MOCK
    // Replace the private key with value of all Fs' which will be definitely bigger than EC N
    // Only replace when instructed by unit test
    if (SYSTEM_MOCK::get()->mock_replace_key() > 0)
    {
        for (alt_u32 i = 0; i < sha_length/4; i++)
        {
            priv_keyx[i] = 0xffffffff;
        }
    }
#endif

    // Unless private key fulfills the condition of having value less than N, else renovate
    while (!check_hmac_validity(priv_keyx, (const alt_u32*)CRYPTO_EC_N_384, crypto_256_or_384))
    {
        reset_hw_watchdog();
        calculate_and_save_sha(priv_keyx, 0, (const alt_u32*)priv_keyx, sha_length, crypto_256_or_384, DISENGAGE_DMA);
    }
}

/*
 * @brief This function prepares all the necessary data for key generation flow.
 * 0)   Create CDI0 appended hash of CFM0 hash and UDS
 * 1)	Create private key 0 from CDI0 and validate the key.
 * 2)	Compute the public key 0 from the private key 0.
 * 3)	Hash CFM1 and combine with CDI0.
 * 4)	Create CDI1 from result in step 3.
 * 5)	Generate private key 1 from CDI1.
 * 6)	Create public key 1 from private key 1.
 * 7)   Create X509 certificate and store in UFM
 *
 * The function also compares pub key0/cfm1 hash/cdi1/private key 1/pub key 1 stored in the UFM.
 * If any of them has changed, this functions will erase those data and overwrite the data.
 * Else, it exit the function. This is done to preserve erase cycles of CPLD flash.
 *
 * Authenticate the generated certificate and regenerate them in case of disruption.
 *
 * @param cfm0_uds : The root data to calculate the keys
 * @param crypto_256_or_384 : Attestation using crypto -256/-384
 *
 * @return 0 if failed; 1 otherwise
 */
static alt_u32 prep_for_attestation(alt_u32* cfm0_uds, CRYPTO_MODE crypto_256_or_384)
{
    alt_u32 metadata_size = (crypto_256_or_384 == CRYPTO_384_MODE) ? UFM_CPLD_METADATA_384_SIZE : UFM_CPLD_METADATA_256_SIZE;
    alt_u32 pub_key_size = (crypto_256_or_384 == CRYPTO_384_MODE) ? (PFR_ROOT_KEY_DATA_SIZE_FOR_384 / 8) : (PFR_ROOT_KEY_DATA_SIZE_FOR_256 / 8);
    alt_u32 sha_word_length = (crypto_256_or_384 == CRYPTO_384_MODE) ? (SHA384_LENGTH / 4) : (SHA256_LENGTH / 4);

    // Temporarily store the metadata
    alt_u32 uds_cfm0_hash[SHA384_LENGTH / 2] = {0};
    alt_u32 cdi0[SHA384_LENGTH / 4] = {0};
    alt_u32 priv_key_0[SHA384_LENGTH / 4] = {0};
    alt_u32 cdi0_and_cfm1_hash[SHA384_LENGTH / 2] = {0};

    // Compute CFM0 hash and group them with UDS
    alt_u32* uds_cfm0_hash_ptr = uds_cfm0_hash;
    alt_u32_memcpy(uds_cfm0_hash_ptr, cfm0_uds, sha_word_length*4);

    uds_cfm0_hash_ptr = (alt_u32*)&uds_cfm0_hash[sha_word_length];
    // Compute CFM0 hash, use DMA to ufm to speed up the process
    calculate_and_save_sha(uds_cfm0_hash_ptr, UFM_CPLD_ROM_IMAGE_OFFSET, get_ufm_ptr_with_offset(UFM_CPLD_ROM_IMAGE_OFFSET),
                           UFM_CPLD_ROM_IMAGE_LENGTH, crypto_256_or_384, ENGAGE_DMA_UFM);

    // Divide the array
    alt_u32* cdi0_and_cfm1_hash_ptr = cdi0_and_cfm1_hash;
    alt_u32* cdio_location_ptr = cdi0_and_cfm1_hash_ptr + sha_word_length;

    // Store all the metadata in an array to simplify data comparison
    alt_u32 key_metadata[UFM_CPLD_METADATA_384_SIZE / 4] = {0};

    // Set location of each metadata elements into the array
    // This is done so that fw is able to compare and copy the data to the ufm in one go
    // Layer 0 metadata
    alt_u32* cx_0_ptr = key_metadata;
    alt_u32* cy_0_ptr = cx_0_ptr + pub_key_size;

    // Layer 1 metadata
    alt_u32* cfm1_hash_ptr = cy_0_ptr + pub_key_size;
    alt_u32* cdi1_ptr = cfm1_hash_ptr + sha_word_length;
    alt_u32* priv_key_1_ptr = cdi1_ptr + sha_word_length;
    alt_u32* cx_1_ptr = priv_key_1_ptr + sha_word_length;
    alt_u32* cy_1_ptr = cx_1_ptr + pub_key_size;

    // Create CDI0 from UDS and cfm0 hash, create private key 0 from CDI0 and validate private key 0
    create_cdix_and_priv_keyx((alt_u32*)cdi0, (alt_u32*)priv_key_0, (alt_u32*)uds_cfm0_hash, crypto_256_or_384);

    // Generate public key 0 cx and cy
    if (!generate_pubkey(cx_0_ptr, cy_0_ptr, (const alt_u32*)priv_key_0, crypto_256_or_384))
    {
        return 0;
    }

    // Compute the hash of CFM1 and store in array which will be grouped with CDI0 for computing CDI1.
    calculate_and_save_sha(cdi0_and_cfm1_hash_ptr, UFM_CPLD_ACTIVE_IMAGE_OFFSET, get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET),
                           UFM_CPLD_ACTIVE_IMAGE_LENGTH, crypto_256_or_384, ENGAGE_DMA_UFM);

    // Also save the cfm1 hash which will be stored in the UFM to a temporary array
    alt_u32_memcpy(cfm1_hash_ptr, (const alt_u32*)cdi0_and_cfm1_hash_ptr, (sha_word_length * 4));

    // Group CFM1 hash and CDIO together
    alt_u32_memcpy(cdio_location_ptr, (const alt_u32*)cdi0, (sha_word_length * 4));

    // Create CDI1 from the grouped cfm1 hash and CDI0, create private key 1 from CDI1 and validate private key 1
    create_cdix_and_priv_keyx(cdi1_ptr, priv_key_1_ptr, cdi0_and_cfm1_hash_ptr, crypto_256_or_384);

    // Generate public key 1 and store in temporary array
    if (!generate_pubkey(cx_1_ptr, cy_1_ptr, (const alt_u32*)priv_key_1_ptr, crypto_256_or_384))
    {
        return 0;
    }

    // X509v3 Certificate creation starts here
    // Signing/Hash algorithm: ECDSA and SHA
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    // Create CPLD's X509 Certificate
    // Get location to store the cert
    alt_u32* cert_ufm_ptr = get_ufm_cpld_cert_ptr();

    alt_u32 cert_chain_buffer[MAX_CERT_CHAIN_SIZE/4] = {0};
    alt_u32 cert_chain_key[PFR_ROOT_KEY_DATA_SIZE_FOR_384 / 4] = {0};
    alt_u32* cert_key = (alt_u32*) cert_chain_key;
    // Create certificate
    alt_u32 cert_chain_size = 0;
    // Get cert size
    alt_u32 cert_chain_root_cert_size = 0;

    // Build and store first 4 bytes of cert header into UFM
    alt_u32 cert_chain_header_first_word = 0;
    alt_u16* cert_chain_header_first_word_u16 = (alt_u16*) &cert_chain_header_first_word;
    // Store cert chain into UFM
#endif

    // By now, we have stored in the array, all the metadata IN SIMILAR ORDER to the data in UFM.
    // These data will be collectively compared with the data in the UFM.
    // If Pub key 0/CFM1 hash/CDI1/Priv key 1/Pub Key 1 is tampered, erase the page content and copy all the newly
    // computed metadata to the UFM. This could also be due to an update/recovery/power cycle to cpld.
    // If these data stored in UFM is exactly the same to the newly computed metadata, FW will not erase the page.
    if (!(is_data_matching_stored_data((alt_u32*)cx_0_ptr, get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_0), metadata_size)))
    {
        // Erase the keys and cert since keys changed
        ufm_erase_page(UFM_CPLD_PUBLIC_KEY_0);
        alt_u32_memcpy_s(get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_0), U_UFM_DATA_BYTES_PER_PAGE, (const alt_u32*)key_metadata, metadata_size);

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
        // Create certificate
        cert_chain_size = pfr_generate_certificate_chain((alt_u8*) cert_chain_buffer, (alt_u32*) priv_key_0, get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_0), get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_1));
        if (cert_chain_size == 0)
        {
            return 0;
        }
        // Get cert size
        cert_chain_root_cert_size = pfr_get_certificate_size((alt_u8*) cert_chain_buffer);
        // Cert chain header size
        cert_chain_header_first_word_u16[0] = (alt_u16) (4 + (sha_word_length*4) + cert_chain_size);
        // Start with cert chain header length
        cert_ufm_ptr[0] = cert_chain_header_first_word;
        // Calculate and store root cert hash into UFM
        calculate_and_save_sha(&cert_ufm_ptr[1], 0, cert_chain_buffer,
        		cert_chain_root_cert_size, crypto_256_or_384, DISENGAGE_DMA);

        // Update the certificate with newly generated keys
        alt_u32_memcpy(&cert_ufm_ptr[1 + sha_word_length], cert_chain_buffer, round_up_to_multiple_of_4(cert_chain_size));
#endif
    }

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)
    // Verify the CPLD's own certificate.
    // Certificate can be corrupted from disruption to power supply.
    // Ensure CPLD's certificate is always valid prior to attestation activities
    alt_u8* cpld_cert = (alt_u8*)(get_ufm_cpld_cert_ptr() + 1 + sha_word_length);
    alt_u16* cert_header_length = (alt_u16*)get_ufm_cpld_cert_ptr();
    cert_chain_size = (alt_u32)(*cert_header_length) - 4 - (sha_word_length*4);

    if (!pfr_verify_der_cert_chain(cpld_cert, cert_chain_size, cert_key, cert_key + sha_word_length))
    {
        // Erase the keys and cert since they are both stored in a single page
        ufm_erase_page(UFM_CPLD_PUBLIC_KEY_0);
        // Re-copied keys
        alt_u32_memcpy_s(get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_0), U_UFM_DATA_BYTES_PER_PAGE, (const alt_u32*)key_metadata, metadata_size);

        // Regenerated certificate
        cert_chain_size = pfr_generate_certificate_chain((alt_u8*) cert_chain_buffer, (alt_u32*) priv_key_0, get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_0), get_ufm_ptr_with_offset(UFM_CPLD_PUBLIC_KEY_1));
        if (cert_chain_size == 0)
        {
            return 0;
        }
        // Get cert size
        cert_chain_root_cert_size = pfr_get_certificate_size((alt_u8*) cert_chain_buffer);
        // Cert chain header size
        cert_chain_header_first_word_u16[0] = (alt_u16) (4 + (sha_word_length*4) + cert_chain_size);
        // Start with cert chain header length
        cert_ufm_ptr[0] = cert_chain_header_first_word;
        // Calculate and store root cert hash into UFM
        calculate_and_save_sha(&cert_ufm_ptr[1], 0, cert_chain_buffer,
        		cert_chain_root_cert_size, crypto_256_or_384, DISENGAGE_DMA);

        // Save the newly generated certificate over to UFM
        alt_u32_memcpy(&cert_ufm_ptr[1 + sha_word_length], cert_chain_buffer, round_up_to_multiple_of_4(cert_chain_size));
    }
#endif
    return 1;
}

#ifdef ENABLE_ATTESTATION_WITH_ECDSA_384

/*
 * @brief This function prepares all the necessary data for key generation flow before
 * proceeding with PFR operations. This includes generating UDS, CFM measurements, keys and cert.
 *
 * @return none
 */

static void prep_boot_flow_pre_requisite_384()
{
    // Direct to the location to store the UDS..
    alt_u32* cfm0_uds = get_ufm_ptr_with_offset(CFM0_UNIQUE_DEVICE_SECRET);
    // Check for bread crumbs in cfm0
    alt_u32* cfm0_breadcrumb = get_ufm_ptr_with_offset(CFM0_BREAD_CRUMB);

    if (*cfm0_breadcrumb != 0)
    {
        // This function should only be called once upon first power up
        // Store the UDS into CFM0
        if (generate_and_store_uds(cfm0_uds))
        {
            // Leave bread crumbs in cfm0
            // Cannot rely on UDS value as they are random
            *cfm0_breadcrumb = 0;
        }
    }

    // Only prepare keys and certs if UDS created pass certified entropy circuit test
    if (*cfm0_breadcrumb == 0)
    {
        // Prepare the keys/certs and perform necessary function
        prep_for_attestation(cfm0_uds, CRYPTO_384_MODE);
    }

    alt_u32 cfm1_hash[SHA384_LENGTH / 4] = {0};

    // Compute the hash of CFM1 and store in array which will be grouped with CDI0 for computing CDI1.
    calculate_and_save_sha(cfm1_hash, UFM_CPLD_ACTIVE_IMAGE_OFFSET, get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET),
                           UFM_CPLD_ACTIVE_IMAGE_LENGTH, CRYPTO_384_MODE, ENGAGE_DMA_UFM);

    if ((*get_ufm_cpld_cert_ptr() == 0xFFFFFFFF) &&
        (!is_data_matching_stored_data((alt_u32*)cfm1_hash, get_ufm_ptr_with_offset(UFM_CPLD_CFM1_STORED_HASH), SHA384_LENGTH)))
    {
        // Erase the page
        ufm_erase_page(UFM_CPLD_PUBLIC_KEY_0);
        alt_u32_memcpy_s(get_ufm_ptr_with_offset(UFM_CPLD_CFM1_STORED_HASH), U_UFM_DATA_BYTES_PER_PAGE, (const alt_u32*)cfm1_hash, SHA384_LENGTH);
    }
}
#endif

#ifdef ENABLE_ATTESTATION_WITH_ECDSA_256
static void prep_boot_flow_pre_requisite_256()
{
    // Direct to the location to store the UDS..
    alt_u32* cfm0_uds = get_ufm_ptr_with_offset(CFM0_UNIQUE_DEVICE_SECRET);
    // Check for bread crumbs in cfm0
    alt_u32* cfm0_breadcrumb = get_ufm_ptr_with_offset(CFM0_BREAD_CRUMB);

    if (*cfm0_breadcrumb != 0)
    {
        // This function should only be called once upon first power up
        // Store the UDS into CFM0
        if (generate_and_store_uds(cfm0_uds))
        {
            // Leave bread crumbs in cfm0
            // Cannot rely on UDS value as they are random
            *cfm0_breadcrumb = 0;
        }
    }

    // Only prepare keys and certs if UDS created pass certified entropy circuit test
    if (*cfm0_breadcrumb == 0)
    {
        // Prepare the keys/certs and perform necessary function
        prep_for_attestation(cfm0_uds, CRYPTO_256_MODE);
    }

    alt_u32 cfm1_hash[SHA256_LENGTH / 4] = {0};

    // Compute the hash of CFM1 and store in array which will be grouped with CDI0 for computing CDI1.
    calculate_and_save_sha(cfm1_hash, UFM_CPLD_ACTIVE_IMAGE_OFFSET, get_ufm_ptr_with_offset(UFM_CPLD_ACTIVE_IMAGE_OFFSET),
                           UFM_CPLD_ACTIVE_IMAGE_LENGTH, CRYPTO_256_MODE, ENGAGE_DMA_UFM);

    if ((*get_ufm_cpld_cert_ptr() == 0xFFFFFFFF) &&
        (!is_data_matching_stored_data((alt_u32*)cfm1_hash, get_ufm_ptr_with_offset(UFM_CPLD_CFM1_STORED_HASH), SHA256_LENGTH)))
    {
        // Erase the page
        ufm_erase_page(UFM_CPLD_PUBLIC_KEY_0);
        alt_u32_memcpy_s(get_ufm_ptr_with_offset(UFM_CPLD_CFM1_STORED_HASH), U_UFM_DATA_BYTES_PER_PAGE, (const alt_u32*)cfm1_hash, SHA256_LENGTH);
    }
}
#endif

#endif /* EAGLESTREAM_INC_ATTESTATION_H_ */
