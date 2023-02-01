#ifndef INC_TESTDATA_FILES_SEAMLESS_SECP384_H
#define INC_TESTDATA_FILES_SEAMLESS_SECP384_H

/************************************************
 *
 * This header describes all the unittests binaries files used to test Seamless Reference Design
 * under fw/test/testdata. All images are generated with gen_pfr_flash_images/... scripts.
 *
 * Many macros from testdata_files_secp384.h are re-defined here.
 ************************************************/
#undef FULL_PFR_IMAGE_PCH_FILE
#undef FULL_PFR_IMAGE_PCH_FILE_SIZE
#undef FULL_PFR_IMAGE_PCH_FILE_HASH
#undef FULL_PFR_IMAGE_PCH_FILE_HASH_SIZE
#undef SIGNED_CAPSULE_PCH_FILE
#undef SIGNED_CAPSULE_PCH_FILE_SIZE
#undef SIGNED_PFM_PCH_FILE
#undef SIGNED_PFM_PCH_FILE_SIZE


/*
 * PFR Images for SPI flash
 */
#define FULL_PFR_IMAGE_PCH_FILE "testdata/full_pfr_image_pch_seamless_secp384.bin"
#define FULL_PFR_IMAGE_PCH_FILE_SIZE 67108864

// Steps
// sha384sum <binary file>
// echo "<insert hash here>" | xxd -r -p | xxd -i
#define FULL_PFR_IMAGE_PCH_FILE_HASH_SIZE 48
#define FULL_PFR_IMAGE_PCH_FILE_HASH {\
  0x15, 0x15, 0x13, 0xe9, 0x7b, 0xbb, 0xac, 0xc4, 0x85, 0x41, 0x07, 0x9d,\
  0xdf, 0xfb, 0xfc, 0x2f, 0xd4, 0xad, 0x2b, 0xe6, 0xb6, 0xca, 0xe4, 0x10,\
  0xbc, 0x24, 0xe5, 0x55, 0x3a, 0xe8, 0xf5, 0xd0, 0x10, 0x7b, 0x3a, 0xe3,\
  0xe2, 0x62, 0xa8, 0x74, 0x1c, 0x3f, 0x3a, 0x4e, 0xd7, 0x6f, 0xda, 0x3a\
}

/*
 * Signed firmware update capsule
 */
#define SIGNED_CAPSULE_PCH_FILE "testdata/signed_capsule_pch_seamless_secp384.bin"
#define SIGNED_CAPSULE_PCH_FILE_SIZE 12581248

/*
 * Signed Seamless FV capsules
 */
#define SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE "testdata/signed_capsule_pch_seamless_bios_secp384.bin"
#define SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE_SIZE 8395136

#define SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE "testdata/signed_capsule_pch_seamless_me_secp384.bin"
#define SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE_SIZE 4082048

#define SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_FILE "testdata/signed_capsule_pch_seamless_ucode1_secp384.bin"
#define SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_FILE_SIZE 55552

#define SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_FILE "testdata/signed_capsule_pch_seamless_ucode2_secp384.bin"
#define SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_FILE_SIZE 39168

/*
 * Signed PFM & FVM
 */
#define SIGNED_PFM_PCH_FILE "testdata/signed_pfm_pch_seamless_secp384.bin"
#define SIGNED_PFM_PCH_FILE_SIZE 1408

#define SIGNED_PFM_FVM_PCH_BIOS_FILE "testdata/signed_fvm_pch_bios_secp384.bin"
#define SIGNED_PFM_FVM_PCH_BIOS_FILE_SIZE 1280

#define SIGNED_PFM_FVM_PCH_ME_FILE "testdata/signed_fvm_pch_me_secp384.bin"
#define SIGNED_PFM_FVM_PCH_ME_FILE_SIZE 1280

#define SIGNED_PFM_FVM_PCH_UCODE1_FILE "testdata/signed_fvm_pch_ucode1_secp384.bin"
#define SIGNED_PFM_FVM_PCH_UCODE1_FILE_SIZE 1152

#define SIGNED_PFM_FVM_PCH_UCODE2_FILE "testdata/signed_fvm_pch_ucode2_secp384.bin"
#define SIGNED_PFM_FVM_PCH_UCODE2_FILE_SIZE 1152

