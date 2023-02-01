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


module sha_scheduler_top (
        clk,
        rst,
	      start,
	      input_valid,
        cnt,
	      hash_size,
        win,
        hash_en,    // indicates whether the SHA engine is busy or not
        w_out
        );

input clk, rst, start;
input input_valid;
input [1:0] hash_size;	
input [6:0] cnt;
input [63:0] win;
input  hash_en;

output [63:0] w_out;

reg [63:0] wreg[15:0];
wire [63:0] msg_out;

wire [63:0] w16,w7;
wire [63:0] w15,w2;

//############### Combinational Logic ######

assign w16 = wreg[15];
assign w15 = wreg[14];
assign w7 = wreg[6];
assign w2 = wreg[1];

assign w_out = wreg[15];

sha_scheduler msg_sch_inst (
	.W_t_2(w2),
	.W_t_7(w7),
	.W_t_15(w15),
	.W_t_16(w16),
	.hash_size(hash_size[1]),
	.W_t(msg_out)
	);

//############### Registers ################

always @(posedge clk or negedge rst)
begin

        if(rst==1'b0)
        begin
        
                wreg[15] <= 64'h0;
                wreg[14] <= 64'h0;
                wreg[13] <= 64'h0;
                wreg[12] <= 64'h0;
                wreg[11] <= 64'h0;
                wreg[10] <= 64'h0;
                wreg[9]  <= 64'h0;
                wreg[8]  <= 64'h0;
                wreg[7]  <= 64'h0;
                wreg[6]  <= 64'h0;
                wreg[5]  <= 64'h0;
                wreg[4]  <= 64'h0;
                wreg[3]  <= 64'h0;
                wreg[2]  <= 64'h0;
                wreg[1]  <= 64'h0;
                wreg[0]  <= 64'h0;
        
        end
        
        else
        begin

                if ((input_valid==1'b1) && (hash_en==1'b0))
                  begin

// Keep shifting the values when either we get a new msg block or subsequent parts of the same msg to be hased. As well as 
// ensuring the SHA Engine is not busy. 

                    if (hash_size== 2'b01)
                      begin
                        
                        wreg[1]  <= wreg[0] ;     
                        wreg[2]  <= wreg[1] ;     
                        wreg[3]  <= wreg[2] ;     
                        wreg[4]  <= wreg[3] ;     
                        wreg[5]  <= wreg[4] ;     
                        wreg[6]  <= wreg[5] ;     
                        wreg[7]  <= wreg[6] ;     
                        wreg[8]  <= wreg[7] ;     
                        wreg[9]  <= wreg[8] ;     
                        wreg[10] <= wreg[9] ;     
                        wreg[11] <= wreg[10];     
                        wreg[12] <= wreg[11];     
                        wreg[13] <= wreg[12];     
                        wreg[14] <= wreg[13];     
                        wreg[15] <= wreg[14]; 

                        wreg[0] <= win;

                      end

                    else
                      begin
                        wreg[0]  [63:32]  <= wreg[0] [31:0];     
                        wreg[1]	[31:0]	<=	wreg[0]	[63:32];
                        wreg[1]	[63:32]	<=	wreg[1]	[31:0];
                        wreg[2]	[31:0]	<=	wreg[1]	[63:32];
                        wreg[2]	[63:32]	<=	wreg[2]	[31:0];
                        wreg[3]	[31:0]	<=	wreg[2]	[63:32];
                        wreg[3]	[63:32]	<=	wreg[3]	[31:0];
                        wreg[4]	[31:0]	<=	wreg[3]	[63:32];
                        wreg[4]	[63:32]	<=	wreg[4]	[31:0];
                        wreg[5]	[31:0]	<=	wreg[4]	[63:32];
                        wreg[5]	[63:32]	<=	wreg[5]	[31:0];
                        wreg[6]	[31:0]	<=	wreg[5]	[63:32];
                        wreg[6]	[63:32]	<=	wreg[6]	[31:0];
                        wreg[7]	[31:0]	<=	wreg[6]	[63:32];
                        wreg[7]	[63:32]	<=	wreg[7]	[31:0];
                        wreg[8]	[31:0]	<=	wreg[7]	[63:32];
                        wreg[8]	[63:32]	<=	wreg[8]	[31:0];
                        wreg[9]	[31:0]	<=	wreg[8]	[63:32];
                        wreg[9]	[63:32]	<=	wreg[9]	[31:0];
                        wreg[10]	[31:0]	<=	wreg[9]	[63:32];
                        wreg[10]	[63:32]	<=	wreg[10]	[31:0];
                        wreg[11]	[31:0]	<=	wreg[10]	[63:32];
                        wreg[11]	[63:32]	<=	wreg[11]	[31:0];
                        wreg[12]	[31:0]	<=	wreg[11]	[63:32];
                        wreg[12]	[63:32]	<=	wreg[12]	[31:0];
                        wreg[13]	[31:0]	<=	wreg[12]	[63:32];
                        wreg[13]	[63:32]	<=	wreg[13]	[31:0];
                        wreg[14]	[31:0]	<=	wreg[13]	[63:32];
                        wreg[14]	[63:32]	<=	wreg[14]	[31:0];
                        wreg[15]	[31:0]	<=	wreg[14]	[63:32];
                        wreg[15]	[63:32]	<=	wreg[15]	[31:0];


                        wreg[0] [31:0] <= win [31:0];


                  end
                end

                
                else if (hash_en == 1'b1)         // to ensure that the shifting only occurs when the regs are populated.
                  begin
                
                        wreg[1]  <= wreg[0] ;     
                        wreg[2]  <= wreg[1] ;     
                        wreg[3]  <= wreg[2] ;     
                        wreg[4]  <= wreg[3] ;     
                        wreg[5]  <= wreg[4] ;     
                        wreg[6]  <= wreg[5] ;     
                        wreg[7]  <= wreg[6] ;     
                        wreg[8]  <= wreg[7] ;     
                        wreg[9]  <= wreg[8] ;     
                        wreg[10] <= wreg[9] ;     
                        wreg[11] <= wreg[10];     
                        wreg[12] <= wreg[11];     
                        wreg[13] <= wreg[12];     
                        wreg[14] <= wreg[13];     
                        wreg[15] <= wreg[14];     
                        
                        wreg[0]  <= msg_out;
               

                  end                
                
        end

end 

endmodule
