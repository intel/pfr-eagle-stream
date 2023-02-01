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


// non_pfr_core
//
// This module implements the toplevel of the Non-PFR IP. It instantiates
// SMBus relays but otherwise drives constants for signals.
//

`timescale 1 ps / 1 ps
`default_nettype none

module non_pfr_core (
    // Clocks and resets
    input wire clk2M,
    input wire clk50M,
    input wire sys_clk,
    input wire spi_clk,
    input wire clk2M_reset_sync_n,
    input wire clk50M_reset_sync_n,
    input wire sys_clk_reset_sync_n,
    input wire spi_clk_reset_sync_n,
    
    // BMC and PCH resets from common core
    input wire cc_RST_RSMRST_PLD_R_N,
    input wire cc_RST_SRST_BMC_PLD_R_N,
    output wire cc_RST_PLTRST_PLD_N,
    
    // LED Control from the common core
    input wire ccLED_CONTROL_0,
    input wire ccLED_CONTROL_1,
    input wire ccLED_CONTROL_2,
    input wire ccLED_CONTROL_3,
    input wire ccLED_CONTROL_4,
    input wire ccLED_CONTROL_5,
    input wire ccLED_CONTROL_6,
    input wire ccLED_CONTROL_7,
    input wire ccFM_POST_7SEG1_SEL_N,
    input wire ccFM_POST_7SEG2_SEL_N,
    input wire ccFM_POSTLED_SEL,

    // HPFR signal
    input wire ccHPFR_IN,
    output wire ccHPFR_OUT,
    input wire ccLEGACY,
    input wire ccHPFR_ACTIVE,

    // LED Output 
    output logic LED_CONTROL_0,
    output logic LED_CONTROL_1,
    output logic LED_CONTROL_2,
    output logic LED_CONTROL_3,
    output logic LED_CONTROL_4,
    output logic LED_CONTROL_5,
    output logic LED_CONTROL_6,
    output logic LED_CONTROL_7,
    output wire FM_POST_7SEG1_SEL_N,
    output wire FM_POST_7SEG2_SEL_N,
    output wire FM_POSTLED_SEL,

    output wire FAN_BMC_PWM_R,
    input wire FM_ME_AUTHN_FAIL,
    input wire FM_ME_BT_DONE,
    output wire FM_PFR_DSW_PWROK_N,
    input wire FM_PFR_FORCE_RECOVERY_N,
    output wire FM_SPI_PFR_BMC_BT_MASTER_SEL_R,
    output wire FM_SPI_PFR_PCH_MASTER_SEL_R,
    output wire FM_PFR_ON_R,
    input wire FM_PFR_POSTCODE_SEL_N,
    output wire FM_PFR_RNDGEN_AUX,
    input wire FM_PFR_TM1_HOLD_N,
    input wire FP_ID_LED_N,
    output wire FP_ID_LED_PFR_N,
    input wire FP_LED_STATUS_AMBER_N,
    output wire FP_LED_STATUS_AMBER_PFR_N,
    input wire FP_LED_STATUS_GREEN_N,
    output wire FP_LED_STATUS_GREEN_PFR_N,
    output wire H_S3M_CPU0_SMBUS_ALERT_LVC1_R_N,
    output wire H_S3M_CPU1_SMBUS_ALERT_LVC1_R_N,
    output wire RST_PFR_EXTRST_R_N,
    output wire RST_PFR_OVR_RTC_R,
    output wire RST_PFR_OVR_SRTC_R,
    output wire RST_SPI_PFR_BMC_BOOT_N,
    output wire RST_SPI_PFR_PCH_N,
    inout wire SMB_BMC_HSBP_STBY_LVC3_SCL,
    inout wire SMB_BMC_HSBP_STBY_LVC3_SDA,
    inout wire SMB_PCH_PMBUS2_STBY_LVC3_SCL,
    inout wire SMB_PCH_PMBUS2_STBY_LVC3_SDA,
    inout wire SMB_PFR_HSBP_STBY_LVC3_SCL,
    inout wire SMB_PFR_HSBP_STBY_LVC3_SDA,
    inout wire SMB_PFR_PMB1_STBY_LVC3_SCL,
    inout wire SMB_PFR_PMB1_STBY_LVC3_SDA,
    inout wire SMB_PFR_PMBUS2_STBY_LVC3_R_SCL,
    inout wire SMB_PFR_PMBUS2_STBY_LVC3_R_SDA,
    input wire SMB_PCIE_STBY_LVC3_B_SCL,
    inout wire SMB_PCIE_STBY_LVC3_B_SDA,
    output wire SMB_PFR_RFID_STBY_LVC3_SCL,
    inout wire SMB_PFR_RFID_STBY_LVC3_SDA,
    inout wire SMB_PMBUS_SML1_STBY_LVC3_SCL,
    inout wire SMB_PMBUS_SML1_STBY_LVC3_SDA,
    input wire SMB_S3M_CPU0_SCL_LVC1,
    inout wire SMB_S3M_CPU0_SDA_LVC1,
    // input wire SMB_S3M_CPU1_SCL_LVC1,
    // inout wire SMB_S3M_CPU1_SDA_LVC1,
    input wire SPI_BMC_BOOT_CS_N,
    input wire SPI_BMC_BT_MUXED_MON_CLK,
    input wire SPI_BMC_BT_MUXED_MON_MISO,
    output wire SPI_BMC_BT_MUXED_MON_MOSI,
    input wire SPI_BMC_BT_MUXED_MON_IO2,
    input wire SPI_BMC_BT_MUXED_MON_IO3,
    input wire SPI_PCH_MUXED_MON_CLK,
    inout wire SPI_PCH_MUXED_MON_IO0,
    inout wire SPI_PCH_MUXED_MON_IO1,
    inout wire SPI_PCH_MUXED_MON_IO2,
    inout wire SPI_PCH_MUXED_MON_IO3,
    input wire SPI_PCH_PFR_CS0_N,
    input wire SPI_PCH_TPM_CS_N,
    input wire SPI_PCH_CS1_N,
    output wire SPI_PFR_PCH_SECURE_CS0_R_N,
    inout wire SPI_PFR_BMC_BOOT_R_IO2,
    inout wire SPI_PFR_BMC_BOOT_R_IO3,
    output wire SPI_PFR_BMC_BT_SECURE_CS_R_N,
    output wire SPI_PFR_BMC_FLASH1_BT_CLK,
    input wire SPI_PFR_BMC_FLASH1_BT_MISO,
    output wire SPI_PFR_BMC_FLASH1_BT_MOSI,
    output wire SPI_PFR_PCH_R_CLK,
    output wire SPI_PFR_TPM_CS_R2_N,
    inout wire SPI_PFR_PCH_R_IO0,
    inout wire SPI_PFR_PCH_R_IO1,
    inout wire SPI_PFR_PCH_R_IO2,
    inout wire SPI_PFR_PCH_R_IO3,
    input wire RST_PLTRST_PLD_N,
    output wire RST_SRST_BMC_PLD_R_N,
    output wire RST_RSMRST_PLD_R_N,
    output wire SPI_PFR_PCH_SECURE_CS1_N,
    output wire FM_PFR_SLP_SUS_EN_R_N
    
);

    ///////////////////////////////////////////////////////////////////////////
    // Continuous assignments
    ///////////////////////////////////////////////////////////////////////////

    // Resets to BMC/PCH come from common core
    assign RST_SRST_BMC_PLD_R_N = cc_RST_SRST_BMC_PLD_R_N;
    assign RST_RSMRST_PLD_R_N = cc_RST_RSMRST_PLD_R_N;
    assign cc_RST_PLTRST_PLD_N = RST_PLTRST_PLD_N;

    ////////////////////////////////////////////////////////////////////////////////// //
    //PFR By pass logic   - Imported from common core. NEED TO ORGANIZE AND DEFINE
    ////////////////////////////////////////////////////////////////////////////////// //
    assign ccHPFR_OUT = 'Z;
    
    assign FAN_BMC_PWM_R = 'Z;
    
    assign FM_PFR_DSW_PWROK_N = 'Z; 
    assign FM_PFR_ON_R = 'Z;
    assign FM_PFR_RNDGEN_AUX = 'Z;

    assign H_S3M_CPU0_SMBUS_ALERT_LVC1_R_N = 'Z;
    assign H_S3M_CPU1_SMBUS_ALERT_LVC1_R_N = 'Z;

    assign RST_PFR_OVR_RTC_R = 'Z; 
    assign RST_PFR_OVR_SRTC_R = 'Z; 
    
    assign SMB_PCIE_STBY_LVC3_B_SDA = 'Z;
    
    assign RST_PFR_EXTRST_R_N = 1'b1;     

    assign FP_ID_LED_PFR_N = FP_ID_LED_N;
    assign FP_LED_STATUS_AMBER_PFR_N = FP_LED_STATUS_AMBER_N;
    assign FP_LED_STATUS_GREEN_PFR_N = FP_LED_STATUS_GREEN_N;
    
    
    // SPI 
    
    assign FM_SPI_PFR_PCH_MASTER_SEL_R = 1'b0;
    assign FM_SPI_PFR_BMC_BT_MASTER_SEL_R = 1'b0; 
    
    assign RST_SPI_PFR_BMC_BOOT_N = 1'b1;
    assign RST_SPI_PFR_PCH_N = 1'b1;

    assign SPI_PFR_BMC_BT_SECURE_CS_R_N = SPI_BMC_BOOT_CS_N;
    assign SPI_PFR_PCH_SECURE_CS0_R_N = SPI_PCH_PFR_CS0_N;
    assign SPI_PFR_PCH_SECURE_CS1_N = SPI_PCH_CS1_N;
