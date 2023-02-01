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

module sha_unit (
	      clk,	            // SHA IP clock
        rst,	            // Active low asynchronous reset
        start,	          // Indicates the start of a new hash operation (high only during the last message block of the new message)
	      input_valid, 	    // Indicates the input data is valid
        win,	            // Input data/message
	      hash_size,      	// Indicates the size of hash --> 00 (Invalid); 01 (SHA-256); 10 (SHA-384); 11 (SHA-512)
        last,             /// Last msg Block being (high when the last message block is being sent 
        output_valid,	    // Output signal indicating a valid hash value
        hout	            // Output hash
        );

input clk, rst, start, last;
input input_valid;
input [1:0] hash_size;
input [31:0] win;

output output_valid;
output [511:0] hout;

wire [63:0] sha_win;
wire [511:0] sha_hout;

assign sha_win [63:0] = {32'h0, win[31:0]};
//assign sha_win [63:0] = (hash_size[1]==1'b0) ? {32'h0,win[31:0]} : win[i63:0];

assign hout[511:384] = (hash_size[1]==1'b0) ? 128'h0            : ((hash_size[0]==1'b0) ? 128'h0            : sha_hout[511:384]);
assign hout[383:256] = (hash_size[1]==1'b0) ? 128'h0            : ((hash_size[0]==1'b0) ? sha_hout[511:384] : sha_hout[383:256]);
assign hout[255:224] = (hash_size[1]==1'b0) ? sha_hout[479:448] : ((hash_size[0]==1'b0) ? sha_hout[383:352] : sha_hout[255:224]);
assign hout[223:192] = (hash_size[1]==1'b0) ? sha_hout[415:384] : ((hash_size[0]==1'b0) ? sha_hout[351:320] : sha_hout[223:192]);
assign hout[191:160] = (hash_size[1]==1'b0) ? sha_hout[351:320] : ((hash_size[0]==1'b0) ? sha_hout[319:288] : sha_hout[191:160]);
assign hout[159:128] = (hash_size[1]==1'b0) ? sha_hout[287:256] : ((hash_size[0]==1'b0) ? sha_hout[287:256] : sha_hout[159:128]);
assign hout[127:96 ] = (hash_size[1]==1'b0) ? sha_hout[223:192] : ((hash_size[0]==1'b0) ? sha_hout[255:224] : sha_hout[127:96 ]);
assign hout[95:64  ] = (hash_size[1]==1'b0) ? sha_hout[159:128] : ((hash_size[0]==1'b0) ? sha_hout[223:192] : sha_hout[95:64  ]);
assign hout[63:32  ] = (hash_size[1]==1'b0) ? sha_hout[95:64  ] : ((hash_size[0]==1'b0) ? sha_hout[191:160] : sha_hout[63:32  ]);
assign hout[31:0   ] = (hash_size[1]==1'b0) ? sha_hout[31:0   ] : ((hash_size[0]==1'b0) ? sha_hout[159:128] : sha_hout[31:0   ]);



sha_top sha_inst (
        .clk(clk),
        .rst(rst),
        .start(start),
	      .input_valid(input_valid),
        .win(sha_win),
	      .hash_size(hash_size),
        .last(last),
        .output_valid(output_valid),
        .hout(sha_hout)
        );
  

endmodule

