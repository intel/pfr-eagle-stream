//////////////////////////////////////////////////////////////////////////////////
/*! 
	\brief      <b>Top Module - Wilson City RP</b>    
	\details    Wilson_City_RP_TOP.\n     
	\file       Wilson_City_RP_TOP.v
	\author     amr/jorge.juarez.campos@intel.com  
	\date       November 28, 2018
	\brief      $RCSfIle: Wilson_City_RP_TOP.v.rca $    Jorge
				$Date:  $ 
				$Author:  $   
				$Revision:  $    
				$Aliases: $
				$Log: Wilson_City_RP_TOP.v.rca $
				  
				<b>Project:</b> Wilson City RP\n        
				<b>Group:</b> BD\n
				<b>Test-bench:</b> 
				<b>Resources:</b>   <ol>
										<li>Max 10 - 10M16
									</ol>  
	\copyright  Intel Proprietary -- Copyright \htmlonly <script>document.write(new Date().getFullYear())</script> \endhtmlonly Intel -- All rights reserved
*/
//////////////////////////////////////////////////////////////////////////////////

`timescale 1ns/1ns   
//define//undef
`undef SIMULATION 

module Wilson_City_RP_TOP (
// Input CLK
	input   CLK_25M_CKMNG_MAIN_PLD,

//GSX Inte  with BMC
	input   SGPIO_BMC_CLK,
	input   SGPIO_BMC_DOUT,
	output  SGPIO_BMC_DIN,
	input   SGPIO_BMC_LD_N,

//LED and 7-Seg Control Logic
	output [7:0]LED_CONTROL,
	output  FM_CPU1_DIMM_CH1_4_FAULT_LED_SEL,
	output  FM_CPU1_DIMM_CH5_8_FAULT_LED_SEL,
	output  FM_CPU2_DIMM_CH1_4_FAULT_LED_SEL,
	output  FM_CPU2_DIMM_CH5_8_FAULT_LED_SEL,
	output  FM_FAN_FAULT_LED_SEL_N,
	output  FM_POST_7SEG1_SEL_N,
	output  FM_POST_7SEG2_SEL_N,
	output  FM_POSTLED_SEL,

//CATERR DLY
	output  FM_CPU_CATERR_DLY_LVT3_N,
	input   FM_CPU_CATERR_PLD_LVT3_N,

//ADR 
	input   FM_ADR_COMPLETE,

	output  FM_ADR_COMPLETE_DLY,
	output  FM_ADR_SMI_GPIO_N,
	output  FM_ADR_TRIGGER_N,

	input   FM_PLD_PCH_DATA_R,
	input   FM_PS_PWROK_DLY_SEL,
	input   FM_DIS_PS_PWROK_DLY,

//ESPI Sup  
	output  FM_PCH_ESPI_MUX_SEL_R,

//System T  E
	input   FM_PMBUS_ALERT_B_EN,
	input   FM_THROTTLE_R_N,
	input   IRQ_SML1_PMBUS_PLD_ALERT_N,

	output  FM_SYS_THROTTLE_LVC3_PLD_R,

// Termtrip dly
	input   FM_CPU1_THERMTRIP_LVT3_PLD_N,
	input   FM_CPU2_THERMTRIP_LVT3_PLD_N,
	input   FM_MEM_THERM_EVENT_CPU1_LVT3_N,
	input   FM_MEM_THERM_EVENT_CPU2_LVT3_N,

	output  FM_THERMTRIP_DLY_R,
//MEMHOT
	input   IRQ_PVDDQ_ABCD_CPU1_VRHOT_LVC3_N,
	input   IRQ_PVDDQ_EFGH_CPU1_VRHOT_LVC3_N,
	input   IRQ_PVDDQ_ABCD_CPU2_VRHOT_LVC3_N,
	input   IRQ_PVDDQ_EFGH_CPU2_VRHOT_LVC3_N,

	output  FM_CPU1_MEMHOT_IN,
	output  FM_CPU2_MEMHOT_IN,

//MEMTRIP
	input   FM_CPU1_MEMTRIP_N,
	input   FM_CPU2_MEMTRIP_N,

//PROCHOT
	input   FM_PVCCIN_CPU1_PWR_IN_ALERT_N,
	input   FM_PVCCIN_CPU2_PWR_IN_ALERT_N,
	input   IRQ_PVCCIN_CPU1_VRHOT_LVC3_N,
	input   IRQ_PVCCIN_CPU2_VRHOT_LVC3_N,

	output  FM_CPU1_PROCHOT_LVC3_N,
	output  FM_CPU2_PROCHOT_LVC3_N,

//PERST &   
	input   FM_RST_PERST_BIT0,
	input   FM_RST_PERST_BIT1,
	input   FM_RST_PERST_BIT2,

	output  RST_PCIE_PERST0_N,
	output  RST_PCIE_PERST1_N,
	output  RST_PCIE_PERST2_N,

	output  RST_CPU1_LVC3_N,
	output  RST_CPU2_LVC3_N,

	output  RST_PLTRST_PLD_B_N,
	input   RST_PLTRST_PLD_N,

//FIVR
	input   FM_CPU1_FIVR_FAULT_LVT3_PLD,
	input   FM_CPU2_FIVR_FAULT_LVT3_PLD,

//CPU Misc
	input   FM_CPU1_PKGID0,
	input   FM_CPU1_PKGID1,
	input   FM_CPU1_PKGID2,

	input   FM_CPU1_PROC_ID0,
	input   FM_CPU1_PROC_ID1,

	input   FM_CPU1_INTR_PRSNT,
	input   FM_CPU1_SKTOCC_LVT3_PLD_N,

	input   FM_CPU2_PKGID0,
	input   FM_CPU2_PKGID1,
	input   FM_CPU2_PKGID2,

	input   FM_CPU2_PROC_ID0,
	input   FM_CPU2_PROC_ID1,

	input   FM_CPU2_INTR_PRSNT,
	input   FM_CPU2_SKTOCC_LVT3_PLD_N,

//BMC
	input   FM_BMC_PWRBTN_OUT_N,
	output  FM_BMC_PLD_PWRBTN_OUT_N,

	input   FM_BMC_ONCTL_N_PLD,
	output  RST_SRST_BMC_PLD_R_N,

	output  FM_P2V5_BMC_EN_R,
	input   PWRGD_P1V1_BMC_AUX,

//PCH
	output  RST_RSMRST_PLD_R_N,

	output  PWRGD_PCH_PWROK_R,
	output  PWRGD_SYS_PWROK_R,

	input   FM_SLP_SUS_RSM_RST_N,
	input   FM_SLPS3_PLD_N,
	input   FM_SLPS4_PLD_N,
	input   FM_PCH_PRSNT_N,

	output  FM_PCH_P1V8_AUX_EN_R,

	input   PWRGD_P1V05_PCH_AUX,
	input   PWRGD_P1V8_PCH_AUX_PLD,

//PSU Ctl
	output  FM_PS_EN_PLD_R,
	input   PWRGD_PS_PWROK_PLD_R,

//Clock Lo   
	output 	FM_CPU_BCLK5_OE_R_N,
	inout   FM_PLD_CLKS_OE_R_N,

//Base Log  
	input   PWRGD_P3V3_AUX_PLD_R,

//Main VR & Logic
	input   PWRGD_P3V3,

	output  FM_P5V_EN,
	output  FM_AUX_SW_EN,

//Mem
	input   PWRGD_CPU1_PVDDQ_ABCD,
	input   PWRGD_CPU1_PVDDQ_EFGH,
	input   PWRGD_CPU2_PVDDQ_ABCD,
	input   PWRGD_CPU2_PVDDQ_EFGH,

	output  FM_PVPP_CPU1_EN_R,
	output  FM_PVPP_CPU2_EN_R,

//CPU
	output  PWRGD_CPU1_LVC3,
	output  PWRGD_CPU2_LVC3,

	input   PWRGD_CPUPWRGD_PLD_R,
	output  PWRGD_DRAMPWRGD_CPU,

	output  FM_P1V1_EN,

	output  FM_P1V8_PCIE_CPU1_EN,
	output  FM_P1V8_PCIE_CPU2_EN,

	output  FM_PVCCANA_CPU1_EN,
	output  FM_PVCCANA_CPU2_EN,

	output  FM_PVCCIN_CPU1_EN_R,
	output  FM_PVCCIN_CPU2_EN_R,

	output  FM_PVCCIO_CPU1_EN_R,
	output  FM_PVCCIO_CPU2_EN_R,

	output  FM_VCCSA_CPU1_EN,
	output  FM_VCCSA_CPU2_EN,

	input   PWRGD_BIAS_P1V1,

	input   PWRGD_P1V8_PCIE_CPU1,
	input   PWRGD_P1V8_PCIE_CPU2,

	input   PWRGD_PVCCIN_CPU1,
	input   PWRGD_PVCCIN_CPU2,
 
	input   PWRGD_PVCCIO_CPU1,
	input   PWRGD_PVCCIO_CPU2,

	input   PWRGD_PVCCSA_CPU1,
	input   PWRGD_PVCCSA_CPU2,

	input   PWRGD_VCCANA_PCIE_CPU1,
	input   PWRGD_VCCANA_PCIE_CPU2,

//Dediprog Detection Support 
	input   RST_DEDI_BUSY_PLD_N,

//DBP 
	input   DBP_POWER_BTN_N,
	input   DBP_SYSPWROK_PLD,

//Debug
	input   FM_FORCE_PWRON_LVC3,
	output  FM_PLD_HEARTBEAT_LVC3,

//Debug pins I/O
	output 	SGPIO_DEBUG_PLD_CLK,
	input 	SGPIO_DEBUG_PLD_DIN,
	output 	SGPIO_DEBUG_PLD_DOUT,
	output 	SGPIO_DEBUG_PLD_LD_N,

	input 	SMB_DEBUG_PLD_SCL,
	inout 	SMB_DEBUG_PLD_SDA,

	input 	FM_PLD_REV_N,

	input 	SPI_BMC_BOOT_R_CS1_N,
	output 	SPI_PFR_BOOT_CS1_N,

// Front Panel 
	output  FP_LED_FAN_FAULT_PWRSTBY_PLD_N,
	output  FP_BMC_PWR_BTN_CO_N,

// PFR Dedicated Signals 
	output  FAN_BMC_PWM_R,

	input   PWRGD_DSW_PWROK_R,
	input   FM_BMC_BMCINIT,

	input   FM_ME_PFR_1,
	input   FM_ME_PFR_2,

	input   FM_PFR_PROV_UPDATE_N,
	input   FM_PFR_DEBUG_MODE_N,
	input   FM_PFR_FORCE_RECOVERY_N,

	output  FM_PFR_MUX_OE_CTL_PLD,

	output 	RST_PFR_EXTRST_N,
	inout   RST_PFR_OVR_RTC_N,
	inout   RST_PFR_OVR_SRTC_N,

	input   FP_ID_BTN_N,
	output  FP_ID_BTN_PFR_N,

	output  FP_ID_LED_PFR_N,
	input   FP_ID_LED_N,

	output  FP_LED_STATUS_AMBER_PFR_N,
	input   FP_LED_STATUS_AMBER_N,

	output  FP_LED_STATUS_GREEN_PFR_N,
	input   FP_LED_STATUS_GREEN_N,

	//  SPI
	output  FM_SPI_PFR_PCH_MASTER_SEL,
	output  FM_SPI_PFR_BMC_BT_MASTER_SEL,

	output  RST_SPI_PFR_BMC_BOOT_N,
	output  RST_SPI_PFR_PCH_N,

	input   SPI_BMC_BOOT_CS_N,
	output  SPI_PFR_BMC_BT_SECURE_CS_N,

	input   SPI_PCH_BMC_PFR_CS0_N,
	output  SPI_PFR_PCH_BMC_SECURE_CS0_N,

	input   SPI_PCH_CS1_N,
	output  SPI_PFR_PCH_SECURE_CS1_N,

	inout   SPI_BMC_BT_MUXED_MON_CLK,
	inout   SPI_BMC_BT_MUXED_MON_MISO,
	inout   SPI_BMC_BT_MUXED_MON_MOSI,

	inout   SPI_PCH_BMC_SAFS_MUXED_MON_CLK,
	inout   SPI_PCH_BMC_SAFS_MUXED_MON_IO0,
	inout   SPI_PCH_BMC_SAFS_MUXED_MON_IO1,
	inout   SPI_PCH_BMC_SAFS_MUXED_MON_IO2,
	inout   SPI_PCH_BMC_SAFS_MUXED_MON_IO3,

	inout   SPI_PFR_BMC_FLASH1_BT_CLK,
	inout   SPI_PFR_BMC_FLASH1_BT_MOSI,
	inout   SPI_PFR_BMC_FLASH1_BT_MISO,
	inout   SPI_PFR_BMC_BOOT_R_IO2,
	inout   SPI_PFR_BMC_BOOT_R_IO3,

	inout   SPI_PFR_PCH_R_CLK,
	inout   SPI_PFR_PCH_R_IO0,
	inout   SPI_PFR_PCH_R_IO1,
	inout   SPI_PFR_PCH_R_IO2,
	inout   SPI_PFR_PCH_R_IO3,

	// SMBus
	inout   SMB_BMC_HSBP_STBY_LVC3_SCL,
	inout   SMB_BMC_HSBP_STBY_LVC3_SDA,
	inout   SMB_PFR_HSBP_STBY_LVC3_SCL,
	inout   SMB_PFR_HSBP_STBY_LVC3_SDA,

	inout   SMB_BMC_SPD_ACCESS_STBY_LVC3_SCL,
	inout   SMB_BMC_SPD_ACCESS_STBY_LVC3_SDA,

	inout   SMB_CPU_PIROM_SCL,
	inout   SMB_CPU_PIROM_SDA,
	inout   SMB_PFR_PMB1_STBY_LVC3_SCL,
	inout   SMB_PFR_PMB1_STBY_LVC3_SDA,
	inout 	SMB_PMBUS_SML1_STBY_LVC3_SDA,
	inout 	SMB_PMBUS_SML1_STBY_LVC3_SCL,

	input   SMB_PCH_PMBUS2_STBY_LVC3_SCL,
	inout   SMB_PCH_PMBUS2_STBY_LVC3_SDA,
	inout   SMB_PFR_PMBUS2_STBY_LVC3_SCL,
	inout   SMB_PFR_PMBUS2_STBY_LVC3_SDA,

	inout   SMB_PFR_RFID_STBY_LVC3_SCL,
	inout   SMB_PFR_RFID_STBY_LVC3_SDA,

	inout 	SMB_PFR_DDRABCD_CPU1_LVC2_SCL,
	inout 	SMB_PFR_DDRABCD_CPU1_LVC2_SDA,
	
	output 	FM_PFR_ON,
	input 	FM_PFR_SLP_SUS_N
);

