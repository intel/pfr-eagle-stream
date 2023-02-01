#**************************************************************
# Time Information
#**************************************************************
set_time_format -unit ns -decimal_places 3

# Propigation delay (in/ns)
# In general on the PCB, the signal travels at the speed of ~160 ps/inch (1000 mils = 1 inch)
global PCB_PROP_IN_PER_NS
set PCB_PROP_IN_PER_NS 6.25

#**************************************************************
# Helper functions
#**************************************************************
proc set_min_relay_timing {port_name trace_length} {
	global PCB_PROP_IN_PER_NS
	set delay [format "%.2f" [expr { ($trace_length/1000.0) / $PCB_PROP_IN_PER_NS }]]
	post_message -type info "Setting min SMB Relay I/O Timing on $port_name of $delay ns"

	set_input_delay -min -clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } $delay [get_ports $port_name]
	set_output_delay -min -clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } $delay [get_ports $port_name]
}

proc set_max_relay_timing {port_name trace_length} {
	global PCB_PROP_IN_PER_NS
	set delay [format "%.2f" [expr { ($trace_length/1000.0) / $PCB_PROP_IN_PER_NS }]]
	post_message -type info "Setting max SMB Relay I/O Timing on $port_name of $delay ns"

	set_input_delay -max -clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } $delay [get_ports $port_name]
	set_output_delay -max -clock { u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1] } $delay [get_ports $port_name]
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
set SYSCLK u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[1]
set CLK50M u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[0]

#**************************************************************
# Create Generated Clock
#**************************************************************

create_generated_clock -name Archer_City_Main_wrapper:u_common_core|Archer_City_Main:mArcher_City_Main|ClkDiv:ClkLC|div_clk -source $CLK2M -divide_by 4 -multiply_by 1 Archer_City_Main_wrapper:u_common_core|Archer_City_Main:mArcher_City_Main|ClkDiv:ClkLC|div_clk
create_generated_clock -name Archer_City_Main_wrapper:u_common_core|Archer_City_Main:mArcher_City_Main|ClkDiv:Ena125mSec|div_clk -source $CLK2M -divide_by 2 -multiply_by 1 Archer_City_Main_wrapper:u_common_core|Archer_City_Main:mArcher_City_Main|ClkDiv:Ena125mSec|div_clk
create_generated_clock -name Archer_City_Main_wrapper:u_common_core|Archer_City_Main:mArcher_City_Main|GlitchFilter:mGlitchFilter_PLD_BUTTON|rFilteredSignals[0] -source $CLK2M -divide_by 1 -multiply_by 1 Archer_City_Main_wrapper:u_common_core|Archer_City_Main:mArcher_City_Main|GlitchFilter:mGlitchFilter_PLD_BUTTON|rFilteredSignals[0]


#**************************************************************
# Set Clock Latency
#**************************************************************



#**************************************************************
# Set Clock Uncertainty
#**************************************************************
derive_clock_uncertainty


#**************************************************************
# Set Input Delay
#**************************************************************


#**************************************************************
# Set Output Delay
#**************************************************************



#**************************************************************
# Set Clock Groups
#**************************************************************
# Set all clocks as exclusive
set_clock_groups -logically_exclusive -group [get_clocks [list \
	$CLK2M \
	$CLK50M \
	$SYSCLK \
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


if { $::TimeQuestInfo(nameofexecutable) == "quartus_fit" } {
	puts "Applying Quartus FIT specific constraints"

	# Add extra uncertaintly on hold. This is to address paths in the common core which would otherwise fail hold.
	set_clock_uncertainty -add -enable_same_physical_edge -from u_pfr_sys_clocks_reset|u_sys_pll_ip|altpll_component|auto_generated|pll1|clk[2] -to Archer_City_Main_wrapper:u_common_core|Archer_City_Main:mArcher_City_Main|ClkDiv:ClkLC|div_clk 20ps
}
