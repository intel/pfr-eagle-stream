`timescale 1 ps / 1 ps
`default_nettype none


// This is an emulator of the Wilson City wrapper

module Archer_City_Main_wrapper (

    // input wire CLK
    input   iClk_2M,
    input   iClk_50M,
    input   ipll_locked,

    output o1mSCE,
    output o1SCE,
    
    input wire  CLK_25M_CKMNG_MAIN_PLD, 

    //BMC SGPIO
    input wire  SGPIO_BMC_CLK,
    input wire  SGPIO_BMC_DOUT,
    output wire SGPIO_BMC_DIN_R,
    input wire  SGPIO_BMC_LD_N,

    //LED and 7-Seg Control Logic
    output wire LED_CONTROL_0,
    output wire LED_CONTROL_1,
    output wire LED_CONTROL_2,
    output wire LED_CONTROL_3,
    output wire LED_CONTROL_4,
    output wire LED_CONTROL_5,
    output wire LED_CONTROL_6,
    output wire LED_CONTROL_7,
    output wire FM_CPU0_DIMM_CH1_4_FAULT_LED_SEL,
    output wire FM_CPU0_DIMM_CH5_8_FAULT_LED_SEL,
    output wire FM_CPU1_DIMM_CH1_4_FAULT_LED_SEL,
    output wire FM_CPU1_DIMM_CH5_8_FAULT_LED_SEL,
    output wire FM_FAN_FAULT_LED_SEL_R_N,
    output wire FM_POST_7SEG1_SEL_N,
    output wire FM_POST_7SEG2_SEL_N,
    output wire FM_POSTLED_SEL,

    //CATERR / RMCA / NMI Logic
    output wire FM_CPU_RMCA_CATERR_DLY_LVT3_R_N,
    input wire  H_CPU_CATERR_LVC1_R_N,
    input wire  H_CPU_RMCA_LVC1_R_N,
    output wire FM_CPU_RMCA_CATERR_LVT3_N,
    output wire H_CPU_NMI_LVC1_R_N,
    input wire  IRQ_BMC_CPU_NMI,
    input wire  IRQ_PCH_CPU_NMI_EVENT_N,

    //ADR
    input wire  FM_ADR_COMPLETE,
    output wire FM_ADR_ACK_R,
    // output wire  FM_ADR_SMI_GPIO_N,
    output wire FM_ADR_TRIGGER_N,
    // input wire   FM_PLD_PCH_DATA_R,
    // input wire   FM_PS_PWROK_DLY_SEL,
    // input wire   FM_DIS_PS_PWROK_DLY,
    
    //SmaRT Logic
    input wire  FM_PMBUS_ALERT_B_EN,
    input wire  FM_THROTTLE_R_N,
    input wire  IRQ_SML1_PMBUS_PLD_ALERT_N,
    output wire FM_SYS_THROTTLE_R_N,
    
    //Termtrip dly
    input wire  H_CPU0_THERMTRIP_LVC1_N,
    input wire  H_CPU1_THERMTRIP_LVC1_N,
    output wire FM_THERMTRIP_DLY_LVC1_R_N,
    
    //MEMTRIP
    input wire  H_CPU0_MEMTRIP_LVC1_N,
    input wire  H_CPU1_MEMTRIP_LVC1_N,
    
    // PROCHOT
    input wire  IRQ_CPU0_VRHOT_N,
    input wire  IRQ_CPU1_VRHOT_N,
    output wire H_CPU0_PROCHOT_LVC1_R_N,
    output wire H_CPU1_PROCHOT_LVC1_R_N,
    
    //PERST & RST
    input wire  FM_RST_PERST_BIT0,
    input wire  FM_RST_PERST_BIT1,
    input wire  FM_RST_PERST_BIT2,
    output wire RST_PCIE_CPU0_DEV_PERST_N,
    output wire RST_PCIE_CPU1_DEV_PERST_N,
    output wire RST_PCIE_PCH_DEV_PERST_N,
    output wire RST_CPU0_LVC1_R_N,
    output wire RST_CPU1_LVC1_R_N,
    output wire RST_PLTRST_PLD_B_N,
    input wire  RST_PLTRST_PLD_N,
    
    //SysChk CPU
    input wire  FM_CPU0_PKGID0,
    input wire  FM_CPU0_PKGID1,
    input wire  FM_CPU0_PKGID2,
    input wire  FM_CPU0_PROC_ID0,
    input wire  FM_CPU0_PROC_ID1,
    input wire  FM_CPU0_INTR_PRSNT,
    input wire  FM_CPU0_SKTOCC_LVT3_PLD_N,
    input wire  FM_CPU1_PKGID0,
    input wire  FM_CPU1_PKGID1,
    input wire  FM_CPU1_PKGID2,
    input wire  FM_CPU1_PROC_ID0,
    input wire  FM_CPU1_PROC_ID1,
    input wire  FM_CPU1_SKTOCC_LVT3_PLD_N,
    
    
    //BMC
    input wire  FM_BMC_INTR_PRSNT,
    input wire  FM_BMC_PWRBTN_OUT_N,
    output wire FM_BMC_PFR_PWRBTN_OUT_R_N,
    input wire  FM_BMC_ONCTL_N_PLD,
    output logic RST_SRST_BMC_PLD_R_N,
    output wire FM_P2V5_BMC_AUX_EN,
    input wire  PWRGD_P2V5_BMC_AUX,
    output wire FM_P1V2_BMC_AUX_EN,
    input wire  PWRGD_P1V2_BMC_AUX,
    output wire FM_P1V0_BMC_AUX_EN,
    input wire  PWRGD_P1V0_BMC_AUX,
    //input wire    FP_BMC_PWR_BTN_R2_N,

    //PCH
    output logic RST_RSMRST_PLD_R_N,
    output logic PWRGD_PCH_PWROK_R,
    output logic PWRGD_SYS_PWROK_R,
    input wire  FM_SLP_SUS_RSM_RST_N,
    input wire  FM_SLPS3_PLD_N,
    input wire  FM_SLPS4_PLD_N,
    input wire  FM_PCH_PRSNT_N,
    output logic FM_PCH_P1V8_AUX_EN,
    input wire  PWRGD_P1V05_PCH_AUX,
    input wire  PWRGD_P1V8_PCH_AUX_PLD,
        
    output wire FM_P1V05_PCH_AUX_EN,
    output wire FM_PVNN_PCH_AUX_EN,
    input wire  PWRGD_PVNN_PCH_AUX,
    
    //PSU Ctl
    output wire FM_PS_EN_PLD_R,
    input wire  PWRGD_PS_PWROK_PLD_R,
    
    //Clock Logic
    output wire FM_PLD_CLKS_DEV_R_EN,
    output wire FM_PLD_CLK_25M_EN,
    
    //PLL reset Main FPGA
    input wire  PWRGD_P1V2_MAX10_AUX_R,  //PWRGD_P3V3_AUX in VRTB board
    
    //Main VR & Logic
    input wire  PWRGD_P3V3,
    output wire FM_P5V_EN,
    output wire FM_AUX_SW_EN,
    
    //CPU VRs
    output wire PWRGD_CPU0_LVC1_R2,
    output wire PWRGD_CPU1_LVC1_R,
    input wire  PWRGD_CPUPWRGD_LVC1,
    output wire PWRGD_PLT_AUX_CPU0_LVT3_R,
    output wire PWRGD_PLT_AUX_CPU1_LVT3_R,
        
    output wire FM_PVCCIN_CPU0_R_EN,
    input wire  PWRGD_PVCCIN_CPU0,
    output wire FM_PVCCFA_EHV_FIVRA_CPU0_R_EN,
    input wire  PWRGD_PVCCFA_EHV_FIVRA_CPU0,
    output wire FM_PVCCINFAON_CPU0_R_EN,
    input wire  PWRGD_PVCCINFAON_CPU0,
    output wire FM_PVCCFA_EHV_CPU0_R_EN,
    input wire  PWRGD_PVCCFA_EHV_CPU0,
    output wire FM_PVCCD_HV_CPU0_EN,
    input wire  PWRGD_PVCCD_HV_CPU0,
    output wire FM_PVNN_MAIN_CPU0_EN,
    input wire  PWRGD_PVNN_MAIN_CPU0,
    output wire FM_PVPP_HBM_CPU0_EN,
    input wire  PWRGD_PVPP_HBM_CPU0,
    output wire FM_PVCCIN_CPU1_R_EN,
    input wire  PWRGD_PVCCIN_CPU1,
    output wire FM_PVCCFA_EHV_FIVRA_CPU1_R_EN,
    input wire  PWRGD_PVCCFA_EHV_FIVRA_CPU1,
    output wire FM_PVCCINFAON_CPU1_R_EN,
    input wire  PWRGD_PVCCINFAON_CPU1,
    output wire FM_PVCCFA_EHV_CPU1_R_EN,
    input wire  PWRGD_PVCCFA_EHV_CPU1,
    output wire FM_PVCCD_HV_CPU1_EN,
    input wire  PWRGD_PVCCD_HV_CPU1,
    output wire FM_PVNN_MAIN_CPU1_EN,
    input wire  PWRGD_PVNN_MAIN_CPU1,
    output wire FM_PVPP_HBM_CPU1_EN,
    input wire  PWRGD_PVPP_HBM_CPU1,
    input wire  IRQ_PSYS_CRIT_N,
    
    //Dediprog Detection Support
    input wire  RST_DEDI_BUSY_PLD_N,

    //Debug Signals
    input wire  FM_FORCE_PWRON_LVC3,
    output wire FM_PLD_HEARTBEAT_LVC3,
    // output wire  SGPIO_DEBUG_PLD_CLK,
    // input wire   SGPIO_DEBUG_PLD_DIN,
    // output wire  SGPIO_DEBUG_PLD_DOUT,
    // output wire  SGPIO_DEBUG_PLD_LD_N,
    //input wire    SMB_DEBUG_PLD_SCL,
    //input wire    SMB_DEBUG_PLD_SDA,
    input wire  FM_PLD_REV_N,

    //PFR Dedicated Signals
    output wire FAN_BMC_PWM_R,
    output wire FM_PFR_DSW_PWROK_N,
    input wire  FM_ME_AUTHN_FAIL,
    input wire  FM_ME_BT_DONE,
    input wire  FM_PFR_TM1_HOLD_N,
    input wire  FM_PFR_POSTCODE_SEL_N,
    input wire  FM_PFR_FORCE_RECOVERY_N,
    output wire FM_PFR_MUX_OE_CTL_PLD,
    output wire RST_PFR_EXTRST_R_N,
    output wire RST_PFR_OVR_RTC_R,
    output wire RST_PFR_OVR_SRTC_R,
    output wire FM_PFR_ON_R,
    output wire FM_PFR_SLP_SUS_EN,
    output wire FP_ID_LED_PFR_N,
    input wire  FP_ID_LED_N,
    output wire FP_LED_STATUS_AMBER_PFR_N,
    input wire  FP_LED_STATUS_AMBER_N,
    output wire FP_LED_STATUS_GREEN_PFR_N,
    input wire  FP_LED_STATUS_GREEN_N,

    //SPI BYPASS TOP!!!
    output wire FM_SPI_PFR_PCH_MASTER_SEL_R,
    output wire FM_SPI_PFR_BMC_BT_MASTER_SEL_R,
    output wire RST_SPI_PFR_BMC_BOOT_N,
    output wire RST_SPI_PFR_PCH_N,
    input wire  SPI_BMC_BOOT_CS_N,
    output wire SPI_PFR_BMC_BT_SECURE_CS_R_N,
    input wire  SPI_PCH_PFR_CS0_N,
    output wire SPI_PFR_PCH_SECURE_CS0_R_N,
    input wire  SPI_PCH_CS1_N,
    output wire SPI_PFR_PCH_SECURE_CS1_N,
    input wire  SPI_BMC_BT_MUXED_MON_CLK,
    input wire  SPI_BMC_BT_MUXED_MON_MISO,
    input wire  SPI_BMC_BT_MUXED_MON_MOSI,
    input wire  SPI_BMC_BT_MUXED_MON_IO2,
    input wire  SPI_BMC_BT_MUXED_MON_IO3,
    input wire  SPI_PCH_MUXED_MON_CLK,
    input wire  SPI_PCH_MUXED_MON_IO0,
    input wire  SPI_PCH_MUXED_MON_IO1,
    input wire  SPI_PCH_MUXED_MON_IO2,
    input wire  SPI_PCH_MUXED_MON_IO3,
    input wire  SPI_PFR_BMC_FLASH1_BT_CLK,
    input wire  SPI_PFR_BMC_FLASH1_BT_MOSI,
    input wire  SPI_PFR_BMC_FLASH1_BT_MISO,
    input wire  SPI_PFR_BMC_BOOT_R_IO2,
    input wire  SPI_PFR_BMC_BOOT_R_IO3,
    input wire  SPI_PFR_PCH_R_CLK,
    input wire  SPI_PFR_PCH_R_IO0,
    input wire  SPI_PFR_PCH_R_IO1,
    input wire  SPI_PFR_PCH_R_IO2,
    input wire  SPI_PFR_PCH_R_IO3,

    //SMBus
    input wire  SMB_BMC_HSBP_STBY_LVC3_SCL,            //changed to input wire due to the SMBus bypass uses CLK-Stretching feature
    input wire  SMB_BMC_HSBP_STBY_LVC3_SDA,
    input wire  SMB_PFR_HSBP_STBY_LVC3_SCL,            //changed to input wire due to the SMBus BYPASS uses CLK-Stretching feature
    input wire  SMB_PFR_HSBP_STBY_LVC3_SDA,
    input wire  SMB_PFR_PMB1_STBY_LVC3_SCL,            //changed to input wire due to the SMBus BYPASS uses CLK-Stretching feature
    input wire  SMB_PFR_PMB1_STBY_LVC3_SDA,
    input wire  SMB_PMBUS_SML1_STBY_LVC3_SDA,
    input wire  SMB_PMBUS_SML1_STBY_LVC3_SCL,          //changed to input wire due to the SMBus BYPASS uses CLK-Stretching feature
    input wire  SMB_PCH_PMBUS2_STBY_LVC3_SCL,
    input wire  SMB_PCH_PMBUS2_STBY_LVC3_SDA,
    input wire  SMB_PFR_PMBUS2_STBY_LVC3_SCL,          //changed to input wire due to the SMBus BYPASS uses CLK-Stretching feature
    input wire  SMB_PFR_PMBUS2_STBY_LVC3_SDA,
    input wire  SMB_PFR_RFID_STBY_LVC3_SCL,            //changed to input wire due to the SMBus BYPASS uses CLK-Stretching feature
    input wire  SMB_PFR_RFID_STBY_LVC3_SDA,

    //Deltas for PFR
    //input wire    SMB_S3M_CPU0_SDA_LVC1,
    //input wire    SMB_S3M_CPU0_SCL_LVC1,
    output wire H_S3M_CPU0_SMBUS_ALERT_LVC1_R_N,
    //input wire    SMB_S3M_CPU1_SDA_LVC1,
    //input wire    SMB_S3M_CPU1_SCL_LVC1,
    output wire H_S3M_CPU1_SMBUS_ALERT_LVC1_R_N,
    input wire  SMB_PCIE_STBY_LVC3_B_SDA,
    input wire  SMB_PCIE_STBY_LVC3_B_SCL,
    input wire  SPI_PCH_TPM_CS_N,
    output wire SPI_PFR_TPM_CS_R2_N,
    output wire FM_PFR_RNDGEN_AUX,

    //Crashlog & Crashdump
    input wire  FM_BMC_CRASHLOG_TRIG_N,
    output wire FM_PCH_CRASHLOG_TRIG_R_N,
    input wire  FM_PCH_PLD_GLB_RST_WARN_N,

    //Reset from CPU to VT to CPLD
    input wire  M_AB_CPU0_RESET_N,
    input wire  M_CD_CPU0_RESET_N,
    input wire  M_EF_CPU0_RESET_N,
    input wire  M_GH_CPU0_RESET_N,
    input wire  M_AB_CPU1_RESET_N,
    input wire  M_CD_CPU1_RESET_N,
    input wire  M_EF_CPU1_RESET_N,
    input wire  M_GH_CPU1_RESET_N,

    //Reset from CPLD to VT to DIMM
    output wire M_AB_CPU0_FPGA_RESET_R_N,
    output wire M_CD_CPU0_FPGA_RESET_R_N,
    output wire M_EF_CPU0_FPGA_RESET_R_N,
    output wire M_GH_CPU0_FPGA_RESET_R_N,
    output wire M_AB_CPU1_FPGA_RESET_R_N,
    output wire M_CD_CPU1_FPGA_RESET_R_N,
    output wire M_EF_CPU1_FPGA_RESET_R_N,
    output wire M_GH_CPU1_FPGA_RESET_R_N,
    //Prochot & Memhot
    input wire  IRQ_CPU0_MEM_VRHOT_N,
    input wire  IRQ_CPU1_MEM_VRHOT_N,

    output wire H_CPU0_MEMHOT_IN_LVC1_R_N,
    output wire H_CPU1_MEMHOT_IN_LVC1_R_N,

    input wire  H_CPU0_MEMHOT_OUT_LVC1_N,
    input wire  H_CPU1_MEMHOT_OUT_LVC1_N,

    input wire  PWRGD_FAIL_CPU0_AB,
    input wire  PWRGD_FAIL_CPU0_CD,
    input wire  PWRGD_FAIL_CPU0_EF,
    input wire  PWRGD_FAIL_CPU0_GH,
    input wire  PWRGD_FAIL_CPU1_AB,
    input wire  PWRGD_FAIL_CPU1_CD,
    input wire  PWRGD_FAIL_CPU1_EF,
    input wire  PWRGD_FAIL_CPU1_GH,

    output wire PWRGD_DRAMPWRGD_CPU0_AB_R_LVC1,
    output wire PWRGD_DRAMPWRGD_CPU0_CD_R_LVC1,
    output wire PWRGD_DRAMPWRGD_CPU0_EF_R_LVC1,
    output wire PWRGD_DRAMPWRGD_CPU0_GH_R_LVC1,
    output wire PWRGD_DRAMPWRGD_CPU1_AB_R_LVC1,
    output wire PWRGD_DRAMPWRGD_CPU1_CD_R_LVC1,
    output wire PWRGD_DRAMPWRGD_CPU1_EF_R_LVC1,
    output wire PWRGD_DRAMPWRGD_CPU1_GH_R_LVC1,

    //IBL
//  output wire FM_IBL_ADR_TRIGGER_PLD_R,
//  input wire  FM_IBL_ADR_COMPLETE_CPU0,
//  input wire  FM_IBL_ADR_ACK_CPU0_R,
//  output wire FM_IBL_WAKE_CPU0_R_N,
//  output wire FM_IBL_SYS_RST_CPU0_R_N,
//  output wire FM_IBL_PWRBTN_CPU0_R_N,
//  output wire FM_IBL_GRST_WARN_PLD_R_N,
//  input wire  FM_IBL_PLT_RST_SYNC_N,
//  input wire  FM_IBL_SLPS3_CPU0_N,
//  input wire  FM_IBL_SLPS4_CPU0_N,
//  input wire  FM_IBL_SLPS5_CPU0_N,
    input wire  FM_IBL_SLPS0_CPU0_N,

    //Errors
    input wire  H_CPU_ERR0_LVC1_N,
    input wire  H_CPU_ERR1_LVC1_N,
    input wire  H_CPU_ERR2_LVC1_N,
    output wire FM_CPU_ERR0_LVT3_N,
    output wire FM_CPU_ERR1_LVT3_N,
    output wire FM_CPU_ERR2_LVT3_N,
                             
    input wire  H_CPU0_MON_FAIL_PLD_LVC1_N,
    input wire  H_CPU1_MON_FAIL_PLD_LVC1_N,

    //eMMC
//  output wire FM_EMMC_PFR_R_EN,
//
//  input wire  EMMC_MUX_BMC_PFR_OUT_CMD,
//  input wire  EMMC_MUX_BMC_PFR_LVT3_R_CMD,
//  input wire  RST_EMMC_MUX_BMC_PFR_LVT3_R_N,
//  input wire  EMMC_MUX_BMC_PFR_LVT3_DATA0_R,
//  input wire  EMMC_MUX_BMC_PFR_LVT3_DATA1_R,
//
//  input wire  EMMC_MUX_BMC_PFR_LVT3_DATA2_R,
//
//  input wire  EMMC_MUX_BMC_PFR_LVT3_DATA3_R,
//  input wire  EMMC_MUX_BMC_PFR_LVT3_DATA4_R,
//  input wire  EMMC_MUX_BMC_PFR_LVT3_DATA5_R,
//  input wire  EMMC_MUX_BMC_PFR_LVT3_DATA6_R,
//  input wire  EMMC_MUX_BMC_PFR_LVT3_DATA7_R,
//
//  input wire  EMMC_MUX_BMC_PFR_LVT3_R_CLK,

    //EMG Val board
    // input wire   FM_VAL_BOARD_PRSNT_N,

    //clock gen

    //In-Field System Test 
//  input wire  H_CPU0_IFST_DONE_PLD_LVC1,
//  input wire  H_CPU0_IFST_TRIG_PLD_LVC1,
//  input wire  H_CPU1_IFST_DONE_PLD_LVC1,
//  input wire  H_CPU1_IFST_TRIG_PLD_LVC1,
//  output wire H_CPU0_IFST_KOT_PLD_LVC1,
//  output wire H_CPU1_IFST_KOT_PLD_LVC1,

    //S3 Pwr-seq control
    output wire FM_SX_SW_P12V_STBY_R_EN,
    output wire FM_SX_SW_P12V_R_EN,
    input wire  FM_DIMM_12V_CPS_S5_N,

    // IDV Tool SGPIO
    // input wire   SGPIO_IDV_CLK_R,
    // input wire   SGPIO_IDV_DOUT_R,
    // output wire  SGPIO_IDV_DIN_R,
    // input wire   SGPIO_IDV_LD_R_N

    // Interposer PFR reuse ports
    input wire  SMB_S3M_CPU0_SDA_LVC1,
    input wire  SMB_S3M_CPU0_SCL_LVC1,
    input wire  SMB_S3M_CPU1_SDA_LVC1,
    input wire  SMB_S3M_CPU1_SCL_LVC1
);


initial begin
    RST_RSMRST_PLD_R_N = 1'b0;
    RST_SRST_BMC_PLD_R_N = 1'b0;
    PWRGD_PCH_PWROK_R = 1'b0;
    PWRGD_SYS_PWROK_R = 1'b0;
    FM_PCH_P1V8_AUX_EN = 1'b0;

    #1us FM_PCH_P1V8_AUX_EN = 1'b1;
    @(posedge PWRGD_P1V8_PCH_AUX_PLD);

    @(posedge FM_SLPS3_PLD_N);
    #1us RST_RSMRST_PLD_R_N = 1'b1;
    #500ps RST_SRST_BMC_PLD_R_N = 1'b1;

    #500ps PWRGD_PCH_PWROK_R = 1'b1;
    #500ps PWRGD_SYS_PWROK_R = 1'b1;
end

endmodule