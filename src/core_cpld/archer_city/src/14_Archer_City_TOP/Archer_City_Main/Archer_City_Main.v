module Archer_City_Main (
    // Input CLK
    input   iClk_2M,
    input   iClk_50M,
    input   iRST_N,

    // LED CONTROL
    output [7:0] oLED_CONTROL,           //data for Status/Fan-Fault/CPU-DIMMs LEDs and 7-Seg Displays 1&2
    output  oFM_POSTLED_SEL,             //to select LEDs
    output  oPostCodeSel1_n,             //to select Display1 (MSNibble)
    output  oPostCodeSel2_n,             //to select Display2 (LSNibble)
    output  oLedFanFaultSel_n,           //to select Fan Fault LEDs
    output  oFmCpu0DimmCh14FaultLedSel,  //to select CPU0 DIMMs 1&2 CH 1-4 fault LEDs
    output  oFmCpu0DimmCh58FaultLedSel,  //to select CPU0 DIMMs 1&2 CH 5-8 fault LEDs
    output  oFmCpu1DimmCh14FaultLedSel,  //to select CPU1 DIMMs 1&2 CH 1-4 fault LEDs
    output  oFmCpu1DimmCh58FaultLedSel,  //to select CPU1 DIMMs 1&2 CH 5-8 fault LEDs

    //Force Power On (VRs)
    input   iFM_FORCE_PWRON,        //static-signal from jumper (FM_FORCE_PWRON_LVC3)

    //Termtrip dly
    input   H_CPU0_THERMTRIP_LVC1_N,
    input   H_CPU1_THERMTRIP_LVC1_N,
    output  FM_THERMTRIP_DLY_LVC1_N,

    //MEMTRIP
    input   H_CPU0_MEMTRIP_LVC1_N,
    input   H_CPU1_MEMTRIP_LVC1_N,

    // PROCHOT
    input   IRQ_CPU0_VRHOT_N,
    input   IRQ_CPU1_VRHOT_N,
    output  H_CPU0_PROCHOT_LVC1_N,
    output  H_CPU1_PROCHOT_LVC1_N,

    //GSX Interface with BMC
    input   SGPIO_BMC_CLK,
    input   SGPIO_BMC_DOUT,
    output  SGPIO_BMC_DIN,
    input   SGPIO_BMC_LD_N,

    //PERST & RST
    input   FM_RST_PERST_BIT0,      // No-Input FF
    input   FM_RST_PERST_BIT1,      // No-Input FF
    input   FM_RST_PERST_BIT2,      // No-Input FF

    output  RST_PCIE_PERST0_N,
    output  RST_PCIE_PERST1_N,
    output  RST_PCIE_PERST2_N,

    output  RST_CPU0_LVC1_N,
    output  RST_CPU1_LVC1_N,

    output  RST_PLTRST_B_N,
    input   RST_PLTRST_N,           // Input FF

    //CPU Misc
    input   FM_CPU0_PKGID0,         // No-Input FF
    input   FM_CPU0_PKGID1,         // No-Input FF
    input   FM_CPU0_PKGID2,         // No-Input FF
    input   FM_CPU0_PROC_ID0,       // No-Input FF
    input   FM_CPU0_PROC_ID1,       // No-Input FF
    input   FM_CPU0_SKTOCC_LVT3_N,  // No-Input FF
    input   FM_CPU1_PKGID0,         // No-Input FF
    input   FM_CPU1_PKGID1,         // No-Input FF
    input   FM_CPU1_PKGID2,         // No-Input FF
    input   FM_CPU1_PROC_ID0,       // No-Input FF
    input   FM_CPU1_PROC_ID1,       // No-Input FF
    input   FM_CPU1_SKTOCC_LVT3_N,  // No-Input FF
    input   FM_CPU0_INTR_PRSNT,     // No-Input FF

    //BMC
    input   FM_BMC_INTR_PRSNT,
    input   FM_BMC_PWRBTN_OUT_N,
//    input   FP_BMC_PWR_BTN_N,       // Input FF
    output  FM_BMC_PFR_PWRBTN_OUT_N,
    input   FM_BMC_ONCTL_N,           // Input FF
    output  RST_SRST_BMC_PLD_N,


    output  FM_P2V5_BMC_AUX_EN,
    input   PWRGD_P2V5_BMC_AUX,     // Input FF
    output  FM_P1V2_BMC_AUX_EN,
    input   PWRGD_P1V2_BMC_AUX,     // Input FF
    output  FM_P1V0_BMC_AUX_EN,
    input   PWRGD_P1V0_BMC_AUX,     // Input FF

    //PCH
    output  RST_RSMRST_N,

    output  PWRGD_PCH_PWROK,
    output  PWRGD_SYS_PWROK,

    input   FM_SLP_SUS_RSM_RST_N,   // Input FF
    input   FM_SLPS3_N,             // Input FF
    input   FM_SLPS4_N,             // Input FF
    input   FM_PCH_PRSNT_N,         // No-Input FF

    output  FM_PCH_P1V8_AUX_EN,
    output  FM_P1V05_PCH_AUX_EN,
    output  FM_PVNN_PCH_AUX_EN,
    input   PWRGD_P1V8_PCH_AUX,     // Input FF
    input   PWRGD_P1V05_PCH_AUX,    // Input FF
    input   PWRGD_PVNN_PCH_AUX,     // Input FF

    //Dediprog Detection Support
    input   RST_DEDI_BUSY_PLD_N,

    //PSU Ctl
    output  FM_PS_EN,
    input   PWRGD_PS_PWROK,         // Input FF

    //Clock Logic
    output  FM_PLD_CLKS_DEV_EN,
    output  FM_PLD_CLK_25M_EN,

    //Base Logic
    input   PWRGD_P3V3_AUX,         // Input FF

    //Main VR & Logic
    input   PWRGD_P3V3,             // Input FF
    output  FM_P5V_EN,
    output  FM_AUX_SW_EN,

    //Mem

    //CPU VRs
    output  PWRGD_CPU0_LVC1,
    output  PWRGD_CPU1_LVC1,

    input   PWRGD_CPUPWRGD_LVC1,        // Input FF
    output  PWRGD_PLT_AUX_CPU0_LVT3,
    output  PWRGD_PLT_AUX_CPU1_LVT3,

    output  FM_PVCCIN_CPU0_EN,
    input   PWRGD_PVCCIN_CPU0,          // Input FF

    output  FM_PVCCFA_EHV_FIVRA_CPU0_EN,
    input   PWRGD_PVCCFA_EHV_FIVRA_CPU0,// Input FF

    output  FM_PVCCINFAON_CPU0_EN,
    input   PWRGD_PVCCINFAON_CPU0,      // Input FF

    output  FM_PVCCFA_EHV_CPU0_EN,
    input   PWRGD_PVCCFA_EHV_CPU0,      // Input FF

    output  FM_PVCCD_HV_CPU0_EN,
    input   PWRGD_PVCCD_HV_CPU0,        // Input FF

    output  FM_PVNN_MAIN_CPU0_EN,
    input   PWRGD_PVNN_MAIN_CPU0,       // Input FF

    output  FM_PVPP_HBM_CPU0_EN,
    input   PWRGD_PVPP_HBM_CPU0,        // Input FF

    output  FM_PVCCIN_CPU1_EN,
    input   PWRGD_PVCCIN_CPU1,          // Input FF

    output  FM_PVCCFA_EHV_FIVRA_CPU1_EN,
    input   PWRGD_PVCCFA_EHV_FIVRA_CPU1,// Input FF

    output  FM_PVCCINFAON_CPU1_EN,
    input   PWRGD_PVCCINFAON_CPU1,      // Input FF

    output  FM_PVCCFA_EHV_CPU1_EN,
    input   PWRGD_PVCCFA_EHV_CPU1,      // Input FF

    output  FM_PVCCD_HV_CPU1_EN,
    input   PWRGD_PVCCD_HV_CPU1,        // Input FF

    output  FM_PVNN_MAIN_CPU1_EN,
    input   PWRGD_PVNN_MAIN_CPU1,       // Input FF

    output  FM_PVPP_HBM_CPU1_EN,
    input   PWRGD_PVPP_HBM_CPU1,        // Input FF
    input   IRQ_PSYS_CRIT_N,            // No-Input FF

    //Debug signals
    output  FM_PLD_HEARTBEAT_LVC3,     //Heart beat led
    input   FM_PLD_REV_N,

    //Reset from CPU to VT to CPLD
    input   M_AB_CPU0_RESET_N,          // No-Input FF
    input   M_CD_CPU0_RESET_N,          // No-Input FF
    input   M_EF_CPU0_RESET_N,          // No-Input FF
    input   M_GH_CPU0_RESET_N,          // No-Input FF
    input   M_AB_CPU1_RESET_N,          // No-Input FF
    input   M_CD_CPU1_RESET_N,          // No-Input FF
    input   M_EF_CPU1_RESET_N,          // No-Input FF
    input   M_GH_CPU1_RESET_N,          // No-Input FF

    //Reset from CPLD to VT to DIMM
    output  M_AB_CPU0_FPGA_RESET_N,
    output  M_CD_CPU0_FPGA_RESET_N,
    output  M_EF_CPU0_FPGA_RESET_N,
    output  M_GH_CPU0_FPGA_RESET_N,
    output  M_AB_CPU1_FPGA_RESET_N,
    output  M_CD_CPU1_FPGA_RESET_N,
    output  M_EF_CPU1_FPGA_RESET_N,
    output  M_GH_CPU1_FPGA_RESET_N,
    //Prochot & Memhot
    input   IRQ_CPU0_MEM_VRHOT_N,       // No-Input FF
    input   IRQ_CPU1_MEM_VRHOT_N,       // No-Input FF

    output  H_CPU0_MEMHOT_IN_LVC1_N,
    output  H_CPU1_MEMHOT_IN_LVC1_N,

    input   H_CPU0_MEMHOT_OUT_LVC1_N,
    input   H_CPU1_MEMHOT_OUT_LVC1_N,

    inout   PWRGD_FAIL_CPU0_AB,
    inout   PWRGD_FAIL_CPU0_CD,
    inout   PWRGD_FAIL_CPU0_EF,
    inout   PWRGD_FAIL_CPU0_GH,
    inout   PWRGD_FAIL_CPU1_AB,
    inout   PWRGD_FAIL_CPU1_CD,
    inout   PWRGD_FAIL_CPU1_EF,
    inout   PWRGD_FAIL_CPU1_GH,

    output  PWRGD_DRAMPWRGD_CPU0_AB_LVC1,
    output  PWRGD_DRAMPWRGD_CPU0_CD_LVC1,
    output  PWRGD_DRAMPWRGD_CPU0_EF_LVC1,
    output  PWRGD_DRAMPWRGD_CPU0_GH_LVC1,
    output  PWRGD_DRAMPWRGD_CPU1_AB_LVC1,
    output  PWRGD_DRAMPWRGD_CPU1_CD_LVC1,
    output  PWRGD_DRAMPWRGD_CPU1_EF_LVC1,
    output  PWRGD_DRAMPWRGD_CPU1_GH_LVC1,

    //Errors
    input   H_CPU_ERR0_LVC1_N,
    input   H_CPU_ERR1_LVC1_N,
    input   H_CPU_ERR2_LVC1_N,
    output  FM_CPU_ERR0_LVT3_N,
    output  FM_CPU_ERR1_LVT3_N,
    output  FM_CPU_ERR2_LVT3_N,
    input   H_CPU0_MON_FAIL_PLD_LVC1_N,
    input   H_CPU1_MON_FAIL_PLD_LVC1_N,


    //Misc Logic for CPU errors, DMI & PCH Crash-log
    output oFM_CPU_RMCA_CATERR_DLY_N,
    input iCPU_CATERR_N,
    input iCPU_RMCA_N,
    output oFM_CPU_RMCA_CATERR_N,
    output oCPU_NMI_N,
    input iIRQ_BMC_CPU_NMI,
    input iIRQ_PCH_CPU_NMI_EVENT_N,
    input iFM_BMC_CRASHLOG_TRIG_N,
    output oFM_PCH_CRASHLOG_TRIG_N,
    input iFM_PCH_PLD_GLB_RST_WARN_N,

    //Smart Logic
    input iFM_PMBUS_ALERT_B_EN,
	input iFM_THROTTLE_R_N,
	input iIRQ_SML1_PMBUS_PLD_ALERT_N,
	output oFM_SYS_THROTTLE_N,

    //CLX interposer VR pwrgd signal (VTT rail)
    input iPWRGD_VTT_PWRGD,

    //S3 Pwr-seq control
    output  FM_SX_SW_P12V_STBY_EN,
    output  FM_SX_SW_P12V_EN,
    input   FM_DIMM_12V_CPS_S5_N,       // No-Input FF


    //ADR signaling
    input   iFM_ADR_COMPLETE,          //from PCH, to indicate ADR process is completed
    output  oFM_ADR_TRIGGER_N,         //To PCH, to indicate a Surprise Pwr-Dwn is detected and ADR needs to start
    output  oFM_ADR_ACK,               //To PCH, once all is done in FPGA so PCH can start regular pwr-down

    //PFR signals
    output FM_PFR_MUX_OE_CTL_PLD,
    input iPFROverride,                //when asserted, PFR Post-Codes take precedence over BIOS or FPGA PostCodes
    input [7:0] iPFRPostCode,          //PFR PostCodes (Hex format, MSNible for Display1 and LSNible for Display2)
                                       //will be encoded inside and added Display2 dot for identification
    output FM_PFR_SLP_SUS_EN,

    // Interposer PFR reuse ports
    input   SMB_S3M_CPU0_SDA,
    input   SMB_S3M_CPU0_SCL,
    input   SMB_S3M_CPU1_SDA,
    input   SMB_S3M_CPU1_SCL

);


