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

module Sigma1 (
	input hash_size,
	input [63:0] in,
	output [63:0] out
	);

wire [63:0] in_rotr6, in_rotr11, in_rotr25; // Rotated input for SHA-256
wire [63:0] in_rotr14, in_rotr18, in_rotr41; // Rotated input for SHA-384/512

assign in_rotr6  = {32'h0, in[5:0], in[31:6]};
assign in_rotr11 = {32'h0, in[10:0], in[31:11]};
assign in_rotr25 = {32'h0, in[24:0], in[31:25]};

assign in_rotr14 = {in[13:0], in[63:14]}; 
assign in_rotr18 = {in[17:0], in[63:18]}; 
assign in_rotr41 = {in[40:0], in[63:41]}; 

assign out = (hash_size==1'b0) ? (in_rotr6 ^ in_rotr11 ^ in_rotr25) : (in_rotr14 ^ in_rotr18 ^ in_rotr41);

endmodule

