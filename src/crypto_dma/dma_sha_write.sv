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

// This module implements write request to Crypto IP block. It write all the
// message data to be hashed as well as padding block to the Crypto IP.

`timescale 1 ps / 1 ps
`default_nettype none

module dma_sha_write #(
    parameter LENGTH_WIDTH          = 28
) (
    input wire                      clk,                // clock source (sys_clk domain which similar to clock used by Crypto IP)
    input wire                      areset,             // reset source synchronous to clk

    output logic [33:0]             wr_data,            // 31:0 is sha data payload, 32 is sha first control bit, 33 is sha last control bit
    output logic                    wr_data_valid,      // when asserted it indicate wr_data is valid
    input wire                      sha_done,           // sha done indication from Crypto IP
    input wire                      dma_ufm_sel,        // GPO bit to select dma to ufm path or dma to spi path
    
    input wire                      spi_read_done,      // synchronized version of read done from spi_dma_read, used to determine when to send padding block
    input wire                      ufm_read_done,      // read done from ufm_dma_read, used to determine when to send padding block
    output logic                    write_done,         // asserted when sending padding block completes
    
    input wire                      spi_fifo_rdempty,       // empty indicatin from fifo, when ot empty, read it and write to Crypto IP
    output logic                    spi_fifo_rdreq,         // fifo read request
    input wire [31:0]               spi_fifo_q,             // fifo output
    
    input wire                      ufm_fifo_rdempty,       // empty indicatin from fifo, when ot empty, read it and write to Crypto IP
    output logic                    ufm_fifo_rdreq,         // fifo read request
    input wire [31:0]               ufm_fifo_q,             // fifo output

    input wire                      crypto_mode,        // crypto 256 or crypto 384 indication from dma_csr
    input wire [LENGTH_WIDTH-1:0]   transfer_length     // transfer length info from dma_csr

);


logic [4:0]                 sha_word_counter;           // counter to track number of word within each sha block
logic [4:0]                 sha_last_word;              // indicates sha last word 
logic                       sha_first_block;            // determines whether this is first block of sha calculation 
logic                       pending_sha;                // set after issuing the last word and cleared when sha done received
logic [511:0]               sha_padding_block_512;      // fix sha padding block structre for crypto 256
logic [1023:0]              sha_padding_block_1024;     // fix sha padding block structure for crypto 384
logic [31:0]                sha_padding_word;           // sha padding word which is a slice of the whole sha padding block
logic                       write_done_nxt;             // write done
logic                       fifo_rdempty;
logic                       fifo_rdreq;
logic [31:0]                fifo_q;
logic                       read_done;


// FSM state
enum logic [1:0] {
    WRITE_SHA_IDLE,     // initial state
    WRITE_SHA_DATA,     // sending sha data payload state
    WRITE_SHA_PADDING,  // sending sha padding block state
    WRITE_SHA_DONE      // sha done state
} write_sha_state;


assign sha_last_word            = crypto_mode ? 5'd31 : 5'd15;
assign sha_padding_block_512    = {1'b1, {(512-LENGTH_WIDTH-4){1'b0}}, transfer_length, 3'b0};
assign sha_padding_block_1024   = {1'b1, {(1024-LENGTH_WIDTH-4){1'b0}}, transfer_length, 3'b0}; 


assign sha_padding_word[7:0]     = crypto_mode ? sha_padding_block_1024[1023 - 32*sha_word_counter[4:0] -: 8] : sha_padding_block_512[511 - 32*sha_word_counter[4:0] -: 8];
assign sha_padding_word[15:8]    = crypto_mode ? sha_padding_block_1024[1015 - 32*sha_word_counter[4:0] -: 8] : sha_padding_block_512[503 - 32*sha_word_counter[4:0] -: 8];
assign sha_padding_word[23:16]   = crypto_mode ? sha_padding_block_1024[1007 - 32*sha_word_counter[4:0] -: 8] : sha_padding_block_512[495 - 32*sha_word_counter[4:0] -: 8];
assign sha_padding_word[31:24]   = crypto_mode ? sha_padding_block_1024[999 - 32*sha_word_counter[4:0] -: 8] : sha_padding_block_512[487 - 32*sha_word_counter[4:0] -: 8];


assign write_done_nxt   = (write_sha_state == WRITE_SHA_DONE);


assign fifo_rdempty = dma_ufm_sel ? ufm_fifo_rdempty : spi_fifo_rdempty;
assign fifo_q       = dma_ufm_sel ? ufm_fifo_q : spi_fifo_q;

assign ufm_fifo_rdreq = dma_ufm_sel ? fifo_rdreq : 1'b0;
assign spi_fifo_rdreq = dma_ufm_sel ? 1'b0 : fifo_rdreq;

