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
module ecdsa384_top_tb;

	// Inputs
	reg clk;
	reg resetn;
	reg sw_reset;
	reg ecc_start;
	reg [383:0] data_in;
	reg din_valid;
	reg [4:0] ecc_ins;
	reg ins_valid;
	reg [47:0] byteena_data_in;
	
	// Outputs
	wire ecc_done;
	wire ecc_busy;
	wire dout_valid;
	wire [383:0] cx_out;
	wire [383:0] cy_out;

   //temp
	reg [22:0] flag;
    reg [3:0] ins_count;


	// Instantiate the Unit Under Test (UUT)
	ecdsa384_top i_uut(
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
        flag = 0;
		ins_count = 0;
		byteena_data_in = 0;
		// Wait 100 ns for global reset to finish
		#100;
            resetn = 1;
		#20 resetn = 0;
		#20 resetn = 1;

   		ins_count = 4'd1; //ECMULT
		ecc_start = 1;
		#20 
        ecc_start = 0;
		#80000000
		
        ins_count = 4'd0;
		ecc_start = 1;
		#20 
        ecc_start = 0;		
        #80000000 //Sign 

		ins_count = 4'd8; //Verify
		ecc_start = 1;
		#20 
        ecc_start = 0;
		#60000000 
      $stop;   
	end

always @(posedge clk) begin
 if(ecc_start) begin
   byteena_data_in = 48'hFFFFFFFFFFFF;
   flag[22:0] <= 23'd1;
 end
 else if(flag[0]) begin
    ecc_ins <= 5'b00001;
	ins_valid <= 1'b1;
	flag[0] <= 1'b0;
	flag[1] <= 1'b1;
 end
 else if(flag[1]) begin
	ins_valid <= 1'b0;
	data_in <= 384'haa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7; //ax for [d]A
	din_valid <= 1'b1;
	flag[1] <= 1'b0;
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
	data_in <=  384'h3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f; //ay
	din_valid <= 1'b1;
	flag[3] <= 1'b0;
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
	if(ins_count == 4'd0) 
	  data_in <= 384'd0;
	else 
	  data_in <= 384'hfda78b0ab4ac4b0ec6c66ac03ceb861637f0e2a2503f92be4abbf2b401e878caa799429b6cd63d7d7ab5dff1cc0a961b;  //bx  
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
	if(ins_count == 4'd8)
	  data_in <= 384'ha07404ae3aed5e2df24fc0bf7c580e6761a882f571ff6642b7a4a7608b16f2cda6c659b8c4d56f5d0749c1838efbb1a6;  //by  
	else if (ins_count == 4'd0)
  	  data_in <= 384'ha4ebcae5a665983493ab3e626085a24c104311a761b5a8fdac052ed1f111a5c44f76f45659d2d111a61b5fdd97583480; //d
	else data_in <= 384'd0;  
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
	data_in <= 384'hfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff;  //p
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
	data_in <= 384'hfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000fffffffc;  //a
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
	data_in <= 384'hffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973;  //n
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
	  data_in <= 384'ha4ebcae5a665983493ab3e626085a24c104311a761b5a8fdac052ed1f111a5c44f76f45659d2d111a61b5fdd97583480; //k
	else if (ins_count == 4'd1)
	  data_in <= 384'ha4ebcae5a665983493ab3e626085a24c104311a761b5a8fdac052ed1f111a5c44f76f45659d2d111a61b5fdd97583480; //d 
	else 
	  data_in <= 384'hfda78b0ab4ac4b0ec6c66ac03ceb861637f0e2a2503f92be4abbf2b401e878caa799429b6cd63d7d7ab5dff1cc0a961b; //r 
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
	data_in <= 384'hffffffffffffffffffffffffffffffffffffffffffffffff86ec2cd33d40aeaef14731a3ce4bca2a0147a0155997f573;   //e
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
	 data_in <= 384'h167cad0a5efef80c66af3a381a725eeaef6155e2de10024574bca90d8127e15a839b9f079bd80333fb085c8aadba987e;  //lambda  
	else 
	 data_in <= 384'h167cad0a5efef80c66af3a381a725eeaef6155e2de10024574bca90d8127e15a839b9f079bd80333fb085c8aadba987e;  //s	
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
   if((cx_out == 384'hfda78b0ab4ac4b0ec6c66ac03ceb861637f0e2a2503f92be4abbf2b401e878caa799429b6cd63d7d7ab5dff1cc0a961b) &(cy_out == 384'h167cad0a5efef80c66af3a381a725eeaef6155e2de10024574bca90d8127e15a839b9f079bd80333fb085c8aadba987e))
    $display("ECDSA Sign PASS");
   else begin
	$display("ECDSA Sign FAIL!\n Expected: \ncx = 384'hfda78b0ab4ac4b0ec6c66ac03ceb861637f0e2a2503f92be4abbf2b401e878caa799429b6cd63d7d7ab5dff1cc0a961b \cy = 384'h167cad0a5efef80c66af3a381a725eeaef6155e2de10024574bca90d8127e15a839b9f079bd80333fb085c8aadba987e");
	$display("Received: \ncx = %x \ncy = %x", cx_out, cy_out);
   end	
  end
  else if(ins_count == 4'd1) begin
   if((cx_out == 384'hfda78b0ab4ac4b0ec6c66ac03ceb861637f0e2a2503f92be4abbf2b401e878caa799429b6cd63d7d7ab5dff1cc0a961b) &(cy_out == 384'ha07404ae3aed5e2df24fc0bf7c580e6761a882f571ff6642b7a4a7608b16f2cda6c659b8c4d56f5d0749c1838efbb1a6))
    $display("ECDSA Public-key-gen PASS");
   else begin
	$display("ECDSA Public-key-gen FAIL!\n Expected: \ncx = 384'hfda78b0ab4ac4b0ec6c66ac03ceb861637f0e2a2503f92be4abbf2b401e878caa799429b6cd63d7d7ab5dff1cc0a961b \cy = 384'ha07404ae3aed5e2df24fc0bf7c580e6761a882f571ff6642b7a4a7608b16f2cda6c659b8c4d56f5d0749c1838efbb1a6e");
	$display("Received: \ncx = %x \ncy = %x", cx_out, cy_out);
   end	
  end
  else if(ins_count == 4'd8) begin
   if((cx_out == 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000))
    $display("ECDSA-VERIFY PASS");
   else begin
	$display("ECDSA-VERIFY FAIL!\n Expected: \ncx = 384'h000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
	$display("Received: \ncx = %x", cx_out);
   end	
  end
 end	
end

endmodule

