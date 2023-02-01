#! /bin/bash

set -e

set UNAME ""
UNAME=$(uname -r)

case "$UNAME" in
	*Microsoft*) WINDOWS_EXE=.exe;;
	*) WINDOWS_EXE=;;
esac

qsys-generate$WINDOWS_EXE --synthesis=verilog --simulation=verilog src/top_ip/dual_config_ip.qsys --output-directory=src/top_ip/dual_config_ip
qsys-generate$WINDOWS_EXE --synthesis=verilog --simulation=verilog src/top_ip/int_osc_ip.qsys --output-directory=src/top_ip/int_osc_ip
qsys-generate$WINDOWS_EXE --synthesis=verilog --simulation=verilog src/pfr_sys/pfr_sys.qsys --output-directory=src/pfr_sys/pfr_sys
qsys-generate$WINDOWS_EXE --synthesis=verilog --simulation=verilog src/recovery_sys/recovery_sys.qsys --output-directory=src/recovery_sys/recovery_sys

make -C fw/bsp/pfr_sys_sim
make -C fw/bsp/pfr_sys
make -C fw/bsp/recovery_sys
make -C fw/code/hw/archer_city
make -C fw/code/hw/archer_city_recovery
cp fw/code/hw/archer_city/mem_init/u_ufm.hex ./pfr_code.hex
cp fw/code/hw/archer_city_recovery/mem_init/u_ufm.hex ./recovery_code.hex

touch prep_done.txt
