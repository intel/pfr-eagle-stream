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


module sha_top (
        clk,
        rst,
        start,
	      input_valid,
        win,
	      hash_size,
        last,
        output_valid,
        hout
        );

input clk, rst, start;
input input_valid;
input [1:0] hash_size;
input last;
input [63:0] win;

output output_valid;
output [511:0] hout;

reg hash_en;

reg [6:0] cnt;
wire [63:0] kt_out;

wire [511:0] h_init;

wire [63:0] hout_a, hout_e, hout_rnd_a, hout_rnd_e; 

wire [63:0] hin_init_a, hin_init_e;
wire [63:0] hout_add_a, hout_add_e;

wire [63:0] wout;

//############### Combinational Logic ######

assign hout_add_a = hout_rnd_a + hin_init_a;
assign hout_add_e = hout_rnd_e + hin_init_e;

assign hout_a = {hout_add_a[63:33], hash_size[1] & hout_add_a[32], hout_add_a[31:0]} ;
assign hout_e = {hout_add_e[63:33], hash_size[1] & hout_add_e[32], hout_add_e[31:0]} ;


//--------------------------------------------------------------------------------------------
// H_out selection:
//      1. Implementing hout as (1) will provide the correct hashout when output valid is high
//         But is valid only for the time period that the output_valid is high. (NOT STATIC)
//      2. Implementing hout as (2) will provide the correct hashout AFTER output valid is high
//         This is a STATIC implementation, which will hold the correct hash.
//--------------------------------------------------------------------------------------------

//assign hout = {hout_a,h_init[511:320],hout_e,h_init[255:64]};

assign hout = h_init;

assign output_valid = (hash_en == 1'b0) ? 1'b0 :
	       (hash_size[1] == 1'b0 && cnt == 7'd63) ? 1'b1 :
	       (hash_size[1] == 1'b1 && cnt == 7'd79) ? 1'b1 :
		1'b0;


//############### Registers ################

always @(posedge clk or negedge rst)
begin

   if (rst==1'b0)
   begin

        hash_en <= 1'b0; 
        cnt <= 7'd0;

   end

   else
   begin
        hash_en <= (last == 1'b1 ) ? 1'b1 : 
		   ((hash_size[1]==1'b0 && cnt== 7'd63) || (hash_size[1]==1'b1 && cnt== 7'd79)) ? 1'b0 :
		   hash_en;

        cnt <= (hash_en == 1'b0)? 7'h0 :
	       ((hash_size[1]==1'b0 && cnt<= 7'd62) || (hash_size[1]==1'b1 && cnt<= 7'd78))? cnt+1'b1 : 7'h0 ;

   end

end

//################## Module Instantiation #####

kt kt_inst (
	.hash_size(hash_size[1]),
	.cnt(cnt),
	.kt_out(kt_out)
	);

sha_scheduler_top msg_schedule_top (
	.clk(clk),
	.rst(rst),
	.start(start),
	.input_valid(input_valid),
	.cnt(cnt),
	.hash_size(hash_size),
  .hash_en(hash_en),
	.win(win),
	.w_out(wout)
	);

hin_init init_state (
	.clk(clk),
	.rst(rst),
	.start(start),
	.cnt(cnt),
	.hash_size(hash_size),
	.hin_init_a_new(hout_a),
	.hin_init_e_new(hout_e),
	.hin_init_a(hin_init_a),
	.hin_init_e(hin_init_e),
	.h_init(h_init)
	);

sha_round_top sha_inst_top (
	.clk(clk),
	.rst(rst),
	.start(start),
	.input_valid(input_valid),
	.hash_size(hash_size),
	.win(wout),
	.kin(kt_out),
	.cnt(cnt),
	.h_init(h_init),
  .hash_en(hash_en),
	.hout_a(hout_rnd_a),
	.hout_e(hout_rnd_e)
	);

endmodule