//////////////////////////////////////////////////////////////////////////////////
// Parameters
//////////////////////////////////////////////////////////////////////////////////
parameter  LOW =1'b0;
parameter  HIGH=1'b1;  
parameter  Z=1'bz;

//////////////////////////////////////////////////////////////////////////////////
// Internal Signals
//////////////////////////////////////////////////////////////////////////////////
wire wClk_RTC, wClk_2M, wClk_3M, wClk_50M;
wire w1mSCE;
wire wFP_BMC_PWR_BTN_CO_N;
wire wnSDAOE;
wire wFM_PLD_CLKS_OE_R_N;
wire wFM_PVCCIN_CPU1_EN;
wire wFM_PVCCIN_CPU2_EN;
wire wSMB_DEBUG_PLD_SDA_OE;
wire wFM_CPU_BCLK5_OE_R_N;


////////////////////////////////////////////////////////////////////////////////// //
// Continuous assignments                                                          //
////////////////////////////////////////////////////////////////////////////////// //
`ifdef SIMULATION
assign 		SMB_PCH_PMBUS2_STBY_LVC3_SDA 		= ( !wnSDAOE ) 				? LOW 	: HIGH;
assign 		FP_BMC_PWR_BTN_CO_N 				= wFP_BMC_PWR_BTN_CO_N 		? HIGH 	: LOW;
assign 		FM_PLD_CLKS_OE_R_N       			= (wFM_PLD_CLKS_OE_R_N) 	? 1'b1 	: 1'b0;//Clock control as OD to remove leakage
assign 		FM_CPU_BCLK5_OE_R_N       			= (wFM_CPU_BCLK5_OE_R_N)	? 1'b1 	: 1'b0;//Clock control as OD to remove leakage
assign 		FM_PVCCIN_CPU1_EN_R        			= (wFM_PVCCIN_CPU1_EN) 		? 1'b1 	: 1'b0;//VCCIN VR Drop Support
assign 		FM_PVCCIN_CPU2_EN_R        			= (wFM_PVCCIN_CPU2_EN) 		? 1'b1 	: 1'b0;//VCCIN VR Drop Support
assign 		FM_BMC_PLD_PWRBTN_OUT_N				= FM_BMC_PWRBTN_OUT_N 		? 1'b1 	: LOW; // De-bouncer is not needed in this case
assign 		SMB_DEBUG_PLD_SDA 					= wSMB_DEBUG_PLD_SDA_OE 	? 1'b1	: LOW;
`else
assign 		SMB_PCH_PMBUS2_STBY_LVC3_SDA 		= ( !wnSDAOE ) 				? LOW	: Z;
assign 		FP_BMC_PWR_BTN_CO_N 				= wFP_BMC_PWR_BTN_CO_N 		? Z 	: LOW;
assign 		FM_PLD_CLKS_OE_R_N       			= (wFM_PLD_CLKS_OE_R_N) 	? 1'bz 	: 1'b0;//Clock control as OD to
assign 		FM_CPU_BCLK5_OE_R_N       			= (wFM_CPU_BCLK5_OE_R_N)	? 1'bz 	: 1'b0;//Clock control as OD to
assign 		FM_PVCCIN_CPU1_EN_R        			= (wFM_PVCCIN_CPU1_EN) 		? 1'bz 	: 1'b0;//VCCIN VR Drop Support
assign 		FM_PVCCIN_CPU2_EN_R        			= (wFM_PVCCIN_CPU2_EN) 		? 1'bz 	: 1'b0;//VCCIN VR Drop Support
assign 		FM_BMC_PLD_PWRBTN_OUT_N				= FM_BMC_PWRBTN_OUT_N 		? Z 	: LOW; // De-bouncer is not needed in this case
assign 		SMB_DEBUG_PLD_SDA 					= wSMB_DEBUG_PLD_SDA_OE 	? Z		: LOW;
`endif


////////////////////////////////////////////////////////////////////////////////// //
//PFR By pass logic   
////////////////////////////////////////////////////////////////////////////////// //
assign     	FAN_BMC_PWM_R = Z;
assign     	RST_PFR_OVR_RTC_N = Z; 
assign     	RST_PFR_OVR_SRTC_N = Z; 
assign     	SPI_BMC_BT_MUXED_MON_CLK = Z; 
assign     	SPI_BMC_BT_MUXED_MON_MISO = Z; 
assign     	SPI_BMC_BT_MUXED_MON_MOSI = Z; 
assign     	SPI_PCH_BMC_SAFS_MUXED_MON_CLK= Z; 
assign     	SPI_PCH_BMC_SAFS_MUXED_MON_IO0= Z; 
assign     	SPI_PCH_BMC_SAFS_MUXED_MON_IO1= Z; 
assign     	SPI_PCH_BMC_SAFS_MUXED_MON_IO2= Z; 
assign     	SPI_PCH_BMC_SAFS_MUXED_MON_IO3= Z; 
assign     	SPI_PFR_BMC_FLASH1_BT_CLK= Z; 
assign     	SPI_PFR_BMC_FLASH1_BT_MOSI= Z; 
assign     	SPI_PFR_BMC_FLASH1_BT_MISO= Z; 
assign     	SPI_PFR_BMC_BOOT_R_IO2= Z; 
assign     	SPI_PFR_BMC_BOOT_R_IO3= Z; 
assign     	SPI_PFR_PCH_R_CLK= Z; 
assign     	SPI_PFR_PCH_R_IO0= Z; 
assign     	SPI_PFR_PCH_R_IO1= Z; 
assign     	SPI_PFR_PCH_R_IO2= Z; 
assign     	SPI_PFR_PCH_R_IO3= Z; 
assign     	SMB_BMC_HSBP_STBY_LVC3_SCL= Z;
assign     	SMB_BMC_HSBP_STBY_LVC3_SDA= Z;
assign     	SMB_PFR_HSBP_STBY_LVC3_SCL= Z;
assign     	SMB_PFR_HSBP_STBY_LVC3_SDA= Z;
assign     	SMB_BMC_SPD_ACCESS_STBY_LVC3_SCL= Z;
assign     	SMB_BMC_SPD_ACCESS_STBY_LVC3_SDA= Z;
assign     	SMB_CPU_PIROM_SCL= Z;
assign     	SMB_CPU_PIROM_SDA= Z;
assign     	SMB_PFR_PMB1_STBY_LVC3_SCL= Z;
assign     	SMB_PFR_PMB1_STBY_LVC3_SDA= Z;
assign     	SMB_PMBUS_SML1_STBY_LVC3_SDA= Z;
assign     	SMB_PMBUS_SML1_STBY_LVC3_SCL= Z;
assign     	SMB_PFR_PMBUS2_STBY_LVC3_SCL= Z;
assign     	SMB_PFR_PMBUS2_STBY_LVC3_SDA= Z;
assign     	SMB_PFR_RFID_STBY_LVC3_SCL= Z;
assign     	SMB_PFR_RFID_STBY_LVC3_SDA= Z;
assign 		SMB_PFR_DDRABCD_CPU1_LVC2_SCL = Z;
assign 		SMB_PFR_DDRABCD_CPU1_LVC2_SDA = Z;
assign 		FM_PFR_ON = 1'b0;

//BMC workaround for CS
assign SPI_PFR_BOOT_CS1_N 			= SPI_BMC_BOOT_CS_N;
assign SPI_PFR_BMC_BT_SECURE_CS_N 	= 1'b1;

//////////////////////////////////////////////////////////////////////////////////
// Instances HW Dedicated 
//////////////////////////////////////////////////////////////////////////////////
`ifdef simul
assign wClk_2M = CLK_25M_CKMNG_MAIN_PLD;
assign wClk_50M = CLK_25M_CKMNG_MAIN_PLD;
`else
PLL_CLK PLL_CLK_inst (
	.inclk0 ( CLK_25M_CKMNG_MAIN_PLD ),
	.c0 ( wClk_50M ),	//50M
	.c2 ( wClk_2M )		//2M
);
`endif


