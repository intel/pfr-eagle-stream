#! /bin/env python

import errno
import getopt
import os
import shutil
import subprocess
import sys
import re

##############################
#
# Helper functions
#
##############################
def print_info(msg):
    print("[INFO] " + msg)


def copy_file(cp_src_file, cp_dest_file, use_symlink):
    mkdir_p(os.path.dirname(cp_dest_file))
    try:
        # Skip existed files
        if not os.path.exists(cp_dest_file):
            if use_symlink:
                os.symlink(cp_src_file, cp_dest_file)
            else:
                shutil.copyfile(cp_src_file, cp_dest_file)
    except shutil.Error as e:
        print(cp_src_file)
        print(cp_dest_file)
        print('[Error] %s' % e)
    except OSError as e:
        print(cp_src_file)
        print(cp_dest_file)
        print('[Error] %s' % e)


def mkdir_p(path):
    """
    Safe mkdir implementation. Equivalent to "mkdir -p" in linux command.
    """
    if path is None or path == "":
        return
    try:
        os.makedirs(path)
    except OSError as exc:
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise


def copy_dir(cp_src_dir, cp_dest_dir, use_symlink, recursive=True):
    print("Copying files from " + cp_src_dir + " into " + cp_dest_dir)
    mkdir_p(cp_dest_dir)
    for filename in os.listdir(cp_src_dir):
        cp_src_file = os.path.join(cp_src_dir, filename)
        cp_dest_file = os.path.join(cp_dest_dir, filename)
        if os.path.isfile(cp_src_file):
            copy_file(cp_src_file, cp_dest_file, use_symlink)
        elif recursive:
            copy_dir(cp_src_file, cp_dest_file, use_symlink)

def flatten_qsf(ship_dir, root_qsf):
    root_qsf_tmp = open(os.path.join(ship_dir, root_qsf + ".tmp"), "w")
    root_qsf_src = open(os.path.join(ship_dir, root_qsf), "r")
    line_source_re = re.compile("^\s*source\s+(\S+)")

    line = root_qsf_src.readline()
    while line:
        line_source_re_match = line_source_re.match(line)
        if line_source_re_match:
            sourced_file = open(os.path.join(ship_dir, line_source_re_match.group(1)), "r")
            sourced_file_line = sourced_file.readline()
            while sourced_file_line:
                root_qsf_tmp.write(sourced_file_line) 
                sourced_file_line = sourced_file.readline()
            sourced_file.close()
        else :
            root_qsf_tmp.write(line) 
        line = root_qsf_src.readline()

    root_qsf_tmp.close()
    root_qsf_src.close()
    os.remove(os.path.join(ship_dir, root_qsf))
    os.rename(os.path.join(ship_dir, root_qsf + ".tmp"), os.path.join(ship_dir, root_qsf))


def copy_qsys_dir(qsys_src_sys_file, qsys_src_dir, qsys_dest_sys_file, qsys_dest_dir):
    print("Copying Qsys system " + qsys_src_dir + "/" + qsys_src_sys_file + " to " + qsys_dest_dir + "/" + qsys_dest_sys_file)
    copy_dir(qsys_src_dir, qsys_dest_dir, False)
    if (qsys_src_sys_file != qsys_dest_sys_file):
        os.rename(os.path.join(qsys_dest_dir, qsys_src_sys_file), os.path.join(qsys_dest_dir, qsys_dest_sys_file))
    # Make qsys file writable
    qsys_f = os.path.join(qsys_dest_dir, qsys_dest_sys_file)
    cmd = "chmod u+w " + qsys_f
    process = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE)
    output, error = process.communicate()

def patch_file(src_file, patch_file):
    # Patch the pfr_sys as required
    cmd = "patch -c " + src_file + " -i " + patch_file + " -o " + src_file + ".new"
    print "Preparing to patch " + src_file
    print cmd
    process = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE)
    output, error = process.communicate()
    print(output)
    if process.returncode is not 0:
        print("Error during patch")
        sys.exit(1)
    os.remove(src_file)
    os.rename(src_file + ".new", src_file)


##############################
#
# Directories and Files
#
##############################
script_dir = os.path.dirname(os.path.realpath(__file__))

# Source directories
fw_dir = os.path.join(script_dir, "../../fw")
src_dir = os.path.join(script_dir, "../../src")
impl_dir = script_dir

