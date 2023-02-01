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
 * @file spi_rw_utils.h
 * @brief Perform read/write/erase to the SPI flashes through SPI control block.
 */

#ifndef EAGLESTREAM_INC_SPI_RW_UTILS_H
#define EAGLESTREAM_INC_SPI_RW_UTILS_H

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "keychain_utils.h"
#include "pfr_pointers.h"
#include "spi_common.h"
#include "timer_utils.h"
#include "utils.h"

#define SPI_FLASH_PAGE_SIZE_OF_4KB 0x1000
#define SPI_FLASH_PAGE_SIZE_OF_64KB 0x10000

// This is a general timeout value obtained from analysis of datasheets
// In future when have more code space, can be extended to have different timeout for different chip
#ifdef USE_SYSTEM_MOCK
#define SPI_FLASH_ACCESS_TIMEOUT 0x00
#else
#define SPI_FLASH_ACCESS_TIMEOUT 0x1F4
#endif

static alt_u32 qe_tracker = 0;

/**
 * @brief Write to spi master
 *
 * @param offset : spi control cs address
 * @param data : data to wrtie
 *
 * @return none
 */
static PFR_ALT_INLINE void PFR_ALT_ALWAYS_INLINE write_to_spi_ctrl_1_csr(
        SPI_CONTROL_1_CSR_OFFSET_ENUM offset, alt_u32 data)
{
    // The CSR offset here is safe to cast to alt_u32
    // There is only a handful of these offsets and they are 1 apart.
    IOWR(SPI_CONTROL_1_CSR_BASE_ADDR, (alt_u32) offset, data);
}

/**
 * @brief Read from spi master
 *
 * @param offset : spi control cs address
 *
 * @return spi data
 */
static PFR_ALT_INLINE alt_u32 PFR_ALT_ALWAYS_INLINE read_from_spi_ctrl_1_csr(
        SPI_CONTROL_1_CSR_OFFSET_ENUM offset)
{
    // The CSR offset here is safe to cast to alt_u32
    // There is only a handful of these offsets and they are 1 apart.
    return IORD(SPI_CONTROL_1_CSR_BASE_ADDR, (alt_u32) offset);
}

/**
 * @brief Notify the SPI master to start sending the command.
 */
static PFR_ALT_INLINE void PFR_ALT_ALWAYS_INLINE trigger_spi_send_cmd()
{
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_CONTROL_OFST, 0x0001);
}

/**
 *  @brief Read the status register from the flash device
 */
static alt_u32 read_spi_chip_id()
{
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_OPERATING_PROTOCOLS_OFST, 0x22220);
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_SETTING_OFST, 0x3800 | SPI_CMD_READ_ID);
    trigger_spi_send_cmd();
    return read_from_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST);
}

/**
 *  @brief Read the status register from the flash device
 */
static alt_u32 read_spi_status_register()
{
#ifdef USE_QUAD_IO
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_OPERATING_PROTOCOLS_OFST, 0x22220);
#else
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_OPERATING_PROTOCOLS_OFST, 0x00000);
#endif
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_SETTING_OFST, 0x1800 | SPI_CMD_READ_STATUS_REG);
    trigger_spi_send_cmd();
    return read_from_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST);
}

/**
 *  @brief Reading the configuration register from the flash for Macronix chip only
 */
static alt_u32 read_spi_configuration_register()
{
#ifdef USE_QUAD_IO
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_OPERATING_PROTOCOLS_OFST, 0x22220);
#else
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_OPERATING_PROTOCOLS_OFST, 0x00000);
#endif
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_SETTING_OFST, 0x1800 | SPI_CMD_READ_CONFIGURATION_REG);
    trigger_spi_send_cmd();
    return read_from_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_READ_DATA0_OFST);
}

/**
 * @brief Send a simple one-byte command to the SPI master
 */
static void write_one_byte_spi_cmd(SPI_COMMAND_ENUM spi_cmd)
{
    // Settings: 'data type' to 1, 'number of data bytes' to 0,
    //   'number of address bytes' to 0, 'number of dummy cycles' to 0
    alt_u32 cmd_word = 0x00000800 | ((alt_u32) spi_cmd & 0x0000ffff);
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_SETTING_OFST, cmd_word);
}

/**
 * @brief Poll the status register and return when Write in Progress (WIP) bit becomes 1.
 */
static alt_u32 poll_status_reg_done()
{
    start_timer(U_TIMER_BANK_TIMER2_ADDR, SPI_FLASH_ACCESS_TIMEOUT);
    while (read_spi_status_register() & SPI_STATUS_WIP_BIT_MASK)
    {
        reset_hw_watchdog();
        if (is_timer_expired(U_TIMER_BANK_TIMER2_ADDR))
        {
            IOWR(U_TIMER_BANK_TIMER2_ADDR, 0, 0);
            return 0;
        }
    }
    IOWR(U_TIMER_BANK_TIMER2_ADDR, 0, 0);
    return 1;
}

/**
 * @brief Execute a SPI command (through SPI master) and wait for it to complete.
 */
static void execute_spi_cmd()
{
    trigger_spi_send_cmd();
    poll_status_reg_done();
}

