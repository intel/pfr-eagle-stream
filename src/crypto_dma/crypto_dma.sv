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

// This module is a custom DMA which perform data transfer from SPI flash to
// Crypto IP block. It reads SPI flash data from SPI master IP core and writes to Crypto IP block for SHA
// calculation. 

`timescale 1 ps / 1 ps
`default_nettype none

module crypto_dma #(
    parameter ADDRESS_WIDTH_SPI = 25,   // Specify max address size this DMA can read from (in word addressing). Should set to largest targeted SPI flash device density.
    parameter ADDRESS_WIDTH_UFM = 21    // Specify max address size this DMA can read from (in word addressing). Depend on address memory map setting in pfr_sys

) (
    input wire                          spi_clk,            // Clock used by dma_read module (facing SPI master IP)
    input wire                          spi_clk_areset,
    
    output logic [ADDRESS_WIDTH_SPI-1:0]    spi_mm_address,         // AVMM read master interface (spi_clk domain)
    output logic                            spi_mm_read,
    output logic [6:0]                      spi_mm_burstcount,
    input wire   [31:0]                     spi_mm_readdata,
    input wire                              spi_mm_readdatavalid,
    input wire                              spi_mm_waitrequest,
    
    output logic [ADDRESS_WIDTH_UFM-1:0]    ufm_mm_address,         // AVMM read master interface (sys_clk domain)
    output logic                            ufm_mm_read,
    output logic [5:0]                      ufm_mm_burstcount,
    input wire   [31:0]                     ufm_mm_readdata,
    input wire                              ufm_mm_readdatavalid,
    input wire                              ufm_mm_waitrequest,

    input wire                          csr_read,           // DMA CSR AVMM slave interface (sys_clk domain)
    input wire                          csr_write,
    input wire   [31:0]                 csr_writedata,
    input wire   [1:0]                  csr_address,
    output logic [31:0]                 csr_readdata,

    input wire                          sys_clk,            // Clock used by dma_sha_write as well as dma_csr
    input wire                          sys_clk_areset,

    output logic [33:0]                 wr_data,            // Conduit interface writing sha data into Crypto block (sys_clk domain)
    output logic                        wr_data_valid,
    input wire                          sha_done,
    input wire                          dma_ufm_sel         // GPO bit to select dma to ufm path or dma to spi path
);


localparam LENGTH_WIDTH_SPI         = ADDRESS_WIDTH_SPI + 1;        // in word format
localparam LENGTH_WIDTH_UFM         = ADDRESS_WIDTH_UFM + 1;        // in word format
localparam ADDRESS_WIDTH_BYTE       = ADDRESS_WIDTH_SPI + 2;        // byte addressing, use larger size (which is SPI) for CSR register
localparam LENGTH_WIDTH_BYTE        = ADDRESS_WIDTH_SPI + 2 + 1;    // in byte format, use larger size (which is SPI) for CSR register
localparam FIFO_DEPTH               = 128;
localparam FIFO_DEPTH_LOG2          = $clog2(FIFO_DEPTH);
localparam MAX_BURST_COUNT_SPI      = 64;
localparam MAX_BURST_COUNT_SPI_W    = $clog2(MAX_BURST_COUNT_SPI) + 1;
localparam MAX_BURST_COUNT_UFM      = 32;
localparam MAX_BURST_COUNT_UFM_W    = $clog2(MAX_BURST_COUNT_UFM) + 1;


logic                           go_bit;             // go_bit from DMA CSR bit
logic                           go_bit_sync;        // synchronized version of go_bit
logic [LENGTH_WIDTH_BYTE-1:0]   transfer_length;    // transfer length from DMA CSR register
logic [ADDRESS_WIDTH_BYTE-1:0]  start_address;      // dma starting address from DMA CSR register

logic [FIFO_DEPTH_LOG2:0]       spi_fifo_wrusedw;   // number of valid entry in the fifo
logic                           spi_fifo_wrempty;   // fifo empty indication
logic                           spi_fifo_rdempty;   // fifo empty indication
logic                           spi_fifo_rdreq;     // fifo read request
logic [31:0]                    spi_fifo_q;         // fifo output bus