# Destination directories
ship_dir = os.path.join(script_dir, "ship")
ship_fw_dir = os.path.join(ship_dir, "fw")
ship_src_dir = os.path.join(ship_dir, "src")

##############################
#
# Parse arguments
#
##############################

use_symlink = False
try:
    opts, args = getopt.getopt(sys.argv[1:],"hs:",["use_symlink="])
    opt, arg = opts[0]
    if arg.upper() == 'ON' or arg == '1':
        use_symlink = True
    elif arg.upper() == 'OFF' or arg == '0':
        use_symlink = False
    else:
        print_info('Usage: gen_ship_package.py --use_symlink <ON|OFF>')
        sys.exit()
except getopt.GetoptError:
    print_info('Usage: gen_ship_package.py --use_symlink <ON|OFF>')
    sys.exit(1)

##############################
#
# Script starts here
#
##############################
print_info("Script starts")

print_info("Assembling ship directory...")
print_info("  Setting USE_SYMLINK = " + ("ON" if use_symlink else "OFF"))

##############################
#
# Copy firmware files
#
##############################
print_info("Copying firmware code and BSP. ")

copy_dirs = ["code", "bcommon", "test", "pfr_tools"]
for dir in copy_dirs:
    copy_dir(os.path.join(fw_dir, dir), os.path.join(ship_fw_dir, dir), use_symlink)

# Copy BSP without symlinks
copy_dir(os.path.join(fw_dir, "bsp"), os.path.join(ship_fw_dir, "bsp"), False)

# Copy pfr_sys BSP to recovery_sys
copy_file(os.path.join(ship_fw_dir, "bsp", "pfr_sys", "settings.bsp"), os.path.join(ship_fw_dir, "bsp", "recovery_sys", "settings.bsp"), False)

# Patch the pfr_sys BSP as required
patch_file(os.path.join(ship_fw_dir, "bsp", "pfr_sys", "settings.bsp"), os.path.join(script_dir, "pfr_setting.bsp.patch"))
# Patch the recovery_sys BSP as required
patch_file(os.path.join(ship_fw_dir, "bsp", "recovery_sys", "settings.bsp"), os.path.join(script_dir, "recovery_setting.bsp.patch"))
# Copy patched pfr_sys BSP to pfr_sys_sim
copy_file(os.path.join(ship_fw_dir, "bsp", "pfr_sys", "settings.bsp"), os.path.join(ship_fw_dir, "bsp", "pfr_sys_sim", "settings.bsp"), False)
# Patch the pfr_sys_sim BSP as required
patch_file(os.path.join(ship_fw_dir, "bsp", "pfr_sys_sim", "settings.bsp"), os.path.join(script_dir, "pfr_sys_sim.settings.bsp.patch"))

# Copy directory-level files only (e.g. makefiles, eclipse setting files)
copy_dir(fw_dir, ship_fw_dir, use_symlink, recursive=False)

##############################
#
# Copy project files
#
##############################
print_info("Copying project files. ")

# Copy all files in impl/archer_city (non-recursive), except Makefile
root_files_to_copy = ["basic.qsf", "gen_asgns.qsf", "gen_gpi_signals_pkg.sv", "gen_gpo_controls_pkg.sv", "gen_gpo_controls_pkg.tcl", "gen_smbus_relay_config_pkg.sv", "non_pfr.cof", "non_pfr_core.sv", "non_pfr.qsf", "non_pfr.sdc", "non_pfr_top.sv", "pfr.cof", "pfr_core.sv", "pfr.qsf", "pfr.sdc", "pfr_top.sv", "platform_defs_pkg.sv", "top.qpf", "compile_non_pfr.sh", "compile_pfr.sh", "prep_revisions.sh", "jtag.sdc", "compile_recovery.sh", "recovery.qsf", "recovery_top.sv","compile_pfr_debug.sh","pfr_debug.qsf","pfr_debug_top.sv","pfr_debug.cof"]

for filename in root_files_to_copy:
    file_path = os.path.join(impl_dir, filename)
    if os.path.isfile(file_path):
        copy_file(file_path, os.path.join(ship_dir, filename), use_symlink)

##############################
#
# Copy source RTL files
#
##############################
print_info("Copying RTL source files. ")