assign read_done    = dma_ufm_sel ? ufm_read_done : spi_read_done;


// combi output derived from FSM state
always_comb begin
    case (write_sha_state)
        WRITE_SHA_IDLE : begin
            fifo_rdreq      = 1'b0;
            wr_data_valid   = 1'b0;
            wr_data         = 32'h0;
        end

        WRITE_SHA_DATA : begin
            if ((fifo_rdempty == 1'b0) && (pending_sha == 1'b0)) begin
                fifo_rdreq      = 1'b1;
                wr_data_valid   = 1'b1;
                
                if (sha_word_counter == sha_last_word) begin
                    wr_data = {1'b1, sha_first_block, fifo_q};
                end else begin
                    wr_data = {1'b0, 1'b0, fifo_q};
                end
            
            end else begin
                fifo_rdreq      = 1'b0;
                wr_data_valid   = 1'b0;
                wr_data         = {1'b0, 1'b0, fifo_q};
            end

        end

        WRITE_SHA_PADDING : begin
            fifo_rdreq      = 1'b0;
            
            if (pending_sha == 1'b0) begin
                wr_data_valid   = 1'b1;
                
                if (sha_word_counter == sha_last_word) begin
                    wr_data = {1'b1, 1'b0, sha_padding_word};
                end else begin
                    wr_data = {1'b0, 1'b0, sha_padding_word};
                end
            
            end else begin
                wr_data_valid   = 1'b0;
                wr_data         = {1'b0, 1'b0, sha_padding_word};
            end


        end

        WRITE_SHA_DONE : begin
            fifo_rdreq = 1'b0;
            wr_data_valid = 1'b0;
            wr_data = 32'h0;
        end

        default : begin
            fifo_rdreq      = 1'b0;
            wr_data_valid   = 1'b0;
            wr_data         = 32'h0;
        end

    endcase
end


always_ff @(posedge clk or posedge areset) begin
    if (areset) begin
        write_done <= 1'b0;
    end
    else begin
        write_done <= write_done_nxt;
    end
end


// FSM and registered output from FSM state
always_ff @(posedge clk or posedge areset) begin
    if (areset) begin
        write_sha_state     <= WRITE_SHA_IDLE;
        sha_word_counter    <= 5'h0;
        pending_sha         <= 1'b0;
        sha_first_block     <= 1'b0;

    end
    else begin

        sha_word_counter    <= sha_word_counter;
        sha_first_block     <= sha_first_block;
        pending_sha         <= (pending_sha && sha_done) ? 1'b0 : pending_sha;
        
        case (write_sha_state)
            
            WRITE_SHA_IDLE : begin
                if (fifo_rdempty == 1'b0) begin
                    write_sha_state <= WRITE_SHA_DATA;
                    sha_first_block <= 1'b1;
                end else begin
                    write_sha_state <= WRITE_SHA_IDLE;
                    sha_first_block <= sha_first_block;
                end
            end

            WRITE_SHA_DATA : begin
                if ((fifo_rdempty == 1'b0) && (pending_sha == 1'b0)) begin
                    
                    if (sha_word_counter == sha_last_word) begin
                        sha_word_counter    <= 5'h0;
                        pending_sha         <= 1'b1;
                        
                        if (sha_first_block) begin
                            sha_first_block <= 1'b0;
                        end
                    
                    end else begin
                        sha_word_counter    <= sha_word_counter + 5'h1;
                    end
                end
                
                if (read_done) begin
                    write_sha_state <= WRITE_SHA_PADDING;
                end else begin
                    write_sha_state <= WRITE_SHA_DATA;
                end

            end

            WRITE_SHA_PADDING : begin
                if (pending_sha == 1'b0) begin
                    
                    if (sha_word_counter == sha_last_word) begin
                        sha_word_counter    <= 5'h0;
                        pending_sha         <= 1'b1;
                        write_sha_state     <= WRITE_SHA_DONE;
                    end else begin
                        sha_word_counter    <= sha_word_counter + 5'h1;
                        write_sha_state     <= WRITE_SHA_PADDING;
                    end

                end
            end
            
            WRITE_SHA_DONE : begin
                if (fifo_rdempty == 1'b0) begin
                    write_sha_state <= WRITE_SHA_DATA;
                    sha_first_block <= 1'b1;
                end else begin
                    write_sha_state <= WRITE_SHA_DONE;
                    sha_first_block <= sha_first_block;
                end
            end

            default : begin
                write_sha_state <= WRITE_SHA_IDLE;
            end

        endcase
    end
end


endmodule
