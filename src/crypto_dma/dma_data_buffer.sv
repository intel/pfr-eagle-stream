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


// This module instantiate DCFIFO which handle data clock crossing from spi_clk to sys_clk

`timescale 1 ps / 1 ps
`default_nettype none

module dma_data_buffer #(
    parameter FIFO_ENABLE = 1
) (
    input wire [31:0]       data,
	input wire              rdclk,
	input wire              rdreq,
	input wire              wrclk,
	input wire              wrreq,
    input wire              areset,
	output logic [31:0]     q,
	output logic            rdempty,
	output logic            wrempty,
	output logic            wrfull,
	output logic [7:0]      wrusedw
);


generate
    if (FIFO_ENABLE) begin
        
        dma_dcfifo u_dma_dcfifo (
            .aclr       (areset),
            .data       (data),
	        .rdclk      (rdclk),
	        .rdreq      (rdreq),
	        .wrclk      (wrclk),
	        .wrreq      (wrreq),
	        .q          (q),
	        .rdempty    (rdempty),
	        .wrempty    (wrempty),
	        .wrfull     (wrfull),
	        .wrusedw    (wrusedw)
        );

    end else begin
        // Double data buffer register implementation
        
        logic wr_ptr;
        logic rd_ptr;
        logic [31:0] q_array[1:0];
        logic [1:0] internal_used;

        always_ff @(posedge wrclk) begin
            if (wrreq) begin
                q_array[wr_ptr] <= data;
            end
        end

        assign q = q_array[rd_ptr];
        
        always_ff @(posedge wrclk or posedge areset) begin
            if (areset) begin
                wr_ptr <= 1'b0;
                rd_ptr <= 1'b0;
                internal_used <= 2'd0;
            end
            else begin
                if (wrreq) begin
                    wr_ptr <= wr_ptr + 1'b1;
                end
                
                if (rdreq) begin
                    rd_ptr <= rd_ptr + 1'b1;
                end

                case ({wrreq, rdreq})
                    2'b01 : internal_used <= internal_used - 1'b1;
                    2'b10 : internal_used <= internal_used + 1'b1;
                    default : internal_used <= internal_used;
                endcase

            end
        end
        
        assign wrempty  = internal_used == 2'd0;
        assign rdempty  = internal_used == 2'd0;
        assign wrfull   = internal_used == 2'd2;
        assign wrusedw  = {6'd0, internal_used};

    end

endgenerate


endmodule