//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////
parameter  LOW =1'b0;
parameter  HIGH=1'b1;
parameter  FPGA_REV = 8'h02;
parameter  FPGA_REV_TEST = 8'h00;
parameter  FPGA_REV_DEBUG = 8'h00;
//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
//CPU SYS CHECK
wire    wSYS_OK;
wire    wCPU_MISMATCH;
wire    wHBM_EN;
wire    wSKT_REMOVED;

//PSU
wire    wPSU_EN;
wire    wPSU_PWR_FLT;
wire    wPSU_PWR_OK;

//BMC
wire    wBMC_EN;
wire    wBMC_PWR_FLT;
wire    wBMC_PWR_OK;
wire    wRST_SRST_BMC_PLD_N;

//PCH
wire    wPCH_EN;
wire    wPCH_PWR_FLT;
wire    wPCH_PWR_OK;
wire    wPCH_PWR_FLT_PVNN;
wire    wPCH_PWR_FLT_P1V05;
wire    wFM_P1V8_PCH_AUX_EN_BP;

//MEM
wire    wCPU_MEM_EN;
wire    wMEM0_PWR_FLT, wMEM1_PWR_FLT;
wire    wPWRGD_DRAMPWRGD_CPU1;
wire    wPWRGD_DRAMPWRGD_CPU0;

//CPU
wire    wCPU_MEM_PWR_OK, wCPU0_MEM_PWR_OK, wCPU1_MEM_PWR_OK;
wire    wFM_PVNN_MAIN_CPU1_EN;
wire    wFM_PVCCINFAON_CPU1_EN;
wire    wCPU_MEM_PWR_FLT, wCPU0_MEM_PWR_FLT, wCPU1_MEM_PWR_FLT;
wire    wPVNN_MAIN_CPU0_EN_BP;

wire    wPWRGD_PLT_AUX_CPU0_LVT3;
wire    wPWRGD_PLT_AUX_CPU1_LVT3;

wire    wBRS_PWRGD_PLT_AUX_CPU0_LVT3;
wire    wBRS_PWRGD_PLT_AUX_CPU1_LVT3;

// SYS THROTTLE
wire wFM_SYS_THROTTLE_N;

//Prohot & Memhot
wire wH_CPU0_MEMHOT_IN_LVC1_N;
wire wH_CPU1_MEMHOT_IN_LVC1_N;
wire wH_CPU0_PROCHOT_LVC1_N;
wire wH_CPU1_PROCHOT_LVC1_N;

//PLT STATES
wire    wS3_STATE;
wire    wS4_S5_STATE;

//DEDIPROG



//sync'ed signals
//   wire FP_BMC_PWR_BTN_N_FF;
   wire FM_BMC_ONCTL_N_FF;
   wire PWRGD_P2V5_BMC_AUX_FF;
   wire PWRGD_P1V2_BMC_AUX_FF;
   wire PWRGD_P1V0_BMC_AUX_FF;
   wire FM_SLP_SUS_RSM_RST_N_FF;
   wire FM_SLPS3_N_FF;
   wire FM_SLPS4_N_FF;
   wire PWRGD_P1V8_PCH_AUX_FF;
   wire PWRGD_P1V05_PCH_AUX_FF;
   wire PWRGD_PVNN_PCH_AUX_FF;
   wire PWRGD_PS_PWROK_FF;
   wire PWRGD_P3V3_AUX_FF;
   wire PWRGD_P3V3_FF;
   wire PWRGD_CPUPWRGD_LVC1_FF;
   wire PWRGD_PVCCIN_CPU0_FF;
   wire PWRGD_PVCCFA_EHV_FIVRA_CPU0_FF;
   wire PWRGD_PVCCINFAON_CPU0_FF;
   wire PWRGD_PVCCFA_EHV_CPU0_FF;
   wire PWRGD_PVCCD_HV_CPU0_FF;
   wire PWRGD_PVNN_MAIN_CPU0_FF;
   wire PWRGD_PVPP_HBM_CPU0_FF;
   wire PWRGD_PVCCIN_CPU1_FF;
   wire PWRGD_PVCCFA_EHV_FIVRA_CPU1_FF;
   wire PWRGD_PVCCINFAON_CPU1_FF;
   wire PWRGD_PVCCFA_EHV_CPU1_FF;
   wire PWRGD_PVCCD_HV_CPU1_FF;
   wire PWRGD_PVNN_MAIN_CPU1_FF;
   wire PWRGD_PVPP_HBM_CPU1_FF;
   wire RST_PLTRST_N_FF;

////////////////////////////////

//VR BYPASS signals (FORCED POWER ON VRs)


   wire wFM_PVNN_MAIN_CPU1_EN_MUX;
   wire wFM_PVCCINFAON_CPU1_EN_MUX;

   wire wFM_P2V5_BMC_AUX_EN, wFM_P2V5_BMC_AUX_EN_BP;
   wire wFM_PCH_P1V8_AUX_EN, wFM_PCH_P1V8_AUX_EN_BP;
   wire wFM_P1V2_BMC_AUX_EN, wFM_P1V2_BMC_AUX_EN_BP;
   wire wFM_P1V0_BMC_AUX_EN, wFM_P1V0_BMC_AUX_EN_BP;
   wire wFM_PCH_P1V05_AUX_EN, wFM_PCH_P1V05_AUX_EN_BP;
   wire wFM_PCH_PVNN_AUX_EN, wFM_PCH_PVNN_AUX_EN_BP;
   wire wFM_PS_EN, wFM_PS_EN_BP;
   wire wFM_P5V_EN, wFM_P5V_EN_BP;
   wire wFM_AUX_SW_EN, wFM_AUX_SW_EN_BP;
   wire wFM_SX_SW_P12V_STBY_EN, wFM_SX_SW_P12V_STBY_EN_BP;
   wire wFM_SX_SW_P12V_EN, wFM_SX_SW_P12V_EN_BP;
   wire wFM_PVPP_HBM_CPU0_EN, wFM_PVPP_HBM_CPU0_EN_BP;
   wire wFM_PVCCFA_EHV_CPU0_EN, wFM_PVCCFA_EHV_CPU0_EN_BP;
   wire wFM_PVCCFA_EHV_FIVRA_CPU0_EN, wFM_PVCCFA_EHV_FIVRA_CPU0_EN_BP;
   wire wFM_PVCCINFAON_CPU0_EN, wFM_PVCCINFAON_CPU0_EN_BP;
   wire wFM_PVNN_MAIN_CPU0_EN, wFM_PVNN_MAIN_CPU0_EN_BP;
   wire wFM_PVCCD_HV_CPU0_EN, wFM_PVCCD_HV_CPU0_EN_BP;
   wire wFM_PVCCIN_CPU0_EN, wFM_PVCCIN_CPU0_EN_BP;
   wire wFM_PVPP_HBM_CPU1_EN, wFM_PVPP_HBM_CPU1_EN_BP;
   wire wFM_PVCCFA_EHV_CPU1_EN, wFM_PVCCFA_EHV_CPU1_EN_BP;
   wire wFM_PVCCFA_EHV_FIVRA_CPU1_EN, wFM_PVCCFA_EHV_FIVRA_CPU1_EN_BP;
   wire wFM_PVCCINFAON_CPU1_EN_BP;
   wire wFM_PVNN_MAIN_CPU1_EN_BP;
   wire wFM_PVCCD_HV_CPU1_EN, wFM_PVCCD_HV_CPU1_EN_BP;
   wire wFM_PVCCIN_CPU1_EN, wFM_PVCCIN_CPU1_EN_BP;

   wire wFM_PLD_CLK_25M_EN, wFM_PLD_CLK_25M_EN_BP;
   wire wFM_PLD_CLKS_DEV_EN, wFM_PLD_CLKS_DEV_EN_BP;


   /////////////////////////////////////////////////////////////////////////////////
   //////////  FPGA POSTCODES
   /////////////////////////////////////////

   wire wCpu0PvppHbmFlt;
   wire wCpu0PvccfaEhvFlt;
   wire wCpu0PvccfaFivrFlt;
   wire wCpu0PvccInfaonFlt;
   wire wCpu0PvnnMainFlt;
   wire wCpu0PvccinFlt;
   wire wCpu0PvccdHvFlt;

   wire wCpu1PvppHbmFlt;
   wire wCpu1PvccfaEhvFlt;
   wire wCpu1PvccfaFivrFlt;
   wire wCpu1PvccInfaonFlt;
   wire wCpu1PvnnMainFlt;
   wire wCpu1PvccinFlt;
   wire wCpu1PvccdHvFlt;

   wire [4:0] wBmcFaultCode;
   wire [3:0] wPchFaultCode;
   wire [2:0] wPsuFaultCode;
   wire [7:0] wCpu0FaultCode;
   wire [7:0] wCpu1FaultCode;

   wire [3:0] wCpu0MemFlt;
   wire [3:0] wCpu1MemFlt;

   wire [8:0] wCpuMemFlt;

   /////////////////////////////////////////////////////////////////////////////////
   //////////  LED CONTROL
   /////////////////////////////////////////

   wire       wBMC_PWR_P2V5_FAULT;
   wire       wBMC_PWR_P1V8_FAULT;
   wire       wBMC_PWR_P1V2_FAULT;
   wire       wBMC_PWR_P1V0_FAULT;


   wire       wStatusledSel;
   wire [7:0] wBiosPostCode;
   wire [7:0] wFanFaultLEDs;

   wire [7:0] wFpgaPostCode1;
   wire [7:0] wFpgaPostCode2;

   wire [15:0] wCpu0DIMMLEDs;
   wire [15:0] wCpu1DIMMLEDs;

   wire [7:0]  wLED_STATUS;     //using this provisionally while it is defined, then signals coming from bypass, could be muxed or so


////////////////////////////////////////////////////////////////
//ADR wire declarations

   wire wAdrEna;
   wire wPwrGdFailCpuAB_adr;    //ADR PwrGdFail signals for 4 groups of 2 channels each group
   wire wPwrGdFailCpuCD_adr;    // There should be 2 sets but in ADR all signals move the same
   wire wPwrGdFailCpuEF_adr;    // so it can be reused for both CPUs, etc
   wire wPwrGdFailCpuGH_adr;
////////////////////////////////////////////////////////////////

   wire wPWRGD_DRAMPWRGD_CPU0_AB_LVC1;
   wire wPWRGD_DRAMPWRGD_CPU0_CD_LVC1;
   wire wPWRGD_DRAMPWRGD_CPU0_EF_LVC1;
   wire wPWRGD_DRAMPWRGD_CPU0_GH_LVC1;

	wire wPWRGD_DRAMPWRGD_CPU1_AB_LVC1;
   wire wPWRGD_DRAMPWRGD_CPU1_CD_LVC1;
   wire wPWRGD_DRAMPWRGD_CPU1_EF_LVC1;
   wire wPWRGD_DRAMPWRGD_CPU1_GH_LVC1;
////////////////////////////////////////////////////////////////
//for the 2msec clock signal
   wire wClk2ms;



//////////////////////////////////////////////////////////////////////////////////
//Even in Single CPU Config, PVNN MAIN CPU1 VR must be Enabled, in the case of interposer PVCCINFAON VR is repurposed for both CLX or BRS.
//EGS
assign wFM_PVNN_MAIN_CPU1_EN_MUX  = FM_CPU1_SKTOCC_LVT3_N ? wFM_PVNN_MAIN_CPU0_EN : wFM_PVNN_MAIN_CPU1_EN;
//CLX or BRS
assign wFM_PVCCINFAON_CPU1_EN_MUX = (!FM_CPU1_SKTOCC_LVT3_N && FM_CPU0_INTR_PRSNT) ? wFM_PVCCINFAON_CPU1_EN : FM_PVCCINFAON_CPU0_EN;

//////////////////////////////////////////////////////////////////////////////////
//Interposer repurposed BRS glue locic
//////////////////////////////////////////////////////////////////////////////////
assign PWRGD_PLT_AUX_CPU0_LVT3 = (!FM_CPU0_SKTOCC_LVT3_N && FM_CPU0_INTR_PRSNT && !FM_CPU1_PKGID2) ? wBRS_PWRGD_PLT_AUX_CPU0_LVT3 : wPWRGD_PLT_AUX_CPU0_LVT3;
assign PWRGD_PLT_AUX_CPU1_LVT3 = (!FM_CPU1_SKTOCC_LVT3_N && FM_CPU0_INTR_PRSNT && !FM_CPU1_PKGID2) ? wBRS_PWRGD_PLT_AUX_CPU1_LVT3 : wPWRGD_PLT_AUX_CPU1_LVT3;