PFR_ByPass mPFR_ByPass
(
	.FM_PFR_PROV_UPDATE_N            	(FM_PFR_PROV_UPDATE_N),
	.FM_PFR_DEBUG_MODE_N             	(FM_PFR_DEBUG_MODE_N),
	.FM_PFR_FORCE_RECOVERY_N         	(FM_PFR_FORCE_RECOVERY_N),

	.RST_PFR_EXTRST_N                	(RST_PFR_EXTRST_N),

	.FP_ID_BTN_N                     	(FP_ID_BTN_N),
	.FP_ID_BTN_PFR_N                 	(FP_ID_BTN_PFR_N),
	.FP_ID_LED_PFR_N                 	(FP_ID_LED_PFR_N),
	.FP_ID_LED_N                     	(FP_ID_LED_N),
	.FP_LED_STATUS_AMBER_PFR_N       	(FP_LED_STATUS_AMBER_PFR_N),
	.FP_LED_STATUS_AMBER_N           	(FP_LED_STATUS_AMBER_N),
	.FP_LED_STATUS_GREEN_PFR_N       	(FP_LED_STATUS_GREEN_PFR_N),
	.FP_LED_STATUS_GREEN_N           	(FP_LED_STATUS_GREEN_N),

	.FM_SPI_PFR_PCH_MASTER_SEL       	(FM_SPI_PFR_PCH_MASTER_SEL),
	.FM_SPI_PFR_BMC_BT_MASTER_SEL    	(FM_SPI_PFR_BMC_BT_MASTER_SEL),

	.RST_SPI_PFR_BMC_BOOT_N          	(RST_SPI_PFR_BMC_BOOT_N),
	.RST_SPI_PFR_PCH_N               	(RST_SPI_PFR_PCH_N),

	// .SPI_BMC_BOOT_CS_N               	(SPI_BMC_BOOT_CS_N),
	// .SPI_PFR_BMC_BT_SECURE_CS_N      	(SPI_PFR_BMC_BT_SECURE_CS_N),//BACKUP
	// .SPI_PFR_BMC_BT_SECURE_CS_N      	(SPI_PFR_BOOT_CS1_N),

	.SPI_PCH_BMC_PFR_CS0_N           	(SPI_PCH_BMC_PFR_CS0_N),
	.SPI_PFR_PCH_BMC_SECURE_CS0_N    	(SPI_PFR_PCH_BMC_SECURE_CS0_N),

	.SPI_PCH_CS1_N                   	(SPI_PCH_CS1_N),
	.SPI_PFR_PCH_SECURE_CS1_N        	(SPI_PFR_PCH_SECURE_CS1_N)
);