logic                           ufm_fifo_wrempty;   // fifo empty indication
logic                           ufm_fifo_rdempty;   // fifo empty indication
logic                           ufm_fifo_rdreq;     // fifo read request
logic [31:0]                    ufm_fifo_q;         // fifo output bus

logic                           spi_read_done;          // completion indication from dma_read module
logic                           spi_read_done_sync;     // synchronized version of read done
logic                           ufm_read_done;          // completion indication from dma_read module

logic                           write_done;         // completion indication from dma_sha_write module
logic                           crypto_mode;        // crypto mode bit from DMA CSR bit
logic                           read_done_out;

// module which implement DMA CSR register bits
dma_csr #(
    .ADDRESS_WIDTH_BYTE     (ADDRESS_WIDTH_BYTE),
    .LENGTH_WIDTH_BYTE      (LENGTH_WIDTH_BYTE)
) u_dma_csr (
    .clk                    (sys_clk),
    .areset                 (sys_clk_areset),
    .csr_read               (csr_read),
    .csr_write              (csr_write),
    .csr_writedata          (csr_writedata),
    .csr_address            (csr_address),
    .csr_readdata           (csr_readdata),
    .go_bit                 (go_bit),
    .transfer_length        (transfer_length),
    .start_address          (start_address),
    .crypto_mode            (crypto_mode),
    .read_done_sync         (read_done_out),
    .write_done             (write_done)
);


assign read_done_out = dma_ufm_sel ? ufm_read_done : spi_read_done_sync;

// module which implement avmm read request to SPI master IP
dma_read #(
    .ADDRESS_WIDTH      (ADDRESS_WIDTH_SPI),
    .LENGTH_WIDTH       (LENGTH_WIDTH_SPI),
    .ADDRESS_WIDTH_BYTE (ADDRESS_WIDTH_BYTE),
    .LENGTH_WIDTH_BYTE  (LENGTH_WIDTH_BYTE),
    .MAX_BURST_COUNT    (MAX_BURST_COUNT_SPI),
    .MAX_BURST_COUNT_W  (MAX_BURST_COUNT_SPI_W),
    .PENDING_READ_WIDTH (FIFO_DEPTH_LOG2), // max pending read depends on depth of fifo
    .FIFO_ENABLE        (1),
    .FIFO_DEPTH         (FIFO_DEPTH),
    .FIFO_DEPTH_LOG2    (FIFO_DEPTH_LOG2)     
) u_dma_read_spi (
    .clk                (spi_clk),
    .areset             (spi_clk_areset),
    .mm_address         (spi_mm_address),
    .mm_read            (spi_mm_read),
    .mm_burstcount      (spi_mm_burstcount),
    .mm_readdata        (spi_mm_readdata),
    .mm_readdatavalid   (spi_mm_readdatavalid),
    .mm_waitrequest     (spi_mm_waitrequest),
    .go_bit_sync        (go_bit_sync && ~dma_ufm_sel),
    .transfer_length    (transfer_length),
    .start_address      (start_address),
    .fifo_wrusedw       (spi_fifo_wrusedw),
    .fifo_wrempty       (spi_fifo_wrempty),
    .sha_done           (1'b0), // not used in FIFO mode
    .crypto_mode        (crypto_mode),
    .read_done          (spi_read_done)
);


// module which implement avmm read request to UFM IP
dma_read #(
    .ADDRESS_WIDTH      (ADDRESS_WIDTH_UFM),
    .LENGTH_WIDTH       (LENGTH_WIDTH_UFM),
    .ADDRESS_WIDTH_BYTE (ADDRESS_WIDTH_BYTE),
    .LENGTH_WIDTH_BYTE  (LENGTH_WIDTH_BYTE),
    .MAX_BURST_COUNT    (MAX_BURST_COUNT_UFM),
    .MAX_BURST_COUNT_W  (MAX_BURST_COUNT_UFM_W),
    .PENDING_READ_WIDTH (5), // max pending read depends on max sha block size which is 32 words
    .FIFO_ENABLE        (0),
    .FIFO_DEPTH         (1), // unused in FIFO-LESS mode
    .FIFO_DEPTH_LOG2    (0)  // unused in FIFO-LESS mode  
) u_dma_read_ufm (
    .clk                (sys_clk),  // Use sys_clk which is in the same clock domain as UFM IP
    .areset             (sys_clk_areset),
    .mm_address         (ufm_mm_address),
    .mm_read            (ufm_mm_read),
    .mm_burstcount      (ufm_mm_burstcount),
    .mm_readdata        (ufm_mm_readdata),
    .mm_readdatavalid   (ufm_mm_readdatavalid),
    .mm_waitrequest     (ufm_mm_waitrequest),
    .go_bit_sync        (go_bit && dma_ufm_sel),
    .transfer_length    (transfer_length),
    .start_address      (start_address),
    .fifo_wrusedw       (1'b0), // Not used in fifoless mode
    .fifo_wrempty       (ufm_fifo_wrempty),
    .sha_done           (sha_done),
    .crypto_mode        (crypto_mode),
    .read_done          (ufm_read_done)
);


// DCFIFO module which handle data clock crossing from spi_clk to sys_clk
dma_data_buffer #(
    .FIFO_ENABLE    (1)
) u_dma_data_buffer_spi (
    .data       (spi_mm_readdata),
    .rdclk      (sys_clk),
    .rdreq      (spi_fifo_rdreq),
    .wrclk      (spi_clk),
    .wrreq      (spi_mm_readdatavalid),
    .areset     (sys_clk_areset), // Not used in FIFO mode
    .q          (spi_fifo_q),
    .rdempty    (spi_fifo_rdempty),
    .wrempty    (spi_fifo_wrempty),
    .wrfull     (),                 // Not used
    .wrusedw    (spi_fifo_wrusedw)
);