//////////////////////////////////////////////////////////////////////////////////
// CPU Reset & PWRGD
//////////////////////////////////////////////////////////////////////////////////
assign PWRGD_CPU0_LVC1 = PWRGD_CPUPWRGD_LVC1_FF; // assign CPU PWRGD bypass, TODO: this logic can be encapsulated in pwrgd-rst logic module
assign PWRGD_CPU1_LVC1 = PWRGD_CPUPWRGD_LVC1_FF; // assign CPU PWRGD bypass, TODO: this logic can be encapsulated in pwrgd-rst logic module
assign RST_CPU0_LVC1_N = RST_PLTRST_N_FF;        // assign CPU RST bypass, TODO: this logic can be encapsulated in pwrgd-rst logic module
assign RST_CPU1_LVC1_N = RST_PLTRST_N_FF;        // assign CPU RST bypass, TODO: this logic can be encapsulated in pwrgd-rst logic module

//////////////////////////////////////////////////////////////////////////////////
// Glue Logic
//////////////////////////////////////////////////////////////////////////////////
assign wCPU_MEM_PWR_OK = wCPU0_MEM_PWR_OK && (wCPU1_MEM_PWR_OK || FM_CPU1_SKTOCC_LVT3_N);
assign wCPU_MEM_PWR_FLT= wCPU0_MEM_PWR_FLT || wCPU1_MEM_PWR_FLT || wMEM0_PWR_FLT || wMEM1_PWR_FLT;

assign RST_SRST_BMC_PLD_N = wRST_SRST_BMC_PLD_N;

//Prohot & Memhot
assign H_CPU0_PROCHOT_LVC1_N    = wH_CPU0_PROCHOT_LVC1_N;
assign H_CPU0_MEMHOT_IN_LVC1_N  = wH_CPU0_MEMHOT_IN_LVC1_N;
assign H_CPU1_PROCHOT_LVC1_N    = wH_CPU1_PROCHOT_LVC1_N;
assign H_CPU1_MEMHOT_IN_LVC1_N  = wH_CPU1_MEMHOT_IN_LVC1_N;

//////////////////////////////////////////////////////////////////////////////////
// BMC-PWRBTN BYPASS
//////////////////////////////////////////////////////////////////////////////////
assign FM_BMC_PFR_PWRBTN_OUT_N = FM_BMC_PWRBTN_OUT_N;

//////////////////////////////////////////////////////////////////////////////////
///  PCIe RST
//////////////////////////////////////////////////////////////////////////////////
wire wRST_PCIE_PERST2_N;
wire wRST_PCIE_PERST1_N;
wire wRST_PCIE_PERST0_N;

//////////////////////////////////////////////////////////////////////////////////
///  Debug signals
//////////////////////////////////////////////////////////////////////////////////
wire wHeartBeat;                                  //Heart-Beat
wire wFM_PLD_REV_N_FF;                            //Power Button clock

assign FM_PLD_HEARTBEAT_LVC3 = wHeartBeat;


//////////////////////////////////////////////////////////////////////////////////
///  Dediprog Support
//////////////////////////////////////////////////////////////////////////////////
reg   rRST_DEDI_BUSY_PLD_N;
reg   rDEDI_BUSY_LOCK;

assign FM_PFR_SLP_SUS_EN      = rDEDI_BUSY_LOCK ? HIGH : LOW;
assign FM_PFR_MUX_OE_CTL_PLD  = rDEDI_BUSY_LOCK ? LOW  : HIGH;

   always @ ( posedge iClk_2M)
     begin
        if (  !iRST_N  )
          begin
             rRST_DEDI_BUSY_PLD_N        <= LOW;
             rDEDI_BUSY_LOCK             <= LOW;
          end
        else
          begin
             rRST_DEDI_BUSY_PLD_N          <= RST_DEDI_BUSY_PLD_N;
             if (RST_RSMRST_N && (rRST_DEDI_BUSY_PLD_N && !RST_DEDI_BUSY_PLD_N))
               begin
                  rDEDI_BUSY_LOCK               <= HIGH;
               end
          end
     end //always



//this is used for the HeartBeat LED
// and as the slow clock signal for the PLD_REV button signal glitch-filter
ClkDiv HeartBeat250mSec
  (
   .iClk(iClk_2M),
   .iRst_n(iRST_N),
   .div_clk(wHeartBeat)
);

//This is used for the Led Control block as a 500Hz clock signal
ClkDiv #(.DIV(2000))   //based on a 2MHz clock this will deliver a 2msec (500Hz) clock signal
    ClkLC
      (
       .iClk(iClk_2M),
       .iRst_n(iRST_N),
       .div_clk(wClk2ms)
       );



GlitchFilter #                                  //(module for VRTB BYPASS ONLY)
(
    .NUMBER_OF_SIGNALS  (1)
) mGlitchFilter_PLD_BUTTON
(
    .iClk               (wHeartBeat),            //Used the 250mS to make button debouncer
    .iRst_n             (iRST_N),
    .iSignal            (FM_PLD_REV_N),
    .oFilteredSignals   (wFM_PLD_REV_N_FF)
);
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
// Clock generation and CE
//////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////
//Inputs Synchronizer
//////////////////////////////////////////////////////////////////////////////////
GlitchFilter #
(
    .NUMBER_OF_SIGNALS  (29)
) mGlitchFilter
(
    .iClk               (iClk_2M),
    .iRst_n             (iRST_N),
    .iSignal            ({
//                        FP_BMC_PWR_BTN_N,
                        FM_BMC_ONCTL_N,
                        PWRGD_P2V5_BMC_AUX,
                        PWRGD_P1V2_BMC_AUX,
                        PWRGD_P1V0_BMC_AUX,
                        FM_SLP_SUS_RSM_RST_N,
                        FM_SLPS3_N,
                        FM_SLPS4_N,
                        PWRGD_P1V8_PCH_AUX,
                        PWRGD_P1V05_PCH_AUX,
                        PWRGD_PVNN_PCH_AUX,
                        PWRGD_PS_PWROK,
                        PWRGD_P3V3_AUX,
                        PWRGD_P3V3,
                        PWRGD_CPUPWRGD_LVC1,
                        PWRGD_PVCCIN_CPU0,
                        PWRGD_PVCCFA_EHV_FIVRA_CPU0,
                        PWRGD_PVCCINFAON_CPU0,
                        PWRGD_PVCCFA_EHV_CPU0,
                        PWRGD_PVCCD_HV_CPU0,
                        PWRGD_PVNN_MAIN_CPU0,
                        PWRGD_PVPP_HBM_CPU0,
                        PWRGD_PVCCIN_CPU1,
                        PWRGD_PVCCFA_EHV_FIVRA_CPU1,
                        PWRGD_PVCCINFAON_CPU1,
                        PWRGD_PVCCFA_EHV_CPU1,
                        PWRGD_PVCCD_HV_CPU1,
                        PWRGD_PVNN_MAIN_CPU1,
                        PWRGD_PVPP_HBM_CPU1,
                        RST_PLTRST_N
        }),

.oFilteredSignals       ({
//                        FP_BMC_PWR_BTN_N_FF,
                        FM_BMC_ONCTL_N_FF,
                        PWRGD_P2V5_BMC_AUX_FF,
                        PWRGD_P1V2_BMC_AUX_FF,
                        PWRGD_P1V0_BMC_AUX_FF,
                        FM_SLP_SUS_RSM_RST_N_FF,
                        FM_SLPS3_N_FF,
                        FM_SLPS4_N_FF,
                        PWRGD_P1V8_PCH_AUX_FF,
                        PWRGD_P1V05_PCH_AUX_FF,
                        PWRGD_PVNN_PCH_AUX_FF,
                        PWRGD_PS_PWROK_FF,
                        PWRGD_P3V3_AUX_FF,
                        PWRGD_P3V3_FF,
                        PWRGD_CPUPWRGD_LVC1_FF,
                        PWRGD_PVCCIN_CPU0_FF,
                        PWRGD_PVCCFA_EHV_FIVRA_CPU0_FF,
                        PWRGD_PVCCINFAON_CPU0_FF,
                        PWRGD_PVCCFA_EHV_CPU0_FF,
                        PWRGD_PVCCD_HV_CPU0_FF,
                        PWRGD_PVNN_MAIN_CPU0_FF,
                        PWRGD_PVPP_HBM_CPU0_FF,
                        PWRGD_PVCCIN_CPU1_FF,
                        PWRGD_PVCCFA_EHV_FIVRA_CPU1_FF,
                        PWRGD_PVCCINFAON_CPU1_FF,
                        PWRGD_PVCCFA_EHV_CPU1_FF,
                        PWRGD_PVCCD_HV_CPU1_FF,
                        PWRGD_PVNN_MAIN_CPU1_FF,
                        PWRGD_PVPP_HBM_CPU1_FF,
                        RST_PLTRST_N_FF
    })
);


//////////////////////////////////////////////////////////////////////////////////
// Power Secuence
//////////////////////////////////////////////////////////////////////////////////

ac_sys_check mSys_Check
(
// ------------------------
// Clock and Reset signals
// ------------------------
    .iClk(iClk_2M),               //clock for sequential logic
    .iRst_n(iRST_N),              //reset signal from PLL Lock, resets state machine to initial state
// ----------------------------
// inputs and outputs
// ----------------------------
     .ivCPU_SKT_OCC ({FM_CPU1_SKTOCC_LVT3_N, FM_CPU0_SKTOCC_LVT3_N}),            //Socket occupied (input vector for CPU0 and CPU1 SKT_OCC signal)
     .ivPROC_ID_CPU0({FM_CPU0_PROC_ID1,      FM_CPU0_PROC_ID0}),                 //CPU0 Processor ID
     .ivPROC_ID_CPU1({FM_CPU1_PROC_ID1,      FM_CPU1_PROC_ID0}),                 //CPU1 Processor ID
     .ivPKG_ID_CPU0 ({FM_CPU0_PKGID2,        FM_CPU0_PKGID1,FM_CPU0_PKGID0}),    //CPU0 Package ID
     .ivPKG_ID_CPU1 ({FM_CPU1_PKGID2,        FM_CPU1_PKGID1,FM_CPU1_PKGID0}),    //CPU1 Package ID
     .iCPU_INTR     ( FM_CPU0_INTR_PRSNT),                               //Interposer present, CPU0/1 interposer
                                                                         //CPU straps are refernced to 3.3V_AUX, so it migth not be needed
     .oSYS_OK(wSYS_OK),                                                  //System validation Ok, once this module has detected a valid CPU configurqation this signal will be set
     .oCPU_MISMATCH(wCPU_MISMATCH),                                      //CPU Mismatch, if not CPU ID or PKG ID were identified then this signal will remain low, this signal is used in BMC SGPIOs module
     .oHBM(wHBM_EN),                                                     //Output enabler for HBM VR
     .oSOCKET_REMOVED(wSKT_REMOVED)                                      //Socket Removed
);

//PSU Control
psu_pwr_on_check  mPSU_CHECK
(
    .iClk(iClk_2M),                             //System clock
    .iRst_n(iRST_N),                            //synchronous Reset
    .ienable(wPSU_EN),                          //internal input signal coming from master to start the FSM
    .iPWRGD_PS_PWROK(PWRGD_PS_PWROK_FF),        //Signal generated by the PSU, indicates wihen 12V Main is stablished
    .iPWRGD_P3V3(PWRGD_P3V3_FF),                //power good coming from 3.3V main voltage regulator (powered by 12V main)
    //Output
    .oFM_PS_EN(wFM_PS_EN),                      //Only output signal, this enables PSU
    .ofault(wPSU_PWR_FLT),                      //Failure output, to indicate to master sequencer that something failed on this block
    .oFaultCode(wPsuFaultCode),                              //This output indicates to Master which is the code of the failure
    .odone(wPSU_PWR_OK),                        //Indicates if the function is done
    .oFM_P5V_EN(wFM_P5V_EN),                    // Enable signal for P5V VR on System VRs     ##NEW
    .oFM_PLD_CLK_DEV_EN(wFM_PLD_CLKS_DEV_EN)    //Output to enable 100MHz external generator from SI5332 or CK440
);



//S3 Switch Logic
psu_sw_logic mPSU_S3_LOGIC
(
    .iClk(iClk_2M),                               //System clock
    .iRst_n(iRST_N),                              //synchronous Reset

    .ienable(wSYS_OK && ~iFM_FORCE_PWRON),                            //start S3Logic module (SysOK) and not in FORCE PWRON mode
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    .iPWRGD_PS_PWROK_PLD_R(PWRGD_PS_PWROK_FF),    //Power Good from Power Supply
    .iFM_SLP3_N(FM_SLPS3_N_FF),                   //Sleep 3 signal
    .iFM_SLP4_N(FM_SLPS4_N_FF),                   //Sleep 4 signal
    .iFM_DIMM_12V_CPS_SX_N(FM_DIMM_12V_CPS_S5_N), //2 Pin header option .. Deofault 3.3V
    // Outputs
    .oFM_AUX_SW_EN(wFM_AUX_SW_EN),                 // Signal to enable P12V_AUX Switch
    .oFM_S3_SW_P12V_STBY_EN(wFM_SX_SW_P12V_STBY_EN),  // Output to select in switch 12V_STBY
    .oFM_S3_SW_P12V_EN(wFM_SX_SW_P12V_EN)         // Output to select in switch 12V

    //.ofault()
    //.oFaultCode(),                                //This output indicates to Master which is the code of the failure    32 bits!
);


