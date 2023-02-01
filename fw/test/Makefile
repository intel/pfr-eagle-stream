# How does this makefile work:
#
# This makefile uses a modular definition to the rules to allow the building
# of the same test files but with different build options and compilers. This
# is enabled by first specifing the module specific options, then souring
# the cpp_rules and test_exe_rules Makefiles. The module options are used 
# to create new rules based on the MODULE_NAME variable

# Enable second expansion
.SECONDEXPANSION:

# Clear all built in suffixes
.SUFFIXES:

# Ensure that command failures from tee are returned
SHELL = /bin/bash -o pipefail

# Include the common files
THIS_MAKEFILE_DIR := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
include $(THIS_MAKEFILE_DIR)/common.mk

##############################################################################
# Google Test settings
##############################################################################
GTEST_ROOT = $(ARC_TOOLS)/google_gtest/1.8.0/1/linux64
GTEST_SOURCE_ROOT = $(GTEST_ROOT)/src
GTEST_INC_ROOT = $(GTEST_ROOT)/include
GTEST_SOURCE_FILES = \
	gmock-all.cpp \
	gtest-all.cpp \
	
GTEST_OBJ_LIST = \
	gmock-all.obj \
	gtest-all.obj

##############################################################################
# Boost settings
##############################################################################
BOOST_ROOT_DIR = $(ARC_TOOLS)/boost/1.69/1/linux64/rel

##############################################################################
# Directories
##############################################################################
# This is where we store the firmware models
SYSTEM_DIR := system

# This is where we store the unittests
UNITTEST_DIR := unittests

##############################################################################
# Global CPP and LD flags affecting all targets
##############################################################################
# Add Gtest to the include
GLOBAL_CPPFLAGS += -I$(GTEST_INC_ROOT) -I$(GTEST_ROOT)

GLOBAL_CPPFLAGS += -I$(BOOST_ROOT_DIR)/include -DBOOST_STACKTRACE_USE_ADDR2LINE
GLOBAL_LDFLAGS += -L$(BOOST_ROOT_DIR)/lib

# Add SYSTEM_MOCK 
GLOBAL_CPPFLAGS += -I. -I$(SYSTEM_DIR)

# Add pfr system directory for header files to include
GLOBAL_CPPFLAGS += -I../code/inc

# Remove dependency on the BSP
GLOBAL_CPPFLAGS += -DPFR_NO_BSP_DEP -DPFR_DEBUG_MODE

# Include BSP directoroes
BSP_DIR := ../bsp/pfr_sys/bsp
GLOBAL_CPPFLAGS += -I$(BSP_DIR)  -I$(BSP_DIR)/HAL/inc  -I$(BSP_DIR)/drivers/inc 

# Add system libs
GLOBAL_LDFLAGS += -ldl -lcrypto

##############################################################################
# Source file OBJs
##############################################################################
MAIN_SYSTEM_OBJS_LIST = \
	$(SYSTEM_DIR)/system_mock.obj \
	$(SYSTEM_DIR)/mailbox_mock.obj \
	$(SYSTEM_DIR)/crypto_mock.obj \
	$(SYSTEM_DIR)/smbus_relay_mock.obj \
	$(SYSTEM_DIR)/rfnvram_mock.obj \
	$(SYSTEM_DIR)/timer_mock.obj \
	$(SYSTEM_DIR)/spi_flash_mock.obj \
	$(SYSTEM_DIR)/ufm_mock.obj \
	$(SYSTEM_DIR)/spi_control_mock.obj \
	$(SYSTEM_DIR)/dual_config_mock.obj \
	$(SYSTEM_DIR)/nios_gpio_mock.obj \

