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
                       // SMBus Address, both the BMC and PCH slaves use this address but they can be modified to use different addresses
                       parameter SMBUS_ADDRESS = 7'h55
                       ) (
                        // Clock input
                        input  logic          clk,

                        // Asyncronous active low reset
                        input  logic          i_resetn,
                        
                        //share the crc8 calculation with RFNVRAM SMBus master
                        input                 rfnvram_pop_txfifo,
                        input  logic [7:0]    rfnvram_txfifo_data,
                        output logic [7:0]    crc_code_out,

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
    logic                                     is_pch_mctp_packet;
    logic                                     pch_smbus_stop;
    logic                                     nack_mctp_pch_packet;
    
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
    logic                                     is_bmc_mctp_packet;
    logic                                     bmc_smbus_stop;
    logic                                     nack_mctp_bmc_packet;
    
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
    logic                                     is_pcie_mctp_packet;
    logic                                     pcie_smbus_stop;
    logic                                     nack_mctp_pcie_packet;
    // used for CRC8 calculation
    logic                                     pec_error;
    logic                                     update_crc;
    logic                                     smbus_stop;
    logic                                     put_databyte;
    logic                                     put_addrbyte;   
    logic                                     rfnvram_pop_txfifo_nxt;    
    logic                                     pch_put_rx_databuffer;
    logic                                     pch_rx_addr_match_det;
    logic [6:0]                               pch_rx_sevenbit_addr;
    logic [7:0]                               pch_wdata_rx_databuffer;    
    logic                                     bmc_put_rx_databuffer;
    logic                                     bmc_rx_addr_match_det;
    logic [6:0]                               bmc_rx_sevenbit_addr;
    logic [7:0]                               bmc_wdata_rx_databuffer; 
    logic                                     pcie_put_rx_databuffer;
    logic                                     pcie_rx_addr_match_det;
    logic [6:0]                               pcie_rx_sevenbit_addr;
    logic [7:0]                               pcie_wdata_rx_databuffer; 
    logic [7:0]                               crc_data_in;
    
    logic                                     mctp_packet_nxt;
    logic                                     mctp_packet;
    logic                                     check_pec;
    
always_ff @(posedge clk or negedge i_resetn) begin
    if (!i_resetn) begin
        rfnvram_pop_txfifo_nxt <= 1'b0;
        mctp_packet_nxt <= 1'b0;
        put_databyte <= 1'b0;
        put_addrbyte <= 1'b0;
    
    end 
    else begin
        rfnvram_pop_txfifo_nxt <= rfnvram_pop_txfifo;
        mctp_packet_nxt <= mctp_packet;
    	
        if (pch_put_rx_databuffer | bmc_put_rx_databuffer | pcie_put_rx_databuffer | rfnvram_pop_txfifo_nxt) begin
            put_databyte <= 1'b1;
        end
        else begin
            put_databyte <= 1'b0;
        end
        
        if (pch_rx_addr_match_det | bmc_rx_addr_match_det | pcie_rx_addr_match_det) begin
            put_addrbyte <= 1'b1;
        end
        else begin
            put_addrbyte <= 1'b0;
        end
    end
end

always_ff @(posedge clk or negedge i_resetn) begin
    if (!i_resetn) begin
        crc_data_in <= 8'd0;
    end 
    else begin
        if (pch_put_rx_databuffer) begin
            crc_data_in <= pch_wdata_rx_databuffer;
        end
        else if (bmc_put_rx_databuffer) begin
            crc_data_in <= bmc_wdata_rx_databuffer;
        end
        else if (pcie_put_rx_databuffer) begin
            crc_data_in <= pcie_wdata_rx_databuffer;
        end
        else if (pch_rx_addr_match_det) begin
            crc_data_in <= {pch_rx_sevenbit_addr, 1'b0};
        end
        else if (bmc_rx_addr_match_det) begin
            crc_data_in <= {bmc_rx_sevenbit_addr, 1'b0};
        end
        else if (pcie_rx_addr_match_det) begin
            crc_data_in <= {pcie_rx_sevenbit_addr, 1'b0};
        end
        else if (rfnvram_pop_txfifo_nxt) begin
            crc_data_in <= rfnvram_txfifo_data;
        end
        else begin
            crc_data_in <= 8'd0;
        end
    end
end

assign update_crc = (put_databyte || put_addrbyte);
assign pec_error = check_pec ? ((crc_code_out == 8'd0) ? 1'b0 : 1'b1) : 1'b0;
assign smbus_stop = pch_smbus_stop || bmc_smbus_stop || pcie_smbus_stop;
assign mctp_packet = (is_pch_mctp_packet && ~pch_smbus_stop) || (is_bmc_mctp_packet && ~bmc_smbus_stop) || (is_pcie_mctp_packet && ~pcie_smbus_stop);
assign check_pec = ~mctp_packet && mctp_packet_nxt;


    i2c_crc8_d8 i_i2c_crc8_d8 (
        .clk                        (clk),
        .reset_n                    (i_resetn),
        .flush_crc                  (smbus_stop),
        .update_crc                 (update_crc),
        .crc_data_in                (crc_data_in),
        .crc_code_out               (crc_code_out)
    );
    
    // PCH SMBus slave
    // Takes SMBus/I2C transactions and translates them into AVMM transactions
    // This module is based off of the quartus i2c to avmm master bridge
    // But it was modified to support 8 bit reads and writes instead of the 32 bits that are expected in AVMM
    i2c_slave #(
                .I2C_SLAVE_ADDRESS(SMBUS_ADDRESS)
                ) pch_slave (     
                                .clk                (clk),
                                .is_mctp_packet     (is_pch_mctp_packet),
                                .nack_mctp_packet   (nack_mctp_pch_packet),								
                                .smbus_stop         (pch_smbus_stop),
                                .pec_error          (pec_error),
                                .invalid_cmd        (pch_invalid_cmd),
                                .put_rx_databuffer  (pch_put_rx_databuffer),
                                .rx_addr_match_det  (pch_rx_addr_match_det),
                                .rx_sevenbit_addr   (pch_rx_sevenbit_addr),
                                .wdata_rx_databuffer(pch_wdata_rx_databuffer),
                                .address            (pch_address),                   
                                .read               (pch_read),                      
                                .readdata           (pch_readdata),     
                                .readdatavalid      (pch_readdatavalid),
                                .waitrequest        (pch_waitrequest),                         
                                .write              (pch_write),                                               
                                .writedata          (pch_writedata),                
                                .rst_n              (i_resetn),  
                                .i2c_data_in        (ia_pch_slave_sda_in),             
                                .i2c_clk_in         (ia_pch_slave_scl_in),             
                                .i2c_data_oe        (o_pch_slave_sda_oe),                  
                                .i2c_clk_oe         (o_pch_slave_scl_oe)                    
                                );

    // BMC SMBus slave
    i2c_slave #(
                .I2C_SLAVE_ADDRESS(SMBUS_ADDRESS)
                ) bmc_slave (
                                .clk                (clk),       
                                .is_mctp_packet     (is_bmc_mctp_packet),	
                                .nack_mctp_packet   (nack_mctp_bmc_packet),
                                .smbus_stop         (bmc_smbus_stop),
                                .pec_error          (pec_error),
                                .invalid_cmd        (bmc_invalid_cmd),
                                .put_rx_databuffer  (bmc_put_rx_databuffer),
                                .rx_addr_match_det  (bmc_rx_addr_match_det),
                                .rx_sevenbit_addr   (bmc_rx_sevenbit_addr),
                                .wdata_rx_databuffer(bmc_wdata_rx_databuffer),
                                .address            (bmc_address),           
                                .read               (bmc_read),           
                                .readdata           (bmc_readdata),           
                                .readdatavalid      (bmc_readdatavalid),           
                                .waitrequest        (bmc_waitrequest),                      
                                .write              (bmc_write),                                  
                                .writedata          (bmc_writedata),           
                                .rst_n              (i_resetn),              
                                .i2c_data_in        (ia_bmc_slave_sda_in),              
                                .i2c_clk_in         (ia_bmc_slave_scl_in),             
                                .i2c_data_oe        (o_bmc_slave_sda_oe),                   
                                .i2c_clk_oe         (o_bmc_slave_scl_oe)                    
                                );
                                
    // HPFR SMBus slave
    i2c_slave #(
                .I2C_SLAVE_ADDRESS(SMBUS_ADDRESS)
                ) pcie_slave (
                                .clk                (clk),      
                                .is_mctp_packet     (is_pcie_mctp_packet),
                                .nack_mctp_packet   (nack_mctp_pcie_packet),
                                .smbus_stop         (pcie_smbus_stop),
                                .pec_error          (pec_error), 
                                .invalid_cmd        (pcie_invalid_cmd),
                                .put_rx_databuffer  (pcie_put_rx_databuffer),
                                .rx_addr_match_det  (pcie_rx_addr_match_det),
                                .rx_sevenbit_addr   (pcie_rx_sevenbit_addr),
                                .wdata_rx_databuffer(pcie_wdata_rx_databuffer),
                                .address            (pcie_address),           
                                .read               (pcie_read),           
                                .readdata           (pcie_readdata),           
                                .readdatavalid      (pcie_readdatavalid),           
                                .waitrequest        (pcie_waitrequest),                      
                                .write              (pcie_write),                                  
                                .writedata          (pcie_writedata),           
                                .rst_n              (i_resetn),              
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
                         .pec_error         (pec_error),
                         
                         .m0_read           (m0_read),
                         .m0_write          (m0_write),
                         .m0_writedata      (m0_writedata[7:0]),
                         .m0_readdata       (m0_readdata),
                         .m0_address        (m0_address), 
                         .m0_readdatavalid  (m0_readdatavalid),

                         .pch_read          (pch_read),
                         .pch_write         (pch_write),
                         .pch_writedata     (pch_writedata[7:0]),
                         .pch_readdata      (pch_readdata),
                         .pch_address       (pch_address[7:0]), 
                         .pch_readdatavalid (pch_readdatavalid),
                         .pch_waitrequest   (pch_waitrequest),
                         .is_pch_mctp_packet(is_pch_mctp_packet),
                         .nack_mctp_pch_packet(nack_mctp_pch_packet),
                         .pch_invalid_cmd   (pch_invalid_cmd),
                         .pch_smbus_stop    (pch_smbus_stop),

                         .bmc_read          (bmc_read),
                         .bmc_write         (bmc_write),
                         .bmc_writedata     (bmc_writedata[7:0]),
                         .bmc_readdata      (bmc_readdata),
                         .bmc_address       (bmc_address[7:0]), 
                         .bmc_readdatavalid (bmc_readdatavalid),
                         .bmc_waitrequest   (bmc_waitrequest),
                         .is_bmc_mctp_packet(is_bmc_mctp_packet),
                         .nack_mctp_bmc_packet(nack_mctp_bmc_packet),
                         .bmc_invalid_cmd   (bmc_invalid_cmd),
                         .bmc_smbus_stop    (bmc_smbus_stop),
                         
                         .pcie_read          (pcie_read),
                         .pcie_write         (pcie_write),
                         .pcie_writedata     (pcie_writedata[7:0]),
                         .pcie_readdata      (pcie_readdata),
                         .pcie_address       (pcie_address[7:0]), 
                         .pcie_readdatavalid (pcie_readdatavalid),
                         .pcie_waitrequest   (pcie_waitrequest),
                         .is_pcie_mctp_packet(is_pcie_mctp_packet),
                         .nack_mctp_pcie_packet(nack_mctp_pcie_packet),
                         .pcie_invalid_cmd   (pcie_invalid_cmd),
                         .pcie_smbus_stop    (pcie_smbus_stop)
                         );

endmodule
