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

module Archer_City_Main_wrapper (
// Input CLK
    input   iClk_2M,
    input   iClk_50M,
    input   ipll_locked,

	//BMC SGPIO
	input        SGPIO_BMC_CLK,
	input        SGPIO_BMC_DOUT,
	output       SGPIO_BMC_DIN_R,
	input        SGPIO_BMC_LD_N,

	//LED and 7-Seg Control Logic
	output LED_CONTROL_0,
	output LED_CONTROL_1,
	output LED_CONTROL_2,
	output LED_CONTROL_3,
	output LED_CONTROL_4,
	output LED_CONTROL_5,
	output LED_CONTROL_6,
	output LED_CONTROL_7,
	output       FM_CPU0_DIMM_CH1_4_FAULT_LED_SEL,
	output       FM_CPU0_DIMM_CH5_8_FAULT_LED_SEL,
	output       FM_CPU1_DIMM_CH1_4_FAULT_LED_SEL,
	output       FM_CPU1_DIMM_CH5_8_FAULT_LED_SEL,
	output       FM_FAN_FAULT_LED_SEL_R_N,
	output       FM_POST_7SEG1_SEL_N,
	output       FM_POST_7SEG2_SEL_N,
	output       FM_POSTLED_SEL,

	//CATERR / RMCA / NMI Logic
	//output       FM_CPU_CATERR_DLY_N, //alarios 20-ago-20 : removed per Arch Request (no longer needed to go to PCH)
	input        H_CPU_CATERR_LVC1_R_N,
	//input        H_CPU_RMCA_LVC1_R_N,          //removed per Arch request (alarios 13-ago-20)
	output       FM_CPU_CATERR_LVT3_R_N, //alarios 13-ago-20 : former FM_CPU_RMCA_CATERR_LVT3_R_N
	output       H_CPU_NMI_LVC1_R,
    output       IRQ_BMC_PCH_NMI, //alarios 12-ago-20 : added per Arch Request, goes to PCH
	input        IRQ_BMC_CPU_NMI,
	input        IRQ_PCH_CPU_NMI_EVENT_N,
    input        FM_BMC_NMI_PCH_EN,   //alarios 13-ago-20: added per Arch Request, selects if BMC NMI goes to CPUs or PCH (comes from board jumper J4000)

	//ADR
	input        FM_ADR_COMPLETE,
	output       FM_ADR_ACK_R,
	// output	FM_ADR_SMI_GPIO_N,
	output       FM_ADR_TRIGGER_N,
	// input	FM_PLD_PCH_DATA_R,
	input   FM_DIS_PS_PWROK_DLY, // named as FM_ADR_EXT_TRIGGER_N in common core
    input        FM_ADR_MODE0, //From PCH, indicates if ADR is enabled and what flow is enabled
    input        FM_ADR_MODE1,
                             
	// input	FM_DIS_PS_PWROK_DLY,
	
	//SmaRT Logic
	input        FM_PMBUS_ALERT_B_EN,
	input        FM_THROTTLE_R_N,
	inout        IRQ_SML1_PMBUS_PLD_ALERT_N,
	output       FM_SYS_THROTTLE_R_N,
	
	//Termtrip dly
	input        H_CPU0_THERMTRIP_LVC1_N,
	input        H_CPU1_THERMTRIP_LVC1_N,
	output       FM_THERMTRIP_DLY_LVC1_R_N,
	
	//MEMTRIP
	input        H_CPU0_MEMTRIP_LVC1_N,
	input        H_CPU1_MEMTRIP_LVC1_N,
	
	// PROCHOT
	input        IRQ_CPU0_VRHOT_N,
	input        IRQ_CPU1_VRHOT_N,
	output       H_CPU0_PROCHOT_LVC1_R_N,
	output       H_CPU1_PROCHOT_LVC1_R_N,
	
	//PERST & RST
	input        FM_RST_PERST_BIT0,
	input        FM_RST_PERST_BIT1,
	input        FM_RST_PERST_BIT2,
	output       RST_PLD_PCIE_CPU0_DEV_PERST_N,
	output       RST_PLD_PCIE_CPU1_DEV_PERST_N,
	output       RST_PLD_PCIE_PCH_DEV_PERST_N,
	output       RST_CPU0_LVC1_R_N,
	output       RST_CPU1_LVC1_R_N,
	output       RST_PLTRST_PLD_B_N,
	input        RST_PLTRST_PLD_N,
	
	//SysChk CPU
	input        FM_CPU0_PKGID0,
	input        FM_CPU0_PKGID1,
	input        FM_CPU0_PKGID2,
	input        FM_CPU0_PROC_ID0,
	input        FM_CPU0_PROC_ID1,
	input        FM_CPU0_INTR_PRSNT,
	input        FM_CPU0_SKTOCC_LVT3_PLD_N,
	input        FM_CPU1_PKGID0,
	input        FM_CPU1_PKGID1,
	input        FM_CPU1_PKGID2,
	input        FM_CPU1_PROC_ID0,
	input        FM_CPU1_PROC_ID1,
	input        FM_CPU1_SKTOCC_LVT3_PLD_N,
	
	
	//BMC
	//input        FM_BMC_INTR_PRSNT,   //alarios 13-ago-20 (reporpused in favor of FM_BMC_NMI_PCH_EN)
    input        FP_BMC_PWR_BTN_R2_N,
	input        FM_BMC_PWRBTN_OUT_N,
	output       FM_BMC_PFR_PWRBTN_OUT_R_N,
	input        FM_BMC_ONCTL_N_PLD,
	output	RST_SRST_BMC_PLD_R_N_REQ,
    input   RST_SRST_BMC_PLD_R_N,
	output       FM_P2V5_BMC_AUX_EN,
	input        PWRGD_P2V5_BMC_AUX,
	output       FM_P1V2_BMC_AUX_EN,
	input        PWRGD_P1V2_BMC_AUX,
	output       FM_P1V0_BMC_AUX_EN,
	input        PWRGD_P1V0_BMC_AUX,


	//PCH
	output	RST_RSMRST_PLD_R_N_REQ,
    input   RST_RSMRST_PLD_R_N,
	output       PWRGD_PCH_PWROK_R,
	output       PWRGD_SYS_PWROK_R,
	input        FM_SLP_SUS_RSM_RST_N,
	input        FM_SLPS3_PLD_N,
	input        FM_SLPS4_PLD_N,
	input        FM_PCH_PRSNT_N,
	output       FM_PCH_P1V8_AUX_EN,
	input        PWRGD_P1V05_PCH_AUX,
	input        PWRGD_P1V8_PCH_AUX_PLD,
		
	output       FM_P1V05_PCH_AUX_EN,
	output       FM_PVNN_PCH_AUX_EN,
	input        PWRGD_PVNN_PCH_AUX,
	
	//PSU Ctl
	output       FM_PS_EN_PLD_R,
	input        PWRGD_PS_PWROK_PLD_R,
	
	//Clock Logic
	output       FM_PLD_CLKS_DEV_R_EN,
	output       FM_PLD_CLK_25M_R_EN,
	
	//PLL reset Main FPGA
	input        PWRGD_P1V2_MAX10_AUX_PLD_R, //PWRGD_P3V3_AUX in VRTB board
	
	//Main VR & Logic
	input        PWRGD_P3V3,
	output       FM_P5V_EN,
	output       FM_AUX_SW_EN,
	
	//CPU VRs
    output       FM_HBM2_HBM3_VPP_SEL, //alarios: for selecting between HBM2 (2.5V) or HBM3 (1.8V) depending if we have SPR or GNR (respectively)
                             
	output       PWRGD_CPU0_LVC1_R,
	output       PWRGD_CPU1_LVC1_R,
	input        PWRGD_CPUPWRGD_LVC1,
	output       PWRGD_PLT_AUX_CPU0_LVT3_R,
	output       PWRGD_PLT_AUX_CPU1_LVT3_R,
		
	output       FM_PVCCIN_CPU0_R_EN,
	input        PWRGD_PVCCIN_CPU0,
	output       FM_PVCCFA_EHV_FIVRA_CPU0_R_EN,
	input        PWRGD_PVCCFA_EHV_FIVRA_CPU0,
	output       FM_PVCCINFAON_CPU0_R_EN,
	input        PWRGD_PVCCINFAON_CPU0,
	output       FM_PVCCFA_EHV_CPU0_R_EN,
	input        PWRGD_PVCCFA_EHV_CPU0,
	output       FM_PVCCD_HV_CPU0_EN,
	input        PWRGD_PVCCD_HV_CPU0,
	output       FM_PVNN_MAIN_CPU0_EN,
	input        PWRGD_PVNN_MAIN_CPU0,
	output       FM_PVPP_HBM_CPU0_EN,
	input        PWRGD_PVPP_HBM_CPU0,
	output       FM_PVCCIN_CPU1_R_EN,
	input        PWRGD_PVCCIN_CPU1,
	output       FM_PVCCFA_EHV_FIVRA_CPU1_R_EN,
	input        PWRGD_PVCCFA_EHV_FIVRA_CPU1,
	output       FM_PVCCINFAON_CPU1_R_EN,
	input        PWRGD_PVCCINFAON_CPU1,
	output       FM_PVCCFA_EHV_CPU1_R_EN,
	input        PWRGD_PVCCFA_EHV_CPU1,
	output       FM_PVCCD_HV_CPU1_EN,
	input        PWRGD_PVCCD_HV_CPU1,
	output       FM_PVNN_MAIN_CPU1_EN,
	input        PWRGD_PVNN_MAIN_CPU1,
	output       FM_PVPP_HBM_CPU1_EN,
	input        PWRGD_PVPP_HBM_CPU1,
	//input        IRQ_PSYS_CRIT_N,   //arch-request, no longer POR (alarios 13-ag0-20)
	
	//Dediprog Detection Support
	input        RST_DEDI_BUSY_PLD_N,

	//Debug Signals
	input        FM_FORCE_PWRON_LVC3,
	output       FM_PLD_HEARTBEAT_LVC3,

    //DEBUG SGPIO (Debug to Main FPGAs)
	input        SGPIO_DEBUG_PLD_CLK,
	input        SGPIO_DEBUG_PLD_DOUT,
    output       SGPIO_DEBUG_PLD_DIN,
	input        SGPIO_DEBUG_PLD_LD,
                             
	//input	SMB_DEBUG_PLD_SCL,
	//inout	SMB_DEBUG_PLD_SDA,
	input        FM_PLD_REV_N,

	// PFR Dedicated Signals driven from common core logic
	output FM_PFR_MUX_OE_CTL_PLD,
	output FM_PFR_SLP_SUS_EN_R_N,
	
	//Crashlog & Crashdump
	input        FM_BMC_CRASHLOG_TRIG_N,
	output       FM_PCH_CRASHLOG_TRIG_R_N,
	input        FM_PCH_PLD_GLB_RST_WARN_N,

	//Reset from CPU to VT to CPLD
	input        M_AB_CPU0_RESET_N,
	input        M_CD_CPU0_RESET_N,
	input        M_EF_CPU0_RESET_N,
	input        M_GH_CPU0_RESET_N,
	input        M_AB_CPU1_RESET_N,
	input        M_CD_CPU1_RESET_N,
	input        M_EF_CPU1_RESET_N,
	input        M_GH_CPU1_RESET_N,

	//Reset from CPLD to VT to DIMM
	output       M_AB_CPU0_FPGA_RESET_R_N,
	output       M_CD_CPU0_FPGA_RESET_R_N,
	output       M_EF_CPU0_FPGA_RESET_R_N,
	output       M_GH_CPU0_FPGA_RESET_R_N,
	output       M_AB_CPU1_FPGA_RESET_R_N,
	output       M_CD_CPU1_FPGA_RESET_R_N,
	output       M_EF_CPU1_FPGA_RESET_R_N,
	output       M_GH_CPU1_FPGA_RESET_R_N,
	//Prochot & Memhot
	input        IRQ_CPU0_MEM_VRHOT_N,
	input        IRQ_CPU1_MEM_VRHOT_N,

	output       H_CPU0_MEMHOT_IN_LVC1_R_N,
	output       H_CPU1_MEMHOT_IN_LVC1_R_N,

	input        H_CPU0_MEMHOT_OUT_LVC1_N,
	input        H_CPU1_MEMHOT_OUT_LVC1_N,

	inout        PWRGD_FAIL_CPU0_AB_PLD,
	inout        PWRGD_FAIL_CPU0_CD_PLD,
	inout        PWRGD_FAIL_CPU0_EF_PLD,
	inout        PWRGD_FAIL_CPU0_GH_PLD,
	inout        PWRGD_FAIL_CPU1_AB_PLD,
	inout        PWRGD_FAIL_CPU1_CD_PLD,
	inout        PWRGD_FAIL_CPU1_EF_PLD,
	inout        PWRGD_FAIL_CPU1_GH_PLD,

	output       PWRGD_DRAMPWRGD_CPU0_AB_R_LVC1,
	output       PWRGD_DRAMPWRGD_CPU0_CD_R_LVC1,
	output       PWRGD_DRAMPWRGD_CPU0_EF_R_LVC1,
	output       PWRGD_DRAMPWRGD_CPU0_GH_R_LVC1,
	output       PWRGD_DRAMPWRGD_CPU1_AB_R_LVC1,
	output       PWRGD_DRAMPWRGD_CPU1_CD_R_LVC1,
	output       PWRGD_DRAMPWRGD_CPU1_EF_R_LVC1,
	output       PWRGD_DRAMPWRGD_CPU1_GH_R_LVC1,

	//IBL
//	output	FM_IBL_ADR_TRIGGER_PLD_R,
//	input	FM_IBL_ADR_COMPLETE_CPU0,
//	input	FM_IBL_ADR_ACK_CPU0_R,
//	output	FM_IBL_WAKE_CPU0_R_N,
//	output	FM_IBL_SYS_RST_CPU0_R_N,
//	output	FM_IBL_PWRBTN_CPU0_R_N,
//	output	FM_IBL_GRST_WARN_PLD_R_N,
//	input	FM_IBL_PLT_RST_SYNC_N,
//	input	FM_IBL_SLPS3_CPU0_N,
//	input	FM_IBL_SLPS4_CPU0_N,
//	input	FM_IBL_SLPS5_CPU0_N,
//	input	FM_IBL_SLPS0_CPU0_N,   //alarios: PIN_L14; it got disconnected for FAB3 as was moved into a 1V bank
    inout        PWRGD_S0_PWROK_CPU0_LVC1_R, //alarios: PIN_U18; this is instead of FM_IBL_SLPS0_CPU0_N (changed name and IO bank)
                                             //This is reporpused for CLX and becomes the VTT PWRGD signal (input).
                                             //For SPR (or GNR) this will be output and we need to control it accordingly
                             
    output       PWRGD_S0_PWROK_CPU1_LVC1_R, //PIN_U17: this one does is not reporpused for CLX (CPU1 does not have it connected in board), 
                                            //thus, no need to reporpuse, control, etc

	//Errors
	input        H_CPU_ERR0_LVC1_R_N,
	input        H_CPU_ERR1_LVC1_R_N,
	input        H_CPU_ERR2_LVC1_R_N,
	output       FM_CPU_ERR0_LVT3_N,
	output       FM_CPU_ERR1_LVT3_N,
	output       FM_CPU_ERR2_LVT3_N,
	input        H_CPU0_MON_FAIL_PLD_LVC1_N,
	input        H_CPU1_MON_FAIL_PLD_LVC1_N,

	//eMMC
//	output	FM_EMMC_PFR_R_EN,
//
//	inout	EMMC_MUX_BMC_PFR_OUT_CMD,
//	inout	EMMC_MUX_BMC_PFR_LVT3_R_CMD,
//	inout	RST_EMMC_MUX_BMC_PFR_LVT3_R_N,
//	inout	EMMC_MUX_BMC_PFR_LVT3_DATA0_R,
//	inout	EMMC_MUX_BMC_PFR_LVT3_DATA1_R,
//
//	inout	EMMC_MUX_BMC_PFR_LVT3_DATA2_R,
//
//	inout	EMMC_MUX_BMC_PFR_LVT3_DATA3_R,
//	inout	EMMC_MUX_BMC_PFR_LVT3_DATA4_R,
//	inout	EMMC_MUX_BMC_PFR_LVT3_DATA5_R,
//	inout	EMMC_MUX_BMC_PFR_LVT3_DATA6_R,
//	inout	EMMC_MUX_BMC_PFR_LVT3_DATA7_R,
//
//	inout	EMMC_MUX_BMC_PFR_LVT3_R_CLK,

	//EMG Val board
	input        FM_VAL_BOARD_PRSNT_N,

	//clock gen

	//In-Field System Test 
//	input	H_CPU0_IFST_DONE_PLD_LVC1,
//	input	H_CPU0_IFST_TRIG_PLD_LVC1,
//	input	H_CPU1_IFST_DONE_PLD_LVC1,
//	input	H_CPU1_IFST_TRIG_PLD_LVC1,
//	output	H_CPU0_IFST_KOT_PLD_LVC1,
//	output	H_CPU1_IFST_KOT_PLD_LVC1,

	//S3 Pwr-seq control
	output       FM_SX_SW_P12V_STBY_R_EN,
	output       FM_SX_SW_P12V_R_EN,
	input        FM_DIMM_12V_CPS_S5_N,

	// IDV Tool SGPIO
	input        SGPIO_IDV_CLK_R,
	input        SGPIO_IDV_DOUT_R,
	output       SGPIO_IDV_DIN_R,
	input        SGPIO_IDV_LD_R_N,

    //S3M
    input        FM_S3M_CPU0_CPLD_CRC_ERROR,
    input        FM_S3M_CPU1_CPLD_CRC_ERROR,

	// Interposer PFR reuse ports
	inout        SMB_S3M_CPU0_SDA_LVC1,
	input        SMB_S3M_CPU0_SCL_LVC1,
	inout        SMB_S3M_CPU1_SDA_LVC1,
	input	     SMB_S3M_CPU1_SCL_LVC1,
    
// HPFR signals
	input	HPFR_TO_MISC,
	output	HPFR_FROM_MISC,
	output	FM_MODULAR_LEGACY,
	output  FM_MODULAR_STANDALONE
    
);

