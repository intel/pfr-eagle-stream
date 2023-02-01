#ifndef UNITTEST_SYSTEM_UT_NIOS_WRAPPER_H_
#define UNITTEST_SYSTEM_UT_NIOS_WRAPPER_H_

#include "ut_common.h"

/***************************************
 *
 *  Macro for unittest timeout
 *
 ***************************************/
#if defined GTEST_RUNNING_LCOV
    #define GTEST_TIMEOUT_SHORT   180  // allow  3 minute  timeout
    #define GTEST_TIMEOUT_MEDIUM  300  // allow  5 minutes timeout
    #define GTEST_TIMEOUT_LONG    900  // allow 15 minutes timeout
#else
    #define GTEST_TIMEOUT_SHORT    60  // allow  1 minute  timeout
    #define GTEST_TIMEOUT_MEDIUM  120  // allow  2 minutes timeout
    #define GTEST_TIMEOUT_LONG    300  // allow  5 minutes timeout
#endif

/********************
 * Manifest type
 *******************/
#define UT_PFM 1
#define UT_AFM 2
#define UT_FVM 3

#define UT_MANIFEST_SIG 1
#define UT_CAPSULE_SIG 2

#define UT_ACTIVE 1
#define UT_STAGING 2
#define UT_RECOVERY 3

/***************************************
 *
 *  GPIOs manipulation
 *
 ***************************************/
static void ut_prep_nios_gpi_signals()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ && addr == U_GPI_1_ADDR)
        {
            // De-assert both PCH and BMC resets from common core (0b11)
            // Signals that ME firmware has booted (0b1000)
            // Keep FORCE_RECOVERY signal inactive (0b1000000)
            SYSTEM_MOCK::get()->set_mem_word(U_GPI_1_ADDR, alt_u32(0x4b), true);

            if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_ENTER_T0) &&
                    check_bit(U_GPO_1_ADDR, GPO_1_RST_RSMRST_PLD_R_N))
            {
                // When Nios firmware just reaches T0 and PCH is out of reset, simulate the hw PLTRST# toggle
                // Set the GPI_1_PLTRST_DETECTED_REARM_ACM_TIMER bit (0b10000)
                SYSTEM_MOCK::get()->set_mem_word(U_GPI_1_ADDR, alt_u32(0x5b), true);
            }
        }
    });
}

static void ut_send_bmc_reset_detected_gpi_once_upon_boot_complete()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ && addr == U_GPI_1_ADDR)
        {
            if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_T0_BOOT_COMPLETE) &&
                    (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
            {
                // De-assert both PCH and BMC resets (0b11)
                // Keep FORCE_RECOVERY signal inactive (0b1000000)
                // BMC reset detected (0b100000)
                SYSTEM_MOCK::get()->set_mem_word(U_GPI_1_ADDR, alt_u32(0x63), true);
            }
        }
    });
}

static void ut_toggle_pltrst_gpi_once_upon_boot_complete()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ && addr == U_GPI_1_ADDR)
        {
            if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_T0_BOOT_COMPLETE) &&
                    (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
            {
                // De-assert both PCH and BMC resets from common core (0b11)
                // Signals that ME firmware has booted (0b1000)
                // Keep FORCE_RECOVERY signal inactive (0b1000000)
                // Set the GPI_1_PLTRST_DETECTED_REARM_ACM_TIMER bit (0b10000)

                // When Nios firmware reaches T0 boot complete, simulate the hw PLTRST# toggle
                SYSTEM_MOCK::get()->set_mem_word(U_GPI_1_ADDR, alt_u32(0x5b), true);
            }
        }
    });
}

/**
 * @brief Return the value in the global state register
 *
 * @return alt_u32 global state
 */
static alt_u32 ut_get_global_state()
{
    return IORD_32DIRECT(U_GLOBAL_STATE_REG_ADDR, 0);
}

/***************************************
 *
 *  Update flow
 *
 ***************************************/
static void ut_send_in_update_intent(MB_REGFILE_OFFSET_ENUM update_intent_offset, alt_u32 update_intent_value)
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [update_intent_offset, update_intent_value](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + update_intent_offset;
            if (addr == update_intent_addr)
            {
                if (read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_T0_BOOT_COMPLETE)
                {
                    SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, update_intent_value, true);
                }
            }
        }
    });
}

