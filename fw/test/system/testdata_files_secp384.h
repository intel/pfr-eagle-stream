#ifndef INC_TESTDATA_FILES_SECP384_H
#define INC_TESTDATA_FILES_SECP384_H

/************************************************
 *
 * This header describes all the unittests binaries files under fw/test/testdata.
 * All BMC and PCH flash images are generated with gen_pfr_flash_images/... scripts.
 *
 ************************************************/
/*
 * UFM data for provisioning
 */
#define UFM_PFR_DATA_EXAMPLE_KEY_FILE "testdata/ufm_data_example_key_with_384bit_rk_hash.hex"
#define UFM_PFR_DATA_EXAMPLE_KEY_FILE_SIZE 316

/*
 * UFM SPI GEO data for provisioning
 */
#define UFM_PFR_DATA_SPI_GEO_EXAMPLE_KEY_FILE "testdata/ufm_data_spi_geo_provisioning_384.hex"
#define UFM_PFR_DATA_SPI_GEO_EXAMPLE_KEY_FILE_SIZE 324

/*
 * PFR Images for SPI flash
 */
#define FULL_PFR_IMAGE_BMC_FILE "testdata/full_pfr_image_bmc_secp384.bin"
#define FULL_PFR_IMAGE_BMC_FILE_SIZE 134217728

// Steps
// sha384sum testdata/full_pfr_image_bmc.bin
// echo "<insert hash here>" | xxd -r -p | xxd -i
#define FULL_PFR_IMAGE_BMC_FILE_HASH_SIZE 48
#define FULL_PFR_IMAGE_BMC_FILE_HASH {\
		0x39, 0x32, 0xbf, 0x9a, 0xc8, 0x83, 0x82, 0x53, 0x6b, 0x57, 0xa9, 0xfb,\
		0x55, 0x23, 0x95, 0x8e, 0xee, 0x93, 0xe6, 0xbd, 0x9c, 0xea, 0x7a, 0x7a,\
		0x26, 0x20, 0x4f, 0xad, 0x70, 0xb8, 0x25, 0x39, 0xa8, 0x0f, 0xa9, 0x60,\
		0x9c, 0x1c, 0x45, 0x5c, 0x22, 0x03, 0xcd, 0x0c, 0xc2, 0x9b, 0x1f, 0x10\
}

#define FULL_PFR_IMAGE_PCH_FILE "testdata/full_pfr_image_pch_secp384.bin"
#define FULL_PFR_IMAGE_PCH_FILE_SIZE 67108864

#define FULL_PFR_IMAGE_PCH_FILE_HASH_SIZE 48
#define FULL_PFR_IMAGE_PCH_FILE_HASH {\
		0x30, 0x55, 0x55, 0x38, 0xce, 0x53, 0x6c, 0x8b, 0x0b, 0x5b, 0x34, 0x7f,\
		0xda, 0xd6, 0x67, 0xe1, 0x8a, 0x18, 0x4c, 0x9d, 0xcd, 0x76, 0xda, 0xaf,\
		0x30, 0xe2, 0x61, 0x69, 0x3c, 0x52, 0xef, 0xc1, 0xdc, 0xf7, 0x54, 0xc4,\
		0x90, 0xd4, 0x96, 0x75, 0xe8, 0xcd, 0xec, 0xca, 0xdb, 0xb3, 0xc9, 0x8d\
}

#define GENERATED_FULL_PFR_IMAGE_BMC_WITH_STAGING_FILE \
		"testdata/generated_full_pfr_image_bmc_with_staging.bin"
#define GENERATED_FULL_PFR_IMAGE_BMC_WITH_STAGING_FILE_SIZE 134217728

/*
 * Signed firmware update capsule
 */
#define SIGNED_CAPSULE_BMC_FILE "testdata/signed_capsule_bmc_secp384.bin"
#define SIGNED_CAPSULE_BMC_FILE_SIZE 21965440

#define SIGNED_CAPSULE_PCH_FILE "testdata/signed_capsule_pch_secp384.bin"
#define SIGNED_CAPSULE_PCH_FILE_SIZE 12565120