/*
 * PCH firmware update
 * Generated file with Major version 0x3 and Minor version 0xC
 */
#define FULL_PFR_IMAGE_PCH_V03P12_NON_SEAMLESS_FILE FULL_PFR_IMAGE_PCH_V03P12_FILE
#define FULL_PFR_IMAGE_PCH_V03P12_NON_SEAMLESS_FILE_SIZE FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE
#undef FULL_PFR_IMAGE_PCH_V03P12_FILE
#undef FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE

#define SIGNED_CAPSULE_PCH_V03P12_NON_SEAMLESS_FILE SIGNED_CAPSULE_PCH_V03P12_FILE
#define SIGNED_CAPSULE_PCH_V03P12_NON_SEAMLESS_FILE_SIZE SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE
#undef SIGNED_CAPSULE_PCH_V03P12_FILE
#undef SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE

#define SIGNED_PFM_PCH_V03P12_NON_SEAMLESS_FILE SIGNED_PFM_PCH_V03P12_FILE
#define SIGNED_PFM_PCH_V03P12_NON_SEAMLESS_FILE_SIZE SIGNED_PFM_PCH_V03P12_FILE_SIZE
#undef SIGNED_PFM_PCH_V03P12_FILE
#undef SIGNED_PFM_PCH_V03P12_FILE_SIZE

#define FULL_PFR_IMAGE_PCH_V03P12_FILE "testdata/pch_firmware_update_seamless/full_pfr_image_pch_seamless_secp384.bin"
#define FULL_PFR_IMAGE_PCH_V03P12_FILE_SIZE 67108864

#define SIGNED_CAPSULE_PCH_V03P12_FILE "testdata/pch_firmware_update_seamless/signed_capsule_pch_seamless_secp384.bin"
#define SIGNED_CAPSULE_PCH_V03P12_FILE_SIZE 12581248

#define SIGNED_PFM_PCH_V03P12_FILE "testdata/pch_firmware_update_seamless/signed_pfm_pch_seamless_secp384.bin"
#define SIGNED_PFM_PCH_V03P12_FILE_SIZE 1408

// Seamless capsule
#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE "testdata/pch_firmware_update_seamless/signed_capsule_pch_seamless_bios_secp384.bin"
#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE_SIZE 8395136

#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE "testdata/pch_firmware_update_seamless/signed_capsule_pch_seamless_me_secp384.bin"
#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE_SIZE 4082048

#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_FILE "testdata/pch_firmware_update_seamless/signed_capsule_pch_seamless_ucode1_secp384.bin"
#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_FILE_SIZE 55552

#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_FILE "testdata/pch_firmware_update_seamless/signed_capsule_pch_seamless_ucode2_secp384.bin"
#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_FILE_SIZE 39168

// Bad Seamless capsule
// ME : Start addr -> 0x4000
// UCODE1 : End addr -> 7d0000
// UCODE2 : Start addr -> 0x7e1000
//        : End addr -> 0x7e8000
// BIOS : Start addr -> 0x3001000
//      : End addr -> 0x3901000
#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_WRONG_SPI_ADDR_FILE "testdata/pch_firmware_update_seamless/signed_capsule_pch_seamless_bios_secp384_wrong_spi_addr.bin"
#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_WRONG_SPI_ADDR_FILE_SIZE 4729216

#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_ME_WRONG_SPI_START_ADDR_FILE "testdata/pch_firmware_update_seamless/signed_capsule_pch_seamless_me_secp384_wrong_spi_start_addr.bin"
#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_ME_WRONG_SPI_START_ADDR_FILE_SIZE 4077952

#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_WRONG_SPI_END_ADDR_FILE "testdata/pch_firmware_update_seamless/signed_capsule_pch_seamless_ucode1_secp384_wrong_spi_end_addr.bin"
#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_WRONG_SPI_END_ADDR_FILE_SIZE 22784

#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_WRONG_SPI_ADDR_FILE "testdata/pch_firmware_update_seamless/signed_capsule_pch_seamless_ucode2_secp384_wrong_spi_addr.bin"
#define PCH_FW_UPDATE_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_WRONG_SPI_ADDR_FILE_SIZE 22784

