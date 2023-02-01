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


module basic_avmm_bridge_rdv #(
    ADDRESS_WIDTH = 1
) (
    input wire clk,
    input wire areset,
    
    input wire [ADDRESS_WIDTH-1:0] avs_address,
    output wire avs_waitrequest,
    input wire avs_read,
    input wire avs_write,
    output wire [31:0] avs_readdata,
    input wire [31:0] avs_writedata,
    output wire avs_readdatavalid,
    
    output wire avm_clk,
    output wire avm_areset,
    output wire [ADDRESS_WIDTH-1:0] avm_address,
    input wire avm_waitrequest,
    output wire avm_read,
    output wire avm_write,
    input wire [31:0] avm_readdata,
    output wire [31:0] avm_writedata,
    input wire avm_readdatavalid

);


    assign avm_clk = clk;
    assign avm_areset = areset;
    assign avm_address = avs_address;
    assign avs_waitrequest = avm_waitrequest;
    assign avm_read = avs_read;
    assign avm_write = avs_write;
    assign avs_readdata = avm_readdata;
    assign avm_writedata = avs_writedata;
    assign avs_readdatavalid = avm_readdatavalid;


endmodule