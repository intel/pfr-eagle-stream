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
                 input logic         pch_pec_error,
                 input logic  [7:0]  pch_writedata,
                 output logic [31:0] pch_readdata,
                 input logic  [7:0]  pch_address,
                 output logic        pch_readdatavalid,
                 output logic        pch_waitrequest,
                 output logic        is_pch_mctp_pkt,
                 output logic        nack_pch_mctp_pkt,
                 output logic        pch_invalid_cmd,
                 input logic         pch_smbus_stop,
                 input logic         pch_smbus_start,

                 // BMC interface. This is translated from SMBus to AVMM using i2c_slave.sv
                 input logic         bmc_read,
                 input logic         bmc_write,
                 input logic         bmc_pec_error,
                 input logic  [7:0]  bmc_writedata,
                 output logic [31:0] bmc_readdata,
                 input logic  [7:0]  bmc_address,
                 output logic        bmc_readdatavalid,
                 output logic        bmc_waitrequest,
                 output logic        is_bmc_mctp_pkt,
                 output logic        nack_bmc_mctp_pkt,
                 output logic        bmc_invalid_cmd,
                 input logic         bmc_smbus_stop,
                 input logic         bmc_smbus_start,
                 
                 // Hierarchical interface. This is translated from SMBus to AVMM using i2c_slave.sv
                 input logic         pcie_read,
                 input logic         pcie_write,
                 input logic         pcie_pec_error,
                 input logic  [7:0]  pcie_writedata,
                 output logic [31:0] pcie_readdata,
                 input logic  [7:0]  pcie_address,
                 output logic        pcie_readdatavalid,
                 output logic        pcie_waitrequest,
                 output logic        is_pcie_mctp_pkt,
                 output logic        nack_pcie_mctp_pkt,
                 output logic        pcie_invalid_cmd,
                 input logic         pcie_smbus_stop,
                 input logic         pcie_smbus_start
                
                 );
                 
    localparam MAXIMUM_NUM_MCTP_MSG = 5'd16;
    localparam MCTP_MSGFLAG_BYTE    = 3'd6;


    // Signals for keeping track of when data is ready
    // used to determine if pch_readdata should be fed by the fifo output or the dual port ram output and when to set pch_readatavalid
    logic                            pch_readdatavalid_reg;
    logic                            pch_readdatavalid_nxt;

    // used to determine if BMC_readata should be fed by the fifo output or the dual port ram output and when to set bmc_readatavalid
    logic                            bmc_readdatavalid_reg;
    logic                            bmc_readdatavalid_nxt;
    
    // used to determine if HPFR_readata should be fed by the fifo output or the dual port ram output and when to set pcie_readatavalid
    logic                            pcie_readdatavalid_reg;
    logic                            pcie_readdatavalid_nxt;
    
    // used to determine when data from the fifo is read
    // fifo data out is muxed with the output of the ram
    logic                            fifo_rdvalid_reg;
    logic                            fifo_rdvalid_nxt;
    
    logic                            m0_fifo_rdvalid_reg;
    logic                            m0_bios_fifo_rdvalid_reg;
    logic                            m0_fifo_rdvalid_nxt;
    logic                            m0_bios_fifo_status_rdvalid_reg;
    logic                            m0_bios_fifo_status_rdvalid_nxt;
    logic                            m0_bios_fifo_rdvalid_nxt;
    
    logic                            mctp_fifo_rdvalid_reg;
    logic                            mctp_fifo_rdvalid_nxt;
    
    logic                            m0_mctp_fifo_rdvalid_reg;
    logic                            m0_mctp_fifo_rdvalid_nxt;

    // output of port a of the dual port ram
    logic [7:0]                      ram_porta_readdata;

    // inputs of port b of the dual port ram
    logic                            ram_portb_wren;
    logic [7:0]                      ram_portb_writedata;
    logic [7:0]                      ram_portb_address;

    // output of port b of the dual port ram
    logic [7:0]                      ram_portb_readdata;
    
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

    logic [7:0]                      bios_fifo_data_in;
    logic                            bios_fifo_wr;
    logic                            bios_fifo_rd;
    logic                            bios_fifo_clear;
    logic                            bios_fifo_empty;
    logic                            bios_fifo_full;
    logic [7:0]                      bios_status_reg;
    // output of the fifo
    logic [7:0]                      bios_fifo_data_out;

    // for MCTP related
    logic pch_write_to_mctpfifo;
    logic pch_check_mctp_msgflag;
    logic pch_mctp_fifo_full;
    logic [3:0] pch_mctp_fifoavail;
    logic [3:0] pch_mctp_rxpkt_error;
    logic [8:0] pch_mctp_databyte_cnt;
    logic [7:0] pch_update_intent;
    logic [7:0] pch_update_intent_part2;
    logic [7:0] pch_mctp_fifo_data_in;
    logic [7:0] pch_mctp_fifo_data_out;
    logic [7:0] pch_mctp_bytecount;
    logic [7:0] pch_mctp_bytecount_reg[0:3];
    logic [2:0] pch_mctp_msgflag_cnt;
    logic pch_mctp_fifo_enqueue;
    logic pch_mctp_fifo_dequeue;
    logic pch_mctp_fifo_rdvalid_reg;
    logic pch_mctp_fifo_rdvalid_nxt;
    logic pch_mctp_bc_rdvalid_reg;
    logic pch_mctp_bc_rdvalid_nxt;
    logic pch_undersize_pkt_err;
    logic pch_oversize_pkt_err;
    logic pch_error_condition;
    logic pch_mctp_fifoavail_rdvalid_reg;
    logic pch_mctp_fifoavail_rdvalid_nxt;
    logic pch_mctp_som;
    logic pch_mctp_eom;
    logic pch_mctp_wait4stop;
    logic pch_invalid_long_msg;
    logic pch_invalid_last_bc; 
    logic pch_invalid_middle_bc; 
    logic [7:0] pch_mctp_som_bc;     
    logic [5:0] pch_mctp_msg_cnt;
    
    logic bmc_write_to_mctpfifo; 
    logic bmc_check_mctp_msgflag;
    logic bmc_mctp_fifo_full;
    logic [3:0] bmc_mctp_fifoavail;
    logic [3:0] bmc_mctp_rxpkt_error;
    logic [8:0] bmc_mctp_databyte_cnt;
    logic [7:0] bmc_update_intent;
    logic [7:0] bmc_update_intent_part2;
    logic [7:0] bmc_mctp_fifo_data_in;
    logic [7:0] bmc_mctp_fifo_data_out;
    logic [7:0] bmc_mctp_bytecount;
    logic [7:0] bmc_mctp_bytecount_reg[0:3];
    logic [2:0] bmc_mctp_msgflag_cnt;
    logic bmc_mctp_fifo_enqueue;
    logic bmc_mctp_fifo_dequeue;
    logic bmc_mctp_fifo_rdvalid_reg;
    logic bmc_mctp_fifo_rdvalid_nxt;
    logic bmc_mctp_bc_rdvalid_reg;
    logic bmc_mctp_bc_rdvalid_nxt;
    logic bmc_undersize_pkt_err;
    logic bmc_oversize_pkt_err;
    logic bmc_error_condition;
    logic bmc_mctp_fifoavail_rdvalid_reg;
    logic bmc_mctp_fifoavail_rdvalid_nxt;
    logic bmc_mctp_som;
    logic bmc_mctp_eom;
    logic bmc_mctp_wait4stop;
    logic bmc_invalid_long_msg;
    logic bmc_invalid_last_bc;
    logic bmc_invalid_middle_bc;
    logic [7:0] bmc_mctp_som_bc;    
    logic [5:0] bmc_mctp_msg_cnt;
        
    logic pcie_write_to_mctpfifo;
    logic pcie_check_mctp_msgflag;
    logic pcie_mctp_fifo_full;
    logic [3:0] pcie_mctp_fifoavail;
    logic [3:0] pcie_mctp_rxpkt_error;
    logic [8:0] pcie_mctp_databyte_cnt;
    logic [7:0] pcie_mctp_fifo_data_in;
    logic [7:0] pcie_mctp_fifo_data_out;
    logic [7:0] pcie_mctp_bytecount;
    logic [7:0] pcie_mctp_bytecount_reg[0:3];
    logic [2:0] pcie_mctp_msgflag_cnt;
    logic pcie_mctp_fifo_enqueue;
    logic pcie_mctp_fifo_dequeue;
    logic pcie_mctp_fifo_rdvalid_reg;
    logic pcie_mctp_fifo_rdvalid_nxt;
    logic pcie_mctp_bc_rdvalid_reg;
    logic pcie_mctp_bc_rdvalid_nxt;
    logic pcie_undersize_pkt_err;
    logic pcie_oversize_pkt_err;
    logic pcie_error_condition;
    logic pcie_mctp_fifoavail_rdvalid_reg;
    logic pcie_mctp_fifoavail_rdvalid_nxt;
    logic pcie_mctp_som;
    logic pcie_mctp_eom;
    logic pcie_mctp_wait4stop;
    logic pcie_invalid_long_msg;
    logic pcie_invalid_last_bc;
    logic pcie_invalid_middle_bc;
    logic [7:0] pcie_mctp_som_bc;
    logic [5:0] pcie_mctp_msg_cnt;
    logic bmc_error_condition_combi, pch_error_condition_combi, pcie_error_condition_combi;
    logic bmc_last_valid_msg, pch_last_valid_msg, pcie_last_valid_msg;
    logic bmc_trigger_nack, pch_trigger_nack, pcie_trigger_nack;
    
    assign pch_check_mctp_msgflag = (pch_mctp_msgflag_cnt == MCTP_MSGFLAG_BYTE);
    assign bmc_check_mctp_msgflag = (bmc_mctp_msgflag_cnt == MCTP_MSGFLAG_BYTE);
    assign pcie_check_mctp_msgflag = (pcie_mctp_msgflag_cnt == MCTP_MSGFLAG_BYTE);
	assign pch_write_to_mctpfifo = (pch_address == platform_defs_pkg::MCTP_WRITE_FIFO_ADDR) && pch_write && ~pch_waitrequest && ~pch_mctp_fifo_full;
	assign bmc_write_to_mctpfifo = (bmc_address == platform_defs_pkg::MCTP_WRITE_FIFO_ADDR) && bmc_write && ~bmc_waitrequest && ~bmc_mctp_fifo_full;
	assign pcie_write_to_mctpfifo = (pcie_address == platform_defs_pkg::MCTP_WRITE_FIFO_ADDR) && pcie_write && ~pcie_waitrequest && ~pcie_mctp_fifo_full;
	assign pch_undersize_pkt_err = ((pch_mctp_databyte_cnt > 9'd0) && (pch_smbus_stop || pch_smbus_start));
	assign bmc_undersize_pkt_err = ((bmc_mctp_databyte_cnt > 9'd0) && (bmc_smbus_stop || bmc_smbus_start));
	assign pcie_undersize_pkt_err = ((pcie_mctp_databyte_cnt > 9'd0) && (pcie_smbus_stop || pcie_smbus_start));
    assign pch_oversize_pkt_err = (pch_write_to_mctpfifo && (pch_mctp_databyte_cnt == 9'd0) && pch_mctp_wait4stop);  
    assign bmc_oversize_pkt_err = (bmc_write_to_mctpfifo && (bmc_mctp_databyte_cnt == 9'd0) && bmc_mctp_wait4stop);
    assign pcie_oversize_pkt_err = (pcie_write_to_mctpfifo && (pcie_mctp_databyte_cnt == 9'd0) && pcie_mctp_wait4stop);
    assign pch_last_valid_msg = ((pch_mctp_msg_cnt == MAXIMUM_NUM_MCTP_MSG) && pch_smbus_stop);
    assign bmc_last_valid_msg = ((bmc_mctp_msg_cnt == MAXIMUM_NUM_MCTP_MSG) && bmc_smbus_stop);
    assign pcie_last_valid_msg = ((pcie_mctp_msg_cnt == MAXIMUM_NUM_MCTP_MSG) && pcie_smbus_stop);
    assign is_pch_mctp_pkt = pch_mctp_wait4stop;
    assign is_bmc_mctp_pkt = bmc_mctp_wait4stop;
    assign is_pcie_mctp_pkt = pcie_mctp_wait4stop;
    assign pch_mctp_fifo_full = (&pch_mctp_fifoavail == 1'b1);
    assign bmc_mctp_fifo_full = (&bmc_mctp_fifoavail == 1'b1);
    assign pcie_mctp_fifo_full = (&pcie_mctp_fifoavail == 1'b1);
    assign pch_mctp_som  = pch_writedata[7];
    assign bmc_mctp_som  = bmc_writedata[7];
    assign pcie_mctp_som = pcie_writedata[7];
    assign pch_mctp_eom  = pch_writedata[6];
    assign bmc_mctp_eom  = bmc_writedata[6];
    assign pcie_mctp_eom = pcie_writedata[6];
    assign bmc_error_condition_combi = bmc_error_condition || bmc_undersize_pkt_err || bmc_pec_error || bmc_last_valid_msg;    
    assign pch_error_condition_combi = pch_error_condition || pch_undersize_pkt_err || pch_pec_error || pch_last_valid_msg;
    assign pcie_error_condition_combi = pcie_error_condition || pcie_undersize_pkt_err || pcie_pec_error || pcie_last_valid_msg;
    
    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            pch_invalid_long_msg  <= 1'b0;
            bmc_invalid_long_msg  <= 1'b0;
            pcie_invalid_long_msg <= 1'b0;
            pch_trigger_nack      <= 1'b0;
            bmc_trigger_nack      <= 1'b0;
            pcie_trigger_nack     <= 1'b0;
        end else begin
            if (pch_last_valid_msg) begin
                pch_invalid_long_msg  <= 1'b1;
            end
            else if (pch_smbus_stop && pch_mctp_msg_cnt == 6'd0) begin
                pch_invalid_long_msg  <= 1'b0;
            end
            
            if (pch_invalid_long_msg && pch_check_mctp_msgflag) begin
                pch_trigger_nack  <= 1'b1;
            end
            else if (pch_smbus_stop) begin
                pch_trigger_nack  <= 1'b0;
            end
            
            if (bmc_last_valid_msg) begin
                bmc_invalid_long_msg  <= 1'b1;
            end
            else if (bmc_smbus_stop && bmc_mctp_msg_cnt == 6'd0) begin
                bmc_invalid_long_msg  <= 1'b0;
            end
            
            if (bmc_invalid_long_msg && bmc_check_mctp_msgflag) begin
                bmc_trigger_nack  <= 1'b1;
            end
            else if (bmc_smbus_stop) begin
                bmc_trigger_nack  <= 1'b0;
            end
            
            if (pcie_last_valid_msg) begin
                pcie_invalid_long_msg  <= 1'b1;
            end
            else if (pcie_smbus_stop && pcie_mctp_msg_cnt == 6'd0) begin
                pcie_invalid_long_msg  <= 1'b0;
            end
            
            if (pcie_invalid_long_msg && pcie_check_mctp_msgflag) begin
                pcie_trigger_nack  <= 1'b1;
            end
            else if (pcie_smbus_stop) begin
                pcie_trigger_nack  <= 1'b0;
            end
        end
    end
    
    
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
            fifo_rdvalid_reg                <= 1'b0;
            m0_fifo_rdvalid_reg             <= 1'b0;
            m0_bios_fifo_rdvalid_reg        <= 1'b0;
            m0_bios_fifo_status_rdvalid_reg <= 1'b0;
            mctp_fifo_rdvalid_reg           <= 1'b0;
            m0_mctp_fifo_rdvalid_reg        <= 1'b0;
            pch_mctp_fifo_rdvalid_reg       <= 1'b0;
            bmc_mctp_fifo_rdvalid_reg       <= 1'b0;
            pcie_mctp_fifo_rdvalid_reg      <= 1'b0;
            pch_mctp_bc_rdvalid_reg         <= 1'b0;
            bmc_mctp_bc_rdvalid_reg         <= 1'b0;
            pcie_mctp_bc_rdvalid_reg        <= 1'b0;
            pch_mctp_fifoavail_rdvalid_reg  <= 1'b0;
            bmc_mctp_fifoavail_rdvalid_reg  <= 1'b0;
            pcie_mctp_fifoavail_rdvalid_reg <= 1'b0;
        end
        else begin
            fifo_rdvalid_reg                <= fifo_rdvalid_nxt;
            m0_fifo_rdvalid_reg             <= m0_fifo_rdvalid_nxt;
            m0_bios_fifo_rdvalid_reg        <= m0_bios_fifo_rdvalid_nxt;
            m0_bios_fifo_status_rdvalid_reg <= m0_bios_fifo_status_rdvalid_nxt;
            mctp_fifo_rdvalid_reg           <= mctp_fifo_rdvalid_nxt;
            m0_mctp_fifo_rdvalid_reg        <= m0_mctp_fifo_rdvalid_nxt;
            pch_mctp_fifo_rdvalid_reg       <= pch_mctp_fifo_rdvalid_nxt;
            bmc_mctp_fifo_rdvalid_reg       <= bmc_mctp_fifo_rdvalid_nxt;
            pcie_mctp_fifo_rdvalid_reg      <= pcie_mctp_fifo_rdvalid_nxt;
            pch_mctp_bc_rdvalid_reg         <= pch_mctp_bc_rdvalid_nxt;
            bmc_mctp_bc_rdvalid_reg         <= bmc_mctp_bc_rdvalid_nxt;
            pcie_mctp_bc_rdvalid_reg        <= pcie_mctp_bc_rdvalid_nxt;
            pch_mctp_fifoavail_rdvalid_reg  <= pch_mctp_fifoavail_rdvalid_nxt;
            bmc_mctp_fifoavail_rdvalid_reg  <= bmc_mctp_fifoavail_rdvalid_nxt;
            pcie_mctp_fifoavail_rdvalid_reg <= pcie_mctp_fifoavail_rdvalid_nxt;
        end
    end

    // pulse if command is invalid and SMBus slave should send a nack
    // signal is routed back up to the SMBus slave module
    always_comb begin
        if ( (pch_write & ~pch_waitrequest & (~pch_address_writable || (pch_address == platform_defs_pkg::PCH_UPDATE_INTENT_ADDR && ~pch_update_intent[7] && |pch_update_intent[6:0] != 1'b0))) || (pch_write & ~pch_waitrequest & (~pch_address_writable || (pch_address == platform_defs_pkg::PCH_UPDATE_INTENT_PART2_ADDR && ~pch_update_intent_part2[7] && |pch_update_intent_part2[6:0] != 1'b0))) ) begin
            pch_invalid_cmd = 1'b1;
        end
        else begin
            pch_invalid_cmd = 1'b0;
        end
    end

    always_comb begin
        if ( (bmc_write & ~bmc_waitrequest & (~bmc_address_writable || (bmc_address == platform_defs_pkg::BMC_UPDATE_INTENT_ADDR && ~bmc_update_intent[7] && |bmc_update_intent[6:0] != 1'b0))) || (bmc_write & ~bmc_waitrequest & (~bmc_address_writable || (bmc_address == platform_defs_pkg::BMC_UPDATE_INTENT_PART2_ADDR && ~bmc_update_intent_part2[7] && |bmc_update_intent_part2[6:0] != 1'b0))) ) begin
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
        if ((pch_read || (pch_write & pch_address_writable)) & ~pch_waitrequest) begin 
            ram_portb_address = pch_address;
        end
        else begin
            ram_portb_address = bmc_address;
        end
    end
    
    always_comb begin
        if (pch_write & pch_address_writable & ~pch_waitrequest) begin     
        	if (pch_address == platform_defs_pkg::PCH_UPDATE_INTENT_ADDR) begin     //register lock capability for update intent value
        	    if ((|pch_update_intent[7:0] == 1'b0) || (pch_update_intent[7] == 1'b1)) begin
                      ram_portb_wren = 1'b1;
                      ram_portb_writedata = {pch_writedata[7], (pch_update_intent[6:0] | pch_writedata[6:0])};
                    end
                    else begin
                      ram_portb_wren = 1'b0;
                      ram_portb_writedata = pch_writedata;
                    end
        	end else if (pch_address == platform_defs_pkg::PCH_UPDATE_INTENT_PART2_ADDR) begin 
                    if ((|pch_update_intent_part2[7:0] == 1'b0) || (pch_update_intent_part2[7] == 1'b1)) begin
                        ram_portb_wren = 1'b1;
                        ram_portb_writedata = {pch_writedata[7], (pch_update_intent_part2[6:0] | pch_writedata[6:0])};
                    end
                    else begin
                        ram_portb_wren = 1'b0;
                        ram_portb_writedata = pch_writedata;
                    end
        	end else begin
                    ram_portb_wren = 1'b1;
                    ram_portb_writedata = pch_writedata;
                    //------Logic to store BIOS_CHECKPOINT_ADDR in FIFO------------
                    
                    //--------------------------------------------------------------
                end
        end
        else if (bmc_write & bmc_address_writable & ~bmc_waitrequest) begin
        	if (bmc_address == platform_defs_pkg::BMC_UPDATE_INTENT_ADDR) begin    //register lock capability for update intent value
        	    if ((|bmc_update_intent[7:0] == 1'b0) || (bmc_update_intent[7] == 1'b1)) begin
                    ram_portb_wren = 1'b1;
                    ram_portb_writedata = {bmc_writedata[7], (bmc_update_intent[6:0] | bmc_writedata[6:0])};
                end
                else begin
                    ram_portb_wren = 1'b0;
                    ram_portb_writedata = bmc_writedata;
                end
        	end else if (bmc_address == platform_defs_pkg::BMC_UPDATE_INTENT_PART2_ADDR) begin 
                if ((|bmc_update_intent_part2[7:0] == 1'b0) || (bmc_update_intent_part2[7] == 1'b1)) begin
                    ram_portb_wren = 1'b1;
                    ram_portb_writedata = {bmc_writedata[7], (bmc_update_intent_part2[6:0] | bmc_writedata[6:0])};
                end
                else begin
                    ram_portb_wren = 1'b0;
                    ram_portb_writedata = bmc_writedata;
                end
        	end else begin
                ram_portb_wren = 1'b1;
                ram_portb_writedata = bmc_writedata;
            end
        end
        else begin
            ram_portb_writedata = bmc_writedata;
            ram_portb_wren = 1'b0;
        end
    end

    always_comb begin
        pch_readdata[31:8] = 24'h0;       
        if (pch_readdatavalid && fifo_rdvalid_reg) begin
            pch_readdata[7:0] = fifo_data_out;
        end
        else if (pch_readdatavalid && mctp_fifo_rdvalid_reg) begin
            pch_readdata[7:0] = 8'h0;
        end
        else begin
            pch_readdata[7:0] = ram_portb_readdata; 
        end

    end

    always_comb begin
        bmc_readdata[31:8] = 24'b0;        
        if (bmc_readdatavalid && fifo_rdvalid_reg) begin
            bmc_readdata[7:0] = fifo_data_out;
        end
        else if (bmc_readdatavalid && mctp_fifo_rdvalid_reg) begin
            bmc_readdata[7:0] = 8'h0;
        end
        else begin
            bmc_readdata[7:0] = ram_portb_readdata;
        end
    end
    
    always_comb begin
        pcie_readdata[31:8] = 24'b0;        
        if (pcie_readdatavalid && fifo_rdvalid_reg) begin
            pcie_readdata[7:0] = fifo_data_out;
        end
        else if (pcie_readdatavalid && mctp_fifo_rdvalid_reg) begin
            pcie_readdata[7:0] = 8'h0;
        end
        else begin
            pcie_readdata[7:0] = ram_portb_readdata;
        end
    end

    always_comb begin
        fifo_rdvalid_nxt = 1'b0;
        if ((pch_read && ~pch_waitrequest && (pch_address == platform_defs_pkg::WRITE_FIFO_ADDR || pch_address == platform_defs_pkg::READ_FIFO_ADDR)) 
            ||(bmc_read && ~bmc_waitrequest && (bmc_address == platform_defs_pkg::WRITE_FIFO_ADDR || bmc_address == platform_defs_pkg::READ_FIFO_ADDR))
            ) 
        begin
            fifo_rdvalid_nxt = 1'b1;
        end
    end

    always_comb begin
        m0_fifo_rdvalid_nxt = 1'b0;
        if (m0_read && (m0_address == platform_defs_pkg::WRITE_FIFO_ADDR || m0_address == platform_defs_pkg::READ_FIFO_ADDR)) begin
            m0_fifo_rdvalid_nxt = 1'b1;
        end
    end
    
    always_comb begin
        mctp_fifo_rdvalid_nxt = 1'b0;
        if ((pch_read && ~pch_waitrequest && pch_address == platform_defs_pkg::MCTP_WRITE_FIFO_ADDR) 
            ||(bmc_read && ~bmc_waitrequest && bmc_address == platform_defs_pkg::MCTP_WRITE_FIFO_ADDR)
            ||(pcie_read && ~pcie_waitrequest && pcie_address == platform_defs_pkg::MCTP_WRITE_FIFO_ADDR)
            ) 
        begin
            mctp_fifo_rdvalid_nxt = 1'b1;
        end
    end

    always_comb begin
        m0_mctp_fifo_rdvalid_nxt = 1'b0;
        if (m0_read && m0_address == platform_defs_pkg::MCTP_WRITE_FIFO_ADDR) begin
            m0_mctp_fifo_rdvalid_nxt = 1'b1;
        end

    end

    always_comb begin
        if (m0_address == platform_defs_pkg::PCH_MCTP_READ_FIFO_ADDR) begin
            pch_mctp_fifo_rdvalid_nxt = 1'b1;
        end
        else begin
            pch_mctp_fifo_rdvalid_nxt  = 1'b0;
        end
        
        if (m0_address == platform_defs_pkg::BMC_MCTP_READ_FIFO_ADDR) begin
            bmc_mctp_fifo_rdvalid_nxt = 1'b1;
        end
        else begin
            bmc_mctp_fifo_rdvalid_nxt  = 1'b0;
        end
        
        if (m0_address == platform_defs_pkg::PCIE_MCTP_READ_FIFO_ADDR) begin
            pcie_mctp_fifo_rdvalid_nxt = 1'b1;
        end
        else begin
            pcie_mctp_fifo_rdvalid_nxt  = 1'b0;
        end
    end
    
    always_comb begin
        if (m0_address == platform_defs_pkg::PCH_MCTP_BYTECOUNT_ADDR) begin
            pch_mctp_bc_rdvalid_nxt = 1'b1;
        end
        else begin
            pch_mctp_bc_rdvalid_nxt  = 1'b0;
        end
        
        if (m0_address == platform_defs_pkg::BMC_MCTP_BYTECOUNT_ADDR) begin
            bmc_mctp_bc_rdvalid_nxt = 1'b1;
        end
        else begin
            bmc_mctp_bc_rdvalid_nxt  = 1'b0;
        end
        
        if (m0_address == platform_defs_pkg::PCIE_MCTP_BYTECOUNT_ADDR) begin
            pcie_mctp_bc_rdvalid_nxt = 1'b1;
        end
        else begin
            pcie_mctp_bc_rdvalid_nxt  = 1'b0;
        end
    end
    
    always_comb begin
        if (m0_address == platform_defs_pkg::PCH_MCTP_FIFO_AVAIL_ADDR) begin
            pch_mctp_fifoavail_rdvalid_nxt = 1'b1;
        end
        else begin
            pch_mctp_fifoavail_rdvalid_nxt = 1'b0;
        end
        
        if (m0_address == platform_defs_pkg::BMC_MCTP_FIFO_AVAIL_ADDR) begin
            bmc_mctp_fifoavail_rdvalid_nxt = 1'b1;
        end
        else begin
            bmc_mctp_fifoavail_rdvalid_nxt = 1'b0;
        end
        
        if (m0_address == platform_defs_pkg::PCIE_MCTP_FIFO_AVAIL_ADDR) begin
            pcie_mctp_fifoavail_rdvalid_nxt = 1'b1;
        end
        else begin
            pcie_mctp_fifoavail_rdvalid_nxt = 1'b0;
        end
    end

    // mux driving m0_readdata
    // driven by RAM unless the read was from the FIFO
    always_comb begin
        m0_readdata[31:8] = 24'h0;
        if (m0_readdatavalid && m0_fifo_rdvalid_reg) begin
            m0_readdata[7:0] = fifo_data_out;
        end
        else if (m0_readdatavalid && m0_bios_fifo_rdvalid_reg) begin
            m0_readdata[7:0] = bios_fifo_data_out;
        end
        else if (m0_readdatavalid && m0_bios_fifo_status_rdvalid_reg) begin
            m0_readdata[7:0] = bios_status_reg;
        end
        else if (m0_readdatavalid && m0_mctp_fifo_rdvalid_reg) begin
            m0_readdata[7:0] = 8'h0;
        end
        else if (m0_readdatavalid && pch_mctp_fifo_rdvalid_reg) begin
            m0_readdata[7:0] = pch_mctp_fifo_data_out;
        end
        else if (m0_readdatavalid && bmc_mctp_fifo_rdvalid_reg) begin
            m0_readdata[7:0] = bmc_mctp_fifo_data_out;
        end
        else if (m0_readdatavalid && pcie_mctp_fifo_rdvalid_reg) begin
            m0_readdata[7:0] = pcie_mctp_fifo_data_out;
        end
        else if (m0_readdatavalid && pch_mctp_bc_rdvalid_reg) begin
            m0_readdata[7:0] = pch_mctp_bytecount_reg[0];
        end
        else if (m0_readdatavalid && bmc_mctp_bc_rdvalid_reg) begin
            m0_readdata[7:0] = bmc_mctp_bytecount_reg[0];
        end
        else if (m0_readdatavalid && pcie_mctp_bc_rdvalid_reg) begin
            m0_readdata[7:0] = pcie_mctp_bytecount_reg[0];
        end
        else if (m0_readdatavalid && pch_mctp_fifoavail_rdvalid_reg) begin
            m0_readdata[7:0] = {pch_mctp_rxpkt_error, pch_mctp_fifoavail};
        end
        else if (m0_readdatavalid && bmc_mctp_fifoavail_rdvalid_reg) begin
            m0_readdata[7:0] = {bmc_mctp_rxpkt_error, bmc_mctp_fifoavail};
        end
        else if (m0_readdatavalid && pcie_mctp_fifoavail_rdvalid_reg) begin
            m0_readdata[7:0] = {pcie_mctp_rxpkt_error, pcie_mctp_fifoavail};
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
        else if (pch_address == platform_defs_pkg::WRITE_FIFO_ADDR && pch_write && ~pch_waitrequest) begin
            fifo_enqueue = 1'b1;
            fifo_data_in = pch_writedata;
        end
        else if (pch_address == platform_defs_pkg::READ_FIFO_ADDR && pch_read && ~pch_waitrequest) begin
            fifo_dequeue = 1'b1;
        end
        else if (bmc_address == platform_defs_pkg::WRITE_FIFO_ADDR && bmc_write && ~bmc_waitrequest) begin
            fifo_enqueue = 1'b1;
            fifo_data_in = bmc_writedata;
        end
        else if (bmc_address == platform_defs_pkg::READ_FIFO_ADDR && bmc_read && ~bmc_waitrequest) begin
            fifo_dequeue = 1'b1;
        end
    end
    //------Logic to store BIOS_CHECKPOINT_ADDR in FIFO------------
    always_comb begin
        bios_fifo_data_in = pch_writedata;
        //wr to fifo
        if (pch_address == platform_defs_pkg::BIOS_CHECKPOINT_ADDR && pch_write && ~pch_waitrequest) begin
            bios_fifo_wr = 1'b1;
        end
        else begin
            bios_fifo_wr = 1'b0;
        end
        //rd from fifo
        if (m0_address == platform_defs_pkg::BIOS_READ_FIFO_ADDR && m0_read) begin
            bios_fifo_rd = 1'b1;
        end
        else begin
            bios_fifo_rd = 1'b0;
        end
    end
    // Fifo 
    // anything less than 8x1023 will result in using 1 M9k
    fifo #(
            .DATA_WIDTH(8),
            .DATA_DEPTH(platform_defs_pkg::SMBUS_BIOS_FIFO_DEPTH)
          ) 
    u_bios_fifo 
        (
            .clk(clk),
            .resetn(resetn),
            .data_in(bios_fifo_data_in),
            .data_out(bios_fifo_data_out),
            .empty(bios_fifo_empty),
            .full(bios_fifo_full),
            .clear(bios_fifo_clear),
            .enqueue(bios_fifo_wr),
            .dequeue(bios_fifo_rd)
        );
    assign bios_status_reg = {2'b00, bios_fifo_full, bios_fifo_empty, 4'h0};
    always_comb begin
        if (m0_read && (m0_address == platform_defs_pkg::BIOS_READ_FIFO_ADDR)) begin
            m0_bios_fifo_rdvalid_nxt = 1'b1;
        end
        else begin
            m0_bios_fifo_rdvalid_nxt = 1'b0;
        end
    end
    always_comb begin
        if ((m0_address == platform_defs_pkg::BIOS_CONTROL_STATUS_REG) && m0_write && (m0_writedata[0] == 1'b1)) begin
            bios_fifo_clear = 1'b1;
        end
        else begin
            bios_fifo_clear = 1'b0;
        end
    end
    always_comb begin
        if ((m0_address == platform_defs_pkg::BIOS_CONTROL_STATUS_REG) && m0_read ) begin
            m0_bios_fifo_status_rdvalid_nxt = 1'b1;
        end
        else begin
            m0_bios_fifo_status_rdvalid_nxt = 1'b0;
        end
    end

    //--------------------------------------------------------------

    // Clears the fifo if bits [2] or [1] are set in address 0xA
    // This is managed in hardware to simplifiy the software requirments of the NIOS
    always_comb begin
        fifo_clear = 1'b0;
        if (m0_address == platform_defs_pkg::COMMAND_TRIGGER_ADDR && m0_write && (|m0_writedata[2:1])
            || bmc_address == platform_defs_pkg::COMMAND_TRIGGER_ADDR && bmc_write && ~bmc_waitrequest && (|bmc_writedata[2:1])
            || pch_address == platform_defs_pkg::COMMAND_TRIGGER_ADDR && pch_write && ~pch_waitrequest && (|pch_writedata[2:1])) begin
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
            pch_error_condition  <= 1'b0;
            bmc_error_condition  <= 1'b0;
            pcie_error_condition <= 1'b0;
        end
        else begin
        	if (pch_oversize_pkt_err || pch_invalid_last_bc || pch_invalid_middle_bc) begin
                pch_error_condition  <= 1'b1;
        	end
        	else if (pch_smbus_stop || pch_smbus_start) begin
                pch_error_condition  <= 1'b0;
        	end
        	
        	if (bmc_oversize_pkt_err || bmc_invalid_last_bc || bmc_invalid_middle_bc) begin
                bmc_error_condition  <= 1'b1;
        	end
        	else if (bmc_smbus_stop || bmc_smbus_start) begin
                bmc_error_condition  <= 1'b0;
        	end
        	
        	if (pcie_oversize_pkt_err || pcie_invalid_last_bc || pcie_invalid_middle_bc) begin
                pcie_error_condition  <= 1'b1;
        	end
        	else if (pcie_smbus_stop || pcie_smbus_start) begin
                pcie_error_condition <= 1'b0;
        	end
        end
    end
    
    // NACK when 1) (&mctp_fifoavail == 1'b1) because FIFO is allowed to store up to 4 entries pkts only
    // 2) if message length > 1024 byte
    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            nack_pch_mctp_pkt  <= 1'b0;
            nack_bmc_mctp_pkt  <= 1'b0;
            nack_pcie_mctp_pkt <= 1'b0;
        end
        else begin 
            if ((pch_address == platform_defs_pkg::MCTP_WRITE_FIFO_ADDR) && pch_write && ~pch_waitrequest) begin
                if (pch_mctp_fifo_full || pch_trigger_nack) begin 
                    nack_pch_mctp_pkt <= 1'b1;
                end
            end
            else if (pch_smbus_stop || (pch_smbus_start && pch_mctp_wait4stop)) begin
                nack_pch_mctp_pkt <= 1'b0;
            end
            
            if ((bmc_address == platform_defs_pkg::MCTP_WRITE_FIFO_ADDR) && bmc_write && ~bmc_waitrequest) begin
                if (bmc_mctp_fifo_full || bmc_trigger_nack) begin 
                    nack_bmc_mctp_pkt <= 1'b1;
                end
            end
            else if (bmc_smbus_stop || (bmc_smbus_start && bmc_mctp_wait4stop)) begin
                nack_bmc_mctp_pkt <= 1'b0;
            end
            
            if ((pcie_address == platform_defs_pkg::MCTP_WRITE_FIFO_ADDR) && pcie_write && ~pcie_waitrequest) begin
                if (pcie_mctp_fifo_full || pcie_trigger_nack) begin 
                    nack_pcie_mctp_pkt <= 1'b1;
                end
            end
            else if (pcie_smbus_stop || (pcie_smbus_start && pcie_mctp_wait4stop)) begin
                nack_pcie_mctp_pkt <= 1'b0;
            end
        end
    end
	
    always_comb begin
        if (m0_address == platform_defs_pkg::PCH_MCTP_READ_FIFO_ADDR && m0_read) begin  //NIOS is not able to write to this MCTP_FIFO
            pch_mctp_fifo_dequeue = 1'b1;
            pch_mctp_fifo_enqueue = 1'b0;
            pch_mctp_fifo_data_in = 8'd0;
        end
        else if (pch_write_to_mctpfifo && pch_mctp_databyte_cnt > 9'd1 && ~pch_invalid_long_msg) begin   // dont push last (PEC) && first (bytecount) data byte into the FIFO
            pch_mctp_fifo_dequeue = 1'b0;
        	pch_mctp_fifo_enqueue = 1'b1;
            pch_mctp_fifo_data_in = pch_writedata;
        end
        else begin
            pch_mctp_fifo_dequeue = 1'b0;
            pch_mctp_fifo_enqueue = 1'b0;
            pch_mctp_fifo_data_in = 8'd0;
        end
        
        if (m0_address == platform_defs_pkg::BMC_MCTP_READ_FIFO_ADDR && m0_read) begin  //NIOS is not able to write to this MCTP_FIFO
            bmc_mctp_fifo_dequeue = 1'b1;
            bmc_mctp_fifo_enqueue = 1'b0;
            bmc_mctp_fifo_data_in = 8'd0;
        end
        else if (bmc_write_to_mctpfifo && bmc_mctp_databyte_cnt > 9'd1 && ~bmc_invalid_long_msg) begin   // dont push last (PEC) && first (bytecount) data byte into the FIFO
            bmc_mctp_fifo_dequeue = 1'b0;
        	bmc_mctp_fifo_enqueue = 1'b1;
            bmc_mctp_fifo_data_in = bmc_writedata;
        end
        else begin
            bmc_mctp_fifo_dequeue = 1'b0;
            bmc_mctp_fifo_enqueue = 1'b0;
            bmc_mctp_fifo_data_in = 8'd0;
        end
        
        if (m0_address == platform_defs_pkg::PCIE_MCTP_READ_FIFO_ADDR && m0_read) begin  //NIOS is not able to write to this MCTP_FIFO
            pcie_mctp_fifo_dequeue = 1'b1;
            pcie_mctp_fifo_enqueue = 1'b0;
            pcie_mctp_fifo_data_in = 8'd0;
        end
        else if (pcie_write_to_mctpfifo && pcie_mctp_databyte_cnt > 9'd1 && ~pcie_invalid_long_msg) begin  // dont push last (PEC) && first (bytecount) data byte into the FIFO
            pcie_mctp_fifo_dequeue = 1'b0;
        	pcie_mctp_fifo_enqueue = 1'b1;
            pcie_mctp_fifo_data_in = pcie_writedata;
        end
        else begin
            pcie_mctp_fifo_dequeue = 1'b0;
            pcie_mctp_fifo_enqueue = 1'b0;
            pcie_mctp_fifo_data_in = 8'd0;
        end
    end
    
    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            pch_mctp_msg_cnt       <= 6'h0;
            bmc_mctp_msg_cnt       <= 6'h0;
            pcie_mctp_msg_cnt      <= 6'h0;
            pch_mctp_som_bc        <= 8'd0;
            bmc_mctp_som_bc        <= 8'd0;
            pcie_mctp_som_bc       <= 8'd0;
            pch_invalid_middle_bc  <= 1'd0;
            bmc_invalid_middle_bc  <= 1'd0;
            pcie_invalid_middle_bc <= 1'd0;
            pch_invalid_last_bc    <= 1'd0;
            bmc_invalid_last_bc    <= 1'd0;
            pcie_invalid_last_bc   <= 1'd0;
        end
        else begin  
            if (pch_check_mctp_msgflag) begin  // SOM - pch_writedata[7], EOM - pch_writedata[6]
            	if (pch_mctp_som) begin
                    pch_mctp_som_bc <= pch_mctp_bytecount;
                end
                
                // error condition 6: Host send first packet but never send last packet after sending lots of middle packets, > 1K bytes
                // assume that minimum transmission unit (bytecount) is 64 byte and maximum is 1024 byte
                if (~pch_mctp_eom && pch_mctp_msg_cnt > 6'd0 && (pch_mctp_msg_cnt <= MAXIMUM_NUM_MCTP_MSG)) begin
                    pch_mctp_msg_cnt <= pch_mctp_msg_cnt + 6'd1;
                end
                else if (pch_mctp_eom) begin  //reset message counter
                    pch_mctp_msg_cnt <= 6'd0;
                end
                else if (pch_mctp_som) begin  //restart message counter
                    pch_mctp_msg_cnt <= 6'd1;
                end
                
                // error condition 4: Host send middle packet size not equal to first packet size
                if (~pch_mctp_som && ~pch_mctp_eom && pch_mctp_som_bc != pch_mctp_bytecount) begin
                    pch_invalid_middle_bc <= 1'd1;    
                end
                
                // error condition 5: Host send last packet size greater than first packet size
                if (~pch_mctp_som && pch_mctp_eom && pch_mctp_som_bc < pch_mctp_bytecount) begin
                    pch_invalid_last_bc <= 1'd1;            	
                end
            end
            else if (pch_smbus_stop || (pch_smbus_start && pch_mctp_wait4stop)) begin
                pch_invalid_middle_bc <= 1'd0; 
                pch_invalid_last_bc <= 1'd0; 
            end
                
            if (bmc_check_mctp_msgflag) begin
            	if (bmc_mctp_som) begin
                    bmc_mctp_som_bc <= bmc_mctp_bytecount;
                end
                
                if (~bmc_mctp_eom && bmc_mctp_msg_cnt > 6'd0 && (bmc_mctp_msg_cnt <= MAXIMUM_NUM_MCTP_MSG)) begin
                    bmc_mctp_msg_cnt <= bmc_mctp_msg_cnt + 6'd1;
                end
                else if (bmc_mctp_eom) begin  //reset message counter
                    bmc_mctp_msg_cnt <= 6'd0;
                end
                else if (bmc_mctp_som) begin  //restart message counter
                    bmc_mctp_msg_cnt <= 6'd1;
                end
                
                if (~bmc_mctp_som && ~bmc_mctp_eom && bmc_mctp_som_bc != bmc_mctp_bytecount) begin
                    bmc_invalid_middle_bc <= 1'd1;            	
                end
                
                if (~bmc_mctp_som && bmc_mctp_eom && bmc_mctp_som_bc < bmc_mctp_bytecount) begin
                    bmc_invalid_last_bc <= 1'd1;            	
                end
            end
            else if (bmc_smbus_stop || (bmc_smbus_start && bmc_mctp_wait4stop)) begin
                bmc_invalid_middle_bc <= 1'd0; 
                bmc_invalid_last_bc <= 1'd0;
            end
            
            if (pcie_check_mctp_msgflag) begin
            	if (pcie_mctp_som) begin
                    pcie_mctp_som_bc <= pcie_mctp_bytecount;
                end
                
                if (~pcie_mctp_eom && pcie_mctp_msg_cnt > 6'd0 && (pcie_mctp_msg_cnt <= MAXIMUM_NUM_MCTP_MSG)) begin
                    pcie_mctp_msg_cnt <= pcie_mctp_msg_cnt + 6'd1;
                end
                else if (pcie_mctp_eom) begin  //reset message counter
                    pcie_mctp_msg_cnt <= 6'd0;
                end
                else if (pcie_mctp_som) begin  //restart message counter
                    pcie_mctp_msg_cnt <= 6'd1;
                end
                
                if (~pcie_mctp_som && ~pcie_mctp_eom && pcie_mctp_som_bc != pcie_mctp_bytecount) begin
                    pcie_invalid_middle_bc <= 1'd1;            	
                end
                
                if (~pcie_mctp_som && pcie_mctp_eom && pcie_mctp_som_bc < pcie_mctp_bytecount) begin
                    pcie_invalid_last_bc <= 1'd1;            	
                end
            end
            else if (pcie_smbus_stop || (pcie_smbus_start && pcie_mctp_wait4stop)) begin
                pcie_invalid_middle_bc <= 1'd0; 
                pcie_invalid_last_bc <= 1'd0; 
            end
        end
    end

    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            pch_mctp_wait4stop  <= 1'd0;
            bmc_mctp_wait4stop  <= 1'd0;
            pcie_mctp_wait4stop <= 1'd0;
        end
        else begin
            if (pch_smbus_stop || pch_smbus_start) begin
                pch_mctp_wait4stop <= 1'd0;
            end
            else if (pch_write_to_mctpfifo && (pch_mctp_databyte_cnt == 9'd0)) begin
                pch_mctp_wait4stop <= 1'd1;
            end
            
            if (bmc_smbus_stop || bmc_smbus_start) begin
                bmc_mctp_wait4stop <= 1'd0;
            end
            else if (bmc_write_to_mctpfifo && (bmc_mctp_databyte_cnt == 9'd0)) begin
                bmc_mctp_wait4stop <= 1'd1;
            end
            
            if (pcie_smbus_stop || pcie_smbus_start) begin
                pcie_mctp_wait4stop <= 1'd0;
            end
            else if (pcie_write_to_mctpfifo && (pcie_mctp_databyte_cnt == 9'd0)) begin
                pcie_mctp_wait4stop <= 1'd1;
            end
        end
    end

    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            pch_mctp_bytecount <= 8'd0;
            bmc_mctp_bytecount <= 8'd0;
            pcie_mctp_bytecount <= 8'd0;
        end
        else begin
            if (pch_write_to_mctpfifo && (pch_mctp_databyte_cnt == 9'd0) && ~pch_mctp_wait4stop) begin
                pch_mctp_bytecount <= pch_writedata;
            end
            
            if (bmc_write_to_mctpfifo && (bmc_mctp_databyte_cnt == 9'd0) && ~bmc_mctp_wait4stop) begin
                bmc_mctp_bytecount <= bmc_writedata;
            end
            
            if (pcie_write_to_mctpfifo && (pcie_mctp_databyte_cnt == 9'd0) && ~pcie_mctp_wait4stop) begin
                pcie_mctp_bytecount <= pcie_writedata;
            end
        end
    end
    
    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            pch_mctp_msgflag_cnt <= 3'd0;
            bmc_mctp_msgflag_cnt <= 3'd0;
            pcie_mctp_msgflag_cnt <= 3'd0;
        end
        else begin
        	if (pch_write_to_mctpfifo && pch_mctp_msgflag_cnt > 3'd0 && pch_mctp_msgflag_cnt < MCTP_MSGFLAG_BYTE) begin
                pch_mctp_msgflag_cnt <= pch_mctp_msgflag_cnt + 3'd1;
            end
            else if (pch_write_to_mctpfifo && (|pch_mctp_databyte_cnt == 1'd0) && (|pch_mctp_msgflag_cnt == 1'd0) && ~pch_oversize_pkt_err && ~pch_error_condition) begin
                pch_mctp_msgflag_cnt <= 3'd1;
            end
            else if (pch_check_mctp_msgflag || pch_undersize_pkt_err) begin
                pch_mctp_msgflag_cnt <= 3'd0;                
            end
            
        	if (bmc_write_to_mctpfifo && bmc_mctp_msgflag_cnt > 3'd0 && bmc_mctp_msgflag_cnt < MCTP_MSGFLAG_BYTE) begin
                bmc_mctp_msgflag_cnt <= bmc_mctp_msgflag_cnt + 3'd1;
            end
            else if (bmc_write_to_mctpfifo && (|bmc_mctp_databyte_cnt == 1'd0) && (|bmc_mctp_msgflag_cnt == 1'd0) && ~bmc_oversize_pkt_err && ~bmc_error_condition) begin
                bmc_mctp_msgflag_cnt <= 3'd1;
            end
            else if (bmc_check_mctp_msgflag || bmc_undersize_pkt_err) begin
                bmc_mctp_msgflag_cnt <= 3'd0;                
            end
            
        	if (pcie_write_to_mctpfifo && pcie_mctp_msgflag_cnt > 3'd0 && pcie_mctp_msgflag_cnt < MCTP_MSGFLAG_BYTE) begin
                pcie_mctp_msgflag_cnt <= pcie_mctp_msgflag_cnt + 3'd1;
            end
            else if (pcie_write_to_mctpfifo && (|pcie_mctp_databyte_cnt == 1'd0) && (|pcie_mctp_msgflag_cnt == 1'd0) && ~pcie_oversize_pkt_err && ~pcie_error_condition) begin
                pcie_mctp_msgflag_cnt <= 3'd1;
            end
            else if (pcie_check_mctp_msgflag || pcie_undersize_pkt_err) begin
                pcie_mctp_msgflag_cnt <= 3'd0;                
            end
        end
    end
    
    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            pch_mctp_databyte_cnt <= 9'd0;
            bmc_mctp_databyte_cnt <= 9'd0;
            pcie_mctp_databyte_cnt <= 9'd0;
        end
        else if (pch_write_to_mctpfifo) begin
            if (pch_mctp_databyte_cnt == 9'd0) begin
            	if (~pch_mctp_wait4stop) begin
            		pch_mctp_databyte_cnt <= pch_writedata + 8'd1; //the first databyte is the MCTP pkt bytecount, +1 for PEC
                end
            end
            else begin
                pch_mctp_databyte_cnt <= pch_mctp_databyte_cnt - 9'd1;
            end
        end
        else if (bmc_write_to_mctpfifo) begin
            if (bmc_mctp_databyte_cnt == 9'd0) begin
            	if (~bmc_mctp_wait4stop) begin
                    bmc_mctp_databyte_cnt <= bmc_writedata + 8'd1; //the first databyte is the MCTP pkt bytecount, +1 for PEC
                end
            end
            else begin
                bmc_mctp_databyte_cnt <= bmc_mctp_databyte_cnt - 9'd1;
            end
        end
        else if (pcie_write_to_mctpfifo) begin
            if (pcie_mctp_databyte_cnt == 9'd0) begin
                if (~pcie_mctp_wait4stop) begin
                    pcie_mctp_databyte_cnt <= pcie_writedata + 8'd1; //the first databyte is the MCTP pkt bytecount, +1 for PEC
                end
            end
            else begin
                pcie_mctp_databyte_cnt <= pcie_mctp_databyte_cnt - 9'd1;
            end
        end
        else if (pch_undersize_pkt_err) begin
            pch_mctp_databyte_cnt <= 9'd0;
        end
        else if (bmc_undersize_pkt_err) begin
            bmc_mctp_databyte_cnt <= 9'd0;
        end
        else if (pcie_undersize_pkt_err) begin
            pcie_mctp_databyte_cnt <= 9'd0;
        end
    end

	// Set fifoavail register when a complete pkt is in the FIFO (NIOS Read to clear)
	// Set rxpkt_error register when error/invalid cases happen
	// Store bytecount value for each packet 
    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
			pch_mctp_fifoavail      <= 4'b0000;
			bmc_mctp_fifoavail      <= 4'b0000;
			pcie_mctp_fifoavail     <= 4'b0000;
            pch_mctp_rxpkt_error    <= 4'b0000;
            bmc_mctp_rxpkt_error    <= 4'b0000;
            pcie_mctp_rxpkt_error   <= 4'b0000;
            pch_mctp_bytecount_reg  <= '{4{'0}};
            bmc_mctp_bytecount_reg  <= '{4{'0}};
            pcie_mctp_bytecount_reg <= '{4{'0}};
        end
        else begin
            if ((pch_smbus_stop && ~nack_pch_mctp_pkt && pch_mctp_wait4stop) || (pch_smbus_start && pch_mctp_wait4stop)) begin 
            	if (pch_mctp_fifoavail & 4'b0100) begin
            	    pch_mctp_rxpkt_error[3]   <= pch_error_condition_combi;
            	    
            	    if (pch_undersize_pkt_err) begin  // case: pkt size < bytecount, modify the orginal bytecount value
            	    	pch_mctp_bytecount_reg[3] <= pch_mctp_bytecount - (pch_mctp_databyte_cnt[7:0] - 8'd1);
            	    end
            	    else begin
            	        pch_mctp_bytecount_reg[3] <= pch_mctp_bytecount;
            	    end
            	end
            	else if (pch_mctp_fifoavail & 4'b0010) begin
            	    pch_mctp_rxpkt_error[2]   <= pch_error_condition_combi;

            	    if (pch_undersize_pkt_err) begin  // case: pkt size < bytecount, modify the orginal bytecount value
            	    	pch_mctp_bytecount_reg[2] <= pch_mctp_bytecount - (pch_mctp_databyte_cnt[7:0] - 8'd1);
            	    end
            	    else begin
            	        pch_mctp_bytecount_reg[2] <= pch_mctp_bytecount;
            	    end
            	end
            	else if (pch_mctp_fifoavail & 4'b0001) begin
            	    pch_mctp_rxpkt_error[1]   <= pch_error_condition_combi;

            	    if (pch_undersize_pkt_err) begin  // case: pkt size < bytecount, modify the orginal bytecount value
            	    	pch_mctp_bytecount_reg[1] <= pch_mctp_bytecount - (pch_mctp_databyte_cnt[7:0] - 8'd1);
            	    end
            	    else begin
            	        pch_mctp_bytecount_reg[1] <= pch_mctp_bytecount;
            	    end
            	end
            	else begin // FIFO was previously empty
            	    pch_mctp_rxpkt_error[0]   <= pch_error_condition_combi;
            	    
            	    if (pch_undersize_pkt_err) begin  // case: pkt size < bytecount, modify the orginal bytecount value
            	    	pch_mctp_bytecount_reg[0] <= pch_mctp_bytecount - (pch_mctp_databyte_cnt[7:0] - 8'd1);
            	    end
            	    else begin
            	        pch_mctp_bytecount_reg[0] <= pch_mctp_bytecount;
            	    end
            	end
            	
                pch_mctp_fifoavail  <= {pch_mctp_fifoavail[2:0], 1'b1};
            end
            else if (m0_write && m0_address == platform_defs_pkg::PCH_MCTP_FIFO_AVAIL_ADDR && m0_writedata == 8'h1) begin  // write 1 to clear avail register
                pch_mctp_fifoavail     <= {1'b0, pch_mctp_fifoavail[3:1]};
                pch_mctp_rxpkt_error   <= {1'b0, pch_mctp_rxpkt_error[3:1]};
            end
            else if (m0_readdatavalid && pch_mctp_bc_rdvalid_reg) begin
                pch_mctp_bytecount_reg[0:2] <= pch_mctp_bytecount_reg[1:3];
                pch_mctp_bytecount_reg[3] <= 8'd0;
            end
            
            if ((bmc_smbus_stop && ~nack_bmc_mctp_pkt && bmc_mctp_wait4stop) || (bmc_smbus_start && bmc_mctp_wait4stop)) begin
            	if (bmc_mctp_fifoavail & 4'b0100) begin
            	    bmc_mctp_rxpkt_error[3]   <= bmc_error_condition_combi;
            	    
            	    if (bmc_undersize_pkt_err) begin  // case: pkt size < bytecount, modify the orginal bytecount value
            	        bmc_mctp_bytecount_reg[3] <= bmc_mctp_bytecount - (bmc_mctp_databyte_cnt[7:0] - 8'd1);
            	    end
            	    else begin
            	    	bmc_mctp_bytecount_reg[3] <= bmc_mctp_bytecount;
            	    end
            	end
            	else if (bmc_mctp_fifoavail & 4'b0010) begin
            	    bmc_mctp_rxpkt_error[2]   <= bmc_error_condition_combi;
   
            	    if (bmc_undersize_pkt_err) begin  // case: pkt size < bytecount, modify the orginal bytecount value
            	        bmc_mctp_bytecount_reg[2] <= bmc_mctp_bytecount - (bmc_mctp_databyte_cnt[7:0] - 8'd1);
            	    end
            	    else begin
            	    	bmc_mctp_bytecount_reg[2] <= bmc_mctp_bytecount;
            	    end
            	end
            	else if (bmc_mctp_fifoavail & 4'b0001) begin
            	    bmc_mctp_rxpkt_error[1]   <= bmc_error_condition_combi;
            	    
            	    if (bmc_undersize_pkt_err) begin  // case: pkt size < bytecount, modify the orginal bytecount value
            	        bmc_mctp_bytecount_reg[1] <= bmc_mctp_bytecount - (bmc_mctp_databyte_cnt[7:0] - 8'd1);
            	    end
            	    else begin
            	    	bmc_mctp_bytecount_reg[1] <= bmc_mctp_bytecount;
            	    end
            	end
            	else begin // FIFO was previously empty
            	    bmc_mctp_rxpkt_error[0]   <= bmc_error_condition_combi;

            	    if (bmc_undersize_pkt_err) begin  // case: pkt size < bytecount, modify the orginal bytecount value
            	        bmc_mctp_bytecount_reg[0] <= bmc_mctp_bytecount - (bmc_mctp_databyte_cnt[7:0] - 8'd1);
            	    end
            	    else begin
            	    	bmc_mctp_bytecount_reg[0] <= bmc_mctp_bytecount;
            	    end
            	end
            	
                bmc_mctp_fifoavail  <= {bmc_mctp_fifoavail[2:0], 1'b1};
            end
            else if (m0_write && m0_address == platform_defs_pkg::BMC_MCTP_FIFO_AVAIL_ADDR && m0_writedata == 8'h1) begin  // write 1 to clear avail register
                bmc_mctp_fifoavail    <= {1'b0, bmc_mctp_fifoavail[3:1]};
                bmc_mctp_rxpkt_error  <= {1'b0, bmc_mctp_rxpkt_error[3:1]};
            end
            else if (m0_readdatavalid && bmc_mctp_bc_rdvalid_reg) begin
                bmc_mctp_bytecount_reg[0:2] <= bmc_mctp_bytecount_reg[1:3];
                bmc_mctp_bytecount_reg[3] <= 8'd0;
            end
            
            if ((pcie_smbus_stop && ~nack_pcie_mctp_pkt && pcie_mctp_wait4stop) || (pcie_smbus_start && pcie_mctp_wait4stop)) begin 
            	if (pcie_mctp_fifoavail & 4'b0100) begin
            	    pcie_mctp_rxpkt_error[3]   <= pcie_error_condition_combi;
            	    
            	    if (pcie_undersize_pkt_err) begin  // case: pkt size < bytecount, modify the orginal bytecount value
            	        pcie_mctp_bytecount_reg[3] <= pcie_mctp_bytecount - (pcie_mctp_databyte_cnt[7:0] - 8'd1);
            	    end
            	    else begin
            	        pcie_mctp_bytecount_reg[3] <= pcie_mctp_bytecount;
            	    end
            	end
            	else if (pcie_mctp_fifoavail & 4'b0010) begin
            	    pcie_mctp_rxpkt_error[2]   <= pcie_error_condition_combi;
            	    
            	    if (pcie_undersize_pkt_err) begin  // case: pkt size < bytecount, modify the orginal bytecount value
            	        pcie_mctp_bytecount_reg[2] <= pcie_mctp_bytecount - (pcie_mctp_databyte_cnt[7:0] - 8'd1);
            	    end
            	    else begin
            	        pcie_mctp_bytecount_reg[2] <= pcie_mctp_bytecount;
            	    end
            	end
            	else if (pcie_mctp_fifoavail & 4'b0001) begin
            	    pcie_mctp_rxpkt_error[1]   <= pcie_error_condition_combi;
            	    
            	    if (pcie_undersize_pkt_err) begin  // case: pkt size < bytecount, modify the orginal bytecount value
            	        pcie_mctp_bytecount_reg[1] <= pcie_mctp_bytecount - (pcie_mctp_databyte_cnt[7:0] - 8'd1);
            	    end
            	    else begin
            	        pcie_mctp_bytecount_reg[1] <= pcie_mctp_bytecount;
            	    end
            	end
            	else begin // FIFO was previously empty
            	    pcie_mctp_rxpkt_error[0]   <= pcie_error_condition_combi;
            	    
            	    if (pcie_undersize_pkt_err) begin  // case: pkt size < bytecount, modify the orginal bytecount value
            	        pcie_mctp_bytecount_reg[0] <= pcie_mctp_bytecount - (pcie_mctp_databyte_cnt[7:0] - 8'd1);
            	    end
            	    else begin
            	        pcie_mctp_bytecount_reg[0] <= pcie_mctp_bytecount;
            	    end
            	end
            	
                pcie_mctp_fifoavail  <= {pcie_mctp_fifoavail[2:0], 1'b1};
            end
            else if (m0_write && m0_address == platform_defs_pkg::PCIE_MCTP_FIFO_AVAIL_ADDR && m0_writedata == 8'h1) begin  // write 1 to clear avail register
                pcie_mctp_fifoavail    <= {1'b0, pcie_mctp_fifoavail[3:1]};
                pcie_mctp_rxpkt_error  <= {1'b0, pcie_mctp_rxpkt_error[3:1]};
            end
            else if (m0_readdatavalid && pcie_mctp_bc_rdvalid_reg) begin
                pcie_mctp_bytecount_reg[0:2] <= pcie_mctp_bytecount_reg[1:3];
                pcie_mctp_bytecount_reg[3] <= 8'd0;
            end
        end
    end
    
    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            bmc_update_intent <= 8'h0;
            pch_update_intent <= 8'h0;
        end
        else begin 
        	if (bmc_address == platform_defs_pkg::BMC_UPDATE_INTENT_ADDR && bmc_write && ~bmc_waitrequest) begin
                bmc_update_intent <= bmc_writedata;
            end
            else if (m0_write && m0_address == platform_defs_pkg::BMC_UPDATE_INTENT_ADDR) begin
                bmc_update_intent <= m0_writedata;
            end
            
        	if (pch_address == platform_defs_pkg::PCH_UPDATE_INTENT_ADDR && pch_write && ~pch_waitrequest) begin
                pch_update_intent <= pch_writedata;
            end
            else if (m0_write && m0_address == platform_defs_pkg::PCH_UPDATE_INTENT_ADDR) begin
                pch_update_intent <= m0_writedata;
            end
        end
    end
    
    always_ff @ (posedge clk or negedge resetn) begin
        if (~resetn) begin
            bmc_update_intent_part2 <= 8'h0;
            pch_update_intent_part2 <= 8'h0;
        end
        else begin 
        	if (bmc_address == platform_defs_pkg::BMC_UPDATE_INTENT_PART2_ADDR && bmc_write && ~bmc_waitrequest) begin
                bmc_update_intent_part2 <= bmc_writedata;
            end
            else if (m0_write && m0_address == platform_defs_pkg::BMC_UPDATE_INTENT_PART2_ADDR) begin
                bmc_update_intent_part2 <= m0_writedata;
            end
            
        	if (pch_address == platform_defs_pkg::PCH_UPDATE_INTENT_PART2_ADDR && pch_write && ~pch_waitrequest) begin
                pch_update_intent_part2 <= pch_writedata;
            end
            else if (m0_write && m0_address == platform_defs_pkg::PCH_UPDATE_INTENT_PART2_ADDR) begin
                pch_update_intent_part2 <= m0_writedata;
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
        
    // Fifo to store MCTP pkt 
    // anything less than 8x1023 will result in using 1 M9k
    fifo #(
            .DATA_WIDTH(8),
            .DATA_DEPTH(platform_defs_pkg::SMBUS_MAILBOX_MCTP_FIFO_DEPTH)
            ) 
    u_pch_mctp_fifo 
        (
            .clk(clk),
            .resetn(resetn),
            .data_in(pch_mctp_fifo_data_in),
            .data_out(pch_mctp_fifo_data_out),
            .clear(1'b0),
            .enqueue(pch_mctp_fifo_enqueue),
            .dequeue(pch_mctp_fifo_dequeue)
        );
        
    // Fifo to store MCTP pkt 
    // anything less than 8x1023 will result in using 1 M9k
    fifo #(
            .DATA_WIDTH(8),
            .DATA_DEPTH(platform_defs_pkg::SMBUS_MAILBOX_MCTP_FIFO_DEPTH)
            ) 
    u_bmc_mctp_fifo 
        (
            .clk(clk),
            .resetn(resetn),
            .data_in(bmc_mctp_fifo_data_in),
            .data_out(bmc_mctp_fifo_data_out),
            .clear(1'b0),
            .enqueue(bmc_mctp_fifo_enqueue),
            .dequeue(bmc_mctp_fifo_dequeue)
        );
        
    // Fifo to store MCTP pkt 
    // anything less than 8x1023 will result in using 1 M9k
    fifo #(
            .DATA_WIDTH(8),
            .DATA_DEPTH(platform_defs_pkg::SMBUS_MAILBOX_MCTP_FIFO_DEPTH)
            ) 
    u_pcie_mctp_fifo 
        (
            .clk(clk),
            .resetn(resetn),
            .data_in(pcie_mctp_fifo_data_in),
            .data_out(pcie_mctp_fifo_data_out),
            .clear(1'b0),
            .enqueue(pcie_mctp_fifo_enqueue),
            .dequeue(pcie_mctp_fifo_dequeue)
        );

endmodule
