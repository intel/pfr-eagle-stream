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

// smbus_filtered_relay
//
// This module implements an SMBus relay between a single master and multiple
// slave devices, with filtering to enable only whitelisted commands to be sent 
// to each slave.
//
// The module uses clock stretching on the interface from the SMBus master
// to allow time for the slave to respond with ACK and read data.
//
// Required files:
//      async_input_filter.sv


`timescale 1 ps / 1 ps
`default_nettype none

module smbus_filtered_relay #(
    parameter FILTER_ENABLE                                     = 1,            // Set to 1 to enable SMBus command filtering, 0 to allow all commands
    parameter CLOCK_PERIOD_PS                                   = 10000,        // period of the input 'clock', in picoseconds (default 10,000 ps = 100 MHz)
    parameter BUS_SPEED_KHZ                                     = 100,          // bus speed of the slowest device on the slave SMBus interface, in kHz (must be 100, 400, or 1000)
    parameter NUM_RELAY_ADDRESSES                               = 1,            // number of addresses to pass through the relay, must be >= 1
    parameter IGNORED_ADDR_BITS                                 = 0,            // number of SMBus Address least significant bits to ignore when determining which relay address is being addressed
    parameter [NUM_RELAY_ADDRESSES:1][6:0] SMBUS_RELAY_ADDRESS  = {(NUM_RELAY_ADDRESSES){7'h0}},
    parameter SCL_LOW_TIMEOUT_PERIOD_MS                         = 36,           // period of the SCL low timeout in ms (need to be multiplied by 1000000000 to convert to ps)default from smbus specs = 35 ms -> 35, 000, 000, 000 ps. Relay adds 1ms of buffer for the default value making the total default parameter value = 36ms. Extra: default for TNP platform is 350ms --> 350,000,000,000 ps = 350 ms. User can set 351ms for this parameter that considers 1ms buffer (up to the user). 
    parameter MCTP_OUT_OF_BAND_ENABLE                           = 0             // Relay MCTP out of band (relay role swap) feature: set 0 to disable (mux_slave_to_master always 0) or set 1 to enable. If enabled, relay will swap master and slave roles if it detects slave start when relay is idle (indicating a slave is initiating to become a master)
) (
    input  wire                            clock,              // master clock for this block
    input  wire                            i_resetn,           // master reset, must be de-asserted synchronously with clock
    input  wire                            i_block_disable,    // when asserted, disables this entire block such that it drives no SMBus signals
    input  wire                            i_filter_disable,   // when asserted, disables command filtering
    input  wire                            i_relay_all_addresses, // when asserted, all smbus addresses is valid 
    
    // Master SMBus interface signals
    // This module acts as a slave on this bus, and accepts commands from an external SMBus Master
    // Each signal on this bus (scl and sda) has an input that comes from the device pin, and an output that turns on a tri-state
    // driver which causes that pin to be driven to 0 by this device.
    input  wire                            ia_master_scl,      // asynchronous input from the SCL pin of the master interface
    output logic                           o_master_scl_oe,    // when asserted, drive the SCL pin of the master interface low
    input  wire                            ia_master_sda,      // asynchronous input from the SDA pin of the master interface
    output logic                           o_master_sda_oe,    // when asserted, drive the SDA pin of the master interface low

    // Slave SMBus interface signals
    // This module acts as a master on this bus, forwarding commands from the SMBus master
    // Each signal on this bus (scl and sda) has an input that comes from the device pin, and an output that turns on a tri-state
    // driver which causes that pin to be driven to 0 by this device.
    input  wire                            ia_slave_scl,       // asynchronous input from the SCL pin of the master interface
    output logic                           o_slave_scl_oe,     // when asserted, drive the SCL pin of the master interface low
    input  wire                            ia_slave_sda,       // asynchronous input from the SDA pin of the master interface
    output logic                           o_slave_sda_oe,     // when asserted, drive the SDA pin of the master interface low    

    // AVMM slave interface to access the command enable memory (note that reads from this memory are not supported)
    input  wire                            i_avmm_write,
    input  wire [7:0]                      i_avmm_address,
    input  wire [31:0]                     i_avmm_writedata
);

    ///////////////////////////////////////
    // Parameter checking
    //
    // Generate an error if any illegal parameter settings or combinations are used
    ///////////////////////////////////////
    initial /* synthesis enable_verilog_initial_construct */
    begin
        if (FILTER_ENABLE != 0 && FILTER_ENABLE != 1) 
            $fatal(1, "Illegal parameterization: expecting FILTER_ENABLE = 0 or 1");
        if (CLOCK_PERIOD_PS < 10000 || CLOCK_PERIOD_PS > 50000)     // This is somewhat arbitrary, assuming a clock between 20 MHz (for minimum 20x over-sampling) and 100 MHz
            $fatal(1, "Illegal parameterization: requre 10000 <= CLOCK_PERIOD_PS <= 50000");
        if (BUS_SPEED_KHZ != 100 && BUS_SPEED_KHZ != 400 && BUS_SPEED_KHZ != 1000) 
            $fatal(1, "Illegal parameterization: expecting BUS_SPEED_KHZ = 100, 400, or 1000");
        if (IGNORED_ADDR_BITS < 0 || IGNORED_ADDR_BITS > 6)
            $fatal(1, "Illegal parameterization: expecting 0 <= IGNORED_ADDR_BITS <= 6");
        if (NUM_RELAY_ADDRESSES <= 0 || NUM_RELAY_ADDRESSES > 32)
            $fatal(1, "Illegal parameterization: require NUM_RELAY_ADDRESSES > 0 and NUM_RELAY_ADDRESSES <= 32");
        if (NUM_RELAY_ADDRESSES <= 32 && NUM_RELAY_ADDRESSES > 23)
            $fatal(1, "Illegal parameterization: NUM_RELAY_ADDRESSES values > 23 and NUM_RELAY_ADDRESSES <= 32 have not been fully tested. Users are recommended to perform additional verification on this block before deploying this feature.");
        if (SCL_LOW_TIMEOUT_PERIOD_MS < 25 || SCL_LOW_TIMEOUT_PERIOD_MS > 351) //In this design, we shall define minimum of SCL low timeout period to be 25ms, and maximum is 351ms (according to TNP pass 350ms timeout + 1ms buffer)
            $fatal(1, "Illegal parameterization: require 25 <= SCL_LOW_TIMEOUT_PERIOD_MS <= 351");
        if (MCTP_OUT_OF_BAND_ENABLE !=0 && MCTP_OUT_OF_BAND_ENABLE != 1)
            $fatal(1, "Illegal parameterization : expecting MCTP_OUT_OF_BAND_ENABLE = 0 or 1");
        for (int i=1; i<=NUM_RELAY_ADDRESSES-1; i++) begin
            for (int j=i+1; j<=NUM_RELAY_ADDRESSES; j++) begin
                if (SMBUS_RELAY_ADDRESS[i][6:IGNORED_ADDR_BITS]==SMBUS_RELAY_ADDRESS[j][6:IGNORED_ADDR_BITS]) 
                    $fatal(1, "Illegal parameterization: all SMBUS_RELAY_ADDRESS entries must be unique (after IGNORED_ADDR_BITS lsbs are ignored)");
            end
        end
    end


    ///////////////////////////////////////
    // Calculate timing parameters
    // Uses timing parameters from the SMBus specification version 3.1
    ///////////////////////////////////////
    
    //convert SCL_LOW_TIMEOUT_PERIOD_MS (ms) to SCL_LOW_TIMEOUT_PERIOD_PS (ps)
    localparam [38:0] SCL_LOW_TIMEOUT_PERIOD_PS = SCL_LOW_TIMEOUT_PERIOD_MS * 1000000000;
    
    // minimum time to hold the SCLK signal low, in clock cycles
    // minimum time to hold SCLK high is less than this, but we will use this same number to simplify logic
    localparam int SCLK_HOLD_MIN_COUNT =
        BUS_SPEED_KHZ == 100 ? ((5000000 + CLOCK_PERIOD_PS - 1) / CLOCK_PERIOD_PS) :
        BUS_SPEED_KHZ == 400 ? ((1300000 + CLOCK_PERIOD_PS - 1) / CLOCK_PERIOD_PS) :
                               (( 500000 + CLOCK_PERIOD_PS - 1) / CLOCK_PERIOD_PS)  ;
    localparam int SCLK_HOLD_COUNTER_BIT_WIDTH = $clog2(SCLK_HOLD_MIN_COUNT)+1;             // counter will count from SCLK_HOLD_MIN_COUNT-1 downto -1

    localparam int NOISE_FILTER_MIN_CLOCK_CYCLES = ((50000 + CLOCK_PERIOD_PS - 1) / CLOCK_PERIOD_PS);           // SMBus spec says to filter out pulses smaller than 50ns
    localparam int NUM_FILTER_REGS = NOISE_FILTER_MIN_CLOCK_CYCLES >= 2 ? NOISE_FILTER_MIN_CLOCK_CYCLES : 2;    // always use at least 2 filter registers, to eliminate single-cycle pulses that might be caused during slow rising/falling edges

    // Read phase ACK arbitration lost use case. 
    // time to detect SCL low timeout, in clock cycles. This will be the same period for any BUS_SPEED_KHZ according to SMBus specs
    localparam [38:0] SCLK_LOW_TIMEOUT_COUNT = ((SCL_LOW_TIMEOUT_PERIOD_PS + CLOCK_PERIOD_PS - 1) / CLOCK_PERIOD_PS);
    localparam int SCLK_LOW_TIMEOUT_COUNTER_BIT_WIDTH = $clog2(SCLK_LOW_TIMEOUT_COUNT)+1;
    
    // start when busy use case. Pull master SDA low for at least 9 SCL
    // time to detect 9 SCL clock cycles based on BUS_SPEED_KHZ. These will be constant values. 
    // 100kHz   --> 10us    : 9 SCL = 9 X 10us  = 90us or 90,000,000ps
    // 400kHz   --> 2.5us   : 9 SCL = 9 X 2.5us = 22.5us or 22,500,000ps
    // 1000kHz  --> 1us    :  9 SCL = 9 X 1us   = 9us or 9,000,000ps
    localparam int NINE_SCLK_COUNT =
        BUS_SPEED_KHZ == 100 ? ((90000000 + CLOCK_PERIOD_PS - 1) / CLOCK_PERIOD_PS) :
        BUS_SPEED_KHZ == 400 ? ((22500000 + CLOCK_PERIOD_PS - 1) / CLOCK_PERIOD_PS) :
                               (( 9000000 + CLOCK_PERIOD_PS - 1) / CLOCK_PERIOD_PS)  ;
    localparam int NINE_SCLK_COUNTER_BIT_WIDTH = $clog2(NINE_SCLK_COUNT)+1;

    //////////////////////////////////////////////////////////
    // Internal wires for SMBus master and slave ports MUX
    //////////////////////////////////////////////////////////
    logic internal_ia_master_scl;      // asynchronous input from the SCL pin of the master interface
    logic internal_o_master_scl_oe;    // when asserted, drive the SCL pin of the master interface low
    logic internal_ia_master_sda;      // asynchronous input from the SDA pin of the master interface
    logic internal_o_master_sda_oe;    // when asserted, drive the SDA pin of the master interface low
    logic internal_ia_slave_scl;       // asynchronous input from the SCL pin of the master interface
    logic internal_o_slave_scl_oe;     // when asserted, drive the SCL pin of the master interface low
    logic internal_ia_slave_sda;       // asynchronous input from the SDA pin of the master interface
    logic internal_o_slave_sda_oe;     // when asserted, drive the SDA pin of the master interface low   
    
    ///////////////////////////////////////
    // Synchronize asynchronous SMBus input signals to clock and detect edges and start/stop conditions
    ///////////////////////////////////////

    // synchronized, metastability hardened versions of the scl and sda input signals for internal use in the rest of this module
    logic master_scl;
    logic master_sda;
    logic master_sda_address;  //used to generate the real address SDA data. (for cases that lose arbitration at address, relay logic will replace ia_master_sda to ia_master_sda_input which pulls SDA line high, so we use this to observe the real ADDRESS that is coming in from SDA to determine to filter or not to filter)
    logic slave_scl;
    logic slave_sda;
    logic common_slave_scl;  //the real common slave bus SCL (for cases where role swapping takes place, where slave interface is now master interface and vice versa. This signal will hold the actual data going on at the slave common bus)
    logic common_slave_sda;  //the real common slave bus SDA (for cases where role swapping takes place, where slave interface is now master interface and vice versa. This signal will hold the actual data going on at the slave common bus)
    
    // detection of positive and negative edges, asserted synchronous with the first '1' or '0' on the master/slave scl signals defined below
    logic master_scl_posedge;
    logic master_scl_negedge;
    logic slave_scl_posedge ;
    logic slave_scl_negedge ;
    logic common_slave_scl_posedge ;
    logic common_slave_scl_negedge ;
    logic master_sda_posedge;
    logic master_sda_negedge;
    logic slave_sda_posedge ;
    logic slave_sda_negedge ;
    logic common_slave_sda_posedge ;
    logic common_slave_sda_negedge ;
    logic master_sda_address_posedge;
    logic master_sda_address_negedge;
    
    // version of ia_master_sda that will be pulled HIGH if it lost arbitration to ensure the master backs off and doesn't interrupt/corrupt the slave SDA line
    logic ia_master_sda_input;
    
    // SDA signals are delayed by 1 extra clock cycle, to provide a small amount of hold timing when sampling SDA on the rising edge of SCL
    async_input_filter #(
        .NUM_METASTABILITY_REGS (2),
        .NUM_FILTER_REGS        (NUM_FILTER_REGS)
    ) sync_master_scl_inst (
        .clock                  ( clock ),
        .ia_async_in            ( internal_ia_master_scl ),
        .o_sync_out             ( master_scl ),
        .o_rising_edge          ( master_scl_posedge ),
        .o_falling_edge         ( master_scl_negedge )
    );

    async_input_filter #(
        .NUM_METASTABILITY_REGS (2),
        .NUM_FILTER_REGS        (NUM_FILTER_REGS+1)
    ) sync_master_sda_inst (
        .clock                  ( clock ),
        .ia_async_in            ( ia_master_sda_input ),  //here we feed the arbitration monitored ia_master_sda instead of the real ia_master_sda 
        .o_sync_out             ( master_sda ),
        .o_rising_edge          ( master_sda_posedge ),
        .o_falling_edge         ( master_sda_negedge )
    );
    
    //this sync is for the master_sda address data. it will always store actual address even if arbitration lost
    async_input_filter #(
        .NUM_METASTABILITY_REGS (2),
        .NUM_FILTER_REGS        (NUM_FILTER_REGS+1)
    ) sync_master_sda_for_addr_inst (
        .clock                  ( clock ),
        .ia_async_in            ( internal_ia_master_sda ),
        .o_sync_out             ( master_sda_address ),
        .o_rising_edge          ( master_sda_address_posedge ),
        .o_falling_edge         ( master_sda_address_negedge )
    );
    
    async_input_filter #(
        .NUM_METASTABILITY_REGS (2),
        .NUM_FILTER_REGS        (NUM_FILTER_REGS)
    ) sync_slave_scl_inst (
        .clock                  ( clock ),
        .ia_async_in            ( internal_ia_slave_scl ),
        .o_sync_out             ( slave_scl ),
        .o_rising_edge          ( slave_scl_posedge ),
        .o_falling_edge         ( slave_scl_negedge )
    );
    
    async_input_filter #(
        .NUM_METASTABILITY_REGS (2),
        .NUM_FILTER_REGS        (NUM_FILTER_REGS+1)
    ) sync_slave_sda_inst (
        .clock                  ( clock ),
        .ia_async_in            ( internal_ia_slave_sda ),
        .o_sync_out             ( slave_sda ),
        .o_rising_edge          ( slave_sda_posedge ),
        .o_falling_edge         ( slave_sda_negedge )
    );
    
    //these 2 syncs are for the common slave bus. 
    async_input_filter #(
        .NUM_METASTABILITY_REGS (2),
        .NUM_FILTER_REGS        (NUM_FILTER_REGS)
    ) common_sync_slave_scl_inst (
        .clock                      ( clock ),
        .ia_async_in                ( ia_slave_scl ),
        .o_sync_out                 ( common_slave_scl ),
        .o_rising_edge              ( common_slave_scl_posedge ),
        .o_falling_edge             ( common_slave_scl_negedge )
    );
    
    async_input_filter #(
        .NUM_METASTABILITY_REGS (2),
        .NUM_FILTER_REGS        (NUM_FILTER_REGS+1)
    ) common_sync_sync_slave_sda_inst (
        .clock                      ( clock ),
        .ia_async_in                ( ia_slave_sda ),
        .o_sync_out                 ( common_slave_sda ),
        .o_rising_edge              ( common_slave_sda_posedge ),
        .o_falling_edge             ( common_slave_sda_negedge )
    );
    
    // detect start and stop conditions on the master bus, asserted 1 clock cycle after the start/stop condition actually occurs
    logic master_start;
    logic master_stop;
    logic master_scl_dly;       // delayed version of master_scl by 1 clock cycle
    
    always_ff @(posedge clock or negedge i_resetn) begin
        if (!i_resetn) begin
            master_start        <= '0;
            master_stop         <= '0;
            master_scl_dly      <= '0;
        end else begin
            master_start        <= master_scl & master_sda_negedge;      // falling edge on master SDA while master SCL high is a start condition
            master_stop         <= master_scl & master_sda_posedge;      // rising edge on master SDA while master SCL high is a stop condition
            master_scl_dly      <= master_scl;
        end
    end
    
    // detect start condition on the slave bus, asserted 1 clock cycle after the start condition actually occurs. 
    logic slave_start;  // slave_start detects start condition on slave bus 
    logic slave_busy;   // slave_busy detects start condition on slave bus and will be latched until the slave completes the transaction with a slave stop
    always_ff @(posedge clock or negedge i_resetn) begin 
        if (!i_resetn) begin
            slave_start     <= '0;
            slave_busy      <= '0;
        end else begin 
            if (common_slave_scl & common_slave_sda_negedge) begin // slave start condition: falling edge on slave SDA while slave SCL high
                slave_start     <= '1; //valid as one clock cycle pulse 
                slave_busy      <= '1;
            end else if (common_slave_scl & common_slave_sda_posedge) begin // slave stop condition: rising edge on slave SDA while slave SCL high is a stop condition
                slave_busy      <= '0;
            end else begin 
                slave_start     <= '0;
            end
        end 
    end 

    ///////////////////////////////////////
    // Track the 'phase' of the master SMBus
    ///////////////////////////////////////
    
    enum {
        SMBMASTER_STATE_IDLE                ,
        SMBMASTER_STATE_START               ,
        SMBMASTER_STATE_MASTER_ADDR         ,
        SMBMASTER_STATE_SLAVE_ADDR_ACK      ,
        SMBMASTER_STATE_MASTER_CMD          ,
        SMBMASTER_STATE_SLAVE_CMD_ACK       ,
        SMBMASTER_STATE_MASTER_WRITE        ,
        SMBMASTER_STATE_SLAVE_WRITE_ACK     ,
        SMBMASTER_STATE_SLAVE_READ          ,
        SMBMASTER_STATE_MASTER_READ_ACK     ,
        SMBMASTER_STATE_STOP                ,
        SMBMASTER_STATE_STOP_THEN_START     ,
        SMBMASTER_STATE_ARBITRATION_LOST    ,
        SMBMASTER_STATE_START_WHEN_BUSY     
    }                               master_smbstate;                // current state of the master SMBus

    logic [3:0]                     master_bit_count;               // number of bits received in the current byte (starts at 0, counts up to 8)
    logic                           clear_master_bit_count;         // reset the master_bit_count to 0 when asserted
    logic [7:0]                     master_byte_in;                 // shift register to store incoming bits, shifted in starting at position 0 and shifting left (so first bit ends up as the msb)
    logic                           check_address_match;            // asserted (for one clock cycle) when a new address has first been received during the SLAVE_ADDRESS phase on the master bus
    logic                           assign_rdwr_bit;                // asserted (for one clock cycle) when read/write bit is received during the SLAVE_ADDRESS phase on the master bus
    logic                           clear_address_match;            // asserted when the address match mask needs to be cleared
    logic [NUM_RELAY_ADDRESSES:1]   address_match_mask;             // a bit is asserted when the incoming address matches the given element in the SMBUS_RELAY_ADDRESS array
    logic                           my_address;                     // asserted when the incoming command address matches ANY of the elements in the SMBUS_RELAY_ADDRESS array
    logic                           command_rd_wrn;                 // captured during the slave address phase, indicates if the current command is a READ (1) or WRITE (0) command
    logic                           command_whitelisted;            // asserted when the current command is on the 'whitelist' of allowed write commands (all read commands are allowed)
    logic                           master_read_nack;               // capture the ack/nack status from the master after a read data byte has been sent
    logic                           master_read_nack_valid;         // used to ensure master_read_nack is only captured on the first rising edge of clock after read data has been sent
    logic                           master_triggered_start;         // used to indicates that the next start condition is a repeated start
    logic                           arbitration_lost;               // arbitration monitor for master states. will be asserted when master loses arbitration and only deasserted when it reaches IDLE again
    logic                           arbitration_lost_count;         // keep count of when the master lost arbitration for the first time. so that it releases ia_master_sda_input to allow the master to retry again.
    logic [7:0]                     master_byte_in_for_address;     // just like master_byte_in, but stores actual master SDA for address (when arbitration lost, we pull SDA high internally so master_byte_in won't be the real address from the master)
    logic                           address_filtered;               // asserted (at SMBMASTER_STATE_MASTER_ADDR stage) when an address gets filtered. deasserted at SMBMASTER_STATE_STOP 
    
    enum {
        RELAY_STATE_IDLE                                    ,
        RELAY_STATE_START_WAIT_TIMEOUT                      ,
        RELAY_STATE_START_WAIT_MSCL_LOW                     ,
        RELAY_STATE_STOP_WAIT_SSCL_LOW                      ,
        RELAY_STATE_STOP_SSCL_LOW_WAIT_TIMEOUT              ,
        RELAY_STATE_STOP_SSDA_LOW_WAIT_SSCL_HIGH            ,
        RELAY_STATE_STOP_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT    ,
        RELAY_STATE_STOP_SSCL_HIGH_WAIT_SSDA_HIGH           ,
        RELAY_STATE_STOP_SSDA_HIGH_SSCL_HIGH_WAIT_TIMEOUT   ,
        RELAY_STATE_STOP_RESET_TIMEOUT_COUNTER              ,
        RELAY_STATE_STOP_WAIT_SECOND_TIMEOUT                ,
        RELAY_STATE_STOP_SECOND_RESET_TIMEOUT_COUNTER       ,
        // in all 'MTOS' (Master to Slave) states, the master is driving the SDA signal to the slave, so SDA must be relayed from the master bus to the slave bus (or sometimes a 'captured' version of SDA)
        RELAY_STATE_MTOS_MSCL_LOW_WAIT_SSCL_LOW             ,
        RELAY_STATE_MTOS_SSCL_LOW_WAIT_TIMEOUT              ,
        RELAY_STATE_MTOS_WAIT_MSCL_HIGH                     ,
        RELAY_STATE_MTOS_WAIT_SSCL_HIGH                     ,
        RELAY_STATE_MTOS_SSCL_HIGH_WAIT_TIMEOUT             ,
        RELAY_STATE_MTOS_WAIT_MSCL_LOW                      ,
        // in all 'STOM' (Slave to Master) states, the slave is driving the SDA signal to the master, so SDA must be relayed from the slave bus to the master bus (or sometimes a 'captured' version of SDA)
        RELAY_STATE_STOM_MSCL_LOW_WAIT_SSCL_LOW             ,
        RELAY_STATE_STOM_SSCL_LOW_WAIT_TIMEOUT              ,
        RELAY_STATE_STOM_WAIT_SSCL_HIGH                     ,
        RELAY_STATE_STOM_SSCL_HIGH_WAIT_MSCL_HIGH           ,
        RELAY_STATE_STOM_SSCL_LOW_WAIT_MSCL_HIGH            ,
        RELAY_STATE_STOM_SSCL_HIGH_WAIT_MSCL_LOW            ,
        RELAY_STATE_STOM_SSCL_LOW_WAIT_MSCL_LOW             ,
        // arbitration relay state
        RELAY_STATE_ARBITRATION_LOST                        ,
        RELAY_STATE_ARBITRATION_LOST_RETRY                  ,
        RELAY_STATE_READ_PHASE_ARBITRATION_LOST             ,
        RELAY_STATE_SET_SSCL_LOW_SSDA_HIGH                  ,
        RELAY_STATE_SET_SSCL_LOW_WAIT_TIMEOUT               ,
        RELAY_STATE_SET_SSDA_LOW_WAIT_SSCL_HIGH             ,
        RELAY_STATE_SET_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT
    }                                               relay_state;                    // current state of the relay between the master and slave busses
    logic [SCLK_HOLD_COUNTER_BIT_WIDTH-1:0]         slave_scl_hold_count;           // counter to determine how long to hold the slave scl signal low/high, counts down to -1 then waits to be restarted (so msb=1 indicates the counter has timed out)
    logic [SCLK_HOLD_COUNTER_BIT_WIDTH-1:0]         slave_sda_hold_count;           
    logic                                           slave_scl_hold_count_restart;   // combinatorial signal, reset the slave_scl_hold_count counter and start a new countdown
    logic                                           slave_sda_hold_count_restart;
    logic                                           slave_scl_hold_count_timeout;   // combinatorial signal (copy of msb of slave_scl_hold_count), indicates the counter has reached its timeout value and is not about to be restarted
    logic                                           slave_sda_hold_count_timeout;
    logic [SCLK_LOW_TIMEOUT_COUNTER_BIT_WIDTH-1:0]  master_scl_low_count;           // counter to determine how long the master scl has been held low (clock stretched), counts down to -1 then waits to be restarted ( so msb=1 indicates the counter has timed out)
    logic                                           master_scl_low_count_restart;   // combinational signal, reset the master_scl_low_count counter and start a new countdown
    logic                                           master_scl_low_count_timeout;   // combinational signal (copy of msb of master_scl_low_count), indicates the counter has reached its timeout value and is not about to be restarted
    logic [NINE_SCLK_COUNTER_BIT_WIDTH-1:0]         master_nine_scl_count;          // counter to determine 9 master scl has been observed, counts down to -1 then waits to be restarted ( so msb=1 indicates the counter has timed out)
    logic                                           master_nine_scl_count_restart;  // combinational signal, reset the master_nine_scl_count counter and start a new countdown
    logic                                           master_nine_scl_count_timeout;  // combinational signal (copy of msb of master_nine_scl_count), indicates the counter has reached its timeout value and is not about to be restarted
    logic                                           slave_captured_sda;             // slave sda signal captured on a rising edge of slave scl
    logic                                           latch_mux_state;                // latch the mux_slave_to_master configuration (master to slave OR slave to master) when detected in relay idle state, stay that way till it returns to idle again 
    logic                                           mux_slave_to_master;            // relay configuration: (0) master to slave default configuration OR (1) slave to master configuration 
    logic                                           role_swap_enable;               // Relay role swap feature: (0) disable (mux_slave_to_master always 0) or (1) enable
    
        
    //////////////////////////////////////////////////////////
    // MUXING logic for SMBus master and slave ports 
    //////////////////////////////////////////////////////////    
    // If the slave is IDLE, and we detect a slave_start condition, this could mean one of the following 
    // (a) another master has already started the slave bus
    // (b) the slave wants to become a master and is initiating the start condition.
    // If either happens, we will swap the slave ports with master ports. Therefore the slave can now start sending transactions as a master or listen to the ongoing winning master transaction.
    // Note that if the slave gets swapped for (b), it will probably get NACKed at address phase as the address won't match the master's address
    assign internal_ia_master_scl   = mux_slave_to_master ? ia_slave_scl                : ia_master_scl;
    assign o_master_scl_oe          = mux_slave_to_master ? internal_o_slave_scl_oe     : internal_o_master_scl_oe;
    assign internal_ia_master_sda   = mux_slave_to_master ? ia_slave_sda                : ia_master_sda;
    assign o_master_sda_oe          = mux_slave_to_master ? internal_o_slave_sda_oe     : internal_o_master_sda_oe;
    assign internal_ia_slave_scl    = mux_slave_to_master ? ia_master_scl               : ia_slave_scl;
    assign o_slave_scl_oe           = mux_slave_to_master ? internal_o_master_scl_oe    : internal_o_slave_scl_oe;
    assign internal_ia_slave_sda    = mux_slave_to_master ? ia_master_sda               : ia_slave_sda;
    assign o_slave_sda_oe           = mux_slave_to_master ? internal_o_master_sda_oe    : internal_o_slave_sda_oe;
    
    //set role_swap_enable based on the MCTP_OUT_OF_BAND_ENABLE parameter
    assign role_swap_enable = MCTP_OUT_OF_BAND_ENABLE[0];
    
    always_ff @(posedge clock or negedge i_resetn) begin
        if (!i_resetn) begin
        
            master_smbstate             <= SMBMASTER_STATE_IDLE;
            master_bit_count            <= 4'h0;
            clear_master_bit_count      <= '0;
            master_byte_in              <= 8'h00;
            check_address_match         <= '0;
            assign_rdwr_bit             <= '0;
            clear_address_match         <= '0;
            address_match_mask          <= {NUM_RELAY_ADDRESSES{1'b0}};
            my_address                  <= '0;
            command_rd_wrn              <= '0;
            master_read_nack            <= '0;
            master_read_nack_valid      <= '0;
            master_triggered_start      <= '0;
            arbitration_lost            <= '0;
            arbitration_lost_count      <= '0;
            master_byte_in_for_address  <= 8'h00;
            address_filtered            <= '0;
            
        end else begin
        
            case ( master_smbstate )
                // IDLE state
                // This is the reset state.  Wait here until a valid START condition is detected on the master smbus
                SMBMASTER_STATE_IDLE: begin
                    // send relay into (master interrupt when busy) flow if it detects a master start but the slave is busy and it's not a slave_start pulse, and also we've not been swapped! 
                    // this means another master is still sending transactions that's why the slave is busy and we've detected a master start which shouldn't be allowed. 
                    // relay sends us to SMBMASTER_STATE_START_WHEN_BUSY and RELAY_STATE_ARBITRATION_LOST_RETRY for start when busy arbitration flow.
                    // if relay has been swapped (mux_slave_to_master = 1), we will detect master stop when slave is busy as the direction is from slave to master. 
                    // Also, we want to make sure it's not slave_start because it is possible a few clock cycles before mux_slave_to_master is asserted, slave_busy has just been asserted + slave_start, so it's actually not a start when busy case but a slave to master case.
                    if (master_start & slave_busy & !mux_slave_to_master & !slave_start) begin          
                        master_smbstate <= SMBMASTER_STATE_START_WHEN_BUSY;
                        arbitration_lost <= '1; //if master gets started while the slave bus is still busy with another master transaction, we consider this as arbitration lost.
                    //leave the idle state to START if we detect a master 'start' condition. (this could be when master itself initiate transaction, or we got swapped and slave/other master initiated transaction)
                    end else if (master_start & ~i_block_disable) begin
                        master_smbstate <= SMBMASTER_STATE_START;
                    end
                    clear_master_bit_count <= '1;                               // hold the bit counter at 0 until we have exited the idle state
                    arbitration_lost_count <= '0;                               //reset arbitration_lost_count 
                    address_filtered <= '0;                                     //reset address_filtered
                end
                
                // START state
                // A start condition was detected on the master bus, we must stay here and clockstretch the master bus as required until the slave bus has 'caught up' and also issued a start condition
                SMBMASTER_STATE_START: begin
                    if (master_stop) begin                                      // start followed immediately by stop is strange, but not necessarily invalid
                        master_smbstate <= SMBMASTER_STATE_STOP;
                    end else begin
                        // once the master scl goes low after the start, we will hold it low, so we know we will stay in this state until the slave bus is ready to proceed
                        if (relay_state == RELAY_STATE_START_WAIT_TIMEOUT) begin        // relay has driven a 'start' condition to the slave bus, and is now 'synchronized' with the master bus
                            master_smbstate <= SMBMASTER_STATE_MASTER_ADDR;             // the first data bits to be received are the SMBus address and read/write bit
                        end
                    end
                    clear_master_bit_count <= '1;                               // hold the bit counter at 0
                end


                // MASTER_ADDR state
                // receive the 7 bit slave addres and the read/write bit
                // leave this state when all 8 bits have been received and the clock has been driven low again by the master
                SMBMASTER_STATE_MASTER_ADDR: begin
                    if (master_stop || master_start) begin                      // unexpected stop/start condition, ignore further master bus activity until we can issue a 'stop' condition on the slave bus
                        master_smbstate <= SMBMASTER_STATE_STOP;               
                    end else begin
                        //To support multi-master arbitration flow, we will always send the master to SMBMASTER_STATE_SLAVE_ADDR_ACK after it receives an address (regardless if its valid/invalid address). We  shall then ACK/NACK if its valid/invalid address to filter it in that stage.
                        if (master_bit_count == 4'h8 && master_scl_negedge == 1'b1 && !my_address) begin   // we have received the address, proceed to STOP is its an invalid address
                            master_smbstate <= SMBMASTER_STATE_SLAVE_ADDR_ACK;            // not our address, send a 'stop' condition on the slave bus and stop forwarding from the master until the next 'start'
                            address_filtered <= '1; //keep track that the address we are observing now is a filtered address
                        end
                        else if (master_bit_count == 4'h8 && master_scl_negedge == 1'b1) begin   // we have received all 8 data bits, time for the ACK. When we reach here, we know that the address has matched.
                            if (!(master_triggered_start && (command_rd_wrn == 1'b0))) begin  // to block write operation after repeated start
                                master_smbstate <= SMBMASTER_STATE_SLAVE_ADDR_ACK;
                            end else begin
                                master_smbstate <= SMBMASTER_STATE_STOP;
                            end
                        end
                        clear_master_bit_count <= '0;                           // do not reset the bit counter in this state, it will be reset during the next ACK state
                    end
                end
                
                // SLAVE_ADDR_ACK state
                // we may be driving the master_scl signal during this state to clock-stretch while awaiting an ACK from the slave bus
                // always enter this state after an scl falling edge
                // leave this state when the ack/nack bit has been sent and the clock has gone high then been drivien low again by the master
                // Even if arbitration lost, always ACK address phase (only if it is supported address) because if we NACK address, the master might think that the device is LOST 
                // Possible scenarios for ADDRESS + WRITE bit: 
                // |  SCENARIO A | Address sent | Arbitration | Address ACK |       next phase                          |
                // | [ Master 1] |  supported   |    WIN      |     ACK     |   complete transaction                    |
                // | [ Master 2] |  supported   |   LOSE      |     ACK     |  gets NACKed at CMD + arbitration lost    |
                //  
                // |  SCENARIO B | Address sent | Arbitration | Address ACK |       next phase                          |
                // | [ Master 1] |  supported   |    WIN      |     ACK     |  complete transaction                     |
                // | [ Master 2] |  unsupported |   LOSE      |     NACK    |  (filtered) STOP                          |
                //
                // |  SCENARIO C | Address sent | Arbitration | Address ACK |       next phase                          |
                // | [ Master 1] |  supported   |    LOSE     |     ACK     |   gets NACKed at CMD + arbitration lost   |
                // | [ Master 2] |  unsupported |    WIN      |     NACK    |   (filtered) STOP                         |
                // Possible scenarios for ADDRESS + READ bit: (if supported address lose, scenario A Master 2 behavior will be observed)
                // |  SCENARIO A | Address sent | Arbitration | Address ACK |       next phase                          |
                // | [ Master 1] |  supported   |    WIN      |     ACK     |  complete transaction                     |
                // | [ Master 2] |  supported   |   LOSE      |     ACK     |  gets clocked stretch till timeout        |
                SMBMASTER_STATE_SLAVE_ADDR_ACK: begin
                    if (master_stop || master_start) begin                      // unexpected stop/start condition, ignore further master bus activity until we can issue a 'stop' condition on the slave bus
                        master_smbstate <= SMBMASTER_STATE_STOP;
                    end else begin
                        if (master_scl_negedge) begin                           // ack/nack sent on clock rising edge, then need to wait for clock to go low again before leaving this state so it's safe to stop driving sda
                            // stop the master if we receive NACK from slave and when we've not lost arbitration (Bcos if we've lost arbitration, we should proceed to CMD phase)! OR if we've received a READ bit and we've already lost arbitration OR if address gets filtered.
                            if ( (slave_captured_sda & !arbitration_lost) || (command_rd_wrn & arbitration_lost) || address_filtered) begin                           // slave_captured_sda contains the ack (0) / nack (1) status of the last command, if we 'nack'ed send a stop on the slave bus and wait for a stop on the master bus
                                master_smbstate <= SMBMASTER_STATE_STOP;
                            end else if (command_rd_wrn & !arbitration_lost) begin
                                master_smbstate <= SMBMASTER_STATE_SLAVE_READ;              // this is a read command, start sending data back from slave
                            end else begin // we will proceed to MASTER_CMD state as long as we ACK regardless if we've lost arbitration or not
                                master_smbstate <= SMBMASTER_STATE_MASTER_CMD;              // receive the command on the next clock cycle
                            end
                        end
                    end
                    clear_master_bit_count <= '1;                               // hold the counter at 0 through this state, so it has the correct value at the start of the next state
                end
                        
                // MASTER_CMD state
                // receive the 8 bit command (the first data byte after the address is called the 'command')
                // always enter this state after an scl falling edge
                // leave this state when all 8 bits have been received and the clock has been driven low again by the master
                SMBMASTER_STATE_MASTER_CMD: begin
                    if (master_stop || master_start) begin                      // unexpected stop/start condition, ignore further master bus activity until we can issue a 'stop' condition on the slave bus
                        master_smbstate <= SMBMASTER_STATE_STOP;
                    end else begin
                        if (master_bit_count == 4'h8 && master_scl_negedge == 1'b1) begin   // we have received all 8 data bits, time for the ACK
                            master_smbstate <= SMBMASTER_STATE_SLAVE_CMD_ACK;
                        end
                    end
                    clear_master_bit_count <= '0;                           // do not reset the bit counter in this state, it will be reset during the next ACK state
                end

                // SLAVE_CMD_ACK or SLAVE_WRITE_ACK state
                // we may be driving the master_scl signal during this state to clock-stretch while awaiting an ACK from the slave bus
                // always enter this state after an scl falling edge
                // leave this state when the ack/nack bit has been sent and the clock has been drivien low again by the master 
                // Relay makes sure we only on NACK for arbitration lost in the SLAVE_WRITE_ACK/SLAVE_CMD_ACK states. Here, if we detect an arbitration_lost, we'll go to the STOP state. 
                SMBMASTER_STATE_SLAVE_CMD_ACK,
                SMBMASTER_STATE_SLAVE_WRITE_ACK: begin
                    if (master_stop || master_start) begin                      // unexpected stop/start condition, ignore further master bus activity until we can issue a 'stop' condition on the slave bus
                        master_smbstate <= SMBMASTER_STATE_STOP;
                    end else begin
                        if (master_scl_negedge) begin                           // ack/nack sent on clock rising edge, then need to wait for clock to go low again before leaving this state so it's safe to stop driving sda
                            if (slave_captured_sda || arbitration_lost) begin                           // slave_captured_sda contains the ack (0) / nack (1) status of the last command, if we 'nack'ed send a stop on the slave bus and wait for a stop on the master bus. arbitration_lost means we're 'nack'-ing the master to stop it as we've lost arbitration.
                                master_smbstate <= SMBMASTER_STATE_STOP;
                            end else begin
                                master_smbstate <= SMBMASTER_STATE_MASTER_WRITE;
                            end
                        end
                    end
                    clear_master_bit_count <= '1;                               // hold the counter at 0 through this state, so it has the correct value (0) at the start of the next state
                end

                // MASTER_WRITE state
                // receive a byte to write to the slave device
                // always enter this state after an scl falling edge
                // for non-whitelisted write commands, expect a 'restart' condition (which will allow a read command to begin) or abort the command by returning to the IDLE state
                // for whitelisted commands, allow the command to proceed
                SMBMASTER_STATE_MASTER_WRITE: begin
                    if (master_stop) begin                                      // unexpected stop condition, ignore further master bus activity until we can issue a 'stop' condition on the slave bus
                        master_smbstate <= SMBMASTER_STATE_STOP;
                    end else begin
                        if (command_whitelisted==1'b0 && i_filter_disable==1'b0 && master_bit_count == 4'h1 && master_scl_negedge == 1'b1) begin   // non-whitelisted command and no restart condition received, send a 'stop' condition on the slave bus and stop forwarding bits from the master bus
                            master_smbstate <= SMBMASTER_STATE_STOP;
                        end else if (master_start) begin                                // restart condition received, allow the command to proceed
                            master_smbstate <= SMBMASTER_STATE_START;
                        end else if (master_bit_count == 4'h8 && master_scl_negedge == 1'b1) begin   // we have received all 8 data bits for a whitelisted command, time for the ACK
                            master_smbstate <= SMBMASTER_STATE_SLAVE_WRITE_ACK;
                        end
                    end
                    clear_master_bit_count <= '0;                           // do not reset the bit counter in this state, it will be reset during the next ACK state
                end

                // SLAVE_READ state
                // receive a byte from the slave device and send it to the master
                // always enter this state after an scl falling edge
                SMBMASTER_STATE_SLAVE_READ: begin
                    if (master_stop || master_start) begin                      // unexpected stop/start condition, ignore further master bus activity until we can issue a 'stop' condition on the slave bus
                        master_smbstate <= SMBMASTER_STATE_STOP;
                    end else begin
                        if (master_bit_count == 4'h8 && master_scl_negedge == 1'b1) begin   // we have sent all 8 data bits, time for the ACK (from the master)
                            master_smbstate <= SMBMASTER_STATE_MASTER_READ_ACK;
                        end
                    end
                    clear_master_bit_count <= '0;                           // do not reset the bit counter in this state, it will be reset during the next ACK state
                end
                
                // MASTER_READ_ACK
                // always enter this state after an scl falling edge
                // leave this state when the ack/nack bit has been received and the clock has been drivien low again by the master 
                SMBMASTER_STATE_MASTER_READ_ACK: begin
                    if (master_stop || master_start) begin                      // unexpected stop/start condition, ignore further master bus activity until we can issue a 'stop' condition on the slave bus
                        master_smbstate <= SMBMASTER_STATE_STOP;
                    end else begin
                        if (master_scl_negedge) begin
                            if (~master_read_nack) begin                        // if we received an ack (not a nack) from the master, continue reading data from slave
                                master_smbstate <= SMBMASTER_STATE_SLAVE_READ;
                            end else begin                                      // on a nack, send a stop condition on the slave bus and wait for a stop on the master bus
                                master_smbstate <= SMBMASTER_STATE_STOP;
                            end
                        end
                    end
                    clear_master_bit_count <= '1;                               // hold the counter at 0 through this state, so it has the correct value (0) at the start of the next state
                end

                // STOP
                // Enter this state to indicate a STOP condition should be sent to the slave bus
                // Once the STOP has been sent on the slave bus, we return to the idle state and wait for another start condition
                // We can enter this state if a stop condition has been received on the master bus, or if we EXPECT a start condition on the master busses
                // We do not wait to actually see a stop condition on the master bus before issuing the stop on the slave bus and proceeding to the IDLE state
                SMBMASTER_STATE_STOP: begin
                    if (master_start) begin
                        master_smbstate <= SMBMASTER_STATE_STOP_THEN_START;     // we may receive a valid start condition while waiting to send a stop, we need to track that case
                    end else if (relay_state == RELAY_STATE_IDLE) begin
                        master_smbstate <= SMBMASTER_STATE_IDLE;
                    end else if (relay_state == RELAY_STATE_ARBITRATION_LOST || relay_state == RELAY_STATE_READ_PHASE_ARBITRATION_LOST) begin //if we've detected that relay is in the arbitration_lost states in relay FSM, bring master FSM to arbitration_lost state as well.
                        //hold the arbitration_count counter at 1 when we enter SMBMASTER_STATE_ARBITRATION_LOST state from SMBMASTER_STATE_STOP. This is so that we can release ia_master_sda_input (that's pulling SDA high to not corrupt slave SDA) now to allow master to retry later if they want to during this state.
                        arbitration_lost_count <= '1;
                        master_smbstate <= SMBMASTER_STATE_ARBITRATION_LOST;
                    end
                    clear_master_bit_count <= '1;                               // hold the counter at 0 through this state, so it has the correct value (0) at the start of the next state
                    address_filtered <= '0;
                end
                
                // ARBITRATION_LOST
                // The master has lost arbitration (ADDR + WRITE bit), we came from CMD or WRITE. We will stay here until the slave is not busy.
                // The master has lost arbitration (ADDR + READ bit), we came from ADDR_ACK. We will stay here till master SCL timeout  
                // If we detect a master start while in this state, means that the master is retrying, bring the master state to SMBMASTER_STATE_START_WHEN_BUSY to make it lose arbitration at ADDR phase and go back to IDLE only when slave is not busy.
                // If we are in normal RELAY_STATE_ARBITRATION_LOST state, bring master to IDLE when slave is not busy anymore.
                // If we are in read type RELAY_STATE_READ_PHASE_ARBITRATION_LOST, bring master to IDLE when detected SCL low timeout.
                SMBMASTER_STATE_ARBITRATION_LOST: begin
                    if (master_start) begin
                        master_smbstate <= SMBMASTER_STATE_START_WHEN_BUSY;
                    end else if (!slave_busy & relay_state == RELAY_STATE_ARBITRATION_LOST) begin
                        arbitration_lost_count <= '0;
                        master_smbstate <= SMBMASTER_STATE_IDLE;
                    end else if (master_scl_low_count_timeout & relay_state == RELAY_STATE_READ_PHASE_ARBITRATION_LOST) begin 
                        arbitration_lost_count <= '0;
                        master_smbstate <= SMBMASTER_STATE_IDLE;
                    end
                    clear_master_bit_count <= '1;                               // hold the counter at 0 through this state, so it has the correct value (0) at the start of the next state
                end

                // START_WHEN_BUSY
                // The master has lost arbitration is now waiting at ARBITRATION_LOST OR the master is not doing anything at the moment at IDLE, but the slave is busy (with other master)
                // In either of those states, if we've detected a master_start condition indicating the master wants to interrupt, and we see that the slave is busy, we shall come here.
                // When the master enters this state, it will be forced to lose arbitration at ADDR phase (in relay_state_arbitration_lost_retry, we force master_sda low)
                // The master will then only be allowed to go back to IDLE if we detect slave is not busy anymore, or if we achieved 9 SCL clocks, whichever is longer.
                SMBMASTER_STATE_START_WHEN_BUSY: begin
                    if (!slave_busy) begin
                        arbitration_lost_count <= '0;
                        master_smbstate <= SMBMASTER_STATE_IDLE;
                    end 
                    clear_master_bit_count <= '1;                               // hold the counter at 0 through this state, so it has the correct value (0) at the start of the next state
                end
                
                // STOP_THEN_START
                // While waiting to send a STOP on the slave bus, we receive a new start condition on the master busses
                // Must finish sending the stop condition on the slave bus, then send a start condition on the slave bus
                SMBMASTER_STATE_STOP_THEN_START: begin
                    if (relay_state == RELAY_STATE_IDLE || relay_state == RELAY_STATE_START_WAIT_TIMEOUT) begin
                        master_smbstate <= SMBMASTER_STATE_START;
                    end
                    clear_master_bit_count <= '1;                               // hold the counter at 0 through this state, so it has the correct value (0) at the start of the next state
                end

                
                default: begin                                                  // illegal state, should never get here
                    master_smbstate <= SMBMASTER_STATE_IDLE;
                    clear_master_bit_count <= '1;
                    arbitration_lost_count <= '0;
                    address_filtered <= '0;
                    // TODO flag some sort of error here?
                end
                
            endcase  

            // counter for bits received on the master bus
            // the master SMBus state machine will clear the counter at the appropriate times, otherwise it increments on every master scl rising edge
            if (clear_master_bit_count || master_start) begin       // need to reset the counter on a start condition to handle the repeated start (simpler than assigning clear signal in this one special case)
                master_bit_count <= 4'h0;
            end else begin
                if (master_scl_posedge) begin
                    master_bit_count <= master_bit_count + 4'b0001;
                end
            end
            
            // shift register to store incoming bits from the master bus
            // shifts on every clock edge regardless of bus state, rely on master_smbstate and master_bit_count to determine when a byte is fully formed
            if (master_scl_posedge) begin
                master_byte_in[7:0] <= {master_byte_in[6:0], master_sda};
                master_byte_in_for_address[7:0] <= {master_byte_in_for_address[6:0], master_sda_address};
            end
            
            // determine if the incoming address matches one of our relay addresses, and if this is a read or write command
            if ( (master_smbstate == SMBMASTER_STATE_MASTER_ADDR) && (master_scl_posedge) && (master_bit_count == 4'h6) ) begin
                check_address_match <= '1;
            end else begin
                check_address_match <= '0;
            end
                
            if ( (master_smbstate == SMBMASTER_STATE_MASTER_ADDR) && (master_scl_posedge) && (master_bit_count == 4'h7) ) begin // 8th bit is captured on the next clock cycle, so that's when it's safe to check the full byte contents
                assign_rdwr_bit <= '1;
            end else begin
                assign_rdwr_bit <= '0;
            end
            
            if ( (master_smbstate == SMBMASTER_STATE_IDLE) ) begin
                clear_address_match <= '1;
            end else begin
                clear_address_match <= '0;
            end
            
            //always use actual master sda address (master_byte_in_for_address) data for address matching and command read/write bit 
            if (clear_address_match) begin
                address_match_mask[NUM_RELAY_ADDRESSES:1] <= {NUM_RELAY_ADDRESSES{1'b0}};
            end else if (check_address_match) begin
                for (int i=1; i<= NUM_RELAY_ADDRESSES; i++) begin
                    if (master_byte_in_for_address[6:IGNORED_ADDR_BITS] == SMBUS_RELAY_ADDRESS[i][6:IGNORED_ADDR_BITS]) begin
                        address_match_mask[i] <= '1;
                    end else begin
                        address_match_mask[i] <= '0;
                    end
                end
            end 
            
            if (assign_rdwr_bit) begin
                command_rd_wrn <= master_byte_in_for_address[0];        // lsb of the byte indicates read (1) or write (0) command. Get the real master_sda read or write, not the internal master_sda used for arbitration
            end
            my_address <= i_relay_all_addresses ? '1 : |address_match_mask;
            
            // capture the read data ACK/NACK from the master after read data has been sent
            // make sure to only capture ack/nack on the first rising edge of SCL using the master_read_nack_valid signal
            if (master_smbstate == SMBMASTER_STATE_MASTER_READ_ACK) begin
                if (master_scl_posedge) begin
                    if (!master_read_nack_valid) begin
                        master_read_nack <= master_sda;
                    end
                    master_read_nack_valid <= '1;
                end
            end else begin
                master_read_nack_valid <= '0;
                master_read_nack <= '1;
            end
            
            // repeated start can only be used to switch from write mode to read mode in SMBus protocol
            if (master_smbstate == SMBMASTER_STATE_MASTER_CMD) begin
                master_triggered_start <= '1;
            end else if ((master_smbstate == SMBMASTER_STATE_STOP) || (master_smbstate == SMBMASTER_STATE_IDLE)) begin            
                master_triggered_start <= '0;
            end

            // monitor for arbitraton lost on the slave bus
            // At the rising edge of master SCL master_scl_posedge, check all 3 states where the master is driving the slave bus (address, cmd, write data)
            // SMBUS protocol dictates that we can only drive LOW as it is open-drain. The master that drives the first 'LOW' bit (internal_ia_slave_sda == 0 && internal_o_slave_sda_oe == 1) wins the bus. 
            // That means the other master that was driving HIGH has already lost, and the data on the slave bus (internal_ia_slave_sda, intended to be 1 but now 0 bcos of arbitration) would already not match with the data intended to be driven by master (internal_o_slave_sda_oe == 0), this master has lost arbitration. 
            if (((master_smbstate == SMBMASTER_STATE_MASTER_ADDR) || (master_smbstate == SMBMASTER_STATE_MASTER_CMD) || (master_smbstate == SMBMASTER_STATE_MASTER_WRITE)) && (internal_ia_slave_sda == 0 && internal_o_slave_sda_oe == 0) && master_scl_posedge) begin 
                arbitration_lost <= '1;
            // release arbitration_lost state when the master goes back to IDLE, 
            end else if (master_smbstate == SMBMASTER_STATE_IDLE) begin 
                arbitration_lost <= '0;
            end 
        end
        
    end
    
    //when arbitration_lost is asserted, we take over the master_sda and pull it to HIGH until the entire transaction is complete (this is to ensure this master backs off since it already lost the arbitration)
    //we will stop this master when it reaches the ACK/NACK stage by 'nack'ing it then stop (only when we reach SLAVE_WRITE_ACK/SLAVE_CMD_ACK which are the stages for master write ack). 
    //only do this for the first round of arbitration (when both masters start at the same time) arbitration_lost_count == 0. Once the master loses, any subsequent master retries will not have to pull ia_master_sda_input anymore as we've stopped forwarding to slave bus
    //once the losing master gets to the SMBMASTER_STATE_ARBITRATION_LOST stage, we can release ia_master_sda_input. 
    assign ia_master_sda_input = (arbitration_lost & master_smbstate != SMBMASTER_STATE_ARBITRATION_LOST & arbitration_lost_count == 0) ? '1 : internal_ia_master_sda;
    
    // map address_match_mask (one-hot encoded) back to a binary representation to use as address inputs to the whitelist RAM
    logic [4:0]         address_match_index;    // (zero-based) index (in SMBUS_RELAY_ADDRESS array) that matches the received SMBus address (if there is no match, value is ignored so is not important)
    always_comb begin
        address_match_index = '0;
        for (int i=0; i<NUM_RELAY_ADDRESSES; i++) begin
            if (address_match_mask[i+1]) begin
                address_match_index |= i[4:0];      // use |= to avoid creating a priority encoder - we know only one bit in the address_match_mask array can be setting
            end
        end
    end

    
    ///////////////////////////////////////
    // Determine the state of the relay, and drive the slave and master SMBus signals based on the state of the master and slave busses
    ///////////////////////////////////////

    always_ff @(posedge clock or negedge i_resetn) begin
        if (!i_resetn) begin

            relay_state                     <= RELAY_STATE_IDLE;
            internal_o_master_scl_oe        <= '0;
            internal_o_master_sda_oe        <= '0;
            internal_o_slave_scl_oe         <= '0;
            internal_o_slave_sda_oe         <= '0;
            slave_scl_hold_count            <= {SCLK_HOLD_COUNTER_BIT_WIDTH{1'b0}};
            slave_sda_hold_count            <= {SCLK_HOLD_COUNTER_BIT_WIDTH{1'b0}};
            master_scl_low_count            <= {SCLK_LOW_TIMEOUT_COUNTER_BIT_WIDTH{1'b0}};
            master_nine_scl_count           <= {NINE_SCLK_COUNTER_BIT_WIDTH{1'b0}};
            slave_captured_sda              <= '0;
            mux_slave_to_master             <= '0;
            latch_mux_state                 <= '0;
            
        end else begin
        
            case ( relay_state )
            
                // IDLE state
                // waiting for a start condition on the master busses
                RELAY_STATE_IDLE: begin
                    if (master_smbstate == SMBMASTER_STATE_START) begin
                        relay_state <= RELAY_STATE_START_WAIT_TIMEOUT;
                    end else if (master_smbstate == SMBMASTER_STATE_START_WHEN_BUSY) begin //if we detect that this master has started while the slave is busy, it will be sent to SMBMASTER_STATE_START_WHEN_BUSY, that means this master tried to interrupt the ongoing transaction from another master who has occupied the slave bus
                        relay_state <= RELAY_STATE_ARBITRATION_LOST_RETRY; //in this case, send the relay to start when busy flow, where relay will force master SDA to LOW to make master lose arbitration. 
                    end 
                    // waiting for a slave_start condition on the slave bus (if this happens, swap the roles so that slave is now the master)
                    // Scenario A: A slave has initiated a start condition
                    // Scenario B: Another master has already won the slave bus and we are seeing it's slave start condition. Once swapped, we will be forwarded the other master's transaction
                    // if slave started the transaction, once it swaps roles, master_start will be detected bcos input to slave is now input to master
                    // Swapped roles slave to master configuration: detected slave_start in relay idle state for the first time, stay swapped till it returns to IDLE again
                    if ( slave_start & latch_mux_state == 0 & role_swap_enable) begin 
                        mux_slave_to_master <= '1;
                        latch_mux_state <= '1;
                    // Original master to slave configuration: did not detect slave_start in relay idle state for the first time, stay swapped till it returns to IDLE again
                    end else if ( !slave_start & latch_mux_state == 0) begin 
                        mux_slave_to_master <= '0;
                    end

                    internal_o_master_scl_oe <= '0;
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '0;
                end

                // START_WAIT_TIMEOUT
                // Start condition has been received on the master bus
                // Drive a start condition on the slave bus (SDA goes low while SCL is high) and wait for a timeout
                // If master SCL goes low during this state, hold it there to prvent further progress on the master bus until the slave bus can 'catch up'
                RELAY_STATE_START_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_START_WAIT_MSCL_LOW;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // if master SCL goes low, we hold it low
                    internal_o_master_sda_oe <= '0; 
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '1;                      // drive a '0' onto the slave SDA bus, to create a start condition
                end

                // START_WAIT_MSCL_LOW
                // Start condition has been received on the master bus
                // Continue to drive a start condition on the slave bus (SDA low while SCL is high) and wait for master scl to go low
                // If master SCL is low during this state, continue to hold it there to prvent further progress on the master bus until the slave bus can 'catch up'
                RELAY_STATE_START_WAIT_MSCL_LOW: begin
                    if (~master_scl) begin
                        relay_state <= RELAY_STATE_MTOS_MSCL_LOW_WAIT_SSCL_LOW;     // after a start, the master is driving the bus
                    end else if (master_smbstate == SMBMASTER_STATE_STOP) begin        // stop right after start may not be legal, but safter to handle this case anyway
                        relay_state <= RELAY_STATE_STOP_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // if master SCL goes low, we hold it low
                    internal_o_master_sda_oe <= '0; 
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '1;                      // drive a '0' onto the slave SDA bus, to create a start condition
                end

                // MTOS_MSCL_LOW_WAIT_SSCL_LOW
                // This state exists to provide some hold time on the slave SDA signal, we drive slave SCL low and wait until we observe it low (which has several clock cycles of delay) before proceeding
                // While in this state, we hold the old value of internal_o_slave_sda_oe
                // Clockstretch the master SCL (which is also low) during this state
                RELAY_STATE_MTOS_MSCL_LOW_WAIT_SSCL_LOW: begin
                    if (~slave_scl) begin
                        relay_state <= RELAY_STATE_MTOS_SSCL_LOW_WAIT_TIMEOUT;
                    end
                    internal_o_master_scl_oe <= ~master_scl;                        // we know master SCL is low here, we continue to hold it low
                    internal_o_master_sda_oe <= '0; 
                    internal_o_slave_scl_oe  <= '1;                                 // we drive the slave SCL low
                    internal_o_slave_sda_oe  <= internal_o_slave_sda_oe;            // do not change the value of internal_o_slave_sda_oe, we are providing some hold time on this signal after the falling edge of slave SCL
                end

                // MTOS_SSCL_LOW_WAIT_TIMEOUT
                // Master is driving the bus, SDA from the master bus to the slave bus
                // We are driving slave SCL low, wait for a timeout before allowing it to go high
                // Clockstretch the master SCL (which is also low) during this state
                RELAY_STATE_MTOS_SSCL_LOW_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_MTOS_WAIT_MSCL_HIGH;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // we know master SCL is low here, we continue to hold it low
                    internal_o_master_sda_oe <= '0; 
                    internal_o_slave_scl_oe  <= '1;                      // we drive the slave SCL low
                    internal_o_slave_sda_oe  <= ~master_sda;             // drive the 'live' value from the master SDA onto the slave SDA
                end

                
                // MTOS_WAIT_MSCL_HIGH
                // Master is driving the bus, SDA from the master bus to the slave bus
                // Release master SCL and wait for it to go high, while continuing to hold slave scl low
                RELAY_STATE_MTOS_WAIT_MSCL_HIGH: begin
                    if (master_smbstate == SMBMASTER_STATE_STOP || master_smbstate == SMBMASTER_STATE_STOP_THEN_START) begin
                        relay_state <= RELAY_STATE_STOP_WAIT_SSCL_LOW;
                    end else begin
                        if (master_scl) begin
                            relay_state <= RELAY_STATE_MTOS_WAIT_SSCL_HIGH;
                        end
                    end
                    internal_o_master_scl_oe <= '0;
                    internal_o_master_sda_oe <= '0; 
                    internal_o_slave_scl_oe  <= '1;                      // we drive the slave SCL low
                    internal_o_slave_sda_oe  <= ~master_sda;             // drive the 'live' value from the master SDA onto the slave SDA
                end
                
                // MTOS_WAIT_SSCL_HIGH
                // Master is driving the bus, SDA from the master bus to the slave bus
                // Release slave SCL and wait for it to go high
                // If master SCL goes low again during this state, hold it there to prvent further progress on the master bus until the slave bus can 'catch up'
                RELAY_STATE_MTOS_WAIT_SSCL_HIGH: begin
                    if (slave_scl) begin
                        relay_state <= RELAY_STATE_MTOS_SSCL_HIGH_WAIT_TIMEOUT;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // if master SCL goes low, we hold it low
                    internal_o_master_sda_oe <= '0; 
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= ~master_byte_in[0];      // drive the most recent captured bit from the master sda onto the slave sda
                end

                // MTOS_SSCL_HIGH_WAIT_TIMEOUT
                // Master is driving the bus, SDA from the master bus to the slave bus
                // Slave SCL is high, wait for a timeout before allowing it to be driven low
                // If master SCL goes low again during this state, hold it there to prvent further progress on the master bus until the slave bus can 'catch up'
                RELAY_STATE_MTOS_SSCL_HIGH_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_MTOS_WAIT_MSCL_LOW;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // if master SCL goes low, we hold it low
                    internal_o_master_sda_oe <= '0; 
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= ~master_byte_in[0];      // drive the most recent captured bit from the master sda onto the slave sda
                end


                // MTOS_WAIT_MSCL_LOW
                // Master is driving the bus, SDA from the master bus to the slave bus
                // Wait for master SCL to go low, then hold it low (clockstretch) and proceed to state where we drive the slave scl low
                RELAY_STATE_MTOS_WAIT_MSCL_LOW: begin
                    if (master_smbstate == SMBMASTER_STATE_START) begin
                        relay_state <= RELAY_STATE_START_WAIT_TIMEOUT;
                    end else if (master_smbstate == SMBMASTER_STATE_STOP || master_smbstate == SMBMASTER_STATE_STOP_THEN_START) begin
                        if (~master_byte_in[0]) begin       // we know slave SCL is high, if slave SDA is low proceed to the appropriate phase of the STOP states
                            relay_state <= RELAY_STATE_STOP_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT;
                        end else begin                      // we need to first drive slave SCL low so we can drive slave SDA low then proceed to create the STOP condition
                            relay_state <= RELAY_STATE_STOP_WAIT_SSCL_LOW;
                        end
                    end else begin
                        if (~master_scl_dly) begin      // need to look at a delayed version of master scl, to give master_smbstate state machine time to update
                            // check if we are in a state where the slave is driving data back to the master bus
                            if (    (master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK) ||
                                    (master_smbstate == SMBMASTER_STATE_SLAVE_CMD_ACK) ||
                                    (master_smbstate == SMBMASTER_STATE_SLAVE_WRITE_ACK) ||
                                    (master_smbstate == SMBMASTER_STATE_SLAVE_READ)
                            ) begin
                                relay_state <= RELAY_STATE_STOM_MSCL_LOW_WAIT_SSCL_LOW;
                            end else begin
                                relay_state <= RELAY_STATE_MTOS_MSCL_LOW_WAIT_SSCL_LOW;
                            end
                        end
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // if master SCL is low, we hold it low
                    internal_o_master_sda_oe <= '0; 
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= ~master_byte_in[0];      // drive the most recent captured bit from the master sda onto the slave sda
                end

                // STOM_MSCL_LOW_WAIT_SSCL_LOW
                // We start driving slave SCL low, wait to observe it actually going low before releasing the internal_o_slave_sda_oe signal
                // This state exists to provide some hold timing on slave SDA after slave SCL goes low
                // Clockstretch the master SCL (which is also low) during this state
                // If arbitration lost happens:
                // When slave responds to master in STOM phase, and an arbitration lost has been detected, we will NACK it if it is the CMD or WRITE phase.
                // If it is the ADDR phase, we ACK it and allow it to proceed to CMD phase and then it will only get NACKed then.(if address is supported; if address is unsupported we will NACK it right away at ADDR phase)
                // In normal scenario, if arbitration lost is not detected, we just forward the slave_sda line actual data back to master (ACK/NACK)
                RELAY_STATE_STOM_MSCL_LOW_WAIT_SSCL_LOW: begin
                    if (~slave_scl) begin
                        relay_state <= RELAY_STATE_STOM_SSCL_LOW_WAIT_TIMEOUT;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // we know master SCL is low here, we continue to hold it low
                    // if arbitration lost and master is NOT at address ACK state (CMD/WRITE ACK state), OR if master is at address ACK state and address is being filtered (don't care if arbitration lost or not), we should NACK the master. 
                    // else if arbitration lost and master is at address ACK state (but the address is not being filtered), we should ACK the master 
                    // else, by default we will just forward the slave sda to master at times when arbitration is not lost.
                    internal_o_master_sda_oe <= ((arbitration_lost & master_smbstate != SMBMASTER_STATE_SLAVE_ADDR_ACK) || (master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK & address_filtered)) ? '0 : (arbitration_lost & master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK) ? '1 : ~slave_sda;              // drive sda from the slave bus to the master bus. if arbitration is lost, we take over the slave_sda and NACK the master. Note that the actual slave_sda will be (ACK) bcos the other master that won arbitration is in control. Therefore, we must NACK the master ourselves.
                    internal_o_slave_scl_oe  <= '1;                      // we drive the slave SCL low
                    internal_o_slave_sda_oe  <= internal_o_slave_sda_oe;          // do not change the value of slave SDA, we are providing hold time on slave SDA in this state after falling edge of slave SCL
                end

                // STOM_SSCL_LOW_WAIT_TIMEOUT
                // Slave is driving data, SDA from the slave bus to the master
                // We are driving slave SCL low, wait for a timeout before allowing it to go high
                // Clockstretch the master SCL (which is also low) during this state
                RELAY_STATE_STOM_SSCL_LOW_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_STOM_WAIT_SSCL_HIGH;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // we know master SCL is low here, we continue to hold it low
                    internal_o_master_sda_oe <= ((arbitration_lost & master_smbstate != SMBMASTER_STATE_SLAVE_ADDR_ACK) || (master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK & address_filtered)) ? '0 : (arbitration_lost & master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK) ? '1 : ~slave_sda;              // drive sda from the slave bus to the master bus. if arbitration is lost, we take over the slave_sda and NACK the master. Note that the actual slave_sda will be (ACK) bcos the other master that won arbitration is in control. Therefore, we must NACK the master ourselves.
                    internal_o_slave_scl_oe  <= '1;                      // we drive the slave SCL low
                    internal_o_slave_sda_oe  <= '0;
                end

                // STOM_WAIT_SSCL_HIGH
                // Slave is driving data, SDA from the slave bus to the master
                // Release slave SCL and wait for it to go high
                // Continue to clockstretch (hold SCL low) on the master bus until we have received data from the slave bus
                RELAY_STATE_STOM_WAIT_SSCL_HIGH: begin
                    if (slave_scl) begin
                        relay_state <= RELAY_STATE_STOM_SSCL_HIGH_WAIT_MSCL_HIGH;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // we know master SCL is low here, we continue to hold it low
                    internal_o_master_sda_oe <= ((arbitration_lost & master_smbstate != SMBMASTER_STATE_SLAVE_ADDR_ACK) || (master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK & address_filtered)) ? '0 : (arbitration_lost & master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK) ? '1 : ~slave_sda;              // drive sda from the slave bus to the master bus. if arbitration is lost, we take over the slave_sda and NACK the master
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '0;
                end

                // STOM_SSCL_HIGH_WAIT_MSCL_HIGH
                // Slave is driving data, SDA from the slave bus to the master
                // Slave SCL is high, release master SCL and wait for it to go high
                // Continue to clockstretch (hold SCL low) on the master bus until we have received data from the slave bus
                RELAY_STATE_STOM_SSCL_HIGH_WAIT_MSCL_HIGH: begin
                    if (master_scl) begin
                        relay_state <= RELAY_STATE_STOM_SSCL_HIGH_WAIT_MSCL_LOW;
                    end else if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_STOM_SSCL_LOW_WAIT_MSCL_HIGH;
                    end
                    internal_o_master_scl_oe <= '0;
                    internal_o_master_sda_oe <= ((arbitration_lost & master_smbstate != SMBMASTER_STATE_SLAVE_ADDR_ACK) || (master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK & address_filtered)) ? '0 : (arbitration_lost & master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK) ? '1 : ~slave_captured_sda;     // drive master sda with the value captured on the rising edge of slave scl. if arbitration is lost, we take over the slave_sda and NACK the master. Note that the actual slave_sda will be (ACK) bcos the other master that won arbitration is in control. Therefore, we must NACK the master ourselves.
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '0;
                end
                
                // STOM_SSCL_LOW_WAIT_MSCL_HIGH
                // Slave is driving data, SDA from the slave bus to the master
                // Slave SCL has been high long enough that a timeout occurred, so need to drive it low again to comply with SMBus timing rules
                // Still waiting for the master SCL to go high so the master bus can 'catch up'
                RELAY_STATE_STOM_SSCL_LOW_WAIT_MSCL_HIGH: begin
                    if (master_scl) begin
                        relay_state <= RELAY_STATE_STOM_SSCL_LOW_WAIT_MSCL_LOW;
                    end
                    internal_o_master_scl_oe <= '0;
                    internal_o_master_sda_oe <= ((arbitration_lost & master_smbstate != SMBMASTER_STATE_SLAVE_ADDR_ACK) || (master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK & address_filtered)) ? '0 : (arbitration_lost & master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK) ? '1 : ~slave_captured_sda;     // drive master sda with the value captured on the rising edge of slave scl. if arbitration is lost, we take over the slave_sda and NACK the master. Note that the actual slave_sda will be (ACK) bcos the other master that won arbitration is in control. Therefore, we must NACK the master ourselves.
                    internal_o_slave_scl_oe  <= '1;                      // drive the slave SCL low to avoid a timing error from leaving scl high for too long
                    internal_o_slave_sda_oe  <= '0;
                end
 
                // STOM_SSCL_HIGH_WAIT_MSCL_LOW
                // Slave is driving data, SDA from the slave bus to the master
                // Slave and master SCL are high, we are waiting for master SCL to go low
                // If arbitration lost happens:
                // send relay to RELAY_STATE_ARBITRATION_LOST if ADDR + WRITE bit OR CMD / WRITE phase 
                // send relay to RELAY_STATE_READ_PHASE_ARBITRATION_LOST if ADDR + READ bit
                RELAY_STATE_STOM_SSCL_HIGH_WAIT_MSCL_LOW: begin
                    if ((master_smbstate == SMBMASTER_STATE_STOP & !arbitration_lost) || master_smbstate == SMBMASTER_STATE_STOP_THEN_START) begin //only send slave to stop if master stops and its not because of arbitration. Send to stop if it is stop then start condition too. 
                        relay_state <= RELAY_STATE_STOP_WAIT_SSCL_LOW;
                    end else begin
                        if (~master_scl_dly) begin      // need to look at a delayed version of master scl, to give master_smbstate state machine time to update
                            if ( master_smbstate == SMBMASTER_STATE_SLAVE_READ & !arbitration_lost ) begin      // only in the SLAVE_READ state do we have two SMBus cycles in a row where the slave drives data to the master (and make sure to only proceed if we've not lost arbitration)
                                relay_state <= RELAY_STATE_STOM_SSCL_LOW_WAIT_TIMEOUT;
                            //  when master loses arbitration, master might be sent to STOP state if it lost arbitration during CMD or WRITE phase AND not a READ bit
                            // or sent to STOP state when arbitration lost at ADDR phase + READ bit + filtered, 
                            // or sent to CMD state when arbitration lost at ADDR phase + WRITE bit. send them all to to arbitration_lost relay state 
                            end else if ((master_smbstate == SMBMASTER_STATE_STOP & arbitration_lost & !command_rd_wrn) || (master_smbstate == SMBMASTER_STATE_STOP & arbitration_lost & command_rd_wrn & address_filtered) || (master_smbstate == SMBMASTER_STATE_MASTER_CMD & arbitration_lost)) begin 
                                relay_state <= RELAY_STATE_ARBITRATION_LOST;
                            // when master loses arbitration at ADDR phase and it is a READ bit, send it to special read_phase_arbitration_lost relay state
                            end else if (master_smbstate == SMBMASTER_STATE_STOP & arbitration_lost & command_rd_wrn) begin
                                relay_state <= RELAY_STATE_READ_PHASE_ARBITRATION_LOST;
                            end else begin
                                relay_state <= RELAY_STATE_MTOS_SSCL_LOW_WAIT_TIMEOUT;
                            end
                        end
                    end
                    internal_o_master_scl_oe <= '0;
                    internal_o_master_sda_oe <= ((arbitration_lost & master_smbstate != SMBMASTER_STATE_SLAVE_ADDR_ACK) || (master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK & address_filtered)) ? '0 : (arbitration_lost & master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK) ? '1 : ~slave_captured_sda;     // drive master sda with the value captured on the rising edge of slave scl. if arbitration is lost, we take over the slave_sda and NACK the master. Note that the actual slave_sda will be (ACK) bcos the other master that won arbitration is in control. Therefore, we must NACK the master ourselves.
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '0;
                end

                // STOM_SSCL_LOW_WAIT_MSCL_LOW
                // Slave is driving data, SDA from the slave bus to the master
                // A timeout occured while waiting for master SCL to go high which forced us to drive slave SCL low
                // Master SCL has gone high, we are waiting for it to go low again
                RELAY_STATE_STOM_SSCL_LOW_WAIT_MSCL_LOW: begin
                    if ((master_smbstate == SMBMASTER_STATE_STOP & !arbitration_lost) || master_smbstate == SMBMASTER_STATE_STOP_THEN_START) begin //only send slave to stop if master stops and its not because of arbitration. Send to stop if it is stop then start condition too. 
                        relay_state <= RELAY_STATE_STOP_WAIT_SSCL_LOW;
                    end else begin
                        if (~master_scl_dly) begin      // need to look at a delayed version of master scl, to give master_smbstate state machine time to update
                            if ( master_smbstate == SMBMASTER_STATE_SLAVE_READ & !arbitration_lost ) begin      // only in the SLAVE_READ state do we have two SMBus cycles in a row where the slave drives data to the master (and only proceed if we've not lost arbitration)
                                relay_state <= RELAY_STATE_STOM_SSCL_LOW_WAIT_TIMEOUT;
                            //  when master loses arbitration, master might be sent to STOP state if it lost arbitration during CMD or WRITE phase AND not a READ bit
                            // or sent to STOP state when arbitration lost at ADDR phase + READ bit + filtered, 
                            // or sent to CMD state when arbitration lost at ADDR phase + WRITE bit. send them all to to arbitration_lost relay state 
                            end else if ((master_smbstate == SMBMASTER_STATE_STOP & arbitration_lost & !command_rd_wrn) || (master_smbstate == SMBMASTER_STATE_STOP & arbitration_lost & command_rd_wrn & address_filtered) || (master_smbstate == SMBMASTER_STATE_MASTER_CMD & arbitration_lost)) begin 
                                relay_state <= RELAY_STATE_ARBITRATION_LOST;
                            end else if (master_smbstate == SMBMASTER_STATE_STOP & arbitration_lost & command_rd_wrn & !address_filtered) begin
                                relay_state <= RELAY_STATE_READ_PHASE_ARBITRATION_LOST;
                            end else begin
                                relay_state <= RELAY_STATE_MTOS_SSCL_LOW_WAIT_TIMEOUT;
                            end
                        end
                    end
                    internal_o_master_scl_oe <= '0;
                    internal_o_master_sda_oe <= ((arbitration_lost & master_smbstate != SMBMASTER_STATE_SLAVE_ADDR_ACK) || (master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK & address_filtered)) ? '0 : (arbitration_lost & master_smbstate == SMBMASTER_STATE_SLAVE_ADDR_ACK) ? '1 : ~slave_captured_sda;     // drive master sda with the value captured on the rising edge of slave scl. if arbitration is lost, we take over the slave_sda and NACK the master. Note that the actual slave_sda will be (ACK) bcos the other master that won arbitration is in control. Therefore, we must NACK the master ourselves.
                    internal_o_slave_scl_oe  <= '1;                      // continue to force slave SCL low
                    internal_o_slave_sda_oe  <= '0;
                end

                // RELAY_STATE_ARBITRATION_LOST
                // Doesn't drive anything from the master bus but only listen to the ongoing slave bus
                // If it detects the master has gone back to IDLE, means all slave is not busy anymore and it's safe to go back to idle. 
                // If it detects that the master goes to START_WHEN_BUSY, means that the master is retrying while the slave is busy, send it to retry state 
                RELAY_STATE_ARBITRATION_LOST: begin
                    if (master_smbstate == SMBMASTER_STATE_IDLE) begin
                        relay_state <= RELAY_STATE_IDLE;     
                        latch_mux_state <= '0; //we can reset the mux_latch when we go back to IDLE
                    end else if (master_smbstate == SMBMASTER_STATE_START_WHEN_BUSY) begin 
                        relay_state <= RELAY_STATE_ARBITRATION_LOST_RETRY;
                    end 
                    internal_o_master_scl_oe <= '0;           
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '0;
                end
                
                // RELAY_STATE_READ_PHASE_ARBITRATION_LOST
                // Only come here if master lost arbitration during ADDRESS phase and it set READ bit = 1.
                // If arbitration was lost when master sent an address with READ bit, relay will have to ACK the address that it supports.
                // The relay will then clock stretch the master SCL so that it can't send anything on the bus. 
                // The relay will clock stretch till time out to make the master drop the transaction.
                RELAY_STATE_READ_PHASE_ARBITRATION_LOST: begin
                    if (master_scl_low_count_timeout) begin 
                        relay_state <= RELAY_STATE_IDLE;  
                        latch_mux_state <= '0; //we can reset the mux_latch when we go back to IDLE
                    end 
                    internal_o_master_scl_oe <= '1;       //master scl is held low, to clock stretch it till timeout occurs    
                    internal_o_master_sda_oe <= '1;
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '0;
                end
                
                // RELAY_STATE_ARBITRATION_LOST_RETRY 
                // Here the master has attempted to start the bus while the slave is busy in RELAY_STATE_ARBITRATION_LOST (master already lost arbitration) or idle in RELAY_STATE_IDLE
                // The relay will allow master to lose arbitration by forcing SDA LOW on master_sda
                // The relay will only leave this state if it detects the master has gone back to IDLE
                RELAY_STATE_ARBITRATION_LOST_RETRY: begin
                    if (master_nine_scl_count_timeout) begin //at least 9 master SCL have been detected 
                        if (master_smbstate == SMBMASTER_STATE_IDLE) begin
                            relay_state <= RELAY_STATE_IDLE;     
                            latch_mux_state <= '0; //we can reset the mux_latch when we go back to IDLE
                        end
                    end 
                    internal_o_master_scl_oe <= '0;  //Allow master SCL to be driven by master       
                    internal_o_master_sda_oe <= '1;  //We force master_sda to be LOW 
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '0;
                end
                
                // STOP_WAIT_SSCL_LOW
                // Sending a stop condition on the slave bus
                // Drive slave SCL low, wait to see it has gone low before driving SDA low (which happens in the next state)
                // Clockstretch the master bus if master SCL is driven low, to prevent the master bus from getting 'ahead' of the slave bus
                RELAY_STATE_STOP_WAIT_SSCL_LOW: begin
                    if (~slave_scl) begin
                        relay_state <= RELAY_STATE_STOP_SSCL_LOW_WAIT_TIMEOUT;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '1;                      // we drive slave SCL low
                    internal_o_slave_sda_oe  <= internal_o_slave_sda_oe;          // provide hold time on slave SDA relative to slave_scl
                end
                
                // STOP_SSCL_LOW_WAIT_TIMEOUT
                // Sending a stop condition on the slave bus
                // Drive slave SCL low, and drive slave SDA low (after allowing for suitable hold time in the previous state)
                // Clockstretch the master bus if master SCL is driven low, to prevent the master bus from getting 'ahead' of the slave bus
                RELAY_STATE_STOP_SSCL_LOW_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_STOP_SSDA_LOW_WAIT_SSCL_HIGH;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '1;                      // we drive slave SCL low
                    internal_o_slave_sda_oe  <= '1;
                end

                // STOP_SSDA_LOW_WAIT_SSCL_HIGH
                // Allow slave SCL to go high, confirm it has gone high before proceeding
                // Clockstretch on the master bus if master SCL goes low
                RELAY_STATE_STOP_SSDA_LOW_WAIT_SSCL_HIGH: begin
                    if (slave_scl) begin
                        relay_state <= RELAY_STATE_STOP_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '1;                      // hold slave SDA low, it will be released after we leave this state, creating the 'stop' condition on the bus
                end

                // STOP_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT
                // Allow slave SCL to go high, then wait for a timeout before proceeding to next state
                // Clockstretch on the master bus if master SCL goes low
                RELAY_STATE_STOP_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_STOP_SSCL_HIGH_WAIT_SSDA_HIGH;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '1;                      // hold slave SDA low, it will be released after we leave this state, creating the 'stop' condition on the bus
                end
                
                // STOP_SSCL_HIGH_WAIT_SSDA_HIGH
                // Allow slave SDA to go high, wait to observe it high before proceeding to the next state
                // This rising edge on SDA while SCL is high is what creates the 'stop' condition on the bus
                // Clockstretch on the master bus if master SCL goes low
                RELAY_STATE_STOP_SSCL_HIGH_WAIT_SSDA_HIGH: begin
                    if (slave_sda) begin
                        relay_state <= RELAY_STATE_STOP_SSDA_HIGH_SSCL_HIGH_WAIT_TIMEOUT;
                    end
                    else if (slave_sda_hold_count_timeout == 1'b1) begin
                        relay_state <= RELAY_STATE_SET_SSCL_LOW_SSDA_HIGH;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '0;
                end
                //--Setting SCL toggle 
                RELAY_STATE_SET_SSCL_LOW_SSDA_HIGH: begin
                    if (~slave_scl) begin
                        relay_state <= RELAY_STATE_SET_SSCL_LOW_WAIT_TIMEOUT;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '1;                      // we drive slave SCL low
                    internal_o_slave_sda_oe  <= internal_o_slave_sda_oe;
                end
                RELAY_STATE_SET_SSCL_LOW_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_SET_SSDA_LOW_WAIT_SSCL_HIGH;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '1;                      // we drive slave SCL low
                    internal_o_slave_sda_oe  <= '1;
               end
                RELAY_STATE_SET_SSDA_LOW_WAIT_SSCL_HIGH: begin
                    if (slave_scl) begin
                        relay_state <= RELAY_STATE_SET_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '1; 
                end
                RELAY_STATE_SET_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_STOP_SSCL_HIGH_WAIT_SSDA_HIGH;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '1;
                end
                // RELAY_STATE_SET_SSCL_LOW_WAIT_TIMEOUT RELAY_STATE_SET_SSDA_LOW_WAIT_SSCL_HIGH RELAY_STATE_SET_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT 
               //--Setting SCL toggle end.

                // STOP_SSDA_HIGH_SSCL_HIGH_WAIT_TIMEOUT
                // Stop condition has been sent, wait for a timeout before proceeding to next state
                // Clockstretch on the master bus if master SCL goes low
                RELAY_STATE_STOP_SSDA_HIGH_SSCL_HIGH_WAIT_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        relay_state <= RELAY_STATE_STOP_RESET_TIMEOUT_COUNTER;
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '0;
                end

                // STOP_RESET_TIMEOUT_COUNTER
                // We need TWO timeout counters after a stop condition, this state exists just to allow the counter to reset
                RELAY_STATE_STOP_RESET_TIMEOUT_COUNTER: begin
                    relay_state <= RELAY_STATE_STOP_WAIT_SECOND_TIMEOUT;
                    internal_o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '0;
                end

                // STOP_WAIT_SECOND_TIMEOUT
                // Allow slave SCL to go high, then wait for a timeout before proceeding to next state
                // Clockstretch on the master bus if master SCL goes low
                RELAY_STATE_STOP_WAIT_SECOND_TIMEOUT: begin
                    if (slave_scl_hold_count_timeout) begin
                        if (master_smbstate == SMBMASTER_STATE_STOP_THEN_START) begin
                            relay_state <= RELAY_STATE_STOP_SECOND_RESET_TIMEOUT_COUNTER;
                        end else begin    
                            relay_state <= RELAY_STATE_IDLE;
                            latch_mux_state <= '0; //we can reset the mux_latch when we go back to IDLE
                        end
                    end
                    internal_o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '0;
                end

                // STOP_SECOND_RESET_TIMEOUT_COUNTER
                // need to reset the timeout counter before we proceed to the start state, which relies on the timeout counter
                RELAY_STATE_STOP_SECOND_RESET_TIMEOUT_COUNTER: begin
                    relay_state <= RELAY_STATE_START_WAIT_TIMEOUT;
                    internal_o_master_scl_oe <= ~master_scl;             // clockstretch on the master bus if master scl is driven low
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '0;
                end

                
                default: begin
                    relay_state <= RELAY_STATE_IDLE;
                    internal_o_master_scl_oe <= '0;             // clockstretch on the master bus if master scl is driven low
                    internal_o_master_sda_oe <= '0;
                    internal_o_slave_scl_oe  <= '0;
                    internal_o_slave_sda_oe  <= '0;
                end

            endcase
            
            // capture data from the slave bus on clock rising edge
            if (slave_scl_posedge) begin
                slave_captured_sda <= slave_sda;
            end
            
            // counter to determine how long to hold slave_scl low or high 
            // used when creating 'artificial' scl clock pulses on slave while clock stretching the master during ack and read data phases
            // counter counts from some positive value down to -1, thus checking the highest bit (sign bit) is adequate to determine if the count is 'done'
            if (slave_scl_hold_count_restart) begin
                slave_scl_hold_count <= SCLK_HOLD_MIN_COUNT[SCLK_HOLD_COUNTER_BIT_WIDTH-1:0] - {{SCLK_HOLD_COUNTER_BIT_WIDTH-1{1'b0}}, 1'b1}; 
            end else if (!slave_scl_hold_count[SCLK_HOLD_COUNTER_BIT_WIDTH-1]) begin        // count down to -1 (first time msb goes high) and then stop
                slave_scl_hold_count <= slave_scl_hold_count - {{SCLK_HOLD_COUNTER_BIT_WIDTH-1{1'b0}}, 1'b1};
            end
            if (slave_sda_hold_count_restart) begin
                slave_sda_hold_count <= SCLK_HOLD_MIN_COUNT[SCLK_HOLD_COUNTER_BIT_WIDTH-1:0] - {{SCLK_HOLD_COUNTER_BIT_WIDTH-1{1'b0}}, 1'b1}; 
            end else if (!slave_sda_hold_count[SCLK_HOLD_COUNTER_BIT_WIDTH-1]) begin        // count down to -1 (first time msb goes high) and then stop
                slave_sda_hold_count <= slave_sda_hold_count - {{SCLK_HOLD_COUNTER_BIT_WIDTH-1{1'b0}}, 1'b1};
            end
            
            // counter to determine how long was the master scl must be held low (clock stretched) to create timeout conditon for a losing master
            // used when arbitration lost occurs for a race condition for a READ bit, after the address + READ bit is ACK-ed, the master SCL clock is stretched till timeout occurs to stop the master. 
            if (master_scl_low_count_restart) begin
                master_scl_low_count <= SCLK_LOW_TIMEOUT_COUNT[SCLK_LOW_TIMEOUT_COUNTER_BIT_WIDTH-1:0] - {{SCLK_LOW_TIMEOUT_COUNTER_BIT_WIDTH-1{1'b0}}, 1'b1}; 
            end else if (!master_scl_low_count[SCLK_LOW_TIMEOUT_COUNTER_BIT_WIDTH-1]) begin        // count down to -1 (first time msb goes high) and then stop
                master_scl_low_count <= master_scl_low_count - {{SCLK_LOW_TIMEOUT_COUNTER_BIT_WIDTH-1{1'b0}}, 1'b1};
            end
            
            // counter to determine if 9 master scl was achieved according to the bus speed
            // used when master tries to start when slave busy. Ensures master SDA was held low to make master knows it lost arbitration for at least 9 master scl.  
            if (master_nine_scl_count_restart) begin
                master_nine_scl_count <= NINE_SCLK_COUNT[NINE_SCLK_COUNTER_BIT_WIDTH-1:0] - {{NINE_SCLK_COUNTER_BIT_WIDTH-1{1'b0}}, 1'b1}; 
            end else if (!master_nine_scl_count[NINE_SCLK_COUNTER_BIT_WIDTH-1]) begin        // count down to -1 (first time msb goes high) and then stop
                master_nine_scl_count <= master_nine_scl_count - {{NINE_SCLK_COUNTER_BIT_WIDTH-1{1'b0}}, 1'b1};
            end
        end
        
    end

    // SCLK_HOLD_MIN_COUNT
    // when the msb of slave_scl_hold_count = 1, that indicates a negative number, which means the timeout has occurred
    assign slave_scl_hold_count_timeout = slave_scl_hold_count[SCLK_HOLD_COUNTER_BIT_WIDTH-1];
    assign slave_sda_hold_count_timeout = slave_sda_hold_count[SCLK_HOLD_COUNTER_BIT_WIDTH-1];
    
    // determine when to reset the counter based on the current relay state
    // create this signal with combinatorial logic so counter will be reset as we enter the next state
    // we never have two states in a row that both require this counter
    // we only start scl hold time count during RELAY_STATE_*_WAIT_TIMEOUT stages
    assign slave_sda_hold_count_restart = (relay_state == RELAY_STATE_STOP_SSCL_HIGH_WAIT_SSDA_HIGH) ? '0 : '1;
    assign slave_scl_hold_count_restart = ( (relay_state == RELAY_STATE_START_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_MTOS_SSCL_LOW_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_MTOS_SSCL_HIGH_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_STOM_SSCL_LOW_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_STOM_SSCL_HIGH_WAIT_MSCL_HIGH) ||
                                            (relay_state == RELAY_STATE_STOP_SSCL_LOW_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_STOP_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_SET_SSCL_LOW_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_SET_SSDA_LOW_SSCL_HIGH_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_STOP_SSDA_HIGH_SSCL_HIGH_WAIT_TIMEOUT) ||
                                            (relay_state == RELAY_STATE_STOP_WAIT_SECOND_TIMEOUT)
                                          ) ? '0 : '1;
    
    // SCLK_LOW_TIMEOUT_COUNT
    // when the msb of master_scl_low_count = 1, that indicates a negative number, which means the timeout has occurred
    assign master_scl_low_count_timeout = master_scl_low_count[SCLK_LOW_TIMEOUT_COUNTER_BIT_WIDTH-1];
    
    // we only start counting for master_scl low timeout if the master has lost arbitration at read phase
    assign master_scl_low_count_restart = ( (relay_state == RELAY_STATE_READ_PHASE_ARBITRATION_LOST) ) ? '0 : '1;
    
    // NINE_SCLK_COUNT
    // when the msb of master_nine_scl_count = 1, that indicates a negative number, which means the timeout has occurred
    assign master_nine_scl_count_timeout = master_nine_scl_count[NINE_SCLK_COUNTER_BIT_WIDTH-1];
    
    // we only start counting for 9 master_scl appearances if master has start when slave is busy
    assign master_nine_scl_count_restart = ( (relay_state == RELAY_STATE_ARBITRATION_LOST_RETRY) ) ? '0 : '1;
                                      
    ///////////////////////////////////////
    // Instantiate an M9K block in simple dual port mode to store command enable whitelist information
    // The memory is only instantiated when filtering is enabled
    ///////////////////////////////////////
    generate 
        if (FILTER_ENABLE) begin    : gen_filter_enabled
            logic [7:0] command;    // current command coming from the Master SMBus.  The first byte after the I2C Address is the 'command' byte
            
				// capture the incoming command 
            always_ff @(posedge clock or negedge i_resetn) begin
                if (!i_resetn) begin
                    command <= 8'h00;
                end else begin
                    if ( (master_smbstate == SMBMASTER_STATE_MASTER_CMD) && (master_bit_count == 4'h8)  ) begin
                        command <= master_byte_in;
                    end else if (master_smbstate == SMBMASTER_STATE_IDLE) begin            // reset the command each time we return to the idle state
                        command <= 8'h00;
                    end
                end
            end

            altsyncram #(
                .address_aclr_b                     ( "NONE"                   ),
                .address_reg_b                      ( "CLOCK0"                 ),
                //.clock_enable_input_a               ( "BYPASS"                 ), // these 'BYPASS' settings are causing problems in simulation, should be fine with default values as all clken ports are tied-off to 1
                //.clock_enable_input_b               ( "BYPASS"                 ),
                //.clock_enable_output_b              ( "BYPASS"                 ),
                //.intended_device_family            .( "MAX 10"                 ),  // this setting shouldn't be needed, will come from the project - leaving it out makes this more generic
                .lpm_type                           ( "altsyncram"             ),
                .numwords_a                         ( 256                      ),
                .numwords_b                         ( 256*32                   ),   // output is 1 bit wide, so 32x as many words on port b
                .operation_mode                     ( "DUAL_PORT"              ),
                .outdata_aclr_b                     ( "NONE"                   ),
                .outdata_reg_b                      ( "CLOCK0"                 ),   // set to 'UNREGISTERED' for no output reg, 'CLOCK0' to use the output reg
                .power_up_uninitialized             ( "FALSE"                  ),
                .ram_block_type                     ( "AUTO"                   ),   // was set to 'M9K', changing to 'AUTO' to prevent any family incompatibility issues in the future
                .read_during_write_mode_mixed_ports ( "DONT_CARE"              ),
                .widthad_a                          ( 8                        ),
                .widthad_b                          ( 8+5                      ),   // output is only 1 bit wide, so 5 extra address bits are required
                .width_a                            ( 32                       ),
                .width_b                            ( 1                        ),
                .width_byteena_a                    ( 1                        )
            ) cmd_enable_mem (		
                .address_a      ( i_avmm_address            ),
                .address_b      ( {address_match_index[4:0],command[7:0]} ),        // msbs select which I2C device is being addressed, lsbs select the individual command
                .clock0         ( clock                     ),
                .data_a         ( i_avmm_writedata          ),
                .data_b         ( 1'b0                      ),
                .wren_a         ( i_avmm_write              ),
                .wren_b         ( 1'b0                      ),
                .q_a            (                           ),
                .q_b            ( command_whitelisted       ),
                .aclr0          ( 1'b0                      ),
                .aclr1          ( 1'b0                      ),
                .addressstall_a ( 1'b0                      ),
                .addressstall_b ( 1'b0                      ),
                .byteena_a      ( 1'b1                      ),
                .byteena_b      ( 1'b1                      ),
                .clock1         ( 1'b1                      ),
                .clocken0       ( 1'b1                      ),
                .clocken1       ( 1'b1                      ),
                .clocken2       ( 1'b1                      ),
                .clocken3       ( 1'b1                      ),
                .eccstatus      (                           ),
                .rden_a         ( 1'b1                      ),
                .rden_b         ( 1'b1                      )
            );
        end else begin              : gen_filter_disabled
            assign command_whitelisted = '1;           
        end
    endgenerate

endmodule





        
                
                
        
        

    