master_fub Master_Seq
  (
    .iClk  (iClk_2M),                             //input clock signal (@2MHz)
    .iRst_n(iRST_N),                              //input reset signal (active low)

    .iSysChkGood(wSYS_OK && ~iFM_FORCE_PWRON),    //start Master if Sys check is high AND FORCE_PWRON jumper is LOW (no FORCE)
 //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//bypassing also ONCTL so we can continue with power-seq
    .iBmcOnCtl_n(LOW),//(FM_BMC_ONCTL_N_FF),              //from BMC, FM_BMC_ONCTL

    .iFmSlp3_n(FM_SLPS3_N_FF),                    //Sleep S3 signal
    .iFmSlp4_n(FM_SLPS4_N_FF),                    //Sleep S4 signal. Between sleep signals (S3&S4) we can determine if we are in S0, S3 or S4

    .iFmSlpSusRsmRst_n(FM_SLP_SUS_RSM_RST_N_FF),  //Sleep SUS (active low) signal, coming from PCH when receiving P3V3_DSW (needed to continue with PCH seq)

    .iDediBusy_n(rDEDI_BUSY_LOCK),            //When asserted, causes a regular power-down sequence (turn off main and above, not PCH nor BMC)

    .iBmcDone(wBMC_PWR_OK),                       //BMC sequencer done
    .iPchDone(wPCH_PWR_OK),                       //PCH sequencer done
    .iPsuDone(wPSU_PWR_OK),                       //PSU_MAIN sequencer done
    .iCpuMemDone(wCPU_MEM_PWR_OK),                //CPU_MEM sequencer done

    .iBmcFailure(wBMC_PWR_FLT),                   //failure indication from BMC sequencer
    .iPchFailure(wPCH_PWR_FLT),                   //failure indication from PCH sequencer
    .iPsuFailure(wPSU_PWR_FLT),                   //failure indication from PSU_MAIN sequencer
    .iCpuMemFailure(wCPU_MEM_PWR_FLT),            //failure indication from CPU_MEM sequencer
    //Outputs
    .oBmcEna(wBMC_EN),                            //Enable BMC sequencer to start
    .oPchEna(wPCH_EN),                            //Enable PCH sequencer to start
    .oPsuEna(wPSU_EN),                            //Enable PSU_MAIN sequencer to start
    .oCpuMemEna(wCPU_MEM_EN),                     //Enable CPU_MEM sequencer to start
    .oAdrEna(wAdrEna),                                   //Enable for ADR FSM

    .oPwrGdPchPwrOK(PWRGD_PCH_PWROK),             //PCH PWR-OK
    .oPwrGdSysPwrOK(PWRGD_SYS_PWROK),             //SYS PWR-OK

    .oS3Indication(wS3_STATE),                    //we are in S3 indication
    .oS5Indication(wS4_S5_STATE),                 //we are in S4/S5 indication

    .oPwrDwn()                                    //Power Down Indication for all sequencers
   );





ac_bmc_pwrseq_fsm mBmc_pwrseq_fsm
(
    //------------------------
    //Clock and Reset signals
    //------------------------
    .iClk(iClk_2M),                               //clock for sequential logic
    .iRst_n(iRST_N),                              //reset signal, resets state machine to initial state
    // ----------------------------
    // Power-up sequence inputs and outputs
    // ----------------------------
    // .iPWRGD_P3V3_AUX(PWRGD_P3V3_AUX_FF),        //Pwrgd from P3V3 Aux VR //alarios: removed by Pedro in ac_bmc_fsm module as it is not necessary
     .iPWRGD_P2V5_BMC_AUX(PWRGD_P2V5_BMC_AUX_FF),  //Pwrgd from P2V5 Aux VR
     .iPWRGD_P1V8_BMC_AUX(PWRGD_P1V8_PCH_AUX_FF),  //Pwrgd from P1V8 Aux VR
     .iPWRGD_P1V2_BMC_AUX(PWRGD_P1V2_BMC_AUX_FF),  //Pwrgd from P1V2 Aux VR
     .iPWRGD_P1V0_BMC_AUX(PWRGD_P1V0_BMC_AUX_FF),  //Pwrgd from P1V0 Aux VR

     .iFM_BMC_INTR_PRSNT(FM_BMC_INTR_PRSNT),      //Hardware strap for BMC interposer support ("0" == no interposer), it will change the power sequence between AST2500 and AST2600
     .iBMC_PWR_EN(wBMC_EN),  					  //Initial condition to start BMC, If set FSM will start power-down sequencing if not set will enable BMC PWR_SEQ
     .iRST_DEDI_BUSY_PLD_N(RST_DEDI_BUSY_PLD_N),  //Dediprog Detection Support
     //Outputs
     .oBMC_PWR_FAULT(wBMC_PWR_FLT),               //Fault BMC VR's
     .oRST_SRST_BMC_N(wRST_SRST_BMC_PLD_N),       //SRST#
     .oFM_P2V5_BMC_AUX_EN(wFM_P2V5_BMC_AUX_EN),        //Output for BMC P2V5 AUX VR enable
     .oFM_PCH_P1V8_AUX_EN(wFM_PCH_P1V8_AUX_EN),        //Output PCH P1V8 VR enable, for module ac_pch_pwrseq_fsm and LPC interface
     .oFM_P1V2_BMC_AUX_EN(wFM_P1V2_BMC_AUX_EN),        //Output BMC P1V2 VR enable
     .oFM_P1V0_BMC_AUX_EN(wFM_P1V0_BMC_AUX_EN),        //Output for BMC P1V0 AUX VR enable

     .oBMC_PWR_P2V5_FAULT(wBMC_PWR_P2V5_FAULT),           //BMC VR Faults
     .oBMC_PWR_P1V8_FAULT(wBMC_PWR_P1V8_FAULT),
     .oBMC_PWR_P1V2_FAULT(wBMC_PWR_P1V2_FAULT),
     .oBMC_PWR_P1V0_FAULT(wBMC_PWR_P1V0_FAULT),

     .oBMC_PWR_OK(wBMC_PWR_OK)                         //If set it indicates all VRs are up

);

ac_pch_seq  mPch_seq
(
    //------------------------
    //Clock and Reset signals
    //------------------------
    .iClk(iClk_2M),                         //clock for sequential logic
    .iRst_n(iRST_N),                        //reset signal, resets state machine to initial state
    // ----------------------------
    // Power-up sequence inputs and outputs
    // ----------------------------
     .iFM_PCH_PRSNT_N(LOW),   //(FM_PCH_PRSNT_N),                    //Detect if the PCH is present
     .iRST_SRST_BMC_N(wRST_SRST_BMC_PLD_N),               //RST BMC
     .iPWRGD_P1V8_BMC_AUX(PWRGD_P1V8_PCH_AUX_FF),         //PCH VR PWRGD P1V8
     .iPWRGD_PCH_PVNN_AUX(PWRGD_PVNN_PCH_AUX_FF),         //PCH VR PVNN
     .iPWRGD_PCH_P1V05_AUX(PWRGD_P1V05_PCH_AUX_FF),       //PCH VR PWRGD P1V05
     .iRST_DEDI_BUSY_PLD_N(RST_DEDI_BUSY_PLD_N),          //Dediprog Detection Support
     .iPCH_PWR_EN(wPCH_EN),                               //Enable PCH
     //Outputs
     .oFM_PVNN_PCH_EN(wFM_PCH_PVNN_AUX_EN),               //PVVN VR EN
     .oFM_P1V05_PCH_EN(wFM_PCH_P1V05_AUX_EN),             //PVVN VR EN
     .oRST_RSMRST_N(RST_RSMRST_N),                        //RSMRST#
     .oPCH_PWRGD(wPCH_PWR_OK),                            //PWRGD of all PCH VR's
     .oPCH_PWR_FLT_PVNN(wPCH_PWR_FLT_PVNN),               //Fault PCH PVNN
     .oPCH_PWR_FLT_P1V05(wPCH_PWR_FLT_P1V05),             //Fault PCH P1V05
     .oPCH_PWR_FLT(wPCH_PWR_FLT),                         //Fault PCH VR's
     .oCLK_INJ_25MHz_EN(wFM_PLD_CLK_25M_EN)                //Enable 25MHz CLOCK_INJ
);


ac_cpu_seq mCpu0_mem_seq
(
// ------------------------
// Clock and Reset signals
// ------------------------

    .iClk(iClk_2M),               //clock for sequential logic
    .iRst_n(iRST_N),              //reset signal, resets state machine to initial state
// ----------------------------
// inputs and outputs
// ----------------------------
    .iCPU_PWR_EN(wCPU_MEM_EN),           //Cpu power sequence enable
    .iENABLE_TIME_OUT(LOW),              //Enable the Timeout
    .iENABLE_HBM(wHBM_EN),               //Enable HBM VR
    .iFM_INTR_PRSNT(FM_CPU0_INTR_PRSNT), //Detect Interposer
    .iFM_CPU_SKTOCC_N(FM_CPU0_SKTOCC_LVT3_N),
    .iINTR_SKU(FM_CPU0_PKGID2),          //Detect Interposer SKU (1 = CLX or 0 = BRS)

    .iPWRGD_PVPP_HBM_CPU(PWRGD_PVPP_HBM_CPU0_FF),                 //CPU VR PWRGD PVPP_HBM
    .iPWRGD_PVCCFA_EHV_CPU(PWRGD_PVCCFA_EHV_CPU0_FF),             //CPU VR PWRGD PVCCFA_EHV
    .iPWRGD_PVCCFA_EHV_FIVRA_CPU(PWRGD_PVCCFA_EHV_FIVRA_CPU0_FF), //CPU VR PWRGD PVCCFA_EHV_FIVRA
    .iPWRGD_PVCCINFAON_CPU(PWRGD_PVCCINFAON_CPU0_FF),             //CPU VR PWRGD PVCCINFAON
    .iPWRGD_PVNN_MAIN_CPU(PWRGD_PVNN_MAIN_CPU0_FF),               //CPU VR PWRGD PVNN
    .imPWRGD_PVCCD_HV_CPU(PWRGD_PVCCD_HV_CPU0_FF),                //CPU VR MEMORY PWRGD PVCCD_HV
    .iPWRGD_PVCCIN_CPU(PWRGD_PVCCIN_CPU0_FF),                     //CPU VR PVCCIN

    //Inputs/Outputs for inteposer support//////////////////////////////////////////
    .iPWRGD_VDDQ_MEM_BRS(SMB_S3M_CPU0_SCL),                     //SMB_S3M_CPU_SCL
    .iPWRGD_P1V8_CPU_BRS(SMB_S3M_CPU0_SDA),                     //SMB_S3M_CPU_SDA
    .oCPU_PWR_FLT_VDDQ_MEM_BRS(),                               //VR fault on interposer, no supported as HW interposer does not have this conection
    .oCPU_PWR_FLT_P1V8_BRS(wBRS_PWRGD_PLT_AUX_CPU0_LVT3),       //VR fault on interposer

    .iPWRGD_VTT(iPWRGD_VTT_PWRGD),   //PWRGD from the Interposer VR (VTT) need to do something for when there is no Interposer
    ////////////////////////////////////////////////////////////////////////////////

    //MEMORY DDR5 Inputs/Outputs  //////////////////////////////////////////////////
    .iMEM_S3_EN(wS3_STATE),              //If set MEM VRs will remain ON
    .oMEM_PRW_FLT(wMEM0_PWR_FLT),        //Fault Memory VR'
    .oMEM_PWRGD(wPWRGD_DRAMPWRGD_CPU0),  //PWRGD of MEMORY
    ////////////////////////////////////////////////////////////////////////////////

    .oFM_PVPP_HBM_CPU_EN(wFM_PVPP_HBM_CPU0_EN),                //CPU VR EN PVPP_HBM
    .oFM_PVCCFA_EHV_CPU_EN(wFM_PVCCFA_EHV_CPU0_EN),            //CPU VR EN PVCCFA_EHV
    .oFM_PVCCFA_EHV_FIVRA_CPU_EN(wFM_PVCCFA_EHV_FIVRA_CPU0_EN),//CPU VR EN PVCCFA_EHV_FIVRA
    .oFM_PVCCINFAON_CPU_EN(wFM_PVCCINFAON_CPU0_EN),            //CPU VR EN PVCCINFAON
    .oFM_PVNN_MAIN_CPU_EN(wFM_PVNN_MAIN_CPU0_EN),              //CPU VR EN PVNN
    .omFM_PVCCD_HV_CPU_EN(wFM_PVCCD_HV_CPU0_EN),               //CPU VR EN MEMORY PVCCD_HV
    .oFM_PVCCIN_CPU_EN(wFM_PVCCIN_CPU0_EN),                //CPU VR EN PVCCIN
    .oPWRGD_PLT_AUX_CPU(wPWRGD_PLT_AUX_CPU0_LVT3),         //CPU PRWGD PLATFORM AUXILIARY

    .oCPU_PWRGD(wCPU0_MEM_PWR_OK),                         //PWRGD of all CPU VR's
    .oCPU_PWR_FLT(wCPU0_MEM_PWR_FLT),                      //Fault CPU VR's

    .oCPU_PWR_FLT_HBM_CPU(wCpu0PvppHbmFlt),             //Fault  PVPP_HBM
    .oCPU_PWR_FLT_EHV_CPU(wCpu0PvccfaEhvFlt),             //Fault  PVCCFA_EHV
    .oCPU_PWR_FLT_EHV_FIVRA_CPU(wCpu0PvccfaFivrFlt),       //Fault  PVCCFA_EHV_FIVRA
    .oCPU_PWR_FLT_PVCCINFAON_CPU(wCpu0PvccInfaonFlt),      //Fault  PVCCINFAON
    .oCPU_PWR_FLT_PVNN(wCpu0PvnnMainFlt),                //Fault  PVNN
    .oCPU_PWR_FLT_PVCCIN(wCpu0PvccinFlt),              //Fault  PVCCIN
    .omFM_PWR_FLT_PVCCD_HV(wCpu0PvccdHvFlt),            //Fault  PVCCD_HV

    .oDBG_CPU_ST()                       //State of CPU Secuence [3:0]

);

