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



// Suppress Quartus Warning Messages
// altera message_off 10036
module sha_round (
    input hash_size,	
    input [511:0]  state_in,
    input [63:0]   w_in,
    input [63:0]   k_in,
    output [63:0] state_out_a,
    output [63:0] state_out_e
);


wire [63:0] state_a;
wire [63:0] state_b;
wire [63:0] state_c;
wire [63:0] state_d;
wire [63:0] state_e;
wire [63:0] state_f;
wire [63:0] state_g;
wire [63:0] state_h;

wire [63:0] updated_st_a;
wire [63:0] updated_st_b;
wire [63:0] updated_st_c;
wire [63:0] updated_st_d;
wire [63:0] updated_st_e;
wire [63:0] updated_st_f;
wire [63:0] updated_st_g;
wire [63:0] updated_st_h;

wire [63:0] Maj, Ch, Sigma0, Sigma1;

wire [63:0] h_kt_w_sum, h_kt_w_carry;
wire [63:0] h_kt_w_ch_sum, h_kt_w_ch_carry;
wire [63:0] h_kt_w_ch_Sigma1_sum, h_kt_w_ch_Sigma1_carry;
wire [63:0] h_kt_w_ch_Sigma1_d_sum, h_kt_w_ch_Sigma1_d_carry;

wire [63:0] Sigma0_maj_Sigma1_sum, Sigma0_maj_Sigma1_carry;
wire [63:0] Sigma0_maj_Sigma1_t1_sum, Sigma0_maj_Sigma1_t1_carry;
wire [63:0] Sigma0_maj_Sigma1_t2_sum, Sigma0_maj_Sigma1_t2_carry;

assign state_a = state_in[511:448];
assign state_b = state_in[447:384];
assign state_c = state_in[383:320];
assign state_d = state_in[319:256];
assign state_e = state_in[255:192];
assign state_f = state_in[191:128];
assign state_g = state_in[127:64 ];
assign state_h = state_in[63:0   ];

//********************* SHA Round Functions ************************
maj fn_maj (
	.a(state_a),
	.b(state_b),
	.c(state_c),
	.out(Maj)
	);

Sigma0 fn_Sigma0 (
	.hash_size(hash_size),
	.in(state_a),
	.out(Sigma0)
	);

ch fn_ch (
	.a(state_e),
	.b(state_f),
	.c(state_g),
	.out(Ch)
	);

Sigma1 fn_Sigma1 (
	.hash_size(hash_size),
	.in(state_e),
	.out(Sigma1)
	);

//******************** 32/64-bit Additions new-E computation ***************************

csa_64 csa0 (
	.hash_size(hash_size),
	.a(state_h),
	.b(k_in),
	.c(w_in),
	.sum_out(h_kt_w_sum),
	.carry_out(h_kt_w_carry)
	);

csa_64 csa1 (
	.hash_size(hash_size),
	.a(h_kt_w_sum),
	.b(h_kt_w_carry),
	.c(Ch),
	.sum_out(h_kt_w_ch_sum),
	.carry_out(h_kt_w_ch_carry)
	);

csa_64 csa2 (
	.hash_size(hash_size),
	.a(h_kt_w_ch_sum),
	.b(h_kt_w_ch_carry),
	.c(Sigma1),
	.sum_out(h_kt_w_ch_Sigma1_sum),
	.carry_out(h_kt_w_ch_Sigma1_carry)
	);

csa_64 csa3 (
	.hash_size(hash_size),
	.a(h_kt_w_ch_Sigma1_sum),
	.b(h_kt_w_ch_Sigma1_carry),
	.c(state_d),
	.sum_out(h_kt_w_ch_Sigma1_d_sum),
	.carry_out(h_kt_w_ch_Sigma1_d_carry)
	);

//******************** 32/64-bit Additions new-A computation ***************************

csa_64 csa4 (
	.hash_size(hash_size),
	.a(Sigma0),
	.b(Maj),
	.c(Sigma1),
	.sum_out(Sigma0_maj_Sigma1_sum),
	.carry_out(Sigma0_maj_Sigma1_carry)
	); 

csa_64 csa5 (
	.hash_size(hash_size),
	.a(Sigma0_maj_Sigma1_sum),
	.b(h_kt_w_ch_sum),
	.c(h_kt_w_ch_carry),
	.sum_out(Sigma0_maj_Sigma1_t1_sum),
	.carry_out(Sigma0_maj_Sigma1_t1_carry)
	); 

csa_64 csa6 (
	.hash_size(hash_size),
	.a(Sigma0_maj_Sigma1_carry),
	.b(Sigma0_maj_Sigma1_t1_sum),
	.c(Sigma0_maj_Sigma1_t1_carry),
	.sum_out(Sigma0_maj_Sigma1_t2_sum),
	.carry_out(Sigma0_maj_Sigma1_t2_carry)
	); 

//*************************************************************************************

assign updated_st_a = Sigma0_maj_Sigma1_t2_sum + Sigma0_maj_Sigma1_t2_carry;
assign updated_st_b = state_a;
assign updated_st_c = state_b;
assign updated_st_d = state_c;
assign updated_st_e = h_kt_w_ch_Sigma1_d_sum + h_kt_w_ch_Sigma1_d_carry;
assign updated_st_f = state_e;
assign updated_st_g = state_f;
assign updated_st_h = state_g;

assign state_out_e = {updated_st_e[63:33], hash_size & updated_st_e[32], updated_st_e[31:0]} ;
assign state_out_a = {updated_st_a[63:33], hash_size & updated_st_a[32], updated_st_a[31:0]} ;

endmodule