MAIN_UNITTEST_OBJS_LIST = \
	$(UNITTEST_DIR)/test_sanity.obj \
	$(UNITTEST_DIR)/test_ufm.obj \
	$(UNITTEST_DIR)/test_global_state.obj \
	$(UNITTEST_DIR)/test_mailbox_utils.obj \
	$(UNITTEST_DIR)/test_initialization.obj \
	$(UNITTEST_DIR)/test_platform_log.obj \
	$(UNITTEST_DIR)/test_pfm.obj \
	$(UNITTEST_DIR)/test_system_mock.obj \
	$(UNITTEST_DIR)/test_t0_routines.obj \
	$(UNITTEST_DIR)/test_timer_utils.obj \
	$(UNITTEST_DIR)/test_tmin1_routines.obj \
	$(UNITTEST_DIR)/test_utils.obj \
	$(UNITTEST_DIR)/test_key_cancellation.obj \
	$(UNITTEST_DIR)/test_keychain.obj \
	$(UNITTEST_DIR)/test_keychain_utils.obj \
	$(UNITTEST_DIR)/test_authentication.obj \
	$(UNITTEST_DIR)/test_authentication_neg.obj \
	$(UNITTEST_DIR)/test_smbus_relay_utils.obj \
	$(UNITTEST_DIR)/test_rfnvram_utils.obj \
	$(UNITTEST_DIR)/test_spi_filter.obj \
	$(UNITTEST_DIR)/test_ufm_utils.obj \
	$(UNITTEST_DIR)/test_ufm_provisioning.obj \
	$(UNITTEST_DIR)/test_spi_rw.obj \
	$(UNITTEST_DIR)/test_timed_boot.obj \
	$(UNITTEST_DIR)/test_flows.obj \
	$(UNITTEST_DIR)/test_decompression_utils.obj \

MAIN_T2_UNITTEST_OBJS_LIST = \
	$(UNITTEST_DIR)/test_testdata.obj \
	$(UNITTEST_DIR)/test_tmin1_routines.obj \
	$(UNITTEST_DIR)/test_flash_validation.obj \
	$(UNITTEST_DIR)/test_capsule_validation.obj \
	$(UNITTEST_DIR)/test_provisioned_flows.obj \
	$(UNITTEST_DIR)/test_tmin1_authentication_flow.obj \
	$(UNITTEST_DIR)/test_wdt_recovery_flow.obj \
	$(UNITTEST_DIR)/test_decompression_flow.obj \
	$(UNITTEST_DIR)/test_key_cancellation_flow.obj \
	$(UNITTEST_DIR)/test_fw_recovery_flow.obj \
	$(UNITTEST_DIR)/test_fw_recovery_through_update.obj \
	$(UNITTEST_DIR)/test_t0_corruption.obj \
	$(UNITTEST_DIR)/test_mix_and_match_attack.obj \
	$(UNITTEST_DIR)/test_fw_update_flow_neg.obj \
	$(UNITTEST_DIR)/test_fw_update_flow.obj \
	$(UNITTEST_DIR)/test_cpld_update_flow.obj \
	$(UNITTEST_DIR)/test_cpld_recovery_flow.obj \
	$(UNITTEST_DIR)/test_decommission_flow.obj \
	$(UNITTEST_DIR)/test_update_flow.obj \
	$(UNITTEST_DIR)/test_anti_rollback_with_csk_id.obj \
	$(UNITTEST_DIR)/test_anti_rollback_with_svn.obj \
	$(UNITTEST_DIR)/test_protect_in_transit.obj \

MAIN_T3_UNITTEST_OBJS_LIST = \
	$(UNITTEST_DIR)/test_pfr_debug_images.obj \

# main_no_bsp_mock
MAIN_NO_BSP_MOCK_OBJS_LIST = \
	main.obj \
	$(UNITTEST_DIR)/test_no_bsp_mock.obj

##############################################################################
# Attestation unit tests options
##############################################################################
GTEST_ATTEST_384 ?= 0
GTEST_ATTEST_256 ?= 0
GTEST_SPDM_DEBUG ?= 0

