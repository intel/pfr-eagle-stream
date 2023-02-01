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

`timescale 1 ps / 1 ps
`default_nettype none

module altr_i2c_avl_mst_intf_gen (
    input              i2c_clk,
    input              i2c_rst_n, 
    input              waitrequest,
    input              put_rx_databuffer,
    input              set_rd_req,		
    input              stop_det,
    input              start_det,
    input       [7:0]  rdata_rx_databuffer,
    input       [31:0] avl_readdata,
    input              avl_readdatavalid,   
    input              slv_tx_chk_ack,
    input              ack_det,
    input              is_mctp_pkt,
    //output reg         put_rxdata,
    output reg         avl_read,
    output reg 	[7:0]  readdatabyte,
    output reg         avl_readdatavalid_reg,
    output reg         avl_write,
    output reg  [31:0] avl_writedata,
    output wire [31:0] avl_addr
);

localparam [3:0] IDLE           = 4'b0000;
localparam [3:0] WRADDRBYTE     = 4'b0001;
localparam [3:0] WRDATABYTE     = 4'b0010;
localparam [3:0] ISSUE_WRITE    = 4'b0011;
localparam [3:0] NEXT_WRITE     = 4'b0100;
localparam [3:0] ISSUE_READ     = 4'b0101;
localparam [3:0] RDDATABYTE     = 4'b0110;

localparam ADDRWIDTH = 32;
localparam BYTEADDRWIDTH = 8;

reg [3:0]  fsm_state;
reg [3:0]  fsm_state_nxt;
reg        avl_read_nxt;
reg [7:0]  readdatabyte_nxt;
reg        avl_write_nxt;
reg [31:0] avl_writedata_nxt;
reg        ack_det_reg;
reg [ADDRWIDTH-1:0] load_addr_value_nxt;
reg [ADDRWIDTH-1:0] load_addr_value;
reg [BYTEADDRWIDTH-1:0] addr_cnt;

wire incr_addr;
wire load_addr;

assign incr_addr = ((fsm_state == RDDATABYTE && avl_readdatavalid) || (is_mctp_pkt == 0 && fsm_state == NEXT_WRITE && put_rx_databuffer));
assign load_addr = fsm_state == WRADDRBYTE;

generate if (BYTEADDRWIDTH == 32) begin
	assign avl_addr = addr_cnt;
