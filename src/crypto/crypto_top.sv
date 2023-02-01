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

`timescale 1 ps / 1 ps
`default_nettype none


module crypto_top #(
    parameter ENABLE_CRYPTO_256         = 0,            // 1 = crypto 256 is enabled, 0 = crypto 256 is disabled
    parameter ENABLE_CRYPTO_384         = 1,            // 1 = crypto 384 is enabled, 0 = crypto 384 is disabled
    parameter USE_ECDSA_BLOCK           = 1,            // 1 = ECDSA block is included, 0 = ECDSA block not included (set to 0 to save area for debug purposes only)
    parameter ENTROPY_SIMULATION_MODE   = "OFF"         // set to ON for simulation only, otherwise it MUST leave as OFF for synthesis
)(
    input  wire             clk,                        // system clock, all inputs/outputs are synchronous with this clock
    input  wire             areset,                     // global reset signal, applied asynchronously, must de-assert synchronously with clk
        
    // AVMM Control/Status register interface (see memory map below)
    // readdata has a fixed 2 clock cycle delay
    input  wire  [6:0]      csr_address,            
    input  wire             csr_read,
    input  wire             csr_write,
    output logic [31:0]     csr_readdata,
    output logic            csr_readdatavalid,
    input  wire  [31:0]     csr_writedata,
    
    input wire              dma_nios_master_sel,        // from gpo to select either nios or crypto dma has access to sha data path (1:dma, 0:nios)
    input wire              dma_writedata_valid,        // when asserted dma_writedata is valid
    input wire [33:0]       dma_writedata,              // dma write data bus (31:0 sha message payload, 32: sha start control bit, 33: sha last control bit)
    output logic            sha_done                    // sha done indication to the crypto dma ip
);


    // AVMM Memory Map
    // For any values that span multiple word addresses, the lowest address will always contain the lsbs of the value
    // 
    // Word Address     | R/W | Description
    //----------------------------------------------------------------------
    // 0x00             |  W  | 0     : SHA start (sent to SHA block each time SHA data input reigster is written)
    //                  |  R  | 1     : SHA done current word (Cleared each time SHA Input Valid is asserted)
    //                  |  W  | 2     : SHA last (sent to SHA block each time SHA data input reigster is written)
    //                  |     | 7:3   : Reserved
    //                  |  W  | 8     : ECDSA Start
    //                  |  W  | 9     : ECDSA Instruction Valid
    //                  |     | 11:10 : Reserved
    //                  |  R  | 12    : ECDSA Dout Valid (cleared on start)
    //                  |     | 15:13 : Reserved
    //                  |  W  | 20:16 : ECDSA Instruction
    //                  |     | 23:21 : Reserved
    //                  |  R  | 24    : entropy source health test error (set to 1 on an error, cleared by reset)
    //                  |  R  | 25    : FIFO available, indicate FIFO is having at least one 32-bit entropy samples 
    //                  |  R  | 26    : FIFO full, indicate FIFO is full of entropy samples, 256 of 32-bit entropy samples
    //                  |     | 27    : Reserved
    //                  |  W  | 28    : Crypto Mode. This bit to select which crypto mode to use
    //                  |     |       : 1'b0 (crypto 256), 1'b1 (crypto384)
    //                  |     |       : This field is valid when more than one crypto block is enabled, else it is reserved
    //                  |     | 29    : Reserved
    //                  |  W  | 30    : Entropy Reset
    //                  |  W  | 31    : Crypto Reset
    //----------------------------------------------------------------------
    // 0x10             |  W  | SHA data input
    //                  |     | Writing to this address causes the current value of start and last from the control 
    //                  |     | register (address 0x00 above) to be sent to the SHA block
    //----------------------------------------------------------------------
    // 0x10-0x1b        |  R  | 384 bit SHA data output
    //                  |     | Only addresses 0x00-0x07 (256 bits) are used when CRYPTO_LENGTH = 256
    //----------------------------------------------------------------------
    // 0x20-0x2b        |  W  | 384 bit ECDSA data input
    //                  |     | Only addresses 0x20-0x28 (256 bits) are used when CRYPTO_LENGTH = 256
    //----------------------------------------------------------------------
    // 0x20-0x2b        |  R  | 384 bit ECDSA Cx output
    //                  |     | Only addresses 0x20-0x28 (256 bits) are used when CRYPTO_LENGTH = 256
    //                  |     | For signature verification this value will be 0 to indicate a valid signature
    //                  |     | For signature generation this is the 'r' value
    //                  |     | For public key generation this is the 'Qx' value
    //----------------------------------------------------------------------
    // 0x30-0x3b        |  R  | 384 bit ECDSA Cy output
    //                  |     | Only addresses 0x30-0x38 (256 bits) are used when CRYPTO_LENGTH = 256
    //                  |     | For signature verification this value can be ignored
    //                  |     | For signature generation this is the 's' value
    //                  |     | For public key generation this is the 'Qy' value
    //----------------------------------------------------------------------
    // 0x40             |  R  | 32 bit entropy sample output
    //                  |     | Read access to FIFO which stores entropy samples  
    //                  |     | Only read this when fifo available status bit is set.

    localparam CSR_SHA_DATA_BASE_ADDRESS            = 7'h10;
    localparam CSR_ECDSA_DATA_WRITE_BASE_ADDRESS    = 7'h20;
    localparam CSR_ECDSA_CX_READ_BASE_ADDRESS       = 7'h20;
    localparam CSR_ECDSA_CY_READ_BASE_ADDRESS       = 7'h30;
    localparam CSR_ENTROPY_OUT_READ_BASE_ADDRESS    = 7'h40;

    localparam CSR_CSR_ADDRESS                      = 7'h00;
    localparam CSR_SHA_START_BIT_POS                = 0;
    localparam CSR_SHA_DONE_CURRENT_WORD_BIT_POS    = 1;
    localparam CSR_SHA_LAST_BIT_POS                 = 2;
    localparam CSR_ECDSA_START_BIT_POS              = 8;
    localparam CSR_ECDSA_INSTR_VALID_BIT_POS        = 9;
    localparam CSR_ECDSA_DOUT_VALID_BIT_POS         = 12;
    localparam CSR_ECDSA_INSTR_MSB_BIT_POS          = 20;
    localparam CSR_ECDSA_INSTR_LSB_BIT_POS          = 16;
    localparam CSR_CRYPTO_MODE_BIT_POS              = 28;
    localparam CSR_ENTROPY_RESET_BIT_POS            = 30;
    localparam CSR_CRYPTO_RESET_BIT_POS             = 31;
    
    // 1 = both crypto 256 and crypto 384 is enabled. 0 = Either crypto 256 or crypto 384 is enabled
    localparam MULTI_CRYPTO_MODE = (ENABLE_CRYPTO_256 == 1) && (ENABLE_CRYPTO_384 == 1);   
    
    // when in multi crypto mode, bit width is set to the larger one
    // when in single crypto mode, bit width is set according to selected crypto blocks (256 or 384)
    localparam CRYPTO_LENGTH = (MULTI_CRYPTO_MODE == 1) ? 384 : (ENABLE_CRYPTO_256 == 1) ? 256 : 384;
    
    ///////////////////////////////////////
    // Parameter checking
    //
    // Generate an error if any illegal parameter settings or combinations are used
    ///////////////////////////////////////
    initial /* synthesis enable_verilog_initial_construct */
    begin
        if (USE_ECDSA_BLOCK != 0 && USE_ECDSA_BLOCK != 1) 
            $fatal(1, "Illegal parameterization: expecting USE_ECDSA_BLOCK = 0 or 1");
        if (ENABLE_CRYPTO_256  != 1 && ENABLE_CRYPTO_384 != 1) 
            $fatal(1, "Illegal parameterization: expecting at least ENABLE_CRYPTO_256 = 1 or ENABLE_CRYPTO_384 = 1");
    end
    

    // Reset for the SHA and ECDSA blocks
    logic crypto_reset;
    
    // Reset for entropy block
    logic entropy_reset;

    // avmm read path (read latency of 2)
    logic csr_read_buf;

    // control bits to select which hash sizes (to SHA block)
    logic [1:0] hash_size;

    ///////////////////////////////////////////////////////////////////////////////
    // SHA block
    ///////////////////////////////////////////////////////////////////////////////

    // signals connected directly to SHA block ports
    logic                       sha_output_valid;           // connected to output_valid port on SHA block, pulses high for one clock cycle each time a new output word is available   
    logic                       sha_input_valid;            // assert to write a new 32-bit word into the SHA block
    logic                       sha_input_last;             // assert coincident with sha_input_valid on the last 32-bit word of a 512/1024 (depending on CRYPTO_LENGTH) bit block
    logic                       sha_input_start;            // assert coincident with sha_input_valid on the last 32-bit word of the first 512/1024 bit block of a new SHA operation
    logic [31:0]                sha_din_muxout;             // data input to the SHA block
    logic [511:0]               raw_sha_out;                // SHA output data is a 512 bit bus regardless of crypto length
    logic                       sha_input_valid_muxout;     // output of dma_nios mux 
    logic                       sha_input_last_muxout;      // output of dma_nios mux
    logic                       sha_input_start_muxout;     // output of dma_nios mux
    
    // signals generated based on SHA outputs
    logic                       sha_done_current_word;      // goes high when sha_output_valid_port asserts, goes low when a new word is written into the SHA block (sha_input_valid is asserted)
    logic [CRYPTO_LENGTH-1:0]   sha_out;                    // SHA output trimmed to appropriate length with correct bits selected from the output bus

    sha_unit u_sha (
        .clk(clk),
        .rst(!crypto_reset),        // NOTE: rst is ACTIVE LOW (despite the name)
        .input_valid(sha_input_valid_muxout),
        .start(sha_input_start_muxout),
        .last(sha_input_last_muxout),
        .win(sha_din_muxout),
        .hash_size(hash_size),
        .output_valid(sha_output_valid),
        .hout(raw_sha_out)
    );

    assign sha_out = raw_sha_out[CRYPTO_LENGTH-1:0];

    // generate sha_done_current_word status bit 
    always_ff @(posedge clk or posedge crypto_reset) begin
        if (crypto_reset) begin
            sha_done_current_word <= '1;
        end else begin
            if (sha_output_valid) begin
                sha_done_current_word <= '1;
            end else if (sha_input_valid_muxout) begin
                sha_done_current_word <= '0;
            end
        end
    end
    
    assign sha_done = sha_done_current_word;

    ///////////////////////////////////////////////////////////////////////////////
    // ECDSA block
    ///////////////////////////////////////////////////////////////////////////////

    // signals connected directly to ECDSA block ports
    
    logic                       ecdsa_block_dout_valid;     // asserts when the ECDSA block has completed it's operation and the output is valid, cleared by (TODO: start?  reset?  either?)
    logic [CRYPTO_LENGTH-1:0]   ecdsa_block_cx;             // cx output, valid when dout_valid is asserted
    logic [CRYPTO_LENGTH-1:0]   ecdsa_block_cy;             // cy output, valid when dout_valid is asserted
    logic [CRYPTO_LENGTH-1:0]   ecdsa_block_din;            // input data to the block
    logic [CRYPTO_LENGTH/8-1:0] ecdsa_block_byteena;        // byte enables for each byte in din
    logic                       ecdsa_block_din_valid;      // assert high to write new data into the block from the din bus
    logic                       ecdsa_block_start;          // assert high after the block has been reset to start a new operation
    logic                       ecdsa_block_instr_valid;    // assert to write a new insruction into the block from the instr bus
    logic [4:0]                 ecdsa_block_instr;          // instruction code

    // signals related to the ECDSA block
    logic                       ecdsa_good;                 // valid when ecdsa_block_dout_valid is asserted, set to 1 if cx output is all 0

    generate
        if (USE_ECDSA_BLOCK == 1) begin
            if (MULTI_CRYPTO_MODE == 1) begin
                // multi crypto mode use-case, instantiate more than one ecdsa block

                logic           ecdsa_256_block_done;           // pulses high for one clock cycle when calculations are complete (unused)
                logic           ecdsa_256_block_busy;           // asserts after reset, cleared (and stays low) when calculations are complete (unused)
                logic           ecdsa_256_block_dout_valid;
                logic [255:0]   ecdsa_256_block_cx;
                logic [255:0]   ecdsa_256_block_cy;
                logic           ecdsa_256_block_din_valid;
                logic           ecdsa_256_block_start;
                logic           ecdsa_256_block_instr_valid;
            
                // Instantiate the 256 bit ECDSA block
                ecdsa256_top u_ecdsa256 (
                    .ecc_done(ecdsa_256_block_done),
                    .ecc_busy(ecdsa_256_block_busy),
                    .dout_valid(ecdsa_256_block_dout_valid),
                    .cx_out(ecdsa_256_block_cx),
                    .cy_out(ecdsa_256_block_cy),
                    .clk(clk),
                    .resetn(!areset),
                    .sw_reset(crypto_reset),
                    .ecc_start(ecdsa_256_block_start),
                    .din_valid(ecdsa_256_block_din_valid),
                    .ins_valid(ecdsa_256_block_instr_valid),
                    .data_in(ecdsa_block_din[255:0]),
                    .byteena_data_in(ecdsa_block_byteena[31:0]),
                    .ecc_ins(ecdsa_block_instr)
                );
                
                logic           ecdsa_384_block_done;           // pulses high for one clock cycle when calculations are complete (unused)
                logic           ecdsa_384_block_busy;           // asserts after reset, cleared (and stays low) when calculations are complete (unused)
                logic           ecdsa_384_block_dout_valid;
                logic [383:0]   ecdsa_384_block_cx;
                logic [383:0]   ecdsa_384_block_cy;
                logic           ecdsa_384_block_din_valid;
                logic           ecdsa_384_block_start;
                logic           ecdsa_384_block_instr_valid;
            
                ecdsa384_top u_ecdsa384 (
                    .ecc_done(ecdsa_384_block_done),
                    .ecc_busy(ecdsa_384_block_busy),
                    .dout_valid(ecdsa_384_block_dout_valid),
                    .cx_out(ecdsa_384_block_cx),
                    .cy_out(ecdsa_384_block_cy),
                    .clk(clk),
                    .resetn(!areset),
                    .sw_reset(crypto_reset),
                    .ecc_start(ecdsa_384_block_start),
                    .din_valid(ecdsa_384_block_din_valid),
                    .ins_valid(ecdsa_384_block_instr_valid),
                    .data_in(ecdsa_block_din[383:0]),
                    .byteena_data_in(ecdsa_block_byteena[47:0]),
                    .ecc_ins(ecdsa_block_instr)
                );
            
                logic   crypto_mode;    // Control bit to select crypto 256 or crypto 384 
            
                always_ff @(posedge clk or posedge areset) begin
                    if (areset) begin
                        crypto_mode <= 1'b0;
                    end
                    else begin
                        if (csr_write && csr_address[6:4] == CSR_CSR_ADDRESS[6:4]) begin
                            crypto_mode <= csr_writedata[CSR_CRYPTO_MODE_BIT_POS];
                        end
                    end
                end
                
                // SHA_HASH_SIZE: 00 (Invalid); 01 (SHA-256); 10 (SHA-384); 11 (SHA-512)
                // in multi crypto mode, hash size of sha block is programmable through register
                assign hash_size = crypto_mode ? 2'b10 : 2'b01;
                
                // muxing for ecdsa output to avmm read path
                assign ecdsa_block_dout_valid   = crypto_mode ? ecdsa_384_block_dout_valid : ecdsa_256_block_dout_valid;
                assign ecdsa_block_cx           = crypto_mode ? ecdsa_384_block_cx : {128'h0, ecdsa_256_block_cx};
                assign ecdsa_block_cy           = crypto_mode ? ecdsa_384_block_cy : {128'h0, ecdsa_256_block_cy};

                // muxing for avmm write path to ecdsa control input
                assign ecdsa_384_block_start        = crypto_mode && ecdsa_block_start;
                assign ecdsa_384_block_din_valid    = crypto_mode && ecdsa_block_din_valid;
                assign ecdsa_384_block_instr_valid  = crypto_mode && ecdsa_block_instr_valid;
                
                assign ecdsa_256_block_start        = ~crypto_mode && ecdsa_block_start;
                assign ecdsa_256_block_din_valid    = ~crypto_mode && ecdsa_block_din_valid;
                assign ecdsa_256_block_instr_valid  = ~crypto_mode && ecdsa_block_instr_valid;
                
            end else if (ENABLE_CRYPTO_256 == 1) begin
                // single crypto mode use-case, instantiate only one ecdsa block
                
                logic   ecdsa_block_done;           // pulses high for one clock cycle when calculations are complete (unused)
                logic   ecdsa_block_busy;           // asserts after reset, cleared (and stays low) when calculations are complete (unused)
                
                // SHA_HASH_SIZE: 00 (Invalid); 01 (SHA-256); 10 (SHA-384); 11 (SHA-512)
                // in single crypto mode, hash size of sha block is fixed
                assign hash_size = 2'b01;
                
                // Instantiate the 256 bit ECDSA block
                ecdsa256_top u_ecdsa (
                    .ecc_done(ecdsa_block_done),
                    .ecc_busy(ecdsa_block_busy),
                    .dout_valid(ecdsa_block_dout_valid),
                    .cx_out(ecdsa_block_cx),
                    .cy_out(ecdsa_block_cy),
                    .clk(clk),
                    .resetn(!areset),
                    .sw_reset(crypto_reset),
                    .ecc_start(ecdsa_block_start),
                    .din_valid(ecdsa_block_din_valid),
                    .ins_valid(ecdsa_block_instr_valid),
                    .data_in(ecdsa_block_din),
                    .byteena_data_in(ecdsa_block_byteena),
                    .ecc_ins(ecdsa_block_instr)
                );
                

            end else begin
                // single crypto mode use-case, instantiate only one ecdsa block
                
                logic   ecdsa_block_done;           // pulses high for one clock cycle when calculations are complete (unused)
                logic   ecdsa_block_busy;           // asserts after reset, cleared (and stays low) when calculations are complete (unused)
                
                // SHA_HASH_SIZE: 00 (Invalid); 01 (SHA-256); 10 (SHA-384); 11 (SHA-512)
                // in single crypto mode, hash size of sha block is fixed
                assign hash_size = 2'b10;
            
                ecdsa384_top u_ecdsa (
                    .ecc_done(ecdsa_block_done),
                    .ecc_busy(ecdsa_block_busy),
                    .dout_valid(ecdsa_block_dout_valid),
                    .cx_out(ecdsa_block_cx),
                    .cy_out(ecdsa_block_cy),
                    .clk(clk),
                    .resetn(!areset),
                    .sw_reset(crypto_reset),
                    .ecc_start(ecdsa_block_start),
                    .din_valid(ecdsa_block_din_valid),
                    .ins_valid(ecdsa_block_instr_valid),
                    .data_in(ecdsa_block_din),
                    .byteena_data_in(ecdsa_block_byteena),
                    .ecc_ins(ecdsa_block_instr)
                );

            end

        end else begin
        
            assign ecdsa_block_dout_valid = '1;
            assign ecdsa_block_cx = '1;
            assign ecdsa_block_cy = '1;
        
        end
    endgenerate
    
    
    ///////////////////////////////////////////////////////////////////////////////
    // Entropy source
    ///////////////////////////////////////////////////////////////////////////////
    
    logic           entropy_health_err;
    logic           read_es_fifo;
    logic           es_fifo_full;
    logic           es_fifo_available;
    logic [31:0]    es_fifo_q;
    
    entropy_source #(
        .ENTROPY_SIMULATION_MODE (ENTROPY_SIMULATION_MODE)   
    ) u_entropy_source ( 
        .clk                        ( clk               ),
        .i_resetn                   ( ~entropy_reset     ),
        .fifo_sclr                  ( 1'b0              ),
        .read_fifo                  ( read_es_fifo      ),
        .fifo_full                  ( es_fifo_full      ),
        .fifo_available             ( es_fifo_available ),
        .fifo_q                     ( es_fifo_q         ),
        .o_health_check_err         ( entropy_health_err )
    );
    


    ///////////////////////////////////////////////////////////////////////////////
    // Avalon-MM CSR
    ///////////////////////////////////////////////////////////////////////////////

    // storage for control register bits for SHA-256
    logic sha_input_start_reg;
    logic sha_input_last_reg;

    // control registers written by the NIOS
    always_ff @(posedge clk or posedge areset) begin
        if (areset) begin

            crypto_reset            <= '1;      // crypto reset signal starts asserted, stays asserted until cleared by NIOS
            entropy_reset           <= '1;      // entropy reset signal starts asserted, stays asserted until cleared by NIOS
            sha_input_start_reg     <= '0;
            sha_input_last_reg      <= '0;
            sha_input_start         <= '0;
            sha_input_last          <= '0;
            sha_input_valid         <= '0;
            ecdsa_block_start       <= '0;
            ecdsa_block_instr_valid <= '0;
            ecdsa_block_instr       <= '0;
            ecdsa_block_din_valid   <= '0;
            ecdsa_block_byteena     <= '0;
            
        end else begin
            
            // control register bits
            if (csr_write && csr_address[6:4] == CSR_CSR_ADDRESS[6:4]) begin
                sha_input_start_reg     <= csr_writedata[CSR_SHA_START_BIT_POS];
                sha_input_last_reg      <= csr_writedata[CSR_SHA_LAST_BIT_POS];
                ecdsa_block_start       <= csr_writedata[CSR_ECDSA_START_BIT_POS];
                ecdsa_block_instr_valid <= csr_writedata[CSR_ECDSA_INSTR_VALID_BIT_POS];
                ecdsa_block_instr       <= csr_writedata[CSR_ECDSA_INSTR_MSB_BIT_POS:CSR_ECDSA_INSTR_LSB_BIT_POS];
                entropy_reset           <= csr_writedata[CSR_ENTROPY_RESET_BIT_POS];
                crypto_reset            <= csr_writedata[CSR_CRYPTO_RESET_BIT_POS];
            end else begin                      // some control register bits are only asserted for a single clock cycle during the write operation, clear them here
                ecdsa_block_start       <= '0;
                ecdsa_block_instr_valid <= '0;
            end

            // SHA data input
            if (csr_write && csr_address[6:4] == CSR_SHA_DATA_BASE_ADDRESS[6:4]) begin
                sha_input_start     <= sha_input_start_reg;
                sha_input_last      <= sha_input_last_reg;
                sha_input_valid     <= '1;
            end else begin
                sha_input_start     <= '0;
                sha_input_last      <= '0;
                sha_input_valid     <= '0;
            end

            // ECDSA byteenables
            for (int i=0; i<(CRYPTO_LENGTH/32); i++) begin
                // use byteen to determine which 32 bit word will actually be written
                ecdsa_block_byteena[4*i +: 4] <= (csr_address[3:0] == i[3:0]) ? 4'b1111 : 4'b0000;      // use csr_address, not csr_address_buf, so that byteena will be co-time with writedata_buf
            end
            
            // ECDSA write enable
            if (csr_write && csr_address[6:4] == CSR_ECDSA_DATA_WRITE_BASE_ADDRESS[6:4]) begin          // use csr_write and csr_address, not _buf versions, so that din_valid will be co-timed with writedata_buf
                ecdsa_block_din_valid <= '1;
            end else begin
                ecdsa_block_din_valid <= '0;
            end
            
        end
            
    end
    
    // delay avmm address used by read logic by 1 clock cycle, to make meeting timing easier and to make sure status bits have time to update after a write
    // delay avmm writedata by 1 clock cycle before feeding into SHA and ECDSA blocks to match timing of control signals generated above
    logic [31:0]    csr_writedata_buf;
    logic [6:0]     csr_address_buf;
    always_ff @(posedge clk) begin
        csr_writedata_buf   <= csr_writedata;
        csr_address_buf     <= csr_address;
        csr_read_buf        <= csr_read;
        csr_readdatavalid   <= csr_read_buf;
        read_es_fifo        <= (csr_read && (csr_address[6:4] == CSR_ENTROPY_OUT_READ_BASE_ADDRESS[6:4]));  // first pipeline of the 2 clock read latency for read entropy sample path
    end

    // data into the ECDSA block, simply recplicate the 32 bit data input (delayed by 1 clock cycle), byteena is used to determine which 32 bit word is actually written
    always_comb begin
        for (int i=0; i<(CRYPTO_LENGTH/32); i++) begin
            ecdsa_block_din[32*i+24 +: 8] = csr_writedata_buf[7:0];
            ecdsa_block_din[32*i+16 +: 8] = csr_writedata_buf[15:8];
            ecdsa_block_din[32*i+8  +: 8] = csr_writedata_buf[23:16];
            ecdsa_block_din[32*i    +: 8] = csr_writedata_buf[31:24];
        end
    end
    
    // data into SHA block, convert to big-endian
    assign sha_din_muxout[24 +: 8] = dma_nios_master_sel ? dma_writedata[7:0] : csr_writedata_buf[7:0];
    assign sha_din_muxout[16 +: 8] = dma_nios_master_sel ? dma_writedata[15:8] : csr_writedata_buf[15:8];
    assign sha_din_muxout[8  +: 8] = dma_nios_master_sel ? dma_writedata[23:16] : csr_writedata_buf[23:16];
    assign sha_din_muxout[0  +: 8] = dma_nios_master_sel ? dma_writedata[31:24] : csr_writedata_buf[31:24];
    
    // control signal into SHA block
    assign sha_input_start_muxout  = dma_nios_master_sel ? dma_writedata[32] : sha_input_start;
    assign sha_input_last_muxout   = dma_nios_master_sel ? dma_writedata[33] : sha_input_last;
    assign sha_input_valid_muxout  = dma_nios_master_sel ? dma_writedata_valid : sha_input_valid;
    
    // status bits read, fixed 2 clock cycle delay (one cycle to register address and read inputs, one to register readdata output)
    logic [31:0]    csr_readdata_buf;
    always_ff @(posedge clk) begin
        if (csr_address_buf[6:4] == CSR_CSR_ADDRESS[6:4]) begin
            csr_readdata_buf[31:27] <= '0;
            csr_readdata_buf[26]    <= es_fifo_full;
            csr_readdata_buf[25]    <= es_fifo_available;
            csr_readdata_buf[24]    <= entropy_health_err;
            csr_readdata_buf[23:13] <= '0;
            csr_readdata_buf[12]    <= ecdsa_block_dout_valid;
            csr_readdata_buf[11:2]  <= '0;
            csr_readdata_buf[1]     <= sha_done_current_word;
            csr_readdata_buf[0]     <= '0;
        end else if (csr_address_buf[6:4] == CSR_SHA_DATA_BASE_ADDRESS[6:4]) begin
            csr_readdata_buf[7:0]   <= sha_out[32*csr_address_buf[3:0]+24 +: 8];
            csr_readdata_buf[15:8]  <= sha_out[32*csr_address_buf[3:0]+16 +: 8];
            csr_readdata_buf[23:16] <= sha_out[32*csr_address_buf[3:0]+8  +: 8];
            csr_readdata_buf[31:24] <= sha_out[32*csr_address_buf[3:0]+0  +: 8];
        end else if (csr_address_buf[6:4] == CSR_ECDSA_CX_READ_BASE_ADDRESS[6:4]) begin
            csr_readdata_buf[7:0]   <= ecdsa_block_cx[32*csr_address_buf[3:0]+24 +: 8];
            csr_readdata_buf[15:8]  <= ecdsa_block_cx[32*csr_address_buf[3:0]+16 +: 8];
            csr_readdata_buf[23:16] <= ecdsa_block_cx[32*csr_address_buf[3:0]+8  +: 8];
            csr_readdata_buf[31:24] <= ecdsa_block_cx[32*csr_address_buf[3:0]+0  +: 8];
        end else if (csr_address_buf[6:4] == CSR_ECDSA_CY_READ_BASE_ADDRESS[6:4]) begin
            csr_readdata_buf[7:0]   <= ecdsa_block_cy[32*csr_address_buf[3:0]+24 +: 8];
            csr_readdata_buf[15:8]  <= ecdsa_block_cy[32*csr_address_buf[3:0]+16 +: 8];
            csr_readdata_buf[23:16] <= ecdsa_block_cy[32*csr_address_buf[3:0]+8  +: 8];
            csr_readdata_buf[31:24] <= ecdsa_block_cy[32*csr_address_buf[3:0]+0  +: 8];
        end else begin
            csr_readdata_buf[31:0]  <= es_fifo_q;
        end
    end
    
    assign csr_readdata = csr_readdata_buf;

endmodule