static void ut_send_in_update_intent_tmin1(MB_REGFILE_OFFSET_ENUM update_intent_offset, alt_u32 update_intent_value)
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [update_intent_offset, update_intent_value](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + update_intent_offset;
            if (addr == update_intent_addr)
            {
                SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, update_intent_value, true);
            }
        }
    });
}

static void ut_send_in_update_intent_once_upon_entry_to_t0(MB_REGFILE_OFFSET_ENUM update_intent_offset, alt_u32 update_intent_value)
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [update_intent_offset, update_intent_value](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + update_intent_offset;
            if (addr == update_intent_addr)
            {
                if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_ENTER_T0) && (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                {
                    SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, update_intent_value, true);
                }
            }
        }
    });
}

static void ut_send_in_update_intent_once_upon_boot_complete(MB_REGFILE_OFFSET_ENUM update_intent_offset, alt_u32 update_intent_value)
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [update_intent_offset, update_intent_value](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* update_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + update_intent_offset;
            if (addr == update_intent_addr)
            {
                if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_T0_BOOT_COMPLETE) && (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                {
                    SYSTEM_MOCK::get()->set_mem_word(update_intent_addr, update_intent_value, true);
                }
            }
        }
    });
}

static alt_u32 ut_get_num_failed_update_attempts()
{
    return num_failed_fw_or_cpld_update_attempts;
}

static alt_u32 ut_nios_is_blocking_update()
{
    return num_failed_fw_or_cpld_update_attempts >= MAX_FAILED_UPDATE_ATTEMPTS_ALLOWED;
}

#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)
/***************************************
 *
 * Attestation
 *
 ***************************************/

/**
 * The mailbox mock has been designed to mimic the behaviour of RTL mailbox.
 * To support attestation feature, the rtl is handling certain error conditions and thus, the mock is also designed to do the same.
 * The mock will also handle the available bit routine to enable firmware to poll for this bit
 *
 * Error condition instantiated by mock and rtl:
 *
 * 1) Packet size < byte count
 *    Mock will send error bit for that particular packet at the last byte of the packet
 *
 * 2) Packet size > byte count
 *    Mock will send error bit for that particular packet whenever the packet byte crosses the byte count.
 *    Mock will silently drop the excess packet.
 *
 * 3) TODO: if packet size mismatch with the previous packet except last packet, send error bit.
 *
 * Handling of Byte Count
 * Mock follows RTL implementation on byte count manipulation. Mock sets a counter to count incoming packet size
 * and compares them with the MCTP 'byte count' field. If they are mismatched, mock silently update the
 * 'byte count' register and dropped the excessive packet.
 *
 * Handling of Available bit
 *
 * Whenever mock detected a complete packet for exp, 1 MCTP complete packet, Mock will set an available bit.
 * The 4 lsb of the register maps to 4 available mctp packets for that particular host's fifo
 * The 4 msb of the register maps to 4 error bit of the mctp packet stored in the fifo
 * The mapping of the available and error register is as follow:
 *
 * available/error byte : 0 0 0 0   |   0 0 0 0
 *                           ^             ^
 *                        error bit     available bit
 *
 * Use case scenario:
 * 1) If BMC writes to the fifo its first mcpt packet, the lsb of the available section will be set.
 *    If there is no error on the first packet, the available/error byte would look like:
 *
 *    0 0 0 0   |   0 0 0 1
 *
 * 2) If however the first mctp packet from bmc is faulty, then the available/error byte would look like:
 *
 *    0 0 0 1   |   0 0 0 1
 *
 * 3) If now BMC writes its second packet to the fifo and the packet is not faulty, the available/error byte would look like:
 *
 *    0 0 0 1   |   0 0 1 1
 *
 * 4) Now if BMC writes a third faulty packet, then the available/error byte would look like:
 *
 *    0 1 0 1   |   0 1 1 1
 *
 * Firmware has the task to write 1 to available/register to clear the bit so that firmware can start to poll for the next available bit.
 * At the same time, firmware must check for the error bit for that particular packet before handling further process like handling SOM, EOM, pkt seq and so on...
 *
 * If the available/error byte look like:
 *
 *    0 1 0 1   |   0 1 1 1
 *
 * When firmware writes '1' to the register and is detected by the mock, the mock will right shift the avail/error byte and would look like:
 *
 *    0 0 1 0   |   0 0 1 1
 *
 * And then if W12C again, it would look like:
 *
 *    0 0 0 1   |   0 0 0 1
 */


