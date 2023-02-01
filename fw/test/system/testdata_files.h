#ifndef INC_TESTDATA_FILES_H
#define INC_TESTDATA_FILES_H

/************************************************
 *
 * This header describes all the unittests binaries files under fw/test/testdata.
 * All BMC and PCH flash images are generated with gen_pfr_flash_images/... scripts.
 *
 ************************************************/

// Choose binary files signed with secp256r1 or secp384r1 curve.
#ifdef GTEST_USE_CRYPTO_384
#include "testdata_files_secp384.h"

#ifdef GTEST_ENABLE_SEAMLESS_FEATURES
#include "testdata_files_seamless_secp384.h"
#endif /* GTEST_ENABLE_SEAMLESS_FEATURES */

#ifdef GTEST_ATTEST_384
#include "testdata_files_attestation_secp384.h"
#include "testdata_files_x509_certificate_secp384.h"
#endif /* GTEST_ATTEST_384 */

#else
// GTEST using CRYPTO 256 mode
#include "testdata_files_secp256.h"

#ifdef GTEST_ENABLE_SEAMLESS_FEATURES
// TODO: Add Seamless binaries signed with secp256 key (WIP: bin images of 2/6 existing unittests added)
//#include "testdata_files_seamless_secp256.h"
#endif /* GTEST_ENABLE_SEAMLESS_FEATURES */

#ifdef GTEST_ATTEST_256
#include "testdata_files_attestation_secp256.h"
#endif /* GTEST_ATTEST_256 */

#endif /* GTEST_USE_CRYPTO_384 */

#endif /* INC_TESTDATA_FILES_H */