# Go through QSFs to extract source files
src_rtl_files = list()
for filename in os.listdir(impl_dir):
    file_path = os.path.join(impl_dir, filename)
    if os.path.isfile(file_path) and filename.endswith("qsf"):
        # Open this QSF file for read
        with open(file_path) as qsf:
            line = qsf.readline()
            while line:
                if "_FILE" in line:
                    # Example line: set_global_assignment -name SYSTEMVERILOG_FILE src/crypto/crypto256_top.sv
                    src_file = line.split()[3].strip()
                    if src_file.startswith("src"):
                        src_rtl_files.append(src_file)
                line = qsf.readline()

# Copy those source files over to ship directory
for src_file in src_rtl_files:
    if "qip" in src_file:
        # Skip Qsys files
        continue
    src_filepath = os.path.join(script_dir, "../../", src_file)
    copy_file(src_filepath, os.path.join(ship_dir, src_file), use_symlink)

# Also copy src/common
copy_dir(os.path.join(src_dir, "common"), os.path.join(ship_src_dir, "common"), use_symlink)

# Copy src/core_cpld/archer_city_fab3/shim/* for simulation
copy_dir(os.path.join(src_dir, "core_cpld/archer_city_fab3/shim"), os.path.join(ship_src_dir, "core_cpld/archer_city_fab3/shim"), use_symlink, recursive=False)

# Specifically copy pfr_core to recovery_core
copy_file("pfr_core.sv", os.path.join(ship_dir, "recovery_core.sv"), False)
patch_file(os.path.join(ship_dir, "recovery_core.sv"), os.path.join(script_dir, "recovery_core.sv.patch"))

# Specifically copy pfr_core to pfr_debug_core
copy_file("pfr_core.sv", os.path.join(ship_dir, "pfr_debug_core.sv"), False)
patch_file(os.path.join(ship_dir, "pfr_debug_core.sv"), os.path.join(script_dir, "pfr_debug_core.sv.patch"))
##############################
#
# Copy bin/* files
# 
# Always copy the blocksign tool to ship directory
##############################
if use_symlink:
    copy_dir(os.path.join(script_dir, "../bin"), os.path.join(ship_dir, "bin"), False)
else: 
    copy_dir(os.path.join(script_dir, "../bin/blocksign_tool"), os.path.join(ship_dir, "bin/blocksign_tool"), False)
    
##############################
#
# Copy qsys file
#
##############################
print_info("Copying and upgrading Qsys systems. ")

# Qsys systems
qsys_mapping = {
                "top_ip": ["int_osc_ip.qsys", "dual_config_ip.qsys"],
                "pfr_sys": ["pfr_sys.qsys"]}

# Copy all qsys directories to package
for qsys_dir in qsys_mapping.keys():
    for qsys_sys_file in qsys_mapping[qsys_dir]:
        qsys_src_dir = os.path.join(src_dir, qsys_dir)
        qsys_dest_dir = os.path.join(ship_src_dir, qsys_dir)
        copy_qsys_dir(qsys_sys_file, qsys_src_dir, qsys_sys_file, qsys_dest_dir)

# Copy pfr_sys to recovery
copy_qsys_dir("pfr_sys.qsys", os.path.join(src_dir, "pfr_sys"), "recovery_sys.qsys", os.path.join(ship_src_dir, "recovery_sys"))

# Patch the pfr_sys as required
patch_file(os.path.join(ship_src_dir, "pfr_sys", "pfr_sys.qsys"), os.path.join(script_dir, "pfr_sys.qsys.patch"))

# Patch the recovery_sys as required
patch_file(os.path.join(ship_src_dir, "recovery_sys", "recovery_sys.qsys"), os.path.join(script_dir, "recovery_sys.qsys.patch"))

# Regenerate Qsys files
regen_mapping = qsys_mapping
regen_mapping["recovery_sys"] = ["recovery_sys.qsys"]
for qsys_dir in regen_mapping.keys():
    for qsys_sys_file in regen_mapping[qsys_dir]:
        qsys_src_dir = os.path.join(src_dir, qsys_dir)
        qsys_dest_dir = os.path.join(ship_src_dir, qsys_dir)
        qsys_f = os.path.join(qsys_dest_dir, qsys_sys_file)
        # Update device in the qsys system
        cmd = "qsys-generate --part=10M50DAF484C7G --upgrade-ip-cores " + qsys_f
        print cmd
        process = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE)
        output, error = process.communicate()

