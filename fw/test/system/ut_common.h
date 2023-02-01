#ifndef UNITTEST_SYSTEM_UT_COMMON_H_
#define UNITTEST_SYSTEM_UT_COMMON_H_

// Always include the BSP mock first
#include "bsp_mock.h"

// Include the GTest headers
#include "gtest_headers.h"

// Include the PFR headers
#include "authentication.h"
#include "attestation.h"
#include "capsule_validation.h"
#include "cpld_update.h"
#include "cpld_reconfig.h"
#include "cpld_recovery.h"
#include "crypto.h"
#include "decompression.h"
#include "dual_config_utils.h"
#include "firmware_recovery.h"
#include "firmware_update.h"
#include "flash_validation.h"
#include "gen_gpi_signals.h"
#include "gen_gpo_controls.h"
#include "gen_smbus_relay_config.h"
#include "global_state.h"
#include "keychain.h"
#include "keychain_utils.h"
#include "mailbox_enums.h"
#include "mailbox_utils.h"
#include "pbc.h"
#include "pbc_utils.h"
#include "pfm.h"
#include "pfm_utils.h"
#include "pfm_validation.h"
#include "pfr_main.h"
#include "pfr_pointers.h"
#include "pfr_sys.h"
#include "platform_log.h"
#include "recovery_main.h"
#include "rfnvram_utils.h"
#include "smbus_relay_utils.h"
#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)
#include "spdm.h"
#include "mctp.h"
#include "mctp_utils.h"
#include "mctp_flow.h"
#include "mctp_transport.h"
#include "spdm_responder.h"
#include "spdm_requester.h"
#include "spdm_utils.h"
#include "cert.h"
#include "cert_utils.h"
#include "cert_flow.h"
#include "cert_gen_options.h"
#endif
#include "spi_common.h"
#include "spi_ctrl_utils.h"
#include "spi_flash_state.h"
#include "spi_rw_utils.h"
#include "status_enums.h"
#include "t0_provisioning.h"
#include "t0_routines.h"
#include "t0_update.h"
#include "t0_watchdog_handler.h"
#include "timer_utils.h"
#include "tmin1_routines.h"
#include "transition.h"
#include "ufm.h"
#include "ufm_rw_utils.h"
#include "ufm_svn_utils.h"
#include "ufm_utils.h"
#include "utils.h"
#include "watchdog_timers.h"

#ifdef GTEST_ENABLE_SEAMLESS_FEATURES
#include "seamless_update.h"
#endif /* GTEST_ENABLE_SEAMLESS_FEATURES */


#endif /* UNITTEST_SYSTEM_UT_COMMON_H_ */
