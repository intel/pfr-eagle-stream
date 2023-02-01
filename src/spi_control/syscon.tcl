set csr_base 0x3c00
set mem_base 0x80000000
set gpo_base 0x3d90

set control_reg 			[expr $csr_base + (0x0 << 2) ]
set baud_rate_reg 			[expr $csr_base + (0x1 << 2) ]
set cs_delay_reg 			[expr $csr_base + (0x2 << 2) ]
set read_capture_reg 		[expr $csr_base + (0x3 << 2) ]
set protocol_setting_reg 	[expr $csr_base + (0x4 << 2) ]
set read_instr_reg 			[expr $csr_base + (0x5 << 2) ]
set write_instr_reg 		[expr $csr_base + (0x6 << 2) ]
set flash_cmd_reg 			[expr $csr_base + (0x7 << 2) ]
set exec_cmd_reg 			[expr $csr_base + (0x8 << 2) ]
set flash_address_reg 		[expr $csr_base + (0x9 << 2) ]
set flash_write_reg			[expr $csr_base + (0xa << 2) ]
set flash_read_reg			[expr $csr_base + (0xc << 2) ]

proc read_chip_id {mp} {
	global flash_cmd_reg
	global exec_cmd_reg 
	global flash_read_reg
	global protocol_setting_reg	

	master_write_32 $mp $protocol_setting_reg 0x00000
	master_write_32 $mp $flash_cmd_reg 0x389f
	master_write_32 $mp $exec_cmd_reg 0x1
	set chip_id [master_read_32 $mp $flash_read_reg 0x1]
	puts "The chip ID is: $chip_id"
	return $chip_id
}

proc read_config_reg {mp} {
    global flash_cmd_reg
    global exec_cmd_reg
    global flash_read_reg
    global protocol_setting_reg

	master_write_32 $mp $protocol_setting_reg 0x00000
    master_write_32 $mp $flash_cmd_reg 0x28b5
    master_write_32 $mp $exec_cmd_reg 0x1 
    set config_reg [master_read_32 $mp $flash_read_reg 0x1]
	puts "The configuration register is: $config_reg"
	return $config_reg
}

proc read_status_reg {mp} {
    global flash_cmd_reg
    global exec_cmd_reg
    global flash_read_reg
    global protocol_setting_reg

    master_write_32 $mp $protocol_setting_reg 0x00000
    master_write_32 $mp $flash_cmd_reg 0x1805
    master_write_32 $mp $exec_cmd_reg 0x1 
    set status_reg [master_read_32 $mp $flash_read_reg 0x1]
	puts "The configuration register is: $status_reg"
	return $status_reg
}

proc read_flag_status_reg {mp} {
    global flash_cmd_reg
    global exec_cmd_reg
    global flash_read_reg

    master_write_32 $mp $flash_cmd_reg 0x1870
    master_write_32 $mp $exec_cmd_reg 0x1 
    set flag_status_reg [master_read_32 $mp $flash_read_reg 0x1]
	puts "The configuration register is: $flag_status_reg"
	return $flag_status_reg
}

proc write_enable {mp} {
    global flash_cmd_reg
    global exec_cmd_reg
    global flash_read_reg

    master_write_32 $mp $flash_cmd_reg 0x0806
    master_write_32 $mp $exec_cmd_reg 0x1 
}

#do not use on real platform
proc erase_chip {mp} {
    global flash_cmd_reg
    global exec_cmd_reg
    global protocol_setting_reg

    write_enable $mp
	master_write_32 $mp $protocol_setting_reg 0x00000
    master_write_32 $mp $flash_cmd_reg 0x0BC4
    master_write_32 $mp $exec_cmd_reg 0x1 
    master_write_32 $mp $protocol_setting_reg 0x22220
}

proc write_enable {mp} {
    global flash_cmd_reg
    global exec_cmd_reg
    global flash_read_reg

    master_write_32 $mp $flash_cmd_reg 0x0806
    master_write_32 $mp $exec_cmd_reg 0x1 
}

proc write_config_reg {mp} {
	global flash_write_reg
	global flash_cmd_reg
	global exec_cmd_reg
	master_write_32 $mp $flash_write_reg 0xffff
	master_write_32 $mp $flash_cmd_reg 0x20b1
    master_write_32 $mp $exec_cmd_reg 0x1
}

proc restore_default_config_reg {mp} {
	master_write_32 $mp $flash_write_reg 0xffff
	master_write_32 $mp $flash_cmd_reg 0x20b1
    master_write_32 $mp $exec_cmd_reg 0x1
}

proc normal_read_byte {mp base_addr num_words} {
	global read_instr_reg
	global protocol_setting_reg
	global mem_base
	master_write_32 $mp $read_instr_reg 0x0003
	master_write_32 $mp $protocol_setting_reg 0x000000
	puts [master_read_8 $mp [expr $base_addr + $mem_base] $num_words]
}

proc quad_read_byte {mp base_addr num_words} {
	global read_instr_reg
	global protocol_setting_reg
	global mem_base
	master_write_32 $mp $read_instr_reg 0x0AEB
	master_write_32 $mp $protocol_setting_reg 0x22220
	puts [master_read_8 $mp [expr $base_addr + $mem_base] $num_words]
}

proc normal_write_byte {mp base_addr data} {
	global write_instr_reg
	global protocol_setting_reg
	global mem_base
	master_write_32 $mp $write_instr_reg 0x0502
	master_write_32 $mp $protocol_setting_reg 0x000000
	master_write_8 $mp [expr $base_addr + $mem_base] $data
}

#does not work
#transmits data on 1 line instead of 4
proc quad_write_byte {mp base_addr data} {
	global write_instr_reg
	global protocol_setting_reg
	global mem_base
	master_write_32 $mp $write_instr_reg 0x0538
	master_write_32 $mp $protocol_setting_reg 0x22220
	master_write_8 $mp [expr $base_addr + $mem_base] $data
}

proc quad_write_word {mp base_addr data} {
	global write_instr_reg
	global protocol_setting_reg
	global mem_base
	master_write_32 $mp $write_instr_reg 0x0538
	master_write_32 $mp $protocol_setting_reg 0x22220
	master_write_32 $mp [expr $base_addr + $mem_base] $data
}

#erases a 4kb sector
proc erase_subsector {mp base_addr} {
	global flash_cmd_reg
	global protocol_setting_reg
	global flash_address_reg
	global exec_cmd_reg
	global mem_base
	master_write_32 $mp $protocol_setting_reg 0x00000
	master_write_32 $mp $flash_cmd_reg 0x0320
	master_write_32 $mp $flash_address_reg [expr $mem_base + $base_addr]
    master_write_32 $mp $exec_cmd_reg 0x1 
}

set mp [lindex [get_service_paths processor] 0]
open_service processor $mp
processor_stop $mp
close_service processor $mp 

set mp [lindex [get_service_paths master] 0]
open_service master $mp

#set read cmd
master_write_32 $mp $read_instr_reg 0x0003
master_write_32 $mp $protocol_setting_reg 0x000000


#set spi muxes to cpld control + spi control mux to pch
master_write_32 $mp $gpo_base 0x680C
#PCH read
read_chip_id $mp
puts [master_read_32 $mp $mem_base 0x100]

#set spi muxes to cpld control + spi control to BMC
master_write_32 $mp $gpo_base 0x780C
#BMC read
read_chip_id $mp 
puts [master_read_32 $mp $mem_base 0x100]

master_write_32 $mp $gpo_base 0x6803

close_service master $mp

