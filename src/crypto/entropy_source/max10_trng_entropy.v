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

module max10_trng_entropy #(
   parameter DATA_WIDTH = 32,
   parameter CHAIN_LEN1 = 17,
   parameter CHAIN_LEN2 = 37,
   parameter ENTROPY_SIMULATION_MODE = "OFF"
) (
   input  wire                  clk,
   input  wire                  rst,
   output wire                  entropy_valid,
   output wire [DATA_WIDTH-1:0] entropy,
   input  wire                  entropy_sel_test_en,    // Test port, set to 1 when we need to explicitly select either racing circuit 1 or racing circuit 2 during entropy validation
                                                        // Expect to tie-off to 0 during user mode
   input  wire                  entropy_sel_test,       // Test port, set by test script
   output reg                   race1_ok,
   output reg                   race2_ok
);

localparam OBSERVE_RUNS = 30;
localparam COUNT_BIT    = 28;
   
wire                  serial_1;
wire                  serial_valid_1; 
wire                  serial_2;
wire                  serial_valid_2;  
wire [CHAIN_LEN1-1:0] calibrate_a_1;
wire [CHAIN_LEN1-1:0] calibrate_b_1;
wire                  adjusting_1;
wire [CHAIN_LEN2-1:0] calibrate_a_2;
wire [CHAIN_LEN2-1:0] calibrate_b_2;
wire                  adjusting_2;
wire 	              consecutive_zeros_1;
wire                  consecutive_zeros_2;
wire                  consecutive_ones_1;
wire                  consecutive_ones_2;
wire                  repeated_bits_1;
wire                  repeated_bits_2;
wire                  health_check_done_1;
wire                  health_check_done_2;
wire [DATA_WIDTH-1:0] parallel_1;
wire [DATA_WIDTH-1:0] parallel_2;
wire                  parallel_valid_1;
wire                  parallel_valid_2;
wire                  entropy_sel_final;
   
reg [COUNT_BIT:0]     count;
reg                   race1_ok_i;
reg                   race2_ok_i;
reg                   entropy_sel;

wire                  serial_1_out;
wire                  serial_2_out;

// By default, this circuit can't be verified in simulation because output from
// the racing circuit will always be the same, thus no valid random bit ever
// generated. In order to enable simple simulation run for this circuit, we manipulate the value of serial output
// from racing circuit going into health circuit.
generate
    if (ENTROPY_SIMULATION_MODE == "OFF") begin
        // For synthesis
        assign serial_1_out = serial_1;
        assign serial_2_out = serial_2;

    end
    else if (ENTROPY_SIMULATION_MODE == "RANDOM") begin
        
        reg rand_bit1;
        reg rand_bit2;
        
        always @(posedge clk or posedge rst) begin
            if (rst) begin
                rand_bit1 <= 1'b0;
                rand_bit2 <= 1'b0;
            end
            else begin
                if (serial_valid_1 && serial_valid_2) begin     // serial_valid_1 and serial_valid_2 expected to assert at the same time
                    rand_bit1 <= $urandom_range(0,1);
                    rand_bit2 <= $urandom_range(0,1);
                end
            end
        end

        assign serial_1_out = rand_bit1;
        assign serial_2_out = rand_bit2;

    end
    else begin

        // For simulation

        reg [4:0] sim_counter;

        always @(posedge clk or posedge rst) begin
            if (rst) begin
                sim_counter <= 5'b0;
            end
            else begin
                if (serial_valid_1 && serial_valid_2) begin     // serial_valid_1 and serial_valid_2 expected to assert at the same time
                    sim_counter <= sim_counter + 5'b1;
                end
            end
        end
        
        // Output from race1 will always be 32'h5555AAAA
        assign serial_1_out = sim_counter[4] ^ sim_counter[0];

        // Output from race2 will always be 32'hAAAA5555
        assign serial_2_out = ~sim_counter[4] ^ sim_counter[0];

    end
endgenerate


// *******************************
// Race circuit 1
// *******************************
// Dual carry chain delay lines
max10_trng_entropy_race #(
   .CHAIN_LEN (CHAIN_LEN1)
) u_trng_entropy_race_1 (
   /*I*/.clk              (clk),
   /*I*/.rst              (rst),
   /*I*/.calibrate_a      (calibrate_a_1),
   /*I*/.calibrate_b      (calibrate_b_1),
   /*O*/.serial_out       (serial_1),			
   /*O*/.serial_out_valid (serial_valid_1)
);

// Health check for race circuit 1
max10_trng_entropy_health #(
   .DATA_WIDTH (DATA_WIDTH)
) u_trng_entropy_health_1 (
   /*I*/.clk                (clk),
   /*I*/.rst                (rst),
   /*I*/.serial_in          (serial_1_out),
   /*I*/.serial_in_valid    (serial_valid_1),
   /*O*/.consecutive_zeros  (consecutive_zeros_1),
   /*O*/.consecutive_ones   (consecutive_ones_1),
   /*O*/.repeated_bits      (repeated_bits_1),
   /*O*/.health_check_done  (health_check_done_1),
   /*O*/.parallel_out       (parallel_1),
   /*O*/.parallel_out_valid (parallel_valid_1)			   
);
   
