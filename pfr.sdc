# Map of pin names to SPI function
global spi_pin_names_map
array set spi_pin_names_map [list]

set spi_pin_names_map(BMC_SPI_DQ0) SPI_PFR_BMC_FLASH1_BT_MOSI
set spi_pin_names_map(BMC_SPI_DQ1) SPI_PFR_BMC_FLASH1_BT_MISO
set spi_pin_names_map(BMC_SPI_DQ2) SPI_PFR_BMC_BOOT_R_IO2
set spi_pin_names_map(BMC_SPI_DQ3) SPI_PFR_BMC_BOOT_R_IO3
set spi_pin_names_map(BMC_SPI_CLK) SPI_PFR_BMC_FLASH1_BT_CLK
set spi_pin_names_map(BMC_SPI_CS_IN) SPI_BMC_BOOT_CS_N
set spi_pin_names_map(BMC_SPI_CS_OUT) SPI_PFR_BMC_BT_SECURE_CS_R_N
set spi_pin_names_map(BMC_SPI_DQ0_MON) SPI_BMC_BT_MUXED_MON_MOSI
set spi_pin_names_map(BMC_SPI_CLK_MON) SPI_BMC_BT_MUXED_MON_CLK

set spi_pin_names_map(PCH_SPI_DQ0) SPI_PFR_PCH_R_IO0
set spi_pin_names_map(PCH_SPI_DQ1) SPI_PFR_PCH_R_IO1
set spi_pin_names_map(PCH_SPI_DQ2) SPI_PFR_PCH_R_IO2
set spi_pin_names_map(PCH_SPI_DQ3) SPI_PFR_PCH_R_IO3
set spi_pin_names_map(PCH_SPI_CLK) SPI_PFR_PCH_R_CLK
set spi_pin_names_map(PCH_SPI_CS_IN) SPI_PCH_PFR_CS0_N
set spi_pin_names_map(PCH_SPI_CS_OUT) SPI_PFR_PCH_SECURE_CS0_R_N
set spi_pin_names_map(PCH_SPI_DQ0_MON) SPI_PCH_MUXED_MON_IO0
set spi_pin_names_map(PCH_SPI_DQ1_MON) SPI_PCH_MUXED_MON_IO1
set spi_pin_names_map(PCH_SPI_DQ2_MON) SPI_PCH_MUXED_MON_IO2
set spi_pin_names_map(PCH_SPI_DQ3_MON) SPI_PCH_MUXED_MON_IO3
set spi_pin_names_map(PCH_SPI_CLK_MON) SPI_PCH_MUXED_MON_CLK

# Map of GPOs to bit numbers
source gen_gpo_controls_pkg.tcl

#**************************************************************
# Time Information
#**************************************************************
set_time_format -unit ns -decimal_places 3

# Propigation delay (in/ns)
# In general on the PCB, the signal travels at the speed of ~160 ps/inch (1000 mils = 1 inch)
global PCB_PROP_IN_PER_NS
set PCB_PROP_IN_PER_NS 6.25

#**************************************************************
# Helper procs
#**************************************************************
proc calc_prop_delay {trace_length} {
	global PCB_PROP_IN_PER_NS

	set delay [format "%.2f" [expr { ($trace_length/1000.0) / $PCB_PROP_IN_PER_NS }]] 
	return $delay
}

proc report_spi_names {} {
	global spi_pin_names_map

	post_message -type info "SPI Pin Map:"
	foreach key [lsort [array names spi_pin_names_map]] {
		post_message -type info "\t$key => $spi_pin_names_map($key)"
	}
}

proc report_pch_spi_timing {} {
	global spi_pin_names_map

	# Report paths
	set T_SPI_PCH_CS_N_to_SPI_PCH_SECURE_CS_N [lindex [report_path -from [get_ports $spi_pin_names_map(PCH_SPI_CS_IN)] -to [get_ports $spi_pin_names_map(PCH_SPI_CS_OUT)]] 1]
	
	report_timing -from [get_ports $spi_pin_names_map(PCH_SPI_CS_IN)] -to [get_ports $spi_pin_names_map(PCH_SPI_CS_OUT)] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||CS to Secure CS : Setup} -setup
	report_timing -from [get_ports $spi_pin_names_map(PCH_SPI_CS_IN)] -to [get_ports $spi_pin_names_map(PCH_SPI_CS_OUT)] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||CS to Secure CS : Hold} -hold

	report_timing -to_clock PCH_SPI_CLK -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||SPI CLK output paths : Setup} -setup
	report_timing -to_clock PCH_SPI_CLK -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||SPI CLK output paths : Hold} -hold
	report_timing -from_clock PCH_SPI_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||SPI CLK input paths : Setup} -setup
	report_timing -from_clock PCH_SPI_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||SPI CLK input paths : Hold} -hold

	report_timing -to_clock PCH_SPI_CLK_virt -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||PCH_SPI_CLK_virt output paths : Setup} -setup
	report_timing -to_clock PCH_SPI_CLK_virt -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||PCH_SPI_CLK_virt output paths : Hold} -hold
	report_timing -from_clock PCH_SPI_CLK_virt -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||PCH_SPI_CLK_virt input paths : Setup} -setup
	report_timing -from_clock PCH_SPI_CLK_virt -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||PCH_SPI_CLK_virt input paths : Hold} -hold

	report_timing -to_clock PCH_SPI_MON_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||Monitored CLK input paths : Setup} -setup
	report_timing -to_clock PCH_SPI_MON_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||PCH SPI||Monitored CLK input paths : Hold} -hold

	report_timing -to_clock { PCH_SPI_CLK_virt } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||PCH SPI||To PCH_SPI_CLK_virt : Setup} -multi_corner
	report_timing -from_clock { PCH_SPI_CLK_virt } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||PCH SPI||From PCH_SPI_CLK_virt : Setup} -multi_corner
	report_timing -to_clock { PCH_SPI_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||PCH SPI||To PCH_SPI_CLK : Setup} -multi_corner
	report_timing -from_clock { PCH_SPI_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||PCH SPI||From PCH_SPI_CLK : Setup} -multi_corner
	report_timing -to_clock { PCH_SPI_MON_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||PCH SPI||To PCH_SPI_MON_CLK : Setup} -multi_corner
	report_timing -from_clock { PCH_SPI_MON_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||PCH SPI||From PCH_SPI_MON_CLK : Setup} -multi_corner

	post_message -type info "PCH CS ($spi_pin_names_map(PCH_SPI_CS_IN)) to Secure CS ($spi_pin_names_map(PCH_SPI_CS_OUT)) Tpd : $T_SPI_PCH_CS_N_to_SPI_PCH_SECURE_CS_N ns"

}

proc report_bmc_spi_timing {} {
	global spi_pin_names_map

	# Report paths
	set T_SPI_BMC_CS_N_to_SPI_BMC_SECURE_CS_N [lindex [report_path -from [get_ports $spi_pin_names_map(BMC_SPI_CS_IN)] -to [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)]] 1]

	report_timing -from [get_ports $spi_pin_names_map(BMC_SPI_CS_IN)] -to [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||CS to Secure CS : Setup} -setup
	report_timing -from [get_ports $spi_pin_names_map(BMC_SPI_CS_IN)] -to [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||CS to Secure CS : Setup} -setup
	report_timing -from [get_ports $spi_pin_names_map(BMC_SPI_CS_IN)] -to [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||CS to Secure CS : Hold} -hold

	report_timing -to_clock BMC_SPI_CLK -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||SPI CLK output paths : Setup} -setup
	report_timing -to_clock BMC_SPI_CLK -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||SPI CLK output paths : Hold} -hold
	report_timing -from_clock BMC_SPI_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||SPI CLK input paths : Setup} -setup
	report_timing -from_clock BMC_SPI_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||SPI CLK input paths : Hold} -hold

	report_timing -to_clock BMC_SPI_CLK_virt -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||BMC_SPI_CLK_virt output paths : Setup} -setup
	report_timing -to_clock BMC_SPI_CLK_virt -to [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||BMC_SPI_CLK_virt output paths : Hold} -hold
	report_timing -from_clock BMC_SPI_CLK_virt -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||BMC_SPI_CLK_virt input paths : Setup} -setup
	report_timing -from_clock BMC_SPI_CLK_virt -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||BMC_SPI_CLK_virt input paths : Hold} -hold

	report_timing -to_clock BMC_SPI_MON_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||Monitored CLK input paths : Setup} -setup
	report_timing -to_clock BMC_SPI_MON_CLK -from [get_ports] -npath 100 -nworst 10 -show_routing -detail full_path -panel {PFR||BMC SPI||Monitored CLK input paths : Hold} -hold

	report_timing -to_clock { BMC_SPI_CLK_virt } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||BMC SPI||To BMC_SPI_CLK_virt : Setup} -multi_corner
	report_timing -from_clock { BMC_SPI_CLK_virt } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||BMC SPI||From BMC_SPI_CLK_virt : Setup} -multi_corner
	report_timing -to_clock { BMC_SPI_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||BMC SPI||To BMC_SPI_CLK : Setup} -multi_corner
	report_timing -from_clock { BMC_SPI_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||BMC SPI||From BMC_SPI_CLK : Setup} -multi_corner
	report_timing -to_clock { BMC_SPI_MON_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||BMC SPI||To BMC_SPI_MON_CLK : Setup} -multi_corner
	report_timing -from_clock { BMC_SPI_MON_CLK } -setup -npaths 100 -detail full_path -show_routing -panel_name {PFR||BMC SPI||From BMC_SPI_MON_CLK : Setup} -multi_corner

	post_message -type info "BMC CS to Secure CS (Tpd) delay: $T_SPI_BMC_CS_N_to_SPI_BMC_SECURE_CS_N ns"
}

proc report_pfr_timing {} {
	report_spi_names
	report_pch_spi_timing
	report_bmc_spi_timing
}

#**************************************************************
# Create Clock
#**************************************************************
create_clock -name {CLK_25M_OSC_MAIN_FPGA} -period 40.000 [get_ports {CLK_25M_OSC_MAIN_FPGA}]
create_clock -name {SGPIO_BMC_CLK} -period 100 [get_ports {SGPIO_BMC_CLK}]
create_clock -name {SGPIO_DEBUG_PLD_CLK} -period 100 [get_ports {SGPIO_DEBUG_PLD_CLK}]

# Derive PLL clocks
derive_pll_clocks

# Create aliases for the PLL clocks
set CLK2M u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[2]
global SYSCLK
set SYSCLK u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1]
set CLK50M u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[0]
global SPICLK
set SPICLK u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[3]

#**************************************************************
# Create Generated Clock
#**************************************************************

# Missing constraints from IPs                                                                           
#https://www.intel.com/content/www/us/en/programmable/support/support-resources/knowledge-base/tools/2017/why-do-we-see-unconstrained-clock--alteradualboot-dualboot0-altd.html
create_generated_clock -name {ru_clk} -source $SYSCLK -divide_by 2 -master_clock $SYSCLK [get_registers {u_core|u_pfr_sys|u_dual_config|alt_dual_boot_avmm_comp|alt_dual_boot|ru_clk}]

#https://www.intel.com/content/www/us/en/programmable/support/support-resources/knowledge-base/tools/2016/warning--332060---node---alteraonchipflash-onchipflash-alteraonc.html
create_generated_clock -name flash_se_neg_reg -divide_by 2 -source [get_pins { u_core|u_pfr_sys|u_ufm|avmm_data_controller|flash_se_neg_reg|clk }] [get_pins { u_core|u_pfr_sys|u_ufm|avmm_data_controller|flash_se_neg_reg|q } ]

# Create the clock for the SPI master IP
create_generated_clock -name spi_master_clk -source $SPICLK -divide_by 2 [get_pins {u_core|u_spi_control|spi_master_inst|qspi_inf_inst|flash_clk_reg|q}]

