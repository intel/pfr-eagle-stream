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

//`timescale 1 ps / 1 ps
module ecdsa256_top_tb;

	// Inputs
	reg clk;
	reg resetn;
	reg sw_reset;
	reg ecc_start;
	reg [255:0] data_in;
	reg din_valid;
	reg [4:0] ecc_ins;
	reg ins_valid;
    reg [31:0] byteena_data_in;
	
	// Outputs
	wire ecc_done;
	wire ecc_busy;
	wire dout_valid;
	wire [255:0] cx_out;
	wire [255:0] cy_out;

   //temp
	reg [23:0] flag;
    reg [3:0] ins_count;


	// Instantiate the Unit Under Test (UUT)
	ecdsa256_top i_uut(
		.ecc_done(ecc_done), 
		.ecc_busy(ecc_busy), 
		.dout_valid(dout_valid), 
		.cx_out(cx_out),
        .cy_out(cy_out),		
		.clk(clk), 
		.resetn(resetn),
        .sw_reset(sw_reset),		
		.ecc_start(ecc_start),
		.data_in(data_in), 
		.din_valid(din_valid), 
		.ecc_ins(ecc_ins), 
		.ins_valid(ins_valid),
		.byteena_data_in(byteena_data_in)
	);

	initial begin
		// Initialize Inputs
		clk = 0;
		resetn = 0;
		sw_reset = 0;
		ecc_start = 0;
		data_in = 0;
		din_valid = 0;
		ecc_ins = 0;
		ins_valid = 0;
		byteena_data_in = 0;
        flag = 0;
		ins_count = 0;
		// Wait 100 ns for global reset to finish
		#100;
        resetn = 1;
		#20 resetn = 0;
		#20 resetn = 1;
        
		ins_count = 4'd1; //ECMULT
		ecc_start = 1;
		#20 
        ecc_start = 0;
		#40000000
		
		ins_count = 4'd0; //Sign
		ecc_start = 1;
		#20 
        ecc_start = 0;
		#40000000
        
		ins_count = 4'd8; //Verify
		ecc_start = 1;
		#20 
        ecc_start = 0;
		#25000000 
      $stop;   
	end