ifeq ($(GTEST_ATTEST_384),1)
GLOBAL_CPPFLAGS += -DGTEST_ATTEST_384
GLOBAL_CPPFLAGS += -DENABLE_ATTESTATION_WITH_ECDSA_384
GTEST_USE_CRYPTO_384 = 1
MAIN_UNITTEST_OBJS_LIST += \
	$(UNITTEST_DIR)/test_attestation_basic_384.obj \
	$(UNITTEST_DIR)/test_mctp_protocol.obj \
	$(UNITTEST_DIR)/test_mctp_flow.obj \
	$(UNITTEST_DIR)/test_mctp_flow_neg.obj \
	$(UNITTEST_DIR)/test_spdm_protocol.obj \
	$(UNITTEST_DIR)/test_spdm_flow.obj \
	$(UNITTEST_DIR)/test_spdm_flow_neg.obj \
	$(UNITTEST_DIR)/test_spdm_utils.obj \
	$(UNITTEST_DIR)/test_t0_attestation_flow.obj \
	$(UNITTEST_DIR)/test_requester_spdm_error_handler.obj \
	$(UNITTEST_DIR)/test_afm_authentication.obj \
	$(UNITTEST_DIR)/test_afm_recovery_flow.obj \
	$(UNITTEST_DIR)/test_t0_attestation_flow_neg.obj \
	$(UNITTEST_DIR)/test_spdm_protocol_neg.obj \
	$(UNITTEST_DIR)/test_responder_spdm_error_handler.obj \
	$(UNITTEST_DIR)/test_afm_update_flow.obj \
	$(UNITTEST_DIR)/test_afm_update_flow_neg.obj \
	$(UNITTEST_DIR)/test_cert_utils.obj \
	$(UNITTEST_DIR)/test_cert_flow.obj \
	$(UNITTEST_DIR)/test_afm_authentication_neg.obj \

MAIN_T2_UNITTEST_OBJS_LIST += \
	$(UNITTEST_DIR)/test_attestation_384_crypto.obj \
	$(UNITTEST_DIR)/test_attestation_384_crypto_neg.obj \

else ifeq ($(GTEST_ATTEST_256),1)
GLOBAL_CPPFLAGS += -DGTEST_ATTEST_256
GLOBAL_CPPFLAGS += -DENABLE_ATTESTATION_WITH_ECDSA_256
GTEST_USE_CRYPTO_384 = 0
MAIN_UNITTEST_OBJS_LIST += \
	$(UNITTEST_DIR)/test_attestation_basic_256.obj \
	$(UNITTEST_DIR)/test_mctp_protocol.obj \
	$(UNITTEST_DIR)/test_mctp_flow.obj \
	$(UNITTEST_DIR)/test_mctp_flow_neg.obj \
	$(UNITTEST_DIR)/test_spdm_protocol.obj \
	$(UNITTEST_DIR)/test_spdm_flow.obj \
	$(UNITTEST_DIR)/test_spdm_flow_neg.obj \
	$(UNITTEST_DIR)/test_spdm_utils.obj \
	$(UNITTEST_DIR)/test_t0_attestation_flow.obj \
	$(UNITTEST_DIR)/test_requester_spdm_error_handler.obj \
	$(UNITTEST_DIR)/test_afm_authentication.obj \

MAIN_T2_UNITTEST_OBJS_LIST += \
	$(UNITTEST_DIR)/test_attestation_256_crypto.obj \
	$(UNITTEST_DIR)/test_attestation_256_crypto_neg.obj \

endif 

ifeq ($(GTEST_SPDM_DEBUG),1)
GLOBAL_CPPFLAGS += -DGTEST_ATTEST_384
GLOBAL_CPPFLAGS += -DENABLE_ATTESTATION_WITH_ECDSA_384
GLOBAL_CPPFLAGS += -DSPDM_DEBUG
GTEST_USE_CRYPTO_384 = 1
MAIN_UNITTEST_OBJS_LIST += \
	$(UNITTEST_DIR)/test_spdm_debug.obj \

