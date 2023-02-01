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
module hin_init(
        clk,
        rst,
        start,
        cnt,
	hash_size,
        hin_init_a_new,
        hin_init_e_new,
        hin_init_a,
        hin_init_e,
        h_init
        );

input clk, rst, start;
input [1:0] hash_size;
input [6:0] cnt;
input [63:0] hin_init_a_new, hin_init_e_new;

output [63:0] hin_init_a, hin_init_e;
output [511:0] h_init;

wire [63:0] A_in,B_in,C_in,D_in,E_in,F_in,G_in,H_in;
wire [63:0] hin_init_a, hin_init_e;

reg [63:0] A;
reg [63:0] B;
reg [63:0] C;
reg [63:0] D;
reg [63:0] E;
reg [63:0] F;
reg [63:0] G;
reg [63:0] H;

assign hin_init_a = D;
assign hin_init_e = H;

assign A_in = hin_init_a_new; 
assign B_in = A;
assign C_in = B;
assign D_in = C;
assign E_in = hin_init_e_new;
assign F_in = E;
assign G_in = F;
assign H_in = G;

assign h_init = {A,B,C,D,E,F,G,H};

always @(posedge clk or negedge rst)
begin

        if(rst==1'b0)
        begin
        
            A  <= 64'h0; 
            B  <= 64'h0;
            C  <= 64'h0;
            D  <= 64'h0;
            E  <= 64'h0;
            F  <= 64'h0;
            G  <= 64'h0;
            H  <= 64'h0;
              
        end
        
        else
        begin

                if (start == 1'b1)
                begin

			                  A  <= (hash_size == 2'b01) ? {32'h0,32'h6A09E667} : ((hash_size == 2'b10) ? 64'hcbbb9d5dc1059ed8 : 64'h6a09e667f3bcc908); 
                        B  <= (hash_size == 2'b01) ? {32'h0,32'hBB67AE85} : ((hash_size == 2'b10) ? 64'h629a292a367cd507 : 64'hbb67ae8584caa73b);
                        C  <= (hash_size == 2'b01) ? {32'h0,32'h3C6EF372} : ((hash_size == 2'b10) ? 64'h9159015a3070dd17 : 64'h3c6ef372fe94f82b);
                        D  <= (hash_size == 2'b01) ? {32'h0,32'hA54FF53A} : ((hash_size == 2'b10) ? 64'h152fecd8f70e5939 : 64'ha54ff53a5f1d36f1);
                        E  <= (hash_size == 2'b01) ? {32'h0,32'h510E527F} : ((hash_size == 2'b10) ? 64'h67332667ffc00b31 : 64'h510e527fade682d1);
                        F  <= (hash_size == 2'b01) ? {32'h0,32'h9B05688C} : ((hash_size == 2'b10) ? 64'h8eb44a8768581511 : 64'h9b05688c2b3e6c1f);
                        G  <= (hash_size == 2'b01) ? {32'h0,32'h1F83D9AB} : ((hash_size == 2'b10) ? 64'hdb0c2e0d64f98fa7 : 64'h1f83d9abfb41bd6b);
                        H  <= (hash_size == 2'b01) ? {32'h0,32'h5BE0CD19} : ((hash_size == 2'b10) ? 64'h47b5481dbefa4fa4 : 64'h5be0cd19137e2179);


                end
        
                else if(hash_size[1]==1'b0 && cnt>=7'd60)
                begin
                
                        A <= hin_init_a_new; 
                        B <= A; 
                        C <= B; 
                        D <= C; 
                        
                        E <= hin_init_e_new; 
                        F <= E; 
                        G <= F; 
                        H <= G; 
                
                end
                
		else if(hash_size[1]==1'b1 && cnt>=7'd76)
                begin

                        A <= hin_init_a_new; 
                        B <= A; 
                        C <= B; 
                        D <= C; 
                        
                        E <= hin_init_e_new; 
                        F <= E; 
                        G <= F; 
                        H <= G; 

		end

        end

end

endmodule

        