/*
 * Signed PFM
 */

#define SIGNED_PFM_BMC_FILE "testdata/signed_pfm_bmc_secp384.bin"
#define SIGNED_PFM_BMC_FILE_SIZE 1536

#define SIGNED_PFM_PCH_FILE "testdata/signed_pfm_pch_secp384.bin"
#define SIGNED_PFM_PCH_FILE_SIZE 1536

/*
 * CPLD image & capsule
 */
#define CFM1_ACTIVE_IMAGE_FILE "testdata/cpld_cfm1_active_image.bin"
#define CFM1_ACTIVE_IMAGE_FILE_SIZE 688128

#define CFM0_RECOVERY_IMAGE_FILE "testdata/cpld_cfm0_recovery_image.bin"
#define CFM0_RECOVERY_IMAGE_FILE_SIZE 688128

#define SIGNED_CAPSULE_CPLD_FILE "testdata/signed_capsule_cpld_secp384.bin"
#define SIGNED_CAPSULE_CPLD_FILE_SIZE 689280

// Refer to impl/*/Makefile on how this hash can be generated
#define CFM1_ACTIVE_IMAGE_FILE_HASH_SIZE 48
#define CFM1_ACTIVE_IMAGE_FILE_HASH {\
		0x28, 0x73, 0x6a, 0x7b, 0xb2, 0x33, 0x24, 0xdb, 0x62, 0xb8, 0x46, 0x3c,\
		0x78, 0x5f, 0x40, 0xc9, 0x42, 0xf8, 0xc5, 0x56, 0x59, 0x11, 0x8d, 0xfd,\
		0x62, 0x0f, 0x8a, 0x28, 0xfe, 0x54, 0xb6, 0x4b, 0xf8, 0x48, 0xc6, 0x5d,\
		0xa2, 0x5b, 0xc1, 0xef, 0xb2, 0x55, 0xe4, 0x83, 0xcb, 0x01, 0x55, 0xee\
}

/*
 * Signed random binary
 */
#define SIGNED_BINARY_BLOCKSIGN_FILE "testdata/signed_binary_blocksign_secp384.bin"
#define SIGNED_BINARY_BLOCKSIGN_FILE_SIZE 1152

/*
 * Key cancellation certificate
 */
#define KEY_CAN_CERT_FILE_SIZE 1152

#define KEY_CAN_CERT_PCH_PFM_KEY2 "testdata/key_cancellation_certs/signed_pch_pfm_key2_can_cert_secp384.bin"
#define KEY_CAN_CERT_PCH_PFM_KEY255 "testdata/key_cancellation_certs/signed_pch_pfm_key255_can_cert_secp384.bin"
#define KEY_CAN_CERT_PCH_PFM_KEY0 "testdata/key_cancellation_certs/signed_pch_pfm_key_can_cert_0_384.bin"

#define KEY_CAN_CERT_MODIFIED_RESERVED_AREA "testdata/key_cancellation_certs/signed_key_can_cert_modified_reserved_area_secp384.bin"
#define KEY_CAN_CERT_BAD_PCH_LEGNTH "testdata/key_cancellation_certs/signed_key_can_cert_bad_pc_length_secp384.bin"
#define KEY_CAN_CERT_BAD_PCH_LEGNTH_FILE_SIZE 1280

#define KEY_CAN_CERT_PCH_CAPSULE_KEY2 "testdata/key_cancellation_certs/signed_pch_capsule_key2_can_cert_secp384.bin"
#define KEY_CAN_CERT_PCH_CAPSULE_KEY2_SECP256 "testdata/key_cancellation_certs/signed_pch_capsule_key2_can_cert.bin"
#define KEY_CAN_CERT_PCH_CAPSULE_KEY10 "testdata/key_cancellation_certs/signed_pch_capsule_key10_can_cert_secp384.bin"

