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
 * @file watchdog_timers.h
 * @brief Define the SW watchdog timers that are used to track BMC/PCH firmware boot progress.
 */

#ifndef EAGLESTREAM_INC_WATCHDOG_TIMERS_H
#define EAGLESTREAM_INC_WATCHDOG_TIMERS_H

// Always include pfr_sys.h first
#include "pfr_sys.h"
#include "timer_utils.h"


/*
 * Watchdog Timers
 */
#define WDT_BMC_TIMER_ADDR        U_TIMER_BANK_TIMER1_ADDR
#define WDT_ME_TIMER_ADDR         U_TIMER_BANK_TIMER2_ADDR
#define WDT_ACM_BIOS_TIMER_ADDR   U_TIMER_BANK_TIMER3_ADDR
#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

#define WDT_CPLD_ATTEST_HOST_ADDR   U_TIMER_BANK_TIMER4_ADDR

#endif

/*
 * Watchdog timer boot progress tracking
 */
#define WDT_BMC_BOOT_DONE_MASK      0b00001
#define WDT_ME_BOOT_DONE_MASK       0b00010
#define WDT_ACM_BOOT_DONE_MASK      0b00100
#define WDT_IBB_BOOT_DONE_MASK      0b01000
#define WDT_OBB_BOOT_DONE_MASK      0b10000
#define WDT_ACM_BIOS_BOOT_DONE_MASK (WDT_ACM_BOOT_DONE_MASK | WDT_IBB_BOOT_DONE_MASK | WDT_OBB_BOOT_DONE_MASK)
#define WDT_PCH_BOOT_DONE_MASK      (WDT_ME_BOOT_DONE_MASK | WDT_ACM_BOOT_DONE_MASK | WDT_IBB_BOOT_DONE_MASK | WDT_OBB_BOOT_DONE_MASK)
#define WDT_ALL_BOOT_DONE_MASK      (WDT_BMC_BOOT_DONE_MASK | WDT_PCH_BOOT_DONE_MASK)

static alt_u8 wdt_boot_status = 0;

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

#define WDT_FIRST_CHAL_DONE_MASK    0b100000

static alt_u8 wdt_attest_status = 0;

#endif

/*
 * Watchdog timer enable/disable
 *
 * In recovery flow, some WDTs may be disabled
 * In update flow, these WDTs can be re-enabled through a successful update.
 */
#define WDT_ENABLE_BMC_TIMER_MASK      0b001
#define WDT_ENABLE_ME_TIMER_MASK       0b010
#define WDT_ENABLE_ACM_BIOS_TIMER_MASK 0b100
#define WDT_ENABLE_PCH_TIMERS_MASK     0b110
#define WDT_ENABLE_ALL_TIMERS_MASK     0b111

static alt_u8 wdt_enable_status = WDT_ENABLE_ALL_TIMERS_MASK;

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE is_first_challenge_done()
{
    return wdt_attest_status == WDT_FIRST_CHAL_DONE_MASK;
}

#endif
/**
 * @brief Check if all components (BMC/ME/ACM/BIOS) have completed boot.
 *
 * @return alt_u32 1 if all components have completed boot. 0, otherwise.
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE is_timed_boot_done()
{
    return wdt_boot_status == WDT_ALL_BOOT_DONE_MASK;
}

/**
 * @brief Check if the given timer is expired
 * If the HW timer is active and the count down value reaches 0,
 * then this timer has expired
 *
 * @return alt_u32 1 if the given timer is expired. 0, otherwise.
 */
static alt_u32 is_wd_timer_expired(alt_u32* timer_addr)
{
    return IORD(timer_addr, 0) == U_TIMER_BANK_TIMER_ACTIVE_MASK;
}

#endif /* EAGLESTREAM_INC_WATCHDOG_TIMERS_H */
