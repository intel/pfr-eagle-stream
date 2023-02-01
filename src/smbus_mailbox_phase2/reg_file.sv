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

// This is the register file that is accessed by the SMBus mailbox
// This module has 3 AVMM interfaces going into it. The PCH_* and BMC_* are SMBus interfaces that have been translated to a AVMM like interace
// This module supports single clock cycle reads so that the m0 AVMM interface does not require a readdatavalid signal
// reads and writes from the AVMM interface are independant of the reads and writes from the SMBus mailbox interfaces
// The SMBus interfaces must share access to the register file so they are arbitrated using the waitrequest signal
// the SMBus interfaces have restrictions on the addresses they can write, but the AVMM interface has full access

`timescale 1 ps / 1 ps
`default_nettype none


module reg_file (
                 input logic         clk,
                 input logic         resetn,
                 
                 input logic         pec_error,

                 // AVMM interface, controlled by the embedded NIOS
                 // The data width of AVM master is 32 bits but the register file is 8 so the uppoer 24 bits are tied to 0
                 input logic         m0_read,
                 input logic         m0_write,
                 input logic  [7:0]  m0_writedata,
                 output logic [31:0] m0_readdata,
                 input logic  [7:0]  m0_address,
                 output logic        m0_readdatavalid,

                 // PCH/CPU interface. This is translated from SMBus to AVMM using i2c_slave.sv
                 input logic         pch_read,
                 input logic         pch_write,
                 input logic  [7:0]  pch_writedata,
                 output logic [31:0] pch_readdata,
                 input logic  [7:0]  pch_address,
                 output logic        pch_readdatavalid,
                 output logic        pch_waitrequest,
                 output logic        is_pch_mctp_packet,
                 output logic        nack_mctp_pch_packet,
                 output logic        pch_invalid_cmd,
                 input logic         pch_smbus_stop,

                 // BMC interface. This is translated from SMBus to AVMM using i2c_slave.sv
                 input logic         bmc_read,
                 input logic         bmc_write,
                 input logic  [7:0]  bmc_writedata,
                 output logic [31:0] bmc_readdata,
                 input logic  [7:0]  bmc_address,
                 output logic        bmc_readdatavalid,
                 output logic        bmc_waitrequest,
                 output logic        is_bmc_mctp_packet,
                 output logic        nack_mctp_bmc_packet,
                 output logic        bmc_invalid_cmd,
                 input logic         bmc_smbus_stop,
                 
                 // Hierarchical interface. This is translated from SMBus to AVMM using i2c_slave.sv
                 input logic         pcie_read,
                 input logic         pcie_write,
                 input logic  [7:0]  pcie_writedata,
                 output logic [31:0] pcie_readdata,
                 input logic  [7:0]  pcie_address,
                 output logic        pcie_readdatavalid,
                 output logic        pcie_waitrequest,
                 output logic        is_pcie_mctp_packet,
                 output logic        nack_mctp_pcie_packet,
                 output logic        pcie_invalid_cmd,
                 input logic         pcie_smbus_stop

                 );


    // Signals for keeping track of when data is ready
    // used to determine if pch_readdata should be fed by the fifo output or the dual port ram output and when to set pch_readatavalid
    logic                            pch_readdatavalid_reg;
    logic                            pch_readdatavalid_next;

    // used to determine if BMC_readata should be fed by the fifo output or the dual port ram output and when to set bmc_readatavalid
    logic                            bmc_readdatavalid_reg;
    logic                            bmc_readdatavalid_next;
    
    // used to determine if HPFR_readata should be fed by the fifo output or the dual port ram output and when to set pcie_readatavalid
    logic                            pcie_readdatavalid_reg;
    logic                            pcie_readdatavalid_next;
    
    // used to determine when data from the fifo is read
    // fifo data out is muxed with the output of the ram
    logic                            fifo_readvalid_reg;
    logic                            fifo_readvalid_next;

    // output of port a of the dual port ram
    logic [7:0]                      ram_porta_readdata;

    // inputs of port b of the dual port ram
    logic                            ram_portb_wren;
    logic [7:0]                      ram_portb_writedata;
    logic [7:0]                      ram_portb_address;

    // output of port b of the dual port ram
    logic [7:0]                      ram_portb_readdata;

    // used to tell if the pch or bmc is being served now
    // if a transaction is not being served, waitrequest is asserted 
    logic                            pch_transaction_active;
    logic                            bmc_transaction_active;
    logic                            pcie_transaction_active;
    
    // output of the address protection module, high if the address can be written to and low otherwise
    // signal is ignored for write transactions
    logic                            pch_address_writable;
    logic                            bmc_address_writable;
    logic                            pcie_address_writable;
    
    // input to the fifo, all interfaces share a single fifo and if both the Nios and te pch/bmc try to use it at the same time it can get corrupted
    // the fifo should only be used in the factory durring provisioning so we are guaranteed a "nice" behavior
    logic [7:0]                      fifo_data_in;
    logic                            fifo_dequeue;
    logic                            fifo_enqueue;
    logic                            fifo_clear;
    
    // output of the fifo
    logic [7:0]                      fifo_data_out;
    logic [7:0]                      mctp_fifo_data_out;
    
    logic pchwrite_to_mctpfifo, bmcwrite_to_mctpfifo, pciewrite_to_mctpfifo;
    logic mctp_packet_eop;
    // to store the packet available flag for 8 entries of packets
    logic [3:0] mctp_fifo_packets_avail;
    logic [3:0] mctp_rx_packet_error;
    logic [7:0] pch_mctp_databyte_cnt, bmc_mctp_databyte_cnt, pcie_mctp_databyte_cnt;
    logic [7:0] mctp_messageflag_byte;
    logic [7:0] mctp_fifo_data_in;
    logic mctp_fifo_enqueue, mctp_fifo_dequeue;
    logic mctp_bmc_waitrequest, mctp_pch_waitrequest, mctp_pcie_waitrequest; 
    logic unexpected_stop_error, error_condition;
    logic mctp_fifo_readvalid_reg, mctp_fifo_readvalid_next;
    logic mctp_fifoavail_readvalid_reg, mctp_fifoavail_readvalid_next;
    logic pch_mctp_eom, bmc_mctp_eom, pcie_mctp_eom;
    
    //delays 1 clock cycle while waiting for the ram or fifo to respond to the read request
    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            pch_readdatavalid  <= 1'b0;
            bmc_readdatavalid  <= 1'b0;
            pcie_readdatavalid <= 1'b0;
        end else begin
            pch_readdatavalid  <= pch_read & ~pch_waitrequest;
            bmc_readdatavalid  <= bmc_read & ~bmc_waitrequest;
            pcie_readdatavalid <= pcie_read & ~pcie_waitrequest;
        end
    end

    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            m0_readdatavalid <= 1'b0;
        end
        else begin
            m0_readdatavalid <= m0_read;
        end
    end

    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            fifo_readvalid_reg           <= 1'b0;
            mctp_fifo_readvalid_reg      <= 1'b0;
            mctp_fifoavail_readvalid_reg <= 1'b0;
        end
        else begin
            fifo_readvalid_reg           <= fifo_readvalid_next;
            mctp_fifo_readvalid_reg      <= mctp_fifo_readvalid_next;
            mctp_fifoavail_readvalid_reg <= mctp_fifoavail_readvalid_next;
        end
    end

    // when read or write == 1 and waitrequest == 0 and the address conforms to the rules we are serving that transaction 
    assign pch_transaction_active = (pch_read || (pch_write && pch_address_writable)) && ~pch_waitrequest;
    assign bmc_transaction_active = (bmc_read || (bmc_write && bmc_address_writable)) && ~bmc_waitrequest;
    assign pcie_transaction_active = (pcie_read || (pcie_write && pcie_address_writable)) && ~pcie_waitrequest;


    // pulse if command is invalid and SMBus slave should send a nack
    // signal is routed back up to the SMBus slave module
    always_comb begin
        if (pch_write & ~pch_waitrequest & ~pch_address_writable) begin
            pch_invalid_cmd = 1'b1;
        end
        else begin
            pch_invalid_cmd = 1'b0;
        end
    end

    always_comb begin
        if (bmc_write & ~bmc_waitrequest & ~bmc_address_writable) begin
            bmc_invalid_cmd = 1'b1;
        end
        else begin
            bmc_invalid_cmd = 1'b0;
        end
    end
    
    always_comb begin
        if (pcie_write & ~pcie_waitrequest & ~pcie_address_writable) begin
            pcie_invalid_cmd = 1'b1;
        end
        else begin
            pcie_invalid_cmd = 1'b0;
        end
    end

    always_comb begin
        if (pch_transaction_active) begin
            ram_portb_address = pch_address;
            ram_portb_writedata = pch_writedata;
            ram_portb_wren = pch_write;
        end
        else if (bmc_transaction_active) begin
            ram_portb_address = bmc_address;
            ram_portb_writedata = bmc_writedata;
            ram_portb_wren = bmc_write;
        end
        else if (pcie_transaction_active) begin
            ram_portb_address = pcie_address;
            ram_portb_writedata = pcie_writedata;
            ram_portb_wren = pcie_write;
        end
        else begin
            ram_portb_address = bmc_address;
            ram_portb_writedata = bmc_writedata;
            ram_portb_wren = 1'b0;
        end
    end

    always_comb begin
        pch_readdata[31:8] = 24'h0;       
        if (pch_readdatavalid && fifo_readvalid_reg) begin
            pch_readdata[7:0] = fifo_data_out;
        end
        else begin
            pch_readdata[7:0] = ram_portb_readdata; 
        end

    end

    always_comb begin
        bmc_readdata[31:8] = 24'b0;        
        if (bmc_readdatavalid && fifo_readvalid_reg) begin
            bmc_readdata[7:0] = fifo_data_out;
        end
        else begin
            bmc_readdata[7:0] = ram_portb_readdata;
        end
    end
    
    always_comb begin
        pcie_readdata[31:8] = 24'b0;        
        if (pcie_readdatavalid && fifo_readvalid_reg) begin
            pcie_readdata[7:0] = fifo_data_out;
        end
        else begin
            pcie_readdata[7:0] = ram_portb_readdata;
        end
    end

    always_comb begin
        fifo_readvalid_next = 1'b0;
        if ((m0_read && (m0_address == platform_defs_pkg::WRITE_FIFO_ADDR || m0_address == platform_defs_pkg::READ_FIFO_ADDR))
            ||(pch_read && pch_transaction_active && (pch_address == platform_defs_pkg::WRITE_FIFO_ADDR || pch_address == platform_defs_pkg::READ_FIFO_ADDR)) 
            ||(bmc_read && bmc_transaction_active && (bmc_address == platform_defs_pkg::WRITE_FIFO_ADDR || bmc_address == platform_defs_pkg::READ_FIFO_ADDR))
            ||(pcie_read && pcie_transaction_active && (pcie_address == platform_defs_pkg::WRITE_FIFO_ADDR || pcie_address == platform_defs_pkg::READ_FIFO_ADDR))
            ) 
        begin
            fifo_readvalid_next = 1'b1;
        end
    end

    always_comb begin
        mctp_fifo_readvalid_next = 1'b0;
        if (m0_address == platform_defs_pkg::MCTP_FIFO_ADDR) 
        begin
            mctp_fifo_readvalid_next = 1'b1;
        end
    end
    
    always_comb begin
        mctp_fifoavail_readvalid_next = 1'b0;
        if (m0_address == platform_defs_pkg::MCTP_FIFO_AVAIL_ADDR) 
        begin
            mctp_fifoavail_readvalid_next = 1'b1;
        end
    end

    // mux driving m0_readdata
    // driven by RAM unless the read was from the FIFO
    always_comb begin
        m0_readdata[31:8] = 24'h0;
        if (m0_readdatavalid && fifo_readvalid_reg) begin
            m0_readdata[7:0] = fifo_data_out;
        end
        else if (m0_readdatavalid && mctp_fifo_readvalid_reg) begin
            m0_readdata[7:0] = mctp_fifo_data_out;
        end
        else if (m0_readdatavalid && mctp_fifoavail_readvalid_reg) begin
            m0_readdata[7:0] = {mctp_rx_packet_error, mctp_fifo_packets_avail};
        end
        else begin
            m0_readdata[7:0] = ram_porta_readdata;
        end
    end

    // logic for determining the push and pop signals going into the fifo
    // data_in is driven by m0_writedata by default and is only driven by the pch/bmc when they read or write to 0x0B of 0x0C
    always_comb begin
        fifo_data_in = m0_writedata;
        fifo_enqueue = 1'b0;
        fifo_dequeue = 1'b0;
        if (m0_address == platform_defs_pkg::WRITE_FIFO_ADDR && m0_write) begin
            fifo_enqueue = 1'b1;
        end 
        else if (m0_address == platform_defs_pkg::READ_FIFO_ADDR && m0_read) begin
            fifo_dequeue = 1'b1;
        end
        else if (pch_address == platform_defs_pkg::WRITE_FIFO_ADDR && pch_write && pch_transaction_active) begin
            fifo_enqueue = 1'b1;
            fifo_data_in = pch_writedata;
        end
        else if (pch_address == platform_defs_pkg::READ_FIFO_ADDR && pch_read && pch_transaction_active) begin
            fifo_dequeue = 1'b1;
        end
        else if (bmc_address == platform_defs_pkg::WRITE_FIFO_ADDR && bmc_write && bmc_transaction_active) begin
            fifo_enqueue = 1'b1;
            fifo_data_in = bmc_writedata;
        end
        else if (bmc_address == platform_defs_pkg::READ_FIFO_ADDR && bmc_read && bmc_transaction_active) begin
            fifo_dequeue = 1'b1;
        end
        else if (pcie_address == platform_defs_pkg::WRITE_FIFO_ADDR && pcie_write && pcie_transaction_active) begin
            fifo_enqueue = 1'b1;
            fifo_data_in = bmc_writedata;
        end
        else if (pcie_address == platform_defs_pkg::READ_FIFO_ADDR && pcie_read && pcie_transaction_active) begin
            fifo_dequeue = 1'b1;
        end
    end

    // Clears the fifo if bits [2] or [1] are set in address 0xA
    // This is managed in hardware to simplifiy the software requirments of the NIOS
    always_comb begin
        fifo_clear = 1'b0;
        if (m0_address == platform_defs_pkg::COMMAND_TRIGGER_ADDR && m0_write && (|m0_writedata[2:1])
            || bmc_address == platform_defs_pkg::COMMAND_TRIGGER_ADDR && bmc_write && bmc_transaction_active && (|bmc_writedata[2:1])
            || pch_address == platform_defs_pkg::COMMAND_TRIGGER_ADDR && pch_write && pch_transaction_active && (|pch_writedata[2:1])
            || pcie_address == platform_defs_pkg::COMMAND_TRIGGER_ADDR && pcie_write && pcie_transaction_active && (|pcie_writedata[2:1])) begin
            fifo_clear = 1'b1;
        end
    end

    // pass the addresses from the SMBus slaves in to determine if they are allowed to write to those addresses
    // conditions can be edited inside platform_defs_pkg.sv
    assign pch_address_writable = platform_defs_pkg::pch_mailbox_writable_address(pch_address);
    assign bmc_address_writable = platform_defs_pkg::bmc_mailbox_writable_address(bmc_address);
    assign pcie_address_writable = platform_defs_pkg::pcie_mailbox_writable_address(pcie_address);
    
    // assert waitrequest on the bus that is not currently being served
    // only relevant if 2 transactions come in simultaneously
    // priority goes pch -> bmc -> other bus
    always_comb begin
        bmc_waitrequest = 1'b0;
        pch_waitrequest = 1'b0;
        pcie_waitrequest = 1'b0;
        if (pch_read | pch_write) begin
            bmc_waitrequest  = 1'b1;
            pcie_waitrequest = 1'b1;
        end
        else if (bmc_read | bmc_write) begin
            pch_waitrequest  = 1'b1;
            pcie_waitrequest = 1'b1;
        end
        else if (pcie_read | pcie_write) begin
            bmc_waitrequest  = 1'b1;
            pch_waitrequest  = 1'b1;
        end
    end

    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            mctp_bmc_waitrequest  <= 1'b0;
            mctp_pch_waitrequest  <= 1'b0;
            mctp_pcie_waitrequest <= 1'b0;
        end
        else begin
            if (pchwrite_to_mctpfifo || pciewrite_to_mctpfifo) begin
               mctp_bmc_waitrequest  <= 1'b1;
            end
            else if ((pch_mctp_databyte_cnt == 8'd0 && pch_mctp_eom) || (pcie_mctp_databyte_cnt == 8'd0 && pcie_mctp_eom)) begin
               mctp_bmc_waitrequest  <= 1'b0;
            end      
            
            if (pchwrite_to_mctpfifo || bmcwrite_to_mctpfifo) begin
               mctp_pcie_waitrequest  <= 1'b1;
            end
            else if ((pch_mctp_databyte_cnt == 8'd0 && pch_mctp_eom) || (bmc_mctp_databyte_cnt == 8'd0 && bmc_mctp_eom)) begin
               mctp_pcie_waitrequest  <= 1'b0;
            end  
            
            if (pciewrite_to_mctpfifo || bmcwrite_to_mctpfifo) begin
               mctp_pch_waitrequest  <= 1'b1;
            end
            else if ((pcie_mctp_databyte_cnt == 8'd0 && pcie_mctp_eom) || (bmc_mctp_databyte_cnt == 8'd0 && bmc_mctp_eom)) begin
               mctp_pch_waitrequest  <= 1'b0;
            end
        end
    end
	
	assign pchwrite_to_mctpfifo = (pch_address == platform_defs_pkg::MCTP_FIFO_ADDR) && pch_write && ~mctp_pch_waitrequest;
	assign bmcwrite_to_mctpfifo = (bmc_address == platform_defs_pkg::MCTP_FIFO_ADDR) && bmc_write && ~mctp_bmc_waitrequest;
	assign pciewrite_to_mctpfifo = (pcie_address == platform_defs_pkg::MCTP_FIFO_ADDR) && pcie_write && ~mctp_pcie_waitrequest;
	assign is_pch_mctp_packet = (pch_mctp_databyte_cnt > 8'd0);
	assign is_bmc_mctp_packet = (bmc_mctp_databyte_cnt > 8'd0);
	assign is_pcie_mctp_packet = (pcie_mctp_databyte_cnt > 8'd0);
	assign unexpected_stop_error = (is_pch_mctp_packet && pch_smbus_stop) || (is_bmc_mctp_packet && bmc_smbus_stop) || (is_pcie_mctp_packet && pcie_smbus_stop);
    assign error_condition = pec_error || unexpected_stop_error;
	
    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            nack_mctp_pch_packet  <= 1'b0;
            nack_mctp_bmc_packet  <= 1'b0;
            nack_mctp_pcie_packet <= 1'b0;
        end
        else begin // NACK when (&mctp_fifo_packets_avail == 1'b1) because FIFO is allowed to store up to 4 entries packets only
            if ((pch_address == platform_defs_pkg::MCTP_FIFO_ADDR) && pch_write) begin
                if (mctp_pch_waitrequest || (&mctp_fifo_packets_avail == 1'b1)) begin 
                    nack_mctp_pch_packet <= 1'b1;
                end
                else begin
                    nack_mctp_pch_packet <= 1'b0;
                end
            end
            else if ((bmc_address == platform_defs_pkg::MCTP_FIFO_ADDR) && bmc_write) begin
                if (mctp_bmc_waitrequest || (&mctp_fifo_packets_avail == 1'b1)) begin 
                    nack_mctp_bmc_packet <= 1'b1;
                end
                else begin
                    nack_mctp_bmc_packet <= 1'b0;
                end
            end
            else if ((pcie_address == platform_defs_pkg::MCTP_FIFO_ADDR) && pcie_write) begin
                if (mctp_pcie_waitrequest || (&mctp_fifo_packets_avail == 1'b1)) begin 
                    nack_mctp_pcie_packet <= 1'b1;
                end
                else begin
                    nack_mctp_pcie_packet <= 1'b0;
                end
            end
        end
    end
	
    always_comb begin
        mctp_fifo_data_in = m0_writedata;
        mctp_fifo_enqueue = 1'b0;
        mctp_fifo_dequeue = 1'b0;
        if (m0_address == platform_defs_pkg::MCTP_FIFO_ADDR && m0_read) begin  //NIOS is not able to write to this MCTP_FIFO
            mctp_fifo_dequeue = 1'b1;
        end
        else if (pchwrite_to_mctpfifo && pch_mctp_databyte_cnt != 8'd1) begin   // dont push last data byte (PEC into the FIFO)
            mctp_fifo_enqueue = 1'b1;
            mctp_fifo_data_in = pch_writedata;
        end
        else if (bmcwrite_to_mctpfifo && bmc_mctp_databyte_cnt != 8'd1) begin
            mctp_fifo_enqueue = 1'b1;
            mctp_fifo_data_in = bmc_writedata;
        end
        else if (pciewrite_to_mctpfifo && pcie_mctp_databyte_cnt != 8'd1) begin
            mctp_fifo_enqueue = 1'b1;
            mctp_fifo_data_in = pcie_writedata;
        end
    end
    
    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            pch_mctp_eom  <= 1'b0;
            bmc_mctp_eom  <= 1'b0;
            pcie_mctp_eom <= 1'b0;
        end
        else begin
            if (pchwrite_to_mctpfifo && (pch_mctp_databyte_cnt == mctp_messageflag_byte)) begin
                if (pch_writedata[6]) pch_mctp_eom  <= 1'b1;             
            end
            else if ((pch_mctp_databyte_cnt == 8'd0 && pch_mctp_eom) || unexpected_stop_error) begin
                pch_mctp_eom  <= 1'b0;
            end
                
            if (bmcwrite_to_mctpfifo && (bmc_mctp_databyte_cnt == mctp_messageflag_byte)) begin
                if (bmc_writedata[6]) bmc_mctp_eom  <= 1'b1;
            end
            else if ((bmc_mctp_databyte_cnt == 8'd0 && bmc_mctp_eom) || unexpected_stop_error) begin
                bmc_mctp_eom  <= 1'b0;
            end
                
            if (pciewrite_to_mctpfifo && (pcie_mctp_databyte_cnt == mctp_messageflag_byte)) begin
                if (pcie_writedata[6]) pcie_mctp_eom  <= 1'b1;
            end
            else if ((pcie_mctp_databyte_cnt == 8'd0 && pcie_mctp_eom) || unexpected_stop_error) begin
                pcie_mctp_eom  <= 1'b0;
            end
        end
    end

    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            pch_mctp_databyte_cnt <= 8'd0;
            bmc_mctp_databyte_cnt <= 8'd0;
            pcie_mctp_databyte_cnt <= 8'd0;
            mctp_messageflag_byte  <= 8'd0;
        end
        else if (pchwrite_to_mctpfifo) begin
            if (pch_mctp_databyte_cnt == 8'd0) begin
                pch_mctp_databyte_cnt <= pch_writedata + 8'd1; //the first databyte is the MCTP packet bytecount, +1 for PEC
                mctp_messageflag_byte <= pch_writedata - 8'd4; //message flag byte is the 5th byte
            end
            else begin
                pch_mctp_databyte_cnt <= pch_mctp_databyte_cnt - 8'd1;
            end
        end
        else if (bmcwrite_to_mctpfifo) begin
            if (bmc_mctp_databyte_cnt == 8'd0) begin
                bmc_mctp_databyte_cnt <= bmc_writedata + 8'd1; //the first databyte is the MCTP packet bytecount, +1 for PEC
                mctp_messageflag_byte <= bmc_writedata - 8'd4; //message flag byte is the 5th byte
            end
            else begin
                bmc_mctp_databyte_cnt <= bmc_mctp_databyte_cnt - 8'd1;
            end
        end
        else if (pciewrite_to_mctpfifo) begin
            if (pcie_mctp_databyte_cnt == 8'd0) begin
                pcie_mctp_databyte_cnt <= pcie_writedata + 8'd1; //the first databyte is the MCTP packet bytecount, +1 for PEC
                mctp_messageflag_byte  <= pcie_writedata - 8'd4; //message flag byte is the 5th byte
            end
            else begin
                pcie_mctp_databyte_cnt <= pcie_mctp_databyte_cnt - 8'd1;
            end
        end
        else if (unexpected_stop_error) begin
            pch_mctp_databyte_cnt <= 8'd0;
            bmc_mctp_databyte_cnt <= 8'd0;
            pcie_mctp_databyte_cnt <= 8'd0;
            mctp_messageflag_byte  <= 8'd0;
        end
    end

    // mctp_fifo_packets_avail register (W1C, RO)
	// Sets this register when a complete packet is in the FIFO and with no PEC error
    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            mctp_packet_eop          <= 1'b0;
			mctp_fifo_packets_avail  <= 4'b0000;
            mctp_rx_packet_error     <= 4'b0000;
        end
        else begin
            if ((pchwrite_to_mctpfifo && pch_mctp_databyte_cnt == 8'd1) || (bmcwrite_to_mctpfifo && bmc_mctp_databyte_cnt == 8'd1) || (pciewrite_to_mctpfifo && pcie_mctp_databyte_cnt == 8'd1)) begin
                mctp_packet_eop  <= 1'b1;
            end
            else begin
                mctp_packet_eop  <= 1'b0;
            end
			
            if (mctp_packet_eop) begin 
                mctp_fifo_packets_avail  <= {mctp_fifo_packets_avail[2:0], 1'b1};
                
                if (error_condition) begin
                    mctp_rx_packet_error  <= {mctp_rx_packet_error[2:0], 1'b1};
                end
                else begin
                    mctp_rx_packet_error  <= {mctp_rx_packet_error[2:0], 1'b0};
                end
            end
            
            if (m0_address == platform_defs_pkg::MCTP_FIFO_AVAIL_ADDR && m0_write && (m0_writedata == 8'd1)) begin
                mctp_fifo_packets_avail  <= {1'b0, mctp_fifo_packets_avail[3:1]};
                mctp_rx_packet_error     <= {1'b0, mctp_rx_packet_error[3:1]};
            end
        end
    end

    // use a Dual port ram to act as the register file
    dp_ram dp_ram_0 (
                     .address_a(m0_address),
                     .address_b(ram_portb_address),
                     .clock(clk),
                     .data_a(m0_writedata),
                     .data_b(ram_portb_writedata),
                     .wren_a(m0_write),
                     .wren_b(ram_portb_wren),
                     .q_a(ram_porta_readdata),
                     .q_b(ram_portb_readdata)

                     );

    // Fifo 
    // anything less than 8x1023 will result in using 1 M9k
    fifo #(
            .DATA_WIDTH(8),
            .DATA_DEPTH(platform_defs_pkg::SMBUS_MAILBOX_FIFO_DEPTH)
            ) 
    u_fifo 
        (
            .clk(clk),
            .resetn(resetn),
            .data_in(fifo_data_in),
            .data_out(fifo_data_out),
            .clear(fifo_clear),
            .enqueue(fifo_enqueue),
            .dequeue(fifo_dequeue)
        );
        
    // Fifo to store MCTP packet 
    // anything less than 8x1023 will result in using 1 M9k
    fifo #(
            .DATA_WIDTH(8),
            .DATA_DEPTH(platform_defs_pkg::SMBUS_MAILBOX_MCTP_FIFO_DEPTH)
            ) 
    u_mctp_fifo 
        (
            .clk(clk),
            .resetn(resetn),
            .data_in(mctp_fifo_data_in),
            .data_out(mctp_fifo_data_out),
            .clear(1'b0),
            .enqueue(mctp_fifo_enqueue),
            .dequeue(mctp_fifo_dequeue)
        );

endmodule