static void ut_send_in_challenge_intent(MB_REGFILE_OFFSET_ENUM challenge_intent_offset, alt_u32 challenge_intent_value)
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [challenge_intent_offset, challenge_intent_value](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* challenge_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + challenge_intent_offset;
            if (addr == challenge_intent_addr)
            {
                SYSTEM_MOCK::get()->set_mem_word(challenge_intent_addr, challenge_intent_value, true);
            }
        }
    });
}

static void ut_send_in_challenge_intent_tmin1(MB_REGFILE_OFFSET_ENUM challenge_intent_offset, alt_u32 challenge_intent_value)
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [challenge_intent_offset, challenge_intent_value](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* challenge_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + challenge_intent_offset;
            if (addr == challenge_intent_addr)
            {
                SYSTEM_MOCK::get()->set_mem_word(challenge_intent_addr, challenge_intent_value, true);
            }
        }
    });
}

static void ut_send_in_challenge_intent_once_upon_entry_to_t0(MB_REGFILE_OFFSET_ENUM challenge_intent_offset, alt_u32 challenge_intent_value)
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [challenge_intent_offset, challenge_intent_value](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* challenge_intent_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + challenge_intent_offset;
            if (addr == challenge_intent_addr)
            {
                if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_ENTER_T0) && (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 0))
                {
                    SYSTEM_MOCK::get()->set_mem_word(challenge_intent_addr, challenge_intent_value, true);
                }
            }
        }
    });
}

static void ut_send_in_bmc_mctp_packet(alt_u32* rxfifo, alt_u8* mctp_pkt, alt_u32 pkt_size, alt_u32 stage_next_msg, alt_u8 time_unit)
{
    for (alt_u32 byte = 0; byte < pkt_size; byte++)
    {
        IOWR(rxfifo, MB_MCTP_BMC_PACKET_READ_RXFIFO, mctp_pkt[byte]);
    }

    IOWR(rxfifo, MB_MCTP_PACKET_WRITE_RXFIFO, 255);

    if (stage_next_msg)
    {
        IOWR(rxfifo, MB_MCTP_PACKET_WRITE_RXFIFO, 201);
    }

    if (time_unit <= 200)
    {
        IOWR(rxfifo, MB_MCTP_PACKET_WRITE_RXFIFO, time_unit);
    }
}

static void ut_send_in_pch_mctp_packet(alt_u32* rxfifo, alt_u8* mctp_pkt, alt_u32 pkt_size, alt_u32 stage_next_msg, alt_u8 time_unit)
{
    for (alt_u32 byte = 0; byte < pkt_size; byte++)
    {
        IOWR(rxfifo, MB_MCTP_PCH_PACKET_READ_RXFIFO, mctp_pkt[byte]);
    }

    IOWR(rxfifo, MB_MCTP_PACKET_WRITE_RXFIFO, 254);

    if (stage_next_msg)
    {
        IOWR(rxfifo, MB_MCTP_PACKET_WRITE_RXFIFO, 202);
    }

    if (time_unit <= 200)
    {
        IOWR(rxfifo, MB_MCTP_PACKET_WRITE_RXFIFO, time_unit);
    }
}

static void ut_send_in_pcie_mctp_packet(alt_u32* rxfifo, alt_u8* mctp_pkt, alt_u32 pkt_size, alt_u32 stage_next_msg, alt_u8 time_unit)
{
    for (alt_u32 byte = 0; byte < pkt_size; byte++)
    {
        IOWR(rxfifo, MB_MCTP_PCIE_PACKET_READ_RXFIFO, mctp_pkt[byte]);
    }

    IOWR(rxfifo, MB_MCTP_PACKET_WRITE_RXFIFO, 253);

    if (stage_next_msg)
    {
        IOWR(rxfifo, MB_MCTP_PACKET_WRITE_RXFIFO, 203);
    }

    if (time_unit <= 200)
    {
        IOWR(rxfifo, MB_MCTP_PACKET_WRITE_RXFIFO, time_unit);
    }
}