#define KEY_CAN_CERT_CPLD_CAPSULE_KEY2 "testdata/key_cancellation_certs/signed_cpld_capsule_key2_can_cert_secp384.bin"
#define KEY_CAN_CERT_CPLD_CAPSULE_KEY10 "testdata/key_cancellation_certs/signed_cpld_capsule_key10_can_cert_secp384.bin"

#define KEY_CAN_CERT_BMC_CAPSULE_KEY1 "testdata/key_cancellation_certs/signed_bmc_capsule_key1_can_cert_secp384.bin"
#define KEY_CAN_CERT_BMC_CAPSULE_KEY10 "testdata/key_cancellation_certs/signed_bmc_capsule_key10_can_cert_secp384.bin"

#define KEY_CAN_CERT_BMC_PFM_KEY2 "testdata/key_cancellation_certs/signed_bmc_pfm_key2_can_cert_secp384.bin"
#define KEY_CAN_CERT_BMC_PFM_KEY10 "testdata/key_cancellation_certs/signed_bmc_pfm_key10_can_cert_secp384.bin"
#define KEY_CAN_CERT_BMC_PFM_KEY1 "testdata/key_cancellation_certs/signed_bmc_pfm_key_can_cert_1_384.bin"

/*
 * BMC firmware update flow
 * Generated file with Major version 0x0 and Minor version 0xE
 */
#define FULL_PFR_IMAGE_BMC_V14_FILE "testdata/bmc_firmware_update/full_pfr_image_bmc_secp384.bin"
#define FULL_PFR_IMAGE_BMC_V14_FILE_SIZE 134217728

#define SIGNED_CAPSULE_BMC_V14_FILE "testdata/bmc_firmware_update/signed_capsule_bmc_secp384.bin"
#define SIGNED_CAPSULE_BMC_V14_FILE_SIZE 21965440

/*
 * PCH firmware update flow
 * Generated file with Major version 0x3 and Minor version 0xC
 */
#define FULL_PFR_IMAGE_PCH_V03P12_FILE "testdata/pch_firmware_update/full_pfr_image_pch_secp384.bin"
#define FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE 67108864

#define SIGNED_CAPSULE_PCH_V03P12_FILE "testdata/pch_firmware_update/signed_capsule_pch_secp384.bin"
#define SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE 12565120

/*
 * CPLD firmware update flow
 */
#define SIGNED_CAPSULE_CPLD_B490_3_FILE "testdata/cpld_update/signed_capsule_cpld_b490_3_secp384.bin"
#define SIGNED_CAPSULE_CPLD_B490_3_FILE_SIZE SIGNED_CAPSULE_CPLD_FILE_SIZE

/*
 * Anti Rollback Tests
 */
#define SIGNED_CAPSULE_PCH_WITH_CSK_ID10_FILE "testdata/anti_rollback_with_csk_id/signed_capsule_pch_with_csk_id10_secp384.bin"
#define SIGNED_CAPSULE_PCH_WITH_CSK_ID10_FILE_SIZE 12565120

/*
 * PCH firmware update capsule with different SVNs
 */
#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN255_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_pfm_with_svn_0xFF_secp384.bin"
#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN255_FILE_SIZE 12565120

#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN65_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_pfm_with_svn65_secp384.bin"
#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN65_FILE_SIZE 12565120

#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN64_FILE "testdata/anti_rollback_with_svn/signed_capsule_pch_pfm_with_svn64_secp384.bin"
#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN64_FILE_SIZE 12565120

#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN9_FILE "testdata/anti_rollback_with_svn/signed_capsule_pch_pfm_with_svn9_secp384.bin"
#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN9_FILE_SIZE 12565120

#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN7_FILE "testdata/anti_rollback_with_svn/signed_capsule_pch_pfm_with_svn7_secp384.bin"
#define SIGNED_CAPSULE_PCH_PFM_WITH_SVN7_FILE_SIZE 12565120

/*
 * PCH firmware update capsules with invalid configurations
 */
