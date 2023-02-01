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


// timer_bank
//
// This module implements the a bank of 20ms timers. Each word represents an
// independent timer. The timer value is bits 19:0. The start/stop bit is
// bit 28.

`timescale 1 ps / 1 ps
`default_nettype none

module timer_bank (
	input wire clk,
	input wire clk2M,
	input wire areset,

	input wire [2:0] avmm_address,
	input wire avmm_read,
	input wire avmm_write,
	output logic [31:0] avmm_readdata,
	input wire [31:0] avmm_writedata
);
	localparam NUM_TIMERS = 5;
	localparam CEIL_LOG2_NUM_TIMERS = $clog2(NUM_TIMERS);
	localparam TIMER_WIDTH = 20;


	reg r20msCE;
	reg [14:0] r20ms_pulse_clk_div;


	reg [TIMER_WIDTH-1:0] timers [NUM_TIMERS-1:0];
	reg [NUM_TIMERS-1:0] timer_active;

	reg [1:0] edge_tracker_20msCE;

	// generate a 20ms clock pulse using the 2Mhz clock
	always_ff @(posedge clk2M or posedge areset) begin
		if (areset) begin
			r20ms_pulse_clk_div <= 15'b0;
			r20msCE <= 1'b0;
		end
		else begin
			r20ms_pulse_clk_div <= r20ms_pulse_clk_div + 15'b1;
			if (r20ms_pulse_clk_div >= 15'd19999) begin
				r20ms_pulse_clk_div <= 15'b0;
				r20msCE <= ~r20msCE;
			end
		end
	end

	// Track the rising edge of the 20msCE. Since this is
	// synchronous to clk, we don't need extra synchronization
	always_ff @(posedge clk or posedge areset) begin
		if (areset) begin
			edge_tracker_20msCE <= 2'b0;
		end
		else begin
			edge_tracker_20msCE <= {edge_tracker_20msCE[0], r20msCE};
		end
	end

	// Implement the timers. Each timer is independely controlled using
	// timer_active. Decrement on the timer only occurs when the 20ms
	// pulse has occurred. 
	//
	// AVMM write is also controlled in this process
	always_ff @(posedge clk or posedge areset) begin
		if (areset) begin
			for (integer i = 0; i < NUM_TIMERS; i++) begin
				timers[i] <= {TIMER_WIDTH{1'b0}};
				timer_active[i] <= 1'b0;
			end
		end
		else begin
			if (edge_tracker_20msCE == 2'b01) begin
				// Active transition on 20ms clock enable
				for (integer i = 0; i < NUM_TIMERS; i++) begin
					if (timer_active[i] && (timers[i] != {TIMER_WIDTH{1'b0}})) begin
						timers[i] <= timers[i] - 1'b1;
					end
				end
			end

			// AVMM Write will overwrite any timer activity from above
			if (avmm_write) begin
				timers[avmm_address[CEIL_LOG2_NUM_TIMERS-1:0]] <= avmm_writedata[TIMER_WIDTH-1:0];
				timer_active[avmm_address[CEIL_LOG2_NUM_TIMERS-1:0]] <= avmm_writedata[28];
			end

		end
	end


	// AVMM read interface
	always_comb begin
		if (avmm_read) begin
			avmm_readdata <= {3'b0, timer_active[avmm_address[CEIL_LOG2_NUM_TIMERS-1:0]], {(28-TIMER_WIDTH){1'b0}}, timers[avmm_address[CEIL_LOG2_NUM_TIMERS-1:0]]};
		end
		else begin
			avmm_readdata = 32'bX;
		end
	end

endmodule