# Create generated clocks from common core logic. The maximum period value is at 2147483ns so for Ena125mSec|divclk we can only put in -divide_by 2 follow by -divide_by 2 in rFilteredSignals[0]
create_generated_clock -name Archer_City_Main_wrapper:u_common_core|Archer_City_Main:mArcher_City_Main|ClkDiv:ClkLC|div_clk -source $CLK2M -divide_by 4 -multiply_by 1 Archer_City_Main_wrapper:u_common_core|Archer_City_Main:mArcher_City_Main|ClkDiv:ClkLC|div_clk
create_generated_clock -name Archer_City_Main_wrapper:u_common_core|Archer_City_Main:mArcher_City_Main|ClkDiv:Ena125mSec|div_clk -source $CLK2M -divide_by 2 -multiply_by 1 Archer_City_Main_wrapper:u_common_core|Archer_City_Main:mArcher_City_Main|ClkDiv:Ena125mSec|div_clk
create_generated_clock -name Archer_City_Main_wrapper:u_common_core|Archer_City_Main:mArcher_City_Main|GlitchFilter:mGlitchFilter_PLD_BUTTON|rFilteredSignals[0] -source $CLK2M -divide_by 1 -multiply_by 1 Archer_City_Main_wrapper:u_common_core|Archer_City_Main:mArcher_City_Main|GlitchFilter:mGlitchFilter_PLD_BUTTON|rFilteredSignals[0]


# Constrain the clock crossing
constrain_alt_handshake_clock_crosser_skew *:u_core|*:u_pfr_sys|*_mm_interconnect_0:mm_interconnect_0|altera_avalon_st_handshake_clock_crosser:crosser

#**************************************************************
# Set Clock Latency
#**************************************************************



#**************************************************************
# Set Clock Uncertainty
#**************************************************************
derive_clock_uncertainty

#**************************************************************
# Set Clock Groups
#**************************************************************
# Set all PLL clocks as exclusive from the SGPIO_BMC_CLK clock
set_clock_groups -logically_exclusive -group [get_clocks [list \
	$CLK2M \
	$CLK50M \
	$SYSCLK \
	$SPICLK
]] -group [get_clocks { \
	SGPIO_BMC_CLK \
    SGPIO_DEBUG_PLD_CLK \
}]  

#**************************************************************
# False paths on asynchronous IOs
#**************************************************************
set false_path_input_port_names [list \
	FM_ADR_COMPLETE \
	FM_BMC_CRASHLOG_TRIG_N \
	FM_BMC_INTR_PRSNT \
	FM_BMC_PWRBTN_OUT_N \
	FM_CPU0_INTR_PRSNT \
	FM_CPU0_PKGID0 \
	FM_CPU0_PKGID1 \
	FM_CPU0_PKGID2 \
	FM_CPU0_PROC_ID0 \
	FM_CPU0_PROC_ID1 \
	FM_CPU0_SKTOCC_LVT3_PLD_N \
	FM_CPU1_PKGID0 \
	FM_CPU1_PKGID1 \
	FM_CPU1_PKGID2 \
	FM_CPU1_PROC_ID0 \
	FM_CPU1_PROC_ID1 \
	FM_CPU1_SKTOCC_LVT3_PLD_N \
	FM_DIMM_12V_CPS_S5_N \
	FM_FORCE_PWRON_LVC3 \
	FM_IBL_SLPS0_CPU0_N \
	FM_PCH_PLD_GLB_RST_WARN_N \
	FM_PLD_REV_N \
	FM_PMBUS_ALERT_B_EN \
	FM_RST_PERST_BIT0 \
	FM_RST_PERST_BIT1 \
	FM_RST_PERST_BIT2 \
	FM_SLPS3_PLD_N \
	FM_SLPS4_PLD_N \
	FM_SLP_SUS_RSM_RST_N \
	FM_THROTTLE_R_N \
	FP_ID_LED_N \
	FP_LED_STATUS_AMBER_N \
	FP_LED_STATUS_GREEN_N \
	H_CPU0_MEMTRIP_LVC1_N \
	H_CPU0_MON_FAIL_PLD_LVC1_N \
	H_CPU0_THERMTRIP_LVC1_N \
	H_CPU1_MEMTRIP_LVC1_N \
	H_CPU1_MON_FAIL_PLD_LVC1_N \
	H_CPU1_THERMTRIP_LVC1_N \
	H_CPU_CATERR_LVC1_R_N \
	H_CPU_ERR0_LVC1_R_N \
	H_CPU_ERR1_LVC1_R_N \
	H_CPU_ERR2_LVC1_R_N \
	H_CPU_RMCA_LVC1_R_N \
	IRQ_BMC_CPU_NMI \
	IRQ_CPU0_MEM_VRHOT_N \
	IRQ_CPU0_VRHOT_N \
	IRQ_CPU1_MEM_VRHOT_N \
	IRQ_CPU1_VRHOT_N \
	IRQ_PCH_CPU_NMI_EVENT_N \
	IRQ_PSYS_CRIT_N \
	IRQ_SML1_PMBUS_PLD_ALERT_N \
	M_AB_CPU0_RESET_N \
	M_AB_CPU1_RESET_N \
	M_CD_CPU0_RESET_N \
	M_CD_CPU1_RESET_N \
	M_EF_CPU0_RESET_N \
	M_EF_CPU1_RESET_N \
	M_GH_CPU0_RESET_N \
	M_GH_CPU1_RESET_N \
	PWRGD_CPUPWRGD_LVC1 \
	PWRGD_FAIL_CPU0_AB_PLD \
	PWRGD_FAIL_CPU0_CD_PLD \
	PWRGD_FAIL_CPU0_EF_PLD \
	PWRGD_FAIL_CPU0_GH_PLD \
	PWRGD_FAIL_CPU1_AB_PLD \
	PWRGD_FAIL_CPU1_CD_PLD \
	PWRGD_FAIL_CPU1_EF_PLD \
	PWRGD_FAIL_CPU1_GH_PLD \
	PWRGD_P1V0_BMC_AUX \
	PWRGD_P1V2_BMC_AUX \
	PWRGD_P1V2_MAX10_AUX_PLD_R \
	PWRGD_P1V05_PCH_AUX \
	PWRGD_P1V8_PCH_AUX_PLD \
	PWRGD_P2V5_BMC_AUX \
	PWRGD_P3V3 \
	PWRGD_PS_PWROK_PLD_R \
	PWRGD_PVCCD_HV_CPU0 \
	PWRGD_PVCCD_HV_CPU1 \
	PWRGD_PVCCFA_EHV_CPU0 \
	PWRGD_PVCCFA_EHV_CPU1 \
	PWRGD_PVCCFA_EHV_FIVRA_CPU0 \
	PWRGD_PVCCFA_EHV_FIVRA_CPU1 \
	PWRGD_PVCCINFAON_CPU0 \
	PWRGD_PVCCINFAON_CPU1 \
	PWRGD_PVCCIN_CPU0 \
	PWRGD_PVCCIN_CPU1 \
	PWRGD_PVNN_MAIN_CPU0 \
	PWRGD_PVNN_MAIN_CPU1 \
	PWRGD_PVNN_PCH_AUX \
	PWRGD_PVPP_HBM_CPU0 \
	PWRGD_PVPP_HBM_CPU1 \
	RST_DEDI_BUSY_PLD_N \
	RST_PLTRST_PLD_N \
	SGPIO_BMC_DOUT \
	SGPIO_BMC_LD_N \
]
set_false_path -from [get_ports $false_path_input_port_names]
foreach key [lsort [array names spi_pin_names_map]] {
	if {[lsearch -exact $false_path_input_port_names $spi_pin_names_map($key)] != -1} {
		post_message -type error "SPI Pin $spi_pin_names_map($key) ($key) found in input false patch list"
	}
}


set false_path_output_port_names [list \
	FM_ADR_ACK_R \
	FM_ADR_TRIGGER_N \
	FM_AUX_SW_EN \
	FM_BMC_PFR_PWRBTN_OUT_R_N \
	FM_CPU0_DIMM_CH1_4_FAULT_LED_SEL \
	FM_CPU0_DIMM_CH5_8_FAULT_LED_SEL \
	FM_CPU1_DIMM_CH1_4_FAULT_LED_SEL \
	FM_CPU1_DIMM_CH5_8_FAULT_LED_SEL \
	FM_CPU_ERR0_LVT3_N \
	FM_CPU_ERR1_LVT3_N \
	FM_CPU_ERR2_LVT3_N \
	FM_CPU_RMCA_CATERR_DLY_LVT3_R_N \
	FM_CPU_RMCA_CATERR_LVT3_R_N \
	FM_FAN_FAULT_LED_SEL_R_N \
	FM_P1V0_BMC_AUX_EN \
	FM_P1V2_BMC_AUX_EN \
	FM_P1V05_PCH_AUX_EN \
	FM_P2V5_BMC_AUX_EN \
	FM_P5V_EN \
	FM_PCH_CRASHLOG_TRIG_R_N \
	FM_PCH_P1V8_AUX_EN \
	FM_PFR_DSW_PWROK_N \
	FM_PFR_MUX_OE_CTL_PLD \
	FM_PFR_ON_R \
	FM_PFR_SLP_SUS_EN_R_N \
	FM_PLD_CLKS_DEV_R_EN \
	FM_PLD_CLK_25M_R_EN \
	FM_PLD_HEARTBEAT_LVC3 \
	FM_POSTLED_SEL \
	FM_POST_7SEG1_SEL_N \
	FM_POST_7SEG2_SEL_N \
	FM_PS_EN_PLD_R \
	FM_PVCCD_HV_CPU0_EN \
	FM_PVCCD_HV_CPU1_EN \
	FM_PVCCFA_EHV_CPU0_R_EN \
	FM_PVCCFA_EHV_CPU1_R_EN \
	FM_PVCCFA_EHV_FIVRA_CPU0_R_EN \
	FM_PVCCFA_EHV_FIVRA_CPU1_R_EN \
	FM_PVCCINFAON_CPU0_R_EN \
	FM_PVCCINFAON_CPU1_R_EN \
	FM_PVCCIN_CPU0_R_EN \
	FM_PVCCIN_CPU1_R_EN \
	FM_PVNN_MAIN_CPU0_EN \
	FM_PVNN_MAIN_CPU1_EN \
	FM_PVNN_PCH_AUX_EN \
	FM_PVPP_HBM_CPU0_EN \
	FM_PVPP_HBM_CPU1_EN \
	FM_SPI_PFR_BMC_BT_MASTER_SEL_R \
	FM_SPI_PFR_PCH_MASTER_SEL_R \
	FM_SX_SW_P12V_R_EN \
	FM_SX_SW_P12V_STBY_R_EN \
	FM_SYS_THROTTLE_R_N \
	FM_THERMTRIP_DLY_LVC1_R_N \
	FP_ID_LED_PFR_N \
	FP_LED_STATUS_AMBER_PFR_N \
	FP_LED_STATUS_GREEN_PFR_N \
	H_CPU0_MEMHOT_IN_LVC1_R_N \
	H_CPU0_PROCHOT_LVC1_R_N \
	H_CPU1_MEMHOT_IN_LVC1_R_N \
	H_CPU1_PROCHOT_LVC1_R_N \
	H_CPU_NMI_LVC1_R \
	LED_CONTROL_0 \
	LED_CONTROL_1 \
	LED_CONTROL_2 \
	LED_CONTROL_3 \
	LED_CONTROL_4 \
	LED_CONTROL_5 \
	LED_CONTROL_6 \
	LED_CONTROL_7 \
	M_AB_CPU0_FPGA_RESET_R_N \
	M_AB_CPU1_FPGA_RESET_R_N \
	M_CD_CPU0_FPGA_RESET_R_N \
	M_CD_CPU1_FPGA_RESET_R_N \
	M_EF_CPU0_FPGA_RESET_R_N \
	M_EF_CPU1_FPGA_RESET_R_N \
	M_GH_CPU0_FPGA_RESET_R_N \
	M_GH_CPU1_FPGA_RESET_R_N \
	PWRGD_CPU0_LVC1_R \
	PWRGD_CPU1_LVC1_R \
	PWRGD_DRAMPWRGD_CPU0_AB_R_LVC1 \
	PWRGD_DRAMPWRGD_CPU0_CD_R_LVC1 \
	PWRGD_DRAMPWRGD_CPU0_EF_R_LVC1 \
	PWRGD_DRAMPWRGD_CPU0_GH_R_LVC1 \
	PWRGD_DRAMPWRGD_CPU1_AB_R_LVC1 \
	PWRGD_DRAMPWRGD_CPU1_CD_R_LVC1 \
	PWRGD_DRAMPWRGD_CPU1_EF_R_LVC1 \
	PWRGD_DRAMPWRGD_CPU1_GH_R_LVC1 \
	PWRGD_FAIL_CPU0_AB_PLD \
	PWRGD_FAIL_CPU0_CD_PLD \
	PWRGD_FAIL_CPU0_EF_PLD \
	PWRGD_FAIL_CPU0_GH_PLD \
	PWRGD_FAIL_CPU1_AB_PLD \
	PWRGD_FAIL_CPU1_CD_PLD \
	PWRGD_FAIL_CPU1_EF_PLD \
	PWRGD_FAIL_CPU1_GH_PLD \
	PWRGD_PCH_PWROK_R \
	PWRGD_PLT_AUX_CPU0_LVT3_R \
	PWRGD_PLT_AUX_CPU1_LVT3_R \
	PWRGD_SYS_PWROK_R \
	RST_CPU0_LVC1_R_N \
	RST_CPU1_LVC1_R_N \
	RST_PLD_PCIE_CPU0_DEV_PERST_N \
	RST_PLD_PCIE_CPU1_DEV_PERST_N \
	RST_PLD_PCIE_PCH_DEV_PERST_N \
	RST_PFR_EXTRST_R_N \
	RST_PFR_OVR_RTC_R \
	RST_PFR_OVR_SRTC_R \
	RST_PLTRST_PLD_B_N \
	RST_RSMRST_PLD_R_N \
	RST_SPI_PFR_BMC_BOOT_N \
	RST_SPI_PFR_PCH_N \
	RST_SRST_BMC_PLD_R_N \
	SGPIO_BMC_DIN_R \
]
set_false_path -to [get_ports $false_path_output_port_names]
foreach key [lsort [array names spi_pin_names_map]] {
	if {[lsearch -exact $false_path_output_port_names $spi_pin_names_map($key)] != -1} {
		post_message -type error "SPI Pin $spi_pin_names_map($key) ($key) found in output false patch list"
	}
}

