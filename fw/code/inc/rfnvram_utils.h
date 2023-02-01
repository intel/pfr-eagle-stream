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
 * @file rfnvram_utils.h
 * @brief Utility functions to communicate with RFNVRAM IP.
 */

#ifndef EAGLESTREAM_INC_RFNVRAM_UTILS_H_
#define EAGLESTREAM_INC_RFNVRAM_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "pfr_pointers.h"
#include "rfnvram.h"
#include "timer_utils.h"

/**
 * @brief This function reads SMBus/i2c message from the smbus master
 *
 * @param read_data : Message to read
 * @param offset : addresss to read from
 * @param nbytes : message size
 *
 * @return none
 */
static void read_from_rfnvram(alt_u8* read_data, alt_u32 offset, alt_u32 nbytes)
{
    // Start the sequence (with write)
    // Lowest bit: High -> Read ; Low -> Write
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, 0x2DC);

    // Write most significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, ((offset & 0xFF00) >> 8));
    // Write least significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, (offset & 0xFF));

    // Start to read
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, 0x2DD);

    alt_u32 write_word = 0x00;
    for (alt_u32 i = 0; i < nbytes; i++)
    {
        // Read another word
        if (i == nbytes - 1)
        {
            // Also put RFNVRAM to idle at the last read
            write_word |= RFNVRAM_IDLE_MASK;
        }
        IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, write_word);

        // Poll to get data
        while (IORD_32DIRECT(RFNVRAM_RX_FIFO_BYTES_LEFT, 0) == 0) {}

        // Read and save the word
        read_data[i] = IORD_32DIRECT(RFNVRAM_RX_FIFO, 0);
    }
}

/**
 * @brief This function sets the DCI_RF_EN bit in the RF access control register in the RFNVRAM.
 *
 * Nios can read 1 byte starting from any byte address, but write must align on word boundaries (write 2 bytes/4 bytes).
 * Before setting the DCI_RF_EN bit, Nios also reads in the existing register value first and modifies based on that value.
 */
static void enable_rf_access()
{
    // Read in the existing value from the RF Access Control register
    alt_u8 rf_access_control[RFNVRAM_RF_ACCESS_CONTROL_LENGTH] = {};
    read_from_rfnvram(rf_access_control, RFNVRAM_RF_ACCESS_CONTROL_OFFSET, RFNVRAM_RF_ACCESS_CONTROL_LENGTH);

    // Modify and write back after setting the DCI_RF_EN bit.
    // Lowest bit: High -> Read ; Low -> Write
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, 0x2DC);
    // Write most significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_RF_ACCESS_CONTROL_MSB_OFFSET);
    // Write least significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_RF_ACCESS_CONTROL_LSB_OFFSET);

    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, rf_access_control[0]);
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, (rf_access_control[1] | RFNVRAM_IDLE_MASK | RFNVRAM_RF_ACCESS_CONTROL_DCI_RF_EN_MASK));
}

/**
 * @brief This function writes the PIT ID at a pre-defined address in the RFNVRAM.
 * RFNVRAM expects 4 data byte at a time. Nios has to write the 8-byte PIT ID in two chunk.
 */