endif

ifeq ($(GTEST_DEBUG_384),1)

GLOBAL_CPPFLAGS += -DUSE_384_HASH
GLOBAL_CPPFLAGS += -DGTEST_USE_CRYPTO_384
GLOBAL_CPPFLAGS += -DGTEST_ATTEST_384
GLOBAL_CPPFLAGS += -DENABLE_ATTESTATION_WITH_ECDSA_384
GLOBAL_CPPFLAGS += -DPLATFORM_SEAMLESS_FEATURES_ENABLED -DGTEST_ENABLE_SEAMLESS_FEATURES
GTEST_USE_CRYPTO_384 = 1

endif

##############################################################################
# Test against Seamless Reference Design
##############################################################################
# Option for testing against Seamless Reference Design
GTEST_ENABLE_SEAMLESS_FEATURES ?= 0

# Additional unittests for Seamless Reference Design
ifeq ($(GTEST_ENABLE_SEAMLESS_FEATURES),1)

# Add compiler option
GLOBAL_CPPFLAGS += -DPLATFORM_SEAMLESS_FEATURES_ENABLED -DGTEST_ENABLE_SEAMLESS_FEATURES

MAIN_UNITTEST_OBJS_LIST += \
	$(UNITTEST_DIR)/test_seamless_spi_geo_provisioning.obj \
        $(UNITTEST_DIR)/test_seamless_spi_geo_update_flow.obj \
        $(UNITTEST_DIR)/test_seamless_spi_geo_update_flow_neg.obj \

MAIN_T2_UNITTEST_OBJS_LIST += \
	$(UNITTEST_DIR)/test_seamless_update_flow.obj \
	$(UNITTEST_DIR)/test_seamless_update_flow_neg.obj \
	$(UNITTEST_DIR)/test_anti_rollback_with_csk_id_seamless.obj \
	$(UNITTEST_DIR)/test_anti_rollback_with_svn_seamless.obj \
	$(UNITTEST_DIR)/test_mix_and_match_attack_seamless.obj \
	$(UNITTEST_DIR)/test_fw_update_flow_seamless.obj \
	
# Only compile unittest in Crypto384 mode for now
# TODO: Remove this and add Seamless files signed with secp256 curve
GTEST_USE_CRYPTO_384 = 1
# TODO: Seamless 256 images for "test_seamless_update_flow" and "test_seamless_update_flow_neg" added.
#       WIP for remaining 4 seamless 256 unittest images.
#       For now, to run seamless 256 unittests, manually remove the 4 seamless uniittests from this Makefile and set GTEST_USE_CRYPTO_384=0.
endif # GTEST_ENABLE_SEAMLESS_FEATURES

##############################################################################
# Hashing/Signing Algorithm
# Test against 256bit or 384bit Crypto mode.
##############################################################################
# Option for testing with secp256 or secp384 curve signed files. 
GTEST_USE_CRYPTO_384 ?= 0

ifeq ($(GTEST_USE_CRYPTO_384),1)
GLOBAL_CPPFLAGS += -DGTEST_USE_CRYPTO_384
GLOBAL_CPPFLAGS += -DUSE_384_HASH
MAIN_UNITTEST_OBJS_LIST += \
	$(UNITTEST_DIR)/test_crypto_384.obj \

else 
# CRYPTO_256 mode
MAIN_UNITTEST_OBJS_LIST += \
	$(UNITTEST_DIR)/test_crypto_256.obj \

endif # GTEST_USE_CRYPTO_384

#############################################################################
# Increase timeout for LCOV in unnittest
#############################################################################
GTEST_RUNNING_LCOV ?= 0

ifeq ($(GTEST_RUNNING_LCOV),1)
GLOBAL_CPPFLAGS += -DGTEST_RUNNING_LCOV
GTEST_RUNNING_LCOV = 1