#define SIGNED_CAPSULE_PCH_WITH_BAD_BITMAP_NBIT_IN_PBC_HEADER_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_with_bad_bitmap_nbit_in_pbc_header_secp384.bin"
#define SIGNED_CAPSULE_PCH_WITH_BAD_BITMAP_NBIT_IN_PBC_HEADER_FILE_SIZE 12565120

#define SIGNED_CAPSULE_PCH_WITH_BAD_PAGE_SIZE_IN_PBC_HEADER_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_with_bad_page_size_in_pbc_header_secp384.bin"
#define SIGNED_CAPSULE_PCH_WITH_BAD_PAGE_SIZE_IN_PBC_HEADER_FILE_SIZE 12565120

#define SIGNED_CAPSULE_PCH_WITH_BAD_PATT_IN_PBC_HEADER_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_with_bad_pattern_in_pbc_header_secp384.bin"
#define SIGNED_CAPSULE_PCH_WITH_BAD_PATT_IN_PBC_HEADER_FILE_SIZE 12565120

#define SIGNED_CAPSULE_PCH_WITH_BAD_TAG_IN_PBC_HEADER_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_with_bad_tag_in_pbc_header_secp384.bin"
#define SIGNED_CAPSULE_PCH_WITH_BAD_TAG_IN_PBC_HEADER_FILE_SIZE 12565120

#define SIGNED_CAPSULE_PCH_WITH_BAD_VERSION_IN_PBC_HEADER_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_with_bad_version_in_pbc_header_secp384.bin"
#define SIGNED_CAPSULE_PCH_WITH_BAD_VERSION_IN_PBC_HEADER_FILE_SIZE 12565120

#define SIGNED_CAPSULE_PCH_WITH_PC_TYPE_10_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_with_pc_type_10_secp384.bin"
#define SIGNED_CAPSULE_PCH_WITH_PC_TYPE_10_FILE_SIZE 12565120

#define SIGNED_CAPSULE_PCH_WITH_WRONG_CSK_PERMISSION_FILE "testdata/bad_firmware_capsule/signed_capsule_pch_with_wrong_csk_permission_secp38.bin"
#define SIGNED_CAPSULE_PCH_WITH_WRONG_CSK_PERMISSION_FILE_SIZE 12565120

/*
 * BMC firmware update capsule with different SVNs
 */
#define SIGNED_CAPSULE_BMC_WITH_SVN2_FILE "testdata/anti_rollback_with_svn/signed_capsule_bmc_with_svn2_secp384.bin"
#define SIGNED_CAPSULE_BMC_WITH_SVN2_FILE_SIZE 21965440

/*
 * CPLD update capsule with different SVNs
 */
#define SIGNED_CAPSULE_CPLD_WITH_SVN1_FILE "testdata/anti_rollback_with_svn/signed_capsule_cpld_with_svn1_secp384.bin"
#define SIGNED_CAPSULE_CPLD_WITH_SVN1_FILE_SIZE SIGNED_CAPSULE_CPLD_FILE_SIZE

#define SIGNED_CAPSULE_CPLD_WITH_SVN64_FILE "testdata/anti_rollback_with_svn/signed_capsule_cpld_with_svn64_secp384.bin"
#define SIGNED_CAPSULE_CPLD_WITH_SVN64_FILE_SIZE SIGNED_CAPSULE_CPLD_FILE_SIZE

#define SIGNED_CAPSULE_CPLD_WITH_SVN65_FILE "testdata/anti_rollback_with_svn/signed_capsule_cpld_with_svn65_secp384.bin"
#define SIGNED_CAPSULE_CPLD_WITH_SVN65_FILE_SIZE SIGNED_CAPSULE_CPLD_FILE_SIZE

/*
 * PCH firmware recovery
 * This generated image is based on the IFWI image taken from HSD case 1507855229.
 * In this image, the BIOS regions (0x37F0000 - 0x3800000; 0x3850000 - 0x3880000; 0x3880000 - 0x3900000)
 * are configured to only be recovered on second recovery.
 */
