# Enable second expansion
.SECONDEXPANSION:

# Clear all built in suffixes
.SUFFIXES:

# Ensure that command failures from tee are returned
SHELL = /bin/bash -o pipefail

# Determine the current directory based on this Makefile
THIS_MAKEFILE_ABSPATH := $(abspath $(lastword $(MAKEFILE_LIST)))
THIS_MAKEFILE_DIR := $(patsubst %/,%,$(dir $(THIS_MAKEFILE_ABSPATH)))

UNAME = $(shell uname -r)
ifeq ($(findstring Microsoft,$(UNAME)),Microsoft)
	WINDOWS_EXE = .exe
endif

##############################################################################
# Env Check
##############################################################################
ifeq ($(PFR_ROT_CPLD_SRC_ROOT),)
$(error PFR_ROT_CPLD_SRC_ROOT not defined)
endif
ifeq ($(PFR_ROT_CPLD_DEST_ROOT),)
$(error PFR_ROT_CPLD_DEST_ROOT not defined)
endif
ifeq ($(PFR_ROT_CPLD_WORK_ROOT),)
$(error PFR_ROT_CPLD_WORK_ROOT not defined)
endif

##############################################################################
# Derive directories
##############################################################################

# The module path is the current directory minus the SRC_ROOT prefix
MODULE_PATH := $(subst $(PFR_ROT_CPLD_SRC_ROOT)/,,$(THIS_MAKEFILE_DIR))

# The bcommon dir is defined as the directory of this file
BCOMMON_ROOT := $(abspath $(PFR_ROT_CPLD_SRC_ROOT)/bcommon)

# Work root is a subdir of this dir
WORK_DIR := $(abspath $(PFR_ROT_CPLD_WORK_ROOT)/$(MODULE_PATH))

# Build dest dir
DEST_DIR := $(abspath $(PFR_ROT_CPLD_DEST_ROOT)/$(MODULE_PATH))

SHIP_DIR := $(abspath ship)

BUILD_OUTPUT_FILE := $(abspath build_output.txt)

AUTOBUILD_DIRS := \
	$(WORK_DIR) \
	$(DEST_DIR)

##############################################################################


REVISIONS_TO_BUILD := \
	non_pfr \
	recovery \
	pfr \
	pfr_debug \

SOFS_TO_BUILD = $(addprefix ship/output_files/,$(addsuffix .sof,$(REVISIONS_TO_BUILD)))
ship/output_files/pfr.sof : ship/output_files/recovery.sof

NUM_PROCS := $(shell nproc)
ifeq ($(NUM_PROCS),)
	NUM_PROCS := 4
endif

GEN_SRC_DIR := gen_src
USE_SYMLINK := OFF

##############################################################################
# Set default goal before any targets. The default goal here is "test"
##############################################################################
DEFAULT_TARGET := test

.DEFAULT_GOAL := default
.PHONY: default
default: $(DEFAULT_TARGET)


# Helper Targets
$(AUTOBUILD_DIRS) :
	mkdir -p $@

##############################################################################
# Toplevel targets
.PHONY: ship
ship : $(WORK_DIR)/ship_target.done

.PHONY: devel
devel :
	$(MAKE) ship USE_SYMLINK=ON

.PHONY: clean
clean: clean_gen_files
	$(info Cleaning ship directory)
	-rm -rf $(SHIP_DIR) $(DEST_DIR) $(WORK_DIR) build_output.txt

.PHONY: dest_clean
dest_clean :
	$(info Cleaning destination directory)
	-rm -rf $(DEST_DIR)

.PHONY: prep
prep: $(WORK_DIR)/ship_target.done
	$(MAKE) -C ship prep |& tee $(BUILD_OUTPUT_FILE)

.PHONY: test
test: deploy
	$(info Running unit tests with $(NUM_PROCS) jobs)
#	$(MAKE) -j $(NUM_PROCS) -C ship/fw/test

.PHONY: test-full
test-full: deploy
	$(info Running unit tests with $(NUM_PROCS) jobs)
	# Run unittest in Crypto256 mode
	$(MAKE) -C ship/fw/test test-full
	$(MAKE) -C ship/fw/test clean
	# Run unittest in Crypto384 mode and Attestation in Crypto384 mode
	$(MAKE) -C ship/fw/test test-full GTEST_USE_CRYPTO_384=1 GTEST_ATTEST_384=1 GTEST_ENABLE_SEAMLESS_FEATURES=1

.PHONY: compile
compile: prep $(SOFS_TO_BUILD)
	$(info [INFO] Completed building $(REVISIONS_TO_BUILD))

.PHONY: deploy
deploy: $(WORK_DIR)/deploy.done

##############################################################################
# Internal targets
$(WORK_DIR)/gen_files.done : | $(WORK_DIR)
	$(MAKE) -C $(GEN_SRC_DIR) 
	touch $@