endif ## GTEST_RUNNING_LCOV

##############################################################################
# Test executable targets
##############################################################################
TEST_EXE_NAMES := 

##############################################################################
# Set default goal before any targets. The default goal here is "all"
##############################################################################
.PHONY: all
all: test-t1 test-t2

##############################################################################
# Global clean target
##############################################################################
.PHONY: clean
clean :
	$(RMDIR) $(WORK_DIR)
	$(RM) $(TEST_EXE_NAMES)
	$(RMDIR) $(COV_REPORT) *.lcov

##############################################################################
# Generic Targets
##############################################################################

# Copy google test source into work
$(addprefix $(WORK_DIR)/,$(patsubst %.obj,%.cc,$(GTEST_OBJ_LIST))) : $(WORK_DIR)/%.cc : $(GTEST_SOURCE_ROOT)/%.cc | $(WORK_DIR)
	$(COPY) $< $@

.PHONY: test-t1
test-t1 : run_asan_ubsan_main_no_bsp_mock run_asan_ubsan_main

.PHONY: test-t2
test-t2 : run_asan_ubsan_main_t2

# This target is automatically incremented based on the rules generated
.PHONY: test-full
test-full :

# Same as test-full but only runs the main exes
.PHONY: test-full-main
test-full-main :

.PHONY: test-t3
test-t3 : run_asan_ubsan_main_t3

##############################################################################
# Define the GCC with asan and ubsan target
##############################################################################
# Clear any existing variables and initalize them
include $(THIS_MAKEFILE_DIR)/init_module.mk

# Compilation variables for this module
COMPILER := gcc
MODULE_NAME := asan_ubsan

# Suppress unused function and unused parameter warnings
WARNING_EXCEPTIONS := -Wno-unused-function -Wno-unused-parameter

# Use max optimizations
CPP_OPT := -O3

# Always enable address sanitizer to catch array out of bounds and other such violations
# Set the initial link library to asan to ensure it is first in the link list. This is
# required by asan
CPPFLAGS += -fsanitize=address
INITIAL_LINK_LIBRARY := -lasan 

# Option to enable Undefined Behaviour sanitizer
ifeq ($(USE_UBSAN),1)
	CPPFLAGS += -fsanitize=undefined
	LDFLAGS += -lubsan
endif

# By default cause the test exes to exit if a sanitizer error is found
ifneq ($(SANITIZER_RECOVERY),1)
	CPPFLAGS += -fno-sanitize-recover=all
endif


ifeq ($(USE_UBSAN),1)
	CPPFLAGS += -fsanitize=undefined
	LDFLAGS += -lubsan
endif

# Source the file to define the cpp rules
include $(THIS_MAKEFILE_DIR)/cpp_rules.mk

# Source the file to define the test exe rules
include $(THIS_MAKEFILE_DIR)/test_exe_rules.mk

##############################################################################
# Define the GCC with thread sanitizer
##############################################################################
# Clear any existing variables and initalize them
include $(THIS_MAKEFILE_DIR)/init_module.mk

# Compilation variables for this module
COMPILER := gcc
MODULE_NAME := tsan

# Suppress unused function and unused parameter warnings
WARNING_EXCEPTIONS := -Wno-unused-function -Wno-unused-parameter

# Use max optimizations
CPP_OPT := -O3

# Enable the thread sanitizer
CPPFLAGS += -fsanitize=thread
LDFLAGS += -ltsan

# By default cause the test exes to exit if a sanitizer error is found
ifneq ($(SANITIZER_RECOVERY),1)
	CPPFLAGS += -fno-sanitize-recover=all
endif

# Source the file to define the cpp rules
include $(THIS_MAKEFILE_DIR)/cpp_rules.mk

# Source the file to define the test exe rules
# TODO: Enable this back. Seeing an segmentation error when turn on this in SUSE 12
# include $(THIS_MAKEFILE_DIR)/test_exe_rules.mk

