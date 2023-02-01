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

module max10_trng_entropy_adjust #(
   parameter DATA_WIDTH   = 32,				    
   parameter CHAIN_LEN    = 32, 
   parameter OBSERVE_RUNS = 5
)(
   input wire                 clk,
   input wire                 rst,
   input wire                 parallel_in_valid,
   input wire 		      consecutive_zeros,
   input wire 		      consecutive_ones,   
   input wire 		      repeated_bits,
   output reg [CHAIN_LEN-1:0] calibrate_a,
   output reg [CHAIN_LEN-1:0] calibrate_b,
   output reg 		      adjusting
);

localparam OBSERVED_COUNT_BITS = $clog2(OBSERVE_RUNS);

reg [OBSERVED_COUNT_BITS-1:0] observe_count;

always@(posedge clk) begin
   if (rst) begin
      adjusting <= 1'b1;
      observe_count <= {OBSERVED_COUNT_BITS{1'b0}};
      calibrate_a <= {{CHAIN_LEN-1{1'b0}},1'b1};
      calibrate_b <= {{CHAIN_LEN-1{1'b0}},1'b1};
   end else begin
      if (parallel_in_valid) begin
         if (consecutive_zeros | consecutive_ones | repeated_bits) begin
            adjusting <= 1'b1;
            observe_count <= {OBSERVED_COUNT_BITS{1'b0}};
            calibrate_a <= {calibrate_a[CHAIN_LEN-2:0], calibrate_a[CHAIN_LEN-1]};
	    if (calibrate_a[CHAIN_LEN-1]) calibrate_b <= {calibrate_b[CHAIN_LEN-2:0], calibrate_b[CHAIN_LEN-1]};	     
	 end else begin
            if (observe_count < OBSERVE_RUNS-1) begin
               adjusting <= 1'b1;
               observe_count <= observe_count + {{OBSERVED_COUNT_BITS-1{1'b0}}, 1'b1};	     
            end else adjusting <= 1'b0;	     
         end
      end
   end
end

endmodule