#****************************************************************
# SMBus Mailbox
#**************************************************************

# Use the set_max_skew constraint to perform maximum allowable skew (<1.5ns) analysis between SCL and SDA pins for input path.
# This is to set the max_skew from the SDA/SCL ports to the first register
set_max_skew -to_clock {u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1]} -from [get_ports {SMB_S3M_CPU0_SCL_LVC1 SMB_S3M_CPU0_SDA_LVC1}] 1.5
set_max_skew -to_clock {u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1]} -from [get_ports {SMB_PCIE_STBY_LVC3_B_SCL SMB_PCIE_STBY_LVC3_B_SDA}] 1.5

# This is to set the max_skew from the SDA/SCL ports to the first register
set_max_skew -from_clock {u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1]} -to [get_ports {SMB_PCIE_STBY_LVC3_B_SCL SMB_PCIE_STBY_LVC3_B_SDA}] 1.5

# Use set_max_delay to constrain the output path of SMB_PFR_DDRABCD_CPU1_LVC2_SDA since it is a INOUT port
# Simply choose a 'reasonable' delay (5 ns) to prevent Quartus from adding excessive routing delay to this signal
# set_max_delay -to [get_ports {SMB_S3M_CPU0_SDA_LVC1}] 5

#****************************************************************
# RFNVRAM
#**************************************************************

# Use the set_max_skew constraint to perform maximum allowable skew (<3ns) analysis between SCL and SDA pins for output path
# This is to set the max_skew from the first register to the SDA/SCL ports
set_max_skew -from_clock {u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1]} -to [get_ports {SMB_PFR_RFID_STBY_LVC3_SDA SMB_PFR_RFID_STBY_LVC3_SCL}] 2

# This is to set the max_skew from the SDA/SCL ports to the first register
set_max_skew -to_clock {u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1]} -from [get_ports {SMB_PFR_RFID_STBY_LVC3_SDA SMB_PFR_RFID_STBY_LVC3_SCL}] 3


#****************************************************************
# BMC/PCH SMBus RELAY 
#**************************************************************
# Use the set_max_skew constraint to perform maximum allowable skew (<1.5ns) analysis between SCL and SDA pins.
# In order to constrain skew across multiple paths, all such paths must be defined within a single set_max_skew constraint.

# This is to set the max_skew from the first register to the SDA/SCL ports
set_max_skew -from_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -to [get_ports {SMB_BMC_HSBP_STBY_LVC3_SDA   SMB_BMC_HSBP_STBY_LVC3_SCL}] 1.5
set_max_skew -from_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -to [get_ports {SMB_PMBUS_SML1_STBY_LVC3_SDA SMB_PMBUS_SML1_STBY_LVC3_SCL}] 1.5
set_max_skew -from_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -to [get_ports {SMB_PFR_PMB1_STBY_LVC3_SDA   SMB_PFR_PMB1_STBY_LVC3_SCL}] 1.5
set_max_skew -from_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -to [get_ports {SMB_PCH_PMBUS2_STBY_LVC3_SDA SMB_PCH_PMBUS2_STBY_LVC3_SCL}] 1.5
set_max_skew -from_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -to [get_ports {SMB_PFR_PMBUS2_STBY_LVC3_R_SDA SMB_PFR_PMBUS2_STBY_LVC3_R_SCL}] 1.5
set_max_skew -from_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -to [get_ports {SMB_PFR_HSBP_STBY_LVC3_SDA   SMB_PFR_HSBP_STBY_LVC3_SCL}] 1.5

# This is to set the max_skew from the SDA/SCL ports to the first register
set_max_skew -to_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -from [get_ports {SMB_BMC_HSBP_STBY_LVC3_SDA   SMB_BMC_HSBP_STBY_LVC3_SCL}] 1.5
set_max_skew -to_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -from [get_ports {SMB_PMBUS_SML1_STBY_LVC3_SDA SMB_PMBUS_SML1_STBY_LVC3_SCL}] 1.5
set_max_skew -to_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -from [get_ports {SMB_PFR_PMB1_STBY_LVC3_SDA   SMB_PFR_PMB1_STBY_LVC3_SCL}] 1.5
set_max_skew -to_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -from [get_ports {SMB_PCH_PMBUS2_STBY_LVC3_SDA SMB_PCH_PMBUS2_STBY_LVC3_SCL}] 1.5
set_max_skew -to_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -from [get_ports {SMB_PFR_PMBUS2_STBY_LVC3_R_SDA SMB_PFR_PMBUS2_STBY_LVC3_R_SCL}] 1.5
set_max_skew -to_clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } -from [get_ports {SMB_PFR_HSBP_STBY_LVC3_SDA   SMB_PFR_HSBP_STBY_LVC3_SCL}] 1.5



# Report the SPI pin map names
report_spi_names

