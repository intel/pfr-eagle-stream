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
// altera message_off 10755
module max10_trng_entropy_race #(
   parameter CHAIN_LEN = 17
) (
   input  wire                 clk,
   input  wire                 rst,
   input  wire [CHAIN_LEN-1:0] calibrate_a,
   input  wire [CHAIN_LEN-1:0] calibrate_b,
   output reg                  serial_out,   
   output reg                  serial_out_valid
);

reg [2:0]           flush /* synthesis preserve */;
reg [CHAIN_LEN-1:0] calibrate_a_dec;
reg [CHAIN_LEN-1:0] calibrate_b_dec;
reg                 a_wins;
reg                 b_wins;
reg                 stable;
reg [1:0]           state /* synthesis preserve */;
reg                 valid_i;

wire [CHAIN_LEN:0] chain_a;
wire [CHAIN_LEN:0] chain_b;
wire               a_out;
wire               b_out;
//purposely designed to be latch
wire               a_wins_latch /* synthesis keep */;
wire               b_wins_latch /* synthesis keep */;
wire               trial_complete;
// Insert extra cell to get rid of the hold time violation
// from flush[*] registers to *_wins registers
wire               flush2 /* synthesis keep */;
wire               flush1 /* synthesis keep */;
wire               flush0 /* synthesis keep */;

///////////////////////
// Path A
///////////////////////
//always @(posedge clk) begin
//   if (flush0) calibrate_a_dec <= {CHAIN_LEN{1'b0}};
//   else calibrate_a_dec <= calibrate_a;
//end

generate
    genvar i;
    for (i=0; i<CHAIN_LEN; i=i+1)
        begin : gen_calibrate_a_dec
            dffeas u_dffeas (
                .d      (calibrate_a[i]),
                .clk    (clk),
                .clrn   (1'b1),
                .prn    (1'b1),
                .ena    (1'b1),
                .asdata (1'b0),
                .aload  (1'b0),
                .sclr   (flush0),
                .sload  (1'b0),
                .q      (calibrate_a_dec[i])
            );
        end
endgenerate


assign chain_a = {CHAIN_LEN{!flush0}} + calibrate_a_dec;
assign a_out = chain_a[CHAIN_LEN];

///////////////////////
// Path B
///////////////////////
//always @(posedge clk) begin
//   if (flush1) calibrate_b_dec <= {CHAIN_LEN{1'b0}};
//   else calibrate_b_dec <= calibrate_b;
//end

generate
    genvar j;
    for (j=0; j<CHAIN_LEN; j=j+1)
        begin : gen_calibrate_b_dec
            dffeas u_dffeas (
                .d      (calibrate_b[j]),
                .clk    (clk),
                .clrn   (1'b1),
                .prn    (1'b1),
                .ena    (1'b1),
                .asdata (1'b0),
                .aload  (1'b0),
                .sclr   (flush1),
                .sload  (1'b0),
                .q      (calibrate_b_dec[j])
            );
        end
endgenerate

assign chain_b = {CHAIN_LEN{!flush1}} + calibrate_b_dec;
assign b_out = chain_b[CHAIN_LEN];

///////////////////////
// Finish line latches
///////////////////////
//assign a_wins_latch = !flush1 & (a_wins_latch | (chain_a[CHAIN_LEN-1] & !chain_b[CHAIN_LEN-1]));
//assign b_wins_latch = !flush0 & (b_wins_latch | (chain_b[CHAIN_LEN-1] & !chain_a[CHAIN_LEN-1]));
//this will create a combinational loop (a_wins_latch and b_wins_latch) but it's safe to waive 
assign a_wins_latch = !flush1 & (a_wins_latch | (chain_a[CHAIN_LEN] & !chain_b[CHAIN_LEN]));
assign b_wins_latch = !flush0 & (b_wins_latch | (chain_b[CHAIN_LEN] & !chain_a[CHAIN_LEN]));
assign trial_complete = !flush2 & a_out & b_out;
   
///////////////////////
// Resynchronize
///////////////////////
always @(posedge clk) begin
   if (flush1) a_wins <= 1'b0;
   else a_wins <= a_wins_latch;
end

always @(posedge clk) begin
   if (flush0) b_wins <= 1'b0;
   else b_wins <= b_wins_latch;
end

always @(posedge clk) begin
   if (rst | flush2) stable <= 1'b0;
   else stable <= trial_complete;
end

////////////////////////////////////
// Control - continuous retrigger
////////////////////////////////////
always @(posedge clk) begin
   if (rst) begin
      flush <= 3'b111;
      state <= 2'b00;
      valid_i <= 1'b0;
      serial_out <= 1'b0;
      serial_out_valid <= 1'b0;
   end else begin
      case (state)
         2'b00 : begin
            flush <= 3'b111;
            valid_i <= 1'b0;
            if (!a_wins & !b_wins & !stable) state <= 2'b01;
//			   else state <= 2'b00;
         end
         2'b01 : begin
            flush <= 3'b000;
            if (stable) state <= 2'b10;
//				else state <= 2'b01;
         end
         2'b10 : begin
            valid_i <= 1'b1;
            state <= 2'b00;
         end
         2'b11 : begin
            // unreachable
            state <= 2'b00;
         end
      endcase

      serial_out <= a_wins ^ b_wins;
      serial_out_valid <= valid_i;
   end
end

assign flush2 = flush[2];
assign flush1 = flush[1];
assign flush0 = flush[0];

endmodule
