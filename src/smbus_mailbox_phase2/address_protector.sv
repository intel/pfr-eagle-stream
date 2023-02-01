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

// Address protector module
// This module contains conditions for all the addresses that the BMC and PCH slaves are allowed to write to
// These conditions currently reflect the PFR HAS version 0.96
// Edit this file if the SMBus mailbox register map changes

`timescale 1 ps / 1 ps
`default_nettype none

module address_protector ( 
	                       input [7:0] pch_address,
	                       output      pch_cmd_valid,

	                       input [7:0] bmc_address,
	                       output      bmc_cmd_valid
	                       );

	// Signals is true if the address is in an allowable range whitelist 
	// The signals are only considered in the top level for writes, all reads are valid

	// update conditions to reflect the allowable ranges for the PCH
	assign pch_cmd_valid = (
							pch_address == 8'h9  ||
							pch_address == 8'hA  ||
							pch_address == 8'hB  ||
							pch_address == 8'h10 ||
							pch_address == 8'h11 ||
							pch_address == 8'h14 ||
							pch_address[7:6] == 2'b10/*(pch_address >= 8'h80 && pch_address <= 8'hBF)*/
							);
								     
	// update conditions to reflect the allowable write ranges for the BMC					     
	assign bmc_cmd_valid = (
							bmc_address == 8'h9  ||
							bmc_address == 8'hA  ||
							bmc_address == 8'hB  ||
							bmc_address == 8'h13 ||
							bmc_address == 8'h15 ||
							bmc_address[7:6] == 2'b11 // (bmc_address >= 8'hC0 && bmc_address <= 8'hFF)
							);



endmodule