#**************************************************************
# BMC/PCH SPI Filter and SPI Master
#**************************************************************
proc set_pch_spi_interface_constraints {} {
	global SPICLK
	global spi_pin_names_map

	post_message -type info "Creating PCH SPI interface constraints"
	post_message -type info [string repeat # 80]

	# SPI Mux properties from datasheet
	set Tmux_prop_min 0.2
	set Tmux_prop_max 0.2

	# PCH SPI Interface numbers
	# Use 40 MHz SPI CLK
	# TCO numbers from from PCH Datasheet 
	set SPI_PCH_SPI_FLASH_CLK_PERIOD 25.000
	set T_PCH_SPI_DQ_TCO_MIN -3.5000
	set T_PCH_SPI_DQ_TCO_MAX 5.000
	set T_PCH_SPI_CS_TCO_MIN 5.000
	set T_PCH_SPI_CS_TCO_MAX 5.000

	# PCH Flash device properties from datasheet
	set T_SPI_PCH_SPI_FLASH_DQ_S 1.75
	set T_SPI_PCH_SPI_FLASH_DQ_H 2.3
	set T_SPI_PCH_SPI_FLASH_CS_S 3.375
	set T_SPI_PCH_SPI_FLASH_CS_H 3.375
	set T_SPI_PCH_SPI_FLASH_DQ_TCO 6.0

	#######################################################################################################
	# PCH Flash board delays
	#######################################################################################################
	# CPLD -> SPI_PFR_PCH_R_CLK mux
	# Path: U31.B16 (SPI_PFR_PCH_R_CLK) to U5P1.13 (SPI_PFR_PCH_CLK)
	# U31.B16 -> (304.98) -> R6D10.1 -> (0) -> R6D10.2 -> (5067.2) -> U5P1.13
	# Total path length: 5372.18
	set T_CPLD_SPI_PCH_CLK_to_MUX [calc_prop_delay [expr {304.98 + 5067.2}]]

	# CPLD -> SPI_PFR_PCH_SECURE_CS0_N mux
	# Path: U31.E20 (SPI_PFR_PCH_SECURE_CS0_R_N) to U5P2.5 (SPI_PFR_PCH_SECURE_CS0_N)
	# U31.E20 -> (299.14) -> R5P20.1 -> (0) -> R5P20.2 -> (6529.6) -> R5P28.1 -> (112.22) -> R5P27.2 -> (240.47) -> U5P2.6 -> (120.27) -> U5P2.5
	# Total path length: 7301.7
	set T_CPLD_SPI_PCH_CS_N_OUT_to_MUX [calc_prop_delay [expr {299.14 + 6529.6 + 112.22 + 240.47 + 120.27}]]

	# CPLD -> SPI_PFR_PCH_R_IO0 mux
	# Path: U31.B17 (SPI_PFR_PCH_R_IO0) to U5P1.3 (SPI_PFR_PCH_IO0)
	# U31.B17 -> (301.55) -> R6D14.1 -> (0) -> R6D14.2 -> (5528.92) -> U5P1.3
	# Total path length: 5830.47
	set T_CPLD_SPI_PCH_IO0_to_MUX [calc_prop_delay [expr {301.55 + 5528.92}]]

	# CPLD -> SPI_PFR_PCH_R_IO1
	# Path: U31.C15 (SPI_PFR_PCH_R_IO1) to U5P1.6 (SPI_PFR_PCH_IO1)
	# U31.C15 -> (399.19) -> R6D17.1 -> (0) -> R6D17.2 -> (5099.46) -> U5P1.6
	# Total path length: 5498.65
	set T_CPLD_SPI_PCH_IO1_to_MUX [calc_prop_delay [expr {399.19 + 5099.46}]]

	# CPLD -> SPI_PFR_PCH_R_IO2 mux
	# Path: U31.C16 (SPI_PFR_PCH_R_IO2) to U5P1.10 (SPI_PFR_PCH_IO2)
	# U31.C16 -> (361.96) -> R6D19.1 -> (0) -> R6D19.2 -> (4815.68) -> U5P1.10
	# Total path length: 5177.64
	set T_CPLD_SPI_PCH_IO2_to_MUX [calc_prop_delay [expr {361.96 + 4815.68}]]

	# CPLD -> SPI_PFR_PCH_R_IO3 mux
	# Path: U31.C17 (SPI_PFR_PCH_R_IO3) to U5P2.3 (SPI_PFR_PCH_IO3)
	# U31.C17 -> (344.09) -> R6D22.1 -> (0) -> R6D22.2 -> (5541.04) -> U5P2.3
	# Total path length: 5885.13
	set T_CPLD_SPI_PCH_IO3_to_MUX [calc_prop_delay [expr {344.09 + 5541.04}]]

	# mux -> SPI_PCH_CLK_FLASH_R1 flash
	# Path: U5P1.12 (SPI_PCH_MUXED_CLK) to XU5D1.16 (SPI_PCH_CLK_FLASH_R1)
	# U5P1.12 -> (178.9) -> R4P3.1 -> (768.32) -> R5D38.1 -> (474.59) -> R5P19.1 -> (2409.57) -> R6D35.1 -> (0) -> R6D35.2 -> (231.93) -> XU5D1.16
	# Total path length: 4063.31
	set Tmux_to_PCH_SPI_FLASH_CLK [calc_prop_delay [expr {178.9 + 768.32 + 474.59 + 2409.57 + 231.93}]]

	# mux -> SPI_PCH_CS_FLASH_R1 flash
	# Path: U5P2.7 (SPI_PCH_MUXED_CS0_N) to XU5D1.7 (SPI_PCH_CS0_FLASH_R1_N)
	# U5P2.7 -> (97.65) -> R5P27.1 -> (610.73) -> R5P12.1 -> (0) -> R5P12.2 -> (293.98) -> XU5D1.7
	# Total path length: 1002.36
	set Tmux_to_PCH_SPI_FLASH_CS [calc_prop_delay [expr {97.65 + 610.73 + 293.98}]]

	# mux -> SPI_PCH_IO0_FLASH_R1 flash
	# Path: U5P1.4 (SPI_PCH_MUXED_IO0) to XU5D1.15 (SPI_PCH_IO0_FLASH_R1)
	# U5P1.4 -> (224.43) -> R5P8.1 -> (978.92) -> R5D36.1 -> (458.26) -> R5P18.1 -> (2324.41) -> R6D34.1 -> (0) -> R6D34.2 -> (211.8) -> XU5D1.15
	# Total path length: 4197.82
	set Tmux_to_PCH_SPI_FLASH_DQ0 [calc_prop_delay [expr {224.43 + 978.92 + 458.26 + 2324.41 + 211.8}]]

	# mux -> SPI_PCH_IO1_FLASH_R1 flash
	# Path: U5P1.7 (SPI_PCH_MUXED_IO1) to XU5D1.8 (SPI_PCH_IO1_FLASH_R1)
	# U5P1.7 -> (138.46) -> R5P5.1 -> (1894.18) -> R5D34.2 -> (1409.7) -> R5P11.1 -> (0) -> R5P11.2 -> (285.7) -> XU5D1.8
	# Total path length: 3728.04
	set Tmux_to_PCH_SPI_FLASH_DQ1 [calc_prop_delay [expr {138.46 + 1894.18 + 1409.7 + 285.7}]]

	# mux -> SPI_PCH_IO2_FLASH_R1 flash
	# Path: U5P1.9 (SPI_PCH_MUXED_IO2) to XU5D1.9 (SPI_PCH_IO2_FLASH_R1)
	# U5P1.9 -> (293.9) -> R4P1.1 -> (2101.72) -> R4P7.1 -> (0) -> R4P7.2 -> (211.8) -> XU5D1.9
	# Total path length: 2607.42
	set Tmux_to_PCH_SPI_FLASH_DQ2 [calc_prop_delay [expr {293.9 + 2101.72 + 211.8}]]

	# mux -> SPI_PCH_IO3_FLASH_R1 flash
	# Path: U5P2.4 (SPI_PCH_MUXED_IO3) to XU5D1.1 (SPI_PCH_IO3_FLASH_R1)
	# U5P2.4 -> (100.46) -> R5P31.1 -> (3274.31) -> R5D6.1 -> (0) -> R5D6.2 -> (304.17) -> XU5D1.1
	# Total path length: 3678.94
	set Tmux_to_PCH_SPI_FLASH_DQ3 [calc_prop_delay [expr {100.46 + 3274.31 + 304.17}]]

################


	# mux -> SPI_PCH_BMC_SAFS_MUXED_MON_CLK cpld pin
	# Path: U5P1.12 (SPI_PCH_MUXED_CLK) to U31.C14 (SPI_PCH_MUXED_MON_CLK)
	# U5P1.12 -> (178.9) -> R4P3.1 -> (768.32) -> R5D38.1 -> (474.59) -> R5P19.1 -> (2409.57) -> R6D35.1 -> (761.3) -> R6D83.1 -> (5683.47) -> R3473.1 -> (0) -> R3473.2 -> (610.39) -> U31.C14
	# Total path length: 10886.54
	set Tmux_to_CPLD_SPI_PCH_CLK_MON [calc_prop_delay [expr {178.9 + 768.32 + 474.59 + 2409.57 + 761.3 + 5683.47 + 610.39}]]

	# mux -> SPI_PCH_BMC_SAFS_MUXED_MON_IO0 cpld pin
	# Path: U5P1.4 (SPI_PCH_MUXED_IO0) to U31.C13 (SPI_PCH_MUXED_MON_IO0)
	# U5P1.4 -> (224.43) -> R5P8.1 -> (978.92) -> R5D36.1 -> (458.26) -> R5P18.1 -> (2324.41) -> R6D34.1 -> (1301.36) -> R6D87.1 -> (6134.65) -> R3474.1 -> (0) -> R3474.2 -> (636.91) -> U31.C13
	# Total path length: 12058.94
	set Tmux_to_CPLD_SPI_PCH_DQ0_MON [calc_prop_delay [expr {224.43 + 978.92 + 458.26 + 2324.41 + 1301.36 + 6134.65 + 636.91}]]

	# mux -> SPI_PCH_BMC_SAFS_MUXED_MON_IO1 cpld pin
	# Path: U5P1.7 (SPI_PCH_MUXED_IO1) to U31.D12 (SPI_PCH_MUXED_MON_IO1)
	# U5P1.7 -> (138.46) -> R5P5.1 -> (1894.18) -> R5D34.2 -> (1409.7) -> R5P11.1 -> (789.68) -> R5D46.1 -> (1584.93) -> R5P17.1 -> (8157.94) -> R3475.1 -> (0) -> R3475.2 -> (701.33) -> U31.D12
	# Total path length: 14676.22
	set Tmux_to_CPLD_SPI_PCH_DQ1_MON [calc_prop_delay [expr {138.46 + 1894.18 + 1409.7 + 789.68 + 1584.93 + 8157.94 + 701.33}]]

	# mux -> SPI_PCH_BMC_SAFS_MUXED_MON_IO2 cpld pin
	# Path: U5P1.9 (SPI_PCH_MUXED_IO2) to U31.D13 (SPI_PCH_MUXED_MON_IO2)
	# U5P1.9 -> (293.9) -> R4P1.1 -> (2101.72) -> R4P7.1 -> (1318.73) -> R6D127.1 -> (6298.43) -> R3471.1 -> (0) -> R3471.2 -> (671.95) -> U31.D13
	# Total path length: 10684.73
	set Tmux_to_CPLD_SPI_PCH_DQ2_MON [calc_prop_delay [expr {293.9 + 2101.72 + 1318.73 + 6298.43 + 671.95}]]

	# mux -> SPI_PCH_BMC_SAFS_MUXED_MON_IO3 cpld pin
	# Path: U5P2.4 (SPI_PCH_MUXED_IO3) to U31.D14 (SPI_PCH_MUXED_MON_IO3)
	# U5P2.4 -> (100.46) -> R5P31.1 -> (3274.31) -> R5D6.1 -> (819.06) -> R5D27.1 -> (5599.61) -> R3472.1 -> (0) -> R3472.2 -> (676.48) -> U31.D14
	# Total path length: 10469.92
	set Tmux_to_CPLD_SPI_PCH_DQ3_MON [calc_prop_delay [expr {100.46 + 3274.31 + 819.06 + 5599.61 + 676.48}]]

	# PCH -> SPI_PCH_R_CLK mux
	# Path: U5P1.14 (SPI_PCH_CLK_R) to U8.BB15 (SPI_PCH_CLK)
	# U5P1.14 -> (110.46) -> R4P4.1 -> (0) -> R4P4.2 -> (3147.95) -> U8.BB15
	# Total path length: 3258.41
	set T_PCH_SPI_CLK_to_mux [calc_prop_delay [expr {110.46 + 3147.95}]]

	# PCH -> DQ0 Mux
	# Path: U5P1.2 (SPI_PCH_IO0) to U8.BA13 (SPI_PCH_IO0)
	# U5P1.2 -> (148.76) -> R5P8.2 -> (3184.49) -> U8.BA13
	# Total path length: 3333.25
	set T_PCH_SPI_DQ0_to_mux [calc_prop_delay [expr {148.76 + 3184.49}]]

	# PCH -> DQ1 Mux
	# Path: U5P1.5 (SPI_PCH_IO1) to U8.AW16 (SPI_PCH_IO1)
	# U5P1.5 -> (114.93) -> R5P5.2 -> (3181.71) -> U8.AW16
	# Total path length: 3296.64
	set T_PCH_SPI_DQ1_to_mux [calc_prop_delay [expr {114.93 + 3181.71}]]

	# PCH -> DQ2 Mux
	# Path: U5P1.11 (SPI_PCH_IO2_R) to U8.AV15 (SPI_PCH_IO2)
	# U5P1.11 -> (101.65) -> R4P2.1 -> (0) -> R4P2.2 -> (3153.62) -> U8.AV15
	# Total path length: 3255.27
	set T_PCH_SPI_DQ2_to_mux [calc_prop_delay [expr {101.65 + 3153.62}]]

	# PCH -> DQ3 Mux
	# Path: U5P2.2 (SPI_PCH_IO3_R) to U8.BA16 (SPI_PCH_IO3)
	# U5P2.2 -> (117.9) -> R5P35.1 -> (0) -> R5P35.2 -> (3180.98) -> U8.BA16
	# Total path length: 3298.88
	set T_PCH_SPI_DQ3_to_mux [calc_prop_delay [expr {117.9 + 3180.98}]]

	# PCH -> SPI_PCH_BMC_PFR_CS0_N CPLD
	# Path: U31.D19 (SPI_PCH_PFR_CS0_N) to U8.AW13 (SPI_PCH_CS0_N)
	# U31.D19 -> (10255.21) -> R5P36.2 -> (0) -> R5P36.1 -> (575.91) -> U8.AW13
	# Total path length: 10831.12
	set T_PCH_SPI_CS_to_CPLD_PCH_SPI_CS_IN [calc_prop_delay [expr {10255.21 + 575.91}]]

	#######################################################################################################
	# Compute shorthand variables
	#######################################################################################################
	set T_CPLD_to_PCH_SPI_FLASH_CLK_min [expr {$T_CPLD_SPI_PCH_CLK_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_CLK}]
	set T_CPLD_to_PCH_SPI_FLASH_CLK_max [expr {$T_CPLD_SPI_PCH_CLK_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_CLK}]

	set T_CPLD_to_PCH_SPI_FLASH_CS_min [expr {$T_CPLD_SPI_PCH_CS_N_OUT_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_CS}]
	set T_CPLD_to_PCH_SPI_FLASH_CS_max [expr {$T_CPLD_SPI_PCH_CS_N_OUT_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_CS}]

	set T_PCH_to_PCH_SPI_FLASH_CLK_min [expr {$T_PCH_SPI_CLK_to_mux + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_CLK}]
	set T_PCH_to_PCH_SPI_FLASH_CLK_max [expr {$T_PCH_SPI_CLK_to_mux + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_CLK}]

	set T_PCH_to_CPLD_PCH_SPI_CLK_min [expr {$T_PCH_SPI_CLK_to_mux + $Tmux_prop_min + $Tmux_to_CPLD_SPI_PCH_CLK_MON}]
	set T_PCH_to_CPLD_PCH_SPI_CLK_max [expr {$T_PCH_SPI_CLK_to_mux + $Tmux_prop_max + $Tmux_to_CPLD_SPI_PCH_CLK_MON}]

	set T_PCH_SPI_DQ0_to_CPLD_min [expr {$T_PCH_SPI_DQ0_to_mux + $Tmux_prop_min + $Tmux_to_CPLD_SPI_PCH_DQ0_MON}]
	set T_PCH_SPI_DQ0_to_CPLD_max [expr {$T_PCH_SPI_DQ0_to_mux + $Tmux_prop_max + $Tmux_to_CPLD_SPI_PCH_DQ0_MON}]
	set T_PCH_SPI_DQ1_to_CPLD_min [expr {$T_PCH_SPI_DQ1_to_mux + $Tmux_prop_min + $Tmux_to_CPLD_SPI_PCH_DQ1_MON}]
	set T_PCH_SPI_DQ1_to_CPLD_max [expr {$T_PCH_SPI_DQ1_to_mux + $Tmux_prop_max + $Tmux_to_CPLD_SPI_PCH_DQ1_MON}]
	set T_PCH_SPI_DQ2_to_CPLD_min [expr {$T_PCH_SPI_DQ2_to_mux + $Tmux_prop_min + $Tmux_to_CPLD_SPI_PCH_DQ2_MON}]
	set T_PCH_SPI_DQ2_to_CPLD_max [expr {$T_PCH_SPI_DQ2_to_mux + $Tmux_prop_max + $Tmux_to_CPLD_SPI_PCH_DQ2_MON}]
	set T_PCH_SPI_DQ3_to_CPLD_min [expr {$T_PCH_SPI_DQ3_to_mux + $Tmux_prop_min + $Tmux_to_CPLD_SPI_PCH_DQ3_MON}]
	set T_PCH_SPI_DQ3_to_CPLD_max [expr {$T_PCH_SPI_DQ3_to_mux + $Tmux_prop_max + $Tmux_to_CPLD_SPI_PCH_DQ3_MON}]

	# Create the clock on the pin for the PCH SPI clock. This clock is created in the CPLD
	create_generated_clock -name PCH_SPI_CLK -source [get_clock_info -targets [get_clocks spi_master_clk]] [get_ports $spi_pin_names_map(PCH_SPI_CLK)]

	# SPI input timing. Data is output on falling edge of SPI clock and latched on rising edge of FPGA clock.
	# The latch edge is aligns with the next falling edge of the launch clock. Since the dest clock
	# is 2x the source clock, a multicycle is used. 
	#  
	#   Data Clock : PCH_SPI_CLK
	#       (Launch)
	#         v
	#   +-----+     +-----+     +-----+
	#   |     |     |     |     |     |	
	#   +     +-----+     +-----+     +-----
	#
	#   FPGA Clock : $SPICLK 
	#            (Latch)
	#                     v
	#   +--+  +--+  +--+  +--+  +--+  +--+
	#   |  |  |  |  |  |  |  |  |  |  |  |	
	#   +  +--+  +--+  +--+  +--+  +--+  +--
	#        (H)         (S)          
	# Setup relationship : PCH_SPI_CLK period == 2 * $SPICLK period
	# Hold relationship : 0
	set_multicycle_path -from [get_clocks PCH_SPI_CLK] -to [get_clocks $SPICLK] -setup -end 2
	set_multicycle_path -from [get_clocks PCH_SPI_CLK] -to [get_clocks $SPICLK] -hold -end 1

	# SPI Output multicycle to ensure previous edge used for hold. This is because the SPI output clock is 180deg
	# from the SPI clock
	#  

	#   Data Clock : $SPICLK 
	#            (Launch)
	#               v
	#   +--+  +--+  +--+  +--+  +--+  +--+
	#   |  |  |  |  |  |  |  |  |  |  |  |	
	#   +  +--+  +--+  +--+  +--+  +--+  +--

	#   Data Clock 2X : spi_master_clk
	#            (Launch)
	#               v
	#   +-----+     +-----+     +-----+
	#   |     |     |     |     |     |	
	#   +     +-----+     +-----+     +-----
	#
	#   Output Clock : PCH_SPI_CLK
	#                  (Latch)
	#                     v
	#   +     +-----+     +-----+     +-----+
	#   |     |     |     |     |     |     |	
	#   +-----+     +-----+     +-----+     +-----
	#        (H)         (S)          
	#
	# Setup relationship : PCH_SPI_CLK period / 2
	# Hold relationship : -PCH_SPI_CLK period / 2
	set_multicycle_path -from [get_clocks $SPICLK] -to [get_clocks PCH_SPI_CLK] -hold -start 1


	# SPI CS timing when in filtering mode. Data is output from BMC/PCH on rising edge and latched on rising edge.
	#  
	#   Data Clock : PCH_SPI_CLK_virt
	#          (Launch)
	#             v
	#   +----+    +----+    +----+
	#   |    |    |    |    |    |	
	#   +    +----+    +----+    +----
	#
	#   SPI Clock : PCH_SPI_CLK_virt
	#                    (Latch)
	#                       v
	#   +----+    +----+    +----+
	#   |    |    |    |    |    |	
	#   +    +----+    +----+    +----
	#            (H)       (S)        
	#
	# Setup relationship : SPI_CLK period
	# Hold relationship : 0

	# Create the virtual clock for the PCH SPI clock located in the PCH. It is exclusive from the PLL clock
	create_clock -name PCH_SPI_CLK_virt -period $SPI_PCH_SPI_FLASH_CLK_PERIOD
	set_clock_groups -logically_exclusive -group {PCH_SPI_CLK_virt} -group {PCH_SPI_CLK}
	# Create the monitored clock
	create_clock -name PCH_SPI_MON_CLK -period $SPI_PCH_SPI_FLASH_CLK_PERIOD [get_ports $spi_pin_names_map(PCH_SPI_CLK_MON)]

	#######################################################################################################
	# Create the output constraints for the SPI Master IP to PCH flash
	#######################################################################################################
	# This are classic input/output delays with respect to the SPI clock for all data out and CS
	# Output max delay = max trace delay for data + tsu - min trace delay of clock
	# Output min delay = min trace delay for data - th - max trace delay of clock

	post_message -type info "PCH CLK Period: $SPI_PCH_SPI_FLASH_CLK_PERIOD ns"
	post_message -type info "CPLD PCH CLK to flash delay: [expr {$T_CPLD_to_PCH_SPI_FLASH_CLK_min}] ns"
	post_message -type info "CPLD PCH CS to flash delay: [expr {$T_CPLD_to_PCH_SPI_FLASH_CS_max}] ns"
	post_message -type info "CPLD PCH DQ0 to flash delay: [expr {$T_CPLD_SPI_PCH_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ0}] ns"
	post_message -type info "CPLD PCH DQ1 to flash delay: [expr {$T_CPLD_SPI_PCH_IO1_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ1}] ns"
	post_message -type info "CPLD PCH DQ2 to flash delay: [expr {$T_CPLD_SPI_PCH_IO2_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ2}] ns"
	post_message -type info "CPLD PCH DQ3 to flash delay: [expr {$T_CPLD_SPI_PCH_IO3_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ3}] ns"
	post_message -type info "PCH Flash DQ Setup : $T_SPI_PCH_SPI_FLASH_DQ_S ns"
	post_message -type info "PCH Flash DQ Hold : $T_SPI_PCH_SPI_FLASH_DQ_H ns"
	post_message -type info "PCH Flash CS Setup : $T_SPI_PCH_SPI_FLASH_CS_S ns"
	post_message -type info "PCH Flash CS Hold : $T_SPI_PCH_SPI_FLASH_CS_H ns"

	set_output_delay -max -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ0 + $T_SPI_PCH_SPI_FLASH_DQ_S - $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ0)]
	set_output_delay -min -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO0_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ0 - $T_SPI_PCH_SPI_FLASH_DQ_H - $T_CPLD_to_PCH_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ0)]

	set_output_delay -max -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO1_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ1 + $T_SPI_PCH_SPI_FLASH_DQ_S - $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ1)]
	set_output_delay -min -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO1_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ1 - $T_SPI_PCH_SPI_FLASH_DQ_H - $T_CPLD_to_PCH_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ1)]

	set_output_delay -max -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO2_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ2 + $T_SPI_PCH_SPI_FLASH_DQ_S - $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ2)]
	set_output_delay -min -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO2_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ2 - $T_SPI_PCH_SPI_FLASH_DQ_H - $T_CPLD_to_PCH_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ2)]

	set_output_delay -max -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO3_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ3 + $T_SPI_PCH_SPI_FLASH_DQ_S - $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ3)]
	set_output_delay -min -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO3_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ3 - $T_SPI_PCH_SPI_FLASH_DQ_H - $T_CPLD_to_PCH_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ3)]

	set_output_delay -max -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_to_PCH_SPI_FLASH_CS_max + $T_SPI_PCH_SPI_FLASH_CS_S - $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_CS_OUT)]
	set_output_delay -min -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_to_PCH_SPI_FLASH_CS_min - $T_SPI_PCH_SPI_FLASH_CS_H - $T_CPLD_to_PCH_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_CS_OUT)]
	#######################################################################################################

	#######################################################################################################
	# Create the input constraints for the SPI Master IP from the PCH Flash
	#######################################################################################################
	# This are classic input/output delays with respect to the SPI clock for all Data in.
	# The SPI devices drive the data on the falling edge of the clock
	# Input max delay = max trace delay for data + tco_max + min trace delay for clock
	# Input min delay = max trace delay for data + tco_min + max trace delay for clock
	post_message -type info "PCH Flash DQ Tco : $T_SPI_PCH_SPI_FLASH_DQ_TCO ns"

	post_message -type info "MON DQ0 Delay : [expr {$T_CPLD_SPI_PCH_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ0 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}]"
	post_message -type info "   T_CPLD_SPI_PCH_IO0_to_MUX : $T_CPLD_SPI_PCH_IO0_to_MUX"
	post_message -type info "   Tmux_prop_max : $Tmux_prop_max"
	post_message -type info "   Tmux_to_PCH_SPI_FLASH_DQ0 : $Tmux_to_PCH_SPI_FLASH_DQ0"
	post_message -type info "   T_SPI_PCH_SPI_FLASH_DQ_TCO : $T_SPI_PCH_SPI_FLASH_DQ_TCO"
	post_message -type info "   T_CPLD_to_PCH_SPI_FLASH_CLK_min : $T_CPLD_to_PCH_SPI_FLASH_CLK_min"

	set_input_delay -max -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ0 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ0)]
	set_input_delay -min -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO0_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ0 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ0)]

	set_input_delay -max -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO1_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ1 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ1)]
	set_input_delay -min -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO1_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ1 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ1)]

	set_input_delay -max -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO2_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ2 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ2)]
	set_input_delay -min -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO2_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ2 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ2)]

	set_input_delay -max -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO3_to_MUX + $Tmux_prop_max + $Tmux_to_PCH_SPI_FLASH_DQ3 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ3)]
	set_input_delay -min -clock_fall -clock [get_clocks PCH_SPI_CLK] [expr {$T_CPLD_SPI_PCH_IO3_to_MUX + $Tmux_prop_min + $Tmux_to_PCH_SPI_FLASH_DQ3 + $T_SPI_PCH_SPI_FLASH_DQ_TCO + $T_CPLD_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ3)]
	#######################################################################################################

	post_message -type info [string repeat - 80]

	#######################################################################################################
	# Create the CSn constraints when in filtering mode. In this mode the clock is the virtual clock in the
	# PCH
	#######################################################################################################
	post_message -type info "PCH CLK to flash delay: $T_PCH_to_PCH_SPI_FLASH_CLK_min ns"
	post_message -type info "   PCH CLK to mux delay: $T_PCH_SPI_CLK_to_mux ns"
	post_message -type info "   PCH CLK mux delay: $Tmux_prop_min ns"
	post_message -type info "   PCH CLK mux to flash delay: $Tmux_to_PCH_SPI_FLASH_CLK ns"
	post_message -type info "PCH CS to CPLD delay: $T_PCH_SPI_CS_to_CPLD_PCH_SPI_CS_IN ns"
	post_message -type info "   CPLD PCH CS to mux delay: [expr {$T_CPLD_SPI_PCH_CS_N_OUT_to_MUX}] ns"
	post_message -type info "   CPLD PCH CS mux delay: [expr {$Tmux_prop_max}] ns"
	post_message -type info "   CPLD PCH CS mux to flash delay: [expr {$Tmux_to_PCH_SPI_FLASH_CS}] ns"
	post_message -type info "CPLD PCH CS to flash delay: [expr {$T_CPLD_to_PCH_SPI_FLASH_CS_max}] ns"
	post_message -type info "PCH SPI CS Tco : $T_PCH_SPI_CS_TCO_MAX ns"
	set_output_delay -add_delay -max -clock PCH_SPI_CLK_virt [expr {$T_CPLD_to_PCH_SPI_FLASH_CS_max + $T_SPI_PCH_SPI_FLASH_CS_S - $T_PCH_to_PCH_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_CS_OUT)]
	set_output_delay -add_delay -min -clock PCH_SPI_CLK_virt [expr {$T_CPLD_to_PCH_SPI_FLASH_CS_max - $T_SPI_PCH_SPI_FLASH_CS_H - $T_PCH_to_PCH_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_CS_OUT)]

	# Don't include the clock delay on the input paths. Only include it above in the output delay
	set_input_delay -max -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_CS_TCO_MAX + $T_PCH_SPI_CS_to_CPLD_PCH_SPI_CS_IN}] [get_ports $spi_pin_names_map(PCH_SPI_CS_IN)]
	set_input_delay -min -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_CS_TCO_MIN + $T_PCH_SPI_CS_to_CPLD_PCH_SPI_CS_IN}] [get_ports $spi_pin_names_map(PCH_SPI_CS_IN)]
	#######################################################################################################

	post_message -type info [string repeat - 80]

	#######################################################################################################
	# Create the DQ/CLK constraints when in filtering mode. In this mode the clock is the virtual clock in the
	# PCH. The PCH output is on the falling edge of the clock
	#######################################################################################################
	# These paths are PCH Tco then PCH to Mux, Mux Tpd, mux to CPLD minus PCH clock to mux to CPLD
	post_message -type info "PCH CLK to CPLD delay: $T_PCH_to_CPLD_PCH_SPI_CLK_min ns"
	post_message -type info "PCH DQ0 to CPLD delay: $T_PCH_SPI_DQ0_to_CPLD_min ns"
	post_message -type info "PCH DQ1 to CPLD delay: $T_PCH_SPI_DQ1_to_CPLD_min ns"
	post_message -type info "PCH DQ2 to CPLD delay: $T_PCH_SPI_DQ2_to_CPLD_min ns"
	post_message -type info "PCH DQ3 to CPLD delay: $T_PCH_SPI_DQ3_to_CPLD_min ns"
	post_message -type info "PCH SPI DQ Tco : $T_PCH_SPI_DQ_TCO_MAX ns"
	post_message -type info "PCH DQ0 input delay : [expr {$T_PCH_SPI_DQ_TCO_MAX + $T_PCH_SPI_DQ0_to_CPLD_max - $T_PCH_to_CPLD_PCH_SPI_CLK_min}] ns"
	set_input_delay -max -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MAX + $T_PCH_SPI_DQ0_to_CPLD_max - $T_PCH_to_CPLD_PCH_SPI_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ0_MON)]
	set_input_delay -min -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MIN + $T_PCH_SPI_DQ0_to_CPLD_min - $T_PCH_to_CPLD_PCH_SPI_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ0_MON)]
	set_input_delay -max -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MAX + $T_PCH_SPI_DQ1_to_CPLD_max - $T_PCH_to_CPLD_PCH_SPI_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ1_MON)]
	set_input_delay -min -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MIN + $T_PCH_SPI_DQ1_to_CPLD_min - $T_PCH_to_CPLD_PCH_SPI_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ1_MON)]
	set_input_delay -max -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MAX + $T_PCH_SPI_DQ2_to_CPLD_max - $T_PCH_to_CPLD_PCH_SPI_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ2_MON)]
	set_input_delay -min -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MIN + $T_PCH_SPI_DQ2_to_CPLD_min - $T_PCH_to_CPLD_PCH_SPI_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ2_MON)]
	set_input_delay -max -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MAX + $T_PCH_SPI_DQ3_to_CPLD_max - $T_PCH_to_CPLD_PCH_SPI_CLK_min}] [get_ports $spi_pin_names_map(PCH_SPI_DQ3_MON)]
	set_input_delay -min -clock_fall -clock PCH_SPI_CLK_virt [expr {$T_PCH_SPI_DQ_TCO_MIN + $T_PCH_SPI_DQ3_to_CPLD_min - $T_PCH_to_CPLD_PCH_SPI_CLK_max}] [get_ports $spi_pin_names_map(PCH_SPI_DQ3_MON)]

	post_message -type info [string repeat # 80]
}
set_pch_spi_interface_constraints