//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////
localparam  LOW =1'b0;
localparam  HIGH=1'b1;  
localparam  Z=1'bz;

wire wFM_ADR_EXT_TRIGGER_N;
wire [7:0]LED_CONTROL;

assign LED_CONTROL_0 = LED_CONTROL[0];
assign LED_CONTROL_1 = LED_CONTROL[1];
assign LED_CONTROL_2 = LED_CONTROL[2];
assign LED_CONTROL_3 = LED_CONTROL[3];
assign LED_CONTROL_4 = LED_CONTROL[4];
assign LED_CONTROL_5 = LED_CONTROL[5];
assign LED_CONTROL_6 = LED_CONTROL[6];
assign LED_CONTROL_7 = LED_CONTROL[7];

/// Instantiate the Main
Archer_City_Main
  #(.FPGA_REV(8'h38), .FPGA_REV_TEST(8'h00))  //version 2.8 official release:  Checking of BMC_ONCTL_N assertion before transitioning to 0.3. on master seq
                                              //                               Reported failure postcode on DIMMs when DIMM not failing is fixed
                                              //                               VR postcode reporting priorities changed to have VCCD_HV as the 1st
                                              //                               Added BaseVer and TestVer for Main & Secondary Fpgas on Displays when S2 is pressed
                                              //                               Main Fpga receives Secondary Fpga versions (base and test) via DEBUG SGPIO I/F
                                              //version 2.8 test 0.1 ->   CPU_RST bypass from PLTRST directly (removed the synchronizer in between
                                              //                     ->   added pins for IDV tool, and corrected BMC SGPIO signals sent to BMC
                                              //version 2.8 test 0.2 ->   Tied input of master sequencer for BMC_ONCTL_N to LOW (to skip checking ONCTL assertion from BMC)
                                              //version 2.8 test 0.3 ->   replaced the SMBus bypass moduel from i2c_relay_two_address to smbus_filtered_relay
                                              //                          first one was provided by FPR, but it seems it is an old version so we want to try the new  version
                                              //                          to address some issues seeing in this link. If this works, we probably replace all smbus bypass
                                              //                          for this one
                                              //version 2.8 test 0.4 ->   corrected an issue with DRAM_PWROK as it was not being de-asserted when shutting down to S5
                                              //version 2.8 test 0.5 ->   Added S5 indication to ddr5_pwrgd_logic so it turns off when going to S5 and not otherwise
                                              //                          not when going to S3, etc.
                                              //version 2.8 test 0.6 ->   ddr5_pwrgd_logic fsm should pwrdwn only with S5 to accelerate deasserting pwrgd_fail#, causing
                                              //                          both, pwrgd_fail and dram_pwrok to be deasserted at the same time, causing dimm_rst to be asserted after
                                              //version 2.8 test 0.7 ->   catched an issue in S3 P12V switch. Fixed in this version (were using PSUPWROK to transition to S3
                                              //                          instead of S3 signal, causing a race condition when switching from MAIN to STBY in the Memory rails
                                              //                          also catched some wrong conditions in the psu_sw_logic (s3 sw control) module that caused that mem VR & DIMMs
                                              //                          be incorrectly turned off when going to S3
                                              //version 2.8 test 0.8 ->   Added FM_S3_SW_P12V_EN from SW control to PSU control in order to know when it is safe to turn PSU MAIN off
                                              //version 2.8 test 0.9 ->   added different 500msec timer to the s3 sw to see if this fixes the issue
                                              //version 2.8 test 0.A ->   modified the switch control for 12V_AUX, to not switch to standby until we are about to turn PSU off
                                              //                          ensuring VRs are off, etc, return timer to 3msec instead of 500msec
                                              //version 2.8 test 0.B ->   sync'ed S3 SW logic with PSU PWR CHK logic for the power down and changed . Changed timer to 3msec
                                              //version 2.8 test 0.C ->   changed timer to 200 msec
                                              //version 2.9          ->   Official release: we are promoting 2v8 test 0.C to be 2v9.
                                              //                     ->   We added one change also for Modular Board design; ONCTL_N is bypassed when in modular, while RP checks for it (again)
                                              //version 2.9 test 0.1 ->   Fixed connection for ddr5 logic for CPU1 (missed to connect S5 indication signal
                                              //version 2.A Official Rel  ease : Promoted 2.9_T01
                                              //version 2.A test 0.1 ->   Modifying ADR accoding to latest ARCH changes: added FM_ADR_MODE[1:0] and changed FM_DIS_PS_PWROK_DLY to FM_ADR_EXT_TRIGGER_N
                                              //                          for now, only ADR Legacy is supported
                                              //                          Also, on ADR state, adding jump back to PCH state on pwrup seq
                                              //version 2.A test 0.2 ->   corrected a minor issue where AdrCompleteFsm was toggling between IDL and INIT while waiting for ADR_COMPLETE
                                              //version 2.A test 0.3 ->   EVBoard requires BMC_ONCTL_N to be overriden on FPGA when present in order to boot as it kidnaps ESPI, hence board BMC does not receive
                                              //                          SLP de-assertions from PCH (done thru ESPI), causing BMC not to assert ONCTL_N. This change is done here
                                              //                          Also, ADR needs to be enabled (thru ADR_MODE 1/0 signals all the time or it will re init the ADR_TRIGGER FSM
                                              //version 2.A test 0.4 ->   Instrumented code to be able to monitor Fpga and BIOS postCodes using the In-System Sources & Probes
                                              //                          and modified the logic table for the detection of EBG, LBG (N-1) & EVB as it was incorrect
                                              //version 2.A test 0.5 ->   instrumented some debug for the EVB, also corrected the DRAM_PWROK to MCs. It was merged so when on DIMM went down, all DRAM_PWROK to all MCs of
                                              //                          the affected CPU, were de-asserted. We removed the merge (and-gates) to have them independent with MC resolution
                                              //version 2.B Official Release: Besides the changes from test versions we added, per Arch Request, changes in the logic for CATERR (removed RMCA from equation),
                                              //                              CATERR_DLY_N signal removed and ME_AUTHN_FAIL took that pin, the previously used pin for ME_AUTHN_FAIL is now unused (so far). 
                                              //                              Modified the NMI logic (reporpused BMC_INTR_PRSNT in favor of FM_BMC_NMI_PCH_EN which selects if BMC NMI is sent to CPUs or PCH
                                              //                              Also instrumented the code so more power-sequence status signals can be monitored thru JTAG with the usb-blaster
                                              //                              This is the first version that supports the R-FAT TOOL (Remote FPGA Access Tool)
                                              //                              Also implemented a 'latch' for THERMTRIP and MEMTRIP signals going to BMC using CPUPWRGD as the enable of the latch
                                              //                              Improved Glitchfilter so it has an enable AND a synch reset (so for other signals using the synch rst, they are restored to default when the
                                              //                              synch rst input goes low.
                                              //version 2vB test 0.1 ->   Arch Request to support an Emulated ADR flow with R-DIMMs (to enable val team). This requires lots of changes
                                              //                          Modified the psu_sw_logic.v to invert logic for SLP signals (mostly S3) so now the signal comes from the master_fub.v
                                              //                          Modified the master_fub.v to receive the AdrAck as an indication the pwr-dwn was due to an ADR flow
                                              //                          Also, by monitoring the ADR_MODE signals, detects if we are in this Emulated ADR mode (2'b10), if so, when system goes to S5, if pwr-dwn is due to ADR
                                              //                          the FPGA should act as if we are going to S3 instead, otherwise normal to S5
                                              //                          Modified the Archer_City_Main.v to do proper connections
                                              //                          Finally, modified the top for the version number and comments but also to instrument debug hooks to test these logic changes etc
                                              //version 2vB test 0.2 ->   Found a bug in the logic for the control of the S3 SW using the new S3/S5 indications. Changed the logic to see if this fixes it
                                              //                          Found a bug in the ddr5 logic FSM when ADR is happening. Due to ADR, pwrgdfail signals are driven low by the ADR logic in the FPGA but 
                                              //                          the ddr5 fsm didn't notice it was due to ADR, hence it was evaluating a fault in the dimms, moving the FSM to FAIL ST. Corrected this by 
                                              //                          adding the ADR indication into the transition condition to distinguish it happened due to an ADR flow
                                              //version 2vB test 0.3 ->   To help testing S3M smbus, we convert both SMBus links into outputs (both SDL & SCA) and drive Z on all 4 ports
                                              //version 2vB test 0.65->   from 0.3 to this version, several things have changed.
                                              //                          Found a strange behavior where due to an ADR flow, the system was trying to reboot. 
                                              //                          Agreed this should not happen. Also, due to the same, the FPGA was following the PCH, so this time we wait for the pwr-btn to be pressed before rebooting
                                              //                          This has been implemented to help validation with the emulated r-dimm flow
                                              //                          When the system is powered down due to a failure, also the system is trying to reboot (PCH de-asserts its slp signals). This was causing the memory voltage switch to
                                              //                          enable 12V_STBY. We have added the condition to signal failure when appropriate so the logic that controls the switch does not follow the SLPS if a failure happened
                                              //                          Also added instrumentation so we can force adr signals (mode, ext_trigger and complete)
                                              //version 2vB test 0.7 ->   Modified the handshake between the master and adr fubs (fsms) as there was a dead lock
                                              //                          Also pwrbtn signal was not connected to master_fub
                                              //version 2vB test 0.72->   Modified the filter for the PWRGDFAIL on the ddr5 fsm when ADR happens as RDIMMs behave differently, keeping PWRGDFAIL LOW
                                              //                          Also added an extra state in the ADR TRIG FSM to avoid seen EXT_TRIGGER as different events, to avoid looping
                                              //version 2vB test 0.8 ->   sent AdrEmulated signal from master to ADR FSM, sent FailID and AdrEmulated to s3 sw control, also sent AdrEvent from ADR fsm to master, all this in order to hold
                                              //                          the system to boot when a failure happened (sometimes PCH deasserts the slps), also when ADR flow happened (s3 sw). Finally modified the master fsm that generates the
                                              //                          slp indications to internal logic to latch the value when in ADR flow, until the adr-release happens (when pwrbtn is pressed) to avoid waking up before and mess with mem
                                              //                          This is our 2vC candidate. Also modified the ddr5 fsm to remove state ST_PSU as it seemed redundant
                                              //version 2vB test 0.9 ->   Noticed issues with code on psu_pwr_on_check & psu_sw_logic modules so, refactored and merged into a single module to make implementation easier
                                              //                          as both work related and need to be sync'ed. Issues were causing pwr dwn issues when doing the surprise pwrdwn
                                              //version 2vB test 0.A ->   Found that AdrEvent signal was not connected to ddr5_cpu1 logic, making it to misfunction (sensing a pwrgdfail error when there was not)
                                              //version 2vB test 0.B ->   We found that ADR_ACK logic was incorrectly handled as active LOW. We modified adr_fub.v to correct this issue and propagated in the main.v and master_fub.v accordingly
                                              //version 2vB test 0.C ->   We created a number of test 0.B_xx versions as we were debugging the handling of the AUX and the called Sx (or S3) switches. This 2vB_test_0C summarizes the findigs
                                              //                     ->   A change requested for modular design also added where non-legacy nodes won't trigger the adr flow themselves. They will wait for the legacy node to be the one who triggers
                                              //                          but still the surprise pwr dwn in a non-legacy node won't cause a local power down. Also added conditional compiilation for when Secondary Fpga is not present
                                              //                          We added a change in the handling of the psu_main & s3_sw logic. We go from S5 to PUS state without passing thru S3 (in pwr up). If we are in S5p (12V on to Dimms) we go to
                                              //                          the transition phase from 12V STBY to MAIN for the 12V to DIMMs (same when from S3 to S0), otherwise we don't do this transition time
                                              //                          For the power down, we go from S0down to S3hold or S5 or S5p directly (no S3->S5*) etc
                                              //version 2vC Official Release: ADR ACK is de-asserted right after ADR COMPLETE is de-asserted. When ADR flow, we only transition to ADR state in master IF AdrEmulated is configured (ADR MODE signals NEED to hold
                                              //                              their values.
                                              //version 2vC test 0.1 ->   SGPIO to BMC had assignments in wrong order (swapped msb with lsb and so on). Corrected here
                                              //version 2vC test 0.2 ->   within the BMC SGPIO transmitted from FPGA to BMC, FPGA revision numers were also backwards, change bit positions to correct this too
                                              //version 2vC test 0.3 ->   Done several modifications to this version. Power sequence changes for VPP_HBM to be the 2nd in the sequence, moved now to be the last, and after CPUPWRGD from PCH is aserted. CPUPWRGD
                                              //                          to the CPUs is delayed until VPP is ON. After VNN, we have added 15msec timer (for the pwr dwn) to ensure VCCINFAON is not down before VNN has a max of 200mV
                                              //                          On ADR, there is an issue where ADR_MODE signals are not hold by PCH on different reset FLOWS. Architecture and Design were thought for these signals to be stable. As this is not the 
                                              //                          case a workaround in desing is required to only enable ADR_MODE capture value after some event that indicates FW already set these signals. We will use postcode 0x48 from BIOS as our
                                              //                          latch event. Similarly, ADR_COMPLETE needs to be taken into account only after same 0x48 post code is submitted and ignored after SLPS are down
                                              //                          For this logic, a new module is created to avoid to much mess in the code structure
                                              //version 2vC test 0.4 ->   We had to create another image as some change requests came right after 2vC_03. First change comes from the selection for PERST source. For CPU devices, now the default must be CPUPWRGD
                                              //                          (instead of the previous PLTRST. Schematics will indicate the changes as well. For PCH devices, the only choice now is PLTRST (no option to select CPUPWRGD now).
                                              //                          The other change is that now AC will not use GNR, so, HBM3 won't be supported any longe, besides this, EMR will have same PROC_ID as GNR (and it will use HBM2), so we need to remove
                                              //                          the automatic selection for HBM3 voltage choice and tie the selection signal to HIGH so it always selects voltage for HBM2 regardless of the CPU present in the system
                                              //version 2vC test 0.5 ->   added a timer to the power down, to keep psu_ena asserted while moving AUX from MAIN to STBY (test to see if this is causing the double power cycle)
                                              //                          Also, there was no need for BMC SGPIO fix, order was fine in 2vC and I was confused
                                              //version 2vC test 0.6 ->   It happens that BMC is expecting the bytes with bit order inverted, so, a simple change in the FPGA code solves the matter. Arch diagram does not really specify which is the msb or the lsb
                                              //version 2vD Official Release: 2vC_06 has been promoted to be official release
                                              //version 2vE Official Release: ADR_ACK GPIO in the PCH is used as a STRAP to PCH, and needs to be HIGH at RSMRST_N rising edge. Hence FPGA hasto implement this functionallity as ADR_ACK inactive state is LOW, so, during
                                              //                              power up, ADR_ACK is HIGHZ (there has to be a pullup or pulldown resistor in the board, depending on how this strap is required), then after GPIOs are configured in the PCH (noted by postcode 0x48), the FPGA changes this signal to be driven LOW
                                              //                              until it is time to be asserted HIGH (when ADR_COMPLETE is received and after the 26usec timer expires)
                                              //                              Also, due to a bug in CPS, we have implemented a workaround to support CPS during direct-to-S5 flow by not overriding iMC_RST signal during ADR flow (direct-to-S5 flow triggers ADR)
                                              //                              By doing this, the DIMM will see DDR_RST signal LOW soon enough for the PMIC to internally generate an S5 message to internal logic, etc.
                                              //version 2vE Official Release: Previous 2vE candidate was dropped, as there is an issue with the CPU PWR SEQ change with CPS that is still not root-caused. This new 2vE version then discards that change in favor
                                              //                              of enabling validation teams and externa customers that require other fixes included in the previous 2vE and 2vD versions that didn't make it thru the BKC because of the issue
                                              //                              with CPS previously mentioned. In this version we include: BMC SGPIO re-order. PERST change, HBM3 support removed, DDR RST override removed, ADR latch for ADR_MODE and ADR COMPLETE
                                              //                              Change for ADR_ACK to be used as strap to PCH first and after RSMRST to be converted into ADR_ACK (going high as strap, then low as ADR_ACK init state)
                                              //version 2vE test 0.1 ->   This version addes 100msec to the timer between P3V3 and the assertion of PCHPWROK (to workaround the exhesive delay on the DB2000 clk buffer)
                                              //                          It also implements a workaround for the FM_BMC_CRASHLOG_TRIG_N signal that starts asserted LOW. Implemented a D-type FlipFlop with enable to keep it high until FM_BMC_ONCTL_N_PLD is asserted LOW
                                              //version 2vE test 0.2 ->   Timer between P3V3 pwrgd and PCHPWROK assertions has been set to 400msec, to ensure output from DB200 clock to all PCIE devices is ensured to be toggling at least 100msec before CPUPWRGD assertion
                                              //                          Also a workaround for the CRASHLOG logic has been implemented. BMC CRASHLOG signal is passed thru a D-type flip-flop with enable, to avoid propagating false assertions at power up to 
                                              //                          CRASHLOG internal logic. This becomes the first 2vF candidate
                                              //version 2vE test 0.3 ->   Adding a change in the PSU_S3_CTRL code, to avoid fsm jump to the ERROR state (removed) when PSU or any VR handled here fails. It only will notify the master FSM for an eventual power down instead
                                              //                          Also, added a 20usec timer between CPUPWRGD and SLPS5 attention (this in the master fub)
                                              //version 2vE test 0.4 ->   This one implements a change in the CRASHLOG logic. Instead of the requested AND, etc, we are using a mux, muxing between GBLRST_WARN and BMC_CRASHLOG using CATERR, as requested by C.Vallin
                                              //                          Also, the latching event for ADR signals (ADR_MODE[1:0] and ADR_COMPLETE) is changed to be postcode 0xB0, instead of 0x48, as it seems the latter duration is shorter and it is sometimes missed
                                              //                          by SGPIO, etc, causing issues at power down.
                                              //version 2vE test 0.5 ->   This one implements a change in the CRASHLOG logic. Again, we are filtering CATERR_N to be HIGH before VNN_CPU0 PWRGD is asserted, to avoid false assertions
                                              //version 2vE test 0.6 ->   This includes recipes for different CPS issues: Dirty Clock Issue, Surprise-Clock-Stop and the 600usec timer between DIMM_RST assertion and DRAMPWROK / CPUPWRGD de-assertion at power-down
                                              //                          It also finally includes the power sequence change for CPU (including the change for HBM packages). This is our most solid 2vF candidate
                                              //version 2vE test 0.7 ->   Integration for 0.6 had an issue as missed to connect the TimerDone output for PsuOnTimer from master fub to cpu_seq. Fixed in this version
                                              //version 2vF Official Release: Grabbed 2vE test07, corrected ddr5_pwrgd_logic.v (added output reg to oContinuePwrDwn output) and changed version
                                              //version 2vF test 0.1 ->   when going from G3 to S5, we don't power DIMMs with 12V from STBY even if jumper (j83) is populated (only when coming from S0 -> S5
                                              //version 2vF test 0.2 ->   Also changed the timer duration to transition from MAIN to STBY and STBY to MAIN for 12V to the DIMMs (from 200 to 400msec)
                                              //                          Also, when shutting down due to AC removal, we don't provide 12v from STBY to DIMMs, we directly shutdown
                                              //version 3v0 Official Release: This is a candidate to BKC, to be 3v0 after working more with CPS team to provide a recipe for other issues described below
                                              //                              First, we inserted issues that broke the S3 flow: When going to S3, ac_cpu_seq was not receiving the ContinuePwrDwn signal from ddr5 fsm, because the timer was not launched (ddr5 fsm does
                                              //                              not move when going to S3). Fixed this by making 2 things: 1st, ac_cpu_seq is not checking for ContinuePwrDwn signal when going to S3. 2nd, ContinuePwrDwn is always asserted in ddr5 fsm,
                                              //                              until it is at LINK state, where signal is de-asserted, to be re-asserted only after receiving S5 indication or AdrComplete and FSM transitions to PWRDWN state, launches the timer, etc.
                                              //                              Also discovered that when modifying the cpu sequence in the ac_cpu_seq, the INIT state added clearing the state for all the related VR enables, so, when reaching from S3, it was not correct
                                              //                              as VCCD_HV needs to remain ON. Fixed this too
                                              //                              Now, for CPS, we changed the behavior for jumper J83 so when going from G3 to S5, DIMMs are still not powered up from STBY (even if J83 is present). Only when coming from S0 to S5
                                              //                              In this latter case, the transition from Main to Stby has been modified from 200msec to 400msec.
                                              //                              Also, if J83 is populated but surprise AC removal happens, DIMMs go directly from MAIN to OFF (no transition to STBY)
                                              //                              Finally, we encountered issues related to metastability with PWRGDFAIL signals when reading in the ddr5 FSM, because there was no synchronizer, nor a filter, etc
                                              //                              We added a 3-tab synch with a comparision between 1st and 2nd tabs, if they are equal, 3rd tab captures from 2nd tab, and this is enabled by the VCCD PWRGD signal to make sure the IO bank
                                              //                              is on (so PWRGDFAIL readings are accurate)
                                              //version 3v1 Official Release: After releasing 3v0, we added a 650msec timer to the CPUPWRGD (only for the version of the signal that goes into the PERST_N signal) in order to increase the distance from when voltage is 
                                              //                              provided to the PCIE card, to when the rst is de-asserted (when CPUPWRGD is the source for PERST_N)
                                              //                              We also discovered a bug, where the indication signal for sleep S5 state was not connected to ac_cpu_seq instance of CPU1. This was causing issues in the PFR flow for PCH FW update, and we 
                                              //                              are suspecting other flows too. This is fix is required to unblock the PCH FW update
                                              //version 3v1 test 0.1 ->  For this test version I am including a 25msec timer when going from S0 to S5 (not caused by a surprise pwr-dwn) and J83 is populated, before transitioning from 12V_MAIN to 12V_STBY in the 12V 
                                              //                         rail to the DIMMs, and fixes made by when J83 is populated and going from G3 to S0 (to not power DIMMs with 12V_STBY) to clean-up that flow
                                              //version 3v1 test 0.2 ->  EXTERNAL TRIGGER input should also cause IRQ_SML1_PMBUS_PLD_ALERT_N to be asserted LOW until ADR_ACK occurs (only when ADR is enabled). This is something it was not implemented before. So implementing
                                              //                         in this version to be added in a future 3v2 release. For this test version I am including also the 25msec timer when going from S0 to S5 (not caused by a surprise pwr-dwn) and J83 is populated, before
                                              //                         transitioning from 12V_MAIN to 12V_STBY in the 12V rail to the DIMMs, and the fixes made by when J83 is populated and going from G3 to S0 (to not power DIMMs with 12V_STBY)
                                              //version 3v1 test 0.3 ->  This test version includes changes for tests 0.1 & 0.2 plus adds extra logic to make sure CPUPWRGD to both sockets (when present) assert at the same time, as a test for the issue with the TSC (Time-Stamp Counter)
                                              //                         Assertion for CPUPWRGD before this version does not happen necessarily at the same time as each sequencer controls each on CPUPWRGD to the respective skt, and it depends on when the VRs assert the PWRGD signal, etc
                                              //                         Although this difference should not affect the TSC, theory is that this may be impacting and we will try with this version
                                              //version 3v1 test 0.4 ->  Encountered a toggling for CPUPWRGD during pwrdwn, so added FSM conditions to avoid this toggling in the CPU_DONE state
                                              //                         Also found that some glue logic is now having issues with 2 skts for the VCCINFAON enable as due to old logic to support interposer was muxing between CPU0 Ena and CPU1 ena signals
                                              //                         removed that mux and connect directly CPU1 ena to output pin for VCCINFAON VR
                                              //version 3v1 test 0.5 ->  looks like when doing fast/legacy ADR, as we changed the behavior to avoid waiting for the PowerButton, etc, we forgot to de-assert the AdrRelease. This version fixes this
                                              //version 3v2 Official Release: promoted 3v1 test 0.5 to official release including all changes for 0.1, 0.2, 0.3, 0.4 and 0.5
                                              //version 3v2 test 0.1 ->  Adds logic to filter out misuse of jumper j5c11 (external ADR Trigger injection)
                                              //version 3v2 test 0.2 ->  added conditions to psu_s3_ctrl logic to shutdown even if slp signals are not asserted
                                              //                         this is because when a CPU VR fails before the sequence has finished, the fpga will try to shutdown, will go into power down but PCH has no way to be the one shutting the
                                              //                         platform down because it did not received the PCH_PWR_OK & SYS_PWR_OK in the first place, so, SLPs won't be asserted
                                              //version 3v2 test 0.3 ->  logic worked apparently, but also seems that once a CPU VR fails, other fail too, so adding logic to latch the first one that failed to nail the issue faster
                                              //version 3v2 test 0.4 ->  Added some singals to RFAT for BMC, PCH, PSU and CPU instances. Need rfat.tcl update too
                                              //version 3v3 Official Release: After a few tests and corrections and because we haven't released to the BKC, we produce a new 3v3 Official Release
                                              //                              3v2 tests 0.1, 0.2, 0.3 and 0.4 were taken to build the 3v3
                                              //                              We have added logic to better flag VR issues to the board display (fpga Post-Codes). 
                                              //                              Adds logic to filter out misuse of jumper j5c11 (external ADR Trigger injection)
                                              //                              Also, we have added a few more signals to the RFAT instrumentation. This implies script change
                                              //version 3v3 test 0.1 ->  A test version requested by CPS team (Kai/Girish) and Juan Orozco (Board Arch). To remove the 25msec from the flow for power down after asserting DIMM_RST and before
                                              //                         de-asserting DRAMPWRGD and CPUPWRGD. Also, removing the 600 usec timer (on same situation but when pwrdwn is because of ADR GLBL RST event) --> Basically, will use ADR_COMPLETE
                                              //                         as differentiator to use or not the 600usec (if SLP_S4, will use them, if ADR_COMPLETE, we will not)
                                              //version 3v3 test 0.2 ->  We support moving from S3 to S5 by adding a condition in INIT state of cpu sequencer, to de-assert PVCCD_HV enable there only if SLP_S5 indication is asserted while there, to avoid
                                              //                         FPGA to see it as a VR failure by loosing PWRGD while keeping ENABLE high (due to PSU will pwrdwn if moving to S5)
                                              //version 3v4 Official Release: After discussing with board Architect and CPS team, it has been decided to remove the 25msec timer between DIMM_RST assertion and DRAMPWRGD de-assertion in a non AC-removal PWRDWN
                                              //                              so, we now wait 600usec always during PWRDWN.
                                              //                              We allow transition from S3 to S5 by disabling VCCD_HV VR when requested to do so (to avoid VR failure is registerd by PLD)
                                              //                              We removed the 26usec timer from when ADR_COMPLETE is asserted to when PLD responds with ADR_ACK (no longer needed)
                                              //                              When PMIC fails, ddr5 FSM asserts DIMM_RST, then waits for 600usec, then de-asserts DRAMPWRGD and goes into FAULT state
                                              //                              If MC VR fails, we go directly to FAULT state, but we do not assert DIMM_FAILURE signal so we don't flag a DIMM failure incorrectly
                                              //                              When on FAULT state, we assert ContinuePwrDwn to avoid CPU seq to get stalled at PWRDWN state
                                              //                              There is a use case, where a GLBL rst is issued before CPUPWRGD is asserted by PCH, which was holding CPU SEQ FSM at PCHPWROK STATE, causing issues at power down
                                              //                                To fix this, we added a condition to move to PWR_DWN in PCHPWROK ST, we also assert CPU_PWR_DONE at PWR_DWN. Then in master fub, we added a timer launched by CpuMemEna going log
                                              //                                So, when master fub checks if needs to go to PSU state at pwrdwn, it won't immediately if CPU_PWR_DONE is low but waits until the timer expires. That gives time to CPU SEQ to move
                                              //                                from PCHPWROK ST to PWR_DWN and assert CPU_PWR_DONE, so master seq won't transtion to PSU state then, until it really happens that CPU SEQ is done with its pwr down
                                              //                                If for some reason the PWR SEQ is not finished by CPU SEQ (i.e. a VR fails), the master won't get stuck, as after timer expires, it will move to PSU state to pwrdown, etc.
                                              //                              Also there is a CCB accepted for the crashlog trigger logic, where we will hold the ADR_ACK assertion until 10msec timer expires launched when the GBL_RST_WARN_N signal is asserted
                                              //                              during a power down when it is not due to a surprise AC removal
                                              //version 3v5 Official Release: We have held more discussions with Arch team (system, platform, board, and CPS) and we need quite a change in the power down to be in compliant with PDG
                                              //                              When ADR, ADR_COMPLETE will make DRAM_RST to be dropped, then, we will wait for SLPS4 to be dropped by PCH and it will cause CPU_RST to go LOW, then CPUPWRGD, then DRAMPWROK.
                                              //                              When S0 to S5 (normal or strait), SLPS4 will make DRAM_RST to go LOW, then CPU_RST, then CPUPWRGD, then DRAMPWROK. We are removing the 600usec timer from the DDR5 FSM to wait between
                                              //                              asserting DRAM_RST and deasserting DRAMPWROK, etc.
                                              //                              Also, these changes implies other changes, like CPU_RST follows PLTRST at pwrup, but not at pwrdwn. We need to keep a 1 usec max delay between DRAM_RST assertion to DRAMPWROK, which
                                              //                              has some challenges as we are using a 500 nsec clock period, and we have 3 states in between (1.5 usec). We may need to add combinational logic to give the sepparation between signals
                                              //                              as we don't have other timing requirements for the other signals (other than they need to be in sequence). Will document how this ended up being implemented.
                                              //version 3v7 Official Release: first thing, 3v6 Official Release was done in a different branch to support wave1 customers.
                                              //                              We found a corner case for SLP_S3
                                              //                               When we send the system to SLP_S3 from OS, it takes some time between Display is OFF to when really the system Powers Down to S3 state. If a wake event happens during this lapse, 
                                              //                               it may be possible that the system is sent SLPS3 by PCH, but then quickly enough, the PCH tries to wake up back again. That could get the FPGA in the middle of powering down the CPU VRs, 
                                              //                               if that happens, the SLPS3 indication is cleared, before PVCCD VR is checked to be powered off or not, causing for it to be disabled, which will cause SLPS3 to fail, moreover, as the PVCCD 
                                              //                               goes off, the DDR5, which is only checking for SLPS5 indication, sees the VR going down when not expected causing the ddr5 fms to see this as an issue, sending the PLD to an error state where 
                                              //                               it cannot recover. To tackle this, we changed what condition we check when it is the turn to disable the PVCCD_HV VR during a power down. We now check for SLPS5 condition, instead of checking 
                                              //                               for S3 condition if we are in S5, we turn PVCCD_HV off, else, we keep it enabled (ON). This was not the only change. In the master sequence, similar situation could happen, 
                                              //                               so we added a condition in PLTRST_ST, so, if we are not in pwrdn and still PLTRST is not de-asserted, we enable CPU FSM to turn back ON
                                              //                              We had to modify certain flows, to be in compliance with what is published in PDG
                                              //                               PDG said CPUPWRGD of the CPU involved in VR failure would de-assert first, while the other CPU PWRGD signal will keep asserted until shutdown is commanded by PCH
                                              //                               In this version we changed this to be in compliance with PDG
                                              //                               we also noted that when PVCCD VR fails, it was considered as a CPU VR failure, while PDG considers this as MEM PWR FAILURE, which follows a different process. 
                                              //                               We have change the qualification in this image, so it is now considered and treated as a MEM PWR FAILURE then
                                              //                              Also, in modular systems we found an architecture open during ADR Flow: When in a multi-node system, one of the non-legacy nodes power-supplys fails, the non-legacy main fpga was registering the 
                                              //                               failure, causing PCHPWROK and SYSPWROK to be dropped right away, preventing the ADR flow to accomplish the full process. This is due to non-legacy main fpga is not receiving the ADR_MODE signals 
                                              //                               properly set, nor the bios post-code event (used as workaround for the PCH GPIOs losing config during Global Rst), to latch the pin values into internal signals (NL main fpga does not receive 
                                              //                               the post-codes from its local BMC). To fix this, we have added a condition to when the Main FPGA should ignore a PSU failure (which should happen when ADR is enabled). We qualify with the LEGACY 
                                              //                               information already available on Main FPGA, so a NL Main Fpga will always ignore the PSU failure, so the Legacy Main Fpga will see it and launch the ADR trigger In RP or 2s config, Main Fpga is 
                                              //                               always qualified as LEGACY, hence, it still needs to look on when ADR flow is enabled or not, to ignore, or not, the PSU failure when happens
                                              //version 3v8 Official Release:  Did a modification in the MiscLogic module, signal H_CPU_NMI_LVC1 was driven by NMI signal coming from BMC as well as PCH, it was requested to remove the condition from the PCH signal, logic is now
                                              //                               driven by NMI signal coming from BMC
   
 mArcher_City_Main