Wilson_City_Main mWilson_City_Main
(
	.iClk_2M                          	(wClk_2M),
	.iClk_50M                         	(wClk_50M),
	.o1mSCE 							(w1mSCE),

//GSX Interface with BMC
	.SGPIO_BMC_CLK                    	(SGPIO_BMC_CLK),
	.SGPIO_BMC_DOUT                   	(SGPIO_BMC_DOUT),
	.SGPIO_BMC_DIN                    	(SGPIO_BMC_DIN),
	.SGPIO_BMC_LD_N                   	(SGPIO_BMC_LD_N),

//I2C Support 
	.SMB_PLD_SDA                      	(SMB_PCH_PMBUS2_STBY_LVC3_SDA),
	.SMB_PLD_SCL                      	(SMB_PCH_PMBUS2_STBY_LVC3_SCL),
	.onSDAOE                          	(wnSDAOE),

//LED and 7-Seg Control Logic
	.LED_CONTROL                      	(LED_CONTROL),
	.FM_CPU1_DIMM_CH1_4_FAULT_LED_SEL 	(FM_CPU1_DIMM_CH1_4_FAULT_LED_SEL),
	.FM_CPU1_DIMM_CH5_8_FAULT_LED_SEL 	(FM_CPU1_DIMM_CH5_8_FAULT_LED_SEL),
	.FM_CPU2_DIMM_CH1_4_FAULT_LED_SEL 	(FM_CPU2_DIMM_CH1_4_FAULT_LED_SEL),
	.FM_CPU2_DIMM_CH5_8_FAULT_LED_SEL 	(FM_CPU2_DIMM_CH5_8_FAULT_LED_SEL),
	.FM_FAN_FAULT_LED_SEL_N           	(FM_FAN_FAULT_LED_SEL_N),
	.FM_POST_7SEG1_SEL_N              	(FM_POST_7SEG1_SEL_N),
	.FM_POST_7SEG2_SEL_N              	(FM_POST_7SEG2_SEL_N),
	.FM_POSTLED_SEL                   	(FM_POSTLED_SEL),

//CATERR DLY
	.FM_CPU_CATERR_DLY_LVT3_N         	(FM_CPU_CATERR_DLY_LVT3_N),
	.FM_CPU_CATERR_PLD_LVT3_N         	(FM_CPU_CATERR_PLD_LVT3_N),

//ADR 
	.FM_ADR_COMPLETE                  	(FM_ADR_COMPLETE),

	.FM_ADR_COMPLETE_DLY              	(FM_ADR_COMPLETE_DLY),
	.FM_ADR_SMI_GPIO_N                	(FM_ADR_SMI_GPIO_N),
	.FM_ADR_TRIGGER_N                 	(FM_ADR_TRIGGER_N),

	.FM_PLD_PCH_DATA                  	(FM_PLD_PCH_DATA_R),
	.FM_PS_PWROK_DLY_SEL              	(FM_PS_PWROK_DLY_SEL),
	.FM_DIS_PS_PWROK_DLY              	(FM_DIS_PS_PWROK_DLY),

//ESPI Support
	.FM_PCH_ESPI_MUX_SEL              	(FM_PCH_ESPI_MUX_SEL_R),

//System THROTTLE
	.FM_PMBUS_ALERT_B_EN              	(FM_PMBUS_ALERT_B_EN),
	.FM_THROTTLE_N                    	(FM_THROTTLE_R_N),
	.IRQ_SML1_PMBUS_PLD_ALERT_N       	(IRQ_SML1_PMBUS_PLD_ALERT_N),

	.FM_SYS_THROTTLE_LVC3_PLD         	(FM_SYS_THROTTLE_LVC3_PLD_R),
// Termtrip dly
	.FM_CPU1_THERMTRIP_LVT3_PLD_N     	(FM_CPU1_THERMTRIP_LVT3_PLD_N),
	.FM_CPU2_THERMTRIP_LVT3_PLD_N     	(FM_CPU2_THERMTRIP_LVT3_PLD_N),
	.FM_MEM_THERM_EVENT_CPU1_LVT3_N   	(FM_MEM_THERM_EVENT_CPU1_LVT3_N),
	.FM_MEM_THERM_EVENT_CPU2_LVT3_N   	(FM_MEM_THERM_EVENT_CPU2_LVT3_N),

	.FM_THERMTRIP_DLY                 	(FM_THERMTRIP_DLY_R),
//MEMHOT
	.IRQ_PVDDQ_ABCD_CPU1_VRHOT_LVC3_N 	(IRQ_PVDDQ_ABCD_CPU1_VRHOT_LVC3_N),
	.IRQ_PVDDQ_EFGH_CPU1_VRHOT_LVC3_N 	(IRQ_PVDDQ_EFGH_CPU1_VRHOT_LVC3_N),
	.IRQ_PVDDQ_ABCD_CPU2_VRHOT_LVC3_N 	(IRQ_PVDDQ_ABCD_CPU2_VRHOT_LVC3_N),
	.IRQ_PVDDQ_EFGH_CPU2_VRHOT_LVC3_N 	(IRQ_PVDDQ_EFGH_CPU2_VRHOT_LVC3_N),

	.FM_CPU1_MEMHOT_IN                	(FM_CPU1_MEMHOT_IN),
	.FM_CPU2_MEMHOT_IN                	(FM_CPU2_MEMHOT_IN),

//MEMTRIP
	.FM_CPU1_MEMTRIP_N                	(FM_CPU1_MEMTRIP_N),
	.FM_CPU2_MEMTRIP_N                	(FM_CPU2_MEMTRIP_N),

//PROCHOT
	.FM_PVCCIN_CPU1_PWR_IN_ALERT_N    	(FM_PVCCIN_CPU1_PWR_IN_ALERT_N),
	.FM_PVCCIN_CPU2_PWR_IN_ALERT_N    	(FM_PVCCIN_CPU2_PWR_IN_ALERT_N),
	.IRQ_PVCCIN_CPU1_VRHOT_LVC3_N     	(IRQ_PVCCIN_CPU1_VRHOT_LVC3_N),
	.IRQ_PVCCIN_CPU2_VRHOT_LVC3_N     	(IRQ_PVCCIN_CPU2_VRHOT_LVC3_N),

	.FM_CPU1_PROCHOT_LVC3_N           	(FM_CPU1_PROCHOT_LVC3_N),
	.FM_CPU2_PROCHOT_LVC3_N           	(FM_CPU2_PROCHOT_LVC3_N),

//PERST & RST
	.FM_RST_PERST_BIT0                	(FM_RST_PERST_BIT0),
	.FM_RST_PERST_BIT1                	(FM_RST_PERST_BIT1),
	.FM_RST_PERST_BIT2                	(FM_RST_PERST_BIT2),

	.RST_PCIE_PERST0_N                	(RST_PCIE_PERST0_N),
	.RST_PCIE_PERST1_N                	(RST_PCIE_PERST1_N),
	.RST_PCIE_PERST2_N                	(RST_PCIE_PERST2_N),

	.RST_CPU1_LVC3_N                  	(RST_CPU1_LVC3_N),
	.RST_CPU2_LVC3_N                  	(RST_CPU2_LVC3_N),

	.RST_PLTRST_B_N                   	(RST_PLTRST_PLD_B_N),
	.RST_PLTRST_N                     	(RST_PLTRST_PLD_N),

//FIVR
	.FM_CPU1_FIVR_FAULT_LVT3          	(FM_CPU1_FIVR_FAULT_LVT3_PLD),
	.FM_CPU2_FIVR_FAULT_LVT3          	(FM_CPU2_FIVR_FAULT_LVT3_PLD),

//CPU Misc
	.FM_CPU1_PKGID0                   	(FM_CPU1_PKGID0),
	.FM_CPU1_PKGID1                   	(FM_CPU1_PKGID1),
	.FM_CPU1_PKGID2                   	(FM_CPU1_PKGID2),

	.FM_CPU1_PROC_ID0                 	(FM_CPU1_PROC_ID0),
	.FM_CPU1_PROC_ID1                 	(FM_CPU1_PROC_ID1),

	.FM_CPU1_INTR_PRSNT               	(FM_CPU1_INTR_PRSNT),
	.FM_CPU1_SKTOCC_LVT3_N            	(FM_CPU1_SKTOCC_LVT3_PLD_N),

	.FM_CPU2_PKGID0                   	(FM_CPU2_PKGID0),
	.FM_CPU2_PKGID1                   	(FM_CPU2_PKGID1),
	.FM_CPU2_PKGID2                   	(FM_CPU2_PKGID2),

	.FM_CPU2_PROC_ID0                 	(FM_CPU2_PROC_ID0),
	.FM_CPU2_PROC_ID1                 	(FM_CPU2_PROC_ID1),

	.FM_CPU2_INTR_PRSNT               	(FM_CPU2_INTR_PRSNT),
	.FM_CPU2_SKTOCC_LVT3_N            	(FM_CPU2_SKTOCC_LVT3_PLD_N),

//BMC
	.FM_BMC_PWRBTN_OUT_N              	(FM_BMC_PWRBTN_OUT_N),

	.FM_BMC_ONCTL_N                   	(FM_BMC_ONCTL_N_PLD),
	.RST_SRST_BMC_PLD_N               	(RST_SRST_BMC_PLD_R_N),

	.FM_P2V5_BMC_EN                   	(FM_P2V5_BMC_EN_R),
	.PWRGD_P1V1_BMC_AUX               	(PWRGD_P1V1_BMC_AUX),

//PCH
	.RST_RSMRST_N                     	(RST_RSMRST_PLD_R_N),

	.PWRGD_PCH_PWROK                  	(PWRGD_PCH_PWROK_R),
	.PWRGD_SYS_PWROK                  	(PWRGD_SYS_PWROK_R),

	.FM_SLP_SUS_RSM_RST_N             	(FM_SLP_SUS_RSM_RST_N),
	.FM_SLPS3_N                       	(FM_SLPS3_PLD_N),
	.FM_SLPS4_N                       	(FM_SLPS4_PLD_N),
	.FM_PCH_PRSNT_N                   	(FM_PCH_PRSNT_N),

	.FM_PCH_P1V8_AUX_EN               	(FM_PCH_P1V8_AUX_EN_R),
  
	.PWRGD_P1V05_PCH_AUX              	(PWRGD_P1V05_PCH_AUX),
	.PWRGD_P1V8_PCH_AUX               	(PWRGD_P1V8_PCH_AUX_PLD),

	.FM_PFR_MUX_OE_CTL_PLD            	(FM_PFR_MUX_OE_CTL_PLD),
	.RST_DEDI_BUSY_PLD_N              	(RST_DEDI_BUSY_PLD_N),

//PSU Ctl
	.FM_PS_EN                         	(FM_PS_EN_PLD_R),
	.PWRGD_PS_PWROK                   	(PWRGD_PS_PWROK_PLD_R),

//Clock Logic    
	.FM_PLD_CLKS_OE_N                 	(wFM_PLD_CLKS_OE_R_N),
	.FM_CPU_BCLK5_OE_R_N  			  	(wFM_CPU_BCLK5_OE_R_N),

//Base Logic
	.PWRGD_P3V3_AUX                   	(PWRGD_P3V3_AUX_PLD_R),

//Main VR & Logic
	.PWRGD_P3V3                       	(PWRGD_P3V3),

	.FM_P5V_EN                        	(FM_P5V_EN),
	.FM_AUX_SW_EN                     	(FM_AUX_SW_EN),

//Mem
	.PWRGD_CPU1_PVDDQ_ABCD            	(PWRGD_CPU1_PVDDQ_ABCD),
	.PWRGD_CPU1_PVDDQ_EFGH            	(PWRGD_CPU1_PVDDQ_EFGH),
	.PWRGD_CPU2_PVDDQ_ABCD            	(PWRGD_CPU2_PVDDQ_ABCD),
	.PWRGD_CPU2_PVDDQ_EFGH            	(PWRGD_CPU2_PVDDQ_EFGH),

	.FM_PVPP_CPU1_EN                  	(FM_PVPP_CPU1_EN_R),
	.FM_PVPP_CPU2_EN                  	(FM_PVPP_CPU2_EN_R),

//CPU
	.PWRGD_CPU1_LVC3                  	(PWRGD_CPU1_LVC3),
	.PWRGD_CPU2_LVC3                  	(PWRGD_CPU2_LVC3),

	.PWRGD_CPUPWRGD                   	(PWRGD_CPUPWRGD_PLD_R),
	.PWRGD_DRAMPWRGD_CPU              	(PWRGD_DRAMPWRGD_CPU),

	.FM_P1V1_EN                       	(FM_P1V1_EN),

	.FM_P1V8_PCIE_CPU1_EN             	(FM_P1V8_PCIE_CPU1_EN),
	.FM_P1V8_PCIE_CPU2_EN             	(FM_P1V8_PCIE_CPU2_EN),

	.FM_PVCCANA_CPU1_EN               	(FM_PVCCANA_CPU1_EN),
	.FM_PVCCANA_CPU2_EN               	(FM_PVCCANA_CPU2_EN),

	.FM_PVCCIN_CPU1_EN                	(wFM_PVCCIN_CPU1_EN),
	.FM_PVCCIN_CPU2_EN                	(wFM_PVCCIN_CPU2_EN),

	.FM_PVCCIO_CPU1_EN                	(FM_PVCCIO_CPU1_EN_R),
	.FM_PVCCIO_CPU2_EN                	(FM_PVCCIO_CPU2_EN_R),

	.FM_PVCCSA_CPU1_EN                	(FM_VCCSA_CPU1_EN),
	.FM_PVCCSA_CPU2_EN                	(FM_VCCSA_CPU2_EN),

	.PWRGD_BIAS_P1V1                  	(PWRGD_BIAS_P1V1),
 
	.PWRGD_P1V8_PCIE_CPU1             	(PWRGD_P1V8_PCIE_CPU1),
	.PWRGD_P1V8_PCIE_CPU2             	(PWRGD_P1V8_PCIE_CPU2),

	.PWRGD_PVCCIN_CPU1                	(PWRGD_PVCCIN_CPU1),
	.PWRGD_PVCCIN_CPU2                	(PWRGD_PVCCIN_CPU2),
 
	.PWRGD_PVCCIO_CPU1                	(PWRGD_PVCCIO_CPU1),
	.PWRGD_PVCCIO_CPU2                	(PWRGD_PVCCIO_CPU2),

	.PWRGD_PVCCSA_CPU1                	(PWRGD_PVCCSA_CPU1),
	.PWRGD_PVCCSA_CPU2                	(PWRGD_PVCCSA_CPU2),

	.PWRGD_VCCANA_PCIE_CPU1           	(PWRGD_VCCANA_PCIE_CPU1),
	.PWRGD_VCCANA_PCIE_CPU2           	(PWRGD_VCCANA_PCIE_CPU2),

//DBP 
	.DBP_POWER_BTN_N                  	(DBP_POWER_BTN_N),
	.DBP_SYSPWROK                     	(DBP_SYSPWROK_PLD),

//Debug
	.FM_FORCE_PWRON_LVC3              	(FM_FORCE_PWRON_LVC3),
	.FM_PLD_HEARTBEAT_LVC3            	(FM_PLD_HEARTBEAT_LVC3),

// Front Panel 
	.FP_LED_FAN_FAULT_PWRSTBY_PLD_N		(FP_LED_FAN_FAULT_PWRSTBY_PLD_N),
	.FP_BMC_PWR_BTN_CO_N           		(wFP_BMC_PWR_BTN_CO_N),

//Debug pins I/O
	.SGPIO_DEBUG_PLD_CLK 				(SGPIO_DEBUG_PLD_CLK),
	.SGPIO_DEBUG_PLD_DIN 				(SGPIO_DEBUG_PLD_DIN),
	.SGPIO_DEBUG_PLD_DOUT 				(SGPIO_DEBUG_PLD_DOUT),
	.SGPIO_DEBUG_PLD_LD_N 				(SGPIO_DEBUG_PLD_LD_N),

	.SMB_DEBUG_PLD_SCL 					(SMB_DEBUG_PLD_SCL),
	.SMB_DEBUG_PLD_SDA 					(SMB_DEBUG_PLD_SDA),
	.oSMB_DEBUG_PLD_SDA_OE				(wSMB_DEBUG_PLD_SDA_OE),

	.FM_PLD_REV_N 						(FM_PLD_REV_N)//,

	// .SPI_BMC_BOOT_R_CS1_N 			(SPI_BMC_BOOT_R_CS1_N),
	// .SPI_PFR_BOOT_CS1_N 			(SPI_PFR_BOOT_CS1_N)
);

endmodule // Wilson_City_RP_TOP
