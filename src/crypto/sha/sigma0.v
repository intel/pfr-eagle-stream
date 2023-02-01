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

module Sigma0 (
	input hash_size,
	input [63:0] in,
	output [63:0] out
	);

wire [63:0] in_rotr2, in_rotr13, in_rotr22; // Rotated input for SHA-256
wire [63:0] in_rotr28, in_rotr34, in_rotr39; // Rotated input for SHA-384/512

assign in_rotr2  = {32'h0, in[1:0], in[31:2]};
assign in_rotr13 = {32'h0, in[12:0], in[31:13]};
assign in_rotr22 = {32'h0, in[21:0], in[31:22]};

assign in_rotr28 = {in[27:0], in[63:28]}; 
assign in_rotr34 = {in[33:0], in[63:34]}; 
assign in_rotr39 = {in[38:0], in[63:39]}; 

assign out = (hash_size==1'b0) ? (in_rotr2 ^ in_rotr13 ^ in_rotr22) : (in_rotr28 ^ in_rotr34 ^ in_rotr39);

endmodule