(
	.iClk_2M                            (iClk_2M),
	.iClk_50M                           (iClk_50M),
	.iRST_N                             (ipll_locked),


    .iFM_FORCE_PWRON                    (FM_FORCE_PWRON_LVC3),

    //LED CONTROL signals
    .oLED_CONTROL                       (LED_CONTROL),
    .oFM_POSTLED_SEL                    (FM_POSTLED_SEL),
    .oPostCodeSel1_n                    (FM_POST_7SEG1_SEL_N),               //to select Display1 (MSNibble)
    .oPostCodeSel2_n                    (FM_POST_7SEG2_SEL_N),               //to select Display2 (LSNibble)
    .oLedFanFaultSel_n                  (FM_FAN_FAULT_LED_SEL_R_N),          //to select Fan Fault LEDs
    .oFmCpu0DimmCh14FaultLedSel         (FM_CPU0_DIMM_CH1_4_FAULT_LED_SEL),  //to select CPU0 DIMMs 1&2 CH 1-4 fault LEDs
    .oFmCpu0DimmCh58FaultLedSel         (FM_CPU0_DIMM_CH5_8_FAULT_LED_SEL),  //to select CPU0 DIMMs 1&2 CH 5-8 fault LEDs
    .oFmCpu1DimmCh14FaultLedSel         (FM_CPU1_DIMM_CH1_4_FAULT_LED_SEL),  //to select CPU1 DIMMs 1&2 CH 1-4 fault LEDs
    .oFmCpu1DimmCh58FaultLedSel         (FM_CPU1_DIMM_CH5_8_FAULT_LED_SEL),  //to select CPU1 DIMMs 1&2 CH 5-8 fault LEDs
    
    //GSX Interface with BMC
    .SGPIO_BMC_CLK                      (SGPIO_BMC_CLK), 
    .SGPIO_BMC_DOUT                     (SGPIO_BMC_DOUT),
    .SGPIO_BMC_DIN                      (SGPIO_BMC_DIN_R),
    .SGPIO_BMC_LD_N                     (SGPIO_BMC_LD_N),
    
    //Termtrip dly
    .H_CPU0_THERMTRIP_LVC1_N			(H_CPU0_THERMTRIP_LVC1_N),
    .H_CPU1_THERMTRIP_LVC1_N			(H_CPU1_THERMTRIP_LVC1_N),
    .FM_THERMTRIP_DLY_LVC1_N			(FM_THERMTRIP_DLY_LVC1_R_N),
    
    //MEMTRIP
    .H_CPU0_MEMTRIP_LVC1_N				(H_CPU0_MEMTRIP_LVC1_N),
    .H_CPU1_MEMTRIP_LVC1_N				(H_CPU1_MEMTRIP_LVC1_N),
    
    // PROCHOT
    .IRQ_CPU0_VRHOT_N 					(IRQ_CPU0_VRHOT_N),
	.IRQ_CPU1_VRHOT_N 					(IRQ_CPU1_VRHOT_N),
	.H_CPU0_PROCHOT_LVC1_N 			    (H_CPU0_PROCHOT_LVC1_R_N),
	.H_CPU1_PROCHOT_LVC1_N 			    (H_CPU1_PROCHOT_LVC1_R_N),
    
	// PERST & RST
	.FM_RST_PERST_BIT0                  (FM_RST_PERST_BIT0), 
	.FM_RST_PERST_BIT1                  (FM_RST_PERST_BIT1), 
	.FM_RST_PERST_BIT2                  (FM_RST_PERST_BIT2), 

	.RST_PCIE_PERST0_N                  (RST_PLD_PCIE_CPU0_DEV_PERST_N),
	.RST_PCIE_PERST1_N                  (RST_PLD_PCIE_CPU1_DEV_PERST_N),
	.RST_PCIE_PERST2_N                  (RST_PLD_PCIE_PCH_DEV_PERST_N),

	.RST_CPU0_LVC1_N                    (RST_CPU0_LVC1_R_N),
	.RST_CPU1_LVC1_N                    (RST_CPU1_LVC1_R_N),

	.RST_PLTRST_B_N                     (RST_PLTRST_PLD_B_N),
	.RST_PLTRST_N                       (RST_PLTRST_PLD_N),
	
//CPU Misc
	.FM_CPU0_PKGID0                     (FM_CPU0_PKGID0), 
	.FM_CPU0_PKGID1                     (FM_CPU0_PKGID1), 
	.FM_CPU0_PKGID2                     (FM_CPU0_PKGID2), 
	.FM_CPU0_PROC_ID0                   (FM_CPU0_PROC_ID0), 
	.FM_CPU0_PROC_ID1                   (FM_CPU0_PROC_ID1), 
	.FM_CPU0_SKTOCC_LVT3_N              (FM_CPU0_SKTOCC_LVT3_PLD_N), 
	.FM_CPU1_PKGID0                     (FM_CPU1_PKGID0), 
	.FM_CPU1_PKGID1                     (FM_CPU1_PKGID1), 
	.FM_CPU1_PKGID2                     (FM_CPU1_PKGID2), 
	.FM_CPU1_PROC_ID0                   (FM_CPU1_PROC_ID0), 
	.FM_CPU1_PROC_ID1                   (FM_CPU1_PROC_ID1), 
	.FM_CPU1_SKTOCC_LVT3_N              (FM_CPU1_SKTOCC_LVT3_PLD_N),

	.FM_CPU0_INTR_PRSNT					(FM_CPU0_INTR_PRSNT),

//BMC
   //.FM_BMC_INTR_PRSNT	                (FM_BMC_INTR_PRSNT),
	.FM_BMC_PWRBTN_OUT_N                (FM_BMC_PWRBTN_OUT_N),
	.FP_BMC_PWR_BTN_N                   (FP_BMC_PWR_BTN_R2_N),                 //From FP-> LOT 6 MUX
	.FM_BMC_PFR_PWRBTN_OUT_N            (FM_BMC_PFR_PWRBTN_OUT_R_N),           //To LOT6 MUX -> PCH
	.FM_BMC_ONCTL_N                     (FM_BMC_ONCTL_N_PLD),
	.RST_SRST_BMC_PLD_N_REQ             (RST_SRST_BMC_PLD_R_N_REQ),
    .RST_SRST_BMC_PLD_N                 (RST_SRST_BMC_PLD_R_N),


	.FM_P2V5_BMC_AUX_EN					(FM_P2V5_BMC_AUX_EN),
	.PWRGD_P2V5_BMC_AUX					(PWRGD_P2V5_BMC_AUX),
	.FM_P1V2_BMC_AUX_EN					(FM_P1V2_BMC_AUX_EN),
	.PWRGD_P1V2_BMC_AUX					(PWRGD_P1V2_BMC_AUX),
	.FM_P1V0_BMC_AUX_EN					(FM_P1V0_BMC_AUX_EN),
	.PWRGD_P1V0_BMC_AUX					(PWRGD_P1V0_BMC_AUX), 

//PCH
	.RST_RSMRST_N_REQ                   (RST_RSMRST_PLD_R_N_REQ),
    .RST_RSMRST_N                       (RST_RSMRST_PLD_R_N),

	.PWRGD_PCH_PWROK                    (PWRGD_PCH_PWROK_R), 
	.PWRGD_SYS_PWROK                    (PWRGD_SYS_PWROK_R),

	.FM_SLP_SUS_RSM_RST_N               (FM_SLP_SUS_RSM_RST_N), 
	.FM_SLPS3_N                         (FM_SLPS3_PLD_N), 
	.FM_SLPS4_N                         (FM_SLPS4_PLD_N), 
	.iFmPchPrsnt_n                      (FM_PCH_PRSNT_N),
    .iFmEvbPrsnt_n                      (FM_VAL_BOARD_PRSNT_N),

	.FM_PCH_P1V8_AUX_EN                 (FM_PCH_P1V8_AUX_EN),
	.FM_P1V05_PCH_AUX_EN				(FM_P1V05_PCH_AUX_EN),
	.FM_PVNN_PCH_AUX_EN					(FM_PVNN_PCH_AUX_EN),
	.PWRGD_P1V8_PCH_AUX                 (PWRGD_P1V8_PCH_AUX_PLD),
	.PWRGD_P1V05_PCH_AUX                (PWRGD_P1V05_PCH_AUX),
	.PWRGD_PVNN_PCH_AUX					(PWRGD_PVNN_PCH_AUX), 
	
//Dediprog Detection Support
	.RST_DEDI_BUSY_PLD_N                (RST_DEDI_BUSY_PLD_N),

//PSU Ctl
	.FM_PS_EN                           (FM_PS_EN_PLD_R), 
	.PWRGD_PS_PWROK                     (PWRGD_PS_PWROK_PLD_R), 

//Clock Logic    
	.FM_PLD_CLKS_DEV_EN                 (FM_PLD_CLKS_DEV_R_EN),
	.FM_PLD_CLK_25M_EN                  (FM_PLD_CLK_25M_R_EN),

//Base Logic
	.PWRGD_P3V3_AUX                     (PWRGD_P1V2_MAX10_AUX_PLD_R),   //originally it was PWRGD_P3V3_AUX, we are now using PWRGD_P1V2_MAX10 from the board

//Main VR & Logic
	.PWRGD_P3V3                         (PWRGD_P3V3), 
	.FM_P5V_EN                          (FM_P5V_EN),
	.FM_AUX_SW_EN                       (FM_AUX_SW_EN), 

//Mem



//CPU VRs
    .FM_HBM2_HBM3_VPP_SEL               (FM_HBM2_HBM3_VPP_SEL),                 //alarios
 
	.PWRGD_CPU0_LVC1 					(PWRGD_CPU0_LVC1_R),
	.PWRGD_CPU1_LVC1 					(PWRGD_CPU1_LVC1_R),

	.PWRGD_CPUPWRGD_LVC1 				(PWRGD_CPUPWRGD_LVC1),
	.PWRGD_PLT_AUX_CPU0_LVT3 			(PWRGD_PLT_AUX_CPU0_LVT3_R),
	.PWRGD_PLT_AUX_CPU1_LVT3  			(PWRGD_PLT_AUX_CPU1_LVT3_R),
		
	.FM_PVCCIN_CPU0_EN 					(FM_PVCCIN_CPU0_R_EN),
	.PWRGD_PVCCIN_CPU0 					(PWRGD_PVCCIN_CPU0),

	.FM_PVCCFA_EHV_FIVRA_CPU0_EN 		(FM_PVCCFA_EHV_FIVRA_CPU0_R_EN),
	.PWRGD_PVCCFA_EHV_FIVRA_CPU0 		(PWRGD_PVCCFA_EHV_FIVRA_CPU0),

	.FM_PVCCINFAON_CPU0_EN 				(FM_PVCCINFAON_CPU0_R_EN),
	.PWRGD_PVCCINFAON_CPU0 				(PWRGD_PVCCINFAON_CPU0),

	.FM_PVCCFA_EHV_CPU0_EN 				(FM_PVCCFA_EHV_CPU0_R_EN),
	.PWRGD_PVCCFA_EHV_CPU0 				(PWRGD_PVCCFA_EHV_CPU0),

	.FM_PVCCD_HV_CPU0_EN 				(FM_PVCCD_HV_CPU0_EN),
	.PWRGD_PVCCD_HV_CPU0 				(PWRGD_PVCCD_HV_CPU0),

	.FM_PVNN_MAIN_CPU0_EN 				(FM_PVNN_MAIN_CPU0_EN),
	.PWRGD_PVNN_MAIN_CPU0 				(PWRGD_PVNN_MAIN_CPU0),

	.FM_PVPP_HBM_CPU0_EN 				(FM_PVPP_HBM_CPU0_EN),
	.PWRGD_PVPP_HBM_CPU0 				(PWRGD_PVPP_HBM_CPU0),

	.FM_PVCCIN_CPU1_EN 					(FM_PVCCIN_CPU1_R_EN),
	.PWRGD_PVCCIN_CPU1 					(PWRGD_PVCCIN_CPU1),

	.FM_PVCCFA_EHV_FIVRA_CPU1_EN 		(FM_PVCCFA_EHV_FIVRA_CPU1_R_EN),
	.PWRGD_PVCCFA_EHV_FIVRA_CPU1 		(PWRGD_PVCCFA_EHV_FIVRA_CPU1),

	.FM_PVCCINFAON_CPU1_EN 				(FM_PVCCINFAON_CPU1_R_EN),
	.PWRGD_PVCCINFAON_CPU1 				(PWRGD_PVCCINFAON_CPU1),

	.FM_PVCCFA_EHV_CPU1_EN 				(FM_PVCCFA_EHV_CPU1_R_EN),
	.PWRGD_PVCCFA_EHV_CPU1 				(PWRGD_PVCCFA_EHV_CPU1),

	.FM_PVCCD_HV_CPU1_EN     			(FM_PVCCD_HV_CPU1_EN),
	.PWRGD_PVCCD_HV_CPU1 				(PWRGD_PVCCD_HV_CPU1),

	.FM_PVNN_MAIN_CPU1_EN 				(FM_PVNN_MAIN_CPU1_EN),
	.PWRGD_PVNN_MAIN_CPU1 				(PWRGD_PVNN_MAIN_CPU1),

	.FM_PVPP_HBM_CPU1_EN 				(FM_PVPP_HBM_CPU1_EN),
	.PWRGD_PVPP_HBM_CPU1 				(PWRGD_PVPP_HBM_CPU1),
	//.IRQ_PSYS_CRIT_N 					(IRQ_PSYS_CRIT_N),   //alarios: 13-ago-20 (removed per Arch request, no longer POR)

	//Debug Signals
	.FM_PLD_HEARTBEAT_LVC3 				(FM_PLD_HEARTBEAT_LVC3), //Heart beat led
	.FM_PLD_REV_N                		(FM_PLD_REV_N),

    //DEBUG SGPIO (Debug to Main FPGAs)
 	.iSGPIO_DEBUG_CLK(SGPIO_DEBUG_PLD_CLK),
	.iSGPIO_DEBUG_DOUT(SGPIO_DEBUG_PLD_DOUT),
    .oSGPIO_DEBUG_DIN(SGPIO_DEBUG_PLD_DIN),
	.iSGPIO_DEBUG_LD(SGPIO_DEBUG_PLD_LD),

	//Reset from CPU to VT to CPLD
	.M_AB_CPU0_RESET_N(M_AB_CPU0_RESET_N),
	.M_CD_CPU0_RESET_N(M_CD_CPU0_RESET_N),
	.M_EF_CPU0_RESET_N(M_EF_CPU0_RESET_N),
	.M_GH_CPU0_RESET_N(M_GH_CPU0_RESET_N),
	.M_AB_CPU1_RESET_N(M_AB_CPU1_RESET_N),
	.M_CD_CPU1_RESET_N(M_CD_CPU1_RESET_N),
	.M_EF_CPU1_RESET_N(M_EF_CPU1_RESET_N),
	.M_GH_CPU1_RESET_N(M_GH_CPU1_RESET_N),

	//Reset from CPLD to VT to DIMM
	.M_AB_CPU0_FPGA_RESET_N(M_AB_CPU0_FPGA_RESET_R_N),  //alarios dbg
	.M_CD_CPU0_FPGA_RESET_N(M_CD_CPU0_FPGA_RESET_R_N),  //alarios dbg
	.M_EF_CPU0_FPGA_RESET_N(M_EF_CPU0_FPGA_RESET_R_N),  //alarios dbg
	.M_GH_CPU0_FPGA_RESET_N(M_GH_CPU0_FPGA_RESET_R_N),  //alarios dbg
	.M_AB_CPU1_FPGA_RESET_N(M_AB_CPU1_FPGA_RESET_R_N),
	.M_CD_CPU1_FPGA_RESET_N(M_CD_CPU1_FPGA_RESET_R_N),
	.M_EF_CPU1_FPGA_RESET_N(M_EF_CPU1_FPGA_RESET_R_N),
	.M_GH_CPU1_FPGA_RESET_N(M_GH_CPU1_FPGA_RESET_R_N),

	.IRQ_CPU0_MEM_VRHOT_N(IRQ_CPU0_MEM_VRHOT_N),
	.IRQ_CPU1_MEM_VRHOT_N(IRQ_CPU1_MEM_VRHOT_N),

	.H_CPU0_MEMHOT_IN_LVC1_N(H_CPU0_MEMHOT_IN_LVC1_R_N),
	.H_CPU1_MEMHOT_IN_LVC1_N(H_CPU1_MEMHOT_IN_LVC1_R_N),

	.H_CPU0_MEMHOT_OUT_LVC1_N(H_CPU0_MEMHOT_OUT_LVC1_N),
	.H_CPU1_MEMHOT_OUT_LVC1_N(H_CPU1_MEMHOT_OUT_LVC1_N),

	.PWRGD_FAIL_CPU0_AB(PWRGD_FAIL_CPU0_AB_PLD),
	.PWRGD_FAIL_CPU0_CD(PWRGD_FAIL_CPU0_CD_PLD),
	.PWRGD_FAIL_CPU0_EF(PWRGD_FAIL_CPU0_EF_PLD),
	.PWRGD_FAIL_CPU0_GH(PWRGD_FAIL_CPU0_GH_PLD),
	.PWRGD_FAIL_CPU1_AB(PWRGD_FAIL_CPU1_AB_PLD),
	.PWRGD_FAIL_CPU1_CD(PWRGD_FAIL_CPU1_CD_PLD),
	.PWRGD_FAIL_CPU1_EF(PWRGD_FAIL_CPU1_EF_PLD),
	.PWRGD_FAIL_CPU1_GH(PWRGD_FAIL_CPU1_GH_PLD),

	.PWRGD_DRAMPWRGD_CPU0_AB_LVC1(PWRGD_DRAMPWRGD_CPU0_AB_R_LVC1),
	.PWRGD_DRAMPWRGD_CPU0_CD_LVC1(PWRGD_DRAMPWRGD_CPU0_CD_R_LVC1),
	.PWRGD_DRAMPWRGD_CPU0_EF_LVC1(PWRGD_DRAMPWRGD_CPU0_EF_R_LVC1),
	.PWRGD_DRAMPWRGD_CPU0_GH_LVC1(PWRGD_DRAMPWRGD_CPU0_GH_R_LVC1),
	.PWRGD_DRAMPWRGD_CPU1_AB_LVC1(PWRGD_DRAMPWRGD_CPU1_AB_R_LVC1),
	.PWRGD_DRAMPWRGD_CPU1_CD_LVC1(PWRGD_DRAMPWRGD_CPU1_CD_R_LVC1),
	.PWRGD_DRAMPWRGD_CPU1_EF_LVC1(PWRGD_DRAMPWRGD_CPU1_EF_R_LVC1),
	.PWRGD_DRAMPWRGD_CPU1_GH_LVC1(PWRGD_DRAMPWRGD_CPU1_GH_R_LVC1),


    //Misc Logic for CPU errors, DMI & PCH Crash-log 
    //.oFM_CPU_CATERR_DLY_N(FM_CPU_CATERR_DLY_N),     //alarios 20-ago-20 : removed per Arch request
    .iCPU_CATERR_N(H_CPU_CATERR_LVC1_R_N),
    //.iCPU_RMCA_N(H_CPU_RMCA_LVC1_R_N), //removed per Arch request (alarios 13-ago-20), no need for PCH or BMC to monitor this
    .oFM_CPU_CATERR_N(FM_CPU_CATERR_LVT3_R_N),
    .oCPU_NMI_N(H_CPU_NMI_LVC1_R),                           //it is active HIGH so name has been corrected in board
    .oIrqBmcPchNmi(IRQ_BMC_PCH_NMI),                         //alarios 13-ago-20 added per Arch request
    .iIRQ_BMC_CPU_NMI(IRQ_BMC_CPU_NMI),
    .iIRQ_PCH_CPU_NMI_EVENT_N(IRQ_PCH_CPU_NMI_EVENT_N),
    .iBmcNmiPchEna(FM_BMC_NMI_PCH_EN),                     //alarios (13-ago-20) added per Arch Request (to select if BMC NMI goes to CPUs or PCH)
    .iFM_BMC_CRASHLOG_TRIG_N(FM_BMC_CRASHLOG_TRIG_N),
    .oFM_PCH_CRASHLOG_TRIG_N(FM_PCH_CRASHLOG_TRIG_R_N),
    .iFM_PCH_PLD_GLB_RST_WARN_N(FM_PCH_PLD_GLB_RST_WARN_N),

    //Smart Logic
    .iFM_PMBUS_ALERT_B_EN(FM_PMBUS_ALERT_B_EN),
	.iFM_THROTTLE_R_N(FM_THROTTLE_R_N),
	.ioIRQ_SML1_PMBUS_PLD_ALERT_N(IRQ_SML1_PMBUS_PLD_ALERT_N),
	.oFM_SYS_THROTTLE_N(FM_SYS_THROTTLE_R_N),

	//Errors
	.H_CPU_ERR0_LVC1_N(H_CPU_ERR0_LVC1_R_N),
	.H_CPU_ERR1_LVC1_N(H_CPU_ERR1_LVC1_R_N),
	.H_CPU_ERR2_LVC1_N(H_CPU_ERR2_LVC1_R_N),
	.FM_CPU_ERR0_LVT3_N(FM_CPU_ERR0_LVT3_N),
	.FM_CPU_ERR1_LVT3_N(FM_CPU_ERR1_LVT3_N),
	.FM_CPU_ERR2_LVT3_N(FM_CPU_ERR2_LVT3_N),
	.H_CPU0_MON_FAIL_PLD_LVC1_N(H_CPU0_MON_FAIL_PLD_LVC1_N),
	.H_CPU1_MON_FAIL_PLD_LVC1_N(H_CPU1_MON_FAIL_PLD_LVC1_N),

 //Interposer VR powerGd signal (VTT) This only for CLX & needs to be modified when not in CLX
 //   .iPWRGD_VTT_PWRGD(FM_IBL_SLPS0_CPU0_N),
    .ioPwrGdS0PwrOkCpu0(PWRGD_S0_PWROK_CPU0_LVC1_R),                     //alarios: connection in the board changed, for CLX it is used as the VTT-PWRGD
    .oPwrGdS0PwrOkCpu1(PWRGD_S0_PWROK_CPU1_LVC1_R),                     //alarios: CLX CPU1 interposer not connected to this one, no need to reporpuse
 
	//S3 Pwr-seq control
	.FM_SX_SW_P12V_STBY_EN(FM_SX_SW_P12V_STBY_R_EN),
	.FM_SX_SW_P12V_EN(FM_SX_SW_P12V_R_EN),
	.FM_DIMM_12V_CPS_S5_N(FM_DIMM_12V_CPS_S5_N),

    //IDV SGPIO signaling
    .iSGPIO_IDV_CLK(SGPIO_IDV_CLK_R),
    .iSGPIO_IDV_DOUT(SGPIO_IDV_DOUT_R),
    .oSGPIO_IDV_DIN(SGPIO_IDV_DIN_R),
    .iSGPIO_IDV_LD_N(SGPIO_IDV_LD_R_N),

    //S3M
    .iS3MCpu0CpldCrcError(FM_S3M_CPU0_CPLD_CRC_ERROR),
    .iS3MCpu1CpldCrcError(FM_S3M_CPU1_CPLD_CRC_ERROR),

    //ADR signaling
    .iFmAdrExtTrigger_n(wFM_ADR_EXT_TRIGGER_N),   //from board jumper (was FM_DIS_PS_PWROK_DLY) (when LOW, ADR flow is triggered, to emulate the Surprise PwrDwn condition for testing)
    .iFmAdrMode0(FM_ADR_MODE0),                  //come from PCH (adr_mode[1:0] and indicate if adr is enabled and what mode 
    .iFmAdrMode1(FM_ADR_MODE1),                  //(Disabled=2'b00, Legacy=2'b01, Emulated DDR5=2'b10,  Emulated Copy to Flash=2'b11)
    .iFM_ADR_COMPLETE(FM_ADR_COMPLETE),          //from PCH, to indicate ADR process is completed
    .oFM_ADR_TRIGGER_N(FM_ADR_TRIGGER_N),        //To PCH, to indicate a Surprise Pwr-Dwn is detected and ADR needs to start
    .oFM_ADR_ACK(FM_ADR_ACK_R),                    //To PCH, once all is done in FPGA so PCH can start regular pwr-down.iFM_ADR_COMPLETE

    //PFR signals
    .iPfrLocalSync(HPFR_TO_MISC),          //Comes from PFR logic, it is used in modular design by PFRs to sync
    .oFmPfrGlbAck(HPFR_FROM_MISC),                //This goes to PFR and it is used in modular design to sync among PFRs (in RP there is no real use)
    .oHPFREnable(FM_MODULAR_STANDALONE),                //Goes to PFR logic (comes from SGPIO I/F from Secondary/Modular FPGA). Used by PFR in Modular Design
    .oIsLegacyNode(FM_MODULAR_LEGACY),
    
    .FM_PFR_MUX_OE_CTL_PLD(FM_PFR_MUX_OE_CTL_PLD),
    .iPFROverride(1'b0),                           //when asserted, PFR Post-Codes take precedence over BIOS or FPGA PostCodes
    .iPFRPostCode(8'hFF),                          //PFR PostCodes (Hex format [7:0], MSNible for Display1 and LSNible for Display2)
                                                   //will be encoded inside and added Display2 dot for identification
	.FM_PFR_SLP_SUS_EN(FM_PFR_SLP_SUS_EN_R_N),     //used here to be able to program with the DediProg by keeping it HIGH whenever DEDI_BUSY_N signal transitions from HIGH to LOW

    // Interposer PFR reuse ports
	.SMB_S3M_CPU0_SDA(SMB_S3M_CPU0_SDA_LVC1),
    .SMB_S3M_CPU0_SCL(SMB_S3M_CPU0_SCL_LVC1),
    .SMB_S3M_CPU1_SDA(SMB_S3M_CPU1_SDA_LVC1),
    .SMB_S3M_CPU1_SCL(SMB_S3M_CPU1_SCL_LVC1)
);


   
   ////////////////////////////////////////////////////////
   //DbgAdrExtTrig dbg1 (
   //                    .source({wNada1, wNada2, wDbgSel1, wDbgAdrExtTrig, wNada3}),
   //                    .source_clk(wClk_2M)
   //                    );

   assign wFM_ADR_EXT_TRIGGER_N = /*wExtTrigSel ? wExtTrigN : */ FM_DIS_PS_PWROK_DLY;

   //DbgPwrGdFail DbgPwrGdFail
   //  (.source(wPwrgdPvccd)
      //.source_clk(iClk)
   //   );

   //assign PWRGD_PVCCD_HV_CPU0 = wPwrgdPvccd ? 1'bZ : 1'b0;
   

endmodule // Archer_City_Main_TOP
