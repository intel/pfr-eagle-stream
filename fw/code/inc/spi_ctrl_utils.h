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
 * @file spi_ctrl_utils.h
 * @brief Utility functions to communicate with the SPI control block.
 */

#ifndef EAGLESTREAM_INC_SPI_CTRL_UTILS_H
#define EAGLESTREAM_INC_SPI_CTRL_UTILS_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "gen_gpo_controls.h"
#include "spi_common.h"
#include "spi_rw_utils.h"
#include "timer_utils.h"
#include "ufm_utils.h"
#include "utils.h"

/**
 * @brief Reset a SPI flash through toggling the @p spi_flash_rst_n_gpo GPO control bit.
 * Note that this reset will bring the SPI flash device back to 3-byte addressing mode.
 *
 * @param spi_flash_rst_n_gpo : BMC or PCH
 *
 * @return none
 */
static void reset_spi_flash(alt_u32 spi_flash_rst_n_gpo)
{
    clear_bit(U_GPO_1_ADDR, spi_flash_rst_n_gpo);
    set_bit(U_GPO_1_ADDR, spi_flash_rst_n_gpo);
    sleep_20ms(1);
}

/**
 * @brief Prepare for CPLD SPI master to drive the indicated SPI bus.
 * This function sets the appropriate GPO control bit and sends a command
 * to request flash device to enter 4-byte addressing mode.
 *
 * The assumption here is that the external agent (e.g. BMC/PCH) is in reset.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 *
 * @return none
 */
static void takeover_spi_ctrl(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        // BMC SPI Mux. 0-BMC, 1-CPLD
        set_bit(U_GPO_1_ADDR, GPO_1_FM_SPI_PFR_BMC_BT_MASTER_SEL);

        // Reset the BMC SPI flash to power-on state
        reset_spi_flash(GPO_1_RST_SPI_PFR_BMC_BOOT_N);
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        // PCH SPI Mux. 0-PCH, 1-CPLD
        set_bit(U_GPO_1_ADDR, GPO_1_FM_SPI_PFR_PCH_MASTER_SEL);

        // Reset the PCH SPI flash to power-on state
        reset_spi_flash(GPO_1_RST_SPI_PFR_PCH_N);
    }

    switch_spi_flash(spi_flash_type);
#ifdef USE_QUAD_IO
    if (read_spi_chip_id() == SPI_FLASH_MICRON)
    {
        write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_OPERATING_PROTOCOLS_OFST, 0x00022220);
        write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_WRITE_INSTRUCTION_OFST, 0x00000538);
    }
    else if (read_spi_chip_id() == SPI_FLASH_MACRONIX)
    {
        write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_OPERATING_PROTOCOLS_OFST, 0x00022220);
        write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_WRITE_INSTRUCTION_OFST, 0x00000538);
        configure_qe_bit(TRACK_QE_BIT);
    }
#endif

    // Send the command to put the flash device in 4-byte address mode
    // Write enable command is required prior to sending the enter/exit 4-byte mode commands
    execute_one_byte_spi_cmd(SPI_CMD_WRITE_ENABLE);
    execute_one_byte_spi_cmd(SPI_CMD_ENTER_4B_ADDR_MODE);
}

/**
 * @brief Clear the external SPI Mux to let BMC/PCH drive the SPI bus.
 * For BMC, this function clears FM_SPI_PFR_BMC_BT_MASTER_SEL mux.
 * For PCH, this function clears FM_SPI_PFR_PCH_MASTER_SEL mux.
 *
 * This function should be ran before releasing BMC/PCH from reset.
 *
 * @param spi_flash_type indicates BMC or PCH flash
 *
 * @return none
 */
static void release_spi_ctrl(SPI_FLASH_TYPE_ENUM spi_flash_type)
{
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        // BMC SPI Mux. 0-BMC, 1-CPLD
        clear_bit(U_GPO_1_ADDR, GPO_1_FM_SPI_PFR_BMC_BT_MASTER_SEL);
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        // PCH SPI Mux. 0-PCH, 1-CPLD
        clear_bit(U_GPO_1_ADDR, GPO_1_FM_SPI_PFR_PCH_MASTER_SEL);
    }
}

/**
 * @brief Let CPLD SPI master drive the SPI busses on the platform.
 *
 * @return none
 */
static void takeover_spi_ctrls()
{
    takeover_spi_ctrl(SPI_FLASH_BMC);
    takeover_spi_ctrl(SPI_FLASH_PCH);
}

/**
 * @brief configure SPI master IP
 * This function must be called before trying to do any other SPI commands (including memory
 * reads/writes) After calling this, you should be set to do normal reads from the memory mapped
 * windows for each flash device
 */
static void configure_spi_master_csr()
{
    // Use standard SPI mode (command sent on DQ0 only)
    //   may want to change this to use 4 bits for address/data
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_OPERATING_PROTOCOLS_OFST, 0x00000000);
    // Write instruction opcode set to 0x05, write operation opcode set to 0x02 - these may change
    // for different FLASH devices
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_WRITE_INSTRUCTION_OFST, 0x00000502);
    // Set dummy cycles to 0 - SOME COMMANDS HAVE DIFFERENT NUMBER OF DUMMY CYCLES
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_READ_INSTRUCTION_OFST, 0x00000003);
}