proc set_bmc_spi_interface_constraints {} {
	global SPICLK
	global spi_pin_names_map

	post_message -type info "Creating BMC SPI interface constraints"
	post_message -type info [string repeat # 80]

	# BMC SPI Interface numbers
	# Use 50 MHz SPI CLK
	# TCO numbers from from BMC Datasheet
	set SPI_BMC_SPI_CLK_PERIOD 20.000
	set T_BMC_SPI_DQ_TCO_MIN 1.000
	set T_BMC_SPI_DQ_TCO_MAX 2.000
	# CS Tco == 2*tAHB + 0.5*tCK
	set T_BMC_SPI_CS_TCO_MIN [expr {$SPI_BMC_SPI_CLK_PERIOD - (2*5.0 + ($SPI_BMC_SPI_CLK_PERIOD/2.0))}]
	set T_BMC_SPI_CS_TCO_MAX [expr {$SPI_BMC_SPI_CLK_PERIOD - (2*5.0 + ($SPI_BMC_SPI_CLK_PERIOD/2.0))}]

	# PCH Flash device properties from datasheet
	set T_SPI_BMC_SPI_FLASH_DQ_S 1.75
	set T_SPI_BMC_SPI_FLASH_DQ_H 2.3
	set T_SPI_BMC_SPI_FLASH_CS_S 3.375
	set T_SPI_BMC_SPI_FLASH_CS_H 3.375
	set T_SPI_BMC_SPI_FLASH_DQ_TCO 6.0
	
	# SPI Mux properties from datasheet
	set Tmux_prop_min 0.2
	set Tmux_prop_max 0.2

	#######################################################################################################
	# BMC Flash board delays
	#######################################################################################################
	# CPLD -> SPI_PFR_BMC_FLASH1_BT_CLK mux
	# Path: U31.H12 (SPI_PFR_BMC_FLASH1_BT_CLK) to U3P1.10 (SPI_PFR_BMC_FLASH1_BT_CLK)
	# U31.H12 -> (3114.09) -> U3P1.10
	# Total path length: 3114.09
	set T_BMC_SPI_CS_to_CPLD_SPI_BMC_CS_N_IN_BMC_SPI_CS_IN [calc_prop_delay [expr {3114.09}]]

	# CPLD -> SPI_PFR_BOOT_CS1_N mux
	# Path: U31.L19 (SPI_PFR_BMC_BT_SECURE_CS_R_N) to U3P1.14 (SPI_PFR_BMC_BT_SECURE_CS_N)
	# U31.L19 -> (300.21) -> R3237.1 -> (0) -> R3237.2 -> (3022.9) -> U3P1.13 -> (137.13) -> U3P1.14
	# Total path length: 3460.24
	set T_CPLD_SPI_BMC_CS_N_OUT_to_MUX [calc_prop_delay [expr {300.21 + 3022.9 + 137.13}]]

	# CPLD -> SPI_PFR_BMC_FLASH1_BT_MOSI
	# Path: U31.J12 (SPI_PFR_BMC_FLASH1_BT_MOSI) to U3P1.3 (SPI_PFR_BMC_FLASH1_BT_MOSI)
	# U31.J12 -> (2928.92) -> U3P1.3
	# Total path length: 2928.92
	set T_CPLD_SPI_BMC_IO0_to_MUX [calc_prop_delay [expr {2928.92}]]

	# CPLD -> SPI_PFR_BMC_FLASH1_BT_MISO mux
	# Path: U31.J11 (SPI_PFR_BMC_FLASH1_BT_MISO) to U3P1.6 (SPI_PFR_BMC_FLASH1_BT_MISO)
	# U31.J11 -> (2903.95) -> U3P1.6
	# Total path length: 2903.95
	set T_CPLD_SPI_BMC_IO1_to_MUX [calc_prop_delay [expr {2903.95}]]

	# CPLD -> BMC SPI FLASH DQ2
	# Path: U31.H14 (SPI_PFR_BMC_BOOT_R_IO2) to U38.3 (SPI_PFR_BMC_BOOT_R_IO2)
	# U31.H14 -> (3224.49) -> U38.3
	# Total path length: 3224.49
	set T_CPLD_SPI_BMC_IO2_to_BMC_SPI_FLASH_DQ2 [calc_prop_delay [expr {3224.49}]]

	# CPLD -> BMC SPI FLASH DQ3
	# Path: U31.J13 (SPI_PFR_BMC_BOOT_R_IO3) to U38.6 (SPI_PFR_BMC_BOOT_R_IO3)
	# U31.J13 -> (3334.75) -> U38.6
	# Total path length: 3334.75
	set T_CPLD_SPI_BMC_IO3_to_BMC_SPI_FLASH_DQ3 [calc_prop_delay [expr {3334.75}]]

	# mux -> BMC SPI FLASH CLK flash
	# Path: U3P1.9 (SPI_PFR_BMC_BOOT_MUXED_CLK) to XU7D1.16 (SPI_BMC_BOOT_MUXED_R_CLK)
	# U3P1.9 -> (135.87) -> R6D79.1 -> (81.73) -> R3P12.2 -> (813.68) -> R4M28.1 -> (1102.88) -> R7D9.1 -> (0) -> R7D9.2 -> (383.86) -> XU7D1.16
	# Total path length: 2518.02
	set Tmux_to_BMC_SPI_FLASH_CLK [calc_prop_delay [expr {135.87 + 81.73 + 813.68 + 1102.88 + 383.86}]]

	# mux -> BMC SPI FLASH CS flash
	# Path: U3P1.12 (SPI_BMC_PFR_BOOT_CS0_N) to XU7D1.7 (SPI_BMC_PFR_BOOT_R_CS0_N)
	# U3P1.12 -> (126.28) -> R3P8.2 -> (205.55) -> R7D2.1 -> (0) -> R7D2.2 -> (41.88) -> R7D4.2 -> (196.2) -> XU7D1.7
	# Total path length: 569.91
	set Tmux_to_BMC_SPI_FLASH_CS [calc_prop_delay [expr {126.28 + 205.55 + 41.88 + 196.2}]]

	# mux -> BMC SPI FLASH DQ0 (MSI) flash
	# Path: U3P1.4 (SPI_BMC_FLASH1_MUXED_MOSI_R) to XU7D1.15 (SPI_BMC_BOOT_MUXED_R_MOSI)
	# U3P1.4 -> (186.25) -> R3P9.1 -> (0) -> R3P9.2 -> (31.78) -> R4P19.1 -> (487.09) -> R4N1.2 -> (1595.73) -> R7D7.1 -> (0) -> R7D7.2 -> (301.35) -> XU7D1.15
	# Total path length: 2602.2
	set Tmux_to_BMC_SPI_FLASH_DQ0 [calc_prop_delay [expr {186.25 + 31.78 + 487.09 + 1595.73 + 301.35}]]

	# mux -> BMC SPI FLASH DQ1 (MSO) flash
	# Path: U3P1.7 (SPI_PFR_BMC_BOOT_MUXED_MISO) to XU7D1.8 (SPI_BMC_BOOT_MUXED_R_MISO)
	# U3P1.7 -> (144.28) -> R6D81.1 -> (36.04) -> R3P11.2 -> (334.83) -> R4M34.2 -> (684.75) -> R7D3.1 -> (0) -> R7D3.2 -> (246.21) -> XU7D1.8
	# Total path length: 1446.11
	set Tmux_to_BMC_SPI_FLASH_DQ1 [calc_prop_delay [expr {144.28 + 36.04 + 334.83 + 684.75 + 246.21}]]

	# mux -> SPI_BMC_BT_MUXED_MON_CLK cpld pin
	# Path: U3P1.9 (SPI_PFR_BMC_BOOT_MUXED_CLK) to U31.D15 (SPI_BMC_BT_MUXED_MON_CLK)
	# U3P1.9 -> (135.87) -> R6D79.1 -> (0) -> R6D79.2 -> (3038.6) -> U31.D15
	# Total path length: 3174.47
	set Tmux_to_CPLD_SPI_BMC_CLK_MON [calc_prop_delay [expr {135.87 + 3038.6}]]

	# mux -> SPI_BMC_BT_MUXED_MON_MOSI cpld pin
	# Path: U3P1.4 (SPI_BMC_FLASH1_MUXED_MOSI_R) to U31.E16 (SPI_BMC_BT_MUXED_MON_MOSI)
	# U3P1.4 -> (186.25) -> R3P9.1 -> (0) -> R3P9.2 -> (31.78) -> R4P19.1 -> (0) -> R4P19.2 -> (2745.48) -> U31.E16
	# Total path length: 2963.51
	set Tmux_to_CPLD_SPI_BMC_DQ0_MON [calc_prop_delay [expr {186.25 + 31.78 + 2745.48}]]

	# BMC -> BMC SPI CLK mux
	# Path: U3P1.11 (SPI_BMC_BOOT_CLK) to U32.AF13 (SPI_BMC_BOOT_R_CLK)
	# U3P1.11 -> (130.1) -> R3P12.1 -> (2909.83) -> R3M96.2 -> (0) -> R3M96.1 -> (473.01) -> U32.AF13
	# Total path length: 3512.94
	set T_BMC_SPI_CLK_to_mux [calc_prop_delay [expr {130.1 + 2909.83 + 473.01}]]

	# BMC -> DQ0 (MSI) Mux
	# Path: U3P1.2 (SPI_BMC_BOOT_MOSI) to U32.AC14 (SPI_BMC_BOOT_R_MOSI)
	# U3P1.2 -> (198.6) -> R3P3.1 -> (2859.14) -> R3M91.2 -> (0) -> R3M91.1 -> (469.31) -> U32.AC14
	# Total path length: 3527.05
	set T_BMC_SPI_DQ0_to_mux [calc_prop_delay [expr {198.6 + 2859.14 + 469.31}]]

	# BMC -> DQ1 Mux
	# Path: U3P1.5 (SPI_BMC_BOOT_MISO) to U32.AB13 (SPI_BMC_BOOT_R_MISO)
	# U3P1.5 -> (337.77) -> R3P11.1 -> (2924.68) -> R3M85.2 -> (0) -> R3M85.1 -> (550.23) -> U32.AB13
	# Total path length: 3812.68
	set T_BMC_SPI_DQ1_to_mux [calc_prop_delay [expr {337.77 + 2924.68 + 550.23}]]

	# BMC -> SPI_BMC_BOOT_CS_N CPLD
	# Path: U31.L18 (SPI_BMC_BOOT_CS_N) to U32.AB14 (SPI_BMC_BOOT_CS_R_N)
	# U31.L18 -> (3515.83) -> R3P8.1 -> (2258.22) -> R7B64.2 -> (0) -> R7B64.1 -> (674.57) -> U32.AB14
	# Total path length: 6448.62
	set T_BMC_SPI_CS_to_CPLD_SPI_BMC_CS_N_IN [calc_prop_delay [expr {3515.83 + 2258.22 + 674.57}]]

	#######################################################################################################
	# Compute shorthand variables
	#######################################################################################################
	set T_CPLD_to_BMC_SPI_FLASH_CLK_min [expr {$T_BMC_SPI_CS_to_CPLD_SPI_BMC_CS_N_IN_BMC_SPI_CS_IN + $Tmux_prop_min + $Tmux_to_BMC_SPI_FLASH_CLK}]
	set T_CPLD_to_BMC_SPI_FLASH_CLK_max [expr {$T_BMC_SPI_CS_to_CPLD_SPI_BMC_CS_N_IN_BMC_SPI_CS_IN + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_CLK}]

	set T_CPLD_to_BMC_SPI_FLASH_CS_min [expr {$T_CPLD_SPI_BMC_CS_N_OUT_to_MUX + $Tmux_prop_min + $Tmux_to_BMC_SPI_FLASH_CS}]
	set T_CPLD_to_BMC_SPI_FLASH_CS_max [expr {$T_CPLD_SPI_BMC_CS_N_OUT_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_CS}]

	set T_BMC_to_BMC_SPI_FLASH_CLK_min [expr {$T_BMC_SPI_CLK_to_mux + $Tmux_prop_min + $Tmux_to_BMC_SPI_FLASH_CLK}]
	set T_BMC_to_BMC_SPI_FLASH_CLK_max [expr {$T_BMC_SPI_CLK_to_mux + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_CLK}]

	set T_BMC_to_CPLD_BMC_SPI_CLK_min [expr {$T_BMC_SPI_CLK_to_mux + $Tmux_prop_min + $Tmux_to_CPLD_SPI_BMC_CLK_MON}]
	set T_BMC_to_CPLD_BMC_SPI_CLK_max [expr {$T_BMC_SPI_CLK_to_mux + $Tmux_prop_max + $Tmux_to_CPLD_SPI_BMC_CLK_MON}]

	set T_BMC_SPI_DQ0_to_CPLD_min [expr {$T_BMC_SPI_DQ0_to_mux + $Tmux_prop_min + $Tmux_to_CPLD_SPI_BMC_DQ0_MON}]
	set T_BMC_SPI_DQ0_to_CPLD_max [expr {$T_BMC_SPI_DQ0_to_mux + $Tmux_prop_max + $Tmux_to_CPLD_SPI_BMC_DQ0_MON}]

	# Create the clock on the pin for the BMC SPI clock
	create_generated_clock -name BMC_SPI_CLK -source [get_clock_info -targets [get_clocks spi_master_clk]] [get_ports $spi_pin_names_map(BMC_SPI_CLK)]

	# SPI input timing. Data is output on falling edge of SPI clock and latched on rising edge of FPGA clock.
	# The latch edge is aligns with the next falling edge of the launch clock. Since the dest clock
	# is 2x the source clock, a multicycle is used. 
	#  
	#   Data Clock : BMC_SPI_CLK
	#       (Launch)
	#         v
	#   +-----+     +-----+     +-----+
	#   |     |     |     |     |     |	
	#   +     +-----+     +-----+     +-----
	#
	#   FPGA Clock : $SPICLK 
	#            (Latch)
	#                     v
	#   +--+  +--+  +--+  +--+  +--+  +--+
	#   |  |  |  |  |  |  |  |  |  |  |  |	
	#   +  +--+  +--+  +--+  +--+  +--+  +--
	#        (H)         (S)          
	# Setup relationship : BMC_SPI_CLK period == 2 * $SPICLK period
	# Hold relationship : 0
	set_multicycle_path -from [get_clocks BMC_SPI_CLK] -to [get_clocks $SPICLK] -setup -end 2
	set_multicycle_path -from [get_clocks BMC_SPI_CLK] -to [get_clocks $SPICLK] -hold -end 1

	# SPI Output multicycle to ensure previous edge used for hold. This is because the SPI output clock is 180deg
	# from the SPI clock
	#  

	#   Data Clock : $SPICLK 
	#            (Launch)
	#               v
	#   +--+  +--+  +--+  +--+  +--+  +--+
	#   |  |  |  |  |  |  |  |  |  |  |  |	
	#   +  +--+  +--+  +--+  +--+  +--+  +--

	#   Data Clock 2X : spi_master_clk
	#            (Launch)
	#               v
	#   +-----+     +-----+     +-----+
	#   |     |     |     |     |     |	
	#   +     +-----+     +-----+     +-----
	#
	#   Output Clock : BMC_SPI_CLK
	#                  (Latch)
	#                     v
	#   +     +-----+     +-----+     +-----+
	#   |     |     |     |     |     |     |	
	#   +-----+     +-----+     +-----+     +-----
	#        (H)         (S)          
	#
	# Setup relationship : BMC_SPI_CLK period / 2
	# Hold relationship : -BMC_SPI_CLK period / 2
	set_multicycle_path -from [get_clocks $SPICLK] -to [get_clocks BMC_SPI_CLK] -hold -start 1


	# SPI CS timing when in filtering mode. Data is output from BMC/PCH on rising edge and latched on rising edge. 
	#  
	#   Data Clock : BMC_SPI_CLK_virt
	#          (Launch)
	#             v
	#   +----+    +----+    +----+
	#   |    |    |    |    |    |	
	#   +    +----+    +----+    +----
	#
	#   SPI Clock : BMC_SPI_CLK_virt
	#                    (Latch)
	#                       v
	#   +----+    +----+    +----+
	#   |    |    |    |    |    |	
	#   +    +----+    +----+    +----
	#            (H)       (S)        
	#
	# Setup relationship : SPI_CLK period
	# Hold relationship : 0

	# Create the virtual clock for the BMC SPI clock located in the BMC. It is exclusive from the PLL clock
	create_clock -name BMC_SPI_CLK_virt -period $SPI_BMC_SPI_CLK_PERIOD
	set_clock_groups -logically_exclusive -group {BMC_SPI_CLK_virt} -group {BMC_SPI_CLK}

	# Create the monitored SPI clock pin. This is the clock from the BMC
	create_clock -name BMC_SPI_MON_CLK -period $SPI_BMC_SPI_CLK_PERIOD [get_ports $spi_pin_names_map(BMC_SPI_CLK_MON)]
	
	#######################################################################################################
	# Create the output constraints for the SPI Master IP to BMC flash
	#######################################################################################################
	# This are classic output delays with respect to the SPI clock for all data out and CS. The SPI master
	# drives these 180deg earlier than then the clock
	# Output max delay = max trace delay for data + tsu - min trace delay of clock
	# Output min delay = min trace delay for data - th - max trace delay of clock

	post_message -type info "BMC CLK Period: $SPI_BMC_SPI_CLK_PERIOD ns"
	post_message -type info "CPLD BMC CLK to flash delay: [expr {$T_CPLD_to_BMC_SPI_FLASH_CLK_min}] ns"
	post_message -type info "CPLD BMC CS to flash delay: [expr {$T_CPLD_to_BMC_SPI_FLASH_CS_max}] ns"
	post_message -type info "CPLD BMC DQ0 to flash delay: [expr {$T_CPLD_SPI_BMC_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_DQ0}] ns"
	post_message -type info "CPLD BMC DQ1 to flash delay: [expr {$T_CPLD_SPI_BMC_IO1_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_DQ1}] ns"
	post_message -type info "CPLD BMC DQ2 to flash delay: [expr {$T_CPLD_SPI_BMC_IO2_to_BMC_SPI_FLASH_DQ2}] ns"
	post_message -type info "CPLD BMC DQ3 to flash delay: [expr {$T_CPLD_SPI_BMC_IO3_to_BMC_SPI_FLASH_DQ3}] ns"
	post_message -type info "BMC Flash DQ Setup : $T_SPI_BMC_SPI_FLASH_DQ_S ns"
	post_message -type info "BMC Flash DQ Hold : $T_SPI_BMC_SPI_FLASH_DQ_H ns"
	post_message -type info "BMC Flash CS Setup : $T_SPI_BMC_SPI_FLASH_CS_S ns"
	post_message -type info "BMC Flash CS Hold : $T_SPI_BMC_SPI_FLASH_CS_H ns"

	set_output_delay -max -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_DQ0 + $T_SPI_BMC_SPI_FLASH_DQ_S - $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ0)]
	set_output_delay -min -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO0_to_MUX + $Tmux_prop_min + $Tmux_to_BMC_SPI_FLASH_DQ0 - $T_SPI_BMC_SPI_FLASH_DQ_H - $T_CPLD_to_BMC_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(BMC_SPI_DQ0)]

	set_output_delay -max -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO1_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_DQ1 + $T_SPI_BMC_SPI_FLASH_DQ_S - $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ1)]
	set_output_delay -min -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO1_to_MUX + $Tmux_prop_min + $Tmux_to_BMC_SPI_FLASH_DQ1 - $T_SPI_BMC_SPI_FLASH_DQ_H - $T_CPLD_to_BMC_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(BMC_SPI_DQ1)]

	set_output_delay -max -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO2_to_BMC_SPI_FLASH_DQ2 + $T_SPI_BMC_SPI_FLASH_DQ_S - $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ2)]
	set_output_delay -min -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO2_to_BMC_SPI_FLASH_DQ2 - $T_SPI_BMC_SPI_FLASH_DQ_H - $T_CPLD_to_BMC_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(BMC_SPI_DQ2)]

	set_output_delay -max -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO3_to_BMC_SPI_FLASH_DQ3 + $T_SPI_BMC_SPI_FLASH_DQ_S - $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ3)]
	set_output_delay -min -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO3_to_BMC_SPI_FLASH_DQ3 - $T_SPI_BMC_SPI_FLASH_DQ_H - $T_CPLD_to_BMC_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(BMC_SPI_DQ3)]

	set_output_delay -max -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_to_BMC_SPI_FLASH_CS_max + $T_SPI_BMC_SPI_FLASH_CS_S - $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)]
	set_output_delay -min -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_to_BMC_SPI_FLASH_CS_min - $T_SPI_BMC_SPI_FLASH_CS_H - $T_CPLD_to_BMC_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)]
	#######################################################################################################

	#######################################################################################################
	# Create the input constraints for the SPI Master IP from the BMC Flash
	#######################################################################################################
	# This are classic input delays with respect to the SPI clock for all Data in.
	# The flash drives the data on the falling edge of the clock
	# Input max delay = max trace delay for data + tco_max + min trace delay for clock
	# Input min delay = max trace delay for data + tco_min + max trace delay for clock

	post_message -type info "BMC Flash DQ Tco : $T_SPI_BMC_SPI_FLASH_DQ_TCO ns"
	post_message -type info "MON DQ0 Delay : [expr {$T_CPLD_SPI_BMC_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_DQ0 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}]"
	post_message -type info "   T_CPLD_SPI_BMC_IO0_to_MUX : $T_CPLD_SPI_BMC_IO0_to_MUX"
	post_message -type info "   Tmux_prop_max : $Tmux_prop_max"
	post_message -type info "   Tmux_to_BMC_SPI_FLASH_DQ0 : $Tmux_to_BMC_SPI_FLASH_DQ0"
	post_message -type info "   T_SPI_BMC_SPI_FLASH_DQ_TCO : $T_SPI_BMC_SPI_FLASH_DQ_TCO"
	post_message -type info "   T_CPLD_to_BMC_SPI_FLASH_CLK_min : $T_CPLD_to_BMC_SPI_FLASH_CLK_min"

	set_input_delay -max -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO0_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_DQ0 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ0)]
	set_input_delay -min -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO0_to_MUX + $Tmux_prop_min + $Tmux_to_BMC_SPI_FLASH_DQ0 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ0)]

	set_input_delay -max -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO1_to_MUX + $Tmux_prop_max + $Tmux_to_BMC_SPI_FLASH_DQ1 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ1)]
	set_input_delay -min -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO1_to_MUX + $Tmux_prop_min + $Tmux_to_BMC_SPI_FLASH_DQ1 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ1)]

	set_input_delay -max -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO2_to_BMC_SPI_FLASH_DQ2 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ2)]
	set_input_delay -min -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO2_to_BMC_SPI_FLASH_DQ2 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ2)]

	set_input_delay -max -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO3_to_BMC_SPI_FLASH_DQ3 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ3)]
	set_input_delay -min -clock_fall -clock [get_clocks BMC_SPI_CLK] [expr {$T_CPLD_SPI_BMC_IO3_to_BMC_SPI_FLASH_DQ3 + $T_SPI_BMC_SPI_FLASH_DQ_TCO + $T_CPLD_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ3)]
	#######################################################################################################

	post_message -type info [string repeat - 80]

	#######################################################################################################
	# Create the CSn constraints when in filtering mode. In this mode the clock is the virtual clock in the
	# BMC
	#######################################################################################################
	post_message -type info "BMC CLK to flash delay: $T_BMC_to_BMC_SPI_FLASH_CLK_min ns"
	post_message -type info "   BMC CLK to mux delay: $T_BMC_SPI_CLK_to_mux ns"
	post_message -type info "   BMC CLK mux delay: $Tmux_prop_min ns"
	post_message -type info "   BMC CLK mux to flash delay: $Tmux_to_BMC_SPI_FLASH_CLK ns"
	post_message -type info "BMC CS to CPLD delay: $T_BMC_SPI_CS_to_CPLD_SPI_BMC_CS_N_IN ns"
	post_message -type info "CPLD BMC CS to flash delay: [expr {$T_CPLD_to_BMC_SPI_FLASH_CS_max}] ns"
	post_message -type info "   CPLD BMC CS to mux delay: [expr {$T_CPLD_SPI_BMC_CS_N_OUT_to_MUX}] ns"
	post_message -type info "   CPLD BMC CS mux delay: [expr {$Tmux_prop_max}] ns"
	post_message -type info "   CPLD BMC CS mux to flash delay: [expr {$Tmux_to_BMC_SPI_FLASH_CS}] ns"
	post_message -type info "BMC SPI CS Tco : $T_BMC_SPI_CS_TCO_MAX ns"

	set_output_delay -add_delay -max -clock BMC_SPI_CLK_virt [expr {$T_CPLD_to_BMC_SPI_FLASH_CS_max + $T_SPI_BMC_SPI_FLASH_CS_S - $T_BMC_to_BMC_SPI_FLASH_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)]
	set_output_delay -add_delay -min -clock BMC_SPI_CLK_virt [expr {$T_CPLD_to_BMC_SPI_FLASH_CS_max - $T_SPI_BMC_SPI_FLASH_CS_H - $T_BMC_to_BMC_SPI_FLASH_CLK_max}] [get_ports $spi_pin_names_map(BMC_SPI_CS_OUT)]

	# Don't include the clock delay here. It is included above.
	set_input_delay -max -clock BMC_SPI_CLK_virt [expr {$T_BMC_SPI_CS_TCO_MAX + $T_BMC_SPI_CS_to_CPLD_SPI_BMC_CS_N_IN}] [get_ports $spi_pin_names_map(BMC_SPI_CS_IN)]
	set_input_delay -min -clock BMC_SPI_CLK_virt [expr {$T_BMC_SPI_CS_TCO_MIN + $T_BMC_SPI_CS_to_CPLD_SPI_BMC_CS_N_IN}] [get_ports $spi_pin_names_map(BMC_SPI_CS_IN)]
	#######################################################################################################

	post_message -type info [string repeat - 80]

	#######################################################################################################
	# Create the DQ/CLK constraints on the monitor interface. In this mode the clock is the virtual clock in the
	# BMC. The BMC output is on the falling edge of the clock
	#######################################################################################################
	# These paths are BMC Tco then BMC to Mux, Mux Tpd, mux to CPLD minus BMC clock to mux to CPLD
	post_message -type info "BMC DQ0 input delay : [expr {$T_BMC_SPI_DQ_TCO_MAX + $T_BMC_SPI_DQ0_to_CPLD_max - $T_BMC_to_CPLD_BMC_SPI_CLK_min}] ns"
	post_message -type info "BMC CLK to CPLD delay: $T_BMC_to_CPLD_BMC_SPI_CLK_min ns"
	post_message -type info "BMC DQ0 to CPLD delay: $T_BMC_SPI_DQ0_to_CPLD_min ns"
	post_message -type info "BMC SPI DQ Tco : $T_BMC_SPI_DQ_TCO_MAX ns"
	set_input_delay -max -clock_fall -clock BMC_SPI_CLK_virt [expr {$T_BMC_SPI_DQ_TCO_MAX + $T_BMC_SPI_DQ0_to_CPLD_max - $T_BMC_to_CPLD_BMC_SPI_CLK_min}] [get_ports $spi_pin_names_map(BMC_SPI_DQ0_MON)]
	set_input_delay -min -clock_fall -clock BMC_SPI_CLK_virt [expr {$T_BMC_SPI_DQ_TCO_MIN + $T_BMC_SPI_DQ0_to_CPLD_min - $T_BMC_to_CPLD_BMC_SPI_CLK_max}] [get_ports $spi_pin_names_map(BMC_SPI_DQ0_MON)]

	post_message -type info [string repeat # 80]
}
set_bmc_spi_interface_constraints