/**
 * @brief Execute a simple one-byte command (through SPI master) and wait for it to complete.
 */
static void execute_one_byte_spi_cmd(SPI_COMMAND_ENUM spi_cmd)
{
    write_one_byte_spi_cmd(spi_cmd);
    execute_spi_cmd();
}

/**
 * @brief Configure Quad Enable bit for specific chip before translating to Quad IO mode
 *
 * @param qe_bit : Enable or disable QE
 *
 * @return none.
 */
static void configure_qe_bit(MACRONIX_QUAD_PROTOCOL_ENUM qe_bit)
{
    alt_u32 status_reg = read_spi_status_register();
    alt_u32 configuration_reg = read_spi_configuration_register();

    if (qe_bit == TRACK_QE_BIT)
    {
    	alt_u32 bit_val = status_reg;
    	bit_val &= (0b1 << 6);
    	bit_val = bit_val >> 6;
    	qe_tracker = (bit_val == SET_QE_BIT) ? SET_QE_BIT : UNSET_QE_BIT;
    	status_reg |= (0b1 << 6);
    }
    else if (qe_bit == SET_QE_BIT)
    {
        status_reg |= (0b1 << 6);
    }
    else if (qe_bit == UNSET_QE_BIT)
    {
    	status_reg &= ~(0b1 << 6);
    }
    else if (qe_bit == RESTORE_QE_BIT)
    {
        if (qe_tracker == SET_QE_BIT)
        {
        	status_reg |= (0b1 << 6);
        }
        else if (qe_tracker == UNSET_QE_BIT)
        {
        	status_reg &= ~(0b1 << 6);
        }
    }

    execute_one_byte_spi_cmd(SPI_CMD_WRITE_ENABLE);
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_SETTING_OFST, 0x2000 | SPI_CMD_WRITE_STATUS_AND_CONFIGURATION_REG);
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_WRITE_DATA0_OFST, status_reg | (configuration_reg << 8));
    execute_spi_cmd();
}

/**
 * @brief Allow CPLD to switch between the BMC flash and the PCH flash
 * Set GPO_1_SPI_MASTER_BMC_PCHN to 1 to have CPLD master talk to the BMC flash
 * Set to 0 to have CPLD master talk to the PCH Flash
 *
 * @param spi_flash_type indicates BMC or PCH flash
 */
static void switch_spi_flash(
        SPI_FLASH_TYPE_ENUM spi_flash_type)
{
#ifdef USE_QUAD_IO
    if (read_spi_chip_id() == SPI_FLASH_MACRONIX)
    {
        configure_qe_bit(UNSET_QE_BIT);
    }
#endif
    if (spi_flash_type == SPI_FLASH_BMC)
    {
        set_bit(U_GPO_1_ADDR, GPO_1_SPI_MASTER_BMC_PCHN);
    }
    else // spi_flash_type == SPI_FLASH_PCH
    {
        clear_bit(U_GPO_1_ADDR, GPO_1_SPI_MASTER_BMC_PCHN);
    }
#ifdef USE_QUAD_IO
    if (read_spi_chip_id() == SPI_FLASH_MICRON)
    {
        write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_READ_INSTRUCTION_OFST, SPI_FLASH_QUAD_READ_PROTOCOL_MICRON);
    }
    else if (read_spi_chip_id() == SPI_FLASH_MACRONIX)
    {
        configure_qe_bit(SET_QE_BIT);
        write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_READ_INSTRUCTION_OFST, SPI_FLASH_QUAD_READ_PROTOCOL_MACRONIX);
    }
#endif
}

/**
 * @brief Erase either a 4kB or 64kB sector of the flash.
 *
 * After this erase, you can write to addresses in this sector through the memory mapped interface.
 * Note that sector address is a FLASH address (starts at 0) not the AVMM address
 * (which starts at some base offset address).
 *
 * @param addr_in_flash an address in the target SPI flash
 * @param erase_cmd a SPI command for 4kB or 64kB erase. Nios expects one of SPI_CMD_4KB_SECTOR_ERASE | SPI_CMD_32KB_SECTOR_ERASE | SPI_CMD_64KB_SECTOR_ERASE.
 */
static void execute_spi_erase_cmd(alt_u32 addr_in_flash, SPI_COMMAND_ENUM erase_cmd)
{
    execute_one_byte_spi_cmd(SPI_CMD_WRITE_ENABLE);
    // Zero dummy cycles, zero data bytes, 4 address bytes, erase command
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_SETTING_OFST, 0x0400 | erase_cmd);
    // Set the FLASH Address Register with the address of the sector to erase
    write_to_spi_ctrl_1_csr(SPI_CONTROL_1_CSR_CS_FLASH_COMMAND_ADDRESS_OFST, addr_in_flash);
    // Execute the command and wait for it to finish
    execute_spi_cmd();
}

/**
 * @brief Erase the defined region on SPI flash device.
 * This function assumes that Nios has set the right muxes to talk to the device.
 * Also, the SPI region start and end addresses are expected to be 4KB aligned.
 *
 * @param region_start_addr Start address of the target SPI region
 * @param nbytes size (in byte) of the target SPI region
 */