// Control logic to adjust delays for unstable behavior
max10_trng_entropy_adjust #(
   .DATA_WIDTH   (DATA_WIDTH),			     
   .CHAIN_LEN    (CHAIN_LEN1), 
   .OBSERVE_RUNS (OBSERVE_RUNS)
) u_trng_entropy_adjust_1 (
   /*I*/.clk               (clk),
   /*I*/.rst               (rst),			  
   /*I*/.parallel_in_valid (health_check_done_1),	
   /*I*/.consecutive_zeros (consecutive_zeros_1),
   /*I*/.consecutive_ones  (consecutive_ones_1),
   /*I*/.repeated_bits     (repeated_bits_1),		  
   /*O*/.calibrate_a       (calibrate_a_1),
   /*O*/.calibrate_b       (calibrate_b_1),
   /*O*/.adjusting         (adjusting_1)
);
   
// *****************************
// Race circuit 2
// *****************************
// Dual carry chain delay lines
max10_trng_entropy_race #(
   .CHAIN_LEN (CHAIN_LEN2)
) u_trng_entropy_race_2 (
   /*I*/.clk              (clk),
   /*I*/.rst              (rst),
   /*I*/.calibrate_a      (calibrate_a_2),
   /*I*/.calibrate_b      (calibrate_b_2),
   /*O*/.serial_out       (serial_2),			
   /*O*/.serial_out_valid (serial_valid_2)
);

// Health check for race circuit 2
max10_trng_entropy_health #(
   .DATA_WIDTH (DATA_WIDTH)
) u_trng_entropy_health_2 (
   /*I*/.clk                (clk),
   /*I*/.rst                (rst),
   /*I*/.serial_in          (serial_2_out),
   /*I*/.serial_in_valid    (serial_valid_2),
   /*O*/.consecutive_zeros  (consecutive_zeros_2),
   /*O*/.consecutive_ones   (consecutive_ones_2),
   /*O*/.repeated_bits      (repeated_bits_2),
   /*O*/.health_check_done  (health_check_done_2),
   /*O*/.parallel_out       (parallel_2),
   /*O*/.parallel_out_valid (parallel_valid_2)			   
);
   
// Control logic to adjust delays for unstable behavior
max10_trng_entropy_adjust #(
   .DATA_WIDTH   (DATA_WIDTH),			     
   .CHAIN_LEN    (CHAIN_LEN2), 
   .OBSERVE_RUNS (OBSERVE_RUNS)
) u_trng_entropy_adjust_2 (
   /*I*/.clk               (clk),
   /*I*/.rst               (rst),			  
   /*I*/.parallel_in_valid (health_check_done_2),	
   /*I*/.consecutive_zeros (consecutive_zeros_2),
   /*I*/.consecutive_ones  (consecutive_ones_2),
   /*I*/.repeated_bits     (repeated_bits_2),		  
   /*O*/.calibrate_a       (calibrate_a_2),
   /*O*/.calibrate_b       (calibrate_b_2),
   /*O*/.adjusting         (adjusting_2)
);
   
// ********************************
// Entropy output selection circuit
// ********************************
always @(posedge clk) begin
   if (rst) begin
      count <= {COUNT_BIT+1{1'b0}};
      race1_ok_i <= 1'b0;
      race2_ok_i <= 1'b0;
      race1_ok <= 1'b0;
      race2_ok <= 1'b0;
      entropy_sel <= 1'b0;
   end else begin
      if (count[COUNT_BIT]) begin
         count <= {COUNT_BIT+1{1'b0}};
      end else begin
         count <= count + {{COUNT_BIT{1'b0}}, 1'b1};
      end

      if (count[COUNT_BIT]) begin
         race1_ok_i <= 1'b0;
         race2_ok_i <= 1'b0;
         race1_ok <= race1_ok_i;
         race2_ok <= race2_ok_i;
      end else begin
         race1_ok_i <= (parallel_valid_1 & ~adjusting_1) ? 1'b1 : race1_ok_i;
         race2_ok_i <= (parallel_valid_2 & ~adjusting_2) ? 1'b1 : race2_ok_i;	 
      end
	
      if (~entropy_sel) begin
         if ((parallel_valid_1 & ~adjusting_1) | count[COUNT_BIT]) entropy_sel <= race2_ok_i;
      end else begin
         if ((parallel_valid_2 & ~adjusting_2) | count[COUNT_BIT]) entropy_sel <= ~race1_ok_i;
      end
   end
end

assign entropy = entropy_sel_final ? parallel_2 : parallel_1;
assign entropy_valid = entropy_sel_final ? (parallel_valid_2 & ~adjusting_2) : (parallel_valid_1 & ~adjusting_1);
assign entropy_sel_final = entropy_sel_test_en ? entropy_sel_test : entropy_sel; 

endmodule
