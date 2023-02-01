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
 * @file mctp_utils.h
 * @brief Utility functions to handle MCTP message transaction.
 */

#ifndef EAGLESTREAM_INC_MCTP_UTILS_H_
#define EAGLESTREAM_INC_MCTP_UTILS_H_

// Always include pfr_sys.h first
#include "pfr_sys.h"

#include "rfnvram_utils.h"
#include "mailbox_utils.h"
#include "mctp.h"

#if defined(ENABLE_ATTESTATION_WITH_ECDSA_384) || defined(ENABLE_ATTESTATION_WITH_ECDSA_256)

/*
 * @brief This function initializes CPLD's own MCTP context.
 *
 * Initializes context such as message type, source address...
 *
 * @param mctp_pkt_header : MCTP packet header
 *
 * @return none.
 */
/*static void init_mctp(MCTP_HEADER* mctp_pkt_header)
{
    mctp_pkt_header->field.command_code = MCTP_COMMAND_CODE;
    // Set TO to 1 to indicate the message is from origin according to MCTP specs
    // TO are set dynamically in SPDM layer for CPLD to distinguish them
    mctp_pkt_header->msg_type = MCTP_MESSAGE_TYPE;
    mctp_pkt_header->source_addr = SRC_SLAVE_ADDR_WR;
}*/

/**
 * @brief This function read MCTP packet from the mailbox fifo associated to the particular
 * mctp-capable hosts.
 * This function reads the mctp packets from the mailbox fifo one byte at a time
 *
 * @param challenger : 3 MCTP-capable hosts either BMC, PCH or PCIE
 * @param mctp_stored_packet : mctp storage location
 * @param pkt_size : Size of the mctp packet in bytes
 *
 * @return none
 */
static void read_mctp_pkt_from_mb(MB_REGFILE_OFFSET_ENUM challenger, alt_u8* mctp_stored_packet, alt_u32 pkt_size)
{
    for (alt_u32 byte = 0; byte < pkt_size; byte++)
    {
    	mctp_stored_packet[byte] = read_from_mailbox(challenger);
    }
}

/**
 * @brief This function process the received mctp packets (containing SPDM message) from the mailbox fifo.
 * This function reads the mctp packets from the mailbox fifo one byte at a time
 *
 * This function will remove the MCTP header from the message and relay only the SPDM message to the
 * upper SPDM handler layer. Also decode the message size (without the MCTP packet). If a message spans more
 * than 1 mctp packet, CPLD will remove the headers from all the MCTP messages and append their bodies to
 * form a complete SPDM message,
 *
 * @param processed_mctp_pkt : message containing only spdm
 * @param mctp_stored_packet_ptr : spdm + mctp
 * @param mctp_pkt_size_collection : collection of individual mctp packets size
 * @param mctp_read_pkt_size : Size of the mctp packet in bytes
 *
 * @return size of message without MCTP headers
 */
static alt_u32 receive_and_disassemble_mctp_pkt(alt_u8* processed_mctp_pkt, alt_u8* mctp_stored_packet_ptr,
                                                alt_u8* mctp_pkt_size_collection, alt_u32 mctp_read_pkt_size)
{
    alt_u32 byte_idx = 0;
    alt_u32 start_idx = 0;
    alt_u32 received_pkt_size = sizeof(MCTP_HEADER) - sizeof(MCTP_HEADER_FILTERED);
    alt_u32 raw_pkt_index = received_pkt_size;
    alt_u32 full_pkt_size = mctp_read_pkt_size;

    while (mctp_read_pkt_size)
    {
        reset_hw_watchdog();
        // Append the message data
        // Remove MCTP header
        for (alt_u8 byte = 0; byte < (mctp_pkt_size_collection[byte_idx] - received_pkt_size); byte++)
        {
            processed_mctp_pkt[start_idx + byte] = mctp_stored_packet_ptr[raw_pkt_index + byte];
        }

        // Size handling
        raw_pkt_index += mctp_pkt_size_collection[byte_idx];
        start_idx += (mctp_pkt_size_collection[byte_idx] - received_pkt_size);
        mctp_read_pkt_size -= mctp_pkt_size_collection[byte_idx];

        // Handle appended message data size
        full_pkt_size -= received_pkt_size;
        byte_idx++;
    }
    return full_pkt_size;
}