static void erase_spi_region(alt_u32 region_start_addr, alt_u32 nbytes)
{
    alt_u32 spi_addr = region_start_addr;
    alt_u32 region_end_addr = region_start_addr + nbytes;
    while (spi_addr < region_end_addr)
    {
        if (((region_end_addr - spi_addr) >= SPI_FLASH_PAGE_SIZE_OF_64KB) && ((spi_addr & 0xFFFF) == 0))
        {
            // The target SPI Region is at least 64KB in size and aligns to 0x10000 boundary.
            // Send 64KB erase command
            execute_spi_erase_cmd(spi_addr, SPI_CMD_64KB_SECTOR_ERASE);
            spi_addr += SPI_FLASH_PAGE_SIZE_OF_64KB;
        }
        else
        {
            // Otherwise, send 4KB erase command
            execute_spi_erase_cmd(spi_addr, SPI_CMD_4KB_SECTOR_ERASE);
            spi_addr += SPI_FLASH_PAGE_SIZE_OF_4KB;
        }
    }
}

/**
 * @brief Check writing to SPI Flash is successful.
 *
 * @param dest_ptr Pointer to dest address of the target SPI region
 * @param dest_size (in byte) of the target SPI region
 * @param src_ptr Pointer to source address of the target SPI region
 * @param nbytes size (in byte) of the target SPI region
 */
static alt_u32 memcpy_s_flash_successful(alt_u32* dest_ptr, const alt_u32 dest_size,
                                         const alt_u32* src_ptr, alt_u32 n_bytes)
{
    alt_u32_memcpy_s(dest_ptr, dest_size, src_ptr, n_bytes);
    if (!poll_status_reg_done())
    {
        return 0;
    }
    return 1;
}

/**
 * @brief Use custom memcpy to copy the signed payload to a given destination address.
 *
 * @param spi_dest_addr pointer to destination address in the SPI address range
 * @param signed_payload pointer to the start address of the signed payload
 */
static alt_u32 memcpy_signed_payload(alt_u32 spi_dest_addr, alt_u32* signed_payload)
{
    alt_u32 nbytes = get_signed_payload_size(signed_payload);

    // Erase destination area first
    erase_spi_region(spi_dest_addr, nbytes);

    // Copy entire signed payload to destination location
    // Largest signed payload is BMC firmware capsule (max 32 MB).
    // Always copy signed payload in four pieces and pet the HW timer in between.
    // Then CPLD HW timer most likely won't timeout in this function, based on platform testing results.
    alt_u32 quarter_nbytes = nbytes >> 2;

    for (alt_u32 i = 0; i < 4; i++)
    {
        alt_u32_memcpy(get_spi_flash_ptr_with_offset(spi_dest_addr),
                       incr_alt_u32_ptr(signed_payload, i * quarter_nbytes),
                       quarter_nbytes);
        if (!poll_status_reg_done())
        {
            return 0;
        }
        spi_dest_addr += quarter_nbytes;

        // Pet CPLD HW timer
        reset_hw_watchdog();
    }
    return 1;
}

/**
 * @brief Copy a binary blob from one SPI flash to the other.
 *
 * It's assumed that size has been authenticated, as part of the update capsule
 * authentication, before it's used. Then, it's guaranteed that (dest_spi_addr + size)
 * and (src_spi_addr + size) will not overflow the designated region on the SPI flash.
 *
 * @param dest_spi_addr an address in the destination SPI flash
 * @param src_spi_addr an address in the destination SPI flash
 * @param dest_spi_type destination SPI flash: BMC or PCH SPI flash
 * @param src_spi_type source SPI flash: BMC or PCH SPI flash
 * @param size number of bytes to copy
 */
static void copy_between_flashes(alt_u32 dest_spi_addr, alt_u32 src_spi_addr,
        SPI_FLASH_TYPE_ENUM dest_spi_type, SPI_FLASH_TYPE_ENUM src_spi_type, alt_u32 size)
{
    switch_spi_flash(dest_spi_type);
    // Get a pointer to the destination SPI flash at offset dest_spi_addr
    alt_u32* dest_flash_ptr = get_spi_flash_ptr_with_offset(dest_spi_addr);

    // Erase destination SPI region
    erase_spi_region(dest_spi_addr, size);

    // Get a pointer to the source SPI flash at offset src_spi_addr
    switch_spi_flash(src_spi_type);
    alt_u32* src_flash_ptr = get_spi_flash_ptr_with_offset(src_spi_addr);

    // Copy the binary like this: BMC -> CPLD, CPLD -> PCH
    for (alt_u32 word_i = 0; word_i < (size / 4); word_i++)
    {
        // Read a word from source SPI flash
        alt_u32 tmp = src_flash_ptr[word_i];
        switch_spi_flash(dest_spi_type);

        // Write the word to destination SPI flash
        dest_flash_ptr[word_i] = tmp;
        if (!poll_status_reg_done())
        {
            return;
        }
        switch_spi_flash(src_spi_type);
    }

    // Pet the HW watchdog
    reset_hw_watchdog();
}

#endif /* EAGLESTREAM_INC_SPI_RW_UTILS_H */