##############################################################################
# Define the GCC with asan and ubsan target with optimization 0 for coverage
##############################################################################
# Clear any existing variables and initalize them
include $(THIS_MAKEFILE_DIR)/init_module.mk

# Compilation variables for this module
COMPILER := gcc
MODULE_NAME := zero_opt

# Suppress unused function and unused parameter warnings
WARNING_EXCEPTIONS := -Wno-unused-function -Wno-unused-parameter

# Use no optimizzations
CPP_OPT := -O0

# Add flags to generate Gcov reports
CPPFLAGS += -fprofile-arcs -ftest-coverage
LDFLAGS += -fprofile-arcs

# Always enable address sanitizer to catch array out of bounds and other such violations
# Set the initial link library to asan to ensure it is first in the link list. This is
# required by asan
CPPFLAGS += -fsanitize=address
INITIAL_LINK_LIBRARY := -lasan 

# Option to enable Undefined Behaviour sanitizer
ifeq ($(USE_UBSAN),1)
	CPPFLAGS += -fsanitize=undefined
	LDFLAGS += -lubsan
endif

# By default cause the test exes to exit if a sanitizer error is found
ifneq ($(SANITIZER_RECOVERY),1)
	CPPFLAGS += -fno-sanitize-recover=all
endif


ifeq ($(USE_UBSAN),1)
	CPPFLAGS += -fsanitize=undefined
	LDFLAGS += -lubsan
endif

# Disable the autorun rule generation. This is because we don't want to
# execute the no optimization versions of the unit test by default
ENABLE_RUN_RULES := 0

# Source the file to define the cpp rules
include $(THIS_MAKEFILE_DIR)/cpp_rules.mk

# Source the file to define the test exe rules
include $(THIS_MAKEFILE_DIR)/test_exe_rules.mk

# Additional rules for creating coverage on the O0 module
COV_DIR := ..
CODE_DIR := $(COV_DIR)/code/inc
COV_REPORT := coverage_report

# Initialize coverage data
.PHONY: init-coverage
init-coverage : build
	lcov --initial --directory $(COV_DIR) --capture

run-coverage-exes : init-coverage
	LD_LIBRARY_PATH=$(GCC_ROOTDIR)/lib64:$(BOOST_ROOT_DIR)/lib ./zero_opt_main
	LD_LIBRARY_PATH=$(GCC_ROOTDIR)/lib64:$(BOOST_ROOT_DIR)/lib ./zero_opt_main_no_bsp_mock
	LD_LIBRARY_PATH=$(GCC_ROOTDIR)/lib64:$(BOOST_ROOT_DIR)/lib ./zero_opt_main_t2

.PHONY: test-coverage
test-coverage : run-coverage-exes
	lcov --directory $(COV_DIR) --capture --output-file test_coverage.lcov
# Exclude coverage on gtest libs and standard libs
	lcov -r test_coverage.lcov '*google_gtest*' '*/boost/*' '*/usr/*' '*/gcc/*' '*test/*' -o test_coverage.lcov
	genhtml --ignore-errors source --highlight --legend test_coverage.lcov -o $(COV_REPORT)
	@echo "[SUCCESS] Generated a coverage report: $(COV_REPORT)"
	@echo "[SUCCESS] Use your favourite browser to view the report." 

##############################################################################
# Define the clang with address sanitizer, ubsan  target
##############################################################################
# Clear any existing variables and initalize them
include $(THIS_MAKEFILE_DIR)/init_module.mk

# Compilation variables for this module
COMPILER := clang
MODULE_NAME := clang_asan_ubsan

# Suppress unused function and unused parameter warnings
WARNING_EXCEPTIONS := -Wno-unused-function -Wno-unused-parameter -Wno-null-pointer-arithmetic -Wno-logical-not-parentheses

# Use max optimizations
CPP_OPT := -O3

