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

// SMBus mailbox top level
// this module instanciates the i2c/SMBus slaves and the register file
// the BMC and PCH slaves both have different read and write access to the register file
// the AVMM interface has full access to the entire register file

`timescale 1 ps / 1 ps
`default_nettype none

module smbus_mailbox #(
                       // SMBus Address, both the BMC and PCH use different addresses but can be set to the same in the top level
                       parameter PCH_SMBUS_ADDRESS  = 7'h55,
                       parameter PCIE_SMBUS_ADDRESS = 7'h55,
                       parameter BMC_SMBUS_ADDRESS  = 7'h55
                       ) (
                        // Clock input
                        input  logic          clk,

                        // Asyncronous active low reset
                        input  logic          i_resetn,
                        
                        output logic [7:0]    crc_code_out,
                        input  logic [2:0]    slave_portid,
    
                        // SMBus signals for the BMC slave 
                        // the input and output enable signals should be combinded in a tristate buffer in the top level of a project using this
                        input  logic          ia_bmc_slave_sda_in,
                        output logic          o_bmc_slave_sda_oe, 
                        input  logic          ia_bmc_slave_scl_in,
                        output logic          o_bmc_slave_scl_oe,

                        // SMBus signals for the PCH/CPU slave
                        input  logic          ia_pch_slave_sda_in,
                        output logic          o_pch_slave_sda_oe, 
                        input  logic          ia_pch_slave_scl_in,
                        output logic          o_pch_slave_scl_oe,
                        
                        // SMBus signals for the HPFR slave
                        input  logic          ia_pcie_slave_sda_in,
                        output logic          o_pcie_slave_sda_oe, 
                        input  logic          ia_pcie_slave_scl_in,
                        output logic          o_pcie_slave_scl_oe,


                          // AVMM master interface for the Nios Controller
                        input  logic          m0_read,
                        input  logic          m0_write,
                        input  logic [31:0]   m0_writedata,
                        output logic [31:0]   m0_readdata,
                        input  logic [7:0]    m0_address,
                        output logic          m0_readdatavalid
                        );
    
    // AVMM like bus, used to connect the i2c_slaves to the register file
    logic [31:0]                              pch_address;
    logic                                     pch_read;
    logic [31:0]                              pch_readdata;
    logic                                     pch_readdatavalid;
    logic                                     pch_write;
    logic [31:0]                              pch_writedata;
    logic                                     pch_waitrequest;
    // used to signify when a transaction is blocked by the register file. 
    logic                                     pch_invalid_cmd;
    logic                                     is_pch_mctp_pkt;
    logic                                     pch_smbus_stop;
    logic                                     pch_smbus_start;
    logic                                     nack_pch_mctp_pkt;
    logic                                     pch_pec_error;
    logic [7:0]                               pch_crc_code_out;
    
    // AVMM like bus, used to connect the i2c_slaves to the register file
    logic [31:0]                              bmc_address;
    logic                                     bmc_read;
    logic [31:0]                              bmc_readdata;
    logic                                     bmc_readdatavalid;
    logic                                     bmc_write;
    logic [31:0]                              bmc_writedata;
    logic                                     bmc_waitrequest;
    // used to signify when a transaction is blocked by the register file.
    logic                                     bmc_invalid_cmd;
    logic                                     is_bmc_mctp_pkt;
    logic                                     bmc_smbus_stop;
    logic                                     bmc_smbus_start;
    logic                                     nack_bmc_mctp_pkt;
    logic                                     bmc_pec_error;
    logic [7:0]                               bmc_crc_code_out;
    
    // AVMM like bus, used to connect the i2c_slaves to the register file
    logic [31:0]                              pcie_address;
    logic                                     pcie_read;
    logic [31:0]                              pcie_readdata;
    logic                                     pcie_readdatavalid;
    logic                                     pcie_write;
    logic [31:0]                              pcie_writedata;
    logic                                     pcie_waitrequest;
    // used to signify when a transaction is blocked by the register file.
    logic                                     pcie_invalid_cmd;
    logic                                     is_pcie_mctp_pkt;
    logic                                     pcie_smbus_stop;
    logic                                     pcie_smbus_start;
    logic                                     nack_pcie_mctp_pkt;
    logic                                     pcie_pec_error;
    logic [7:0]                               pcie_crc_code_out;
    
    assign crc_code_out = (slave_portid == 3'b001) ? pch_crc_code_out :
                          (slave_portid == 3'b010) ? bmc_crc_code_out : 
                          (slave_portid == 3'b011) ? pcie_crc_code_out : 8'd0;
                            	
    // PCH SMBus slave
    // Takes SMBus/I2C transactions and translates them into AVMM transactions
    // This module is based off of the quartus i2c to avmm master bridge
    // But it was modified to support 8 bit reads and writes instead of the 32 bits that are expected in AVMM
    i2c_slave #(
                .I2C_SLAVE_ADDRESS(PCH_SMBUS_ADDRESS)
                ) pch_slave (     
                                .clk                (clk),
                                .is_mctp_pkt        (is_pch_mctp_pkt),
                                .nack_mctp_pkt      (nack_pch_mctp_pkt),	
                                .smbus_stop         (pch_smbus_stop),
                                .smbus_start        (pch_smbus_start),
                                .pec_error          (pch_pec_error),
                                .invalid_cmd        (pch_invalid_cmd),
                                .address            (pch_address),                   
                                .read               (pch_read),                      
                                .readdata           (pch_readdata),     
                                .readdatavalid      (pch_readdatavalid),
                                .waitrequest        (pch_waitrequest),                         
                                .write              (pch_write),                                               
                                .writedata          (pch_writedata),                
                                .rst_n              (i_resetn), 
                                .crc_code_out       (pch_crc_code_out),
                                .i2c_data_in        (ia_pch_slave_sda_in),             
                                .i2c_clk_in         (ia_pch_slave_scl_in),             
                                .i2c_data_oe        (o_pch_slave_sda_oe),                  
                                .i2c_clk_oe         (o_pch_slave_scl_oe)                    
                                );

    // BMC SMBus slave
    i2c_slave #(
                .I2C_SLAVE_ADDRESS(BMC_SMBUS_ADDRESS)
                ) bmc_slave (
                                .clk                (clk),       
                                .is_mctp_pkt        (is_bmc_mctp_pkt),	
                                .nack_mctp_pkt      (nack_bmc_mctp_pkt),
                                .smbus_stop         (bmc_smbus_stop),
                                .smbus_start        (bmc_smbus_start),
                                .pec_error          (bmc_pec_error),
                                .invalid_cmd        (bmc_invalid_cmd),
                                .address            (bmc_address),           
                                .read               (bmc_read),           
                                .readdata           (bmc_readdata),           
                                .readdatavalid      (bmc_readdatavalid),           
                                .waitrequest        (bmc_waitrequest),                      
                                .write              (bmc_write),                                  
                                .writedata          (bmc_writedata),           
                                .rst_n              (i_resetn), 
                                .crc_code_out       (bmc_crc_code_out),
                                .i2c_data_in        (ia_bmc_slave_sda_in),              
                                .i2c_clk_in         (ia_bmc_slave_scl_in),             
                                .i2c_data_oe        (o_bmc_slave_sda_oe),                   
                                .i2c_clk_oe         (o_bmc_slave_scl_oe)                    
                                );
                                
    // PCIe SMBus slave
    i2c_slave #(
                .I2C_SLAVE_ADDRESS(PCIE_SMBUS_ADDRESS)
                ) pcie_slave (
                                .clk                (clk),      
                                .is_mctp_pkt        (is_pcie_mctp_pkt),
                                .nack_mctp_pkt      (nack_pcie_mctp_pkt),
                                .smbus_stop         (pcie_smbus_stop),
                                .smbus_start        (pcie_smbus_start),
                                .pec_error          (pcie_pec_error), 
                                .invalid_cmd        (pcie_invalid_cmd),
                                .address            (pcie_address),           
                                .read               (pcie_read),           
                                .readdata           (pcie_readdata),           
                                .readdatavalid      (pcie_readdatavalid),           
                                .waitrequest        (pcie_waitrequest),                      
                                .write              (pcie_write),                                  
                                .writedata          (pcie_writedata),           
                                .rst_n              (i_resetn),   
                                .crc_code_out       (pcie_crc_code_out),
                                .i2c_data_in        (ia_pcie_slave_sda_in),              
                                .i2c_clk_in         (ia_pcie_slave_scl_in),             
                                .i2c_data_oe        (o_pcie_slave_sda_oe),                   
                                .i2c_clk_oe         (o_pcie_slave_scl_oe)                    
                                );
                                
    // Register file
    // module contains the register map as well as the write protection circuitry 
    // this module currently is hardcoded to support 2 SMBus slave ports but more can be added later
    reg_file reg_file_0 (
                         .clk               (clk),   
                         .resetn            (i_resetn),     
                         
                         .m0_read           (m0_read),
                         .m0_write          (m0_write),
                         .m0_writedata      (m0_writedata[7:0]),
                         .m0_readdata       (m0_readdata),
                         .m0_address        (m0_address), 
                         .m0_readdatavalid  (m0_readdatavalid),
                         
                         .pch_read          (pch_read),
                         .pch_write         (pch_write),
                         .pch_pec_error     (pch_pec_error),
                         .pch_writedata     (pch_writedata[7:0]),
                         .pch_readdata      (pch_readdata),
                         .pch_address       (pch_address[7:0]), 
                         .pch_readdatavalid (pch_readdatavalid),
                         .pch_waitrequest   (pch_waitrequest),
                         .is_pch_mctp_pkt   (is_pch_mctp_pkt),
                         .nack_pch_mctp_pkt (nack_pch_mctp_pkt),
                         .pch_invalid_cmd   (pch_invalid_cmd),
                         .pch_smbus_stop    (pch_smbus_stop),
                         .pch_smbus_start   (pch_smbus_start),

                         .bmc_read          (bmc_read),
                         .bmc_write         (bmc_write),
                         .bmc_pec_error     (bmc_pec_error),
                         .bmc_writedata     (bmc_writedata[7:0]),
                         .bmc_readdata      (bmc_readdata),
                         .bmc_address       (bmc_address[7:0]), 
                         .bmc_readdatavalid (bmc_readdatavalid),
                         .bmc_waitrequest   (bmc_waitrequest),
                         .is_bmc_mctp_pkt   (is_bmc_mctp_pkt),
                         .nack_bmc_mctp_pkt (nack_bmc_mctp_pkt),
                         .bmc_invalid_cmd   (bmc_invalid_cmd),
                         .bmc_smbus_stop    (bmc_smbus_stop),
                         .bmc_smbus_start   (bmc_smbus_start),
                         
                         .pcie_read          (pcie_read),
                         .pcie_write         (pcie_write),
                         .pcie_pec_error     (pcie_pec_error),
                         .pcie_writedata     (pcie_writedata[7:0]),
                         .pcie_readdata      (pcie_readdata),
                         .pcie_address       (pcie_address[7:0]), 
                         .pcie_readdatavalid (pcie_readdatavalid),
                         .pcie_waitrequest   (pcie_waitrequest),
                         .is_pcie_mctp_pkt   (is_pcie_mctp_pkt),
                         .nack_pcie_mctp_pkt (nack_pcie_mctp_pkt),
                         .pcie_invalid_cmd   (pcie_invalid_cmd),
                         .pcie_smbus_stop    (pcie_smbus_stop),
                         .pcie_smbus_start   (pcie_smbus_start)
                         );

endmodule
