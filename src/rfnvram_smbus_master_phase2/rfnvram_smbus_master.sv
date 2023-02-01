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

`timescale 1 ps / 1 ps
`default_nettype none

// SMBus address = 0xDC
module rfnvram_smbus_master #(
	parameter FIFO_DEPTH = 8
)(
	input wire clk,
	input wire resetn,

	// AVMM interface to connect the NIOS to the CSR interface
	input  wire [3:0]	csr_address,
	input  wire 	  	csr_read,
	output wire [31:0]	csr_readdata,
	input  wire 		csr_write,
	input  wire [31:0]  csr_writedata,
    output logic        csr_readdatavalid,

	input wire sda_in,
	output wire sda_oe,
	input wire scl_in,
	output wire scl_oe

);

    logic readdatavalid_dly;
    always_ff @(posedge clk or negedge resetn) begin
        if (!resetn) begin
            csr_readdatavalid <= 1'b0;
            readdatavalid_dly <= 1'b0;
        
        end else begin
            readdatavalid_dly <= csr_read;
            csr_readdatavalid <= readdatavalid_dly;
        end
    end

    localparam FIFO_DEPTH_LOG2 = $clog2(FIFO_DEPTH);
    i2c_master #(.FIFO_DEPTH(FIFO_DEPTH),
                 .FIFO_DEPTH_LOG2(FIFO_DEPTH_LOG2))
    u0 (
        .clk                     (clk),   
        .rst_n                   (resetn),
        .sda_in                  (sda_in),
        .scl_in                  (scl_in),
        .sda_oe                  (sda_oe),
        .scl_oe                  (scl_oe),
        .addr                    (csr_address),       
        .read                    (csr_read),          
        .write                   (csr_write),         
        .writedata               (csr_writedata),     
        .readdata                (csr_readdata)
    );



endmodule