#define FULL_PFR_IMAGE_PCH_ONLY_RECOVER_BIOS_REGIONS_ON_2ND_LEVEL_FILE "testdata/pch_firmware_recovery/full_pfr_image_pch_only_recover_bios_regions_on_2nd_level_secp384.bin"
#define FULL_PFR_IMAGE_PCH_ONLY_RECOVER_BIOS_REGIONS_ON_2ND_LEVEL_FILE_SIZE 67108864

/*
 * Decommission capsule
 */
#define SIGNED_DECOMMISSION_CAPSULE_FILE "testdata/signed_decommission_capsule_secp384.bin"
#define SIGNED_DECOMMISSION_CAPSULE_FILE_SIZE 1152

#define SIGNED_DECOMMISSION_CAPSULE_WITH_CSK_ID3_FILE "testdata/anti_rollback_with_csk_id/signed_decomm_capsule_with_csk_id3_secp384.bin"
#define SIGNED_DECOMMISSION_CAPSULE_WITH_CSK_ID3_FILE_SIZE SIGNED_DECOMMISSION_CAPSULE_FILE_SIZE

#define SIGNED_DECOMMISSION_CAPSULE_WITH_INVALID_PC_FILE "testdata/bad_firmware_capsule/signed_decomm_capsule_with_invalid_pc_secp384.bin"
#define SIGNED_DECOMMISSION_CAPSULE_WITH_INVALID_PC_FILE_SIZE 1280

/*
 * Pfr Debug Flash Images
 */
// Example provisioning data space with different PFM offsets
// Here the active PFM is 0x29c0000 for AMI BMC images
#define UFM_PFR_DATA_AMI_BMC_EXAMPLE_KEY_FILE "testdata/ufm_pfm_act_2c90000_no_attestation_secp384r1.hex"
#define UFM_PFR_DATA_AMI_BMC_EXAMPLE_KEY_FILE_SIZE 316

#define PFR_DEBUG_FULL_BMC_IMAGE "testdata/pfr_debug_full_image/full_pfr_image_bmc_secp384.bin"
#define PFR_DEBUG_FULL_PCH_IMAGE "testdata/pfr_debug_full_image/full_pfr_image_pch_secp384.bin"

#define PFR_DEBUG_FULL_BMC_WITH_AFM_IMAGE "testdata/pfr_debug_full_image/full_pfr_image_bmc_attestation_secp384_v2p2_svn2_1afm_v1p1_svn4_uuid9.bin"
//#define PFR_DEBUG_FULL_BMC_WITH_AFM_IMAGE "testdata/pfr_debug_full_image/OBMC-egs-0.68-0-g659438-4d99a37-pfr-full_afm.bin"

#define PFR_DEBUG_FULL_BMC_WITH_AFM_IMAGE_2 "testdata/pfr_debug_full_image/OBMC-egs-0.69-0-gcd0201-9dccc52-pfr-full_afm.bin"
#define PFR_DEBUG_FULL_BMC_WITH_AFM_IMAGE_SIZE_2 134217728

#define PFR_DEBUG_BMC_UPDATE_CAPSULE "testdata/pfr_debug_full_image/OBMC-egs-0.69-0-gcd0201-9dccc52-pfr-oob.bin"
#define PFR_DEBUG_BMC_UPDATE_CAPSULE_SIZE 21965440

// Example AFM capsule
#define PFR_DEBUG_AFM_ACTIVE_UPDATE_CAPSULE "testdata/pfr_debug_full_image/OBMC-egs-0.69-0-gcd0201-9dccc52-pfr-afm_staging_capsule.bin"
#define PFR_DEBUG_AFM_ACTIVE_UPDATE_CAPSULE_SIZE 131072

// Example PCH image with seamless
#define PFR_DEBUG_SEAMLESS_FULL_IMAGE "testdata/pfr_debug_full_image/full_pfr_image_pch_seamless_secp384.bin"
#define PFR_DEBUG_SEAMLESS_UPDATE_CAPSULE "testdata/pfr_debug_full_image/signed_capsule_pch_seamless_bios_secp384.bin"


#endif /* INC_TESTDATA_FILES_SECP384_H */