// FVMs
#define PCH_FW_UPDATE_SIGNED_FVM_PCH_BIOS_FILE "testdata/pch_firmware_update_seamless/signed_fvm_pch_bios_secp384.bin"
#define PCH_FW_UPDATE_SIGNED_FVM_PCH_BIOS_FILE_SIZE 1280

#define PCH_FW_UPDATE_SIGNED_FVM_PCH_ME_FILE "testdata/pch_firmware_update_seamless/signed_fvm_pch_me_secp384.bin"
#define PCH_FW_UPDATE_SIGNED_FVM_PCH_ME_FILE_SIZE 1280

#define PCH_FW_UPDATE_SIGNED_FVM_PCH_UCODE1_FILE "testdata/pch_firmware_update_seamless/signed_fvm_pch_ucode1_secp384.bin"
#define PCH_FW_UPDATE_SIGNED_FVM_PCH_UCODE1_FILE_SIZE 1152

#define PCH_FW_UPDATE_SIGNED_FVM_PCH_UCODE2_FILE "testdata/pch_firmware_update_seamless/signed_fvm_pch_ucode2_secp384.bin"
#define PCH_FW_UPDATE_SIGNED_FVM_PCH_UCODE2_FILE_SIZE 1152

/*
 * Generated PCH binaries with different SPI region configuration / FV type / SVN
 * Directory: testdata/pch_firmware_update_seamless_set2
 *
 * Things that are different in this set of images:
 * PFM Major Revision: 0xA
 * PFM Minor Revision: 0x19
 * PFM SPI regions:
 *   - 0x0 - 0x1000
 *   - 0x1000 - 0x3000
 *   - 0x3000 - 0x28000
 *   - 0x7f0000 - 0x1bf0000
 *   - 0x1bf0000 - 0x3000000
 *
 * BIOS FV: FV type == 0; SVN == 0
 *   - Static: 0x3000000 - 0x3850000
 *   - Static: 0x3850000 - 0x3900000
 *   - Dynamic: 0x3900000 - 0x4000000
 *
 * Ucode1 FV: FV type == 2; SVN == 1
 * Ucode2 FV: FV type == 0x10; SVN == 0
 *
 * ME FV: FV type == 1; SVN == 0
 *   - Dynamic: 0x28000 - 0x94000
 *   - Static: 0x94000 - 0x7c7000
 *   - Static: 0x7c7000 - 0x7c8000
 */
#define SET2_FULL_PFR_IMAGE_PCH_FILE "testdata/pch_firmware_update_seamless_set2/full_pfr_image_pch_seamless_secp384.bin"
#define SET2_FULL_PFR_IMAGE_PCH_FILE_SIZE 67108864

#define SET2_SIGNED_CAPSULE_PCH_FILE "testdata/pch_firmware_update_seamless_set2/signed_capsule_pch_seamless_secp384.bin"
#define SET2_SIGNED_CAPSULE_PCH_FILE_SIZE 12581248

#define SET2_SIGNED_PFM_PCH_FILE "testdata/pch_firmware_update_seamless_set2/signed_pfm_pch_seamless_secp384.bin"
#define SET2_SIGNED_PFM_PCH_FILE_SIZE 1408

#define SET2_SIGNED_PFM_PCH_FILE_MAJOR_REV 0xA
#define SET2_SIGNED_PFM_PCH_FILE_MINOR_REV 0x19

// Seamless capsule
#define SET2_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE "testdata/pch_firmware_update_seamless_set2/signed_capsule_pch_seamless_bios_secp384.bin"
#define SET2_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE_SIZE 8395136

#define SET2_SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE "testdata/pch_firmware_update_seamless_set2/signed_capsule_pch_seamless_me_secp384.bin"
#define SET2_SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE_SIZE 4082048

#define SET2_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_FILE "testdata/pch_firmware_update_seamless_set2/signed_capsule_pch_seamless_ucode1_secp384.bin"
#define SET2_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_FILE_SIZE 55552

#define SET2_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_FILE "testdata/pch_firmware_update_seamless_set2/signed_capsule_pch_seamless_ucode2_secp384.bin"
#define SET2_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_FILE_SIZE 39168