ac_cpu_seq mCpu1_mem_seq
(
// ------------------------
// Clock and Reset signals
// ------------------------

    .iClk(iClk_2M),               //clock for sequential logic
    .iRst_n(iRST_N),              //reset signal, resets state machine to initial state
// ----------------------------
// inputs and outputs
// ----------------------------
    .iCPU_PWR_EN(wCPU_MEM_EN),           //Cpu power sequence enable
    .iENABLE_TIME_OUT(LOW),              //Enable the Timeout
    .iENABLE_HBM(wHBM_EN),               //Enable HBM VR
    .iFM_INTR_PRSNT(FM_CPU0_INTR_PRSNT),  //Detect Interposer
    .iFM_CPU_SKTOCC_N(FM_CPU1_SKTOCC_LVT3_N),
    .iINTR_SKU(FM_CPU1_PKGID2),          //Detect Interposer SKU (1 = CLX or 0 = BRS)

    .iPWRGD_PVPP_HBM_CPU(PWRGD_PVPP_HBM_CPU1_FF),                 //CPU VR PWRGD PVPP_HBM
    .iPWRGD_PVCCFA_EHV_CPU(PWRGD_PVCCFA_EHV_CPU1_FF),             //CPU VR PWRGD PVCCFA_EHV
    .iPWRGD_PVCCFA_EHV_FIVRA_CPU(PWRGD_PVCCFA_EHV_FIVRA_CPU1_FF), //CPU VR PWRGD PVCCFA_EHV_FIVRA
    .iPWRGD_PVCCINFAON_CPU(PWRGD_PVCCINFAON_CPU1_FF),             //CPU VR PWRGD PVCCINFAON
    .iPWRGD_PVNN_MAIN_CPU(PWRGD_PVNN_MAIN_CPU1_FF),               //CPU VR PWRGD PVNN
    .imPWRGD_PVCCD_HV_CPU(PWRGD_PVCCD_HV_CPU1_FF),                //CPU VR MEMORY PWRGD PVCCD_HV
    .iPWRGD_PVCCIN_CPU(PWRGD_PVCCIN_CPU1_FF),                     //CPU VR PVCCIN

    //Inputs/Outputs for inteposer support//////////////////////////////////////////
    .iPWRGD_VDDQ_MEM_BRS(SMB_S3M_CPU1_SCL),                     //SMB_S3M_CPU_SCL
    .iPWRGD_P1V8_CPU_BRS(SMB_S3M_CPU1_SDA),                     //SMB_S3M_CPU_SDA
    .oCPU_PWR_FLT_VDDQ_MEM_BRS(),                               //VR fault on interposer
    .oCPU_PWR_FLT_P1V8_BRS(wBRS_PWRGD_PLT_AUX_CPU1_LVT3),       //VR fault on interposer

    .iPWRGD_VTT(HIGH), //Only have this signal available for CPU0, so we are tying this to '1'
    ////////////////////////////////////////////////////////////////////////////////

    //MEMORY DDR5 Inputs/Outputs  //////////////////////////////////////////////////
    .iMEM_S3_EN(wS3_STATE),              //If set MEM VRs will remain ON
    .oMEM_PRW_FLT(wMEM1_PWR_FLT),        //Fault Memory VR'
    .oMEM_PWRGD(wPWRGD_DRAMPWRGD_CPU1),  //PWRGD of MEMORY
    ////////////////////////////////////////////////////////////////////////////////

    .oFM_PVPP_HBM_CPU_EN(wFM_PVPP_HBM_CPU1_EN),                //CPU VR EN PVPP_HBM
    .oFM_PVCCFA_EHV_CPU_EN(wFM_PVCCFA_EHV_CPU1_EN),            //CPU VR EN PVCCFA_EHV
    .oFM_PVCCFA_EHV_FIVRA_CPU_EN(wFM_PVCCFA_EHV_FIVRA_CPU1_EN),//CPU VR EN PVCCFA_EHV_FIVRA
    .oFM_PVCCINFAON_CPU_EN(wFM_PVCCINFAON_CPU1_EN),           //CPU VR EN PVCCINFAON
    .oFM_PVNN_MAIN_CPU_EN(wFM_PVNN_MAIN_CPU1_EN),             //CPU VR EN PVNN
    .omFM_PVCCD_HV_CPU_EN(wFM_PVCCD_HV_CPU1_EN),              //CPU VR EN MEMORY PVCCD_HV
    .oFM_PVCCIN_CPU_EN(wFM_PVCCIN_CPU1_EN),                //CPU VR EN PVCCIN
    .oPWRGD_PLT_AUX_CPU(wPWRGD_PLT_AUX_CPU1_LVT3),         //CPU PRWGD PLATFORM AUXILIARY

    .oCPU_PWRGD(wCPU1_MEM_PWR_OK),                         //PWRGD of all CPU VR's
    .oCPU_PWR_FLT(wCPU1_MEM_PWR_FLT),                      //Fault CPU VR's

    .oCPU_PWR_FLT_HBM_CPU(wCpu1PvppHbmFlt),             //Fault  PVPP_HBM
    .oCPU_PWR_FLT_EHV_CPU(wCpu1PvccfaEhvFlt),             //Fault  PVCCFA_EHV
    .oCPU_PWR_FLT_EHV_FIVRA_CPU(wCpu1PvccfaFivrFlt),       //Fault  PVCCFA_EHV_FIVRA
    .oCPU_PWR_FLT_PVCCINFAON_CPU(wCpu1PvccInfaonFlt),      //Fault  PVCCINFAON
    .oCPU_PWR_FLT_PVNN(wCpu1PvnnMainFlt),                //Fault  PVNN
    .oCPU_PWR_FLT_PVCCIN(wCpu1PvccinFlt),              //Fault  PVCCIN
    .omFM_PWR_FLT_PVCCD_HV(wCpu1PvccdHvFlt),            //Fault  PVCCD_HV

    .oDBG_CPU_ST()                       //State of CPU Secuence [3:0]

);

//MEM DDR4-5 RST & PWRGD_FAIL LOGIC
ddr5_pwrgd_logic_modular #
    (
    .MC_SIZE(4)
    ) mDDR5_Cpu0_Pwrgd_Fail
    (
    .iClk(iClk_2M),                             //Clock input
    .iRst_n(iRST_N),                            //Reset enable on high
    .iFM_INTR_PRSNT(FM_CPU0_INTR_PRSNT),        //Detect Interposer
    .iINTR_SKU(FM_CPU0_PKGID2),                 //Detect Interposer SKU (1 = CLX or 0 = BRS) to bypass CPU MC_RESET to DIMMs in DDR4
    .iPWRGD_PS_PWROK(PWRGD_PS_PWROK_FF),        //PWRGD with generate by Power Supply.

    .iPWRGD_DRAMPWRGD_DDRIO({                   //input from MC VR, bypassed in CPU_MEM module to support interposer
    wPWRGD_DRAMPWRGD_CPU0,
    wPWRGD_DRAMPWRGD_CPU0,
    wPWRGD_DRAMPWRGD_CPU0,
    wPWRGD_DRAMPWRGD_CPU0
    }),
    .iMC_RST_N({                                //Reset from memory controller
    M_AB_CPU0_RESET_N,
    M_CD_CPU0_RESET_N,
    M_EF_CPU0_RESET_N,
    M_GH_CPU0_RESET_N
    }),
    .iADR_LOGIC({                               //ADR controls pwrgd/fail output
    wPwrGdFailCpuAB_adr,
    wPwrGdFailCpuCD_adr,
    wPwrGdFailCpuEF_adr,
    wPwrGdFailCpuGH_adr
    }),
    .ioPWRGD_FAIL_CH_DIMM_CPU({                 //PWRGD_FAIL bidirectional signal
    PWRGD_FAIL_CPU0_AB,
    PWRGD_FAIL_CPU0_CD,
    PWRGD_FAIL_CPU0_EF,
    PWRGD_FAIL_CPU0_GH
    }),

    .oDIMM_MEM_FLT(wCpu0MemFlt),                //MEM Fault, informs that something in ddr5_pwrgd_logic module went wrong!

    .oPWRGD_DRAMPWRGD_OK({                      //DRAM PWR OK to CPU
    wPWRGD_DRAMPWRGD_CPU0_AB_LVC1,
    wPWRGD_DRAMPWRGD_CPU0_CD_LVC1,
    wPWRGD_DRAMPWRGD_CPU0_EF_LVC1,
    wPWRGD_DRAMPWRGD_CPU0_GH_LVC1
    }),
    .oFPGA_DIMM_RST_N({                         //Reset to DIMMs
    M_AB_CPU0_FPGA_RESET_N,
    M_CD_CPU0_FPGA_RESET_N,
    M_EF_CPU0_FPGA_RESET_N,
    M_GH_CPU0_FPGA_RESET_N
    })
);

assign PWRGD_DRAMPWRGD_CPU0_AB_LVC1 = wPWRGD_DRAMPWRGD_CPU0_AB_LVC1 && wPWRGD_DRAMPWRGD_CPU0_CD_LVC1 && wPWRGD_DRAMPWRGD_CPU0_EF_LVC1 && wPWRGD_DRAMPWRGD_CPU0_GH_LVC1;
assign PWRGD_DRAMPWRGD_CPU0_CD_LVC1 = wPWRGD_DRAMPWRGD_CPU0_AB_LVC1 && wPWRGD_DRAMPWRGD_CPU0_CD_LVC1 && wPWRGD_DRAMPWRGD_CPU0_EF_LVC1 && wPWRGD_DRAMPWRGD_CPU0_GH_LVC1;
assign PWRGD_DRAMPWRGD_CPU0_EF_LVC1 = wPWRGD_DRAMPWRGD_CPU0_AB_LVC1 && wPWRGD_DRAMPWRGD_CPU0_CD_LVC1 && wPWRGD_DRAMPWRGD_CPU0_EF_LVC1 && wPWRGD_DRAMPWRGD_CPU0_GH_LVC1;
assign PWRGD_DRAMPWRGD_CPU0_GH_LVC1 = wPWRGD_DRAMPWRGD_CPU0_AB_LVC1 && wPWRGD_DRAMPWRGD_CPU0_CD_LVC1 && wPWRGD_DRAMPWRGD_CPU0_EF_LVC1 && wPWRGD_DRAMPWRGD_CPU0_GH_LVC1;