// Single data register 
dma_data_buffer #(
    .FIFO_ENABLE    (0)
) u_dma_data_buffer_ufm (
    .data       (ufm_mm_readdata),
    .rdclk      (sys_clk),
    .rdreq      (ufm_fifo_rdreq),
    .wrclk      (sys_clk),
    .wrreq      (ufm_mm_readdatavalid),
    .areset     (sys_clk_areset),
    .q          (ufm_fifo_q),
    .rdempty    (ufm_fifo_rdempty),
    .wrempty    (ufm_fifo_wrempty),
    .wrfull     (),                 // Not used
    .wrusedw    ()                  // Not used
);


// module whch implement write request to Crypto IP block
dma_sha_write u_dma_sha_write (
    .clk                (sys_clk),
    .areset             (sys_clk_areset),
    .wr_data            (wr_data),
    .wr_data_valid      (wr_data_valid),
    .sha_done           (sha_done),
    .dma_ufm_sel        (dma_ufm_sel),
    .spi_read_done      (spi_read_done_sync),
    .ufm_read_done      (ufm_read_done),
    .write_done         (write_done),
    .spi_fifo_rdempty   (spi_fifo_rdempty),
    .spi_fifo_rdreq     (spi_fifo_rdreq),
    .spi_fifo_q         (spi_fifo_q),
    .ufm_fifo_rdempty   (ufm_fifo_rdempty),
    .ufm_fifo_rdreq     (ufm_fifo_rdreq),
    .ufm_fifo_q         (ufm_fifo_q),
    .crypto_mode        (crypto_mode),
    .transfer_length    (transfer_length)
);


altera_std_synchronizer #(
    .depth    (2)
) read_done_synchronizer (
    .clk        (sys_clk),
    .reset_n    (~sys_clk_areset),
    .din        (spi_read_done),
    .dout       (spi_read_done_sync)
);


altera_std_synchronizer #(
    .depth    (2)
) go_bit_synchronizer (
    .clk        (spi_clk),
    .reset_n    (~spi_clk_areset),
    .din        (go_bit),
    .dout       (go_bit_sync)
);


endmodule