##############################
#
# Flatten QSFs when non using simplinks
#
##############################
if not use_symlink:
    flatten_qsf(ship_dir, "non_pfr.qsf")
    flatten_qsf(ship_dir, "pfr.qsf")
    flatten_qsf(ship_dir, "recovery.qsf")
    flatten_qsf(ship_dir, "pfr_debug.qsf")
    os.remove(os.path.join(ship_dir, "gen_asgns.qsf"))
    os.remove(os.path.join(ship_dir, "basic.qsf"))

##############################
#
# Make script executable
#
##############################
if not use_symlink:
    root_files_to_chmod = ["compile_non_pfr.sh", "compile_pfr.sh", "compile_recovery.sh", "compile_pfr_debug.sh", "prep_revisions.sh"]
    for filename in root_files_to_chmod:
        file_path = os.path.join(ship_dir, filename)
        cmd = "chmod u+x " + file_path
        process = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE)
        output, error = process.communicate()

##############################
#
# Create Makefile for the package when using simlinks
#
##############################

if use_symlink:
    ship_makefile = os.path.join(ship_dir, "Makefile")

    with open(ship_makefile, "w") as makefile:
        makefile.write(
        """ 
THIS_MK_ABSPATH := $(abspath $(lastword $(MAKEFILE_LIST)))
THIS_MK_DIR := $(patsubst %/,%,$(dir $(THIS_MK_ABSPATH)))
# Enable second expansion
.SECONDEXPANSION:

# Clear all built in suffixes
.SUFFIXES:

UNAME = $(shell uname -r)
ifeq ($(findstring Microsoft,$(UNAME)),Microsoft)
	WINDOWS_EXE = .exe
endif

##############################################################################
# Set default goal before any targets. The default goal here is "test"
##############################################################################
DEFAULT_TARGET := test

.DEFAULT_GOAL := default
.PHONY: default
default: $(DEFAULT_TARGET)

##############################################################################
# Project Definitions
##############################################################################
Q_PROJECT := top
Q_REVISION := top

PFR_CODE_HEX_SRC := fw/code/hw/archer_city/mem_init/u_ufm.hex
PFR_CODE_HEX_DEST := pfr_code.hex

RECOVERY_CODE_HEX_SRC := fw/code/hw/archer_city_recovery/mem_init/u_ufm.hex
RECOVERY_CODE_HEX_DEST := recovery_code.hex

CPLD_UPDATE_CAPSULE_SRC := output_files/pfr_cfm1_auto.rpd
CPLD_UPDATE_CAPSULE_SECP256_DEST := output_files/cpld_update_capsule_secp256.bin
CPLD_UPDATE_CAPSULE_SECP384_DEST := output_files/cpld_update_capsule_secp384.bin
CPLD_UPDATE_CAPSULE_UNSIGNED_DEST := output_files/cpld_update_capsule_unsigned.bin

CPLD_UPDATE_CAPSULE_CFM_IMAGE_SHA256_CHECKSUMS_DEST := output_files/cpld_update_capsule_cfm_image_sha256_checksums.txt
CPLD_UPDATE_CAPSULE_CFM_IMAGE_SHA384_CHECKSUMS_DEST := output_files/cpld_update_capsule_cfm_image_sha384_checksums.txt

##############################################################################
# Makefile starts here
##############################################################################
.PHONY: prep
prep: prep_done.txt

prep_done.txt:
	sh ./prep_revisions.sh

$(PFR_CODE_HEX_SRC): prep
$(PFR_CODE_HEX_DEST): $(PFR_CODE_HEX_SRC)
	cp $< $@

$(RECOVERY_CODE_HEX_SRC): prep
$(RECOVERY_CODE_HEX_DEST): $(RECOVERY_CODE_HEX_SRC)
	cp $< $@

.PHONY: compile
compile: prep pfr non_pfr recovery

output_files/pfr.pof : output_files/recovery.sof output_files/pfr.sof $(PFR_CODE_HEX_DEST) $(RECOVERY_CODE_HEX_DEST)
	quartus_cpf$(WINDOWS_EXE) -c pfr.cof

output_files/non_pfr.pof : output_files/non_pfr.sof
	quartus_cpf$(WINDOWS_EXE) -c non_pfr.cof

output_files/%.sof : | prep
	$(info Compiling design to $@)
	sh ./compile_$(notdir $*).sh
	perl bin/check_timing.pl $(Q_PROJECT) $(notdir $*)
	quartus_sh$(WINDOWS_EXE) -t bin/check_power.tcl $(Q_PROJECT) $(notdir $*)

.PHONY: pfr
pfr : prep output_files/pfr.pof

.PHONY: non_pfr
non_pfr : prep output_files/non_pfr.pof

.PHONY: recovery
recovery : prep output_files/recovery.sof

$(CPLD_UPDATE_CAPSULE_SRC) : pfr

$(CPLD_UPDATE_CAPSULE_SECP256_DEST) : $(CPLD_UPDATE_CAPSULE_SRC)
	make -C bin/blocksign_tool/source 
	bin/blocksign_tool/source/blocksign -c bin/blocksign_tool/config_xmls/cpld_update_capsule.xml -o $(CPLD_UPDATE_CAPSULE_SECP256_DEST) $(CPLD_UPDATE_CAPSULE_SRC)
	mv $(CPLD_UPDATE_CAPSULE_SRC)_aligned $(CPLD_UPDATE_CAPSULE_UNSIGNED_DEST)

$(CPLD_UPDATE_CAPSULE_SECP384_DEST) : $(CPLD_UPDATE_CAPSULE_SRC)
	make -C bin/blocksign_tool/source 
	bin/blocksign_tool/source/blocksign -c bin/blocksign_tool/config_xmls/cpld_update_capsule_secp384r1.xml -o $(CPLD_UPDATE_CAPSULE_SECP384_DEST) $(CPLD_UPDATE_CAPSULE_SRC)
	mv $(CPLD_UPDATE_CAPSULE_SRC)_aligned $(CPLD_UPDATE_CAPSULE_UNSIGNED_DEST)

# This target generates the SHA256/SHA384 checksums of the CFM image inside the update capsule. 
# Note that the update capsule contains a SVN field besides the CFM image. Hence, we cannot simply hash the update capsule protected content. 
.PHONY: generate_cfm1_hashes
generate_cfm1_hashes: $(CPLD_UPDATE_CAPSULE_SECP256_DEST) $(CPLD_UPDATE_CAPSULE_SECP384_DEST)
	bin/blocksign_tool/source/blocksign -c bin/blocksign_tool/config_xmls/cpld_update_capsule_without_svn.xml -o tmp.bin $(CPLD_UPDATE_CAPSULE_SRC)
	sha256sum $(CPLD_UPDATE_CAPSULE_SRC)_aligned | head -c 64 > $(CPLD_UPDATE_CAPSULE_CFM_IMAGE_SHA256_CHECKSUMS_DEST)
	sha384sum $(CPLD_UPDATE_CAPSULE_SRC)_aligned | head -c 128 > $(CPLD_UPDATE_CAPSULE_CFM_IMAGE_SHA384_CHECKSUMS_DEST)
	rm $(CPLD_UPDATE_CAPSULE_SRC)_aligned tmp.bin


.PHONY: clean
clean:
	$(info Cleaning Project)
	-rm -rf output_files db incremental_db reg.rout *.qws
	-rm -rf u_ufm.hex

.PHONY: deepclean
deepclean: clean
	$(info Deep Cleaning Project)
	-$(MAKE) -C $(FW_CODE_DIR) clean
	-$(MAKE) -C $(FW_BSP_DIR) clean
	-rm -rf $(SRC_DIR)/pfr_sys/pfr_sys.sopcinfo $(SRC_DIR)/pfr_sys/pfr_sys
	
.PHONY: test
test: compile generate_cfm1_hashes

.PHONY: claim
claim:
	-arc shell board/bench/wilsoncity/005

.PHONY: powercycle
powercycle:
	../../bin/powercycle_platform.sh

.PHONY: erase.pgm
erase.pgm:
	quartus_pgm$(WINDOWS_EXE) --cable=1 -m JTAG -o "rb;output_files/pfr.pof@1"

.PHONY: pfr.pgm
pfr.pgm: output_files/pfr.pof
	quartus_pgm$(WINDOWS_EXE) --cable=1 -m JTAG -o "pv;output_files/pfr.pof@1"

.PHONY: non_pfr.pgm
non_pfr.pgm: output_files/non_pfr.pof
	quartus_pgm$(WINDOWS_EXE) --cable=1 -m JTAG -o "pv;output_files/non_pfr.pof@1"

.PHONY: check
check: prep
	quartus_map$(WINDOWS_EXE) $(Q_PROJECT) -c pfr
	quartus_map$(WINDOWS_EXE) $(Q_PROJECT) -c non_pfr

.PHONY: bootcheck
bootcheck:
	chmod u+wx bin/check_bios_messages.pl
	rsync -avz -e ssh bin/check_bios_messages.pl ttolab-prr018-r6.tor.intel.com:/build/$(USER)/wilson_city/
	ssh ttolab-prr018-r6.tor.intel.com /build/$(USER)/wilson_city/check_bios_messages.pl
        """
        )