# Enable the address sanitizer and UB sanitizer
ifeq ($(USE_UBSAN),1)
	CPPFLAGS += -fsanitize=address,undefined
	LDFLAGS += -fsanitize=address,undefined
else
	CPPFLAGS += -fsanitize=address
	LDFLAGS += -fsanitize=address
endif

# By default cause the test exes to exit if a sanitizer error is found
ifneq ($(SANITIZER_RECOVERY),1)
	CPPFLAGS += -fno-sanitize-recover=all
endif

# Source the file to define the cpp rules
include $(THIS_MAKEFILE_DIR)/cpp_rules.mk

# Source the file to define the test exe rules
include $(THIS_MAKEFILE_DIR)/test_exe_rules.mk

##############################################################################
# Define the clang with control flow sanitizer target
##############################################################################
ifeq ($(ENABLE_CLANG_CFSAN),1)

# Clear any existing variables and initalize them
include $(THIS_MAKEFILE_DIR)/init_module.mk

# Compilation variables for this module
COMPILER := clang
MODULE_NAME := clang_cfsan

# Suppress unused function and unused parameter warnings
WARNING_EXCEPTIONS := -Wno-unused-function -Wno-unused-parameter -Wno-null-pointer-arithmetic -Wno-logical-not-parentheses

# Use max optimizations
CPP_OPT := -O3

# Enable the control flow sanitizer. This also requires setting visability to hidden and link time optimization (LTO)
CPPFLAGS += -fsanitize=cfi -fvisibility=hidden -flto
LDFLAGS += -fsanitize=cfi -flto

# By default cause the test exes to exit if a sanitizer error is found
ifneq ($(SANITIZER_RECOVERY),1)
	CPPFLAGS += -fno-sanitize-recover=all
endif

# Source the file to define the cpp rules
include $(THIS_MAKEFILE_DIR)/cpp_rules.mk

# Source the file to define the test exe rules
include $(THIS_MAKEFILE_DIR)/test_exe_rules.mk

endif #ENABLE_CLANG_CFSAN

##############################################################################
# Define the clang with safe stack sanitizer target
##############################################################################

# Clear any existing variables and initalize them
include $(THIS_MAKEFILE_DIR)/init_module.mk

# Compilation variables for this module
COMPILER := clang
MODULE_NAME := clang_stacksan

# Suppress unused function and unused parameter warnings
WARNING_EXCEPTIONS := -Wno-unused-function -Wno-unused-parameter -Wno-null-pointer-arithmetic -Wno-logical-not-parentheses

# Use max optimizations
CPP_OPT := -O3

# Enable the safe stack sanitizer
CPPFLAGS += -fsanitize=safe-stack
LDFLAGS += -fsanitize=safe-stack

# By default cause the test exes to exit if a sanitizer error is found
ifneq ($(SANITIZER_RECOVERY),1)
	CPPFLAGS += -fno-sanitize-recover=all
endif

# Source the file to define the cpp rules
include $(THIS_MAKEFILE_DIR)/cpp_rules.mk

# Source the file to define the test exe rules
include $(THIS_MAKEFILE_DIR)/test_exe_rules.mk

##############################################################################
# Define the clang with thread sanitizer target
##############################################################################
ifeq ($(ENABLE_CLANG_TSAN),1)

# Clear any existing variables and initalize them
include $(THIS_MAKEFILE_DIR)/init_module.mk

# Compilation variables for this module
COMPILER := clang
MODULE_NAME := clang_tsan

# Suppress unused function and unused parameter warnings
WARNING_EXCEPTIONS := -Wno-unused-function -Wno-unused-parameter -Wno-null-pointer-arithmetic -Wno-logical-not-parentheses

# Use max optimizations
CPP_OPT := -O3

# Enable the thread sanitizer
CPPFLAGS += -fsanitize=thread
LDFLAGS += -fsanitize=thread

# By default cause the test exes to exit if a sanitizer error is found
ifneq ($(SANITIZER_RECOVERY),1)
	CPPFLAGS += -fno-sanitize-recover=all