/**
 * @brief This function process the SPDM layer messages and attach MCTP header onto the SPDM message.
 *
 * This function handles the byte count, packet info and other fields as defined in MCTP protocol.
 * SPDM over MCTP has message type 05h. After the SPDM message is processed, CPLD sends the processed
 * message to the SMBus Master. This function also handles messages which cannot fit into one MCTP packet.
 * This function will divided them into several MCTP chunks (packets) and sends them at once.
 *
 * @param mctp_ctx : mctp context
 * @param dest_ptr : message to be processed
 * @param payload_size : packet size
 *
 * @return none
 */
static void assemble_and_send_mctp_pkt(MCTP_CONTEXT* mctp_ctx, alt_u8* dest_ptr, alt_u32 payload_size)
{
    alt_u32 pkt_size = 0;
    alt_u32 max_fifo_size = 0;
    alt_u8 max_byte_count_multiple = 0;
    alt_u8 mctp_pkt_to_dispatch[258] = {0};
    alt_u8 dest_addr = 0;
    alt_u8 mctp_header_size = 0;

    // Initialise the MCTP header with common values
    MCTP_HEADER mctp_pkt_header;
    MCTP_HEADER* mctp_pkt_header_ptr = (MCTP_HEADER*)&mctp_pkt_header;
    reset_buffer((alt_u8*)mctp_pkt_header_ptr, sizeof(MCTP_HEADER));

    max_fifo_size = MAX_BYTE_COUNT;
    mctp_header_size = sizeof(MCTP_HEADER);
    dest_addr = mctp_ctx->cached_addr;

    mctp_pkt_header_ptr->field.command_code = MCTP_COMMAND_CODE;
    mctp_pkt_header_ptr->field.dest_addr = dest_addr;
    mctp_pkt_header_ptr->source_addr = SRC_SLAVE_ADDR_PCIE;

    if (mctp_ctx->challenger_type == BMC)
    {
        mctp_pkt_header_ptr->source_addr = SRC_SLAVE_ADDR_WR;
    }

    // Set TO to 1 to indicate the message is from origin according to MCTP specs
    // TO are set dynamically in SPDM layer for CPLD to distinguish them
    // MCTP header version
    mctp_pkt_header_ptr->transport_header.hdr_version = MCTP_HDR_VER;
    mctp_pkt_header_ptr->transport_header.msg_type = mctp_ctx->msg_type;
    mctp_pkt_header_ptr->transport_header.dest_eid = mctp_ctx->cached_dest_eid;
    mctp_pkt_header_ptr->transport_header.src_eid = mctp_ctx->cached_cpld_eid;
    mctp_pkt_header_ptr->transport_header.pkt_info = FIRST_AND_LAST_PKT | (mctp_ctx->cached_pkt_info & ~PKT_SEQ_NO_MSK);

    // Always do a soft reset on SMBus master IP before configuring them
    // It takes only a single clock cycle for IP to retrieve this hence it is ok.
    // Anyhow, Nios still poll for it to be ready
    IOWR_32DIRECT(RFNVRAM_MCTP_CONTROL, 0, RFVNRAM_MCTP_CONTROL_SMBUS_MASTER_RESET);

    // Divides the SPDM message into multiple packets
    // Handles the SOM and EOM in the MCTP header in each packet
    // Write packet by packet to the SMBus master.
    // Example: When a SPDM message is split into three packets:
    //          Packet 1 (SOM high) > Packet 2 (SOM/EOM low) > Packet 3 (EOM high)
    while (payload_size)
    {
        if (payload_size > max_fifo_size)
        {
            if ((mctp_pkt_header_ptr->transport_header.pkt_info & FIRST_AND_LAST_PKT) == FIRST_AND_LAST_PKT)
            {
                mctp_pkt_header_ptr->transport_header.pkt_info &= ~LAST_PKT;
            }
            else if (mctp_pkt_header_ptr->transport_header.pkt_info & FIRST_PKT)
            {
                mctp_pkt_header_ptr->transport_header.pkt_info &= ~FIRST_AND_LAST_PKT;
            }
            mctp_pkt_header_ptr->field.byte_count = max_fifo_size + MCTP_HEADER_AFTER_BYTE_COUNT_SIZE;
            payload_size -= max_fifo_size;
            pkt_size = max_fifo_size;
        }
        else
        {
            if ((mctp_pkt_header_ptr->transport_header.pkt_info & FIRST_AND_LAST_PKT) != FIRST_AND_LAST_PKT)
            {
                mctp_pkt_header_ptr->transport_header.pkt_info &= ~FIRST_PKT;
                mctp_pkt_header_ptr->transport_header.pkt_info |= LAST_PKT;
            }
            mctp_pkt_header_ptr->field.byte_count = payload_size + MCTP_HEADER_AFTER_BYTE_COUNT_SIZE;

            pkt_size = payload_size;
            payload_size = 0;
        }

        // Attach the MCTP packet header onto the SPDM message
        alt_u8_memcpy(mctp_pkt_to_dispatch, (alt_u8*)mctp_pkt_header_ptr, mctp_header_size);

        // Append the SPDM message to the back of the MCTP packet header
        for (alt_u8 byte = 0; byte < pkt_size; byte++)
        {
            mctp_pkt_to_dispatch[byte + mctp_header_size] = dest_ptr[max_byte_count_multiple*max_fifo_size + byte];
        }

        alt_u32 total_msg_size = mctp_header_size + pkt_size;

        // Write to SMBus Master the MCTP packet (containing SPDM message)
            if (write_mctp_pkt_to_rfnvram((alt_u8*) mctp_pkt_to_dispatch, total_msg_size, dest_addr,
            		                      (ATTESTATION_HOST)mctp_ctx->challenger_type))
        {
            // Write transaction failed
            mctp_ctx->error_type = 1;
            break;
        }

        // Increase the packet sequence by modulo 4
        alt_u8 packet_info_seq_num = (((mctp_pkt_header_ptr->transport_header.pkt_info & PKT_SEQ_NO_MSK) >> 4) + 1) % MAX_MCTP_I2C_PKT_IDX;
        mctp_pkt_header_ptr->transport_header.pkt_info = (mctp_pkt_header_ptr->transport_header.pkt_info & (~PKT_SEQ_NO_MSK)) |
                                        (packet_info_seq_num << 4);
        max_byte_count_multiple++;
    }
}

