// (C) 2001-2018 Intel Corporation. All rights reserved.
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


`timescale 1ps / 1ps
`default_nettype none

module altera_avalon_i2c_csr #(
    parameter USE_AV_ST = 0,
    parameter FIFO_DEPTH = 8,
    parameter FIFO_DEPTH_LOG2 = 3
) (
    input                       clk,
    input                       rst_n,
    input [3:0]                 addr,
    input                       read,
    input                       write,
    input [31:0]                writedata,
    input                       arb_lost,
    input                       txfifo_empty,
    input                       txfifo_full,
    input                       mstfsm_idle_state,
    input                       abrt_txdata_noack,
    input                       abrt_7b_addr_noack,
    input                       abrt_txcrc8_noack,
    input                       rxfifo_full,
    input                       rxfifo_empty,
    input [7:0]                 rx_fifo_data_out,
    input                       push_rxfifo,
    input [FIFO_DEPTH_LOG2:0]   txfifo_navail,
    input [FIFO_DEPTH_LOG2:0]   rxfifo_navail,
    
    // Remove AVALON_ST feature and interrupt interface signal since it is not used
    output [31:0]       readdata,
    output              ctrl_en,
    output reg [2:0]    slave_portid,
    output reg          pec_enable,
    output              flush_txfifo,
    output              write_txfifo,
    output              read_rxfifo,
    output [9:0]        txfifo_writedata,
    output [15:0]       scl_lcnt,
    output [15:0]       scl_hcnt,
    output reg [15:0]   sda_hold,
    output              speed_mode


);


reg [5:0]   ctrl;
reg         arblost_det;
reg         nack_det;
reg [15:0]  scl_low;
reg [15:0]  scl_high;
reg [31:0]  readdata_dly2;

reg         rx_data_rden_dly;
reg         rx_data_rden_dly2;

reg [3:0]   status;
reg [3:0]   control;
// 1. Remove some register access from AvMM Bus as runtime setting modification is not required
// 2. Disabled interrupt since this feature is not used
// Modified registers are:
//  - Control register, set to 6'b11: Core is enabled, BUS_SPEED is fast mode
//  - Interrupt status enable register, removed
//  - Interrupt status register, removed roll over register
//  - SCL Low Counter register, set to 16'd130
//  - SCL High Counter register, set to 16'd130
//  - SDA Hold Counter register, set to 16'h1
//  - Status register read access is removed

reg         tfr_cmd_fifo_lvl_rden_dly;
reg         rx_data_fifo_lvl_rden_dly;
reg         status_rden_dly;
reg         control_rden_dly;
reg         slave_portid_rden_dly;

wire    tfr_cmd_addr;
wire    rx_data_addr;
wire    control_addr;
wire    status_addr;
wire    slave_portid_addr;
wire    tfr_cmd_fifo_lvl_addr;
wire    rx_data_fifo_lvl_addr;

wire    tfr_cmd_wren;
wire    status_wren;
wire    control_wren;
wire    slave_portid_wren;

wire    rx_data_rden;
wire    control_rden;
wire    status_rden;
wire    slave_portid_rden;
wire    tfr_cmd_fifo_lvl_rden;
wire    rx_data_fifo_lvl_rden;


wire [31:0] rx_data_internal;
wire [31:0] tfr_cmd_fifo_lvl_internal;
wire [31:0] rx_data_fifo_lvl_internal;

wire [31:0] readdata_nxt;

localparam FIFO_DEPTH_LOG2_MASK = 32 - (FIFO_DEPTH_LOG2 + 1);

