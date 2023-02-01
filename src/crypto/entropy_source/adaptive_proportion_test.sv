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

// entropy_source
//
// This module implements an entropy source for a true random number generator
// based on data collected from a temperature sensor.  The temperature sensor
// is implemented in a hardened ADC block in the Max 10 family.
//
// Samples are generated continuously from the ADC at a rate of approximately
// 50,000 per second.  The lsbs of each 12 bit sample (normally 4 lsbs, although
// this can be controlled with a parameter) are concatenated into 32 bit words and
// stored in a local FIFO.  An AVMM interface allows these samples to be read out
// for use as an entropy source in a TRNG (they must be passed through a suitable
// entropy extractor).  If the FIFO is empty, waitrequest will be asserted on the
// AVMM interface until a new 32-bit sample is available.


`timescale 1 ps / 1 ps
`default_nettype none

module adaptive_proportion_test (
    input wire          clk,
    input wire          rst_n,
    input wire          entropy_valid,
    input wire [31:0]   entropy_word,
    output logic        test_fail

);

localparam [10:0] CUTOFF_VALUE_H = 11'd840;     // Based on min-entropy of 0.4, the recommended cutoff value is 840 per NIST SP800-90B
localparam [10:0] CUTOFF_VALUE_L = 11'd184;     // 1024-840

logic [4:0]     word_cnt;           // window size = 1024, the required number of word = 32
logic [4:0]     bit_cnt;            // word size = 32
logic [10:0]    total_cnt;          // max total occurence count is 1024
logic           sample_bit;         // stores the sample bit value for each proportion window
logic [31:0]    entropy_word_reg;   // captures the current entropy word (entropy_word input is not static)
logic           bit_cnt_en;         // bit that control when to count each entropy word's bit
logic           total_cnt_done;     // pulse control bit - end of the window

always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        sample_bit          <= 1'b0;
        bit_cnt_en          <= 1'b0;
        bit_cnt             <= 5'd0;
        word_cnt            <= 5'd0;
        entropy_word_reg    <= 32'd0;
        total_cnt_done      <= 1'b0;
    end
    else begin
        // latch the sample bit value for each new proportion test window
        if ((word_cnt == 5'd0) && (bit_cnt == 5'd0) && entropy_valid) begin
            sample_bit  <= entropy_word[0];
        end
       
        if (entropy_valid) begin
            entropy_word_reg <= entropy_word;
        end

        if (entropy_valid) begin
            bit_cnt_en  <= 1'b1;
        end
        else if (bit_cnt == 5'd31) begin
            bit_cnt_en  <= 1'b0;
        end

        if (bit_cnt_en) begin
            bit_cnt <= bit_cnt + 5'b1;
        end

        if (bit_cnt == 5'd31) begin
            word_cnt <= word_cnt + 5'b1;
        end

        total_cnt_done <= (word_cnt == 5'd31) & (bit_cnt == 5'd31);
    end
end

always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        total_cnt   <= 11'd0;
        test_fail   <= 1'b0;
    end
    else begin
        if ((word_cnt == 5'd0) && (bit_cnt == 5'd0) && entropy_valid) begin
            total_cnt <= 11'd0;
        end
        else if (bit_cnt_en && (sample_bit == entropy_word_reg[bit_cnt]))begin
            total_cnt <= total_cnt + 11'b1;
        end
        
        // capture error once total count equal cutoff value, OR
        // capture error when final total count less than or equal than 184
        // error will stay 1 until block is reset
        if ((total_cnt == CUTOFF_VALUE_H[10:0]) || ((total_cnt <= CUTOFF_VALUE_L[10:0]) && total_cnt_done)) begin
            test_fail <= 1'b1;
        end
    end
end


endmodule