else:
    ship_makefile = os.path.join(ship_dir, "Makefile")

    with open(ship_makefile, "w") as makefile:
        makefile.write(
        """ 
THIS_MK_ABSPATH := $(abspath $(lastword $(MAKEFILE_LIST)))
THIS_MK_DIR := $(patsubst %/,%,$(dir $(THIS_MK_DIR)))
# Enable second expansion
.SECONDEXPANSION:

# Clear all built in suffixes
.SUFFIXES:

UNAME = $(shell uname -r)
ifeq ($(findstring Microsoft,$(UNAME)),Microsoft)
	WINDOWS_EXE = .exe
endif

##############################################################################
# Set default goal before any targets. The default goal here is "compile"
##############################################################################
DEFAULT_TARGET := compile

.DEFAULT_GOAL := default
.PHONY: default
default: $(DEFAULT_TARGET)

##############################################################################
# Makefile starts here
##############################################################################

.PHONY: prep
prep: prep_done.txt

prep_done.txt:
	sh ./prep_revisions.sh

.PHONY: compile_fw
compile_fw:
	cd fw/code/hw/archer_city/;make clean
	cd fw/code/hw/archer_city_recovery/;make clean
	make -C fw/code/hw/archer_city/
	make -C fw/code/hw/archer_city_recovery/
	cp fw/code/hw/archer_city/mem_init/u_ufm.hex ./pfr_code.hex
	cp fw/code/hw/archer_city_recovery/mem_init/u_ufm.hex ./recovery_code.hex
	quartus_cpf$(WINDOWS_EXE) -c pfr.cof


.PHONY: compile
compile: prep 
	sh ./compile_non_pfr.sh
	sh ./compile_recovery.sh
	sh ./compile_pfr.sh
	sh ./compile_pfr_debug.sh
# Create the signed/unsigned CPLD update capsule
	make -C bin/blocksign_tool/source 
	bin/blocksign_tool/source/blocksign -c bin/blocksign_tool/config_xmls/cpld_update_capsule.xml -o output_files/cpld_update_capsule.bin output_files/pfr_cfm1_auto.rpd
	bin/blocksign_tool/source/blocksign -c bin/blocksign_tool/config_xmls/cpld_update_capsule_secp384r1.xml -o output_files/cpld_update_capsule_secp384.bin output_files/pfr_cfm1_auto.rpd
	bin/blocksign_tool/source/blocksign -c bin/blocksign_tool/config_xmls/cpld_update_capsule_secp384r1_svn1.xml -o output_files/cpld_update_capsule_secp384_svn1.bin output_files/pfr_cfm1_auto.rpd
	bin/blocksign_tool/source/blocksign -c bin/blocksign_tool/config_xmls/cpld_update_capsule_secp384r1_svn2.xml -o output_files/cpld_update_capsule_secp384_svn2.bin output_files/pfr_cfm1_auto.rpd
	mv output_files/pfr_cfm1_auto.rpd_aligned output_files/cpld_update_capsule_unsigned.bin
# This target generates the SHA256/SHA384 checksums of the CFM image inside the update capsule. 
# Note that the update capsule contains a SVN field besides the CFM image. Hence, we cannot simply hash the update capsule protected content. 
	bin/blocksign_tool/source/blocksign -c bin/blocksign_tool/config_xmls/cpld_update_capsule_without_svn.xml -o tmp.bin output_files/pfr_cfm1_auto.rpd
	sha256sum output_files/pfr_cfm1_auto.rpd_aligned | head -c 64 > output_files/cpld_update_capsule_cfm_image_sha256_checksums.txt
	sha384sum output_files/pfr_cfm1_auto.rpd_aligned | head -c 128 > output_files/cpld_update_capsule_cfm_image_sha384_checksums.txt
	rm output_files/pfr_cfm1_auto.rpd_aligned tmp.bin
        """
        )


##############################
#
# Script ends
#
##############################

print_info("Script completed ")




