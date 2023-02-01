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


// pfr_core
//
// This module implements the toplevel of the PFR IP. It instantiates the
// Nios II system along with all of the other toplevel IPs such as SMBus
// relay, SPI filter, etc.
//

`timescale 1 ps / 1 ps
`default_nettype none

module pfr_core (
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
    
    // HPFR Signals
    input wire ccHPFR_IN,
    output wire ccHPFR_OUT,
    input wire ccLEGACY,
    input wire ccHPFR_ACTIVE, 
    
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
    inout wire SMB_PCIE_STBY_LVC3_B_SCL,
    inout wire SMB_PCIE_STBY_LVC3_B_SDA,
    output wire SMB_PFR_RFID_STBY_LVC3_SCL,
    inout wire SMB_PFR_RFID_STBY_LVC3_SDA,
    inout wire SMB_PMBUS_SML1_STBY_LVC3_SCL,
    inout wire SMB_PMBUS_SML1_STBY_LVC3_SDA,
    input wire SMB_S3M_CPU0_SCL_LVC1,
    inout wire SMB_S3M_CPU0_SDA_LVC1,
    //inout wire SMB_S3M_CPU1_SCL_LVC1,
    //inout wire SMB_S3M_CPU1_SDA_LVC1,
    input wire SPI_BMC_BOOT_CS_N,
    input wire SPI_BMC_BT_MUXED_MON_CLK,
    input wire SPI_BMC_BT_MUXED_MON_MISO,
    input wire SPI_BMC_BT_MUXED_MON_MOSI,
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
    inout wire SPI_PFR_BMC_FLASH1_BT_MISO,
    inout wire SPI_PFR_BMC_FLASH1_BT_MOSI,
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

    // Import the GPI names
    import gen_gpi_signals_pkg::*;

    //Import the platform defs names
    import platform_defs_pkg::*;
    
    // Nios general purpose outputs
    wire [31:0] gpo_1;
    
    // Nios general purpose inputs
    wire [31:0] gpi_1;

    // Nios global state broadcast
    wire [31:0] global_state;

    // SBus Mailbox AVMM interface
    wire        mailbox_avmm_clk;
    wire        mailbox_avmm_areset;
    wire [7:0]  mailbox_avmm_address;
    wire        mailbox_avmm_waitrequest;
    wire        mailbox_avmm_read;
    wire        mailbox_avmm_write;
    wire [31:0] mailbox_avmm_readdata;
    wire [31:0] mailbox_avmm_writedata;
    wire        mailbox_avmm_readdatavalid;

    // RFNVRAM master AVMM interface
    wire        rfnvram_avmm_clk;
    wire        rfnvram_avmm_areset;
    wire [3:0]  rfnvram_avmm_address;
    wire        rfnvram_avmm_waitrequest;
    wire        rfnvram_avmm_read;
    wire        rfnvram_avmm_write;
    wire [31:0] rfnvram_avmm_readdata;
    wire [31:0] rfnvram_avmm_writedata;
    wire        rfnvram_avmm_readdatavalid;
    
    // SMBUS relay AVMM interfaces
    wire [7:0]  relay1_avmm_address;
    wire        relay1_avmm_write;
    wire [31:0] relay1_avmm_writedata;
    wire [7:0]  relay2_avmm_address;
    wire        relay2_avmm_write;
    wire [31:0] relay2_avmm_writedata;
    wire [7:0]  relay3_avmm_address;
    wire        relay3_avmm_write;
    wire [31:0] relay3_avmm_writedata;

    // Timer bank AVMM interface
    wire        timer_bank_avmm_clk;
    wire        timer_bank_avmm_areset;
    wire [2:0]  timer_bank_avmm_address;
    wire        timer_bank_avmm_waitrequest;
    wire        timer_bank_avmm_read;
    wire        timer_bank_avmm_write;
    wire [31:0] timer_bank_avmm_readdata;
    wire [31:0] timer_bank_avmm_writedata;

    // Crypto block AVMM interface
    wire        crypto_avmm_clk;
    wire        crypto_avmm_areset;
    wire [6:0]  crypto_avmm_address;
    wire        crypto_avmm_read;
    wire        crypto_avmm_write;
    wire [31:0] crypto_avmm_readdata;
    wire        crypto_avmm_readdatavalid;
    wire [31:0] crypto_avmm_writedata;

    // SPI Master CSR AVMM interface
    wire [5:0]  spi_master_csr_avmm_address;
    wire        spi_master_csr_avmm_waitrequest;
    wire        spi_master_csr_avmm_read;
    wire        spi_master_csr_avmm_write;
    wire [31:0] spi_master_csr_avmm_readdata;
    wire        spi_master_csr_avmm_readdatavalid;
    wire [31:0] spi_master_csr_avmm_writedata;

    // SPI Master AVMM interface to SPI memory
    wire [24:0] spi_master_avmm_address;
    wire        spi_master_avmm_waitrequest;
    wire        spi_master_avmm_read;
    wire        spi_master_avmm_write;
    wire [31:0] spi_master_avmm_readdata;
    wire [31:0] spi_master_avmm_writedata;
    wire        spi_master_avmm_readdatavalid;

    // SPI Filter AVMM interface to BMC and PCH SPI filters to control which sections of each FLASH device suppor write/erase commands (to be configured based on the PFM)
    wire        spi_filter_bmc_we_avmm_clk;
    wire        spi_filter_bmc_we_avmm_areset;
    wire [13:0] spi_filter_bmc_we_avmm_address;     // address window is larger than actually required, some top address bits will be ignored
    wire        spi_filter_bmc_we_avmm_write;
    wire [31:0] spi_filter_bmc_we_avmm_writedata;
    wire        spi_filter_bmc_we_avmm_read;
    wire [31:0] spi_filter_bmc_we_avmm_readdata;
    wire        spi_filter_bmc_we_avmm_readdatavalid;
    wire [13:0] spi_filter_pch_we_avmm_address;     // address window is larger than actually required, some top address bits will be ignored
    wire        spi_filter_pch_we_avmm_write;
    wire [31:0] spi_filter_pch_we_avmm_writedata;
    wire        spi_filter_pch_we_avmm_read;
    wire [31:0] spi_filter_pch_we_avmm_readdata;
    wire        spi_filter_pch_we_avmm_readdatavalid;
	 
    // Crypto DMA AVMM interface to Crypto DMA's CSR
    logic           crypto_dma_avmm_clk;
    logic           crypto_dma_avmm_areset;
    logic [1:0]     crypto_dma_avmm_address;
    logic           crypto_dma_avmm_read;
    logic           crypto_dma_avmm_write;
    logic [31:0]    crypto_dma_avmm_readdata;
    logic [31:0]    crypto_dma_avmm_writedata;
	 
    // signals for logging blocked SPI commands
    logic [31:0]                         bmc_spi_filtered_command_info; // 31:14 - address bits 31:14 of filtered command, 13 - 1=illegal command, 0=illegal write/erase region, 12:8- number of filtered commands, 7:0 - command that was filtered
    logic [31:0]                         pch_spi_filtered_command_info; // 31:14 - address bits 31:14 of filtered command, 13 - 1=illegal command, 0=illegal write/erase region, 12:8- number of filtered commands, 7:0 - command that was filtered
	
    // Crypto DMA to Crypto IP
    logic           dma_nios_master_sel;
    logic           dma_writedata_valid;
    logic [33:0]    dma_writedata;
    logic           sha_done;
    
    logic [24:0]   dma_spi_avmm_mem_address;
    logic [31:0]   dma_spi_avmm_mem_readdata;
    logic          dma_spi_avmm_mem_readdatavalid;
    logic          dma_spi_avmm_mem_waitrequest;
    logic          dma_spi_avmm_mem_read;
    logic [6:0]    dma_spi_avmm_mem_burstcount;

    // bus to mux between which SMBus port for SMBus master to send SMBus traffic 
    logic [2:0] slave_portid;
    
    // DMA to Onchip flash IP avmm interface
    logic           dma_to_ufm_avmm_waitrequest;
    logic [31:0]    dma_to_ufm_avmm_readdata;
    logic           dma_to_ufm_avmm_readdatavalid;
    logic [20:0]    dma_to_ufm_avmm_address;         
    logic           dma_to_ufm_avmm_read;            
    logic [5:0]     dma_to_ufm_avmm_burstcount; 
    
    // GPO to enable DMA to Onchip flash IP path
    logic           dma_ufm_sel;

   ///////////////////////////////////////////////////////////////////////////
    // Nios IIe system
    ///////////////////////////////////////////////////////////////////////////
    pfr_sys u_pfr_sys(
        // System clocks and synchronized resets. Note that sys_clk must be
        // less than 80MHz as it is connected to the dual config IP, and that
        // IP has a max frequency of 80MHz. 
        .sys_clk_clk(sys_clk),
        .spi_clk_clk(spi_clk),

        .sys_clk_reset_reset(!sys_clk_reset_sync_n),
        .spi_clk_reset_reset(!spi_clk_reset_sync_n),

        .global_state_export(global_state),

        .gpo_1_export(gpo_1),

        .gpi_1_export(gpi_1),

        // AVMM interface to the timer bank
        .timer_bank_avmm_clk(timer_bank_avmm_clk),
        .timer_bank_avmm_areset(timer_bank_avmm_areset),
        .timer_bank_avmm_address(timer_bank_avmm_address),
        .timer_bank_avmm_waitrequest(timer_bank_avmm_waitrequest),
        .timer_bank_avmm_read(timer_bank_avmm_read),
        .timer_bank_avmm_write(timer_bank_avmm_write),
        .timer_bank_avmm_readdata(timer_bank_avmm_readdata),
        .timer_bank_avmm_writedata(timer_bank_avmm_writedata),

        // AVMM interface to SMBus Mailbox         
        .mailbox_avmm_clk(mailbox_avmm_clk),
        .mailbox_avmm_areset(mailbox_avmm_areset),
        .mailbox_avmm_address(mailbox_avmm_address),
        .mailbox_avmm_waitrequest(mailbox_avmm_waitrequest),
        .mailbox_avmm_read(mailbox_avmm_read),
        .mailbox_avmm_write(mailbox_avmm_write),
        .mailbox_avmm_readdata(mailbox_avmm_readdata),
        .mailbox_avmm_writedata(mailbox_avmm_writedata),
        .mailbox_avmm_readdatavalid(mailbox_avmm_readdatavalid),

        .rfnvram_avmm_clk                  (rfnvram_avmm_clk),
        .rfnvram_avmm_areset               (rfnvram_avmm_areset),
        .rfnvram_avmm_address              (rfnvram_avmm_address),
        .rfnvram_avmm_waitrequest          (rfnvram_avmm_waitrequest),
        .rfnvram_avmm_read                 (rfnvram_avmm_read),
        .rfnvram_avmm_write                (rfnvram_avmm_write),
        .rfnvram_avmm_readdata             (rfnvram_avmm_readdata),
        .rfnvram_avmm_writedata            (rfnvram_avmm_writedata),
        .rfnvram_avmm_readdatavalid        (rfnvram_avmm_readdatavalid),
        // AVMM interfaces to SMBus relay command whitelist memory
        .relay1_avmm_clk        (  ),
        .relay1_avmm_areset     (  ),
        .relay1_avmm_address    ( relay1_avmm_address ),
        .relay1_avmm_waitrequest( '0 ),
        .relay1_avmm_read       (  ),
        .relay1_avmm_write      ( relay1_avmm_write ),
        .relay1_avmm_readdata   ( '0 ),
        .relay1_avmm_writedata  ( relay1_avmm_writedata ),
        .relay2_avmm_clk        (  ),
        .relay2_avmm_areset     (  ),
        .relay2_avmm_address    ( relay2_avmm_address ),
        .relay2_avmm_waitrequest( '0 ),
        .relay2_avmm_read       (  ),
        .relay2_avmm_write      ( relay2_avmm_write ),
        .relay2_avmm_readdata   ( '0 ),
        .relay2_avmm_writedata  ( relay2_avmm_writedata ),
        .relay3_avmm_clk        (  ),
        .relay3_avmm_areset     (  ),
        .relay3_avmm_address    ( relay3_avmm_address ),
        .relay3_avmm_waitrequest( '0 ),
        .relay3_avmm_read       (  ),
        .relay3_avmm_write      ( relay3_avmm_write ),
        .relay3_avmm_readdata   ( '0 ),
        .relay3_avmm_writedata  ( relay3_avmm_writedata ),

        // AVMM interface to the crypto block
        .crypto_avmm_clk(crypto_avmm_clk),
        .crypto_avmm_areset(crypto_avmm_areset),
        .crypto_avmm_address(crypto_avmm_address),
        .crypto_avmm_waitrequest(1'b0),
        .crypto_avmm_read(crypto_avmm_read),
        .crypto_avmm_write(crypto_avmm_write),
        .crypto_avmm_readdata(crypto_avmm_readdata),
        .crypto_avmm_readdatavalid(crypto_avmm_readdatavalid),
        .crypto_avmm_writedata(crypto_avmm_writedata),

        // AVMM interface to the SPI Filter write-enable memories
        .spi_filter_bmc_we_avmm_clk        ( spi_filter_bmc_we_avmm_clk     ),
        .spi_filter_bmc_we_avmm_areset     ( spi_filter_bmc_we_avmm_areset  ),
        .spi_filter_bmc_we_avmm_address    ( spi_filter_bmc_we_avmm_address ),
        .spi_filter_bmc_we_avmm_waitrequest( '0 ),
        .spi_filter_bmc_we_avmm_read       ( spi_filter_bmc_we_avmm_read),
        .spi_filter_bmc_we_avmm_write      ( spi_filter_bmc_we_avmm_write ),
        .spi_filter_bmc_we_avmm_readdata   ( spi_filter_bmc_we_avmm_readdata ),
        .spi_filter_bmc_we_avmm_readdatavalid( spi_filter_bmc_we_avmm_readdatavalid ),
        .spi_filter_bmc_we_avmm_writedata  ( spi_filter_bmc_we_avmm_writedata ),
        .spi_filter_pch_we_avmm_clk        (  ),
        .spi_filter_pch_we_avmm_areset     (  ),
        .spi_filter_pch_we_avmm_address    ( spi_filter_pch_we_avmm_address ),
        .spi_filter_pch_we_avmm_waitrequest( '0 ),
        .spi_filter_pch_we_avmm_read       ( spi_filter_pch_we_avmm_read ),
        .spi_filter_pch_we_avmm_write      ( spi_filter_pch_we_avmm_write ),
        .spi_filter_pch_we_avmm_readdata   ( spi_filter_pch_we_avmm_readdata ),       
        .spi_filter_pch_we_avmm_readdatavalid( spi_filter_pch_we_avmm_readdatavalid ),       
        .spi_filter_pch_we_avmm_writedata  ( spi_filter_pch_we_avmm_writedata ),

        // AVMM interface to the SPI filter CSR
        .spi_filter_csr_avmm_clk            (  ),
        .spi_filter_csr_avmm_areset         (  ),
        .spi_filter_csr_avmm_address        (spi_master_csr_avmm_address),
        .spi_filter_csr_avmm_waitrequest    (spi_master_csr_avmm_waitrequest),
        .spi_filter_csr_avmm_read           (spi_master_csr_avmm_read),
        .spi_filter_csr_avmm_write          (spi_master_csr_avmm_write),
        .spi_filter_csr_avmm_readdata       (spi_master_csr_avmm_readdata),
        .spi_filter_csr_avmm_readdatavalid  (spi_master_csr_avmm_readdatavalid),
        .spi_filter_csr_avmm_writedata      (spi_master_csr_avmm_writedata),

        // AVMM interface to the SPI filter
        .spi_filter_avmm_clk                (  ),
        .spi_filter_avmm_areset             (  ),
        .spi_filter_avmm_address            (spi_master_avmm_address),
        .spi_filter_avmm_waitrequest        (spi_master_avmm_waitrequest),
        .spi_filter_avmm_read               (spi_master_avmm_read),
        .spi_filter_avmm_write              (spi_master_avmm_write),
        .spi_filter_avmm_readdata           (spi_master_avmm_readdata),
        .spi_filter_avmm_readdatavalid      (spi_master_avmm_readdatavalid),
        .spi_filter_avmm_writedata          (spi_master_avmm_writedata),
	
        // AVMM interface from Crypto DMA to UFM
        .dma_to_ufm_avmm_clk            (),     
        .dma_to_ufm_avmm_areset		    (), 
        .dma_to_ufm_avmm_address		(dma_to_ufm_avmm_address),         
        .dma_to_ufm_avmm_waitrequest	(dma_to_ufm_avmm_waitrequest),      
        .dma_to_ufm_avmm_read		    (dma_to_ufm_avmm_read),            
        .dma_to_ufm_avmm_write		    (1'b0),             
        .dma_to_ufm_avmm_readdata		(dma_to_ufm_avmm_readdata),     
        .dma_to_ufm_avmm_writedata		(32'b0),        
        .dma_to_ufm_avmm_readdatavalid	(dma_to_ufm_avmm_readdatavalid),
        .dma_to_ufm_avmm_burstcount		(dma_to_ufm_avmm_burstcount), 

        // AVMM interface to Crypto DMA
        .crypto_dma_avmm_clk                (crypto_dma_avmm_clk),
        .crypto_dma_avmm_areset             (crypto_dma_avmm_areset),
        .crypto_dma_avmm_address            (crypto_dma_avmm_address),
        .crypto_dma_avmm_waitrequest        (1'b0),
        .crypto_dma_avmm_read               (crypto_dma_avmm_read),
        .crypto_dma_avmm_write              (crypto_dma_avmm_write),
        .crypto_dma_avmm_readdata           (crypto_dma_avmm_readdata),
        .crypto_dma_avmm_writedata          (crypto_dma_avmm_writedata)
    );

	
    ///////////////////////////////////////////////////////////////////////////
    // GPI Connectivity from Nios
    ///////////////////////////////////////////////////////////////////////////
    reg rearm_acm_timer;
    reg hold_pltrst_in_seamless;
    
    logic me_done_reg;
    logic me_done_flag;
    logic clear_me_done_flag;
    
    // PCH and BMC resets from common core
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_cc_RST_RSMRST_PLD_R_N_BIT_POS] = cc_RST_RSMRST_PLD_R_N;
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_cc_RST_SRST_BMC_PLD_R_N_BIT_POS] = cc_RST_SRST_BMC_PLD_R_N;
    
    
    // ME firmware uses these two GPIOs to signal its security status to PFR RoT 
    // TODO figure out if we still need these signals
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_FM_ME_PFR_1_BIT_POS] = FM_ME_AUTHN_FAIL;
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_FM_ME_PFR_2_BIT_POS] = me_done_flag;
    
    assign clear_me_done_flag = gpo_1[gen_gpo_controls_pkg::GPO_1_CLEAR_ME_DONE_FLAG_BIT_POS];

    always_ff @(posedge sys_clk or posedge timer_bank_avmm_areset) begin
        if (timer_bank_avmm_areset) begin
            me_done_reg <= 1'b0;
            me_done_flag <= 1'b0;
        end
        else begin
            me_done_reg <= FM_ME_BT_DONE;
            if (!me_done_reg && FM_ME_BT_DONE) begin
                me_done_flag <= 1'b1;
            end
            if (clear_me_done_flag) begin
                me_done_flag <= 1'b0;
            end
        end
    end
    
    
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_PLTRST_DETECTED_REARM_ACM_TIMER_BIT_POS] = rearm_acm_timer;

    // status bit from the SPI filter block
    logic spi_control_bmc_ibb_access_detected;
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_BMC_SPI_IBB_ACCESS_DETECTED_BIT_POS] = spi_control_bmc_ibb_access_detected;
    // Forces recovery in manual mode
    // We repurposed the FM_PFR_PROV_UPDATE pin because the FORCE_RECOVERY pin could not be used
    // TODO This signal might not exist in archer city
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_FM_PFR_FORCE_RECOVERY_N_BIT_POS] = FM_PFR_FORCE_RECOVERY_N;
    // HPFR Inputs
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_HPFR_IN_BIT_POS] = ccHPFR_IN;
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_HPFR_ACTIVE_BIT_POS] = ccHPFR_ACTIVE;
    assign gpi_1[gen_gpi_signals_pkg::GPI_1_LEGACY_BIT_POS] = ccLEGACY;

    // unused GPI signals
    assign gpi_1[31:gen_gpi_signals_pkg::GPI_1_UNUSED_BITS_START_BIT_POS] = '0;

    
    ///////////////////////////////////////////////////////////////////////////
    // GPO Connectivity from Nios	
    ///////////////////////////////////////////////////////////////////////////
    // BMC Reset. Active low
    assign RST_SRST_BMC_PLD_R_N = gpo_1[gen_gpo_controls_pkg::GPO_1_RST_SRST_BMC_PLD_R_N_BIT_POS] & cc_RST_RSMRST_PLD_R_N;
    // PCH Reset. Active low
    // When Nios drives the GPO high, let common core drives the RSMRST#. 
    // The common core will have code that waits for this SLP_SUS signal before asserting RSMRST#.
    assign RST_RSMRST_PLD_R_N = (gpo_1[gen_gpo_controls_pkg::GPO_1_RST_RSMRST_PLD_R_N_BIT_POS]) ? cc_RST_RSMRST_PLD_R_N : 1'b0;
    
    // Mux select pins
    // PCH SPI Mux. 0-PCH, 1-CPLD
    assign FM_SPI_PFR_PCH_MASTER_SEL_R = gpo_1[gen_gpo_controls_pkg::GPO_1_FM_SPI_PFR_PCH_MASTER_SEL_BIT_POS];
    // BMC SPI Mux. 0-BMC, 1-CPLD
    assign FM_SPI_PFR_BMC_BT_MASTER_SEL_R = gpo_1[gen_gpo_controls_pkg::GPO_1_FM_SPI_PFR_BMC_BT_MASTER_SEL_BIT_POS];

    // Deep sleep power-ok to PCH
    assign FM_PFR_DSW_PWROK_N = (gpo_1[gen_gpo_controls_pkg::GPO_1_PWRGD_DSW_PWROK_R_BIT_POS]) ? 1'b0 : 1'bz;
    
    // BMC external reset. Used for BMC only update
    // Going away in fab2
    assign RST_PFR_EXTRST_R_N = gpo_1[gen_gpo_controls_pkg::GPO_1_RST_PFR_EXTRST_N_BIT_POS];
    // TODO check if polarity changed
    assign FM_PFR_SLP_SUS_EN_R_N = gpo_1[gen_gpo_controls_pkg::GPO_1_FM_PFR_SLP_SUS_N_BIT_POS];
    assign FM_PFR_ON_R = gpo_1[gen_gpo_controls_pkg::GPO_1_FM_PFR_ON_BIT_POS];
    
    // pin to allow the Nios to clear the pltrest detect register
    wire clear_pltrst_detect_flag;
    assign clear_pltrst_detect_flag = gpo_1[gen_gpo_controls_pkg::GPO_1_CLEAR_PLTRST_DETECT_FLAG_BIT_POS];

    ///////////////////////////////////////////////////////////////////////////
    // PLTRST_N edge detect   
    ///////////////////////////////////////////////////////////////////////////
    reg pltrst_reg;
    
    always_ff @(posedge sys_clk or posedge timer_bank_avmm_areset) begin
        if (timer_bank_avmm_areset) begin
            pltrst_reg <= 1'b0;
            rearm_acm_timer <=1'b0;
        end else begin 
            pltrst_reg <= cc_RST_PLTRST_PLD_N;
            if (!pltrst_reg & cc_RST_PLTRST_PLD_N) begin
                rearm_acm_timer <= 1'b1;
            end
            if (clear_pltrst_detect_flag) begin
                rearm_acm_timer <= 1'b0;
            end
        end
    end
    
    always_ff @(posedge sys_clk or posedge timer_bank_avmm_areset) begin
        if (timer_bank_avmm_areset) begin
            hold_pltrst_in_seamless <= 1'b0;
        end else begin 
            if (pltrst_reg & !RST_PLTRST_PLD_N & FM_SPI_PFR_PCH_MASTER_SEL_R) begin
                hold_pltrst_in_seamless <= 1'b1;
            end
            if (!FM_SPI_PFR_PCH_MASTER_SEL_R) begin
                hold_pltrst_in_seamless <= 1'b0;
            end
        end
    end
    
    // SMBus Relay control pins
    logic relay1_block_disable;
    logic relay1_filter_disable;
    logic relay1_all_addresses;
    assign relay1_block_disable  = gpo_1[gen_gpo_controls_pkg::GPO_1_RELAY1_BLOCK_DISABLE_BIT_POS] ;
    assign relay1_filter_disable = gpo_1[gen_gpo_controls_pkg::GPO_1_RELAY1_FILTER_DISABLE_BIT_POS];
    assign relay1_all_addresses  = gpo_1[gen_gpo_controls_pkg::GPO_1_RELAY1_ALL_ADDRESSES_BIT_POS];
    logic relay2_block_disable;
    logic relay2_filter_disable;
    logic relay2_all_addresses;
    assign relay2_block_disable  = gpo_1[gen_gpo_controls_pkg::GPO_1_RELAY2_BLOCK_DISABLE_BIT_POS] ;
    assign relay2_filter_disable = gpo_1[gen_gpo_controls_pkg::GPO_1_RELAY2_FILTER_DISABLE_BIT_POS];
    assign relay2_all_addresses  = gpo_1[gen_gpo_controls_pkg::GPO_1_RELAY2_ALL_ADDRESSES_BIT_POS];
    logic relay3_block_disable;
    logic relay3_filter_disable;
    logic relay3_all_addresses;
    assign relay3_block_disable  = gpo_1[gen_gpo_controls_pkg::GPO_1_RELAY3_BLOCK_DISABLE_BIT_POS] ;
    assign relay3_filter_disable = gpo_1[gen_gpo_controls_pkg::GPO_1_RELAY3_FILTER_DISABLE_BIT_POS];
    assign relay3_all_addresses  = gpo_1[gen_gpo_controls_pkg::GPO_1_RELAY3_ALL_ADDRESSES_BIT_POS];


    // SPI control block control pins
    logic spi_control_pfr_bmc_master_sel;
    logic spi_control_pfr_pch_master_sel;
    logic spi_control_spi_master_bmc_pchn;
    logic spi_control_bmc_filter_disable;
    logic spi_control_pch_filter_disable;
    logic spi_control_bmc_clear_ibb_detected;
    logic spi_control_bmc_addr_mode_set_3b;
    assign spi_control_pfr_bmc_master_sel       = gpo_1[gen_gpo_controls_pkg::GPO_1_FM_SPI_PFR_BMC_BT_MASTER_SEL_BIT_POS];
    assign spi_control_pfr_pch_master_sel       = gpo_1[gen_gpo_controls_pkg::GPO_1_FM_SPI_PFR_PCH_MASTER_SEL_BIT_POS];
    assign spi_control_spi_master_bmc_pchn      = gpo_1[gen_gpo_controls_pkg::GPO_1_SPI_MASTER_BMC_PCHN_BIT_POS];
    assign spi_control_bmc_filter_disable       = gpo_1[gen_gpo_controls_pkg::GPO_1_BMC_SPI_FILTER_DISABLE_BIT_POS];
    assign spi_control_pch_filter_disable       = gpo_1[gen_gpo_controls_pkg::GPO_1_PCH_SPI_FILTER_DISABLE_BIT_POS];
    assign spi_control_bmc_clear_ibb_detected   = gpo_1[gen_gpo_controls_pkg::GPO_1_BMC_SPI_CLEAR_IBB_DETECTED_BIT_POS];
    assign spi_control_bmc_addr_mode_set_3b     = gpo_1[gen_gpo_controls_pkg::GPO_1_BMC_SPI_ADDR_MODE_SET_3B_BIT_POS];
    
    // PCH Resets. They are used to trigger Top Swap Reset during the last stage of PCH recovery. 
    //TODO check if polarity changed
    assign RST_PFR_OVR_RTC_R = (gpo_1[gen_gpo_controls_pkg::GPO_1_TRIGGER_TOP_SWAP_RESET_BIT_POS]) ? 1'b1 : 1'bZ; 
    assign RST_PFR_OVR_SRTC_R = (gpo_1[gen_gpo_controls_pkg::GPO_1_TRIGGER_TOP_SWAP_RESET_BIT_POS]) ? 1'b1 : 1'bZ; 

    // HPFR output
    assign ccHPFR_OUT = gpo_1[gen_gpo_controls_pkg::GPO_1_HPFR_OUT_BIT_POS];
    
    // DMA or NIOS path to Crypto IP and SPI Master IP
    assign dma_nios_master_sel = gpo_1[gen_gpo_controls_pkg::GPO_1_DMA_NIOS_MASTER_SEL_BIT_POS];
    
    // DMA to Onchip flash IP path
    assign dma_ufm_sel = gpo_1[gen_gpo_controls_pkg::GPO_1_DMA_UFM_SEL_BIT_POS];

    ///////////////////////////////////////////////////////////////////////////
    // Unused toplevel pins. Drive to Z
    ///////////////////////////////////////////////////////////////////////////
    assign FAN_BMC_PWM_R = 'Z;
    
    ///////////////////////////////////////////////////////////////////////////
    // Unused toplevel pins that are going away in Wilson City FAB2
    ///////////////////////////////////////////////////////////////////////////
    assign RST_PFR_OVR_RTC_R = 'Z; 
    assign RST_PFR_OVR_SRTC_R = 'Z; 
    
    /////////////////////////////////////////
    // REFACTOR THESE. CONNECT TO IPS AS REQUIRED
    /////////////////////////////////////////

    // TODO: LED signaling
    //assign FP_ID_BTN_PFR_N = FP_ID_BTN_N;
    assign FP_ID_LED_PFR_N = FP_ID_LED_N;
    assign FP_LED_STATUS_AMBER_PFR_N = FP_LED_STATUS_AMBER_N;
    assign FP_LED_STATUS_GREEN_PFR_N = FP_LED_STATUS_GREEN_N;
    
    // SPI 
    assign RST_SPI_PFR_BMC_BOOT_N = gpo_1[gen_gpo_controls_pkg::GPO_1_RST_SPI_PFR_BMC_BOOT_N_BIT_POS];
    assign RST_SPI_PFR_PCH_N = gpo_1[gen_gpo_controls_pkg::GPO_1_RST_SPI_PFR_PCH_N_BIT_POS];
     
    ///////////////////////////////////////////////////////////////////////////
    // Timer bank
    ///////////////////////////////////////////////////////////////////////////
    timer_bank u_timer_bank (
        .clk(timer_bank_avmm_clk),
        .areset(timer_bank_avmm_areset),
        .clk2M(clk2M),
        
        .avmm_address(timer_bank_avmm_address),
        .avmm_read(timer_bank_avmm_read),
        .avmm_write(timer_bank_avmm_write),
        .avmm_readdata(timer_bank_avmm_readdata),
        .avmm_writedata(timer_bank_avmm_writedata)
    );
    assign timer_bank_avmm_waitrequest = 1'b0;

    ///////////////////////////////////////////////////////////////////////////
    // Crypto block
    ///////////////////////////////////////////////////////////////////////////
    // Note that waitrequest can be tied low, and thus reduce the area of the
    // fabric, if the master guarantees to poll for data-ready after delivering
    // 128-bytes (32 words)
    crypto_top #
    (
        .ENABLE_CRYPTO_256  (platform_defs_pkg::ENABLE_CRYPTO_256),
        .ENABLE_CRYPTO_384  (platform_defs_pkg::ENABLE_CRYPTO_384),
        .USE_ECDSA_BLOCK    (1)
    ) u_crypto (
        .clk(crypto_avmm_clk),
        .areset(crypto_avmm_areset),
       
        .csr_address(crypto_avmm_address),
        .csr_read(crypto_avmm_read),
        .csr_write(crypto_avmm_write),
        .csr_readdata(crypto_avmm_readdata),
        .csr_readdatavalid(crypto_avmm_readdatavalid),
        .csr_writedata(crypto_avmm_writedata),

        .dma_nios_master_sel(dma_nios_master_sel || dma_ufm_sel), // Either one set enables DMA path to SHA block
        .dma_writedata_valid(dma_writedata_valid),
        .dma_writedata(dma_writedata),
        .sha_done(sha_done)
    );
	 
    ///////////////////////////////////////////////////////////////////////////
    // Crypto DMA
    ///////////////////////////////////////////////////////////////////////////
    crypto_dma #(
        .ADDRESS_WIDTH_SPI  (platform_defs_pkg::CRYPTO_DMA_SPI_ADDRESS_WIDTH), // in word addressing format based on the largest supported flash device in platform
        .ADDRESS_WIDTH_UFM  (platform_defs_pkg::CRYPTO_DMA_UFM_ADDRESS_WIDTH) 
    ) u_crypto_dma (
        .spi_clk                (spi_clk),
        .spi_clk_areset         (~spi_clk_reset_sync_n),
        .spi_mm_address         (dma_spi_avmm_mem_address),
        .spi_mm_read            (dma_spi_avmm_mem_read),
        .spi_mm_burstcount      (dma_spi_avmm_mem_burstcount),
        .spi_mm_readdata        (dma_spi_avmm_mem_readdata),
        .spi_mm_readdatavalid   (dma_spi_avmm_mem_readdatavalid),
        .spi_mm_waitrequest     (dma_spi_avmm_mem_waitrequest),
        .ufm_mm_address         (dma_to_ufm_avmm_address),
        .ufm_mm_read            (dma_to_ufm_avmm_read),
        .ufm_mm_burstcount      (dma_to_ufm_avmm_burstcount),
        .ufm_mm_readdata        (dma_to_ufm_avmm_readdata),
        .ufm_mm_readdatavalid   (dma_to_ufm_avmm_readdatavalid),
        .ufm_mm_waitrequest     (dma_to_ufm_avmm_waitrequest),
        .csr_read               (crypto_dma_avmm_read),      
        .csr_write              (crypto_dma_avmm_write),     
        .csr_writedata          (crypto_dma_avmm_writedata),    
        .csr_address            (crypto_dma_avmm_address),     
        .csr_readdata           (crypto_dma_avmm_readdata),                
        .sys_clk                (crypto_dma_avmm_clk),     
        .sys_clk_areset         (crypto_dma_avmm_areset),     
        .wr_data                (dma_writedata),
        .wr_data_valid          (dma_writedata_valid),
        .sha_done               (sha_done),
        .dma_ufm_sel            (dma_ufm_sel)
 
    );

    ///////////////////////////////////////////////////////////////////////////
    // SMBus mailbox
    ///////////////////////////////////////////////////////////////////////////

    logic bmc_mailbox_slave_scl_in;
    logic bmc_mailbox_slave_scl_oe;
    logic bmc_mailbox_slave_sda_in;
    logic bmc_mailbox_slave_sda_oe;

    logic pch_mailbox_slave_scl_in;
    logic pch_mailbox_slave_scl_oe;
    logic pch_mailbox_slave_sda_in;
    logic pch_mailbox_slave_sda_oe;
    
    logic rfnvram_sda_in;
    logic rfnvram_sda_oe;
    logic rfnvram_scl_in;
    logic rfnvram_scl_oe;
    
    logic pcie_mailbox_slave_scl_in;
    logic pcie_mailbox_slave_scl_oe;
    logic pcie_mailbox_slave_sda_in;
    logic pcie_mailbox_slave_sda_oe;
    
    logic [7:0] crc_code_out;

    smbus_mailbox #(
        .PCH_SMBUS_ADDRESS(platform_defs_pkg::PCH_SMBUS_MAILBOX_ADDR),
        .BMC_SMBUS_ADDRESS(platform_defs_pkg::BMC_SMBUS_MAILBOX_ADDR),
        .PCIE_SMBUS_ADDRESS(platform_defs_pkg::PCIE_SMBUS_MAILBOX_ADDR)
    ) u_smbus_mailbox (
        .clk(mailbox_avmm_clk),
        .i_resetn(!mailbox_avmm_areset),
        .slave_portid(slave_portid),
        .crc_code_out(crc_code_out),
        .ia_bmc_slave_sda_in(bmc_mailbox_slave_sda_in),
        .o_bmc_slave_sda_oe(bmc_mailbox_slave_sda_oe), 
        .ia_bmc_slave_scl_in(bmc_mailbox_slave_scl_in),
        .o_bmc_slave_scl_oe(bmc_mailbox_slave_scl_oe),
        .ia_pch_slave_sda_in(pch_mailbox_slave_sda_in),
        .o_pch_slave_sda_oe(pch_mailbox_slave_sda_oe), 
        .ia_pch_slave_scl_in(pch_mailbox_slave_scl_in),
        .o_pch_slave_scl_oe(pch_mailbox_slave_scl_oe),
        .ia_pcie_slave_sda_in(pcie_mailbox_slave_sda_in),
        .o_pcie_slave_sda_oe(pcie_mailbox_slave_sda_oe), 
        .ia_pcie_slave_scl_in(pcie_mailbox_slave_scl_in),
        .o_pcie_slave_scl_oe(pcie_mailbox_slave_scl_oe),

        .m0_read(mailbox_avmm_read),
        .m0_write(mailbox_avmm_write),
        .m0_writedata(mailbox_avmm_writedata),
        .m0_readdata(mailbox_avmm_readdata),
        .m0_address(mailbox_avmm_address),
        .m0_readdatavalid(mailbox_avmm_readdatavalid)
    );
    // Remove tristate buffer because SMB_PFR_DDRABCD_CPU1_LVC2_SCL & SMB_BMC_HSBP_STBY_LVC3_SCL is changed to input port
    // Not expecting clock stretching from SMBus mailbox
    // comment off SMB_S3M_CPU0_SCL_LVC1 because it is an INPUT port in Archer City schematic, common core is consuming this port too
    //assign SMB_S3M_CPU0_SCL_LVC1 = ((rfnvram_scl_oe && (slave_portid == 3'b001)) || pch_mailbox_slave_scl_oe) ? 1'b0 : 1'bz;
    assign SMB_S3M_CPU0_SDA_LVC1 = ((rfnvram_sda_oe && (slave_portid == 3'b001)) || pch_mailbox_slave_sda_oe) ? 1'b0 : 1'bz;
    assign pch_mailbox_slave_scl_in = SMB_S3M_CPU0_SCL_LVC1;
    assign pch_mailbox_slave_sda_in = SMB_S3M_CPU0_SDA_LVC1;

    assign SMB_BMC_HSBP_STBY_LVC3_SCL = ((rfnvram_scl_oe && (slave_portid == 3'b010)) || bmc_mailbox_slave_scl_oe) ? 1'b0 : 1'bz;
    assign SMB_BMC_HSBP_STBY_LVC3_SDA = ((rfnvram_sda_oe && (slave_portid == 3'b010)) || bmc_mailbox_slave_sda_oe) ? 1'b0 : 1'bz;
    assign bmc_mailbox_slave_scl_in = SMB_BMC_HSBP_STBY_LVC3_SCL;
    assign bmc_mailbox_slave_sda_in = SMB_BMC_HSBP_STBY_LVC3_SDA;
    
    assign SMB_PCIE_STBY_LVC3_B_SCL = ((rfnvram_scl_oe && (slave_portid == 3'b011)) || pcie_mailbox_slave_scl_oe) ? 1'b0 : 1'bz;
    assign SMB_PCIE_STBY_LVC3_B_SDA = ((rfnvram_sda_oe && (slave_portid == 3'b011)) || pcie_mailbox_slave_sda_oe) ? 1'b0 : 1'bz;
    assign pcie_mailbox_slave_scl_in = SMB_PCIE_STBY_LVC3_B_SCL;
    assign pcie_mailbox_slave_sda_in = SMB_PCIE_STBY_LVC3_B_SDA;

    assign mailbox_avmm_waitrequest =1'b0;

    ///////////////////////////////////////////////////////////////////////////
    // RFNVRAM Master
    ///////////////////////////////////////////////////////////////////////////

    rfnvram_smbus_master #(.FIFO_DEPTH(platform_defs_pkg::RFNVRAM_FIFO_SIZE), .CLOCK_PERIOD_PS(platform_defs_pkg::SYS_CLOCK_PERIOD_PS))
    u_rfnvram_master (
        .clk(rfnvram_avmm_clk),
        .resetn(!rfnvram_avmm_areset),

        // AVMM interface to connect the NIOS to the CSR interface
        .csr_address(rfnvram_avmm_address),
        .csr_read(rfnvram_avmm_read),
        .csr_readdata(rfnvram_avmm_readdata),
        .csr_write(rfnvram_avmm_write),
        .csr_writedata(rfnvram_avmm_writedata),
        .csr_readdatavalid(rfnvram_avmm_readdatavalid),
        
        .slave_portid(slave_portid),

        .sda_in(rfnvram_sda_in),
        .sda_oe(rfnvram_sda_oe),
        .scl_in(rfnvram_scl_in),
        .scl_oe(rfnvram_scl_oe)

    );

    assign SMB_PFR_RFID_STBY_LVC3_SCL = (rfnvram_scl_oe && (slave_portid == 3'b000)) ? 1'b0 : 1'bz;
    assign SMB_PFR_RFID_STBY_LVC3_SDA = (rfnvram_sda_oe && (slave_portid == 3'b000)) ? 1'b0 : 1'bz;
    assign rfnvram_scl_in = (slave_portid == 3'b001) ? SMB_S3M_CPU0_SCL_LVC1 :
                            (slave_portid == 3'b010) ? SMB_BMC_HSBP_STBY_LVC3_SCL : 
                            (slave_portid == 3'b011) ? SMB_PCIE_STBY_LVC3_B_SCL : 
                            (slave_portid == 3'b100) ? SMB_PMBUS_SML1_STBY_LVC3_SCL : 
                            (slave_portid == 3'b101) ? SMB_PCH_PMBUS2_STBY_LVC3_SCL : 
                            (slave_portid == 3'b110) ? SMB_BMC_HSBP_STBY_LVC3_SCL : SMB_PFR_RFID_STBY_LVC3_SCL;
    assign rfnvram_sda_in = (slave_portid == 3'b001) ? SMB_S3M_CPU0_SDA_LVC1 :
                            (slave_portid == 3'b010) ? SMB_BMC_HSBP_STBY_LVC3_SDA : 
                            (slave_portid == 3'b011) ? SMB_PCIE_STBY_LVC3_B_SDA : 
                            (slave_portid == 3'b100) ? SMB_PMBUS_SML1_STBY_LVC3_SDA : 
                            (slave_portid == 3'b101) ? SMB_PCH_PMBUS2_STBY_LVC3_SDA : 
                            (slave_portid == 3'b110) ? SMB_BMC_HSBP_STBY_LVC3_SDA : SMB_PFR_RFID_STBY_LVC3_SDA;
    assign rfnvram_avmm_waitrequest = 1'b0;

    ///////////////////////////////////////////////////////////////////////////
    // SPI Filter and master
    ///////////////////////////////////////////////////////////////////////////

    // signals for acting as SPI bus master on the BMC SPI bus
    logic [3:0]                          bmc_spi_master_data_in;        // data signals from the SPI data pins
    logic [3:0]                          bmc_spi_master_data_out;       // data signals driven to the SPI data pins
    logic [3:0]                          bmc_spi_master_data_oe;        // when asserted, bmc_spi_data drives the respective BMC SPI data pin

    // signals for acting as SPI bus master on the PCH SPI bus
    logic [3:0]                          pch_spi_master_data_in;        // data signals from the SPI data pins
    logic [3:0]                          pch_spi_master_data_out;       // data signals driven to the SPI data pins
    logic [3:0]                          pch_spi_master_data_oe;        // when asserted, pch_spi_data drives the respective PCH SPI data pin
    
    localparam  BMC_ENABLE_COMMAND_LOG = 0;
    localparam  PCH_ENABLE_COMMAND_LOG = 0;
    
    spi_control #(
        .BMC_IBB_ADDRESS_MSBS       ( platform_defs_pkg::BMC_IBB_ADDRESS_MSBS   ),
        .BMC_FLASH_ADDRESS_BITS     ( platform_defs_pkg::BMC_FLASH_ADDRESS_BITS ),
        .PCH_FLASH_ADDRESS_BITS     ( platform_defs_pkg::PCH_FLASH_ADDRESS_BITS ),
        .BMC_ENABLE_COMMAND_LOG     ( BMC_ENABLE_COMMAND_LOG                    ),
        .PCH_ENABLE_COMMAND_LOG     ( PCH_ENABLE_COMMAND_LOG                    )
    ) u_spi_control (
        .clock                      ( spi_clk                               ),
        .i_resetn                   ( spi_clk_reset_sync_n                  ),
        .sys_clock                  ( spi_filter_bmc_we_avmm_clk            ),
        .i_sys_resetn               ( !spi_filter_bmc_we_avmm_areset        ),
        .o_bmc_spi_master_sclk      ( SPI_PFR_BMC_FLASH1_BT_CLK             ),  // connect directly to output pin
        .i_bmc_spi_master_data      ( bmc_spi_master_data_in                ),
        .o_bmc_spi_master_data      ( bmc_spi_master_data_out               ),
        .o_bmc_spi_master_data_oe   ( bmc_spi_master_data_oe                ),
        .o_pch_spi_master_sclk      ( SPI_PFR_PCH_R_CLK                     ),  // connect directly to output pin
        .i_pch_spi_master_data      ( pch_spi_master_data_in                ),
        .o_pch_spi_master_data      ( pch_spi_master_data_out               ),
        .o_pch_spi_master_data_oe   ( pch_spi_master_data_oe                ),
        .clk_bmc_spi_mon_sclk       ( SPI_BMC_BT_MUXED_MON_CLK              ),
        .i_bmc_spi_mon_mosi         ( SPI_BMC_BT_MUXED_MON_MOSI             ),
        .i_bmc_spi_mon_csn          ( SPI_BMC_BOOT_CS_N                     ),
        .clk_pch_spi_mon_sclk       ( SPI_PCH_MUXED_MON_CLK        ),
        .i_pch_spi_mon_data         ( {SPI_PCH_MUXED_MON_IO3, SPI_PCH_MUXED_MON_IO2, SPI_PCH_MUXED_MON_IO1, SPI_PCH_MUXED_MON_IO0} ),
        .i_pch_spi_mon_csn          ( SPI_PCH_PFR_CS0_N                     ),
        .o_bmc_spi_csn              ( SPI_PFR_BMC_BT_SECURE_CS_R_N          ), // Note that this is CS1 due to a schematic error in CC rev 1
        .o_pch_spi_csn              ( SPI_PFR_PCH_SECURE_CS0_R_N            ), 
        .i_pfr_bmc_master_sel       ( spi_control_pfr_bmc_master_sel        ),
        .i_pfr_pch_master_sel       ( spi_control_pfr_pch_master_sel        ),
        .i_spi_master_bmc_pchn      ( spi_control_spi_master_bmc_pchn       ),
        .i_bmc_filter_disable       ( spi_control_bmc_filter_disable        ),
        .i_pch_filter_disable       ( spi_control_pch_filter_disable        ),
        .o_bmc_ibb_access_detected  ( spi_control_bmc_ibb_access_detected   ),
        .i_bmc_clear_ibb_detected   ( spi_control_bmc_clear_ibb_detected    ),
        .i_bmc_addr_mode_set_3b     ( spi_control_bmc_addr_mode_set_3b      ),
        .o_bmc_filtered_command_info( bmc_spi_filtered_command_info         ),
        .o_pch_filtered_command_info( pch_spi_filtered_command_info         ),
        .i_avmm_bmc_we_write        ( spi_filter_bmc_we_avmm_write          ),
        .i_avmm_bmc_we_address      ( spi_filter_bmc_we_avmm_address[platform_defs_pkg::BMC_WE_AVMM_ADDRESS_BITS-1+SPI_FILTER_AVMM_CSR_WIDTH:0]   ),
        .i_avmm_bmc_we_writedata    ( spi_filter_bmc_we_avmm_writedata      ),
        .i_avmm_bmc_we_read         ( spi_filter_bmc_we_avmm_read           ),
        .i_avmm_bmc_we_readdata     ( spi_filter_bmc_we_avmm_readdata       ),
        .i_avmm_bmc_we_readdatavalid( spi_filter_bmc_we_avmm_readdatavalid  ),
        .i_avmm_pch_we_write        ( spi_filter_pch_we_avmm_write          ),
        .i_avmm_pch_we_address      ( spi_filter_pch_we_avmm_address[platform_defs_pkg::PCH_WE_AVMM_ADDRESS_BITS-1+SPI_FILTER_AVMM_CSR_WIDTH:0]   ),
        .i_avmm_pch_we_writedata    ( spi_filter_pch_we_avmm_writedata      ),
        .i_avmm_pch_we_read         ( spi_filter_pch_we_avmm_read           ),
        .i_avmm_pch_we_readdata     ( spi_filter_pch_we_avmm_readdata       ),
        .i_avmm_pch_we_readdatavalid( spi_filter_pch_we_avmm_readdatavalid  ),
        .i_avmm_csr_address         ( spi_master_csr_avmm_address           ),
        .i_avmm_csr_read            ( spi_master_csr_avmm_read              ),
        .i_avmm_csr_write           ( spi_master_csr_avmm_write             ),
        .o_avmm_csr_waitrequest     ( spi_master_csr_avmm_waitrequest       ),
        .i_avmm_csr_writedata       ( spi_master_csr_avmm_writedata         ),
        .o_avmm_csr_readdata        ( spi_master_csr_avmm_readdata          ),
        .o_avmm_csr_readdatavalid   ( spi_master_csr_avmm_readdatavalid     ),
        .i_avmm_mem_address         ( spi_master_avmm_address               ),
        .i_avmm_mem_read            ( spi_master_avmm_read                  ),
        .i_avmm_mem_write           ( spi_master_avmm_write                 ),
        .o_avmm_mem_waitrequest     ( spi_master_avmm_waitrequest           ),
        .i_avmm_mem_writedata       ( spi_master_avmm_writedata             ),
        .o_avmm_mem_readdata        ( spi_master_avmm_readdata              ),
        .o_avmm_mem_readdatavalid   ( spi_master_avmm_readdatavalid         ),
        .i_dma_nios_master_sel      ( dma_nios_master_sel                   ),
        .i_dma_avmm_mem_address         ( dma_spi_avmm_mem_address          ),
        .i_dma_avmm_mem_read            ( dma_spi_avmm_mem_read             ),
        .i_dma_avmm_mem_write           ( 1'b0                              ),
        .i_dma_avmm_mem_burstcount      ( dma_spi_avmm_mem_burstcount       ),
        .o_dma_avmm_mem_waitrequest     ( dma_spi_avmm_mem_waitrequest      ),
        .i_dma_avmm_mem_writedata       ( 32'b0                             ),
        .o_dma_avmm_mem_readdata        ( dma_spi_avmm_mem_readdata         ),
        .o_dma_avmm_mem_readdatavalid   ( dma_spi_avmm_mem_readdatavalid    )

    );
    
    // TPM CS, just pass thorugh when you aren't the SPI master and block it when you are
    assign SPI_PFR_TPM_CS_R2_N = (FM_SPI_PFR_PCH_MASTER_SEL_R) ? 1: SPI_PCH_TPM_CS_N;
    // Other CS signals. Not currently used ????
    assign SPI_PFR_PCH_SECURE_CS1_N = SPI_PCH_CS1_N;

    // implement tri-state drivers and input connections for SPI master data pins
    assign bmc_spi_master_data_in       = {SPI_PFR_BMC_BOOT_R_IO3, SPI_PFR_BMC_BOOT_R_IO2, SPI_PFR_BMC_FLASH1_BT_MISO, SPI_PFR_BMC_FLASH1_BT_MOSI};
    assign SPI_PFR_BMC_BOOT_R_IO3       = bmc_spi_master_data_oe[3] ? bmc_spi_master_data_out[3] : 1'bz;
    assign SPI_PFR_BMC_BOOT_R_IO2       = bmc_spi_master_data_oe[2] ? bmc_spi_master_data_out[2] : 1'bz;
    assign SPI_PFR_BMC_FLASH1_BT_MISO   = bmc_spi_master_data_oe[1] ? bmc_spi_master_data_out[1] : 1'bz;
    assign SPI_PFR_BMC_FLASH1_BT_MOSI   = bmc_spi_master_data_oe[0] ? bmc_spi_master_data_out[0] : 1'bz;
    assign pch_spi_master_data_in       = {SPI_PFR_PCH_R_IO3, SPI_PFR_PCH_R_IO2, SPI_PFR_PCH_R_IO1, SPI_PFR_PCH_R_IO0};
    assign SPI_PFR_PCH_R_IO3            = pch_spi_master_data_oe[3] ? pch_spi_master_data_out[3] : 1'bz;
    assign SPI_PFR_PCH_R_IO2            = pch_spi_master_data_oe[2] ? pch_spi_master_data_out[2] : 1'bz;
    assign SPI_PFR_PCH_R_IO1            = pch_spi_master_data_oe[1] ? pch_spi_master_data_out[1] : 1'bz;
    assign SPI_PFR_PCH_R_IO0            = pch_spi_master_data_oe[0] ? pch_spi_master_data_out[0] : 1'bz;

    // FM_SPI_PFR_PCH_MASTER_SEL_R: 0 -> PCH, 1 -> CPLD  
    // ---------------------------------------------------------------------------------------------------------
    // RST_RSMRST_PLD_R_N | FM_SPI_PFR_PCH_MASTER_SEL_R  |   Descption
    // ---------------------------------------------------------------------------------------------------------
    // 0                  |    0                         |   Platform reset, pass through
    // 0                  |    1                         |   CPLD goes to PCH tmins 1 flow, pass through
    // 1                  |    0                         |   PCH is up, pass through
    // 1                  |    1                         |   Seamless update, hold it asserted if pltrst happen
    assign cc_RST_PLTRST_PLD_N = (RST_RSMRST_PLD_R_N & FM_SPI_PFR_PCH_MASTER_SEL_R & hold_pltrst_in_seamless) ? 1'b0: RST_PLTRST_PLD_N;

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
    logic smbus_relay1_slave_scl_in;
    logic smbus_relay1_slave_scl_oe;
    logic smbus_relay1_slave_sda_in;
    logic smbus_relay1_slave_sda_oe;
    assign smbus_relay1_master_scl_in = SMB_PMBUS_SML1_STBY_LVC3_SCL;
    assign smbus_relay1_master_sda_in = SMB_PMBUS_SML1_STBY_LVC3_SDA;
    assign smbus_relay1_slave_scl_in  = SMB_PFR_PMB1_STBY_LVC3_SCL;
    assign smbus_relay1_slave_sda_in  = SMB_PFR_PMB1_STBY_LVC3_SDA;
    // this is to keep the bus busy so that host will not transmit transaction when RFNVRAM master is accessing the bus
    assign SMB_PMBUS_SML1_STBY_LVC3_SCL = ((rfnvram_scl_oe && (slave_portid == 3'b100)) || smbus_relay1_master_scl_oe) ? 1'b0 : 1'bz;
    assign SMB_PMBUS_SML1_STBY_LVC3_SDA = ((rfnvram_sda_oe && (slave_portid == 3'b100)) || smbus_relay1_master_sda_oe) ? 1'b0 : 1'bz;
    assign SMB_PFR_PMB1_STBY_LVC3_SCL   = smbus_relay1_slave_scl_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_PMB1_STBY_LVC3_SDA   = smbus_relay1_slave_sda_oe ? 1'b0 : 1'bz;
    
    // Instantiate the relay block with no address or command filtering enabled
    smbus_filtered_relay #(
        .FILTER_ENABLE              ( 1                                                 ),      // enable command filtering
        .CLOCK_PERIOD_PS            ( platform_defs_pkg::SYS_CLOCK_PERIOD_PS            ),
        .BUS_SPEED_KHZ              ( platform_defs_pkg::RELAY1_BUS_SPEED_KHZ           ),
        .NUM_RELAY_ADDRESSES        ( gen_smbus_relay_config_pkg::RELAY1_NUM_ADDRESSES  ),
        .SMBUS_RELAY_ADDRESS        ( gen_smbus_relay_config_pkg::RELAY1_I2C_ADDRESSES  ),
        .SCL_LOW_TIMEOUT_PERIOD_MS  ( platform_defs_pkg::SCL_LOW_TIMEOUT_PERIOD_MS      ),
        .MCTP_OUT_OF_BAND_ENABLE    ( 0                                                 )       //mctp out of band feature turned off by default
    ) u_smbus_filtered_relay_1 (
        .clock                 ( sys_clk                       ),
        .i_resetn              ( sys_clk_reset_sync_n          ),
        .i_block_disable       ( relay1_block_disable          ),
        .i_filter_disable      ( relay1_filter_disable         ),
        .i_relay_all_addresses ( relay1_all_addresses          ),
        .ia_master_scl         ( smbus_relay1_master_scl_in    ),
        .o_master_scl_oe       ( smbus_relay1_master_scl_oe    ),
        .ia_master_sda         ( smbus_relay1_master_sda_in    ),
        .o_master_sda_oe       ( smbus_relay1_master_sda_oe    ),
        .ia_slave_scl          ( smbus_relay1_slave_scl_in     ),
        .o_slave_scl_oe        ( smbus_relay1_slave_scl_oe     ),
        .ia_slave_sda          ( smbus_relay1_slave_sda_in     ),
        .o_slave_sda_oe        ( smbus_relay1_slave_sda_oe     ),
        .i_avmm_write          ( relay1_avmm_write             ),
        .i_avmm_address        ( relay1_avmm_address           ),
        .i_avmm_writedata      ( relay1_avmm_writedata         )
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
    logic smbus_relay2_slave_scl_in;
    logic smbus_relay2_slave_scl_oe;
    logic smbus_relay2_slave_sda_in;
    logic smbus_relay2_slave_sda_oe;
    assign smbus_relay2_master_scl_in = SMB_PCH_PMBUS2_STBY_LVC3_SCL;
    assign smbus_relay2_master_sda_in = SMB_PCH_PMBUS2_STBY_LVC3_SDA;
    assign smbus_relay2_slave_scl_in  = SMB_PFR_PMBUS2_STBY_LVC3_R_SCL;
    assign smbus_relay2_slave_sda_in  = SMB_PFR_PMBUS2_STBY_LVC3_R_SDA;
    assign SMB_PCH_PMBUS2_STBY_LVC3_SCL = ((rfnvram_scl_oe && (slave_portid == 3'b101)) || smbus_relay2_master_scl_oe) ? 1'b0 : 1'bz;
    assign SMB_PCH_PMBUS2_STBY_LVC3_SDA = ((rfnvram_sda_oe && (slave_portid == 3'b101)) || smbus_relay2_master_sda_oe) ? 1'b0 : 1'bz;  // common core SMBus register file needs to be able to drive this open-drain signal as well
    assign SMB_PFR_PMBUS2_STBY_LVC3_R_SCL = smbus_relay2_slave_scl_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_PMBUS2_STBY_LVC3_R_SDA = smbus_relay2_slave_sda_oe ? 1'b0 : 1'bz;
    
    // Instantiate the relay block with no address or command filtering enabled
    smbus_filtered_relay #(
        .FILTER_ENABLE              ( 1                                                 ),      // enable command filtering
        .CLOCK_PERIOD_PS            ( platform_defs_pkg::SYS_CLOCK_PERIOD_PS            ),
        .BUS_SPEED_KHZ              ( platform_defs_pkg::RELAY2_BUS_SPEED_KHZ           ),
        .NUM_RELAY_ADDRESSES        ( gen_smbus_relay_config_pkg::RELAY2_NUM_ADDRESSES  ),
        .SMBUS_RELAY_ADDRESS        ( gen_smbus_relay_config_pkg::RELAY2_I2C_ADDRESSES  ),
        .SCL_LOW_TIMEOUT_PERIOD_MS  ( platform_defs_pkg::SCL_LOW_TIMEOUT_PERIOD_MS      ),
        .MCTP_OUT_OF_BAND_ENABLE    ( 0                                                 )       //mctp out of band feature turned off by default
    ) u_smbus_filtered_relay_2 (
        .clock                 ( sys_clk                       ),
        .i_resetn              ( sys_clk_reset_sync_n          ),
        .i_block_disable       ( relay2_block_disable          ),
        .i_filter_disable      ( relay2_filter_disable         ),
        .i_relay_all_addresses ( relay2_all_addresses          ),
        .ia_master_scl         ( smbus_relay2_master_scl_in    ),
        .o_master_scl_oe       ( smbus_relay2_master_scl_oe    ),
        .ia_master_sda         ( smbus_relay2_master_sda_in    ),
        .o_master_sda_oe       ( smbus_relay2_master_sda_oe    ),
        .ia_slave_scl          ( smbus_relay2_slave_scl_in     ),
        .o_slave_scl_oe        ( smbus_relay2_slave_scl_oe     ),
        .ia_slave_sda          ( smbus_relay2_slave_sda_in     ),
        .o_slave_sda_oe        ( smbus_relay2_slave_sda_oe     ),
        .i_avmm_write          ( relay2_avmm_write             ),
        .i_avmm_address        ( relay2_avmm_address           ),
        .i_avmm_writedata      ( relay2_avmm_writedata         )
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
    logic smbus_relay3_slave_scl_in;
    logic smbus_relay3_slave_scl_oe;
    logic smbus_relay3_slave_sda_in;
    logic smbus_relay3_slave_sda_oe;
    assign smbus_relay3_master_scl_in = SMB_BMC_HSBP_STBY_LVC3_SCL;
    assign smbus_relay3_master_sda_in = SMB_BMC_HSBP_STBY_LVC3_SDA;
    assign smbus_relay3_slave_scl_in  = SMB_PFR_HSBP_STBY_LVC3_SCL;
    assign smbus_relay3_slave_sda_in  = SMB_PFR_HSBP_STBY_LVC3_SDA;
    assign SMB_BMC_HSBP_STBY_LVC3_SCL = ((rfnvram_scl_oe && (slave_portid == 3'b110)) || smbus_relay3_master_scl_oe) ? 1'b0 : 1'bz;
    assign SMB_BMC_HSBP_STBY_LVC3_SDA = ((rfnvram_sda_oe && (slave_portid == 3'b110)) || smbus_relay3_master_sda_oe) ? 1'b0 : 1'bz;
    assign SMB_PFR_HSBP_STBY_LVC3_SCL = smbus_relay3_slave_scl_oe ? 1'b0 : 1'bz;
    assign SMB_PFR_HSBP_STBY_LVC3_SDA   = smbus_relay3_slave_sda_oe ? 1'b0 : 1'bz;
    
    // Instantiate the relay block with no address or command filtering enabled
    smbus_filtered_relay #(
        .FILTER_ENABLE              ( 1                                                 ),      // enable command filtering
        .CLOCK_PERIOD_PS            ( platform_defs_pkg::SYS_CLOCK_PERIOD_PS            ),
        .BUS_SPEED_KHZ              ( platform_defs_pkg::RELAY3_BUS_SPEED_KHZ           ),
        .NUM_RELAY_ADDRESSES        ( gen_smbus_relay_config_pkg::RELAY3_NUM_ADDRESSES  ),
        .SMBUS_RELAY_ADDRESS        ( gen_smbus_relay_config_pkg::RELAY3_I2C_ADDRESSES  ),
        .SCL_LOW_TIMEOUT_PERIOD_MS  ( platform_defs_pkg::SCL_LOW_TIMEOUT_PERIOD_MS      ),
        .MCTP_OUT_OF_BAND_ENABLE    ( 1                                                 )       //mctp out of band feature turned on for BMC relay only
    ) u_smbus_filtered_relay_3 (
        .clock                 ( sys_clk                       ),
        .i_resetn              ( sys_clk_reset_sync_n          ),
        .i_block_disable       ( relay3_block_disable          ),
        .i_filter_disable      ( relay3_filter_disable         ),
        .i_relay_all_addresses ( relay3_all_addresses          ),
        .ia_master_scl         ( smbus_relay3_master_scl_in    ),
        .o_master_scl_oe       ( smbus_relay3_master_scl_oe    ),
        .ia_master_sda         ( smbus_relay3_master_sda_in    ),
        .o_master_sda_oe       ( smbus_relay3_master_sda_oe    ),
        .ia_slave_scl          ( smbus_relay3_slave_scl_in     ),
        .o_slave_scl_oe        ( smbus_relay3_slave_scl_oe     ),
        .ia_slave_sda          ( smbus_relay3_slave_sda_in     ),
        .o_slave_sda_oe        ( smbus_relay3_slave_sda_oe     ),
        .i_avmm_write          ( relay3_avmm_write             ),
        .i_avmm_address        ( relay3_avmm_address           ),
        .i_avmm_writedata      ( relay3_avmm_writedata         )
    );


    //#########################################################################
    // LED Control
    //#########################################################################
    logic [6:0] pfr_seven_seg;
    //assign global_state = {4'hD, 4'hE};
    always_comb begin
        if (!sys_clk_reset_sync_n)
           pfr_seven_seg = 7'b1000000;
        else
            case (ccFM_POST_7SEG1_SEL_N ? global_state[3:0] : global_state[7:4])
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
        if (!FM_PFR_POSTCODE_SEL_N) begin
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
                LED_CONTROL_0 = 1'b1; // Green
                LED_CONTROL_1 = 1'b1; // Green
                LED_CONTROL_2 = 1'b1; // Green
                LED_CONTROL_3 = 1'b1; // Green 
                LED_CONTROL_4 = 1'b0; // Amber
                LED_CONTROL_5 = 1'b0; // Amber
                LED_CONTROL_6 = 1'b0; // Amber
                LED_CONTROL_7 = 1'b0; // Amber 
            end
        end
    end
    
    // Pass through the control signals
    assign FM_POST_7SEG1_SEL_N = ccFM_POST_7SEG1_SEL_N;
    assign FM_POST_7SEG2_SEL_N = ccFM_POST_7SEG2_SEL_N;
    assign FM_POSTLED_SEL = ccFM_POSTLED_SEL;


endmodule
    