endif

# Source the file to define the cpp rules
include $(THIS_MAKEFILE_DIR)/cpp_rules.mk

# Source the file to define the test exe rules
include $(THIS_MAKEFILE_DIR)/test_exe_rules.mk

endif #ENABLE_CLANG_TSAN

##############################################################################
# Define the clang with memory sanitizer target
##############################################################################
ifeq ($(ENABLE_CLANG_MSAN),1)
# Clear any existing variables and initalize them
include $(THIS_MAKEFILE_DIR)/init_module.mk

# Compilation variables for this module
COMPILER := clang
MODULE_NAME := clang_msan

# Suppress unused function and unused parameter warnings
WARNING_EXCEPTIONS := -Wno-unused-function -Wno-unused-parameter -Wno-null-pointer-arithmetic -Wno-logical-not-parentheses

# Use max optimizations
CPP_OPT := -O3

# Enable the memory sanitizer
CPPFLAGS += -fsanitize=memory
LDFLAGS += -fsanitize=memory

# By default cause the test exes to exit if a sanitizer error is found
ifneq ($(SANITIZER_RECOVERY),1)
	CPPFLAGS += -fno-sanitize-recover=all
endif

# Source the file to define the cpp rules
include $(THIS_MAKEFILE_DIR)/cpp_rules.mk

# Source the file to define the test exe rules
include $(THIS_MAKEFILE_DIR)/test_exe_rules.mk

endif #ENABLE_CLANG_MSAN

##############################################################################
# Define the clang with libfuzzer targets
# Based on https://github.com/google/fuzzer-test-suite/blob/master/tutorial/libFuzzerTutorial.md
##############################################################################
ifeq ($(ENABLE_FUZZER),1)
# Clear any existing variables and initalize them
include $(THIS_MAKEFILE_DIR)/init_module.mk

# Compilation variables for this module
COMPILER := clang
MODULE_NAME := libfuzz

# Suppress unused function and unused parameter warnings
WARNING_EXCEPTIONS := -Wno-unused-function -Wno-unused-parameter -Wno-null-pointer-arithmetic -Wno-logical-not-parentheses

# Use max optimizations
CPP_OPT := -O3

# Enable the safe stack sanitizer
CPPFLAGS += -fsanitize=address,fuzzer
LDFLAGS += -fsanitize=address,fuzzer

# By default cause the test exes to exit if a sanitizer error is found
ifneq ($(SANITIZER_RECOVERY),1)
	CPPFLAGS += -fno-sanitize-recover=all
endif

# Source the file to define the cpp rules
include $(THIS_MAKEFILE_DIR)/cpp_rules.mk

# Create all the targets for fuzzing. The parameter passed is the 
# name of the file and the target. Each target is compiled with all
# $(MAIN_SYSTEM_OBJS_LIST)
$(eval $(call create_fuzz_target,fuzz_main))

endif #ENABLE_FUZZER

##############################################################################
# Define the PFM decoder EXE
##############################################################################
# Clear any existing variables and initalize them
include $(THIS_MAKEFILE_DIR)/init_module.mk

# Compilation variables for this module
COMPILER := gcc
MODULE_NAME := pfm_decoder

# Suppress unused function and unused parameter warnings
WARNING_EXCEPTIONS := -Wno-unused-function -Wno-unused-parameter

# Use max optimizations
CPP_OPT := -O3

# Source the file to define the cpp rules
include $(THIS_MAKEFILE_DIR)/cpp_rules.mk

# Create all the targets for fuzzing. The parameter passed is the 
# name of the file and the target. Each target is compiled with all
# $(MAIN_SYSTEM_OBJS_LIST)
$(eval $(call create_exe_target,pfm_decoder))



##############################################################################
# Final targets. Must appear at end of file
##############################################################################
.PHONY: build
build : $(TEST_EXE_NAMES)