static void ut_send_in_mctp_packet(alt_u32* rxfifo, alt_u8* mctp_pkt, alt_u32 pkt_size, alt_u32 stage_next_msg, alt_u8 time_unit, ATTESTATION_HOST host)
{
    if (host == BMC)
    {
        ut_send_in_bmc_mctp_packet(rxfifo, mctp_pkt, pkt_size, stage_next_msg, time_unit);
    }
    else if (host == PCIE)
    {
        ut_send_in_pcie_mctp_packet(rxfifo, mctp_pkt, pkt_size, stage_next_msg, time_unit);
    }
    else if (host == PCH)
    {
        ut_send_in_pch_mctp_packet(rxfifo, mctp_pkt, pkt_size, stage_next_msg, time_unit);
    }
}

#endif


/***************************************
 *
 *  UFM
 *
 ***************************************/
static alt_u32 ut_check_ufm_prov_status(MB_UFM_PROV_STATUS_MASK_ENUM status_mask)
{
    return (IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & status_mask) == status_mask;
}

static void ut_send_in_ufm_command(MB_UFM_PROV_CMD_ENUM ufm_prov_cmd)
{
    alt_u32* mb_ufm_prov_cmd_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_PROVISION_CMD;
    alt_u32* mb_ufm_cmd_trigger_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_UFM_CMD_TRIGGER;

    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_prov_cmd_addr, ufm_prov_cmd, true);
    SYSTEM_MOCK::get()->set_mem_word(mb_ufm_cmd_trigger_addr, MB_UFM_CMD_EXECUTE_MASK, true);
}

static void ut_wait_for_ufm_prov_cmd_done()
{
    while ((IORD(U_MAILBOX_AVMM_BRIDGE_ADDR, MB_PROVISION_STATUS) & MB_UFM_PROV_CMD_DONE_MASK) == 0) {}
}

/***************************************
 *
 *  Watchdog timer
 *
 ***************************************/
static void ut_send_block_complete_chkpt_msg()
{
    // Signals that BMC/ACM/BIOS have all booted after one check
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* bmc_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BMC_CHECKPOINT;
            alt_u32* acm_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_ACM_CHECKPOINT;
            alt_u32* bios_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BIOS_CHECKPOINT;

            if (addr == bmc_ckpt_addr)
            {
                SYSTEM_MOCK::get()->set_mem_word(bmc_ckpt_addr, MB_CHKPT_COMPLETE, true);
            }
            else if (addr == acm_ckpt_addr)
            {
                // Nios is supposed to ignore the BOOT_START/BOOT_DONE checkpoint messages from ACM.
                // Sending this shouldn't hurt.
                SYSTEM_MOCK::get()->set_mem_word(acm_ckpt_addr, MB_CHKPT_COMPLETE, true);
            }
            else if (addr == bios_ckpt_addr)
            {
                if (wdt_boot_status & WDT_ACM_BOOT_DONE_MASK)
                {
                    // Once ACM is booted, keep sending BOOT_DONE checkpoint to BIOS checkpoint register.
                    // With this, Nios is supposed to turn off IBB and OBB WDT.
                    SYSTEM_MOCK::get()->set_mem_word(bios_ckpt_addr, MB_CHKPT_COMPLETE, true);
                }
                else
                {
                    // Sends BOOT_START to BIOS checkpoint, when ACM is booting.
                    // Nios is supposed to turn off ACM WDT when IBB starts to boot.
                    SYSTEM_MOCK::get()->set_mem_word(bios_ckpt_addr, MB_CHKPT_START, true);
                }
            }
        }
    });
}