ddr5_pwrgd_logic_modular #
    (
    .MC_SIZE(4)
    ) mDDR5_Cpu1_Pwrgd_Fail
    (
    .iClk(iClk_2M),                             //Clock input
    .iRst_n(iRST_N),                            //Reset enable on high
    .iFM_INTR_PRSNT(FM_CPU0_INTR_PRSNT),        //Detect Interposer, only one physical HW signal so CPU1 will use smae as CPU0
    .iINTR_SKU(FM_CPU1_PKGID2),                 //Detect Interposer SKU (1 = CLX or 0 = BRS) to bypass CPU MC_RESET to DIMMs in DDR4
    .iPWRGD_PS_PWROK(PWRGD_PS_PWROK_FF),        //PWRGD with generate by Power Supply.

    .iPWRGD_DRAMPWRGD_DDRIO({                   //input from MC VR, bypassed in CPU_MEM module to support interposer
    wPWRGD_DRAMPWRGD_CPU1,
    wPWRGD_DRAMPWRGD_CPU1,
    wPWRGD_DRAMPWRGD_CPU1,
    wPWRGD_DRAMPWRGD_CPU1
    }),
    .iMC_RST_N({                                //Reset from memory controller
    M_AB_CPU1_RESET_N,
    M_CD_CPU1_RESET_N,
    M_EF_CPU1_RESET_N,
    M_GH_CPU1_RESET_N
    }),
    .iADR_LOGIC({                               //ADR controls pwrgd/fail output
    wPwrGdFailCpuAB_adr,
    wPwrGdFailCpuCD_adr,
    wPwrGdFailCpuEF_adr,
    wPwrGdFailCpuGH_adr
    }),
    .ioPWRGD_FAIL_CH_DIMM_CPU({                 //PWRGD_FAIL bidirectional signal
    PWRGD_FAIL_CPU1_AB,
    PWRGD_FAIL_CPU1_CD,
    PWRGD_FAIL_CPU1_EF,
    PWRGD_FAIL_CPU1_GH
    }),

    .oDIMM_MEM_FLT(wCpu1MemFlt),                //MEM Fault, informs that something in ddr5_pwrgd_logic module went wrong!

    .oPWRGD_DRAMPWRGD_OK({                      //DRAM PWR OK to CPU
    wPWRGD_DRAMPWRGD_CPU1_AB_LVC1,
    wPWRGD_DRAMPWRGD_CPU1_CD_LVC1,
    wPWRGD_DRAMPWRGD_CPU1_EF_LVC1,
    wPWRGD_DRAMPWRGD_CPU1_GH_LVC1
    }),
    .oFPGA_DIMM_RST_N({                         //Reset to DIMMs
    M_AB_CPU1_FPGA_RESET_N,
    M_CD_CPU1_FPGA_RESET_N,
    M_EF_CPU1_FPGA_RESET_N,
    M_GH_CPU1_FPGA_RESET_N
    })
);

assign PWRGD_DRAMPWRGD_CPU1_AB_LVC1 = wPWRGD_DRAMPWRGD_CPU1_AB_LVC1 && wPWRGD_DRAMPWRGD_CPU1_CD_LVC1 && wPWRGD_DRAMPWRGD_CPU1_EF_LVC1 && wPWRGD_DRAMPWRGD_CPU1_GH_LVC1;
assign PWRGD_DRAMPWRGD_CPU1_CD_LVC1 = wPWRGD_DRAMPWRGD_CPU1_AB_LVC1 && wPWRGD_DRAMPWRGD_CPU1_CD_LVC1 && wPWRGD_DRAMPWRGD_CPU1_EF_LVC1 && wPWRGD_DRAMPWRGD_CPU1_GH_LVC1;
assign PWRGD_DRAMPWRGD_CPU1_EF_LVC1 = wPWRGD_DRAMPWRGD_CPU1_AB_LVC1 && wPWRGD_DRAMPWRGD_CPU1_CD_LVC1 && wPWRGD_DRAMPWRGD_CPU1_EF_LVC1 && wPWRGD_DRAMPWRGD_CPU1_GH_LVC1;
assign PWRGD_DRAMPWRGD_CPU1_GH_LVC1 = wPWRGD_DRAMPWRGD_CPU1_AB_LVC1 && wPWRGD_DRAMPWRGD_CPU1_CD_LVC1 && wPWRGD_DRAMPWRGD_CPU1_EF_LVC1 && wPWRGD_DRAMPWRGD_CPU1_GH_LVC1;

//////////////////////////////////////////////////////////////////////////////////
//Prochot & Memhot
//////////////////////////////////////////////////////////////////////////////////

ac_prochot_memhot mProchot_Memhot_cpu0
(
    //Inputs
    .iIRQ_CPU_MEM_VRHOT_N(IRQ_CPU0_MEM_VRHOT_N),
    .iIRQ_PSYS_CRIT_N(IRQ_PSYS_CRIT_N),
    .iIRQ_CPU_VRHOT_LVC3_N(IRQ_CPU0_VRHOT_N),
    .iFM_SYS_THROTTLE_LVC3_N(wFM_SYS_THROTTLE_N),
    //Outputs
    .oFM_PROCHOT_LVC3_N(wH_CPU0_PROCHOT_LVC1_N),
    .oFM_H_CPU_MEMHOT_N(wH_CPU0_MEMHOT_IN_LVC1_N)
);

ac_prochot_memhot mProchot_Memhot_cpu1
(
    //Inputs
    .iIRQ_CPU_MEM_VRHOT_N(IRQ_CPU1_MEM_VRHOT_N),
    .iIRQ_PSYS_CRIT_N(IRQ_PSYS_CRIT_N),
    .iIRQ_CPU_VRHOT_LVC3_N(IRQ_CPU1_VRHOT_N),
    .iFM_SYS_THROTTLE_LVC3_N(wFM_SYS_THROTTLE_N),
    //Outputs
    .oFM_PROCHOT_LVC3_N(wH_CPU1_PROCHOT_LVC1_N),
    .oFM_H_CPU_MEMHOT_N(wH_CPU1_MEMHOT_IN_LVC1_N)
);

//////////////////////////////////////////////////////////////////////////////////
//SmaRT Logic (glue logic to generate SYS_THROTTLE_N output signal)
//////////////////////////////////////////////////////////////////////////////////

SmartLogic_fub SmartLogic
  (
   .iFmThrottle_n(iFM_THROTTLE_R_N),                           //from PCH
   .iIrqSml1PmbusAlert_n(iIRQ_SML1_PMBUS_PLD_ALERT_N),         //from PDB
   .iFmPmbusAlertBEna(iFM_PMBUS_ALERT_B_EN),                   //from PCH

   .oFmSysThrottle_n(wFM_SYS_THROTTLE_N)                     //to PCIe Slots (B,D & E) and Risers 1 & 2, as well as internally in the FPGA to MISC (for MEMHOT, PROCHOT of both CPUs)
   );


//////////////////////////////////////////////////////////////////////////////////
//THERMTRIP DLY
//////////////////////////////////////////////////////////////////////////////////
ac_thermtrip_dly   mThermtrip_Dly
(
    .iClk_2M(iClk_2M),                   //2MHz Clock Input
    .iRst_n (iRST_N),

    .iTherm_Trip_Dly_En(LOW),
    .iFM_CPU0_THERMTRIP_LVT3_N(H_CPU0_THERMTRIP_LVC1_N),
    .iFM_CPU1_THERMTRIP_LVT3_N(H_CPU1_THERMTRIP_LVC1_N),
    .iFM_MEM_THERM_EVENT_CPU0_LVT3_N(H_CPU0_MEMTRIP_LVC1_N),
    .iFM_MEM_THERM_EVENT_CPU1_LVT3_N(H_CPU1_MEMTRIP_LVC1_N),
    .iFM_CPU1_SKTOCC_LVT3_N(FM_CPU1_SKTOCC_LVT3_N),
    // Outputs
    .oFM_THERMTRIP_DLY_N(FM_THERMTRIP_DLY_LVC1_N)
);

//assign FM_THERMTRIP_DLY_LVC1_N = HIGH;

//////////////////////////////////////////////////////////////////////////////////
// LED Logic control
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
// PCIE Reset Logic
//////////////////////////////////////////////////////////////////////////////////
perst mRst_Perst
(   .PWRG_CPUPWRGD_LVC1(PWRGD_CPUPWRGD_LVC1_FF),
    .RST_PLTRST_N(RST_PLTRST_N_FF),
    .FM_RST_PERST_BIT({FM_RST_PERST_BIT2,FM_RST_PERST_BIT1,FM_RST_PERST_BIT0}),
    .RST_PCIE_CPU0_DEV_PERST_N(wRST_PCIE_PERST0_N),
    .RST_PCIE_CPU1_DEV_PERST_N(wRST_PCIE_PERST1_N),
    .RST_PCIE_PCH_DEV_PERST_N(wRST_PCIE_PERST2_N),
    .RST_PLTRST_PLD_B_N(RST_PLTRST_B_N)
);
//W.A. keep PCIe and CPU in reset to have a valid know state, seing 3.3V_AUX leagke in Foxville as PCIe_RST is floating, Fab 3 added a PD to fix it
assign RST_PCIE_PERST2_N = wPCH_PWR_OK ? wRST_PCIE_PERST2_N : LOW;
assign RST_PCIE_PERST1_N = wPCH_PWR_OK ? wRST_PCIE_PERST1_N : LOW;
assign RST_PCIE_PERST0_N = wPCH_PWR_OK ? wRST_PCIE_PERST0_N : LOW;

//////////////////////////////////////////////////////////////////////////////////
///Misc Logic (Cpu-Errors, NMI & PCH-CrashLog logic)
//////////////////////////////////////////////////////////////////////////////////


MiscLogic MiscLogic
(
    .iClk(iClk_2M),                                        //used for the delayed version of CATERR & RMCA output. Depends on the freq, is the parameter used, we need 500 us delay
    .iRst_n(iRST_N),                                       //synch reset input (active low)

    .iCpuCatErr_n(iCPU_CATERR_N),                          //CPU_CATERR_N comming from both CPUs
    .iCpuRmca_n(iCPU_RMCA_N),                              //CPU_RMCA_N coming from both CPUs

    .iFmBmcCrashLogTrig_n(iFM_BMC_CRASHLOG_TRIG_N),        //these 2, combined with the CATERR, generate the CRASHLOG_TRIG_N signal to PCH
    .iFmGlbRstWarn_n(iFM_PCH_PLD_GLB_RST_WARN_N),

    .iIrqPchCpuNmiEvent_n(iIRQ_PCH_CPU_NMI_EVENT_N),       //PCH NMI event to CPU
    .iIrqBmcCpuNmi(iIRQ_BMC_CPU_NMI),                      //BMC NMI event to CPU

    .oCpuRmcaCatErr_n(oFM_CPU_RMCA_CATERR_N),              //combined CATERR & RMCA output
    .oCpuRmcaCatErrDly_n(oFM_CPU_RMCA_CATERR_DLY_N),       //delayed output of CATERR & RMCA

    .oFmPchCrashlogTrig_n(oFM_PCH_CRASHLOG_TRIG_N),        //Crashlog trigger asserted when any of CATERR, BMC_CRASHLOG_TRIGGER or GLB_RST_WARN goes low

    .oCpuNmi(oCPU_NMI_N)                                   //CPU NMI asserted when BMC NMI or PCH NMI are asserted
);



//////////////////////////////////////////////////////////////////////////////////
//SPI control
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
//BMC Workaround
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
//DBP Power button de-bouncer
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// BMC Serial GPIO Logic
//////////////////////////////////////////////////////////////////////////////////
// BMC Serial GPIO expander: Tx - {CPUs  info and CPLDs version} Rx - {Port80 decoded Data}

   wire [31:0] wBmcSgpioRsrvd;    //for reserved inputs from BMC-SGPIO


ngsx #(
        .IN_BYTE_REGS (10), 
        .OUT_BYTE_REGS (10)
     ) mNgsx
    (
        .iRst_n(iRST_N),                // active low reset signal (driven from internal FPGA logic)
        .iClk  (SGPIO_BMC_CLK),         // serial clock (driven from SGPIO master)
        .iLoad (SGPIO_BMC_LD_N),        // Load signal (driven from SGPIO master to capture serial data input in parallel register)
        .iSData(SGPIO_BMC_DOUT),        // Serial data input (driven from SGPIO master)
        .iPData({                       // Parallel data from internal logic to master
                                // First Byte
                                FM_CPU0_PROC_ID1,                       // SGPIO 7
                                FM_CPU0_PROC_ID0,                       // SGPIO 6
                                H_CPU0_MEMHOT_OUT_LVC1_N,               // SGPIO 5
                                IRQ_CPU0_MEM_VRHOT_N,                   // SGPIO 4
                                H_CPU0_MON_FAIL_PLD_LVC1_N,             // SGPIO 3
                                IRQ_CPU0_VRHOT_N,                       // SGPIO 2
                                H_CPU0_THERMTRIP_LVC1_N,                // SGPIO 1
                                FM_CPU0_SKTOCC_LVT3_N,                  // SGPIO 0

                                // Second Byte
                                H_CPU1_MEMHOT_OUT_LVC1_N,               // SGPIO 15
                                IRQ_CPU1_MEM_VRHOT_N,                   // SGPIO 14
                                H_CPU1_MON_FAIL_PLD_LVC1_N,             // SGPIO 13
                                IRQ_CPU1_VRHOT_N,                       // SGPIO 12
                                H_CPU1_THERMTRIP_LVC1_N,                // SGPIO 11
                                FM_CPU1_SKTOCC_LVT3_N,                  // SGPIO 10
                                IRQ_PSYS_CRIT_N,                        // SGPIO 9
                                wCPU_MISMATCH,                          // SGPIO 8

                                // Third Byte
                                4'b1111,                                // SGPIO [23:20] - CPU3 info
                                1'b1,                                   // SGPIO 19  -Spare
                                wCPU_MISMATCH,                          // SGPIO 18
                                FM_CPU1_PROC_ID1,                       // SGPIO 17
                                FM_CPU1_PROC_ID0,                       // SGPIO 16

                                // Fourth Byte
                                8'b11101111,                            // SGPIO [31:24] - PWRGD

                                // Fifth Byte
                                8'b10111111,                            // SGPIO [39:32] - CPU4 info

                                // Sixth Byte
                                FPGA_REV_TEST,                          // SGPIO [47:40] - Revision code

                                 // Seventh Byte
                                FPGA_REV,                               // SGPIO [55:48] - PLD Rev

                                 // Eighth Byte
                                wPCH_PWR_FLT,                             // SGPIO63
                                1'b1,                                     // SGPIO62
                                1'b1,                                     // SGPIO61
                                wPSU_PWR_FLT,                             // SGPIO60
                                PWRGD_P3V3_FF,                            // SGPIO59
                                (wCPU0_MEM_PWR_FLT || wCPU1_MEM_PWR_FLT), // SGPIO58
                                (wMEM0_PWR_FLT || wMEM1_PWR_FLT),         // SGPIO57
                                1'b0,                                     // SGPIO56

                                // Nineth Byte
                                H_CPU1_MEMTRIP_LVC1_N,                  // SGPIO71
                                FM_CPU1_PKGID2,                         // SGPIO70
                                FM_CPU1_PKGID1,                         // SGPIO69
                                FM_CPU1_PKGID0,                         // SGPIO68
                                H_CPU0_MEMTRIP_LVC1_N,                  // SGPIO67
                                FM_CPU0_PKGID2,                         // SGPIO66
                                FM_CPU0_PKGID1,                         // SGPIO65
                                FM_CPU0_PKGID0,                         // SGPIO64

                                // Tenth Byte
                                8'hFF                                   // SGPIO [79-72]   PKG CPU3 and CPU4 info

                }),
        .oSData(SGPIO_BMC_DIN),                                         // Serial data output to SGPIO master

        .oPData({                                                      // Parallel data to internal registers (slave)
                                wBmcSgpioRsrvd[31:24],                // 4   72 - 75
                                wCpu1DIMMLEDs[15:12],                 // 4   68 - 71
                                wCpu0DIMMLEDs[15:12],                 // 4   64 - 67
                                wBmcSgpioRsrvd[23:16],                // 8   56 - 63
                                wBmcSgpioRsrvd[15:8],                 // 8   48 - 55
                                wBmcSgpioRsrvd[7:0],                  // 8   40 - 47
                                wCpu1DIMMLEDs[11:0],                  // 12   28 - 39
                                wFanFaultLEDs[7:0],                   // 8    20 - 27
                                wCpu0DIMMLEDs[11:0],                  // 12   8 - 19
                                wBiosPostCode[7:0]                    // 8    0-7
                                //First_Byte[7:0]
                })
    );