### set false paths for SPI signals which cross clock domains

# false paths from the SPI Filter chip select disable output to the SPI master clock(s), we do not filter any commands from the CPLD internal master
set_false_path -from {u_core|u_spi_control|pch_spi_filter_inst|o_spi_disable_cs} -to [get_clocks {PCH_SPI_CLK PCH_SPI_CLK_virt}]
set_false_path -from {u_core|u_spi_control|bmc_spi_filter_inst|o_spi_disable_cs} -to [get_clocks {BMC_SPI_CLK BMC_SPI_CLK_virt}]

# false paths from NIOS controlled GPO signals to SPI CS pins, these GPO signals will only change state in T-1 when there is no activity on the SPI bus
set_false_path -from "u_core|u_pfr_sys|u_gpo_1|data_out\[$gen_gpo_controls_pkg(GPO_1_SPI_MASTER_BMC_PCHN_BIT_POS)\]" -to [get_ports [list $spi_pin_names_map(PCH_SPI_CS_OUT) $spi_pin_names_map(PCH_SPI_DQ0) $spi_pin_names_map(PCH_SPI_DQ1) $spi_pin_names_map(PCH_SPI_DQ2) $spi_pin_names_map(PCH_SPI_DQ3)]]
set_false_path -from "u_core|u_pfr_sys|u_gpo_1|data_out\[$gen_gpo_controls_pkg(GPO_1_SPI_MASTER_BMC_PCHN_BIT_POS)\]" -to [get_ports [list $spi_pin_names_map(BMC_SPI_CS_OUT) $spi_pin_names_map(BMC_SPI_DQ0) $spi_pin_names_map(BMC_SPI_DQ1) $spi_pin_names_map(BMC_SPI_DQ2) $spi_pin_names_map(BMC_SPI_DQ3)]]