static void ut_send_acm_chkpt_msg()
{
    // Signals that BMC/ACM/BIOS have all booted after one check
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* acm_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_ACM_CHECKPOINT;

            if (addr == acm_ckpt_addr)
            {
                // Nios is supposed to ignore the BOOT_START/BOOT_DONE checkpoint messages from ACM.
                // Sending this shouldn't hurt.
                SYSTEM_MOCK::get()->set_mem_word(acm_ckpt_addr, MB_CHKPT_COMPLETE, true);
            }
        }
    });
}

static void ut_send_acm_bios_chkpt_msg()
{
    // Signals that BMC/ACM/BIOS have all booted after one check
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* acm_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_ACM_CHECKPOINT;
            alt_u32* bios_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BIOS_CHECKPOINT;

            if (addr == acm_ckpt_addr)
            {
                // Nios is supposed to ignore the BOOT_START/BOOT_DONE checkpoint messages from ACM.
                // Sending this shouldn't hurt.
                SYSTEM_MOCK::get()->set_mem_word(acm_ckpt_addr, MB_CHKPT_COMPLETE, true);
            }
            else if (addr == bios_ckpt_addr)
            {
                if (wdt_boot_status & WDT_ACM_BOOT_DONE_MASK)
                {
                    // Once ACM is booted, keep sending BOOT_DONE checkpoint to BIOS checkpoint register.
                    // With this, Nios is supposed to turn off IBB and OBB WDT.
                    SYSTEM_MOCK::get()->set_mem_word(bios_ckpt_addr, MB_CHKPT_COMPLETE, true);
                }
                else
                {
                    // Sends BOOT_START to BIOS checkpoint, when ACM is booting.
                    // Nios is supposed to turn off ACM WDT when IBB starts to boot.
                    SYSTEM_MOCK::get()->set_mem_word(bios_ckpt_addr, MB_CHKPT_START, true);
                }
            }
        }
    });
}

static void ut_send_bmc_chkpt_msg()
{
    // Signals that BMC/ACM/BIOS have all booted after one check
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* bmc_ckpt_addr = U_MAILBOX_AVMM_BRIDGE_ADDR + MB_BMC_CHECKPOINT;

            if (addr == bmc_ckpt_addr)
            {
                SYSTEM_MOCK::get()->set_mem_word(bmc_ckpt_addr, MB_CHKPT_COMPLETE, true);
            }
        }
    });
}

static alt_u32 spi_breadcrumbs_act = 0;
static alt_u32 spi_breadcrumbs_rec = 0;
static alt_u32 spi_stat_read = 0;

static void ut_block_spi_read_access_once_during_bmc_auth()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* spi_stat = SPI_CONTROL_1_CSR_BASE_ADDR + SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST;

            if (addr == spi_stat)
            {
                if (read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_BMC_FLASH_AUTHENTICATION)
                {
                    alt_u32* m_flash_ptr = get_spi_flash_ptr();
                    if (!spi_stat_read)
                    {
                        spi_breadcrumbs_act = *m_flash_ptr;
                        *m_flash_ptr = 0xdeadcafe;
                        m_flash_ptr = get_spi_flash_ptr_with_offset(get_recovery_region_offset(SPI_FLASH_BMC));
                        spi_breadcrumbs_rec = *m_flash_ptr;
                        *m_flash_ptr = 0xdeadcafe;
                    }
                    else if (spi_stat_read == 2)
                    {
                        *m_flash_ptr = spi_breadcrumbs_act;
                        m_flash_ptr = get_spi_flash_ptr_with_offset(get_recovery_region_offset(SPI_FLASH_BMC));
                        *m_flash_ptr = spi_breadcrumbs_rec;
                    }
                    spi_stat_read += 1;
                }
            }
        }
    });
}

