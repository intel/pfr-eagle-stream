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

module entropy_source #(
    parameter ENTROPY_SIMULATION_MODE = "OFF"
) (
    input  wire                         clk,                // master clock for this block, outputs are synchronous to this clock
    input  wire                         i_resetn,           // master reset, must be de-asserted synchronously with respect to clk

    input  wire                         fifo_sclr,          // used during entropy samples collection for trng certification 
                                                            // this allow us to reset the fifo so that we able to collect several sets of consecutive samples (refer to NIST SP 800-90B section 3.1.1)   

    // status bit for health check circuit, should be wired to a GPI where it can be read by the NIOS
    output logic                        o_health_check_err, // set to 1 if the health check (see NIST SP 800-90B section 4) fails, only cleared by reset
    
    // read-only interface, provides access to a FIFO which stores pre-generated random data
    // The interface uses polling status bit method to determine if data is available in the FIFO
    input  wire                         read_fifo,          // control signal which perform read to FIFO (derived from avmm logic externally)
                                                            // it will increment FIFO's read pointer and FIFO's output value will be updated
    output logic                        fifo_full,          // goes to CSR status bit, indicate FIFO is full of entropy samples (256 of 32-bit entropy samples)
    output logic                        fifo_available,     // goes to CSR status bit, indicate FIFO is having at least one 32-bit entropy samples
    output logic [31:0]                 fifo_q              // direct output from FIFO's q port
);

    logic           entropy_valid;
    logic [31:0]    entropy_word;
    logic           race1_ok;
    logic           race2_ok;


    max10_trng_entropy #(
        .DATA_WIDTH (32),
        .CHAIN_LEN1 (63), // 15,31,47,63,79
        .CHAIN_LEN2 (79),
        .ENTROPY_SIMULATION_MODE (ENTROPY_SIMULATION_MODE)
    ) entropy_source_inst (
        .clk                    (clk),
        .rst                    (~i_resetn),
        .entropy_valid          (entropy_valid),
        .entropy                (entropy_word),
        .entropy_sel_test_en    (1'b0),
        .entropy_sel_test       (1'b0),
        .race1_ok               (race1_ok),
        .race2_ok               (race2_ok)

    );

    adaptive_proportion_test adaptive_proportion_test_inst (
        .clk            (clk),
        .rst_n          (i_resetn),
        .entropy_valid  (entropy_valid),
        .entropy_word   (entropy_word),
        .test_fail      (o_health_check_err)

    );

    
    ///////////////////////////////////////
    // FIFO to create a backlog of entropy samples
    ///////////////////////////////////////
    
    // signals to connect to the FIFO
    logic                       fifo_rdreq;
    logic                       fifo_wrreq;
    logic                       fifo_empty;

    assign fifo_available   = ~fifo_empty;
    assign fifo_wrreq       = entropy_valid && ~fifo_full;
    assign fifo_rdreq       = read_fifo & ~fifo_empty;     // read a word out of the FIFO each time an AVMM read occurs
    
    // Instantiate the FIFO
    scfifo #(
        .add_ram_output_register    ( "ON"                  ),  // register the output to make meeting timing on AVMM interface logic easier
        .lpm_hint                   ( "RAM_BLOCK_TYPE=M9K"  ),  // this FIFO is sized to fit into a single M9K block
        .lpm_numwords               ( 256                   ),
        .lpm_showahead              ( "ON"                  ),  // read data reaches q port as soon as it is available
        .lpm_type                   ( "scfifo"              ),
        .lpm_width                  ( 32                    ),
        .lpm_widthu                 ( 8                     ),
        .overflow_checking          ( "OFF"                 ),  // external logic will ensure we never write into a full FIFO, so no need for internal overflow checking
        .underflow_checking         ( "OFF"                 ),  // external logic will ensure we never read from an empty FIFO, so no need for internal underflow checking
        .use_eab                    ( "ON"                  )
    )entropy_fifo (
        .aclr                       ( ~i_resetn             ),
        .clock                      ( clk                   ),
        .data                       ( entropy_word          ),
        .rdreq                      ( fifo_rdreq            ),
        .wrreq                      ( fifo_wrreq            ),
        .empty                      ( fifo_empty            ),
        .full                       ( fifo_full             ),
        .q                          ( fifo_q                ),
        .almost_empty               (                       ),
        .almost_full                (                       ),
        .eccstatus                  (                       ),
        .sclr                       ( fifo_sclr             ),
        .usedw                      (                       )
    );
    
    
            
endmodule


    