/**
 * @brief Return the pointer to SPI Filter Write Enable (WE) memory, based on the SPI_MASTER_BMC_PCHN mux.
 *
 * If SPI_MASTER_BMC_PCHN is set to 1, meaning CPLD is talking to BMC, return the pointer to WE memory in BMC SPI filter.
 * If SPI_MASTER_BMC_PCHN is set to 0, meaning CPLD is talking to PCH, return the pointer to WE memory in PCH SPI filter.
 *
 * @return pointer to start of SPI Filter Write Enable memory for the connecting SPI flash.
 * @see switch_spi_flash
 */
static alt_u32* get_spi_we_mem_ptr()
{
    if (check_bit(U_GPO_1_ADDR, GPO_1_SPI_MASTER_BMC_PCHN))
    {
        // Return pointer to WE memory in BMC SPI filter
        return __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_SPI_FILTER_BMC_WE_AVMM_BRIDGE_BASE, 0);
    }
    // Return pointer to WE memory in PCH SPI filter
    return __IO_CALC_ADDRESS_NATIVE_ALT_U32(U_SPI_FILTER_PCH_WE_AVMM_BRIDGE_BASE, 0);
}

/**
 * @brief Set up write protection in the SPI filter for the given SPI region.
 *
 * In each SPI filter, there's a Write Enable Memory, where the write protection rules are
 * stored. Each bit represents 4KB page. If bit[0] == 1, the first page is writable. If
 * bit[1] == 0, writes from BMC or PCH to the first page of its SPI flash is blocked.
 *
 * @param start_addr start address of the SPI region
 * @param end_addr end address of the SPI region
 * @param is_write_allowed 0 if this SPI region is write-protected; otherwise, allow write on this SPI region
 */
static void apply_spi_write_protection(alt_u32 start_addr, alt_u32 end_addr, alt_u32 is_write_allowed)
{
    alt_u32* we_mem = get_spi_we_mem_ptr();

    // 12 bits to get to 4kB chunks, 5 more bits because we have 32 (=2^5) bits in each word in the memory
    alt_u32 start_word_pos = start_addr >> (12 + 5);
    alt_u32 end_word_pos = end_addr >> (12 + 5);

    // Grab the 5 address bits that tell us the starting bit position within the 32 bit word
    alt_u32 start_bit_pos = (start_addr >> 12) & 0x0000001f;
    alt_u32 end_bit_pos = (end_addr >> 12) & 0x0000001f;

    // Writing to a word higher than the end address word will cause unintended writes as
    // this will not work if address spi address definition is not expected to be incremental
    if (end_bit_pos == 0)
    {
        // Only deduct when address range not less than 4kB
        // There is no control to end address set in PFM and FVM once we trust the user.
        if (end_word_pos)
        {
            end_word_pos--;
        }
    }

    // Get the existing rules on the first and last word
    alt_u32 orig_rule_in_start_word = IORD(we_mem, start_word_pos);
    alt_u32 orig_rule_in_end_word = IORD(we_mem, end_word_pos);

    // In WE memory, a 0 means that write is restricted on the 4KB page. On the other hand,
    // 1 means that write is allowed on the 4KB page.
    alt_u32 write_rule = 0;
    if (is_write_allowed)
    {
        write_rule = 0xFFFFFFFF;
    }

    // Apply the new SPI write protection rules on all the words this SPI region occupies in WE memory
    for (alt_u32 word_pos = start_word_pos; word_pos <= end_word_pos; word_pos++)
    {
        IOWR(we_mem, word_pos, write_rule);
    }

    if (start_bit_pos != 0)
    {
        // Bits representing this SPI region only occupies part of a WE memory word.
        // Combine the original rules and new rules in this word in WE memory
        alt_u32 bit_mask = 0xFFFFFFFF << start_bit_pos;
        orig_rule_in_start_word &= ~bit_mask;
        alt_u32 combined_rule = orig_rule_in_start_word | (write_rule & bit_mask);

        IOWR(we_mem, start_word_pos, combined_rule);
    }

    if (end_bit_pos != 0)
    {
        // Bits representing this SPI region only occupies part of a WE memory word.
        // Read from WE memory again, in case the rule is modified again
        // in the above if-block (i.e. start word is also end word)
        alt_u32 read_rule = IORD(we_mem, end_word_pos);

        // Combine the new rules and original rules in this word in WE memory
        alt_u32 bit_mask = 0xFFFFFFFF << end_bit_pos;
        read_rule &= ~bit_mask;
        orig_rule_in_end_word &= bit_mask;
        alt_u32 combined_rule = read_rule | orig_rule_in_end_word;

        IOWR(we_mem, end_word_pos, combined_rule);
    }
}

/**
 * @brief Set the CPLD recovery region to read only.
 *
 * Nios should not rely on BMC PFM to mark this SPI region with correct permission.
 */
static void write_protect_cpld_recovery_region()
{
    switch_spi_flash(SPI_FLASH_BMC);
    apply_spi_write_protection(
            BMC_CPLD_RECOVERY_IMAGE_OFFSET,
            BMC_CPLD_RECOVERY_IMAGE_OFFSET + BMC_CPLD_RECOVERY_REGION_SIZE,
            0
    );
}

/**
 * @brief Set the CPLD staging region to read only.
 */
static void write_protect_cpld_staging_region()
{
    switch_spi_flash(SPI_FLASH_BMC);
    apply_spi_write_protection(
            get_staging_region_offset(SPI_FLASH_BMC) + BMC_STAGING_REGION_CPLD_UPDATE_CAPSULE_OFFSET,
            BMC_CPLD_RECOVERY_IMAGE_OFFSET,
            0
    );
}

#endif /* EAGLESTREAM_INC_SPI_CTRL_UTILS_H */