// FVMs
#define SET2_SIGNED_FVM_PCH_BIOS_FILE "testdata/pch_firmware_update_seamless_set2/signed_fvm_pch_bios_secp384.bin"
#define SET2_SIGNED_FVM_PCH_BIOS_FILE_SIZE 1280

#define SET2_SIGNED_FVM_PCH_ME_FILE "testdata/pch_firmware_update_seamless_set2/signed_fvm_pch_me_secp384.bin"
#define SET2_SIGNED_FVM_PCH_ME_FILE_SIZE 1280

#define SET2_SIGNED_FVM_PCH_UCODE1_FILE "testdata/pch_firmware_update_seamless_set2/signed_fvm_pch_ucode1_secp384.bin"
#define SET2_SIGNED_FVM_PCH_UCODE1_FILE_SIZE 1152

#define SET2_SIGNED_FVM_PCH_UCODE2_FILE "testdata/pch_firmware_update_seamless_set2/signed_fvm_pch_ucode2_secp384.bin"
#define SET2_SIGNED_FVM_PCH_UCODE2_FILE_SIZE 1152

/*
 * Some bad PCH binaries
 */
#define FULL_PFR_IMAGE_PCH_WITH_NOT_MATCHING_FV_TYPES_FILE "testdata/bad_pch_binaries_seamless/full_pfr_image_pch_seamless_secp384_with_not_matching_fv_types.bin"
#define FULL_PFR_IMAGE_PCH_WITH_NOT_MATCHING_FV_TYPES_FILE_SIZE 67108864

#define SIGNED_CAPSULE_PCH_WITH_NOT_MATCHING_FV_TYPES_FILE "testdata/bad_pch_binaries_seamless/signed_capsule_pch_seamless_secp384_with_not_matching_fv_types.bin"
#define SIGNED_CAPSULE_PCH_WITH_NOT_MATCHING_FV_TYPES_FILE_SIZE 12581248

/*
 * Generated PCH binaries with SVNs > 1
 * Directory: testdata/anti_rollback_with_svn_seamless
 *
 * PCH PFM: SVN == 1
 * BIOS FV: FV type == 0; SVN == 33
 * ME FV: FV type == 1; SVN == 1
 * Ucode1 FV: FV type == 2; SVN == 3
 * Ucode2 FV: FV type == 3; SVN == 5
 */
#define ANTI_ROLLBACK_FULL_PFR_IMAGE_PCH_FILE "testdata/anti_rollback_with_svn_seamless/full_pfr_image_pch_seamless_secp384.bin"
#define ANTI_ROLLBACK_FULL_PFR_IMAGE_PCH_FILE_SIZE 67108864
#define ANTI_ROLLBACK_FULL_PFR_IMAGE_PCH_FILE_SVN 1

#define ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_FILE "testdata/anti_rollback_with_svn_seamless/signed_capsule_pch_seamless_secp384.bin"
#define ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_FILE_SIZE 12581248
#define ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_FILE_SVN 1

#define ANTI_ROLLBACK_SIGNED_PFM_PCH_FILE "testdata/anti_rollback_with_svn_seamless/signed_pfm_pch_seamless_secp384.bin"
#define ANTI_ROLLBACK_SIGNED_PFM_PCH_FILE_SIZE 1408

// Seamless capsule
#define ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE "testdata/anti_rollback_with_svn_seamless/signed_capsule_pch_seamless_bios_secp384.bin"
#define ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE_SIZE 8395136
#define ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_FILE_SVN 33

#define ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE "testdata/anti_rollback_with_svn_seamless/signed_capsule_pch_seamless_me_secp384.bin"
#define ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE_SIZE 4082048
#define ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_ME_FILE_SVN 1

#define ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_FILE "testdata/anti_rollback_with_svn_seamless/signed_capsule_pch_seamless_ucode1_secp384.bin"
#define ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_FILE_SIZE 55552
#define ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE1_FILE_SVN 3

#define ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_FILE "testdata/anti_rollback_with_svn_seamless/signed_capsule_pch_seamless_ucode2_secp384.bin"
#define ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_FILE_SIZE 39168
#define ANTI_ROLLBACK_SIGNED_CAPSULE_PCH_SEAMLESS_UCODE2_FILE_SVN 5