static void ut_block_spi_read_access_once_during_pch_auth()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* spi_stat = SPI_CONTROL_1_CSR_BASE_ADDR + SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST;

            if (addr == spi_stat)
            {
                if (read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_PCH_FLASH_AUTHENTICATION)
                {
                    alt_u32* m_flash_ptr = get_spi_flash_ptr();
                    if (!spi_stat_read)
                    {
                        spi_breadcrumbs_act = *m_flash_ptr;
                        *m_flash_ptr = 0xdeadcafe;
                        m_flash_ptr = get_spi_flash_ptr_with_offset(get_recovery_region_offset(SPI_FLASH_PCH));
                        spi_breadcrumbs_rec = *m_flash_ptr;
                        *m_flash_ptr = 0xdeadcafe;
                    }
                    else if (spi_stat_read == 2)
                    {
                        *m_flash_ptr = spi_breadcrumbs_act;
                        m_flash_ptr = get_spi_flash_ptr_with_offset(get_recovery_region_offset(SPI_FLASH_PCH));
                        *m_flash_ptr = spi_breadcrumbs_rec;
                    }
                    spi_stat_read += 1;
                }
            }
        }
    });
}

static void ut_block_spi_read_access_once_during_afm_auth()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* spi_stat = SPI_CONTROL_1_CSR_BASE_ADDR + SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST;

            if (addr == spi_stat)
            {
                if (read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_BMC_FLASH_AUTHENTICATION)
                {
                    alt_u32* m_flash_ptr = get_spi_active_afm_ptr(SPI_FLASH_BMC);
                    if (spi_stat_read == 6)
                    {
                        spi_breadcrumbs_act = *m_flash_ptr;
                        *m_flash_ptr = 0xdeadcafe;
                        m_flash_ptr = get_spi_recovery_afm_ptr(SPI_FLASH_BMC);
                        spi_breadcrumbs_rec = *m_flash_ptr;
                        *m_flash_ptr = 0xdeadcafe;
                    }
                    else if (spi_stat_read == 8)
                    {
                        *m_flash_ptr = spi_breadcrumbs_act;
                        m_flash_ptr = get_spi_recovery_afm_ptr(SPI_FLASH_BMC);
                        *m_flash_ptr = spi_breadcrumbs_rec;
                    }
                    spi_stat_read += 1;
                }
            }
        }
    });
}

static void ut_block_spi_access_during_seamless_update()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* spi_stat = SPI_CONTROL_1_CSR_BASE_ADDR + SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST;

            if (addr == spi_stat)
            {
                if (read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_PCH_FV_SEAMLESS_UPDATE)
                {
                    SYSTEM_MOCK::get()->set_mem_word(spi_stat, 0x01, true);
                }
                else
                {
                    SYSTEM_MOCK::get()->set_mem_word(spi_stat, 0x00, true);
                }
            }
        }
    });
}

static void ut_block_spi_access_during_cpld_recovery()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* spi_stat = SPI_CONTROL_1_CSR_BASE_ADDR + SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST;

            if (addr == spi_stat)
            {
                if (read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_CPLD_RECOVERY_IN_RECOVERY_MODE)
                {
                    SYSTEM_MOCK::get()->set_mem_word(spi_stat, 0x01, true);
                }
                else
                {
                    SYSTEM_MOCK::get()->set_mem_word(spi_stat, 0x00, true);
                }
            }
        }
    });
}

static void ut_block_spi_access_during_active_update()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* spi_stat = SPI_CONTROL_1_CSR_BASE_ADDR + SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST;

            if (addr == spi_stat)
            {
                if ((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_PCH_FW_UPDATE) ||
                    (read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_BMC_FW_UPDATE))
                {
                    SYSTEM_MOCK::get()->set_mem_word(spi_stat, 0x01, true);
                }
                else
                {
                    SYSTEM_MOCK::get()->set_mem_word(spi_stat, 0x00, true);
                }
            }
        }
    });
}

static void ut_block_spi_access_during_recovery_update()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* spi_stat = SPI_CONTROL_1_CSR_BASE_ADDR + SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST;

            if (addr == spi_stat)
            {
                if (((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_PCH_FW_UPDATE) ||
                     (read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_BMC_FW_UPDATE)) &&
                     (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 2))
                {
                    SYSTEM_MOCK::get()->set_mem_word(spi_stat, 0x01, true);
                }
                else
                {
                    SYSTEM_MOCK::get()->set_mem_word(spi_stat, 0x00, true);
                }
            }
        }
    });
}