static void write_ufm_pit_id_to_rfnvram()
{
    // Get the PIT ID from UFM and write it to RFNVRAM
    alt_u8* ufm_pit_id = (alt_u8*) get_ufm_pfr_data()->pit_id;

    // Always do a soft reset before configuration
    IOWR_32DIRECT(RFNVRAM_MCTP_CONTROL, 0, RFVNRAM_MCTP_CONTROL_SMBUS_MASTER_RESET);
    // Configure to port ID 0 for rf-nvram usage
    IOWR_32DIRECT(RFNVRAM_MCTP_SMBUS_SLAVE_PORT_ID, 0, RFNVRAM_CHIP_SMBUS_PORT);
    // Write 4 data byte at a time

    // Start to write the first half of PIT ID
    // Lowest bit: High -> Read ; Low -> Write
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, 0x2DC);

    // Write most significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_PIT_ID_MSB_OFFSET);
    // Write least significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_PIT_ID_LSB_OFFSET);

    // Write 4 data byte
    // (Using for-loop here costs more space)
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, ufm_pit_id[0]);
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, ufm_pit_id[1]);
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, ufm_pit_id[2]);
    // Write the last byte and put RFNVRAM to idle
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_IDLE_MASK | ufm_pit_id[3]);

    // In RFNVRAM, write operation can take 4.7-5ms. During this time,
    // it will ignore any command. There's also no busy bit to pull in the IP.
    // Sleep for 20ms to be safe.
    sleep_20ms(1);

    // Now, RFNVRAM should be ready to accept the next write
    // Start to write the second half of PIT ID
    // Lowest bit: High -> Read ; Low -> Write
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, 0x2DC);

    // Write most significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_PIT_ID_MSB_OFFSET);
    // Write least significant byte
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_PIT_ID_LSB_OFFSET + 4);

    // Write 4 data byte
    // (Using for-loop here costs more space)
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, ufm_pit_id[4]);
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, ufm_pit_id[5]);
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, ufm_pit_id[6]);
    // Write the last byte and put RFNVRAM to idle
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_IDLE_MASK | ufm_pit_id[RFNVRAM_PIT_ID_LENGTH - 1]);

    // In RFNVRAM, write operation can take 4.7-5ms. During this time,
    // it will ignore any command. There's also no busy bit to pull in the IP.
    // Sleep for 20ms to be safe.
    sleep_20ms(1);

    // Enable RF access
    enable_rf_access();
}

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/**
 * @brief This function polls for smbus master status bit before proceeding to send message over SMBus
 *
 * @return 1 if SMBus master busy; else return 0
 */
static alt_u32 poll_for_smbus_master_status_bit()
{
    // Wait for 40ms
    start_timer(U_TIMER_BANK_TIMER4_ADDR, 2);
    while((IORD_32DIRECT(RFNVRAM_MCTP_STATUS, 0) & 1) && !(is_timer_expired(U_TIMER_BANK_TIMER4_ADDR)))
    {
        reset_hw_watchdog();
    }
    return is_timer_expired(U_TIMER_BANK_TIMER4_ADDR);
}

/**
 * @brief This function writes MCTP + SPDM message to the SMBus Master.
 *
 * @param mctp_pkt_ptr : Message to send
 * @param pkt_size : Message size
 * @param dest_addr : Destination address of host
 * @param challenger : Hosts
 *
 * @return 1 if write to SMBus master fail; else return 0
 */
static alt_u32 write_mctp_pkt_to_rfnvram(alt_u8* mctp_pkt_ptr, alt_u32 pkt_size, alt_u32 dest_addr, ATTESTATION_HOST challenger)
{
    alt_u32 smbus_slave_port_id = 0;

    alt_u32 past_time = IORD(U_TIMER_BANK_TIMER4_ADDR, 0);
    // Poll for status bit to make sure FSM is idle
    if (poll_for_smbus_master_status_bit())
    {
        start_timer(U_TIMER_BANK_TIMER4_ADDR, past_time);
        return 1;
    }
    start_timer(U_TIMER_BANK_TIMER4_ADDR, past_time);

    // There are 3 challengers BMC, PCH and PCIE
    switch (challenger)
    {
        case BMC: smbus_slave_port_id = BMC_SMBUS_PORT;
                  break;

        case PCH: smbus_slave_port_id = PCH_SMBUS_PORT;
                  break;

        // PCIE
        default : smbus_slave_port_id = PCIE_SMBUS_PORT;
                  break;
    }

    IOWR_32DIRECT(RFNVRAM_MCTP_SMBUS_SLAVE_PORT_ID, 0, smbus_slave_port_id);

    IOWR_32DIRECT(RFNVRAM_MCTP_CONTROL, 0, RFVNRAM_MCTP_CONTROL_PEC_ENABLED);

    // Lowest bit: High -> Read ; Low -> Write
    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, (0x200 | (dest_addr << 1)));

    for (alt_u32 byte = 1; byte < pkt_size - 1; byte++)
    {
        IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, mctp_pkt_ptr[byte]);
    }

    IOWR_32DIRECT(U_RFNVRAM_SMBUS_MASTER_ADDR, 0, RFNVRAM_IDLE_MASK | mctp_pkt_ptr[pkt_size - 1]);

    return 0;
}


#endif

#endif /* EAGLESTREAM_INC_RFNVRAM_UTILS_H_ */
