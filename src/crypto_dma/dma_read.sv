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

// This module implements avmm read request to SPI master IP

`timescale 1 ps / 1 ps
`default_nettype none

module dma_read #(
    parameter ADDRESS_WIDTH         = 25,
    parameter LENGTH_WIDTH          = 26,
    parameter ADDRESS_WIDTH_BYTE    = 27,
    parameter LENGTH_WIDTH_BYTE     = 28,
    parameter MAX_BURST_COUNT       = 64,
    parameter MAX_BURST_COUNT_W     = 7,
    parameter PENDING_READ_WIDTH    = 7,
    parameter FIFO_ENABLE           = 1,
    parameter FIFO_DEPTH            = 128,
    parameter FIFO_DEPTH_LOG2       = 7     // fifo depth of 128
) (
    input wire                              clk,                // clock source (spi_clk domain, which similar to SPI master IP clock domain)
    input wire                              areset,             // reset source synchronous to clk
    
    output logic [ADDRESS_WIDTH-1:0]        mm_address,         // AVMM read master interface
    output logic                            mm_read,
    output logic [MAX_BURST_COUNT_W-1:0]    mm_burstcount,
    input wire   [31:0]                     mm_readdata,
    input wire                              mm_readdatavalid,
    input wire                              mm_waitrequest,

    input wire                              go_bit_sync,        // synchronized version of go bit from DMA CSR
    input wire   [LENGTH_WIDTH_BYTE-1:0]    transfer_length,    // transfer length info from DMA CSR
    input wire   [ADDRESS_WIDTH_BYTE-1:0]   start_address,      // start address info from DMA CSR
    input wire   [FIFO_DEPTH_LOG2:0]        fifo_wrusedw,       // number of valid entry in the FIFO, used to calculate how many fifo space left
    input wire                              fifo_wrempty,       // fifo empty indication
    input wire                              sha_done,           // indicates sha block operation completes (used in FIFO-LESS mode)
    input wire                              crypto_mode,        // crypto 256 or crypto 384 indication from dma_csr
    output logic                            read_done           // read done to indicate that DMA read operation has completed as
                                                                // indication to dma_sha_write module to proceed with padding block
);

logic [LENGTH_WIDTH-1:0]        length_counter;
logic [ADDRESS_WIDTH-1:0]       address_counter;
logic [PENDING_READ_WIDTH:0]    pending_read_counter;
logic [PENDING_READ_WIDTH:0]    pending_read_counter_nxt;
logic                           read_accepted;
logic                           read_done_nxt;
logic                           go_bit_sync_dly;
logic                           go_bit_pulse;
logic                           set_mm_read;
logic [MAX_BURST_COUNT_W-1:0]   max_burstcount;

assign max_burstcount   = FIFO_ENABLE ? MAX_BURST_COUNT[MAX_BURST_COUNT_W-1:0] : crypto_mode ? 6'd32 : 6'd16; 
assign mm_burstcount    = (length_counter < max_burstcount) ? length_counter[MAX_BURST_COUNT_W-1:0] : max_burstcount;
assign read_accepted    = (mm_read == 1'b1) && (mm_waitrequest == 1'b0);

// fifo_wrempty is included to avoid race condition observed in data_sha_write module
assign read_done_nxt    = (length_counter == 0) && (pending_read_counter == 0) && (fifo_wrempty == 1'b1);
assign mm_address       = address_counter;
assign go_bit_pulse     = go_bit_sync && ~go_bit_sync_dly;


generate
    if (FIFO_ENABLE) begin

        logic enough_fifo_space;

        // keep issuing read while length counter is not zero and there is enough space in FIFO to store returned read data (FIFO mode)
        assign set_mm_read          = (length_counter > 0) && enough_fifo_space;
        assign enough_fifo_space    = ((FIFO_DEPTH - fifo_wrusedw - pending_read_counter) >= mm_burstcount);

    end
    else begin

        logic sha_done_dly;
        logic go_bit_pulse_dly;

        // issue the next read only after sha block operation completes (FIFO-LESS mode)
        assign set_mm_read = (length_counter > 0) && ((sha_done && ~sha_done_dly) || go_bit_pulse_dly);

        always_ff @(posedge clk or posedge areset) begin
            if (areset) begin
                sha_done_dly        <= 1'b1;
                go_bit_pulse_dly    <= 1'b0;
            end
            else begin
                sha_done_dly        <= sha_done;
                go_bit_pulse_dly    <= go_bit_pulse;
            end
        end

    end
endgenerate


always_ff @(posedge clk or posedge areset) begin
    if (areset) begin
        go_bit_sync_dly <= 1'b0;
    end
    else begin
        go_bit_sync_dly <= go_bit_sync;
    end
end


always_ff @(posedge clk or posedge areset) begin
    if (areset) begin
        length_counter      <= '0;
        address_counter     <= '0;
    end
    else begin
        if (go_bit_pulse) begin                                         // load the required data length and start address of the DMA transfer
            length_counter  <= transfer_length[LENGTH_WIDTH+2-1:2];     // length in word format
            address_counter <= start_address[ADDRESS_WIDTH+2-1:2];      // word addressing as SPI master IP observes word addressing
        end else if (read_accepted) begin
            length_counter  <= length_counter - mm_burstcount;
            address_counter <= address_counter + mm_burstcount;         // word addressing as SPI master IP observes word addressing
        end else begin
            length_counter  <= length_counter;
            address_counter <= address_counter;
        end
    end
end


always_ff @(posedge clk or posedge areset) begin
    if (areset) begin
        mm_read     <= 1'b0;
    end
    else begin
        if (read_accepted) begin
            mm_read <= 1'b0; 
        end else if (set_mm_read) begin
            mm_read <= 1'b1;
        end else begin
            mm_read <= mm_read;
        end
    end
end

always_ff @(posedge clk or posedge areset) begin
    if (areset) begin
        pending_read_counter    <= '0;
        read_done               <= 1'b0;
    end
    else begin
        pending_read_counter    <= pending_read_counter_nxt;
        read_done               <= read_done_nxt;
    end
end


// track the number of pending read data to be returned
always_comb begin
    case ({mm_readdatavalid, read_accepted})
        2'b00 : pending_read_counter_nxt = pending_read_counter;
        2'b01 : pending_read_counter_nxt = pending_read_counter + mm_burstcount;
        2'b10 : pending_read_counter_nxt = pending_read_counter - 1'b1;
        2'b11 : pending_read_counter_nxt = pending_read_counter + mm_burstcount - 1'b1;

    endcase
end


endmodule