static void ut_block_spi_access_once_during_recovery_update()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* spi_stat = SPI_CONTROL_1_CSR_BASE_ADDR + SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST;

            if (addr == spi_stat)
            {
                if (((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_PCH_FW_UPDATE) ||
                     (read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_BMC_FW_UPDATE)) &&
                     (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 2))
                {
                    if (read_from_mailbox(MB_MINOR_ERROR_CODE) == MINOR_ERROR_AUTH_RECOVERY)
                    {

                    }
                    else
                    {
                        SYSTEM_MOCK::get()->set_mem_word(spi_stat, 0x01, true);
                    }
                }
                else
                {
                    SYSTEM_MOCK::get()->set_mem_word(spi_stat, 0x00, true);
                }
            }
        }
    });
}

#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)

static void ut_block_spi_access_once_during_afm_recovery_update()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* spi_stat = SPI_CONTROL_1_CSR_BASE_ADDR + SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST;

            if (addr == spi_stat)
            {
                if (((read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_PCH_FW_UPDATE) ||
                     (read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_BMC_FW_UPDATE)) &&
                     (read_from_mailbox(MB_PANIC_EVENT_COUNT) == 2))
                {
                    if (read_from_mailbox(MB_MINOR_ERROR_CODE) == MINOR_ERROR_AFM_AUTH_RECOVERY)
                    {

                    }
                    else
                    {
                        SYSTEM_MOCK::get()->set_mem_word(spi_stat, 0x01, true);
                    }
                }
                else
                {
                    SYSTEM_MOCK::get()->set_mem_word(spi_stat, 0x00, true);
                }
            }
        }
    });
}

#endif

static void ut_block_spi_access_during_bmc_auth()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* spi_stat = SPI_CONTROL_1_CSR_BASE_ADDR + SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST;

            if (addr == spi_stat)
            {
                if (read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_BMC_FLASH_AUTHENTICATION)
                {
                    SYSTEM_MOCK::get()->set_mem_word(spi_stat, 0x01, true);
                }
            }
        }
    });
}

static void ut_block_spi_access_during_pch_auth()
{
    SYSTEM_MOCK::get()->register_read_write_callback(
            [](SYSTEM_MOCK::READ_OR_WRITE read_or_write, void* addr, alt_u32 data) {
        if (read_or_write == SYSTEM_MOCK::READ_OR_WRITE::READ)
        {
            alt_u32* spi_stat = SPI_CONTROL_1_CSR_BASE_ADDR + SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST;

            if (addr == spi_stat)
            {
                if (read_from_mailbox(MB_PLATFORM_STATE) == PLATFORM_STATE_PCH_FLASH_AUTHENTICATION)
                {
                    SYSTEM_MOCK::get()->set_mem_word(spi_stat, 0x01, true);
                }
            }
        }
    });
}

static void ut_disable_watchdog_timers()
{
    wdt_enable_status = 0;
}

static void ut_reset_watchdog_timers()
{
    wdt_boot_status = 0;
    wdt_enable_status = WDT_ENABLE_ALL_TIMERS_MASK;

#if defined(GTEST_ATTEST_384) || defined(GTEST_ATTEST_256)
    wdt_attest_status = 0;
#endif
}

/***************************************
 *
 *  T0 / T-1 transition
 *
 ***************************************/
static alt_u32 ut_is_bmc_out_of_reset()
{
    return check_bit(U_GPO_1_ADDR, GPO_1_RST_SRST_BMC_PLD_R_N);
}

static alt_u32 ut_is_pch_out_of_reset()
{
    return check_bit(U_GPO_1_ADDR, GPO_1_RST_RSMRST_PLD_R_N);
}

static void ut_reset_fw_recovery_levels()
{
    reset_fw_recovery_level(SPI_FLASH_PCH);
    reset_fw_recovery_level(SPI_FLASH_BMC);
}

static void ut_reset_fw_spi_flash_state()
{
    bmc_flash_state = 0;
    pch_flash_state = 0;
}

static void ut_reset_nios_fw()
{
    ut_reset_watchdog_timers();
    clear_update_failure();
    ut_reset_fw_recovery_levels();
    ut_reset_fw_spi_flash_state();

    spi_breadcrumbs_act = 0;
    spi_breadcrumbs_rec = 0;
    spi_stat_read = 0;
}