/**
 * @brief This function process the MCTP packets to check their validity.
 * There are multiple scenarios where a MCTP packet is erroneous
 * 1)    SOM is not set to '1' for first mctp packet
 * 2)    SOM is set to '1' again after the previous packet has SOM set to '1'
 * 3)    SOM and EOM is set to '1' after SOM is already set to '1' for previous packet
 * 4)    EOM is set to '1' without SOM being set to '1' for the packet
 * 5)    Intermediary packet is sent without SOM being set to '1'
 * 6)    Last packet is sent without SOM being set to '1'
 * 7)    When the error bit of the particular mctp packet is high
 *
 * For the case where SOM is set to '1' for each read, the firmware will collect the previous set of data and
 * dropped them. The firmware then resets the tracking counter and treats the latest available packet with SOM set to '1'
 * as the first packet again.
 *
 * For the case where EOM is set to '1' without SOM being set to '1', the firmware will dropped the erroneous packet.
 *
 * @param mctp_ctx : MCTP info
 * @param processed_mctp_pkt : received MCTP message
 * @param source_addr : source address of host
 *
 * @return 0 if there is any error or timeout, otherwise return the size of message.
 *
 */

static alt_u32 responder_cpld_process_mctp_pkt(MCTP_CONTEXT* mctp_ctx, alt_u8* processed_mctp_pkt, alt_u8* source_addr)
{
    // Check for challenges coming from three mctp-capable hosts
    MB_REGFILE_OFFSET_ENUM challenger_avail_error_flag_reg;
    MB_REGFILE_OFFSET_ENUM byte_count_reg;
    MB_REGFILE_OFFSET_ENUM challenger_mctp_pckt_reg;

    // There are 3 challengers BMC, PCH and PCIE
    switch (mctp_ctx->challenger_type)
    {
        case BMC: challenger_avail_error_flag_reg = MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR;
                  byte_count_reg = MB_MCTP_BMC_BYTE_COUNT;
                  challenger_mctp_pckt_reg = MB_MCTP_BMC_PACKET_READ_RXFIFO;
                  break;

        case PCH: challenger_avail_error_flag_reg = MB_MCTP_PCH_PACKET_AVAIL_AND_RX_PACKET_ERROR;
                  byte_count_reg = MB_MCTP_PCH_BYTE_COUNT;
                  challenger_mctp_pckt_reg = MB_MCTP_PCH_PACKET_READ_RXFIFO;
                  break;

        // PCIE
        default : challenger_avail_error_flag_reg = MB_MCTP_PCIE_PACKET_AVAIL_AND_RX_PACKET_ERROR;
                  byte_count_reg = MB_MCTP_PCIE_BYTE_COUNT;
                  challenger_mctp_pckt_reg = MB_MCTP_PCIE_PACKET_READ_RXFIFO;
                  break;
    }

    alt_u32 mctp_read_pkt_size = 0;
    alt_u32 current_pkt_size = 0;
    alt_u32 som_eom_tracker_index = 0;
    // At the beginning of the transaction, set this to low to track first incoming mctp packet
    alt_u32 has_som_been_set_for_first_pkt = 0;
    alt_u32 is_mctp_pkt_valid = VALID_PKT;

    // Default this tracker to first packet at the beginning of operation
    alt_u32 som_eom_tracker = FIRST_PKT;

    // Store the mctp packet into an array
    // From the specs, it is assumed that the largest mctp packet would be 1024 bytes
    alt_u8 mctp_stored_packet[MAX_MCTP_I2C_BUFFER_SIZE] = {0};
    alt_u8 mctp_pkt_size_collection[MAX_MCTP_I2C_PKT_IDX] = {0};
    alt_u8 mctp_pkt_size_index = 0;
    alt_u8 mctp_first_pkt_size_idx = 0;
    // Points to stored packets
    alt_u8* mctp_stored_packet_ptr = mctp_stored_packet;
    alt_u8* mctp_first_pkt_ptr = mctp_stored_packet_ptr;

    // Poll for the available flag of the mctp packets from an incoming hosts
    while (read_from_mailbox(challenger_avail_error_flag_reg) & MCTP_PKT_AVAIL_BIT)
    {
        reset_hw_watchdog();
        alt_u32 dest_eid_idx = som_eom_tracker_index + 3;
        alt_u32 pkt_info_idx = som_eom_tracker_index + 4;
        alt_u32 msg_type_idx = som_eom_tracker_index + 5;
        
        // Extract the size of the mctp packet, could be first, second packet etc...
        // Store all the incoming packet size
        current_pkt_size = read_from_mailbox(byte_count_reg);

        // Read to pop out bytes which collectively forms the mctp packet from the mailbox fifo
        // extract the "current" packet size and accumulate them to get the total size of the message
        read_mctp_pkt_from_mb(challenger_mctp_pckt_reg, mctp_stored_packet_ptr, current_pkt_size);

        // If error bit is set, this means the current mctp packet is erroneous and fw has to post process and discard this mctp packets
        is_mctp_pkt_valid = (read_from_mailbox(challenger_avail_error_flag_reg) & MCTP_PKT_ERROR_BIT)
                             ? INVALID_PKT : VALID_PKT;

        // This is the source address which is needed as destination address when transmitting with SMBus Master.
        // This is the first byte of MCTP packet
        // Byte 5 of the mctp packet contains both SOM and EOM
        // This one tracks if the SOM and EOM is set based on the mappings
        *source_addr = mctp_stored_packet[som_eom_tracker_index];

        mctp_ctx->cached_addr = (mctp_stored_packet[som_eom_tracker_index] >> 1);
        
        // Write available and error register to clear the bit for polling, waiting for next MCTP message
        write_to_mailbox(challenger_avail_error_flag_reg, 1);

        mctp_pkt_size_collection[mctp_pkt_size_index] = current_pkt_size;
        mctp_read_pkt_size += current_pkt_size;

        som_eom_tracker = (FIRST_AND_LAST_PKT & mctp_stored_packet[pkt_info_idx]);
        mctp_ctx->cached_dest_eid = mctp_stored_packet[dest_eid_idx];
        mctp_ctx->cached_pkt_info = mctp_stored_packet[pkt_info_idx];
        mctp_ctx->cached_msg_type = mctp_stored_packet[msg_type_idx];

        if (is_mctp_pkt_valid)
        {
            if (som_eom_tracker == FIRST_PKT)
            {
                mctp_first_pkt_ptr = mctp_stored_packet_ptr;
                mctp_first_pkt_size_idx = mctp_pkt_size_index;

                has_som_been_set_for_first_pkt = 1;
            }
            else if (som_eom_tracker == FIRST_AND_LAST_PKT)
            {
                // Entering here means the packet is completed and is made up of only a single transaction
                mctp_first_pkt_ptr = mctp_stored_packet_ptr;
                mctp_first_pkt_size_idx = mctp_pkt_size_index;
                // As fw has served the host, fw will proceed to other operations
                break;
            }
            else if (som_eom_tracker == INTERMEDIARY_PKT)
            {
                // It is expected that the first packet with SOM high has been sent prior to receiving the middle packets
                if (!has_som_been_set_for_first_pkt)
                {
                    // This means that SOM has not been set and could mean that the middle packet arrive before the first packet.
                    is_mctp_pkt_valid = INVALID_PKT;
                }
            }
            else if (som_eom_tracker == LAST_PKT)
            {
                // If EOM is set high without SOM being first set high, the mctp packet is not valid
                if (!has_som_been_set_for_first_pkt)
                {
                    // This would mean the SOM has not been set high but EOM is
                    is_mctp_pkt_valid = INVALID_PKT;
                }
                else
                {
                    // breaks as the last packet has been served
                    break;
                }
            }
        }

        // If fw reached this stage, it means that last MCTP packet has not been received by CPLD.
        // Raise the pointer to store the next mctp packets
        if (!is_mctp_pkt_valid)
        {
            // For example, two 'SOM's sent to CPLD consequtively..
            // Reset counter
            mctp_stored_packet_ptr = &mctp_stored_packet[0];
            som_eom_tracker_index = 0;
            mctp_pkt_size_index = 0;
            mctp_read_pkt_size = 0;
        }
        else
        {
            mctp_stored_packet_ptr += current_pkt_size;
            // Raise the SOM and EOM index tracker
            som_eom_tracker_index += current_pkt_size;
            // Raise size index
            mctp_pkt_size_index++;
            if (mctp_pkt_size_index >= MAX_MCTP_I2C_PKT_IDX)
            {
                is_mctp_pkt_valid = INVALID_PKT;
                break;
            }
        }

        // Since last MCTP packet has not been received, wait until it is available
        while (!(read_from_mailbox(challenger_avail_error_flag_reg) & MCTP_PKT_AVAIL_BIT))
        {
            reset_hw_watchdog();
            // Last packet not received, hence timeout
            if (is_timer_expired(U_TIMER_BANK_TIMER4_ADDR))
            {
                break;
            }
        }

        // Handles timeout if devices are late in sending their packet
        if (is_timer_expired(U_TIMER_BANK_TIMER4_ADDR))
        {
            is_mctp_pkt_valid = INVALID_PKT;
            break;
        }
    }

    if (is_mctp_pkt_valid)
    {
        // Here removes all the incomplete first packet stored on previous iteration
        alt_u8* mctp_pkt_size_collection_ptr = mctp_pkt_size_collection;
        mctp_pkt_size_collection_ptr += mctp_first_pkt_size_idx;
        for (alt_u8 i = 0; i < mctp_first_pkt_size_idx; i++)
        {
            mctp_read_pkt_size -= mctp_pkt_size_collection[i];
        }
        return receive_and_disassemble_mctp_pkt(processed_mctp_pkt,
               mctp_first_pkt_ptr, mctp_pkt_size_collection_ptr, mctp_read_pkt_size);
    }
    return 0;
}

