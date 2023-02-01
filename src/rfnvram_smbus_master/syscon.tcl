set rfnvram_base 0x3d00
set cmd_fifo $rfnvram_base
set rx_fifo [expr $rfnvram_base + 0x4]
set cntl_register [expr $rfnvram_base + 0x8]
set scl_low [expr $rfnvram_base + (0x8<<2)]
set scl_high [expr $rfnvram_base + (0x9<<2)]
set sda_hold [expr $rfnvram_base + (0xa<<2)]
set status_reg [expr $rfnvram_base + (0x5<<2)]
set isr_reg [expr $rfnvram_base + (0x4<<2)]
set rx_fifo_level [expr $rfnvram_base + (0x7<<2)]

proc flush_fifo {mp} {
	global rx_fifo_level
	global rx_fifo
	while {[master_read_32 $mp $rx_fifo_level 1] != 0} {
	master_read_32 $mp $rx_fifo 1
	}
}

proc read_addr {mp addr} {
	global cmd_fifo
	global rx_fifo
	global rx_fifo_level
	set msb [expr $addr & 0xFF00]
	set lsb [expr $addr & 0xFF]
	master_write_32 $mp $cmd_fifo 0x2DC
	master_write_32 $mp $cmd_fifo $msb
	master_write_32 $mp $cmd_fifo $lsb
	master_write_32 $mp $cmd_fifo 0x2DD
	master_write_32 $mp $cmd_fifo 0x100
	while {[master_read_32 $mp $rx_fifo_level 1] == 0} {}
	return [master_read_32 $mp $rx_fifo 1]

}

set mp [lindex [get_service_paths processor] 0]
open_service master $mp
processor_stop $mp
close_service processor $mp

set mp [lindex [get_service_paths master] 0]
open_service master $mp

#master_write_32 $mp $cntl_register 0x1
#master_write_32 $mp $scl_low 0xfa
#master_write_32 $mp $scl_high 0xfa

set lsb 0
set msb 0
while {[master_read_32 $mp $rx_fifo_level 1] != 0} {
	master_read_32 $mp $rx_fifo 1
}

master_write_32 $mp $cmd_fifo 0x2DC
master_write_32 $mp $cmd_fifo 0
master_write_32 $mp $cmd_fifo 0
master_write_32 $mp $cmd_fifo 0x2DD
for {set i 0} {$i < 2047} {incr i} {

	master_write_32 $mp $cmd_fifo 0x00
	while {[master_read_32 $mp $rx_fifo_level 1] == 0} {}
	set result [master_read_32 $mp $rx_fifo 0x1]
	puts "ADDR = $i data = $result"
}
master_write_32 $mp $cmd_fifo 0x100

close_service master $mp