/***************************************
 *
 *  Main
 *
 ***************************************/
static void ut_alter_key()
{
	SYSTEM_MOCK::get()->incr_flag();
}


static void ut_setup_for_recovery_main()
{
    ut_prep_nios_gpi_signals();
}

static void ut_setup_for_pfr_main()
{
    ut_prep_nios_gpi_signals();

    // Send checkpoint messages
    ut_send_block_complete_chkpt_msg();
}

/**
 * @brief This function runs pfr_main() or recovery_main() based on the
 * ConfigSelect register in the Dual-Config IP.
 */
static void ut_run_main(CPLD_CFM_TYPE_ENUM expect_cfm_one_or_zero, bool expect_throw)
{
    // Read ConfigSelect register; bit[1] represent CFM selection
    alt_u32 config_sel = IORD(U_DUAL_CONFIG_BASE, 1);

    if (config_sel & 0b10)
    {
        // CFM1: Active Image
        EXPECT_EQ(CPLD_CFM1, expect_cfm_one_or_zero);

        ut_setup_for_pfr_main();

        // Always run pfr_main with the timeout
        if (expect_throw)
        {
            ASSERT_DURATION_LE(500, EXPECT_ANY_THROW({ pfr_main(); }));
        }
        else
        {
            ASSERT_DURATION_LE(500, EXPECT_NO_THROW({ pfr_main(); }));
        }
    }
    else
    {
        // CFM0: ROM Image
        EXPECT_EQ(CPLD_CFM0, expect_cfm_one_or_zero);

        ut_setup_for_recovery_main();

        // Always run recovery_main with the timeout
        if (expect_throw)
        {
            ASSERT_DURATION_LE(500, EXPECT_ANY_THROW({ recovery_main(); }));
        }
        else
        {
            ASSERT_DURATION_LE(500, EXPECT_NO_THROW({ recovery_main(); }));
        }
    }
}

static void ut_debug_log_error(alt_u32 manifest_type, alt_u32 sig_type, alt_u32 image_type)
{
    char err[200];
    strcpy(err, "[  ERROR   ] ");

    if (manifest_type == UT_PFM)
    {
        if (sig_type == UT_MANIFEST_SIG)
        {
            char err_data[] = "Invalid PFM Signature: Check block 0 and block 1 chain of ";
            strcat(err, err_data);
        }
        else if (sig_type != UT_CAPSULE_SIG)
        {
            char err_data[] = "Invalid PFM Content: Check pfm rule set of ";
            strcat(err, err_data);
        }
    }

    if (manifest_type == UT_AFM)
    {
        if (sig_type == UT_MANIFEST_SIG)
        {
            char err_data[] = "Invalid AFM Signature: Check block 0 and block 1 chain of ";
            strcat(err, err_data);
        }
        else if (sig_type != UT_CAPSULE_SIG)
        {
            char err_data[] = "Invalid AFM Content: Check afm rule set of ";
            strcat(err, err_data);
        }
    }

    if (manifest_type == UT_FVM)
    {
        if (sig_type == UT_MANIFEST_SIG)
        {
            char err_data[] = "Invalid FVM Signature: Check block 0 and block 1 chain of ";
            strcat(err, err_data);
        }
        else if (sig_type != UT_CAPSULE_SIG)
        {
            char err_data[] = "Invalid FVM Content: Check fvm rule set of ";
            strcat(err, err_data);
        }
    }

    if (sig_type == UT_CAPSULE_SIG)
    {
        char err_data[] = "Invalid Capsule Signature: Check block 0 and block 1 chain of ";
        strcat(err, err_data);
    }

    if (image_type == UT_ACTIVE)
    {
        char err_data[] = "active image\n";
        strcat(err, err_data);
    }
    else if (image_type == UT_STAGING)
    {
        char err_data[] = "staging capsule\n";
        strcat(err, err_data);
    }
    else
    {
        char err_data[] = "recovery image\n";
        strcat(err, err_data);
    }

    GTEST_FATAL_FAILURE_(err);
}

#endif /* UNITTEST_SYSTEM_UT_NIOS_WRAPPER_H_ */
