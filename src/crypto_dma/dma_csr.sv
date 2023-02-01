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

// This module implements DMA CSR register bits

`timescale 1 ps / 1 ps
`default_nettype none

module dma_csr #(
    parameter ADDRESS_WIDTH_BYTE    = 27,
    parameter LENGTH_WIDTH_BYTE     = 28
) (
    input wire                              clk,                // clock source (sys_clk domain)
    input wire                              areset,             // reset source synchrnous to clk
    input wire                              csr_read,           // AVMM slave interface with fixed read latency of 0
    input wire                              csr_write,
    input wire [31:0]                       csr_writedata,
    input wire [1:0]                        csr_address,
    output logic [31:0]                     csr_readdata,
    output logic                            go_bit,             // go bit from CSR
    output logic [LENGTH_WIDTH_BYTE-1:0]    transfer_length,    // transfer length from CSR
    output logic [ADDRESS_WIDTH_BYTE-1:0]   start_address,      // start address from CSR
    output logic                            crypto_mode,        // indicate crypto 256 or crypto 384 mode, consumed by dma_sha_write module
    input wire                              read_done_sync,     // synchronized version of read done from dma_read module
    input wire                              write_done          // write done from dma_sha_write module (already in sys_clk domain)
);

localparam DMA_CSR_CSR_BASE_ADDRESS             = 2'h0;
localparam DMA_CSR_START_ADDRESS_BASE_ADDRESS   = 2'h1;
localparam DMA_CSR_TRANSFER_LENGTH_BASE_ADDRESS = 2'h2;

localparam DMA_CSR_GO_BIT_POSITION          = 0;
localparam DMA_CSR_DMA_DONE_BIT_POSITION    = 1;
localparam DMA_CSR_CRYPTO_MODE_BIT_POSITION = 2;
localparam DMA_CSR_DMA_RESET_BIT_POSITION   = 31;

logic dma_done;
logic set_dma_done;
logic set_dma_done_dly;
logic set_dma_done_pulse;


always_ff @(posedge clk or posedge areset) begin
    if (areset) begin
        go_bit          <= 1'b0;
        crypto_mode     <= 1'b0;
        transfer_length <= '0;
        start_address   <= '0;
    end
    else begin
        if (csr_write && (csr_address == DMA_CSR_CSR_BASE_ADDRESS)) begin   // go_bit: once set, it remains set until DMA operation completes
            go_bit      <= csr_writedata[DMA_CSR_GO_BIT_POSITION];
        end else if (set_dma_done_pulse) begin
            go_bit      <= 1'b0;
        end else begin
            go_bit      <= go_bit;
        end
        
        if (csr_write && (csr_address == DMA_CSR_CSR_BASE_ADDRESS)) begin
            crypto_mode <= csr_writedata[DMA_CSR_CRYPTO_MODE_BIT_POSITION];
        end else begin
            crypto_mode <= crypto_mode;
        end

        if (csr_write && (csr_address == DMA_CSR_START_ADDRESS_BASE_ADDRESS)) begin
            start_address <= csr_writedata[ADDRESS_WIDTH_BYTE-1:0];
        end else begin
            start_address <= start_address;
        end

        if (csr_write && (csr_address == DMA_CSR_TRANSFER_LENGTH_BASE_ADDRESS)) begin
            transfer_length <= csr_writedata[LENGTH_WIDTH_BYTE-1:0];
        end else begin
            transfer_length <= transfer_length;
        end
    end
end

assign dma_done = ~go_bit;

always_ff @(posedge clk or posedge areset) begin
    if (areset) begin
        set_dma_done_dly <= 1'b0;
    end
    else begin
        set_dma_done_dly <= set_dma_done;
    end
end

assign set_dma_done = read_done_sync && write_done;             // dma_done is set when both DMA read and DMA write to Crypto block complete
assign set_dma_done_pulse = set_dma_done && ~set_dma_done_dly;

always_comb begin
    if (csr_read && (csr_address == DMA_CSR_CSR_BASE_ADDRESS)) begin
        csr_readdata = {30'h0, dma_done, 1'b0}; 
    end else begin
        csr_readdata = 32'h0; 
    end
end


endmodule