end
else begin
	assign avl_addr = {{ADDRWIDTH-BYTEADDRWIDTH{1'b0}}, addr_cnt};
end
endgenerate

always @(posedge i2c_clk or negedge i2c_rst_n) begin
    if (!i2c_rst_n)
        fsm_state <= IDLE;
    else
        fsm_state <= fsm_state_nxt;
end

// Address increment counter
always @(posedge i2c_clk or negedge i2c_rst_n) begin
    if (!i2c_rst_n)
        addr_cnt <= {BYTEADDRWIDTH{1'b0}};
    else if (incr_addr)							// priority else if
        addr_cnt <= addr_cnt + 1'b1;
    else if (load_addr)
        addr_cnt <= load_addr_value[BYTEADDRWIDTH-1:0];
end

always @* begin
    case (fsm_state)
        IDLE: begin
            if (put_rx_databuffer)
            	fsm_state_nxt = WRADDRBYTE;
            else if (set_rd_req)		// to issue read for current address read
            	fsm_state_nxt = ISSUE_READ;
            else
            	fsm_state_nxt = IDLE;
        end
        
        WRADDRBYTE:  begin
            if (stop_det || start_det)                            
            	fsm_state_nxt = IDLE;	
            else if (put_rx_databuffer)
                fsm_state_nxt = WRDATABYTE;
            else
            	fsm_state_nxt = WRADDRBYTE;
        end
        
        WRDATABYTE: begin
            if (stop_det || start_det)  // to detect stop or repeated start                                       
                fsm_state_nxt = IDLE;
            else
                fsm_state_nxt = ISSUE_WRITE;
        end
        
        ISSUE_WRITE: begin
            if (stop_det || start_det)  // to detect stop or repeated start                                       
            	fsm_state_nxt = IDLE;
            else begin
                if (~waitrequest)
                    fsm_state_nxt = NEXT_WRITE;
                else begin
                	fsm_state_nxt = ISSUE_WRITE;
                end
            end
        end
        
        NEXT_WRITE: begin
            if (stop_det || start_det)  // to detect stop or repeated start     
            	fsm_state_nxt = IDLE;
            else if (put_rx_databuffer)
            	fsm_state_nxt = WRDATABYTE;
            else
            	fsm_state_nxt = NEXT_WRITE;
        end
        
        ISSUE_READ: begin
            if (stop_det || start_det)  // to detect stop or repeated start                                       
            	fsm_state_nxt = IDLE;
            else if (~waitrequest)
            	fsm_state_nxt = RDDATABYTE;
            else
            	fsm_state_nxt = ISSUE_READ;
        end
        
        RDDATABYTE: begin
            if (stop_det || start_det)  // to detect stop or repeated start                                       
            	fsm_state_nxt = IDLE;
            else if (slv_tx_chk_ack && ack_det && ~ack_det_reg)
            	fsm_state_nxt = ISSUE_READ;
            else
            	fsm_state_nxt = RDDATABYTE;
        end
        
        default: begin
            fsm_state_nxt = 4'bxxxx;
        end
    endcase
end

always @(posedge i2c_clk or negedge i2c_rst_n) begin
    if (!i2c_rst_n) begin
        avl_read				<= 1'b0;
        load_addr_value         <= {ADDRWIDTH{1'b0}};
        readdatabyte			<= {8{1'b0}};
        avl_readdatavalid_reg	<= 1'b0;
        ack_det_reg				<= 1'b0;
        avl_write				<= 1'b0;
        avl_writedata			<= {32{1'b0}};
    end
    else begin
		avl_read				<= avl_read_nxt;
		load_addr_value			<= load_addr_value_nxt;
		readdatabyte    		<= readdatabyte_nxt;
		avl_readdatavalid_reg	<= avl_readdatavalid;
		ack_det_reg				<= ack_det;
		avl_write				<= avl_write_nxt;
		avl_writedata			<= avl_writedata_nxt;
    end
end

always @* begin
    avl_read_nxt = avl_read;
    load_addr_value_nxt = load_addr_value;
    readdatabyte_nxt = readdatabyte;
    avl_write_nxt = avl_write;
    avl_writedata_nxt = avl_writedata;
    		
    case (fsm_state_nxt)
        IDLE: begin
            avl_read_nxt = 1'b0;
            load_addr_value_nxt = {ADDRWIDTH{1'b0}};
            readdatabyte_nxt = {8{1'b0}};
            avl_write_nxt = 1'b0;
            avl_writedata_nxt = {32{1'b0}};
        end
        
        WRADDRBYTE: begin
            load_addr_value_nxt = {24'h0, rdata_rx_databuffer};
        end 
        
        WRDATABYTE: begin
            avl_write_nxt = 1'b0;
        end 
        
        ISSUE_WRITE: begin
            avl_write_nxt = 1'b1;
            avl_writedata_nxt = {24'h0, rdata_rx_databuffer};
        end
        
        NEXT_WRITE: begin   
            avl_write_nxt = 1'b0;
        end
        
        ISSUE_READ:  begin
            avl_read_nxt = 1'b1;
        end
        
        RDDATABYTE:  begin
            avl_read_nxt = 1'b0;
            if (avl_readdatavalid) begin
                readdatabyte_nxt = avl_readdata[7:0];
            end
        end
        
        default: begin
            avl_read_nxt = 1'bx;
            load_addr_value_nxt = {ADDRWIDTH{1'bx}};
            readdatabyte_nxt = {8{1'bx}};
            avl_write_nxt = 1'bx;
            avl_writedata_nxt = {32{1'bx}};
        end
    endcase
end

endmodule
