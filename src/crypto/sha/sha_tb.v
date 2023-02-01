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

`include "ch.v"
`include "maj.v"
`include "sigma1.v"
`include "sigma0.v"
`include "csa_64.v"
`include "sha_scheduler_top.v"
`include "sha_round.v"
`include "sha_round_top.v"
`include "sha_scheduler.v"
`include "kt.v"
`include "hin_init.v"
`include "sha_top.v"
`include "sha_unit.v"

module sha_unit_tb ();

reg clk, rst, last;
reg [4:0] count;
reg [1023:0] msg_in;
reg input_valid;
reg new_block;

logic [31:0] win;
logic [511:0] hout;

logic [9:0] cnt_in;
logic [511:0] hash_golden;

logic [1:0] hash_size;
logic [5:0] test;
logic start;
logic output_valid;

initial 
begin


        $vcdplusfile("sha.vpd");
        $vcdpluson(0,sha_unit_tb);
        $vcdplusmemon();

        clk = 1'b0;
	cnt_in = 10'b0;
        rst =  1'b0;
	new_block = 1'b0;
        start = 1'b0;
	last = 1'b0;
        input_valid = 1'b0;
	count = 5'b0;
	

	#125 rst = 1'b1;

//****************** SHA-512: Message = 'abc' ************************
	
	msg_in[1023:960] = 64'h6162638000000000;
	msg_in[959:896 ] = 64'h0000000000000000;
	msg_in[895:832 ] = 64'h0000000000000000;
	msg_in[831:768 ] = 64'h0000000000000000;
	msg_in[767:704 ] = 64'h0000000000000000;
	msg_in[703:640 ] = 64'h0000000000000000;
	msg_in[639:576 ] = 64'h0000000000000000;
	msg_in[575:512 ] = 64'h0000000000000000;
	msg_in[511:448 ] = 64'h0000000000000000;
	msg_in[447:384 ] = 64'h0000000000000000;
	msg_in[383:320 ] = 64'h0000000000000000;
	msg_in[319:256 ] = 64'h0000000000000000;
	msg_in[255:192 ] = 64'h0000000000000000;
	msg_in[191:128 ] = 64'h0000000000000000;
	msg_in[127:64  ] = 64'h0000000000000000;
	msg_in[63:0    ] = 64'h0000000000000018;
	new_block = 1'b1;	

	hash_golden = 512'hddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f;

	test = 8'd4;
	hash_size = 2'b11;

	$display ("\n***********Testing SHA-512: 24-bit message*********");
	$display ("Message: abc");
	$display ("Golden hash: %512h",hash_golden); 			


	#100
	rst =  1'b0;
	//start = 1'b1;
	//input_valid=1'b1;
	#100
	rst = 1'b1;
	//start = 1'b0;
	//input_valid=1'b0;

//****************** SHA-256: Message = 'abc' ************************
	#20000
	count = 5'b0;
	cnt_in = 10'b0;
	msg_in[511:448] = 64'h6162638000000000;
	msg_in[447:384] = 64'h0000000000000000;
	msg_in[383:320] = 64'h0000000000000000;
	msg_in[319:256] = 64'h0000000000000000;
	msg_in[255:192] = 64'h0000000000000000;
	msg_in[191:128] = 64'h0000000000000000;
	msg_in[127:64 ] = 64'h0000000000000000;
	msg_in[63:0   ] = 64'h0000000000000018;
	new_block = 1'b1;
	
	msg_in[1023:512] = 512'h0;

	hash_golden = 512'h0000000000000000000000000000000000000000000000000000000000000000ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad;

	test = 8'd1;
	hash_size = 2'd01;

	$display ("\n\n***********Testing SHA-256: 24-bit message*********");
	$display ("Message: abc");
	$display ("Golden hash: %512h",hash_golden); 	


	#100
	rst =  1'b0;
	//start = 1'b1;
	//input_valid=1'b1;
	#100
	rst = 1'b1;
	//start = 1'b0;
	//input_valid=1'b0;

//****************** SHA-256: Message = 'abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq'************************	
	#20000
	count = 5'b0;
	cnt_in = 10'b0;
	msg_in[511:448] = 64'h6162636462636465;
	msg_in[447:384] = 64'h6364656664656667;
	msg_in[383:320] = 64'h6566676866676869;
	msg_in[319:256] = 64'h6768696a68696a6b;
	msg_in[255:192] = 64'h696a6b6c6a6b6c6d;
	msg_in[191:128] = 64'h6b6c6d6e6c6d6e6f;
	msg_in[127:64 ] = 64'h6d6e6f706e6f7071;
	msg_in[63:0   ] = 64'h8000000000000000;
	new_block = 1'b1;

	msg_in[1023:512] = 512'h0;

	hash_golden = 512'h000000000000000000000000000000000000000000000000000000000000000085e655d6417a17953363376a624cde5c76e09589cac5f811cc4b32c1f20e533a;

	test = 8'd2;
	hash_size = 2'd01;

	$display ("\n\n***********Testing SHA-256: 448-bit message*********");
	$display ("Message: abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
	$display ("Golden hash: %512h",hash_golden); 		

	#100
	//start = 1'b1;
	//input_valid=1'b1;
	#100
	//start = 1'b0;
	//input_valid=1'b0;

	#20000
	count = 5'b0;
	cnt_in = 10'b0;
	new_block = 1'b0;
	msg_in[511:448] = 64'h0000000000000000;
	msg_in[447:384] = 64'h0000000000000000;
	msg_in[383:320] = 64'h0000000000000000;
	msg_in[319:256] = 64'h0000000000000000;
	msg_in[255:192] = 64'h0000000000000000;
	msg_in[191:128] = 64'h0000000000000000;
	msg_in[127:64 ] = 64'h0000000000000000;
	msg_in[63:0   ] = 64'h00000000000001c0;

	msg_in[1023:512] = 512'h0;

	hash_golden = 512'h0000000000000000000000000000000000000000000000000000000000000000248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1;

	test = 8'd3;
	hash_size = 2'd01;

	$display ("\n***********Testing SHA-256: 448-bit message (Continued)*********");
	$display ("Message: abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
	$display ("Golden hash: %512h",hash_golden); 		

	#100
	//start = 1'b0;
	//input_valid=1'b1;
	#100
	//start = 1'b0;
	//input_valid=1'b0;

//************************* Finish ****************************//

	#200000
	$finish;    

end

always
#50 clk = ~clk;

assign win = (hash_size ==2'b01) ? msg_in [511:480] : msg_in [1023:992];

always @(posedge clk)
begin

   if(rst==1'b1)
   begin
       cnt_in <= cnt_in + 1'b1;

      if (cnt_in ==10'd5)  input_valid <= 1'b1;

      if (cnt_in==10'd1023) cnt_in <= cnt_in;

      if (input_valid == 1'b1)
	    begin
	      count <= count + 1'b1;
	  
        if (hash_size == 2'b01)
        begin

          msg_in [511:32] <= msg_in[479:0];
          msg_in [31:0] <= msg_in[511:480];

            if (count==5'd14)
              begin
              last <= 1'b1;
              if (new_block==1'b1) start <= 1'b1;
              end

            else 
              begin
                last <= 1'b0;
                start <= 1'b0;

		        if (count == 5'd15) input_valid <= 1'b0;
	      end
	      end

	  else 
	    begin
	
        msg_in [1023:32] <= msg_in[991:0];
        msg_in [31:0] <= msg_in[1023:992];

          if (count==5'd30)
            begin
              last <= 1'b1;
            if (new_block==1'b1) start <= 1'b1;
            end

        else 
          begin
            last <= 1'b0;
            start <= 1'b0;

        if (count == 5'd31) input_valid <= 1'b0;
      end
      
	  end

	end


if(output_valid==1'b1) begin

		$display ("Output hash: %512h",hout);
		if(hash_golden==hout) $display ("\t\t\t***PASS***\n\n"); 		 
		else $display ("\t\t\t***FAIL***\n\n");

	end

  end	

end


      

sha_unit sha_inst (
        .clk(clk),
        .rst(rst),
	.last(last),
        .start(start),
	.input_valid(input_valid),
        .win(win),
	.hash_size(hash_size),
        .output_valid(output_valid),
        .hout(hout)
        );

endmodule