//////////////////////////////////////////////////////////////////////////////////
// THERMTRIP Latcher
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
// Led Mux Logic
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
//Fault detection
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
//SMBus registers module
//////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////
//////////  ADR FSM
/////////////////////////////////////////


adr_fub ADR_FSM
  (
   .iClk(iClk_2M),                                  //input clock signal (@2MHz)
   .iRst_n(iRST_N),                                 //input reset signal (active low)
   .iAdrEna(wAdrEna),                               //ADR enable (to enable FSM execution from Master sequencer)
   .iPwrGdPchPwrOk(PWRGD_PS_PWROK_FF),              //PWROK from the Power Supply, used to detect surprise power-down condition
   .iFmSlp3Pld_n(FM_SLPS3_N_FF),                    //s3 indication (from PCH), used to detect surprise power-down condition

   .iFmAdrComplete(iFM_ADR_COMPLETE),               //comes from PCH and indicates the ADR flow completion from PCH perspective, when triggered by board (FPGA)
   .oFmAdrTrigger_n(oFM_ADR_TRIGGER_N),             //goes to PCH to signal the need of ADR as a surprise power down condition was detected
   .oFmAdrAck_n(oFM_ADR_ACK),                       //fpga uses this signal to acknoledge the PCH the reception of completion signal. FPGA will assert it 26 usec after
                                                    //asserted pwrgd_fail_cpu signals, to allow NVDIMMs to move data from volatile to non-volatile memory

   .oPwrGdFailCpuAB(wPwrGdFailCpuAB_adr),           //power good/fail# signal for DDR5 DIMMs. Used to indicate the DIMMs to move data from volatil to non-volatil memory
   .oPwrGdFailCpuCD(wPwrGdFailCpuCD_adr),
   .oPwrGdFailCpuEF(wPwrGdFailCpuEF_adr),
   .oPwrGdFailCpuGH(wPwrGdFailCpuGH_adr)

   );