set_false_path -from "u_core|u_pfr_sys|u_gpo_1|data_out\[$gen_gpo_controls_pkg(GPO_1_FM_SPI_PFR_PCH_MASTER_SEL_BIT_POS)\]" -to [get_ports [list $spi_pin_names_map(PCH_SPI_CS_OUT) $spi_pin_names_map(PCH_SPI_DQ0) $spi_pin_names_map(PCH_SPI_DQ1) $spi_pin_names_map(PCH_SPI_DQ2) $spi_pin_names_map(PCH_SPI_DQ3)]]
set_false_path -from "u_core|u_pfr_sys|u_gpo_1|data_out\[$gen_gpo_controls_pkg(GPO_1_FM_SPI_PFR_BMC_BT_MASTER_SEL_BIT_POS)\]" -to [get_ports [list $spi_pin_names_map(BMC_SPI_CS_OUT) $spi_pin_names_map(BMC_SPI_DQ0) $spi_pin_names_map(BMC_SPI_DQ1) $spi_pin_names_map(BMC_SPI_DQ2) $spi_pin_names_map(BMC_SPI_DQ3)]]

# Cut the async path from the filter disable to CS
set_false_path -from "u_core|u_pfr_sys|u_gpo_1|data_out\[$gen_gpo_controls_pkg(GPO_1_PCH_SPI_FILTER_DISABLE_BIT_POS)\]" -to {u_core|u_spi_control|pch_spi_filter_inst|o_spi_disable_cs}
set_false_path -from "u_core|u_pfr_sys|u_gpo_1|data_out\[$gen_gpo_controls_pkg(GPO_1_BMC_SPI_FILTER_DISABLE_BIT_POS)\]" -to {u_core|u_spi_control|bmc_spi_filter_inst|o_spi_disable_cs}


