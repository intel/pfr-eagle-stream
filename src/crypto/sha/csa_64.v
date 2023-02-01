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

module csa_64 (
        input hash_size,       
        input [63:0]  a,
        input [63:0]  b,
        input [63:0]  c,
        output [63:0] sum_out,
        output [63:0] carry_out
);


wire [63:0] a_and_b;
wire [63:0] b_and_c;
wire [63:0] a_and_c;
wire [63:0] cout_prior_to_shift;

assign  carry_out[63:0]   = ({ cout_prior_to_shift[62:32], hash_size & cout_prior_to_shift[31], cout_prior_to_shift[30:0], 1'b0});
assign  a_and_b[63:0] = b[63:0] & a[63:0];
assign  b_and_c[63:0] = c[63:0] & b[63:0];
assign  a_and_c[63:0] = c[63:0] & a[63:0];
assign  cout_prior_to_shift[63:0] = ( a_and_c[63:0] | b_and_c[63:0] | a_and_b[63:0] );
assign  sum_out[63:0]     = ( a[63:0] ^ b[63:0] ^ c[63:0] );

endmodule