$(WORK_DIR)/ship_target.done : $(WORK_DIR)/gen_files.done $(WORK_DIR)/gen_ship_package.done | $(WORK_DIR)
	$(info [INFO] Created ship dir)
	touch $@

$(WORK_DIR)/gen_ship_package.done : | $(WORK_DIR)
	python gen_ship_package.py --use_symlink $(USE_SYMLINK)
	touch $@

# Compile and check timing/power/code size
ship/output_files/%.sof : $(WORK_DIR)/ship_target.done | $(WORK_DIR)
	$(info Building $*)
	cd ship && sh compile_$*.sh |& tee -a $(BUILD_OUTPUT_FILE)
	cd ship && perl ../bin/check_timing.pl top $* |& tee -a $(BUILD_OUTPUT_FILE)
	cd ship && quartus_sh$(WINDOWS_EXE) -t ../bin/check_power.tcl top $* |& tee -a $(BUILD_OUTPUT_FILE)
	# Ensure the code is less than 65535-600 = 64935
	# Reserved 600 Bytes for stack. Re-run static analysis to evaluate actual stack usage when the design is mature.  
	#cd ship && perl ../bin/check_code_size.pl --elf_file=fw/code/hw/archer_city/main.elf --size_in_bytes=64935 |& tee -a $(BUILD_OUTPUT_FILE)
	#cd ship && perl ../bin/check_code_size.pl --elf_file=fw/code/hw/archer_city/main.elf --size_in_bytes=64935 |& tee -a $(BUILD_OUTPUT_FILE)
	#cd ship/fw/worst_case_stack && sh pfr_wcs_script.sh |& tee -a $(BUILD_OUTPUT_FILE)
	-rm -f ship/output_files/parse_* ship/output_files/print_enable.txt ship/output_files/*.parsed_experiment ship/output_files/compiled_parse_rules.pl ship/output_files/top.job.conf ship/output_files/..parsed_experiment

$(WORK_DIR)/deploy.done : $(SOFS_TO_BUILD) | $(WORK_DIR) $(DEST_DIR)
	# Create the signed/unsigned CPLD update capsule
	cd ship && make -C bin/blocksign_tool/source 
	cd ship && bin/blocksign_tool/source/blocksign -c bin/blocksign_tool/config_xmls/cpld_update_capsule.xml -o output_files/cpld_update_capsule_secp256.bin output_files/pfr_cfm1_auto.rpd 
	cd ship && bin/blocksign_tool/source/blocksign -c bin/blocksign_tool/config_xmls/cpld_update_capsule_secp384r1.xml -o output_files/cpld_update_capsule_secp384.bin output_files/pfr_cfm1_auto.rpd
	cd ship && bin/blocksign_tool/source/blocksign -c bin/blocksign_tool/config_xmls/cpld_update_capsule_secp384r1_svn1.xml -o output_files/cpld_update_capsule_secp384_svn1.bin output_files/pfr_cfm1_auto.rpd
	cd ship && bin/blocksign_tool/source/blocksign -c bin/blocksign_tool/config_xmls/cpld_update_capsule_secp384r1_svn2.xml -o output_files/cpld_update_capsule_secp384_svn2.bin output_files/pfr_cfm1_auto.rpd
	cd ship && mv output_files/pfr_cfm1_auto.rpd_aligned output_files/cpld_update_capsule_unsigned.bin
	# Calculate and save the CFM1 hash 
	cd ship && bin/blocksign_tool/source/blocksign -c bin/blocksign_tool/config_xmls/cpld_update_capsule_without_svn.xml -o tmp.bin output_files/pfr_cfm1_auto.rpd
	cd ship && sha256sum output_files/pfr_cfm1_auto.rpd_aligned | head -c 64 > output_files/cpld_update_capsule_cfm_image_sha256_checksums.txt
	cd ship && sha384sum output_files/pfr_cfm1_auto.rpd_aligned | head -c 128 > output_files/cpld_update_capsule_cfm_image_sha384_checksums.txt
	cd ship && rm output_files/pfr_cfm1_auto.rpd_aligned tmp.bin
	# Assemble deliverables (cpld_pfr.zip, output_files, quartus_project.tar.gz)
	zip -j cpld_pfr.zip ship/output_files/pfr.pof ship/output_files/cpld_update_capsule_secp256.bin ship/output_files/cpld_update_capsule_secp384.bin
	cp -fr ship/output_files $(DEST_DIR)
	gzip -r $(DEST_DIR)/*
	mv cpld_pfr.zip $(DEST_DIR)/cpld_pfr.zip
	cd ship && tar -czf $(DEST_DIR)/quartus_project.tar.gz *
	touch $@

.PHONY: clean_gen_files
clean_gen_files :
	$(info [INFO] Cleaning generated files)
	$(MAKE) -C $(GEN_SRC_DIR) clean




