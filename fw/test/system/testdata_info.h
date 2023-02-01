#ifndef INC_TESTDATA_INFO_H
#define INC_TESTDATA_INFO_H

// Choose binary signed with secp256r1 or secp384r1 curve.
#ifdef GTEST_USE_CRYPTO_384
#include "testdata_info_secp384.h"

#ifdef GTEST_ENABLE_SEAMLESS_FEATURES
#include "testdata_info_seamless_secp384.h"
#endif /* GTEST_ENABLE_SEAMLESS_FEATURES */

#else
#include "testdata_info_secp256.h"

#ifdef GTEST_ENABLE_SEAMLESS_FEATURES
// TODO: Add Seamless binaries signed with secp256 key (WIP: bin images of 2/6 existing unittests added)
//#include "testdata_info_seamless_secp256.h"
#endif /* GTEST_ENABLE_SEAMLESS_FEATURES */

#endif /* GTEST_USE_CRYPTO_384 */

// Arrays used in FW Update/Recovery flow unittests
// PCH FW
const alt_u32 testdata_pch_spi_regions_start_addr[PCH_NUM_SPI_REGIONS] = PCH_SPI_REGIONS_START_ADDR;
const alt_u32 testdata_pch_spi_regions_end_addr[PCH_NUM_SPI_REGIONS] = PCH_SPI_REGIONS_END_ADDR;

const alt_u32 testdata_pch_static_regions_start_addr[PCH_NUM_STATIC_SPI_REGIONS] = PCH_STATIC_SPI_REGIONS_START_ADDR;
const alt_u32 testdata_pch_static_regions_end_addr[PCH_NUM_STATIC_SPI_REGIONS] = PCH_STATIC_SPI_REGIONS_END_ADDR;

const alt_u32 testdata_pch_dynamic_regions_start_addr[PCH_NUM_DYNAMIC_SPI_REGIONS] = PCH_DYNAMIC_SPI_REGIONS_START_ADDR ;
const alt_u32 testdata_pch_dynamic_regions_end_addr[PCH_NUM_DYNAMIC_SPI_REGIONS] = PCH_DYNAMIC_SPI_REGIONS_END_ADDR ;

// BMC FW
const alt_u32 testdata_bmc_spi_regions_start_addr[BMC_NUM_SPI_REGIONS] = BMC_SPI_REGIONS_START_ADDR;
const alt_u32 testdata_bmc_spi_regions_end_addr[BMC_NUM_SPI_REGIONS] = BMC_SPI_REGIONS_END_ADDR;

const alt_u32 testdata_bmc_static_regions_start_addr[BMC_NUM_STATIC_SPI_REGIONS] = BMC_STATIC_SPI_REGIONS_START_ADDR;
const alt_u32 testdata_bmc_static_regions_end_addr[BMC_NUM_STATIC_SPI_REGIONS] = BMC_STATIC_SPI_REGIONS_END_ADDR;

const alt_u32 testdata_bmc_dynamic_regions_start_addr[BMC_NUM_DYNAMIC_SPI_REGIONS] = BMC_DYNAMIC_SPI_REGIONS_START_ADDR;
const alt_u32 testdata_bmc_dynamic_regions_end_addr[BMC_NUM_DYNAMIC_SPI_REGIONS] = BMC_DYNAMIC_SPI_REGIONS_END_ADDR;


#endif /* INC_TESTDATA_INFO_H */