// FVMs
#define ANTI_ROLLBACK_SIGNED_FVM_PCH_BIOS_FILE "testdata/anti_rollback_with_svn_seamless/signed_fvm_pch_bios_secp384.bin"
#define ANTI_ROLLBACK_SIGNED_FVM_PCH_BIOS_FILE_SIZE 1280

#define ANTI_ROLLBACK_SIGNED_FVM_PCH_ME_FILE "testdata/anti_rollback_with_svn_seamless/signed_fvm_pch_me_secp384.bin"
#define ANTI_ROLLBACK_SIGNED_FVM_PCH_ME_FILE_SIZE 1280

#define ANTI_ROLLBACK_SIGNED_FVM_PCH_UCODE1_FILE "testdata/anti_rollback_with_svn_seamless/signed_fvm_pch_ucode1_secp384.bin"
#define ANTI_ROLLBACK_SIGNED_FVM_PCH_UCODE1_FILE_SIZE 1152

#define ANTI_ROLLBACK_SIGNED_FVM_PCH_UCODE2_FILE "testdata/anti_rollback_with_svn_seamless/signed_fvm_pch_ucode2_secp384.bin"
#define ANTI_ROLLBACK_SIGNED_FVM_PCH_UCODE2_FILE_SIZE 1152

/*
 * Anti-rollback with CSK tests
 */
#define SIGNED_CAPSULE_PCH_SEAMLESS_WITH_CSK_ID10_FILE "testdata/anti_rollback_with_csk_id_seamless/signed_capsule_pch_seamless_with_csk_id10_secp384.bin"
#define SIGNED_CAPSULE_PCH_SEAMLESS_WITH_CSK_ID10_FILE_SIZE 12581248

#define SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_WITH_CSK_ID10_FILE "testdata/anti_rollback_with_csk_id_seamless/signed_capsule_pch_seamless_bios_with_csk_id10_secp384.bin"
#define SIGNED_CAPSULE_PCH_SEAMLESS_BIOS_WITH_CSK_ID10_FILE_SIZE 8395136

#define SIGNED_CAPSULE_PCH_SEAMLESS_ME_WITH_CSK_ID10_FILE "testdata/anti_rollback_with_csk_id_seamless/signed_capsule_pch_seamless_me_with_csk_id10_secp384.bin"
#define SIGNED_CAPSULE_PCH_SEAMLESS_ME_WITH_CSK_ID10_FILE_SIZE 4082048

#define SIGNED_FVM_PCH_ME_WITH_CSK_ID2_FILE "testdata/anti_rollback_with_csk_id_seamless/signed_fvm_pch_me_with_csk_id2_secp384.bin"
#define SIGNED_FVM_PCH_ME_WITH_CSK_ID2_FILE_SIZE 1280

#define SIGNED_FVM_PCH_UCODE1_WITH_CSK_ID2_FILE "testdata/anti_rollback_with_csk_id_seamless/signed_fvm_pch_ucode1_with_csk_id2_secp384.bin"
#define SIGNED_FVM_PCH_UCODE1_WITH_CSK_ID2_FILE_SIZE 1152

/*
 * FV type test
 */
#define PCH_SEAMLESS_FULL_IMAGE_GOOD_FV_TYPE_FILE "testdata/full_pfr_image_pch_seamless_supported_fv_secp384.bin"
#define PCH_SEAMLESS_FULL_IMAGE_BAD_UCODE1_FV_TYPE_50_FILE "testdata/full_pfr_image_pch_seamless_bad_ucode1_fv_type_50_secp384.bin"

#define SIGNED_CAPSULE_PCH_SEAMLESS_FULL_IMAGE_BAD_UCODE1_FV_TYPE_50_FILE "testdata/signed_capsule_pch_seamless_bad_ucode1_fv_type_50_secp384.bin"

/*
 * Key Cancellation Certificate
 */
#define KEY_CAN_CERT_PCH_FV_SEAMLESS_CAPSULE_KEY10 "testdata/key_cancellation_certs/signed_pch_fv_seamless_capsule_key10_can_cert_secp384.bin"


#endif /* INC_TESTDATA_FILES_SEAMLESS_SECP384_H */
