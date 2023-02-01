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

module multr_all_384x384(cout, out, done, mode, in1, in2, inp, cin, clk, resetn, sw_reset, start);
input [383:0]   in1, in2, inp;
input [1:0]     mode;
input           cin;
input           clk, resetn, sw_reset, start;
output [383:0]  out;
output          cout;
output reg      done;

reg  [767:0]  out_m;
wire [383:0]  in_t1;
wire [392:0]  out_w;
reg  [392:0]  out_w_out;
wire [7:0]    in_t21, in_t22, in_t23;
wire [8:0]    in_t2;
reg           mult_in_progress, active_acc, active_red_mult, active_red_sub;
wire          next_mult, mult_done, red_done;

wire [4:0]    raddr_in_t2; 
reg  [1:0]    in_t2_sel;
reg  [9:0]    rflag;
reg  [1:0]    aflag;
reg  [4:0]    mflag;
reg  [3:0]    count;
reg c1;
wire c2;
wire next_t2_mem;

wire [392:0]  as_out; 
wire [392:0]  as_in1, as_in2;
wire c_in;
wire c_out;
assign out = out_m[767:384];

crypto_mem3 i_cm31(.clock(clk), .data(in2[127:0]  ), .rdaddress(raddr_in_t2), .wraddress(1'b0), .wren(start), .q(in_t21));
crypto_mem3 i_cm32(.clock(clk), .data(in2[255:128]), .rdaddress(raddr_in_t2), .wraddress(1'b0), .wren(start), .q(in_t22));
crypto_mem3 i_cm33(.clock(clk), .data(in2[383:256]), .rdaddress(raddr_in_t2), .wraddress(1'b0), .wren(start), .q(in_t23));

assign raddr_in_t2 = {1'b0,count[3:0]};
assign in_t2 = (in_t2_sel == 2'b00) ? {1'b0,in_t21} : (in_t2_sel == 2'b01) ? {1'b0,in_t22} : (in_t2_sel == 2'b10) ? {1'b0,in_t23} : {1'b0,out_m[767:760]};
assign in_t1 = (mult_in_progress) ? in1 : inp;

mult_384x9 i_m384_9(.out(out_w), .in1(in_t1), .in2(in_t2));

always @(posedge clk) begin
 out_w_out[383:0] <= (start & ~(mode[1]&mode[0]))? in2 : out_w[383:0];
 out_w_out[392:384] <= (start & ~mode[0])? 9'd0 : (start & ~mode[1]& mode[0])? 9'h1ff : out_w[392:384];
 out_m <= (start) ? 768'd0 : (active_acc)? {as_out[391:0],out_m[383:8]} : (active_red_sub  | (rflag[8] & c2) | aflag[0] | (aflag[1] & ((~mode[1] & mode[0] & ~c1) | (mode[1] & ~mode[0] &(c1|c2)))))? {as_out[383:0],out_m[375:0],8'd0} : out_m;
 c1 <= (aflag[0]) ? as_out[384] : c1;
end

assign c2 = as_out[384];
assign as_in1 = (active_acc | aflag[1] | rflag[8])? {9'd0,out_m[767:384]} : (active_red_sub)? {1'b0,out_m[767:376]} : {9'd0,in1};  
assign as_in2 = (active_red_sub | (aflag[0] & ~mode[1]&mode[0]))? ~out_w_out[392:0] : ((aflag[1] & (mode[1]&~mode[0])) | rflag[8]) ? {9'd0,~inp} : (aflag[1] & (~mode[1]&mode[0])) ? {9'd0,inp} : out_w_out[392:0]; 
assign c_in = (mode == 2'd0) ? cin : ((aflag[0] & ~mode[1]&mode[0]) | (aflag[1] & mode[1]&~mode[0]) | active_red_sub | rflag[8]) ? 1'b1 : 1'b0;
assign cout = c1;

csl_add_sub_393 i_cs411(.c_out(c_out), .out(as_out), .in1(as_in1), .in2(as_in2), .c_in(c_in));

always @(posedge clk) begin
 if(mflag[0] | rflag[0]) count <= 4'd0;
 else count <= count + 1'd1;
end

assign next_mult = (count == 4'hf) ? 1'b1 : 1'b0;   //short counter is used to generate in_t2 from three 128-bit memory
assign next_t2_mem = (count == 4'h1) ? 1'b1 : 1'b0;
assign mult_done = next_t2_mem & mflag[4];
assign red_done = next_mult & rflag[6];

always @(posedge clk or negedge resetn) begin
 if(~resetn) begin
  rflag[9:0] <= 10'd0;
  mflag[4:0] <= 5'd0;
  aflag[1:0] <= 2'd0;
  mult_in_progress <= 1'b0;
  active_acc <= 1'b0;
  active_red_mult <= 1'b0;
  active_red_sub <= 1'b0;
  done <= 1'b0;
 end
 else if(sw_reset) begin
  rflag[9:0] <= 10'd0;
  mflag[4:0] <= 5'd0;
  aflag[1:0] <= 2'd0;
  mult_in_progress <= 1'b0;
  active_acc <= 1'b0;
  active_red_mult <= 1'b0;
  active_red_sub <= 1'b0;
  done <= 1'b0;
 end
 else if(start) begin
  mflag[4:0] <= {4'd0,(mode[1]&mode[0])};
  rflag[9:0] <= 10'd0;
  aflag[1:0] <= {1'd0,~(mode[1]&mode[0])};
  mult_in_progress <= 1'b0;
  active_acc <= 1'b0;
  active_red_mult <= 1'b0;
  active_red_sub <= 1'b0;
  done <= 1'b0;
 end 
 else if(mflag[0]) begin
  mflag[0] <= 1'b0;
  mflag[1] <= 1'b1;
  in_t2_sel <= 2'b00;
  mult_in_progress <= 1'b1;
 end
 else if(mflag[1]) begin
  if(count == 4'h2) active_acc <= 1'b1;
  if(next_mult) begin
   mflag[1] <= 1'b0;
   mflag[2] <= 1'b1;
  end
 end
 else if(mflag[2]) begin
  if(next_t2_mem) in_t2_sel <= 2'b01;
  if(next_mult) begin
   mflag[2] <= 1'b0;
   mflag[3] <= 1'b1;
  end
 end
 else if(mflag[3]) begin
  if(next_t2_mem) in_t2_sel <= 2'b10;
  if(next_mult) begin
   mflag[3] <= 1'b0;
   mflag[4] <= 1'b1;
  end
 end 
 else if(mflag[4]) begin
  if(mult_done) begin
   mflag[4] <= 1'b0;
   rflag[0] <= 1'b1;
   mult_in_progress <= 1'b0;
   in_t2_sel <= 2'b11;  //select out_m as in_t2 during reduction
  end
 end 
 else if(rflag[0]) begin
  active_acc <= 1'b0;
  rflag[0] <= 1'b0;
  rflag[1] <= 1'b1; 
  active_red_mult <= 1'b1;
  active_red_sub <= 1'b0;
 end
 else if(rflag[1]) begin
  active_red_mult <= ~active_red_mult;
  active_red_sub <= ~active_red_sub;
  if(next_mult) begin
   rflag[1] <= 1'b0;
   rflag[2] <= 1'b1;  
  end	
 end
 else if(rflag[2]) begin
  active_red_mult <= ~active_red_mult;
  active_red_sub <= ~active_red_sub;
  if(next_mult) begin
   rflag[2] <= 1'b0;
   rflag[3] <= 1'b1;  
  end  
 end
 else if(rflag[3]) begin
  active_red_mult <= ~active_red_mult;
  active_red_sub <= ~active_red_sub;
  if(next_mult) begin
   rflag[3] <= 1'b0;
   rflag[4] <= 1'b1; 
  end  
 end
 else if(rflag[4]) begin
  active_red_mult <= ~active_red_mult;
  active_red_sub <= ~active_red_sub;
  if(next_mult) begin
   rflag[4] <= 1'b0;
   rflag[5] <= 1'b1; 
  end  
 end
 else if(rflag[5]) begin
  active_red_mult <= ~active_red_mult;
  active_red_sub <= ~active_red_sub;
  if(next_mult) begin
   rflag[5] <= 1'b0;
   rflag[6] <= 1'b1; 
  end  
 end
 else if(rflag[6]) begin
  active_red_mult <= ~active_red_mult;
  active_red_sub <= ~active_red_sub;
  if(red_done) begin
   rflag[6] <= 1'b0;
   rflag[7] <= 1'b1; 
  end  
 end
else if(rflag[7]) begin
  rflag[7] <= 1'b0;
  rflag[8] <= 1'b1;
  active_red_mult <= 1'b0;
  active_red_sub <= 1'b0;
 end
 else if(rflag[8]) begin
  rflag[8] <= 1'b0;
  rflag[9] <= 1'b1;
 end
 else if(rflag[9]) begin
  rflag[9] <= 1'b0;
  done <= 1'b1;
 end
 else if(aflag[0]) begin
  aflag[0] <= 1'b0;
  aflag[1] <= 1'b1;
 end
 else if(aflag[1]) begin
  aflag[1] <= 1'b0;
  done <= 1'b1; 
  end
 else begin
  done <= 1'b0; 
 end
end

endmodule