/**
 * @brief This function is used when CPLD is a requester.
 *
 * Sends the message (MCTP + SPDM) over to the SMBus Master
 *
 * @param mctp_ctx : mctp context
 * @param send_data : message to send
 * @param send_data_size : packet size
 *
 * @return none
 */
static void send_message(MCTP_CONTEXT* mctp_ctx, alt_u8* send_data, alt_u32 send_data_size)
{
    mctp_ctx->cached_pkt_info |= TAG_OWNER;
    assemble_and_send_mctp_pkt(mctp_ctx, send_data, send_data_size);
}

/**
 * @brief This function is used when CPLD is a requester/responder.
 *
 * Receive the message (MCTP + SPDM) from host by reading from Mailbox Fifo
 *
 * @param mctp_ctx : mctp context
 * @param receive_data : message to be received
 * @param timeout : timeout
 *
 * @return 0 if error from MCTP layer or timeout, otherwise size of received message
 */
static alt_u32 receive_message(MCTP_CONTEXT* mctp_ctx, alt_u8* receive_data, alt_u32 timeout)
{
    MB_REGFILE_OFFSET_ENUM challenger_avail_error_flag_reg;

    alt_u8 dest_addr = 0;
    alt_u32 is_received_msg_good = 0;

    // There are 3 challengers BMC, PCH and PCIE
    switch (mctp_ctx->challenger_type)
    {
        case BMC: dest_addr = (alt_u8)BMC_SLAVE_ADDRESS;
                  challenger_avail_error_flag_reg = MB_MCTP_BMC_PACKET_AVAIL_AND_RX_PACKET_ERROR;
                  break;

        case PCH: dest_addr = (alt_u8)PCH_SLAVE_ADDRESS;
                  challenger_avail_error_flag_reg = MB_MCTP_PCH_PACKET_AVAIL_AND_RX_PACKET_ERROR;
                  break;

        // PCIE
        default : dest_addr = (alt_u8)PCIE_SLAVE_ADDRESS;
                  challenger_avail_error_flag_reg = MB_MCTP_PCIE_PACKET_AVAIL_AND_RX_PACKET_ERROR;
                  break;
    }

    alt_u32 past_time = IORD(U_TIMER_BANK_TIMER4_ADDR, 0);

    // Check timer for any timeout event
    start_timer(U_TIMER_BANK_TIMER4_ADDR, timeout);
    while (!is_timer_expired(U_TIMER_BANK_TIMER4_ADDR))
    {
        reset_hw_watchdog();
        if (read_from_mailbox(challenger_avail_error_flag_reg) & MCTP_PKT_AVAIL_BIT)
        {
            is_received_msg_good = responder_cpld_process_mctp_pkt(mctp_ctx, receive_data, &dest_addr);
            break;
        }
    }
    start_timer(U_TIMER_BANK_TIMER4_ADDR, past_time);

    return is_received_msg_good;
}

#endif

#endif /* EAGLESTREAM_INC_MCTP_UTILS_H_ */