# false paths from the CPLD internal SPI master to the 'virtual' SPI clocks. When the BMC/PCH are driving CS then the internal SPI master is not
set_false_path -from {u_core|u_spi_control|spi_master_inst|qspi_inf_inst|ncs_reg[0]} -to [get_clocks PCH_SPI_CLK_virt]
set_false_path -from {u_core|u_spi_control|spi_master_inst|qspi_inf_inst|ncs_reg[0]} -to [get_clocks BMC_SPI_CLK_virt]

# false path from CPLD reset signal to external SPI clock
set_false_path -from {*u_pfr_sys|altera_reset_controller:rst_controller|r_sync_rst} -to [get_clocks [list PCH_SPI_MON_CLK BMC_SPI_MON_CLK]]

# false path from ibb_addr_detected to ibb_access_detectedn
set_false_path -from {u_core|u_spi_control|bmc_spi_filter_inst|ibb_addr_detected} -to {u_core|u_spi_control|bmc_spi_filter_inst|ibb_access_detectedn}

# Disable constraints for now
set_false_path -from "u_core|u_pfr_sys|u_gpo_1|data_out\[$gen_gpo_controls_pkg(GPO_1_BMC_SPI_ADDR_MODE_SET_3B_BIT_POS)\]" -to {u_core|u_spi_control|bmc_spi_filter_inst|address_mode_4b}


# false paths from the spi filter 'filtered_command_info' registers to the system clock
# these signals change infrequently and will be read by the NIOS multiple times to ensure data consistency before result of read is used
set regs [get_registers *_spi_filter_inst|o_filtered_command_info* -nowarn]
foreach_in_collection reg $regs {
    set_false_path -from {*_spi_filter_inst|o_filtered_command_info*} -to [get_clocks {u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1]}]
}

proc report_pfr_overconstraints {} {
	report_timing -to {pfr_core:u_core|pfr_sys:u_pfr_sys|altera_onchip_flash:u_ufm|altera_onchip_flash_block:altera_onchip_flash_block|*} -npath 100 -nworst 10 -show_routing -detail full_path  -multi_corner -panel {PFR||Flash Block||C2P : Setup} -setup
	report_timing -to {pfr_core:u_core|pfr_sys:u_pfr_sys|altera_onchip_flash:u_ufm|altera_onchip_flash_block:altera_onchip_flash_block|*} -npath 100 -nworst 10 -show_routing -detail full_path  -multi_corner -panel {PFR||Flash Block||C2P : Hold} -hold

	report_timing -from {pfr_core:u_core|pfr_sys:u_pfr_sys|altera_onchip_flash:u_ufm|altera_onchip_flash_block:altera_onchip_flash_block|*} -npath 100 -nworst 10 -show_routing -detail full_path  -multi_corner -panel {PFR||Flash Block||P2C : Setup} -setup
	report_timing -from {pfr_core:u_core|pfr_sys:u_pfr_sys|altera_onchip_flash:u_ufm|altera_onchip_flash_block:altera_onchip_flash_block|*} -npath 100 -nworst 10 -show_routing -detail full_path  -multi_corner -panel {PFR||Flash Block||P2C : Hold} -hold
}

if { $::TimeQuestInfo(nameofexecutable) == "quartus_fit" } {
	puts "Applying Quartus FIT specific constraints"

	# Add extra uncertaintly on hold. This is to address paths in the common core which would otherwise fail hold.
	set_clock_uncertainty -add -enable_same_physical_edge -from u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[2] -to Archer_City_Main_wrapper:u_common_core|Archer_City_Main:mArcher_City_Main|ClkDiv:ClkLC|div_clk 20ps
}



