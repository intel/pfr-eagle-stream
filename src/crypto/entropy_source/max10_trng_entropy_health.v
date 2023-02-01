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

module max10_trng_entropy_health #(
   parameter DATA_WIDTH = 32				    
)(
   input  wire                  clk,
   input  wire                  rst,
   input  wire                  serial_in,
   input  wire                  serial_in_valid,
   output reg                   consecutive_zeros,
   output reg                   consecutive_ones,
   output reg                   repeated_bits,
   output wire                  health_check_done,
   output wire [DATA_WIDTH-1:0] parallel_out,
   output wire                  parallel_out_valid
);

integer j;
reg [DATA_WIDTH-1:0] serial_in_sr;
reg [DATA_WIDTH-1:0] serial_in_valid_sr;
reg [1:0]            valid;
   
always @(posedge clk) begin
   if (rst) begin
      serial_in_sr <= {DATA_WIDTH{1'b0}};      
      serial_in_valid_sr <= {DATA_WIDTH{1'b0}};
      consecutive_zeros <= 1'b1; 
      consecutive_ones <= 1'b1;
      repeated_bits <= 1'b0;
      valid <= 2'b00;
   end else begin
      // Shift serial bit from race circuit 1 into 32-bit shift register
      if (serial_in_valid_sr[DATA_WIDTH-1]) begin
         serial_in_valid_sr <= {DATA_WIDTH{1'b0}};
      end else // Health check & recalibration requires 2 clock cycles
      if (~valid[0]) begin // so do not take in any new serial bit within this period     
         if (serial_in_valid) begin
            serial_in_sr[0] <= serial_in;
            serial_in_valid_sr[0] <= 1'b1;
            for (j=1; j<DATA_WIDTH; j=j+1) begin
               serial_in_sr[j] <= serial_in_sr[j-1];
               serial_in_valid_sr[j] <= serial_in_valid_sr[j-1];
            end	    
         end	     
      end 

      // Valid[0] = health check is done
      // Valid[1] = recalibration is done = parallel_out_valid
      valid[0] <= serial_in_valid_sr[DATA_WIDTH-1]; 
      valid[1] <= valid[0];
      
      // Check if 16 consecutive 0s (non-overlapping) is found at upper and lower 16-bit regions (for DATA_WIDTH = 32)
      // In this case, the longest run length is 30. 
      // ie. line 1 = 32'h80000001
      //     OR
      //     line 1 = 32'hxxxx8000
      //     line 2 = 32'h0001xxxx   
      if (serial_in_valid_sr[DATA_WIDTH-1]) begin
         if ((serial_in_sr[DATA_WIDTH-1:DATA_WIDTH/2] == {DATA_WIDTH/2{1'b0}}) || 
	     (serial_in_sr[DATA_WIDTH/2-1:0] == {DATA_WIDTH/2{1'b0}}))
            consecutive_zeros <= 1'b1;
         else consecutive_zeros <= 1'b0;
      end

      // Check if 16 consecutive 1s (non-overlapping) is found at upper and lower 16-bit regions (for DATA_WIDTH = 32)
      // In this case, the longest run length is 30. 
      // ie. line 1 = 32'h7ffffffe
      //     OR
      //     line 1 = 32'hxxxx7fff
      //     line 2 = 32'hfffexxxx		
      if (serial_in_valid_sr[DATA_WIDTH-1]) begin
         if ((serial_in_sr[DATA_WIDTH-1:DATA_WIDTH/2] == {DATA_WIDTH/2{1'b1}}) || 
             (serial_in_sr[DATA_WIDTH/2-1:0] == {DATA_WIDTH/2{1'b1}}))
            consecutive_ones <= 1'b1;
         else consecutive_ones <= 1'b0;
      end
		
      // Check if repeating sequence is found between two 16-bit chunks of data.
      if (serial_in_valid_sr[DATA_WIDTH-1]) repeated_bits <= serial_in_sr[DATA_WIDTH-1:DATA_WIDTH/2] == serial_in_sr[DATA_WIDTH/2-1:0];
   end
end

assign health_check_done = valid[0];
assign parallel_out = serial_in_sr;
assign parallel_out_valid = valid[1];

endmodule