// Address Decode
assign tfr_cmd_addr             = (addr == 4'h0);
assign rx_data_addr             = (addr == 4'h1);
assign control_addr             = (addr == 4'h2);
assign status_addr              = (addr == 4'h3);
assign slave_portid_addr        = (addr == 4'h4);
assign tfr_cmd_fifo_lvl_addr    = (addr == 4'h6);
assign rx_data_fifo_lvl_addr    = (addr == 4'h7);

// Write access
assign tfr_cmd_wren            = tfr_cmd_addr & write;
assign control_wren            = control_addr & write;
assign status_wren             = status_addr & write; 
assign control_wren            = control_addr & write; 
assign slave_portid_wren       = slave_portid_addr & write; 
assign tfr_cmd_wren            = tfr_cmd_addr & write;

// Read access
assign rx_data_rden             = rx_data_addr & read;
assign status_rden              = status_addr & read;
assign control_rden             = control_addr & read;
assign slave_portid_rden        = slave_portid_addr & read; 
assign tfr_cmd_fifo_lvl_rden    = tfr_cmd_fifo_lvl_addr & read;
assign rx_data_fifo_lvl_rden    = rx_data_fifo_lvl_addr & read;

// ARBLOST_DET bit
always @(posedge clk or negedge rst_n) begin
    if (!rst_n)
        arblost_det     <= 1'b0;
    else if (arb_lost) 
        arblost_det     <= 1'b1;
    else
        arblost_det     <= 1'b0;
end

// NACK_DET bit
always @(posedge clk or negedge rst_n) begin
    if (!rst_n)
        nack_det    <= 1'b0;
    else if (abrt_txdata_noack | abrt_7b_addr_noack) 
        nack_det    <= 1'b1;
    else
        nack_det    <= 1'b0;
end

// TX packet nack register & core FSM state
always @(posedge clk or negedge rst_n) begin
    if (!rst_n)
        status       <= 4'b0000;
    else if (mstfsm_idle_state)
        status[0]    <= 1'b1;
    else if (!mstfsm_idle_state)
        status[0]    <= 1'b0;
    else if (abrt_txcrc8_noack)
        status[1]    <= 1'b1;
    else if (abrt_txdata_noack)
        status[2]    <= 1'b1;
    else if (abrt_7b_addr_noack)
        status[3]    <= 1'b1;
    else if (status_wren & writedata[1])  // NIOS write 1 to clear
        status[1]    <= 1'b0;
    else if (status_wren & writedata[2])  // NIOS write 1 to clear
        status[2]    <= 1'b0;
    else if (status_wren & writedata[3])  // NIOS write 1 to clear
        status[3]    <= 1'b0;
end

always @(posedge clk or negedge rst_n) begin
    if (!rst_n)
        control       <= 2'b00;
    else if (control_wren)
        control       <= writedata[1:0];
end

always @(posedge clk or negedge rst_n) begin
    if (!rst_n)
        slave_portid       <= 3'b000;
    else if (slave_portid_wren)
        slave_portid       <= writedata[2:0];
end

assign speed_mode = control[0];

assign pec_enable = control[1];

// SCL_LOW Register
assign scl_low = speed_mode ? 16'd38 : 16'd150;  // speed_mode = 0 is 100khz speed_mode = 1 is 400khz

// SCL_HIGH Register
assign scl_high = speed_mode ? 16'd38 : 16'd150;  // speed_mode = 0 is 100khz speed_mode = 1 is 400khz

// SDA_HOLD Register
assign sda_hold = 16'd1;

// AVMM read data path
assign rx_data_internal             = {24'h0, rx_fifo_data_out};
assign tfr_cmd_fifo_lvl_internal    = {{FIFO_DEPTH_LOG2_MASK{1'b0}}, txfifo_navail};
assign rx_data_fifo_lvl_internal    = {{FIFO_DEPTH_LOG2_MASK{1'b0}}, rxfifo_navail};

assign readdata_nxt     =   {{28{1'b0}}, (status & {4{status_rden_dly}})} |
                            {{30{1'b0}}, (control & {2{control_rden_dly}})} |
                            {{29{1'b0}}, (slave_portid & {3{slave_portid_rden_dly}})} | 
                            (tfr_cmd_fifo_lvl_internal & {32{tfr_cmd_fifo_lvl_rden_dly}})   |
                            (rx_data_fifo_lvl_internal & {32{rx_data_fifo_lvl_rden_dly}});
                            
// AVMM readdata register
always @(posedge clk or negedge rst_n) begin
    if (!rst_n)
        readdata_dly2 <= 32'h0;
    else
        readdata_dly2 <= readdata_nxt;
end

assign readdata = (rx_data_rden_dly2) ? rx_data_internal : readdata_dly2;

always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        rx_data_rden_dly            <= 1'b0;
        rx_data_rden_dly2           <= 1'b0;
        tfr_cmd_fifo_lvl_rden_dly   <= 1'b0;
        rx_data_fifo_lvl_rden_dly   <= 1'b0;
        status_rden_dly             <= 1'b0;
        control_rden_dly            <= 1'b0;
        slave_portid_rden_dly       <= 1'b0; 
    end
    else begin
        rx_data_rden_dly            <= rx_data_rden;
        rx_data_rden_dly2           <= rx_data_rden_dly;
        tfr_cmd_fifo_lvl_rden_dly   <= tfr_cmd_fifo_lvl_rden;
        rx_data_fifo_lvl_rden_dly   <= rx_data_fifo_lvl_rden;
        status_rden_dly             <= status_rden;
        control_rden_dly            <= control_rden;
        slave_portid_rden_dly       <= slave_portid_rden;
    end
end

// output assignment
assign write_txfifo     = tfr_cmd_wren;
assign read_rxfifo      = rx_data_rden;
assign ctrl_en          = 1'b1;
assign flush_txfifo     = arblost_det | nack_det;
assign scl_lcnt         = scl_low;
assign scl_hcnt         = scl_high;
assign txfifo_writedata = writedata[9:0];

endmodule