//    assign SPI_PFR_BMC_FLASH1_BT_CLK = SPI_BMC_BT_MUXED_MON_CLK;
//    assign  SPI_PFR_PCH_R_CLK               = SPI_PCH_MUXED_MON_CLK;

    assign SPI_PFR_TPM_CS_R2_N = SPI_PCH_TPM_CS_N;
    
    assign SPI_BMC_BT_MUXED_MON_MOSI = 'Z; 
    
    assign SPI_PCH_MUXED_MON_IO0 = 'Z; 
    assign SPI_PCH_MUXED_MON_IO1 = 'Z; 
    assign SPI_PCH_MUXED_MON_IO2 = 'Z; 
    assign SPI_PCH_MUXED_MON_IO3 = 'Z; 
    
    assign SPI_PFR_BMC_FLASH1_BT_MOSI = 'Z; 
    assign SPI_PFR_BMC_BOOT_R_IO2 = 'Z; 
    assign SPI_PFR_BMC_BOOT_R_IO3 = 'Z; 
    
    assign SPI_PFR_PCH_R_CLK = 'Z; 
    assign SPI_PFR_PCH_R_IO0 = 'Z; 
    assign SPI_PFR_PCH_R_IO1 = 'Z; 
    assign SPI_PFR_PCH_R_IO2 = 'Z; 
    assign SPI_PFR_PCH_R_IO3 = 'Z;

    assign SMB_PFR_RFID_STBY_LVC3_SCL = 'Z;
    assign SMB_PFR_RFID_STBY_LVC3_SDA = 'Z;
    
    assign FM_PFR_SLP_SUS_EN_R_N = 1'b0;

    //#########################################################################
 
    // No T-1 signaling. Resets come from common core
    assign RST_SRST_BMC_PLD_R_N = cc_RST_SRST_BMC_PLD_R_N;
    assign RST_RSMRST_PLD_R_N = cc_RST_RSMRST_PLD_R_N;
    //#########################################################################
    // SMBus Relays
    //#########################################################################

    
    ///////////////////////////////
    // Relay 1:
    // SMB_PMBUS_SML1_STBY_LVC3_SCL => SMB_PFR_PMB1_STBY_LVC3_SCL
    // SMB_PMBUS_SML1_STBY_LVC3_SDA => SMB_PFR_PMB1_STBY_LVC3_SDA

    // logic to implement the open-drain pins for scl/sda on the slave and master busses
    logic smbus_relay1_master_scl_in;
    logic smbus_relay1_master_scl_oe;
    logic smbus_relay1_master_sda_in;
    logic smbus_relay1_master_sda_oe;
    logic smbus_relay1_slave_scl_in ;
    logic smbus_relay1_slave_scl_oe ;
    logic smbus_relay1_slave_sda_in ;
    logic smbus_relay1_slave_sda_oe ;
    assign smbus_relay1_master_scl_in = SMB_PMBUS_SML1_STBY_LVC3_SCL;
    assign smbus_relay1_master_sda_in = SMB_PMBUS_SML1_STBY_LVC3_SDA;
    assign smbus_relay1_slave_scl_in  = SMB_PFR_PMB1_STBY_LVC3_SCL;
    assign smbus_relay1_slave_sda_in  = SMB_PFR_PMB1_STBY_LVC3_SDA;
    assign SMB_PMBUS_SML1_STBY_LVC3_SCL = smbus_relay1_master_scl_oe ? 1'b0 : 1'bz;
    assign SMB_PMBUS_SML1_STBY_LVC3_SDA = smbus_relay1_master_sda_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_PMB1_STBY_LVC3_SCL   = smbus_relay1_slave_scl_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_PMB1_STBY_LVC3_SDA   = smbus_relay1_slave_sda_oe ? 1'b0 : 1'bz;
    
    // Instantaite the relay block with no address or command filtering enabled
    smbus_filtered_relay #(
        .FILTER_ENABLE      ( 0                                                 ),      // do not perform command filtering
        .CLOCK_PERIOD_PS    ( platform_defs_pkg::SYS_CLOCK_PERIOD_PS            ),
        .BUS_SPEED_KHZ      ( platform_defs_pkg::RELAY1_BUS_SPEED_KHZ           ),
        .NUM_RELAY_ADDRESSES( gen_smbus_relay_config_pkg::RELAY1_NUM_ADDRESSES  ),
        .SMBUS_RELAY_ADDRESS( gen_smbus_relay_config_pkg::RELAY1_I2C_ADDRESSES  )
    ) u_smbus_filtered_relay_1 (
        .clock                 ( sys_clk                    ),
        .i_resetn              ( sys_clk_reset_sync_n       ),
        .i_block_disable       ( '0                         ),
        .i_filter_disable      ( '0                         ),
        .i_relay_all_addresses ( '1                         ),
        .ia_master_scl         ( smbus_relay1_master_scl_in ),
        .o_master_scl_oe       ( smbus_relay1_master_scl_oe ),
        .ia_master_sda         ( smbus_relay1_master_sda_in ),
        .o_master_sda_oe       ( smbus_relay1_master_sda_oe ),
        .ia_slave_scl          ( smbus_relay1_slave_scl_in  ),
        .o_slave_scl_oe        ( smbus_relay1_slave_scl_oe  ),
        .ia_slave_sda          ( smbus_relay1_slave_sda_in  ),
        .o_slave_sda_oe        ( smbus_relay1_slave_sda_oe  ),
        .i_avmm_write          ( '0                         ),
        .i_avmm_address        ( '0                         ),
        .i_avmm_writedata      ( '0                         )
    );
    

    ///////////////////////////////
    // Relay 2:
    // SMB_PFR_PMBUS2_STBY_LVC3_R_SCL => SMB_PCH_PMBUS2_STBY_LVC3_SCL
    // SMB_PFR_PMBUS2_STBY_LVC3_R_SDA => SMB_PCH_PMBUS2_STBY_LVC3_SDA
    // Note the Master SMBus pins are shared with the interface to the Common Core register file

    // logic to implement the open-drain pins for scl/sda on the slave and master busses
    logic smbus_relay2_master_scl_in;
    logic smbus_relay2_master_scl_oe;
    logic smbus_relay2_master_sda_in;
    logic smbus_relay2_master_sda_oe;
    logic smbus_relay2_slave_scl_in ;
    logic smbus_relay2_slave_scl_oe ;
    logic smbus_relay2_slave_sda_in ;
    logic smbus_relay2_slave_sda_oe ;
    assign smbus_relay2_master_scl_in = SMB_PCH_PMBUS2_STBY_LVC3_SCL;
    assign smbus_relay2_master_sda_in = SMB_PCH_PMBUS2_STBY_LVC3_SDA;
    assign smbus_relay2_slave_scl_in  = SMB_PFR_PMBUS2_STBY_LVC3_R_SCL;
    assign smbus_relay2_slave_sda_in  = SMB_PFR_PMBUS2_STBY_LVC3_R_SDA;
    assign SMB_PCH_PMBUS2_STBY_LVC3_SCL = smbus_relay2_master_scl_oe ? 1'b0 : 1'bz;
    assign SMB_PCH_PMBUS2_STBY_LVC3_SDA = smbus_relay2_master_sda_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_PMBUS2_STBY_LVC3_R_SCL = smbus_relay2_slave_scl_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_PMBUS2_STBY_LVC3_R_SDA = smbus_relay2_slave_sda_oe ? 1'b0 : 1'bz;
    
    // Instantaite the relay block with no address or command filtering enabled
    smbus_filtered_relay #(
        .FILTER_ENABLE      ( 0                                                 ),      // do not perform command filtering
        .CLOCK_PERIOD_PS    ( platform_defs_pkg::SYS_CLOCK_PERIOD_PS            ),
        .BUS_SPEED_KHZ      ( platform_defs_pkg::RELAY2_BUS_SPEED_KHZ           ),
        .NUM_RELAY_ADDRESSES( gen_smbus_relay_config_pkg::RELAY2_NUM_ADDRESSES  ),
        .SMBUS_RELAY_ADDRESS( gen_smbus_relay_config_pkg::RELAY2_I2C_ADDRESSES  )
    ) u_smbus_filtered_relay_2 (
        .clock                 ( sys_clk                    ),
        .i_resetn              ( sys_clk_reset_sync_n       ),
        .i_block_disable       ( '0                         ),
        .i_filter_disable      ( '0                         ),
        .i_relay_all_addresses ( '1                         ),
        .ia_master_scl         ( smbus_relay2_master_scl_in ),
        .o_master_scl_oe       ( smbus_relay2_master_scl_oe ),
        .ia_master_sda         ( smbus_relay2_master_sda_in ),
        .o_master_sda_oe       ( smbus_relay2_master_sda_oe ),
        .ia_slave_scl          ( smbus_relay2_slave_scl_in  ),
        .o_slave_scl_oe        ( smbus_relay2_slave_scl_oe  ),
        .ia_slave_sda          ( smbus_relay2_slave_sda_in  ),
        .o_slave_sda_oe        ( smbus_relay2_slave_sda_oe  ),
        .i_avmm_write          ( '0                         ),
        .i_avmm_address        ( '0                         ),
        .i_avmm_writedata      ( '0                         )
    );

    ///////////////////////////////
    // Relay 3:
    //SMB_PFR_HSBP_STBY_LVC3_SCL => SMB_BMC_HSBP_STBY_LVC3_SCL
    //SMB_PFR_HSBP_STBY_LVC3_SDA => SMB_BMC_HSBP_STBY_LVC3_SDA

    // logic to implement the open-drain pins for scl/sda on the slave and master busses
    logic smbus_relay3_master_scl_in;
    logic smbus_relay3_master_scl_oe;
    logic smbus_relay3_master_sda_in;
    logic smbus_relay3_master_sda_oe;
    logic smbus_relay3_slave_scl_in ;
    logic smbus_relay3_slave_scl_oe ;
    logic smbus_relay3_slave_sda_in ;
    logic smbus_relay3_slave_sda_oe ;
    assign smbus_relay3_master_scl_in = SMB_BMC_HSBP_STBY_LVC3_SCL;
    assign smbus_relay3_master_sda_in = SMB_BMC_HSBP_STBY_LVC3_SDA;
    assign smbus_relay3_slave_scl_in  = SMB_PFR_HSBP_STBY_LVC3_SCL;
    assign smbus_relay3_slave_sda_in  = SMB_PFR_HSBP_STBY_LVC3_SDA;
    assign SMB_BMC_HSBP_STBY_LVC3_SCL = smbus_relay3_master_scl_oe ? 1'b0 : 1'bz;
    assign SMB_BMC_HSBP_STBY_LVC3_SDA = smbus_relay3_master_sda_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_HSBP_STBY_LVC3_SCL = smbus_relay3_slave_scl_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_HSBP_STBY_LVC3_SDA   = smbus_relay3_slave_sda_oe ? 1'b0 : 1'bz;
    
    // Instantaite the relay block with no address or command filtering enabled
    smbus_filtered_relay #(
        .FILTER_ENABLE      ( 0                                                 ),      // do not perform command filtering
        .CLOCK_PERIOD_PS    ( platform_defs_pkg::SYS_CLOCK_PERIOD_PS            ),
        .BUS_SPEED_KHZ      ( platform_defs_pkg::RELAY2_BUS_SPEED_KHZ           ),
        .NUM_RELAY_ADDRESSES( gen_smbus_relay_config_pkg::RELAY2_NUM_ADDRESSES  ),
        .SMBUS_RELAY_ADDRESS( gen_smbus_relay_config_pkg::RELAY2_I2C_ADDRESSES  )
    ) u_smbus_filtered_relay_3 (
        .clock                 ( sys_clk                    ),
        .i_resetn              ( sys_clk_reset_sync_n       ),
        .i_block_disable       ( '0                         ),
        .i_filter_disable      ( '0                         ),
        .i_relay_all_addresses ( '1                         ),
        .ia_master_scl         ( smbus_relay3_master_scl_in ),
        .o_master_scl_oe       ( smbus_relay3_master_scl_oe ),
        .ia_master_sda         ( smbus_relay3_master_sda_in ),
        .o_master_sda_oe       ( smbus_relay3_master_sda_oe ),
        .ia_slave_scl          ( smbus_relay3_slave_scl_in  ),
        .o_slave_scl_oe        ( smbus_relay3_slave_scl_oe  ),
        .ia_slave_sda          ( smbus_relay3_slave_sda_in  ),
        .o_slave_sda_oe        ( smbus_relay3_slave_sda_oe  ),
        .i_avmm_write          ( '0                         ),
        .i_avmm_address        ( '0                         ),
        .i_avmm_writedata      ( '0                         )
    );


    //#########################################################################
    // LED Control
    //#########################################################################

    // For now hard code the global state
    wire [3:0] global_state;
    assign global_state = 4'd12;


    logic [6:0] pfr_seven_seg;
    always_comb begin
        if (!sys_clk_reset_sync_n)
           pfr_seven_seg = 7'b1000000;
        else
            case (global_state[3:0])
                7'd0:  pfr_seven_seg = 7'b1000000; //0 
                7'd1:  pfr_seven_seg = 7'b1111001; //1 
                7'd2:  pfr_seven_seg = 7'b0100100; //2
                7'd3:  pfr_seven_seg = 7'b0110000; //3 
                7'd4:  pfr_seven_seg = 7'b0011001; //4
                7'd5:  pfr_seven_seg = 7'b0010010; //5 .
                7'd6:  pfr_seven_seg = 7'b0000010; //6 
                7'd7:  pfr_seven_seg = 7'b1111000; //7 
                7'd8:  pfr_seven_seg = 7'b0000000; //8 
                7'd9:  pfr_seven_seg = 7'b0011000; //9 
                7'd10: pfr_seven_seg = 7'b0001000; //A 
                7'd11: pfr_seven_seg = 7'b0000011; //B 
                7'd12: pfr_seven_seg = 7'b1000110; //C 
                7'd13: pfr_seven_seg = 7'b0100001; //D 
                7'd14: pfr_seven_seg = 7'b0000110; //E  
                7'd15: pfr_seven_seg = 7'b0001110; //F 
                default: 
                       pfr_seven_seg = 7'b1000000; //0 
            endcase
    end 

    always_comb begin
        // Defaults
        LED_CONTROL_0 = ccLED_CONTROL_0;
        LED_CONTROL_1 = ccLED_CONTROL_1;
        LED_CONTROL_2 = ccLED_CONTROL_2;
        LED_CONTROL_3 = ccLED_CONTROL_3;
        LED_CONTROL_4 = ccLED_CONTROL_4;
        LED_CONTROL_5 = ccLED_CONTROL_5;
        LED_CONTROL_6 = ccLED_CONTROL_6;
        LED_CONTROL_7 = ccLED_CONTROL_7;

        // Is CPLD Debug enabled?
        if (/*!FM_PFR_DEBUG_MODE_N*/1'b1) begin
            if (!ccFM_POST_7SEG1_SEL_N) begin
                // Display on the first 7-seg
                LED_CONTROL_0 = pfr_seven_seg[0];
                LED_CONTROL_1 = pfr_seven_seg[1];
                LED_CONTROL_2 = pfr_seven_seg[2];
                LED_CONTROL_3 = pfr_seven_seg[3];
                LED_CONTROL_4 = pfr_seven_seg[4];
                LED_CONTROL_5 = pfr_seven_seg[5];
                LED_CONTROL_6 = pfr_seven_seg[6];
                // Decimal point
                LED_CONTROL_7 = 1'b0;
            end
            else if (!ccFM_POST_7SEG2_SEL_N) begin
                // Display on the second 7-seg
                LED_CONTROL_0 = pfr_seven_seg[0];
                LED_CONTROL_1 = pfr_seven_seg[1];
                LED_CONTROL_2 = pfr_seven_seg[2];
                LED_CONTROL_3 = pfr_seven_seg[3];
                LED_CONTROL_4 = pfr_seven_seg[4];
                LED_CONTROL_5 = pfr_seven_seg[5];
                LED_CONTROL_6 = pfr_seven_seg[6];
                // Decimal point
                LED_CONTROL_7 = 1'b0;
            end
            else if (ccFM_POSTLED_SEL) begin
                // Display on the post code LEDs
                LED_CONTROL_0 = 1'b0; // Green
                LED_CONTROL_1 = 1'b0; // Green
                LED_CONTROL_2 = 1'b0; // Green
                LED_CONTROL_3 = 1'b0; // Green 
                LED_CONTROL_4 = 1'b1; // Amber
                LED_CONTROL_5 = 1'b1; // Amber
                LED_CONTROL_6 = 1'b1; // Amber
                LED_CONTROL_7 = 1'b1; // Amber 
            end
        end
    end
    
    // Pass through the control signals
    assign FM_POST_7SEG1_SEL_N = ccFM_POST_7SEG1_SEL_N;
    assign FM_POST_7SEG2_SEL_N = ccFM_POST_7SEG2_SEL_N;
    assign FM_POSTLED_SEL = ccFM_POSTLED_SEL;

    // Instantiate the dual configuration IP, but with the AVMM interface tied off.
    // This allows the generation of the non_pfr design for single or dual configuration
    // mode as Quartus checks to ensure the dual config atom exists when compiling for
    // dual configuration mode.
    dual_config_ip u_dual_config (
        .clk                (sys_clk),
        .nreset             (sys_clk_reset_sync_n),
        .avmm_rcv_address   (3'b0),
        .avmm_rcv_read      (1'b0),
        .avmm_rcv_writedata (32'b0),
        .avmm_rcv_write     (1'b0),
        .avmm_rcv_readdata  ()
    );

endmodule
    