always @(posedge clk) begin
 if(ecc_start) begin
   flag[23:0] <= 24'd1;
   byteena_data_in = 32'hFFFFFFFF;
 end
 else if(flag[0]) begin
    ecc_ins <= 5'b00001;
	ins_valid <= 1'b1;
	flag[0] <= 1'b0;
	flag[1] <= 1'b1;
 end
 else if(flag[1]) begin
	ins_valid <= 1'b0;
	data_in <= 256'h6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296; //ax for [d]A
	din_valid <= 1'b1;
	byteena_data_in = 32'h0000FFFF;
	flag[1] <= 1'b0;
	flag[23] <= 1'b1;
 end
 else if(flag[23]) begin
	ins_valid <= 1'b0;
	data_in <= 256'h6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296; //ax for [d]A
	din_valid <= 1'b1;
	byteena_data_in = 32'hFFFF0000;
	flag[23] <= 1'b0;
	flag[2] <= 1'b1;
 end
 else if(flag[2]) begin
   ecc_ins <= 5'b00010;
	ins_valid <= 1'b1;
	din_valid <= 1'b0;
	flag[2] <= 1'b0;
	flag[3] <= 1'b1;
 end
 else if(flag[3]) begin
	ins_valid <= 1'b0;
	byteena_data_in = 32'hFFFFFFFF;
	data_in <=  256'h4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5; //ay
	din_valid <= 1'b1;
	flag[3] <= 1'b0;
	if((ins_count > 4'd2) & (ins_count < 4'd8))
	  flag[8] <= 1'b1;
	else 
	  flag[4] <= 1'b1;
 end
 else if(flag[4]) begin
   ecc_ins <= 5'b00011;
	ins_valid <= 1'b1;
	din_valid <= 1'b0;
	flag[4] <= 1'b0;
	flag[5] <= 1'b1;
 end
 else if(flag[5]) begin
	ins_valid <= 1'b0;
	if(ins_count == 4'd8)
	  data_in <= 256'h942c9f408ead9d82d34a1b9a6a827ebe3e2ddf782b448d23be1b6143988ccef4;  //bx  
	else 
      data_in <= 256'd0;	
	din_valid <= 1'b1;
	flag[5] <= 1'b0;
	flag[6] <= 1'b1;
 end
 else if(flag[6]) begin
   ecc_ins <= 5'b00100;
	ins_valid <= 1'b1;
	din_valid <= 1'b0;
	flag[6] <= 1'b0;
	flag[7] <= 1'b1;
 end
 else if(flag[7]) begin
	ins_valid <= 1'b0;
	if((ins_count == 4'd0))
	  data_in <= 256'hc51e4753afdec1e6b6c6a5b992f43f8dd0c7a8933072708b6522468b2ffb06fd; //d 
	else if(ins_count == 4'd8)
	  data_in <= 256'h8c9eaf6c0d14d992fc63bad3e2496be2eee61cb5b97f65f428ca94a5d0ee19a1;  //by 
	else 
      data_in <= 256'd0;  
	din_valid <= 1'b1;
	flag[7] <= 1'b0;
	flag[8] <= 1'b1;
 end
 else if(flag[8]) begin
   ecc_ins <= 5'b00101;
	ins_valid <= 1'b1;
	din_valid <= 1'b0;
	flag[8] <= 1'b0;
	flag[9] <= 1'b1;
 end
 else if(flag[9]) begin
	ins_valid <= 1'b0;
	data_in <= 256'hffffffff00000001000000000000000000000000ffffffffffffffffffffffff;  //p
	din_valid <= 1'b1;
	flag[9] <= 1'b0;
	flag[13] <= 1'b1;
 end
 else if(flag[13]) begin
   ecc_ins <= 5'b00110;
	ins_valid <= 1'b1;
	din_valid <= 1'b0;
	flag[13] <= 1'b0;
	flag[14] <= 1'b1;
 end
 else if(flag[14]) begin
	ins_valid <= 1'b0;
	data_in <= 256'hffffffff00000001000000000000000000000000fffffffffffffffffffffffc;  //a
	din_valid <= 1'b1;
	flag[14] <= 1'b0;
	flag[15] <= 1'b1;
 end
 else if(flag[15]) begin
   ecc_ins <= 5'b10000;
	ins_valid <= 1'b1;
	din_valid <= 1'b0;
	flag[15] <= 1'b0;
	flag[16] <= 1'b1;
 end
 else if(flag[16]) begin
	ins_valid <= 1'b0;
	data_in <= 256'hffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551;  //n
	din_valid <= 1'b1;
	flag[16] <= 1'b0;
	flag[17] <= 1'b1;
 end
 else if(flag[17]) begin
   ecc_ins <= 5'b10001;
	ins_valid <= 1'b1;
	din_valid <= 1'b0;
	flag[17] <= 1'b0;
	flag[18] <= 1'b1;
 end
 else if(flag[18]) begin
	ins_valid <= 1'b0;
	din_valid <= 1'b1;
	if(ins_count == 4'd0)
	  data_in <= 256'hb51e4753afdec1e6b6c6a5b992f43f8dd0c7a8933072708b6522468b2ffb06ec; //k
	else if (ins_count == 4'd1)
	  data_in <= 256'hc51e4753afdec1e6b6c6a5b992f43f8dd0c7a8933072708b6522468b2ffb06fd; //d
	else 
	  data_in <= 256'hed597ca056fb8cd8a6bbd0ad11e8139663fa2314bd5fc89552d7f5a0cc1bc92d; //r
    flag[18] <= 1'b0;
	flag[19] <= 1'b1;
 end
 else if(flag[19]) begin
   ecc_ins <= 5'b10010;
	ins_valid <= 1'b1;
	din_valid <= 1'b0;
	flag[19] <= 1'b0;
	flag[20] <= 1'b1;
 end
 else if(flag[20]) begin
	ins_valid <= 1'b0;
    data_in <= 256'h1d90963ec91ee36586ec2cd33d40aeaef14731a3ce4bca2a0147a0155997f573;   //e
    din_valid <= 1'b1;
	flag[20] <= 1'b0;
	flag[21] <= 1'b1;
 end
  else if(flag[21]) begin
   ecc_ins <= 5'b10011;
	ins_valid <= 1'b1;
	din_valid <= 1'b0;
	flag[21] <= 1'b0;
	flag[22] <= 1'b1;
 end
 else if(flag[22]) begin
	ins_valid <= 1'b0;
	if((ins_count == 4'd0) | (ins_count == 4'd1))
	 data_in <= 256'h9867cc6e8a12d977f8268cc247058b58ead7c9d16a4458a3791d874b5647ef9e;  //lambda
	else
     data_in <= 256'h070a6c05a41852218f9e887102d332da6d5bbc312a89ed1536e0c09a2577050a;  //s
    din_valid <= 1'b1;
	flag[22] <= 1'b0;
	flag[10] <= 1'b1;
 end
 else if(flag[10]) begin
   ecc_ins <= 5'b00111 + ins_count;  //[d]A
	ins_valid <= 1'b1;
	din_valid <= 1'b0;
	flag[10] <= 1'b0;
	flag[11] <= 1'b1;
 end
 else if(flag[11]) begin
	ins_valid <= 1'b0;
	if(ecc_done) begin
	  flag[11] <= 1'b0;
	  flag[12] <= 1'b1;
	end  
 end
 else if(flag[12]) begin
	flag[12] <= 1'b0;
 end
end


always #10 clk = ~clk;
	
always @(posedge clk) begin
 if(ecc_done) begin 
  if(ins_count == 4'd0) begin
   if((cx_out == 256'hed597ca056fb8cd8a6bbd0ad11e8139663fa2314bd5fc89552d7f5a0cc1bc92d) & (cy_out == 256'h070a6c05a41852218f9e887102d332da6d5bbc312a89ed1536e0c09a2577050a))
    $display("ECDSA-SIGN PASS");
   else begin
	$display("ECDSA-SIGN FAIL!\n Expected: \ncx = 256'hed597ca056fb8cd8a6bbd0ad11e8139663fa2314bd5fc89552d7f5a0cc1bc92d \ncy = 256'h070a6c05a41852218f9e887102d332da6d5bbc312a89ed1536e0c09a2577050a");
	$display("Received: \ncx = %x\ncx = %x", cx_out, cy_out);
   end	
  end
  else if(ins_count == 4'd1) begin
   if((cx_out == 256'h942c9f408ead9d82d34a1b9a6a827ebe3e2ddf782b448d23be1b6143988ccef4) & (cy_out == 256'h8c9eaf6c0d14d992fc63bad3e2496be2eee61cb5b97f65f428ca94a5d0ee19a1))
    $display("ECDSA-Public-key-gen PASS");
   else begin
	$display("ECDSA--Public-key-gen FAIL!\n Expected: \ncx = 256'h942c9f408ead9d82d34a1b9a6a827ebe3e2ddf782b448d23be1b6143988ccef4 \ncy = 256'h8c9eaf6c0d14d992fc63bad3e2496be2eee61cb5b97f65f428ca94a5d0ee19a1");
	$display("Received: \ncx = %x\ncx = %x", cx_out, cy_out);
   end	
  end
  else if (ins_count == 4'd8)begin
   if((cx_out == 256'd0))
    $display("ECDSA-VERIFY PASS");
   else begin
	$display("ECDSA-VERIFY FAIL!\n Expected: \ncx = 256'h0");
	$display("Received: \ncx = %x", cx_out);
   end	
  end
 end	
end

endmodule