/////////////////////////////////////////////////////////////////////////////////
//////////  FPGA POSTCODES
/////////////////////////////////////////


   assign     wBmcFaultCode  = {1'b0/*timer expired or pwrgd fall*/,wBMC_PWR_P1V0_FAULT,wBMC_PWR_P1V2_FAULT,wBMC_PWR_P1V8_FAULT,wBMC_PWR_P2V5_FAULT};
   assign     wPchFaultCode  = {1'b0/*timer expired or pwrgd fall*/,wPCH_PWR_FLT_P1V05,wPCH_PWR_FLT_PVNN,1'b0/*PCH_P1V8*/};
   assign     wCpu0FaultCode = {1'b0/*timer expired or pwrgd fall*/,wCpu0PvccinFlt,wCpu0PvccdHvFlt,wCpu0PvnnMainFlt,wCpu0PvccInfaonFlt,wCpu0PvccfaFivrFlt,wCpu0PvccfaEhvFlt,wCpu0PvppHbmFlt};
   assign     wCpu1FaultCode = {1'b0/*timer expired or pwrgd fall*/,wCpu1PvccinFlt,wCpu1PvccdHvFlt,wCpu1PvnnMainFlt,wCpu1PvccInfaonFlt,wCpu1PvccfaFivrFlt,wCpu1PvccfaEhvFlt,wCpu1PvppHbmFlt};
   //if in CLX, DIMM failures are ignored as CLX uses DDR4 and it doesn't have a VR inside the DIMM
   assign     wCpuMemFlt = (FM_CPU0_INTR_PRSNT && FM_CPU1_PKGID2) ? {1'b0, wCpu1MemFlt[3], wCpu0MemFlt[3], wCpu1MemFlt[2], wCpu0MemFlt[2], wCpu1MemFlt[1], wCpu0MemFlt[1], wCpu1MemFlt[0], wCpu0MemFlt[0]} : 9'b000000000;



postcode_fub FPGA_POSTCODES
  (
   .iClk(iClk_2M),
   .iRst_n(iRST_N),

   .iBmcErrorCode(wBmcFaultCode),                                                                                                        //BMC Error Code (for 4 VRs, 8 error id's)
   .iPchErrorCode(wPchFaultCode),                  //PCH Error Code (for 3 VRs, 6 error id's)
   .iPsuErrorCode(wPsuFaultCode),                                                                 //PSU-MAIN Error Code (for 2 VRs, 4 error id's)
   .iCpu0ErrorCode(wCpu0FaultCode),                //CPU0 Error Code (for 7 VRs, 14 error id's)
   .iCpu1ErrorCode(wCpu1FaultCode),                //CPU1 Error Code (for 7 VRs, 14 error id's)
   .iMemErrorCode(wCpuMemFlt),                     //MEM Error Code (for 8 PWRGD_FAIL signals, 16 error id's)
   .oBmcFail(),                             //BMC failure indication for Master Seq
   .oPchFail(),                             //PCH failure indication for Master Seq
   .oPsuFail(),                             //PSU_MAIN failure indication for Master Seq
   .oCpuMemFail(),
   .oDisplay1(wFpgaPostCode2),              //on postcode fub, display1 is the LSNibble, while in LED_CONTROL is the MSNibble
   .oDisplay2(wFpgaPostCode1)
   );


   /////////////////////////////////////////////////////////////////////////////////
   //////////  LED CONTROL
   /////////////////////////////////////////


led_control
  #(.MAINFPGAVER(FPGA_REV), .DEBUGFPGAVER(FPGA_REV_DEBUG))
    LED_CTRL
   (
   .iClk(wClk2ms),                            //%Clock input
   .iRst_n(iRST_N),                            //%Reset enable on Low

   .iRstPltRst_n(RST_PLTRST_N_FF),             //PLTRST_N signal comming from PCH (used to automatically switch from FPGA to BIOS postcodes in displays)

   .iStatusLeds(wLED_STATUS),                                                    //right now [4:0] => from Bypass logic (FORCE_PWRON asserted), [5] =>  not used, [6] => ~PWRGD_VTT and [7] => iRST_N
   .oStatusLedSel(oFM_POSTLED_SEL),                                              //output generated by this module to select when STATUS LEDs should take into account the LED_CONTROL output pins

   .iFpgaPostCode1(wFpgaPostCode1),                                              //Main Fpga postcode for 7-segment display 1 (MSB) before PLT_RST_N is deasserted (already encoded for 7-seg Display)
   .iFpgaPostCode2(wFpgaPostCode2),                                              //Main Fpga postcode for 7-segment display 2 (LSB) before PLT_RST_N is deasserted (already encoded for 7-seg Display)
   .iBiosPostCode(wBiosPostCode),                                                //Port 80 post-codes from BIOS for the 2 7-Segment displays, after PLT_RST_N is deasserted
   .iPFRPostCode(iPFRPostCode),                                                  //PFR postcode to be displayed on 7-segment displays (MS nibble on Display1 and LS nibble on Display2),
                                                                                 // by asserting PFR override signal (overriding Fpga & BIOS post-codes)

   .iPFROverride(iPFROverride),                                     //PRF override signal, if asserted, PRF postcode data is displayed in 7-segment displays, otherwise,
                                                                                 // FPGA or BIOS postcodes will be displayed depending on RST_PLTRST_N signal

   .oPostCodeSel1_n(oPostCodeSel1_n),                               //to latch LED_CONTROL outputs into the 7-segment display1 (MSB) (active low)
   .oPostCodeSel2_n(oPostCodeSel2_n),                               //to latch LED_CONTROL outputs into the 7-segment display2 (LSB) (active low)

   .iLedFanFault(wFanFaultLEDs),                                    //input to be reflected at FAN FAULT LEDs, latched into LED_CONTROL when oLedFanFaultSel is asserted
   .oLedFanFaultSel_n(oLedFanFaultSel_n),                           //output to latch LED_CONTROL outputs into Fan Fault LEDs (active low)

   .iLedCpu0DimmCh1Fault(wCpu0DIMMLEDs[7:0]),                         //input for the Cpu0 Dimms 1&2 CH 1-4 Fault LEDs indications
   .oLedCpu0DimmCh1FaultSel(oFmCpu0DimmCh14FaultLedSel),            //to latch LED_CONTROL output into CPU0 Dimms 1&2 on CH 1-4 Fault LEDs

   .iLedCpu0DimmCh5Fault(wCpu0DIMMLEDs[15:8]),                        //input for the Cpu0 Dimms 1&2 CH 5-8 Fault LEDs indications
   .oLedCpu0DimmCh5FaultSel(oFmCpu0DimmCh58FaultLedSel),            //to latch LED_CONTROL output into CPU0 Dimm 1&2 on CH 5-8 Fault LEDs

   .iLedCpu1DimmCh1Fault(wCpu1DIMMLEDs[7:0]),                         //input for the Cpu1 Dimms 1&2 CH 1-4 Fault LEDs indications
   .oLedCpu1DimmCh1FaultSel(oFmCpu1DimmCh14FaultLedSel),            //to latch LED_CONTROL output into CPU1 Dimm1 on CH 1-4 Fault LEDs

   .iLedCpu1DimmCh5Fault(wCpu1DIMMLEDs[15:8]),                        //input for the Cpu1 Dimms 1&2 CH 5-8 Fault LEDs indications
   .oLedCpu1DimmCh5FaultSel(oFmCpu1DimmCh58FaultLedSel),            //to latch LED_CONTROL output into CPU1 Dimm2 on CH 5-8 Fault LEDs

   .iPldRev_n(wFM_PLD_REV_N_FF || iFM_FORCE_PWRON),                 //when asserted (active low) and not in FORCE_PWRON, will show Main/Debug FPGA versions in 7-Segment Displays 2&1

   .oLedControl(oLED_CONTROL)                                  //output of the mux to all LEDs resources
   );




///LAI



//////////////////////////////////////////////////////////////////////////////////
//VR bypass sequence
//////////////////////////////////////////////////////////////////////////////////
vr_bypass vrBypass_rtl
  (
   //.iClk(iClk_2M),                                                    //input clock signal (@2MHz)


   //using push-button for VRTB testing to enable step-by-step power up seq
   .iClk(wFM_PLD_REV_N_FF),


   .iRst_n(iRST_N),                                                   //input reset signal (active low)

                                                                      //these 3 are used as protection to avoid override if CPUs or PCH are present in board
   .iCpu0SktOcc_n(FM_CPU0_SKTOCC_LVT3_N),                             //Cpu0 socket occupy (active low)
   .iCpu1SktOcc_n(FM_CPU1_SKTOCC_LVT3_N),                             //Cpu1 socket occupy (active low)
   .iPchPrsnt_n(FM_PCH_PRSNT_N),                                      //PCH Present signal (active low)

   .iForcePwrOn(iFM_FORCE_PWRON),                                     //taken from jumper J1C1 (FM_FORCE_PWRON_LVC3_R) active HIGH : 1-2 LOW (default), 2-3 HIGH

   .iP2v5BmcAuxPwrGdBP(PWRGD_P2V5_BMC_AUX_FF),                        //PWRGD signals of BMC related VRs
   .iP1v2BmcAuxPwrGdBP(PWRGD_P1V2_BMC_AUX_FF),
   .iP1v0BmcAuxPwrGdBP(PWRGD_P1V0_BMC_AUX_FF),

   .iP1v8PchAuxPwrGdBP(PWRGD_P1V8_PCH_AUX_FF),                        //PWRGD signals of PCH related VRs
   .iP1v05PchAuxPwrGdBP(PWRGD_P1V05_PCH_AUX_FF),
   .iPvnnPchAuxPwrGdBP(PWRGD_PVNN_PCH_AUX_FF),
   .iPsuPwrOKBP(PWRGD_PS_PWROK_FF),
   .iP3v3MainPwrGdBP(PWRGD_P3V3_FF),

   .iPvppHbmCpu0PwrGdBP(PWRGD_PVPP_HBM_CPU0_FF),                       //PWRGD signals of CPU0 related VRs
   .iPvccfaEhvCpu0PwrGdBP(PWRGD_PVCCFA_EHV_CPU0_FF),
   .iPvccfaEhvfivraCpu0PwrGdBP(PWRGD_PVCCFA_EHV_FIVRA_CPU0_FF),
   .iPvccinfaonCpu0PwrGdBP(PWRGD_PVCCINFAON_CPU0_FF),
   .iPvnnMainCpu0PwrGdBP(PWRGD_PVNN_MAIN_CPU0_FF),
   .iPvccdHvCpu0PwrGdBP(PWRGD_PVCCD_HV_CPU0_FF),                       //PWRGD signals of MEM CPU0 related VRs
   .iPvccinCpu0PwrGdBP(PWRGD_PVCCIN_CPU0_FF),

   .iPvppHbmCpu1PwrGdBP(PWRGD_PVPP_HBM_CPU1_FF),                       //PWRGD signals of CPU1 related VRs
   .iPvccfaEhvCpu1PwrGdBP(PWRGD_PVCCFA_EHV_CPU1_FF),
   .iPvccfaEhvfivraCpu1PwrGdBP(PWRGD_PVCCFA_EHV_FIVRA_CPU1_FF),
   .iPvccinfaonCpu1PwrGdBP(PWRGD_PVCCINFAON_CPU1_FF),
   .iPvnnMainCpu1PwrGdBP(PWRGD_PVNN_MAIN_CPU1_FF),
   .iPvccdHvCpu1PwrGdBP(PWRGD_PVCCD_HV_CPU1_FF),                       //PWRGD signals of CPU1 related VRs
   .iPvccinCpu1PwrGdBP(PWRGD_PVCCIN_CPU1_FF),


   .oLedStatus(wLED_STATUS[4:0]),                                    //to status leds for visibility in the board

   .oP2v5BmcAuxEnaBP(wFM_P2V5_BMC_AUX_EN_BP),                         //BMC related VR enables
   .oP1v2BmcAuxEnaBP(wFM_P1V2_BMC_AUX_EN_BP),
   .oP1v0BmcAuxEnaBP(wFM_P1V0_BMC_AUX_EN_BP),

                                                     //PCH related VR enables
   .oP1v8PchAuxEnaBP(wFM_PCH_P1V8_AUX_EN_BP),
   .oP1v05PchAuxEnaBP(wFM_PCH_P1V05_AUX_EN_BP),
   .oPvnnPchAuxEnaBP(wFM_PCH_PVNN_AUX_EN_BP),

   .oFmPldClk25MEna(wFM_PLD_CLK_25M_EN_BP),                           //25MHz clk enable
   .oFmPldClksDevEna(wFM_PLD_CLKS_DEV_EN_BP),                         //CKMNG enable for 100MHz clocks

                                                     //PSU related enables
   .oPsEnaBP(wFM_PS_EN_BP),
   .oP5vEnaBP(wFM_P5V_EN_BP),

                                                     //S3 SW related enables
   .oAuxSwEnaBP(wFM_AUX_SW_EN_BP),                              //select source for 12V AUX from STBY or MAIN
   .oSxSwP12vStbyEnaBP(wFM_SX_SW_P12V_STBY_EN_BP),                       //Switch ctrl to select 12V source for MEM VRs
   .oSxSwP12vEnaBP(wFM_SX_SW_P12V_EN_BP),


                                                     //CPU0 related VR enables
   .oPvppHbmCpu0EnaBP(wFM_PVPP_HBM_CPU0_EN_BP),
   .oPvccfaEhvCpu0EnaBP(wFM_PVCCFA_EHV_CPU0_EN_BP),
   .oPvccfaEhvfivraCpu0EnaBP(wFM_PVCCFA_EHV_FIVRA_CPU0_EN_BP),
   .oPvccinfaonCpu0EnaBP(wFM_PVCCINFAON_CPU0_EN_BP),
   .oPvnnMainCpu0EnaBP(wFM_PVNN_MAIN_CPU0_EN_BP),

                                                     //MEM CPU0 related enables
   .oPvccdHvCpu0EnaBP(wFM_PVCCD_HV_CPU0_EN_BP),
   .oPvccinCpu0EnaBP(wFM_PVCCIN_CPU0_EN_BP),

                                                     //CPU1 related VR enables
   .oPvppHbmCpu1EnaBP(wFM_PVPP_HBM_CPU1_EN_BP),
   .oPvccfaEhvCpu1EnaBP(wFM_PVCCFA_EHV_CPU1_EN_BP),
   .oPvccfaEhvfivraCpu1EnaBP(wFM_PVCCFA_EHV_FIVRA_CPU1_EN_BP),
   .oPvccinfaonCpu1EnaBP(wFM_PVCCINFAON_CPU1_EN_BP),
   .oPvnnMainCpu1EnaBP(wFM_PVNN_MAIN_CPU1_EN_BP),

                                                     //MEM CPU1 related enables
   .oPvccdHvCpu1EnaBP(wFM_PVCCD_HV_CPU1_EN_BP),
   .oPvccinCpu1EnaBP(wFM_PVCCIN_CPU1_EN_BP)
   );

///////////////////////////////////////////////////////////////////////////////////////////

//bypass mux to select from bypassed signals or normal signals depending on J1C1 jumper position
   //if JUMPER @ 2-3 (HIGH) bypass enabled, if jumper @ 1-2 (LOW) bypass disabled

   assign FM_P2V5_BMC_AUX_EN    = iFM_FORCE_PWRON ? wFM_P2V5_BMC_AUX_EN_BP                : wFM_P2V5_BMC_AUX_EN;
   assign FM_PCH_P1V8_AUX_EN    = iFM_FORCE_PWRON ? wFM_PCH_P1V8_AUX_EN_BP                : wFM_PCH_P1V8_AUX_EN;
   assign FM_P1V2_BMC_AUX_EN    = iFM_FORCE_PWRON ? wFM_P1V2_BMC_AUX_EN_BP                : wFM_P1V2_BMC_AUX_EN;
   assign FM_P1V0_BMC_AUX_EN    = iFM_FORCE_PWRON ? wFM_P1V0_BMC_AUX_EN_BP                : wFM_P1V0_BMC_AUX_EN;
   assign FM_P1V05_PCH_AUX_EN   = iFM_FORCE_PWRON ? wFM_PCH_P1V05_AUX_EN_BP               : wFM_PCH_P1V05_AUX_EN;
   assign FM_PVNN_PCH_AUX_EN    = iFM_FORCE_PWRON ? wFM_PCH_PVNN_AUX_EN_BP                : wFM_PCH_PVNN_AUX_EN;
   assign FM_PS_EN              = iFM_FORCE_PWRON ? wFM_PS_EN_BP                          : wFM_PS_EN;
   assign FM_P5V_EN             = iFM_FORCE_PWRON ? wFM_P5V_EN_BP                         : wFM_P5V_EN;
   assign FM_AUX_SW_EN          = iFM_FORCE_PWRON ? wFM_AUX_SW_EN_BP                      : wFM_AUX_SW_EN;
   assign FM_SX_SW_P12V_STBY_EN = iFM_FORCE_PWRON ? wFM_SX_SW_P12V_STBY_EN_BP             : wFM_SX_SW_P12V_STBY_EN;
   assign FM_SX_SW_P12V_EN      = iFM_FORCE_PWRON ? wFM_SX_SW_P12V_EN_BP                  : wFM_SX_SW_P12V_EN;
   assign FM_PVPP_HBM_CPU0_EN   = iFM_FORCE_PWRON ? wFM_PVPP_HBM_CPU0_EN_BP               : wFM_PVPP_HBM_CPU0_EN;
   assign FM_PVCCFA_EHV_CPU0_EN = iFM_FORCE_PWRON ? wFM_PVCCFA_EHV_CPU0_EN_BP             : wFM_PVCCFA_EHV_CPU0_EN;
   assign FM_PVCCFA_EHV_FIVRA_CPU0_EN = iFM_FORCE_PWRON ? wFM_PVCCFA_EHV_FIVRA_CPU0_EN_BP : wFM_PVCCFA_EHV_FIVRA_CPU0_EN;
   assign FM_PVCCINFAON_CPU0_EN = iFM_FORCE_PWRON ? wFM_PVCCINFAON_CPU0_EN_BP             : wFM_PVCCINFAON_CPU0_EN;

   assign FM_PVNN_MAIN_CPU0_EN  = iFM_FORCE_PWRON ? wFM_PVNN_MAIN_CPU0_EN_BP              : wFM_PVNN_MAIN_CPU0_EN;

   // assign FM_PVNN_MAIN_CPU0_EN  = PWRGD_P3V3_FF;
   // assign FM_PVNN_MAIN_CPU1_EN  = PWRGD_P3V3_FF;
   // assign PWRGD_PCH_PWROK   = PWRGD_PVNN_MAIN_CPU0_FF && PWRGD_PVNN_MAIN_CPU1_FF;
   // assign PWRGD_SYS_PWROK   = PWRGD_PVNN_MAIN_CPU0_FF && PWRGD_PVNN_MAIN_CPU1_FF;

   assign FM_PVCCD_HV_CPU0_EN   = iFM_FORCE_PWRON ? wFM_PVCCD_HV_CPU0_EN_BP               : wFM_PVCCD_HV_CPU0_EN;
   assign FM_PVCCIN_CPU0_EN     = iFM_FORCE_PWRON ? wFM_PVCCIN_CPU0_EN_BP                 : wFM_PVCCIN_CPU0_EN;
   assign FM_PVPP_HBM_CPU1_EN   = iFM_FORCE_PWRON ? wFM_PVPP_HBM_CPU1_EN_BP               : wFM_PVPP_HBM_CPU1_EN;
   assign FM_PVCCFA_EHV_CPU1_EN = iFM_FORCE_PWRON ? wFM_PVCCFA_EHV_CPU1_EN_BP             : wFM_PVCCFA_EHV_CPU1_EN;
   assign FM_PVCCFA_EHV_FIVRA_CPU1_EN = iFM_FORCE_PWRON ? wFM_PVCCFA_EHV_FIVRA_CPU1_EN_BP : wFM_PVCCFA_EHV_FIVRA_CPU1_EN;
   assign FM_PVCCINFAON_CPU1_EN = iFM_FORCE_PWRON ? wFM_PVCCINFAON_CPU1_EN_BP             : wFM_PVCCINFAON_CPU1_EN_MUX;
   assign FM_PVNN_MAIN_CPU1_EN  = iFM_FORCE_PWRON ? wFM_PVNN_MAIN_CPU1_EN_BP              : wFM_PVNN_MAIN_CPU1_EN_MUX;
   assign FM_PVCCD_HV_CPU1_EN   = iFM_FORCE_PWRON ? wFM_PVCCD_HV_CPU1_EN_BP               : wFM_PVCCD_HV_CPU1_EN;
   assign FM_PVCCIN_CPU1_EN     = iFM_FORCE_PWRON ? wFM_PVCCIN_CPU1_EN_BP                 : wFM_PVCCIN_CPU1_EN;

   assign FM_PLD_CLK_25M_EN     = iFM_FORCE_PWRON ? wFM_PLD_CLK_25M_EN_BP                 : wFM_PLD_CLK_25M_EN;
   assign FM_PLD_CLKS_DEV_EN    = iFM_FORCE_PWRON ? wFM_PLD_CLKS_DEV_EN_BP                : wFM_PLD_CLKS_DEV_EN;



   assign wLED_STATUS[5] = 1'b0;   //turn off unused STATUS LEDs (for VRTB BYPASS ONLY)
   assign wLED_STATUS[6] = ~iPWRGD_VTT_PWRGD;
   assign wLED_STATUS[7] = iRST_N;


///////////////////////////////////////////////////////////


   assign oFM_SYS_THROTTLE_N = wFM_SYS_THROTTLE_N;

   
   //CPU error
   assign FM_CPU_ERR0_LVT3_N = H_CPU_ERR0_LVC1_N;
   assign FM_CPU_ERR1_LVT3_N = H_CPU_ERR1_LVC1_N;
   assign FM_CPU_ERR2_LVT3_N = H_CPU_ERR2_LVC1_N;
   

endmodule
